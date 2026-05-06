#include "AuthService.h"
#include "utils/JwtUtil.h"
#include "plugins/RedisPlugin.h"
#include <drogon/drogon.h>
#include <crypt.h>
#include <random>
#include <chrono>

namespace CloudDrive {

AuthService::AuthService() {
    auto& config = drogon::app().getCustomConfig();
    jwtSecret_ = config.get("jwt", Json::Value()).get("secret", "CloudDrive-JWT-Secret-Key-2024").asString();
    accessTokenExpireSeconds_ = config.get("jwt", Json::Value()).get("access_token_expire_seconds", 1800).asInt();
    refreshTokenExpireSeconds_ = config.get("jwt", Json::Value()).get("refresh_token_expire_seconds", 604800).asInt();
}

AuthService::~AuthService() {}

std::string AuthService::hashPassword(const std::string& password) {
    // Generate bcrypt salt
    static const std::string charset = 
        "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dist(0, charset.size() - 1);
    
    char salt[30];
    // $2b$12$ + 22 chars of salt
    salt[0] = '$'; salt[1] = '2'; salt[2] = 'b'; salt[3] = '$';
    salt[4] = '1'; salt[5] = '2'; salt[6] = '$';
    for (int i = 0; i < 22; i++) {
        salt[7 + i] = charset[dist(gen)];
    }
    salt[29] = '\0';
    
    // Use crypt_r for thread safety
    struct crypt_data data;
    data.initialized = 0;
    
    char* result = crypt_r(password.c_str(), salt, &data);
    if (!result) {
        LOG_ERROR << "bcrypt hash failed";
        return "";
    }
    
    return std::string(result);
}

bool AuthService::verifyPassword(const std::string& password, const std::string& hash) {
    struct crypt_data data;
    data.initialized = 0;
    
    // Extract salt from hash (first 29 chars: $2b$12$ + 22 salt chars)
    std::string salt = hash.substr(0, 29);
    
    char* result = crypt_r(password.c_str(), salt.c_str(), &data);
    if (!result) {
        LOG_ERROR << "bcrypt verify failed";
        return false;
    }
    
    return hash == std::string(result);
}

std::string AuthService::generateAccessToken(long userId, const std::string& username) {
    Json::Value payload;
    payload["user_id"] = (Json::Value::Int64)userId;
    payload["username"] = username;
    payload["type"] = "access";
    
    return JwtUtil::generateToken(payload, jwtSecret_, accessTokenExpireSeconds_);
}

std::string AuthService::generateRefreshToken(long userId, const std::string& username) {
    Json::Value payload;
    payload["user_id"] = (Json::Value::Int64)userId;
    payload["username"] = username;
    payload["type"] = "refresh";
    
    return JwtUtil::generateToken(payload, jwtSecret_, refreshTokenExpireSeconds_);
}

Json::Value AuthService::verifyAccessToken(const std::string& token) {
    Json::Value payload = JwtUtil::verifyToken(token, jwtSecret_);
    if (payload.isNull() || !payload.isMember("type") || payload["type"].asString() != "access") {
        return Json::Value();
    }
    return payload;
}

Json::Value AuthService::verifyRefreshToken(const std::string& token) {
    Json::Value payload = JwtUtil::verifyToken(token, jwtSecret_);
    if (payload.isNull() || !payload.isMember("type") || payload["type"].asString() != "refresh") {
        return Json::Value();
    }
    
    // Check if refresh token is revoked
    long userId = payload["user_id"].asInt64();
    auto tokenKey = "refresh_token:" + std::to_string(userId) + ":" + token.substr(0, 16);
    
    auto redis = drogon::app().getPlugin<RedisPlugin>();
    if (redis && redis->isConnected()) {
        std::string storedToken = redis->get(tokenKey);
        if (storedToken.empty()) {
            return Json::Value(); // Token revoked or not found
        }
    }
    
    return payload;
}

bool AuthService::saveRefreshToken(const std::string& tokenKey, long userId, int expireSeconds) {
    auto redis = drogon::app().getPlugin<RedisPlugin>();
    if (!redis || !redis->isConnected()) {
        return false;
    }
    return redis->setex(tokenKey, expireSeconds, std::to_string(userId));
}

bool AuthService::revokeRefreshToken(const std::string& tokenKey) {
    auto redis = drogon::app().getPlugin<RedisPlugin>();
    if (!redis || !redis->isConnected()) {
        return false;
    }
    return redis->del(tokenKey);
}

bool AuthService::isRefreshTokenRevoked(const std::string& tokenKey) {
    auto redis = drogon::app().getPlugin<RedisPlugin>();
    if (!redis || !redis->isConnected()) {
        return true; // If Redis is down, consider tokens revoked for safety
    }
    return !redis->exists(tokenKey);
}

long AuthService::getUserIdFromToken(const std::string& token) {
    auto payload = verifyAccessToken(token);
    if (payload.isNull()) return -1;
    return payload["user_id"].asInt64();
}

std::string AuthService::getUsernameFromToken(const std::string& token) {
    auto payload = verifyAccessToken(token);
    if (payload.isNull()) return "";
    return payload["username"].asString();
}

} // namespace CloudDrive
