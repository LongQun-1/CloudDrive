#ifndef CLOUD_DRIVE_REDIS_PLUGIN_H
#define CLOUD_DRIVE_REDIS_PLUGIN_H

#include <drogon/plugins/Plugin.h>
#include <hiredis/hiredis.h>
#include <string>
#include <mutex>
#include <vector>

namespace CloudDrive {

class RedisPlugin : public drogon::Plugin<RedisPlugin> {
public:
    void initAndStart(const Json::Value& config) override;
    void shutdown() override;
    
    // Basic Redis operations
    bool set(const std::string& key, const std::string& value);
    bool setex(const std::string& key, int seconds, const std::string& value);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    long incr(const std::string& key);
    long decr(const std::string& key);
    bool expire(const std::string& key, int seconds);
    
    // Set operations
    bool setBit(const std::string& key, size_t offset, int value);
    int getBit(const std::string& key, size_t offset);
    long bitCount(const std::string& key);
    
    // Lock operations
    bool lock(const std::string& key, int ttlSeconds = 10);
    bool unlock(const std::string& key);
    
    // Connection status
    bool isConnected() const;
    
private:
    redisContext* getContext();
    void reconnect();
    
    std::string host_;
    int port_;
    redisContext* context_{nullptr};
    std::mutex mutex_;
    bool connected_{false};
};

} // namespace CloudDrive

#endif
