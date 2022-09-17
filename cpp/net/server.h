#pragma once
#include <string>
#include <functional>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace spatialdb {
namespace net {

struct ClientConn {
    int         fd;
    std::string addr;
    std::string read_buf;
    std::string write_buf;
    bool        closed = false;
    uint64_t    id;
};

using RequestHandler = std::function<std::string(const std::string& cmd,
                                                  const std::vector<std::string>& args,
                                                  ClientConn& conn)>;

struct ServerConfig {
    std::string host        = "0.0.0.0";
    int         port        = 9851;
    int         backlog     = 128;
    size_t      max_clients = 10000;
    int         read_timeout_ms  = 5000;
    int         write_timeout_ms = 5000;
};

class TCPServer {
public:
    explicit TCPServer(ServerConfig config);
    ~TCPServer();

    void setHandler(RequestHandler handler);
    bool start();
    void stop();
    bool isRunning() const { return running_.load(); }

    size_t clientCount() const;

private:
    ServerConfig    config_;
    RequestHandler  handler_;
    int             listen_fd_ = -1;
    std::atomic<bool> running_{false};
    std::thread     accept_thread_;

    mutable std::mutex                          clients_mu_;
    std::unordered_map<uint64_t, ClientConn>   clients_;
    std::vector<std::thread>                    client_threads_;
    uint64_t                                    next_client_id_ = 1;

    void acceptLoop();
    void handleClient(uint64_t client_id);
    bool setNonBlocking(int fd);
    bool setKeepAlive(int fd);
    std::string processRequest(const std::string& raw, ClientConn& conn);
};

} // namespace net
} // namespace spatialdb
