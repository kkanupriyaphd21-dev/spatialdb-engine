#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

namespace spatialdb {
namespace net {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> query_params;
};

struct HttpResponse {
    int         status  = 200;
    std::string body;
    std::unordered_map<std::string, std::string> headers;

    static HttpResponse ok(std::string body, std::string ct = "application/json") {
        return {200, std::move(body), {{"Content-Type", ct}}};
    }
    static HttpResponse notFound() {
        return {404, "{\"error\":\"not found\"}", {{"Content-Type","application/json"}}};
    }
    static HttpResponse badRequest(std::string msg) {
        return {400, "{\"error\":\"" + msg + "\"}", {{"Content-Type","application/json"}}};
    }
    static HttpResponse internalError() {
        return {500, "{\"error\":\"internal error\"}", {{"Content-Type","application/json"}}};
    }
};

using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

struct Route {
    std::string  method;
    std::string  path;
    RouteHandler handler;
};

class HttpRouter {
public:
    HttpRouter& GET(std::string path, RouteHandler h);
    HttpRouter& POST(std::string path, RouteHandler h);
    HttpRouter& DELETE(std::string path, RouteHandler h);

    HttpResponse dispatch(const HttpRequest& req) const;
    HttpRequest  parseRequest(const std::string& raw) const;
    std::string  serializeResponse(const HttpResponse& resp) const;

private:
    std::vector<Route> routes_;
    bool matchPath(const std::string& pattern, const std::string& actual,
                   std::unordered_map<std::string, std::string>& params) const;
};

} // namespace net
} // namespace spatialdb
