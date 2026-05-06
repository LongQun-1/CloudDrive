#ifndef CLOUD_DRIVE_USER_MODEL_H
#define CLOUD_DRIVE_USER_MODEL_H

#include <string>
#include <drogon/orm/DbClient.h>

namespace CloudDrive {

class UserModel {
public:
    struct UserInfo {
        long id{0};
        std::string username;
        std::string nickname;
        std::string avatar;
        long storageUsed{0};
        long storageLimit{1073741824};
        std::string createdAt;
        std::string updatedAt;
        
        Json::Value toJson() const {
            Json::Value json;
            json["id"] = (Json::Value::Int64)id;
            json["username"] = username;
            json["nickname"] = nickname;
            json["avatar"] = avatar;
            json["storage_used"] = (Json::Value::Int64)storageUsed;
            json["storage_limit"] = (Json::Value::Int64)storageLimit;
            json["created_at"] = createdAt;
            json["updated_at"] = updatedAt;
            return json;
        }
    };

    // Create a new user
    static std::pair<int, std::string> createUser(const std::string& username, 
                                                    const std::string& passwordHash,
                                                    const std::string& nickname);
    
    // Get user by username
    static UserInfo getUserByUsername(const std::string& username);
    
    // Get user by ID
    static UserInfo getUserById(long id);
    
    // Update user info
    static bool updateUserInfo(long userId, const std::string& nickname, const std::string& avatar);
    
    // Update password
    static bool updatePassword(long userId, const std::string& newPasswordHash);
    
    // Update storage used (atomic)
    static bool updateStorageUsed(long userId, long delta);
    
    // Check if username exists
    static bool usernameExists(const std::string& username);
};

} // namespace CloudDrive

#endif
