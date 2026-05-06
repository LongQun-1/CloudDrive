#include "RateLimitMiddleware.h"
#include "plugins/RedisPlugin.h"
#include "utils/ResponseUtil.h"
#include <drogon/drogon.h>
#include <vector>

namespace CloudDrive {

std::unordered_map<std::string, RateLimitMiddleware::RateLimitConfig> RateLimitMiddleware::configs_;

void RateLimitMiddleware::setRateLimit(const std::string& pathPrefix, int maxRequests, int windowSeconds) {
    configs_[pathPrefix] = {maxRequests, windowSeconds};
}

void RateLimitMiddleware::invoke(const drogon::HttpRequestPtr& req,
                                  drogon::MiddlewareNextCallback&& nextCb,
                                  drogon::MiddlewareCallback&& mcb) {
    // Skip OPTIONS requests
    if (req->method() == drogon::Options) {
        nextCb(std::move(mcb));
        return;
    }
    
    // Get client IP
    std::string clientIp = req->getHeader("X-Real-IP");
    if (clientIp.empty()) {
        clientIp = req->getHeader("X-Forwarded-For");
        if (!clientIp.empty()) {
            auto pos = clientIp.find(',');
            if (pos != std::string::npos) {
                clientIp = clientIp.substr(0, pos);
            }
        }
    }
    if (clientIp.empty()) {
        clientIp = req->getPeerAddr().toIp();
    }
    
    // Get request path
    std::string path = req->path();
    
    // Find matching rate limit config
    int maxRequests = 60;  // default: 60/min
    int windowSeconds = 60;
    
    for (const auto& [prefix, config] : configs_) {
        if (path.find(prefix) == 0) {
            maxRequests = config.maxRequests;
            windowSeconds = config.windowSeconds;
            break;
        }
    }
    
    // Check rate limit via Redis
    auto redis = drogon::app().getPlugin<RedisPlugin>();
    if (redis && redis->isConnected()) {
        std::string rateKey = "rate_limit:" + clientIp + ":" + path;
        
        long count = redis->incr(rateKey);
        if (count == 1) {
            redis->expire(rateKey, windowSeconds);
        }
        
        if (count > maxRequests) {
            auto resp = ResponseUtil::error(1006, "request too frequent, please try again later");
            mcb(resp);
            return;
        }
    }
    
    nextCb(std::move(mcb));
}

} // namespace CloudDrive
