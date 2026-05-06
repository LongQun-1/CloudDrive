#ifndef CLOUD_DRIVE_JWT_UTIL_H
#define CLOUD_DRIVE_JWT_UTIL_H

#include <string>
#include <json/json.h>

namespace CloudDrive {

class JwtUtil {
public:
    // Generate a JWT token with the given payload and expiration
    static std::string generateToken(const Json::Value& payload, 
                                      const std::string& secret,
                                      long expireSeconds);
    
    // Verify and decode a JWT token, returns empty Json::Value on failure
    static Json::Value verifyToken(const std::string& token, const std::string& secret);

    // Extract user_id from token
    static long getUserId(const std::string& token, const std::string& secret);

private:
    static std::string base64UrlEncode(const std::string& input);
    static std::string base64UrlDecode(const std::string& input);
    static std::string hmacSha256(const std::string& data, const std::string& key);
};

} // namespace CloudDrive

#endif
