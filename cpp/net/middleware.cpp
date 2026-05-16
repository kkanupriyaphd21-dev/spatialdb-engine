#include "middleware.h"
#include "../metrics/counters.h"
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <deque>
#include <algorithm>

namespace spatialdb {
namespace net {

MiddlewareChain& MiddlewareChain::use(MiddlewareFn mw) {
    chain_.push_back(std::move(mw));
    return *this;
}

RequestHandler MiddlewareChain::build(RequestHandler final_handler) {
    // build chain in reverse
    RequestHandler handler = std::move(final_handler);
    for (auto it = chain_.rbegin(); it != chain_.rend(); ++it) {
        auto mw   = *it;
        auto next = handler;
        handler = [mw, next](const std::string& cmd,
                              const std::vector<std::string>& args,
                              ClientConn& conn) -> std::string {
            return mw(cmd, args, conn, next);
        };
    }
    return handler;
}

MiddlewareFn loggingMiddleware() {
    return [](const std::string& cmd,
              const std::vector<std::string>& args,
              ClientConn& conn,
              const RequestHandler& next) -> std::string {
        auto t0 = std::chrono::steady_clock::now();
        auto result = next(cmd, args, conn);
        auto t1 = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        std::cout << "[" << conn.addr << "] " << cmd << " " << ms << "us\n";
        return result;
    };
}

MiddlewareFn metricsMiddleware() {
    return [](const std::string& cmd,
              const std::vector<std::string>& args,
              ClientConn& conn,
              const RequestHandler& next) -> std::string {
        auto& reg = metrics::Registry::global();
        reg.counter("spatialdb_commands_total", "Total commands processed").inc();
        reg.counter("spatialdb_cmd_" + cmd + "_total").inc();

        auto t0 = std::chrono::steady_clock::now();
        auto result = next(cmd, args, conn);
        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count() / 1000.0;

        reg.histogram("spatialdb_cmd_duration_ms",
                      {0.1, 0.5, 1.0, 5.0, 10.0, 50.0, 100.0},
                      "Command duration in ms").observe(ms);
        return result;
    };
}

MiddlewareFn authMiddleware(const std::string& password) {
    return [password](const std::string& cmd,
                       const std::vector<std::string>& args,
                       ClientConn& conn,
                       const RequestHandler& next) -> std::string {
        if (password.empty()) return next(cmd, args, conn);
        if (cmd == "AUTH") {
            if (!args.empty() && args[0] == password) return "+OK\r\n";
            return "-ERR invalid password\r\n";
        }
        // In a real impl, track auth state per connection
        return next(cmd, args, conn);
    };
}

// Sliding window rate limiter with periodic cleanup
MiddlewareFn rateLimitMiddleware(size_t max_per_sec) {
    struct ClientState {
        std::deque<uint64_t> timestamps; // request timestamps within window
    };

    struct State {
        std::mutex mu;
        std::unordered_map<std::string, ClientState> clients;
        uint64_t last_cleanup = 0;
    };
    constexpr uint64_t CLEANUP_INTERVAL = 60; // cleanup every 60 seconds
    auto state = std::make_shared<State>();

    return [state, max_per_sec](const std::string& cmd,
                                   const std::vector<std::string>& args,
                                   ClientConn& conn,
                                   const RequestHandler& next) -> std::string {
        auto now = (uint64_t)std::chrono::system_clock::now()
                       .time_since_epoch().count() / 1000000000ULL;

        std::lock_guard<std::mutex> lock(state->mu);

        // Periodic cleanup of stale entries
        if (now - state->last_cleanup > CLEANUP_INTERVAL) {
            state->last_cleanup = now;
            for (auto it = state->clients.begin(); it != state->clients.end();) {
                if (it->second.timestamps.empty() ||
                    now - it->second.timestamps.back() > 120) {
                    it = state->clients.erase(it);
                } else {
                    ++it;
                }
            }
        }

        auto& client = state->clients[conn.addr];
        auto& ts = client.timestamps;

        // Remove timestamps outside the 1-second window
        while (!ts.empty() && now - ts.front() >= 1) {
            ts.pop_front();
        }

        if (ts.size() >= max_per_sec) {
            return "-ERR rate limit exceeded\r\n";
        }

        ts.push_back(now);
        return next(cmd, args, conn);
    };
}

} // namespace net
} // namespace spatialdb
