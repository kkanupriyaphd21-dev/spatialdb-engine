#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "server.h"

namespace spatialdb {
namespace net {

using MiddlewareFn = std::function<std::string(
    const std::string& cmd,
    const std::vector<std::string>& args,
    ClientConn& conn,
    const RequestHandler& next)>;

class MiddlewareChain {
public:
    MiddlewareChain& use(MiddlewareFn mw);
    RequestHandler   build(RequestHandler final_handler);

private:
    std::vector<MiddlewareFn> chain_;
};

// Built-in middleware
MiddlewareFn rateLimitMiddleware(size_t max_per_sec);
MiddlewareFn authMiddleware(const std::string& password);
MiddlewareFn loggingMiddleware();
MiddlewareFn metricsMiddleware();

} // namespace net
} // namespace spatialdb
