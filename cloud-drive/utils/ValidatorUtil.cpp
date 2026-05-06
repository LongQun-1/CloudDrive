#include "ValidatorUtil.h"
#include <cctype>
#include <algorithm>

namespace CloudDrive {

bool ValidatorUtil::isValidUsername(const std::string& username) {
    if (username.length() < 3 || username.length() > 50) return false;
    
    for (char c : username) {
        if (!std::isalnum(c) && c != '_') return false;
    }
    
    return true;
}

bool ValidatorUtil::isValidPassword(const std::string& password) {
    if (password.length() < 8) return false;
    
    bool hasLetter = false;
    bool hasDigit = false;
    
    for (char c : password) {
        if (std::isalpha(c)) hasLetter = true;
        if (std::isdigit(c)) hasDigit = true;
    }
    
    return hasLetter && hasDigit;
}

bool ValidatorUtil::isValidNickname(const std::string& nickname) {
    return !nickname.empty() && nickname.length() <= 50;
}

bool ValidatorUtil::isValidFilename(const std::string& filename) {
    if (filename.empty() || filename.length() > 255) return false;
    
    for (char c : filename) {
        if (c == '/' || c == '\\' || c == '\0' || c == ':' || c == '*') {
            return false;
        }
    }
    
    return true;
}

bool ValidatorUtil::isValidEmail(const std::string& email) {
    if (email.length() < 3 || email.length() > 254) return false;
    
    auto atPos = email.find('@');
    if (atPos == std::string::npos || atPos == 0 || atPos == email.length() - 1) {
        return false;
    }
    
    auto dotPos = email.find('.', atPos);
    if (dotPos == std::string::npos || dotPos == email.length() - 1) {
        return false;
    }
    
    return true;
}

std::string ValidatorUtil::sanitizeFilename(const std::string& filename) {
    std::string result;
    result.reserve(filename.length());
    
    for (char c : filename) {
        if (c == '/' || c == '\\' || c == '\0') {
            result += '_';
        } else {
            result += c;
        }
    }
    
    // Trim leading/trailing spaces and dots
    auto start = result.find_first_not_of(" .");
    auto end = result.find_last_not_of(" .");
    
    if (start == std::string::npos) {
        return "_";
    }
    
    return result.substr(start, end - start + 1);
}

} // namespace CloudDrive
