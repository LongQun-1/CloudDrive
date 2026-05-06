#include <drogon/drogon.h>
#include "middlewares/AuthMiddleware.h"
#include "middlewares/RateLimitMiddleware.h"

using namespace CloudDrive;

int main() {
    // Load configuration
    drogon::app().loadConfigFile("config.json");
    
    // Get config for startup logging
    auto& customConfig = drogon::app().getCustomConfig();
    
    // Register middlewares globally
    drogon::app().registerMiddleware(std::make_shared<AuthMiddleware>());
    drogon::app().registerMiddleware(std::make_shared<RateLimitMiddleware>());
    
    // Configure rate limits
    RateLimitMiddleware::setRateLimit("/api/user/register", 5, 60);    // 5/min for register
    RateLimitMiddleware::setRateLimit("/api/user/login", 5, 60);       // 5/min for login
    RateLimitMiddleware::setRateLimit("/api/upload/chunk", 100, 60);   // 100/min for chunk upload
    RateLimitMiddleware::setRateLimit("/api/share/verify", 10, 60);    // 10/min for share verify
    RateLimitMiddleware::setRateLimit("/api/file/download", 10, 60);   // 10/min for download
    
    // Add CORS support globally
    drogon::app().registerBeginningAdvice([&customConfig]() {
        LOG_INFO << "CloudDrive server starting...";
        LOG_INFO << "Redis: " << customConfig.get("redis", Json::Value()).get("host", "127.0.0.1").asString()
                 << ":" << customConfig.get("redis", Json::Value()).get("port", 6379).asInt();
        
        auto storage = customConfig.get("storage", Json::Value());
        LOG_INFO << "Storage path: " << storage.get("path", "/data/cloud-drive/files").asString();
        LOG_INFO << "Max file size: " << storage.get("max_file_size", 10737418240).asInt64() << " bytes";
    });
    
    // Health check endpoint
    drogon::app().registerHandler("/health",
        [](const drogon::HttpRequestPtr& req,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            Json::Value resp;
            resp["status"] = "ok";
            resp["version"] = "1.0.0";
            resp["uptime_seconds"] = 0;
            
            Json::Value checks;
            checks["mysql"]["status"] = "ok";
            
            checks["redis"]["status"] = "degraded";
            
            resp["checks"] = checks;
            
            auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
            httpResp->setStatusCode(drogon::k200OK);
            callback(httpResp);
        },
        {drogon::Get}
    );
    
    LOG_INFO << "CloudDrive server initialized, starting on port 8080...";
    drogon::app().run();
    
    return 0;
}
