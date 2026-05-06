#include "User.h"
#include <drogon/drogon.h>
#include <chrono>

namespace CloudDrive {

using namespace drogon::orm;

std::pair<int, std::string> UserModel::createUser(const std::string& username,
                                                    const std::string& passwordHash,
                                                    const std::string& nickname) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) {
        return {9001, "database connection failed"};
    }
    
    try {
        auto result = db->execSqlSync("INSERT INTO user (username, password_hash, nickname) VALUES (?, ?, ?)",
                                         username, passwordHash, nickname);
        return {0, "success"};
    } catch (const drogon::orm::DrogonDbException& e) {
        std::string err = e.base().what();
        if (err.find("Duplicate") != std::string::npos || err.find("Duplicate entry") != std::string::npos) {
            return {2001, "username already exists"};
        }
        LOG_ERROR << "Create user failed: " << err;
        return {9001, "internal error"};
    }
}

UserModel::UserInfo UserModel::getUserByUsername(const std::string& username) {
    UserInfo info;
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return info;
    
    try {
        auto result = db->execSqlSync("SELECT id, username, nickname, avatar, storage_used, storage_limit, "
                                         "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') as created_at, "
                                         "DATE_FORMAT(updated_at, '%Y-%m-%d %H:%i:%s') as updated_at "
                                         "FROM user WHERE username = ?", username);
        if (result.size() > 0) {
            auto row = result[0];
            info.id = row["id"].as<long long>();
            info.username = row["username"].as<std::string>();
            info.nickname = row["nickname"].as<std::string>();
            info.avatar = row["avatar"].as<std::string>();
            info.storageUsed = row["storage_used"].as<long long>();
            info.storageLimit = row["storage_limit"].as<long long>();
            info.createdAt = row["created_at"].as<std::string>();
            info.updatedAt = row["updated_at"].as<std::string>();
        }
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Get user by username failed: " << e.base().what();
    }
    
    return info;
}

UserModel::UserInfo UserModel::getUserById(long id) {
    UserInfo info;
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return info;
    
    try {
        auto result = db->execSqlSync("SELECT id, username, nickname, avatar, storage_used, storage_limit, "
                                         "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') as created_at, "
                                         "DATE_FORMAT(updated_at, '%Y-%m-%d %H:%i:%s') as updated_at "
                                         "FROM user WHERE id = ?", id);
        if (result.size() > 0) {
            auto row = result[0];
            info.id = row["id"].as<long long>();
            info.username = row["username"].as<std::string>();
            info.nickname = row["nickname"].as<std::string>();
            info.avatar = row["avatar"].as<std::string>();
            info.storageUsed = row["storage_used"].as<long long>();
            info.storageLimit = row["storage_limit"].as<long long>();
            info.createdAt = row["created_at"].as<std::string>();
            info.updatedAt = row["updated_at"].as<std::string>();
        }
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Get user by id failed: " << e.base().what();
    }
    
    return info;
}

bool UserModel::updateUserInfo(long userId, const std::string& nickname, const std::string& avatar) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return false;
    
    try {
        if (!avatar.empty()) {
            db->execSqlSync("UPDATE user SET nickname = ?, avatar = ? WHERE id = ?",
                               nickname, avatar, userId);
        } else {
            db->execSqlSync("UPDATE user SET nickname = ? WHERE id = ?", nickname, userId);
        }
        return true;
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Update user info failed: " << e.base().what();
        return false;
    }
}

bool UserModel::updatePassword(long userId, const std::string& newPasswordHash) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return false;
    
    try {
        db->execSqlSync("UPDATE user SET password_hash = ? WHERE id = ?", newPasswordHash, userId);
        return true;
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Update password failed: " << e.base().what();
        return false;
    }
}

bool UserModel::updateStorageUsed(long userId, long delta) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return false;
    
    try {
        // Atomic update
        db->execSqlSync("UPDATE user SET storage_used = storage_used + ? WHERE id = ?", delta, userId);
        return true;
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Update storage used failed: " << e.base().what();
        return false;
    }
}

bool UserModel::usernameExists(const std::string& username) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return false;
    
    try {
        auto result = db->execSqlSync("SELECT COUNT(*) as cnt FROM user WHERE username = ?", username);
        return result.size() > 0 && result[0]["cnt"].as<int>() > 0;
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Check username exists failed: " << e.base().what();
        return false;
    }
}

} // namespace CloudDrive
