#include "RedisPlugin.h"
#include <drogon/drogon.h>
#include <chrono>
#include <thread>

namespace CloudDrive {

void RedisPlugin::initAndStart(const Json::Value& config) {
    host_ = config.get("host", "127.0.0.1").asString();
    port_ = config.get("port", 6379).asInt();
    
    reconnect();
    
    LOG_INFO << "RedisPlugin initialized: " << host_ << ":" << port_;
}

void RedisPlugin::shutdown() {
    if (context_) {
        redisFree(context_);
        context_ = nullptr;
    }
    connected_ = false;
    LOG_INFO << "RedisPlugin shutdown";
}

void RedisPlugin::reconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (context_) {
        redisFree(context_);
        context_ = nullptr;
    }
    
    context_ = redisConnect(host_.c_str(), port_);
    if (!context_ || context_->err) {
        LOG_ERROR << "Redis connection failed: " << (context_ ? context_->errstr : "unknown");
        connected_ = false;
        return;
    }
    connected_ = true;
}

redisContext* RedisPlugin::getContext() {
    if (!connected_ || !context_) {
        reconnect();
    }
    return context_;
}

bool RedisPlugin::isConnected() const {
    return connected_ && context_ && !context_->err;
}

// ---- Basic Operations ----

bool RedisPlugin::set(const std::string& key, const std::string& value) {
    auto ctx = getContext();
    if (!ctx) return false;
    
    auto reply = (redisReply*)redisCommand(ctx, "SET %b %b", key.data(), key.size(), value.data(), value.size());
    if (!reply) { reconnect(); return false; }
    
    bool ok = (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0);
    freeReplyObject(reply);
    return ok;
}

bool RedisPlugin::setex(const std::string& key, int seconds, const std::string& value) {
    auto ctx = getContext();
    if (!ctx) return false;
    
    auto reply = (redisReply*)redisCommand(ctx, "SETEX %b %d %b", key.data(), key.size(), seconds, value.data(), value.size());
    if (!reply) { reconnect(); return false; }
    
    bool ok = (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0);
    freeReplyObject(reply);
    return ok;
}

std::string RedisPlugin::get(const std::string& key) {
    auto ctx = getContext();
    if (!ctx) return "";
    
    auto reply = (redisReply*)redisCommand(ctx, "GET %b", key.data(), key.size());
    if (!reply) { reconnect(); return ""; }
    
    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result.assign(reply->str, reply->len);
    }
    freeReplyObject(reply);
    return result;
}

bool RedisPlugin::del(const std::string& key) {
    auto ctx = getContext();
    if (!ctx) return false;
    
    auto reply = (redisReply*)redisCommand(ctx, "DEL %b", key.data(), key.size());
    if (!reply) { reconnect(); return false; }
    
    bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);
    return ok;
}

bool RedisPlugin::exists(const std::string& key) {
    auto ctx = getContext();
    if (!ctx) return false;
    
    auto reply = (redisReply*)redisCommand(ctx, "EXISTS %b", key.data(), key.size());
    if (!reply) { reconnect(); return false; }
    
    bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);
    return ok;
}

long RedisPlugin::incr(const std::string& key) {
    auto ctx = getContext();
    if (!ctx) return -1;
    
    auto reply = (redisReply*)redisCommand(ctx, "INCR %b", key.data(), key.size());
    if (!reply) { reconnect(); return -1; }
    
    long result = (reply->type == REDIS_REPLY_INTEGER) ? reply->integer : -1;
    freeReplyObject(reply);
    return result;
}

long RedisPlugin::decr(const std::string& key) {
    auto ctx = getContext();
    if (!ctx) return -1;
    
    auto reply = (redisReply*)redisCommand(ctx, "DECR %b", key.data(), key.size());
    if (!reply) { reconnect(); return -1; }
    
    long result = (reply->type == REDIS_REPLY_INTEGER) ? reply->integer : -1;
    freeReplyObject(reply);
    return result;
}

bool RedisPlugin::expire(const std::string& key, int seconds) {
    auto ctx = getContext();
    if (!ctx) return false;
    
    auto reply = (redisReply*)redisCommand(ctx, "EXPIRE %b %d", key.data(), key.size(), seconds);
    if (!reply) { reconnect(); return false; }
    
    bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);
    return ok;
}

// ---- Set Operations ----

bool RedisPlugin::setBit(const std::string& key, size_t offset, int value) {
    auto ctx = getContext();
    if (!ctx) return false;
    
    auto reply = (redisReply*)redisCommand(ctx, "SETBIT %b %zu %d", key.data(), key.size(), offset, value);
    if (!reply) { reconnect(); return false; }
    
    bool ok = (reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(reply);
    return ok;
}

int RedisPlugin::getBit(const std::string& key, size_t offset) {
    auto ctx = getContext();
    if (!ctx) return -1;
    
    auto reply = (redisReply*)redisCommand(ctx, "GETBIT %b %zu", key.data(), key.size(), offset);
    if (!reply) { reconnect(); return -1; }
    
    int result = (reply->type == REDIS_REPLY_INTEGER) ? reply->integer : -1;
    freeReplyObject(reply);
    return result;
}

long RedisPlugin::bitCount(const std::string& key) {
    auto ctx = getContext();
    if (!ctx) return -1;
    
    auto reply = (redisReply*)redisCommand(ctx, "BITCOUNT %b", key.data(), key.size());
    if (!reply) { reconnect(); return -1; }
    
    long result = (reply->type == REDIS_REPLY_INTEGER) ? reply->integer : -1;
    freeReplyObject(reply);
    return result;
}

// ---- Lock Operations ----

bool RedisPlugin::lock(const std::string& key, int ttlSeconds) {
    auto ctx = getContext();
    if (!ctx) return false;
    
    auto reply = (redisReply*)redisCommand(ctx, "SET %b 1 NX EX %d", key.data(), key.size(), ttlSeconds);
    if (!reply) { reconnect(); return false; }
    
    bool ok = (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0);
    freeReplyObject(reply);
    return ok;
}

bool RedisPlugin::unlock(const std::string& key) {
    return del(key);
}

} // namespace CloudDrive
