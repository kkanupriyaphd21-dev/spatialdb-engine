#include "../net/server.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace spatialdb::net;

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void name(); void name()
#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        std::cerr << "FAILED: " << #a << " != " << #b \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        tests_failed++; return; \
    } \
} while(0)
#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        std::cerr << "FAILED: " << #x << " is false" \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        tests_failed++; return; \
    } \
} while(0)
#define ASSERT_FALSE(x) ASSERT_TRUE(!(x))
#define RUN_TEST(name) do { \
    std::cout << "  Running " << #name << "... "; \
    name(); \
    std::cout << "OK" << std::endl; \
    tests_passed++; \
} while(0)

static int connectToServer(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    return sock;
}

static void sendRequest(int sock, const std::string& req) {
    send(sock, req.c_str(), req.size(), 0);
}

static std::string readResponse(int sock, int timeout_ms = 1000) {
    char buf[4096];
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    if (select(sock + 1, &fds, nullptr, nullptr, &tv) > 0) {
        ssize_t n = recv(sock, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            buf[n] = '\0';
            return std::string(buf, n);
        }
    }
    return "";
}

TEST(Server_StartStop) {
    ServerConfig cfg;
    cfg.port = 19851;
    cfg.worker_threads = 2;
    cfg.max_clients = 100;

    TCPServer server(cfg);
    server.setHandler([](const std::string& cmd, const std::vector<std::string>&, ClientConn&) {
        return "+OK\r\n";
    });

    ASSERT_TRUE(server.start());
    ASSERT_TRUE(server.isRunning());
    ASSERT_EQ(server.workerCount(), 2u);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    server.stop();
    ASSERT_FALSE(server.isRunning());
}

TEST(Server_HandleRequest) {
    ServerConfig cfg;
    cfg.port = 19852;
    cfg.worker_threads = 2;
    cfg.max_clients = 100;

    TCPServer server(cfg);
    server.setHandler([](const std::string& cmd, const std::vector<std::string>&, ClientConn&) {
        if (cmd == "PING") return "+PONG\r\n";
        return "-ERR unknown command\r\n";
    });

    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int sock = connectToServer(19852);
    ASSERT_TRUE(sock > 0);

    sendRequest(sock, "PING\r\n");
    std::string resp = readResponse(sock);
    ASSERT_EQ(resp, "+PONG\r\n");

    close(sock);
    server.stop();
}

TEST(Server_ConnectionLimit) {
    ServerConfig cfg;
    cfg.port = 19853;
    cfg.worker_threads = 2;
    cfg.max_clients = 2;

    TCPServer server(cfg);
    server.setHandler([](const std::string&, const std::vector<std::string>&, ClientConn&) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return "+OK\r\n";
    });

    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int sock1 = connectToServer(19853);
    int sock2 = connectToServer(19853);
    ASSERT_TRUE(sock1 > 0);
    ASSERT_TRUE(sock2 > 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    int sock3 = connectToServer(19853);
    ASSERT_TRUE(sock3 > 0);

    std::string resp = readResponse(sock3, 2000);
    ASSERT_TRUE(resp.find("too many connections") != std::string::npos);

    close(sock1);
    close(sock2);
    close(sock3);
    server.stop();
}

TEST(Server_ClientCount) {
    ServerConfig cfg;
    cfg.port = 19854;
    cfg.worker_threads = 2;
    cfg.max_clients = 100;

    TCPServer server(cfg);
    server.setHandler([](const std::string&, const std::vector<std::string>&, ClientConn& conn) {
        conn.closed = true;
        return "+OK\r\n";
    });

    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_EQ(server.clientCount(), 0u);

    int sock = connectToServer(19854);
    ASSERT_TRUE(sock > 0);
    sendRequest(sock, "TEST\r\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    close(sock);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(server.clientCount(), 0u);

    server.stop();
}

TEST(Server_MultipleWorkers) {
    ServerConfig cfg;
    cfg.port = 19855;
    cfg.worker_threads = 8;
    cfg.max_clients = 100;

    TCPServer server(cfg);
    server.setHandler([](const std::string& cmd, const std::vector<std::string>&, ClientConn&) {
        return "+OK " + cmd + "\r\n";
    });

    ASSERT_TRUE(server.start());
    ASSERT_EQ(server.workerCount(), 8u);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<int> socks;
    for (int i = 0; i < 5; i++) {
        int sock = connectToServer(19855);
        ASSERT_TRUE(sock > 0);
        socks.push_back(sock);
    }

    for (int i = 0; i < 5; i++) {
        sendRequest(socks[i], "ECHO\r\n");
    }

    for (int i = 0; i < 5; i++) {
        std::string resp = readResponse(socks[i]);
        ASSERT_EQ(resp, "+OK ECHO\r\n");
        close(socks[i]);
    }

    server.stop();
}

int main() {
    std::cout << "TCPServer Tests" << std::endl;
    std::cout << "===============" << std::endl;

    RUN_TEST(Server_StartStop);
    RUN_TEST(Server_HandleRequest);
    RUN_TEST(Server_ConnectionLimit);
    RUN_TEST(Server_ClientCount);
    RUN_TEST(Server_MultipleWorkers);

    std::cout << std::endl;
    std::cout << "Results: " << tests_passed << " passed, "
              << tests_failed << " failed" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
