#include "AuthMiddleware.h"
#include "services/AuthService.h"
#include "utils/ResponseUtil.h"
#include <drogon/drogon.h>

namespace CloudDrive {

void AuthMiddleware::invoke(const drogon::HttpRequestPtr& req,
                            drogon::MiddlewareNextCallback&& nextCb,
                            drogon::MiddlewareCallback&& mcb) {
    // Skip OPTIONS requests (CORS preflight)
    if (req->method() == drogon::Options) {
        nextCb(std::move(mcb));
        return;
    }

    std::string token = extractToken(req);
    if (token.empty()) {
        auto resp = ResponseUtil::error(1002, "missing authorization token");
        mcb(resp);
        return;
    }

    AuthService authService;
    auto payload = authService.verifyAccessToken(token);

    if (payload.isNull()) {
        auto resp = ResponseUtil::error(1002, "invalid or expired token");
        mcb(resp);
        return;
    }

    // Store user info in request attributes for downstream controllers
    req->attributes()->insert("user_id", payload["user_id"].asInt64());
    req->attributes()->insert("username", payload["username"].asString());

    nextCb(std::move(mcb));
}

std::string AuthMiddleware::extractToken(const drogon::HttpRequestPtr& req) {
    auto authHeader = req->getHeader("Authorization");
    if (authHeader.empty()) {
        return "";
    }
    
    // Expect "Bearer <token>"
    if (authHeader.length() > 7 && authHeader.substr(0, 7) == "Bearer ") {
        return authHeader.substr(7);
    }
    
    return authHeader;
}

long AuthMiddleware::getUserId(const drogon::HttpRequestPtr& req) {
    try {
        return static_cast<long>(req->attributes()->get<int64_t>("user_id"));
    } catch (...) {
        return -1;
    }
}

} // namespace CloudDrive
