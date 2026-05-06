#include "UserController.h"
#include "middlewares/AuthMiddleware.h"
#include "services/AuthService.h"
#include "models/User.h"
#include "utils/ValidatorUtil.h"
#include "utils/ResponseUtil.h"
#include "plugins/RedisPlugin.h"
#include <drogon/drogon.h>

namespace CloudDrive {

// POST /api/user/register
void UserController::registerUser(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json) {
        callback(ResponseUtil::error(1001, "request body must be JSON"));
        return;
    }
    
    std::string username = (*json)["username"].asString();
    std::string password = (*json)["password"].asString();
    std::string nickname = (*json)["nickname"].asString();
    if (nickname.empty()) nickname = username;
    
    // Validate input
    if (!ValidatorUtil::isValidUsername(username)) {
        callback(ResponseUtil::error(1001, "invalid username: 3-50 chars, alphanumeric and underscore only"));
        return;
    }
    
    if (!ValidatorUtil::isValidPassword(password)) {
        callback(ResponseUtil::error(1001, "invalid password: min 8 chars, must contain letters and digits"));
        return;
    }
    
    if (!ValidatorUtil::isValidNickname(nickname)) {
        callback(ResponseUtil::error(1001, "invalid nickname: max 50 chars"));
        return;
    }
    
    // Hash password
    AuthService auth;
    std::string passwordHash = auth.hashPassword(password);
    if (passwordHash.empty()) {
        callback(ResponseUtil::error(9001, "internal error: password hashing failed"));
        return;
    }
    
    // Create user
    auto [code, message] = UserModel::createUser(username, passwordHash, nickname);
    if (code != 0) {
        callback(ResponseUtil::error(code, message));
        return;
    }
    
    LOG_INFO << "User registered: " << username;
    callback(ResponseUtil::success(Json::Value(), message));
}

// POST /api/user/login
void UserController::login(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json) {
        callback(ResponseUtil::error(1001, "request body must be JSON"));
        return;
    }
    
    std::string username = (*json)["username"].asString();
    std::string password = (*json)["password"].asString();
    
    // Validate input
    if (!ValidatorUtil::isValidUsername(username)) {
        callback(ResponseUtil::error(1001, "invalid username format"));
        return;
    }
    
    // Get user
    auto userInfo = UserModel::getUserByUsername(username);
    if (userInfo.id == 0) {
        // Don't reveal whether username exists
        callback(ResponseUtil::error(2003, "incorrect username or password"));
        return;
    }
    
    // Verify password
    // Need to get password_hash from DB
    auto db = drogon::app().getDbClient("cloud_drive");
    std::string storedHash;
    try {
        auto result = db->execSqlSync("SELECT password_hash FROM user WHERE username = ?", username);
        if (result.size() > 0) {
            storedHash = result[0]["password_hash"].as<std::string>();
        }
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Login query failed: " << e.base().what();
        callback(ResponseUtil::error(9001, "internal error"));
        return;
    }
    
    AuthService auth;
    if (!auth.verifyPassword(password, storedHash)) {
        callback(ResponseUtil::error(2003, "incorrect username or password"));
        return;
    }
    
    // Generate tokens
    std::string accessToken = auth.generateAccessToken(userInfo.id, username);
    std::string refreshToken = auth.generateRefreshToken(userInfo.id, username);
    
    // Save refresh token to Redis
    auto redis = drogon::app().getPlugin<RedisPlugin>();
    if (redis && redis->isConnected()) {
        std::string tokenKey = "refresh_token:" + std::to_string(userInfo.id) + ":" + refreshToken.substr(0, 16);
        auth.saveRefreshToken(tokenKey, userInfo.id);
    }
    
    Json::Value data;
    data["access_token"] = accessToken;
    data["refresh_token"] = refreshToken;
    data["access_token_expires_in"] = 1800;
    data["refresh_token_expires_in"] = 604800;
    data["user"] = userInfo.toJson();
    
    LOG_INFO << "User logged in: " << username;
    callback(ResponseUtil::success(data, "login successful"));
}

// POST /api/user/refresh
void UserController::refreshToken(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json) {
        callback(ResponseUtil::error(1001, "request body must be JSON"));
        return;
    }
    
    std::string refreshToken = (*json)["refresh_token"].asString();
    if (refreshToken.empty()) {
        callback(ResponseUtil::error(1001, "refresh_token is required"));
        return;
    }
    
    AuthService auth;
    auto payload = auth.verifyRefreshToken(refreshToken);
    if (payload.isNull()) {
        callback(ResponseUtil::error(1002, "invalid or expired refresh token"));
        return;
    }
    
    long userId = payload["user_id"].asInt64();
    std::string username = payload["username"].asString();
    
    // Revoke old refresh token (one-time use)
    std::string oldTokenKey = "refresh_token:" + std::to_string(userId) + ":" + refreshToken.substr(0, 16);
    auth.revokeRefreshToken(oldTokenKey);
    
    // Issue new tokens
    std::string newAccessToken = auth.generateAccessToken(userId, username);
    std::string newRefreshToken = auth.generateRefreshToken(userId, username);
    
    // Save new refresh token
    auto redis = drogon::app().getPlugin<RedisPlugin>();
    if (redis && redis->isConnected()) {
        std::string newTokenKey = "refresh_token:" + std::to_string(userId) + ":" + newRefreshToken.substr(0, 16);
        auth.saveRefreshToken(newTokenKey, userId);
    }
    
    Json::Value data;
    data["access_token"] = newAccessToken;
    data["refresh_token"] = newRefreshToken;
    data["access_token_expires_in"] = 1800;
    data["refresh_token_expires_in"] = 604800;
    
    callback(ResponseUtil::success(data, "token refreshed"));
}

