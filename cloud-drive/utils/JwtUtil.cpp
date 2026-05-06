#include "JwtUtil.h"
#include <openssl/hmac.h>
#include <sstream>
#include <chrono>
#include <cstring>

namespace CloudDrive {

std::string JwtUtil::base64UrlEncode(const std::string& input) {
    static const std::string base64Chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string result;
    size_t i = 0;
    while (i < input.size()) {
        unsigned char c1 = input[i++];
        result += base64Chars[c1 >> 2];
        if (i == input.size()) {
            result += base64Chars[(c1 & 0x03) << 4];
            result += "==";
            break;
        }
        unsigned char c2 = input[i++];
        result += base64Chars[((c1 & 0x03) << 4) | (c2 >> 4)];
        if (i == input.size()) {
            result += base64Chars[(c2 & 0x0F) << 2];
            result += "=";
            break;
        }
        unsigned char c3 = input[i++];
        result += base64Chars[((c2 & 0x0F) << 2) | (c3 >> 6)];
        result += base64Chars[c3 & 0x3F];
    }
    
    // Replace + with -, / with _, remove trailing =
    for (auto& c : result) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    size_t pos = result.find('=');
    if (pos != std::string::npos) {
        result = result.substr(0, pos);
    }
    return result;
}

std::string JwtUtil::base64UrlDecode(const std::string& input) {
    std::string normalized = input;
    for (auto& c : normalized) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    
    // Add padding
    size_t padding = 4 - (normalized.size() % 4);
    if (padding != 4) {
        normalized.append(padding, '=');
    }
    
    static const std::string base64Chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string result;
    size_t i = 0;
    while (i < normalized.size() && normalized[i] != '=') {
        unsigned char c1 = base64Chars.find(normalized[i++]);
        unsigned char c2 = base64Chars.find(normalized[i++]);
        result += (c1 << 2) | (c2 >> 4);
        if (i >= normalized.size() || normalized[i] == '=') break;
        unsigned char c3 = base64Chars.find(normalized[i++]);
        result += ((c2 & 0x0F) << 4) | (c3 >> 2);
        if (i >= normalized.size() || normalized[i] == '=') break;
        unsigned char c4 = base64Chars.find(normalized[i++]);
        result += ((c3 & 0x03) << 6) | c4;
    }
    return result;
}

std::string JwtUtil::hmacSha256(const std::string& data, const std::string& key) {
    unsigned char* result = HMAC(EVP_sha256(), key.c_str(), key.size(),
                                  reinterpret_cast<const unsigned char*>(data.c_str()),
                                  data.size(), nullptr, nullptr);
    return std::string(reinterpret_cast<char*>(result), 32);
}

std::string JwtUtil::generateToken(const Json::Value& payload, 
                                    const std::string& secret,
                                    long expireSeconds) {
    // Header
    Json::Value header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";
    std::string headerStr = Json::FastWriter().write(header);
    // Remove trailing newline
    if (!headerStr.empty() && headerStr.back() == '\n') headerStr.pop_back();
    
    // Payload with timestamps
    Json:: Value finalPayload = payload;
    auto now = std::chrono::system_clock::now();
    auto nowSeconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    finalPayload["iat"] = (Json::Value::Int64)nowSeconds;
    finalPayload["exp"] = (Json::Value::Int64)(nowSeconds + expireSeconds);
    
    std::string payloadStr = Json::FastWriter().write(finalPayload);
    if (!payloadStr.empty() && payloadStr.back() == '\n') payloadStr.pop_back();
    
    // Sign
    std::string headerB64 = base64UrlEncode(headerStr);
    std::string payloadB64 = base64UrlEncode(payloadStr);
    std::string signingInput = headerB64 + "." + payloadB64;
    std::string signature = hmacSha256(signingInput, secret);
    std::string signatureB64 = base64UrlEncode(signature);
    
    return signingInput + "." + signatureB64;
}

Json::Value JwtUtil::verifyToken(const std::string& token, const std::string& secret) {
    // Split token
    std::vector<std::string> parts;
    std::stringstream ss(token);
    std::string part;
    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }
    
    if (parts.size() != 3) {
        return Json::Value();
    }
    
    // Verify signature
    std::string signingInput = parts[0] + "." + parts[1];
    std::string expectedSig = hmacSha256(signingInput, secret);
    std::string expectedSigB64 = base64UrlEncode(expectedSig);
    
    if (parts[2] != expectedSigB64) {
        return Json::Value();
    }
    
    // Decode payload
    std::string payloadJson = base64UrlDecode(parts[1]);
    Json::Value payload;
    Json::Reader reader;
    if (!reader.parse(payloadJson, payload)) {
        return Json::Value();
    }
    
    // Check expiration
    auto now = std::chrono::system_clock::now();
    auto nowSeconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    if (payload.isMember("exp") && payload["exp"].asInt64() < nowSeconds) {
        return Json::Value();
    }
    
    return payload;
}

long JwtUtil::getUserId(const std::string& token, const std::string& secret) {
    Json::Value payload = verifyToken(token, secret);
    if (payload.isNull() || !payload.isMember("user_id")) {
        return -1;
    }
    return payload["user_id"].asInt64();
}

} // namespace CloudDrive
