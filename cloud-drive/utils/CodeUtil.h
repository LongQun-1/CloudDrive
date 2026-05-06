#ifndef CLOUD_DRIVE_CODE_UTIL_H
#define CLOUD_DRIVE_CODE_UTIL_H

#include <string>

namespace CloudDrive {

class CodeUtil {
public:
    // Generate a random string of given length (alphanumeric)
    static std::string generateRandomCode(int length);
    
    // Generate a share URL (8 characters, alphanumeric)
    static std::string generateShareUrl();
    
    // Generate a share code (8 characters, alphanumeric)
    static std::string generateShareCode();
};

} // namespace CloudDrive

#endif