// POST /api/user/logout
void UserController::logout(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    long userId = AuthMiddleware::getUserId(req);
    if (userId < 0) {
        callback(ResponseUtil::error(1002, "unauthorized"));
        return;
    }
    
    auto json = req->getJsonObject();
    if (json && (*json).isMember("refresh_token")) {
        std::string refreshToken = (*json)["refresh_token"].asString();
        std::string tokenKey = "refresh_token:" + std::to_string(userId) + ":" + refreshToken.substr(0, 16);
        
        auto redis = drogon::app().getPlugin<RedisPlugin>();
        if (redis) {
            redis->del(tokenKey);
        }
    }
    
    callback(ResponseUtil::success(Json::Value(), "logged out"));
}

// GET /api/user/info
void UserController::getUserInfo(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    long userId = AuthMiddleware::getUserId(req);
    if (userId < 0) {
        callback(ResponseUtil::error(1002, "unauthorized"));
        return;
    }
    
    auto userInfo = UserModel::getUserById(userId);
    if (userInfo.id == 0) {
        callback(ResponseUtil::error(2002, "user not found"));
        return;
    }
    
    // Calculate storage usage percentage
    Json::Value data = userInfo.toJson();
    double usagePercent = (double)userInfo.storageUsed / (double)userInfo.storageLimit * 100.0;
    data["storage_usage_percent"] = usagePercent;
    
    callback(ResponseUtil::success(data));
}

// PUT /api/user/info
void UserController::updateUserInfo(const drogon::HttpRequestPtr& req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    long userId = AuthMiddleware::getUserId(req);
    if (userId < 0) {
        callback(ResponseUtil::error(1002, "unauthorized"));
        return;
    }
    
    auto json = req->getJsonObject();
    if (!json) {
        callback(ResponseUtil::error(1001, "request body must be JSON"));
        return;
    }
    
    std::string nickname = (*json).get("nickname", "").asString();
    std::string avatar = (*json).get("avatar", "").asString();
    
    // At least one field must be provided
    if (nickname.empty() && avatar.empty()) {
        callback(ResponseUtil::error(1001, "at least one field (nickname or avatar) is required"));
        return;
    }
    
    if (!nickname.empty() && !ValidatorUtil::isValidNickname(nickname)) {
        callback(ResponseUtil::error(1001, "invalid nickname"));
        return;
    }
    
    if (!UserModel::updateUserInfo(userId, nickname, avatar)) {
        callback(ResponseUtil::error(9001, "update failed"));
        return;
    }
    
    auto userInfo = UserModel::getUserById(userId);
    callback(ResponseUtil::success(userInfo.toJson(), "user info updated"));
}

// PUT /api/user/password
void UserController::changePassword(const drogon::HttpRequestPtr& req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    long userId = AuthMiddleware::getUserId(req);
    if (userId < 0) {
        callback(ResponseUtil::error(1002, "unauthorized"));
        return;
    }
    
    auto json = req->getJsonObject();
    if (!json) {
        callback(ResponseUtil::error(1001, "request body must be JSON"));
        return;
    }
    
    std::string oldPassword = (*json)["old_password"].asString();
    std::string newPassword = (*json)["new_password"].asString();
    
    if (oldPassword.empty() || newPassword.empty()) {
        callback(ResponseUtil::error(1001, "old_password and new_password are required"));
        return;
    }
    
    if (!ValidatorUtil::isValidPassword(newPassword)) {
        callback(ResponseUtil::error(1001, "invalid new password: min 8 chars, must contain letters and digits"));
        return;
    }
    
    // Verify old password
    auto db = drogon::app().getDbClient("cloud_drive");
    std::string storedHash;
    try {
        auto result = db->execSqlSync("SELECT password_hash FROM user WHERE id = ?", userId);
        if (result.size() > 0) {
            storedHash = result[0]["password_hash"].as<std::string>();
        }
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Password change query failed: " << e.base().what();
        callback(ResponseUtil::error(9001, "internal error"));
        return;
    }
    
    AuthService auth;
    if (!auth.verifyPassword(oldPassword, storedHash)) {
        callback(ResponseUtil::error(2003, "incorrect old password"));
        return;
    }
    
    // Update password
    std::string newHash = auth.hashPassword(newPassword);
    if (newHash.empty()) {
        callback(ResponseUtil::error(9001, "password hashing failed"));
        return;
    }
    
    if (!UserModel::updatePassword(userId, newHash)) {
        callback(ResponseUtil::error(9001, "password update failed"));
        return;
    }
    
    // Revoke all refresh tokens for this user
    // In a real app, we'd iterate and revoke all - for now, logout effect
    callback(ResponseUtil::success(Json::Value(), "password changed successfully"));
}

} // namespace CloudDrive
