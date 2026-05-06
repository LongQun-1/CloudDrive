#ifndef CLOUD_DRIVE_AUTH_SERVICE_H
#define CLOUD_DRIVE_AUTH_SERVICE_H

#include <string>
#include <json/json.h>

namespace CloudDrive {

class AuthService {
public:
    AuthService();
    ~AuthService();

    // Password operations using bcrypt
    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);

    // JWT operations
    std::string generateAccessToken(long userId, const std::string& username);
    std::string generateRefreshToken(long userId, const std::string& username);
    
    // Token verification
    Json::Value verifyAccessToken(const std::string& token);
    Json::Value verifyRefreshToken(const std::string& token);

    // Refresh token lifecycle
    bool saveRefreshToken(const std::string& tokenKey, long userId, int expireSeconds = 604800);
    bool revokeRefreshToken(const std::string& tokenKey);
    bool isRefreshTokenRevoked(const std::string& tokenKey);

    // Get user info from token
    long getUserIdFromToken(const std::string& token);
    std::string getUsernameFromToken(const std::string& token);

private:
    std::string jwtSecret_;
    int accessTokenExpireSeconds_;
    int refreshTokenExpireSeconds_;
};

} // namespace CloudDrive

#endif
