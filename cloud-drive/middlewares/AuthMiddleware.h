#ifndef CLOUD_DRIVE_AUTH_MIDDLEWARE_H
#define CLOUD_DRIVE_AUTH_MIDDLEWARE_H

#include <drogon/HttpMiddleware.h>
#include <string>
#include <vector>

namespace CloudDrive {

class AuthMiddleware : public drogon::HttpMiddleware<AuthMiddleware, false> {
public:
    void invoke(const drogon::HttpRequestPtr& req,
                drogon::MiddlewareNextCallback&& nextCb,
                drogon::MiddlewareCallback&& mcb) override;
    
    // Parse token from Authorization header
    static std::string extractToken(const drogon::HttpRequestPtr& req);
    
    // Get authenticated user ID from request attributes
    static long getUserId(const drogon::HttpRequestPtr& req);
};

} // namespace CloudDrive

#endif
