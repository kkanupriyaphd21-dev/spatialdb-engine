#include "server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace spatialdb {
namespace net {

TCPServer::TCPServer(ServerConfig config) : config_(std::move(config)) {}

TCPServer::~TCPServer() {
    stop();
}

void TCPServer::setHandler(RequestHandler handler) {
    handler_ = std::move(handler);
}

bool TCPServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

bool TCPServer::setKeepAlive(int fd) {
    int yes = 1;
    return setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == 0;
}

bool TCPServer::start() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        std::cerr << "socket() failed: " << strerror(errno) << "\n";
        return false;
    }

    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(config_.port);
    inet_pton(AF_INET, config_.host.c_str(), &addr.sin_addr);

    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind() failed: " << strerror(errno) << "\n";
        close(listen_fd_);
        return false;
    }

    if (listen(listen_fd_, config_.backlog) < 0) {
        std::cerr << "listen() failed: " << strerror(errno) << "\n";
        close(listen_fd_);
        return false;
    }

    running_ = true;
    accept_thread_ = std::thread(&TCPServer::acceptLoop, this);
    std::cout << "TCPServer listening on " << config_.host << ":" << config_.port << "\n";
    return true;
}

void TCPServer::acceptLoop() {
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t   addr_len = sizeof(client_addr);

        int client_fd = accept(listen_fd_,
                               reinterpret_cast<sockaddr*>(&client_addr),
                               &addr_len);
        if (client_fd < 0) {
            if (errno == EINTR || errno == EWOULDBLOCK) continue;
            if (!running_) break;
            continue;
        }

        setKeepAlive(client_fd);

        char ip_buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf));

        uint64_t cid;
        {
            std::lock_guard<std::mutex> lock(clients_mu_);
            cid = next_client_id_++;
            clients_.emplace(cid, ClientConn{client_fd, ip_buf, "", "", false, cid});
        }

        client_threads_.emplace_back([this, cid]() { handleClient(cid); });
    }
}

void TCPServer::handleClient(uint64_t client_id) {
    char buf[4096];

    while (running_) {
        ClientConn* conn = nullptr;
        {
            std::lock_guard<std::mutex> lock(clients_mu_);
            auto it = clients_.find(client_id);
            if (it == clients_.end()) break;
            conn = &it->second;
        }

        ssize_t n = recv(conn->fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) break;

        buf[n] = '\0';
        conn->read_buf += buf;

        auto response = processRequest(conn->read_buf, *conn);
        conn->read_buf.clear();

        if (!response.empty()) {
            send(conn->fd, response.data(), response.size(), 0);
        }

        if (conn->closed) break;
    }

    std::lock_guard<std::mutex> lock(clients_mu_);
    auto it = clients_.find(client_id);
    if (it != clients_.end()) {
        close(it->second.fd);
        clients_.erase(it);
    }
}

std::string TCPServer::processRequest(const std::string& raw, ClientConn& conn) {
    if (!handler_) return "-ERR no handler\r\n";

    // very simple line-based dispatch for now
    std::istringstream ss(raw);
    std::string line;
    if (!std::getline(ss, line)) return "";
    if (!line.empty() && line.back() == '\r') line.pop_back();

    std::istringstream words(line);
    std::string cmd;
    words >> cmd;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    std::vector<std::string> args;
    std::string arg;
    while (words >> arg) args.push_back(arg);

    return handler_(cmd, args, conn);
}

void TCPServer::stop() {
    running_ = false;
    if (listen_fd_ >= 0) {
        close(listen_fd_);
        listen_fd_ = -1;
    }
    if (accept_thread_.joinable()) accept_thread_.join();
    for (auto& t : client_threads_) {
        if (t.joinable()) t.join();
    }
}

size_t TCPServer::clientCount() const {
    std::lock_guard<std::mutex> lock(clients_mu_);
    return clients_.size();
}

} // namespace net
} // namespace spatialdb
