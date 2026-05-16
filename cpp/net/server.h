#pragma once
#include <string>
#include <functional>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <queue>

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
    std::string host         = "0.0.0.0";
    int         port         = 9851;
    int         backlog      = 128;
    size_t      max_clients  = 10000;
    size_t      worker_threads = 4;
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
    size_t workerCount() const;

private:
    ServerConfig    config_;
    RequestHandler  handler_;
    int             listen_fd_ = -1;
    std::atomic<bool> running_{false};
    std::thread     accept_thread_;

    mutable std::mutex                          clients_mu_;
    std::unordered_map<uint64_t, ClientConn>   clients_;
    uint64_t                                    next_client_id_ = 1;

    // Thread pool
    std::vector<std::thread>                    worker_threads_;
    std::mutex                                  queue_mu_;
    std::condition_variable                     queue_cv_;
    std::queue<uint64_t>                        work_queue_;

    void acceptLoop();
    void workerLoop();
    void handleClient(uint64_t client_id);
    bool setNonBlocking(int fd);
    bool setKeepAlive(int fd);
    bool setTcpNoDelay(int fd);
    bool sendAll(int fd, const char* data, size_t len);
    std::string processRequest(const std::string& raw, ClientConn& conn);
};

} // namespace net
} // namespace spatialdb
