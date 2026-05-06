#include "CodeUtil.h"
#include <random>

namespace CloudDrive {

std::string CodeUtil::generateRandomCode(int length) {
    static const std::string charset = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dist(0, charset.size() - 1);
    
    std::string result;
    result.reserve(length);
    for (int i = 0; i < length; i++) {
        result += charset[dist(gen)];
    }
    return result;
}

std::string CodeUtil::generateShareUrl() {
    return generateRandomCode(8);
}

std::string CodeUtil::generateShareCode() {
    return generateRandomCode(8);
}

} // namespace CloudDrive
