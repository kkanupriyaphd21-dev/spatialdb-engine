#include "http_handler.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace spatialdb {
namespace net {

HttpRouter& HttpRouter::GET(std::string path, RouteHandler h) {
    routes_.push_back({"GET", std::move(path), std::move(h)});
    return *this;
}
HttpRouter& HttpRouter::POST(std::string path, RouteHandler h) {
    routes_.push_back({"POST", std::move(path), std::move(h)});
    return *this;
}
HttpRouter& HttpRouter::DELETE(std::string path, RouteHandler h) {
    routes_.push_back({"DELETE", std::move(path), std::move(h)});
    return *this;
}

bool HttpRouter::matchPath(const std::string& pattern, const std::string& actual,
                             std::unordered_map<std::string, std::string>& params) const {
    // simple segment matching: /foo/:id/bar
    auto split = [](const std::string& s, char delim) {
        std::vector<std::string> out;
        std::istringstream ss(s);
        std::string part;
        while (std::getline(ss, part, delim)) {
            if (!part.empty()) out.push_back(part);
        }
        return out;
    };

    auto pparts = split(pattern, '/');
    auto aparts = split(actual, '/');
    if (pparts.size() != aparts.size()) return false;

    for (size_t i = 0; i < pparts.size(); ++i) {
        if (pparts[i][0] == ':') {
            params[pparts[i].substr(1)] = aparts[i];
        } else if (pparts[i] != aparts[i]) {
            return false;
        }
    }
    return true;
}

HttpResponse HttpRouter::dispatch(const HttpRequest& req) const {
    for (const auto& route : routes_) {
        if (route.method != req.method) continue;
        std::unordered_map<std::string, std::string> params;
        if (matchPath(route.path, req.path, params)) {
            HttpRequest enriched = req;
            for (auto& [k, v] : params) enriched.query_params[k] = v;
            try {
                return route.handler(enriched);
            } catch (...) {
                return HttpResponse::internalError();
            }
        }
    }
    return HttpResponse::notFound();
}

HttpRequest HttpRouter::parseRequest(const std::string& raw) const {
    HttpRequest req;
    std::istringstream ss(raw);
    std::string line;

    // request line
    if (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream ls(line);
        ls >> req.method >> req.path;
    }

    // headers
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            auto key = line.substr(0, colon);
            auto val = line.substr(colon + 2);
            req.headers[key] = val;
        }
    }

    // body
    while (std::getline(ss, line)) {
        req.body += line + "\n";
    }

    return req;
}

std::string HttpRouter::serializeResponse(const HttpResponse& resp) const {
    std::ostringstream ss;
    ss << "HTTP/1.1 " << resp.status << " ";

    switch (resp.status) {
        case 200: ss << "OK"; break;
        case 400: ss << "Bad Request"; break;
        case 404: ss << "Not Found"; break;
        case 500: ss << "Internal Server Error"; break;
        default:  ss << "Unknown"; break;
    }
    ss << "\r\n";

    for (const auto& [k, v] : resp.headers) {
        ss << k << ": " << v << "\r\n";
    }
    ss << "Content-Length: " << resp.body.size() << "\r\n";
    ss << "\r\n";
    ss << resp.body;
    return ss.str();
}

} // namespace net
} // namespace spatialdb
