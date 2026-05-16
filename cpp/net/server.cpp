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
#include <queue>
#include <condition_variable>

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

bool TCPServer::setTcpNoDelay(int fd) {
    int yes = 1;
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) == 0;
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
        listen_fd_ = -1;
        return false;
    }

    if (listen(listen_fd_, config_.backlog) < 0) {
        std::cerr << "listen() failed: " << strerror(errno) << "\n";
        close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }

    running_ = true;

    // Start worker thread pool
    for (size_t i = 0; i < config_.worker_threads; i++) {
        worker_threads_.emplace_back([this]() { workerLoop(); });
    }

    // Start accept thread
    accept_thread_ = std::thread(&TCPServer::acceptLoop, this);

    return true;
}

void TCPServer::acceptLoop() {
    while (running_ && !shutting_down_) {
        sockaddr_in client_addr{};
        socklen_t   addr_len = sizeof(client_addr);

        int client_fd = accept(listen_fd_,
                               reinterpret_cast<sockaddr*>(&client_addr),
                               &addr_len);
        if (client_fd < 0) {
            if (errno == EINTR || errno == EWOULDBLOCK) continue;
            if (!running_) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Enforce connection limit
        {
            std::lock_guard<std::mutex> lock(clients_mu_);
            if (clients_.size() >= config_.max_clients) {
                const char* msg = "-ERR too many connections\r\n";
                send(client_fd, msg, strlen(msg), 0);
                close(client_fd);
                continue;
            }
        }

        setKeepAlive(client_fd);
        setTcpNoDelay(client_fd);

        char ip_buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf));

        uint64_t cid;
        {
            std::lock_guard<std::mutex> lock(clients_mu_);
            cid = next_client_id_++;
            clients_.emplace(cid, ClientConn{client_fd, ip_buf, "", "", false, cid});
        }

        // Dispatch to worker pool via queue
        {
            std::lock_guard<std::mutex> lock(queue_mu_);
            work_queue_.push(cid);
            queue_cv_.notify_one();
        }
    }
}

void TCPServer::workerLoop() {
    while (running_ || !work_queue_.empty()) {
        uint64_t cid;
        {
            std::unique_lock<std::mutex> lock(queue_mu_);
            queue_cv_.wait(lock, [this]() {
                return !work_queue_.empty() || !running_;
            });
            if (!running_ && work_queue_.empty()) return;
            cid = work_queue_.front();
            work_queue_.pop();
        }

        handleClient(cid);
    }
}

void TCPServer::handleClient(uint64_t client_id) {
    char buf[4096];

    while (running_) {
        ClientConn* conn = nullptr;
        {
            std::lock_guard<std::mutex> lock(clients_mu_);
            auto it = clients_.find(client_id);
            if (it == clients_.end()) return;
            conn = &it->second;
        }

        ssize_t n = recv(conn->fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            if (n < 0 && (errno == EINTR || errno == EAGAIN)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            break;
        }

        buf[n] = '\0';
        conn->read_buf.append(buf, static_cast<size_t>(n));

        auto response = processRequest(conn->read_buf, *conn);
        conn->read_buf.clear();

        if (!response.empty()) {
            sendAll(conn->fd, response.data(), response.size());
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

bool TCPServer::sendAll(int fd, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, data + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}

std::string TCPServer::processRequest(const std::string& raw, ClientConn& conn) {
    if (!handler_) return "-ERR no handler\r\n";

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
    queue_cv_.notify_all();

    if (listen_fd_ >= 0) {
        close(listen_fd_);
        listen_fd_ = -1;
    }

    if (accept_thread_.joinable()) accept_thread_.join();
    for (auto& t : worker_threads_) {
        if (t.joinable()) t.join();
    }

    // Close remaining client connections
    std::lock_guard<std::mutex> lock(clients_mu_);
    for (auto& [id, conn] : clients_) {
        close(conn.fd);
    }
    clients_.clear();
}

void TCPServer::shutdown() {
    if (shutting_down_.exchange(true)) return; // already shutting down

    std::cout << "Graceful shutdown initiated, draining " << clientCount() << " connections...\n";

    // Stop accepting new connections
    if (listen_fd_ >= 0) {
        close(listen_fd_);
        listen_fd_ = -1;
    }

    // Wait for active connections to drain or timeout
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(config_.drain_timeout_ms);

    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lock(clients_mu_);
            if (clients_.empty()) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Force close remaining connections
    size_t remaining = 0;
    {
        std::lock_guard<std::mutex> lock(clients_mu_);
        remaining = clients_.size();
        for (auto& [id, conn] : clients_) {
            close(conn.fd);
        }
        clients_.clear();
    }
    if (remaining > 0) {
        std::cout << "Force closed " << remaining << " remaining connections\n";
    }

    // Stop workers
    running_ = false;
    queue_cv_.notify_all();

    if (accept_thread_.joinable()) accept_thread_.join();
    for (auto& t : worker_threads_) {
        if (t.joinable()) t.join();
    }

    std::cout << "Server shutdown complete\n";
}

size_t TCPServer::clientCount() const {
    std::lock_guard<std::mutex> lock(clients_mu_);
    return clients_.size();
}

size_t TCPServer::workerCount() const {
    return config_.worker_threads;
}

} // namespace net
} // namespace spatialdb
