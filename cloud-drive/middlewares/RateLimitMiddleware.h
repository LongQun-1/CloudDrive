#ifndef CLOUD_DRIVE_RATE_LIMIT_MIDDLEWARE_H
#define CLOUD_DRIVE_RATE_LIMIT_MIDDLEWARE_H

#include <drogon/HttpMiddleware.h>
#include <string>
#include <unordered_map>

namespace CloudDrive {

class RateLimitMiddleware : public drogon::HttpMiddleware<RateLimitMiddleware, false> {
public:
    void invoke(const drogon::HttpRequestPtr& req,
                drogon::MiddlewareNextCallback&& nextCb,
                drogon::MiddlewareCallback&& mcb) override;
    
    // Configure rate limits for different path patterns
    static void setRateLimit(const std::string& pathPrefix, int maxRequests, int windowSeconds);

private:
    struct RateLimitConfig {
        int maxRequests;
        int windowSeconds;
    };
    
    static std::unordered_map<std::string, RateLimitConfig> configs_;
};

} // namespace CloudDrive

#endif
