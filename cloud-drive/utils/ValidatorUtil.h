#ifndef CLOUD_DRIVE_VALIDATOR_UTIL_H
#define CLOUD_DRIVE_VALIDATOR_UTIL_H

#include <string>

namespace CloudDrive {

class ValidatorUtil {
public:
    // Validate username: 3-50 chars, alphanumeric + underscore
    static bool isValidUsername(const std::string& username);
    
    // Validate password: min 8 chars, at least one letter and one digit
    static bool isValidPassword(const std::string& password);
    
    // Validate nickname: 1-50 chars
    static bool isValidNickname(const std::string& nickname);
    
    // Validate filename: not empty, <= 255 chars, no path chars
    static bool isValidFilename(const std::string& filename);
    
    // Validate email format (basic)
    static bool isValidEmail(const std::string& email);
    
    // Sanitize filename: remove dangerous characters
    static std::string sanitizeFilename(const std::string& filename);
};

} // namespace CloudDrive

#endif
