#include "HashUtil.h"
#include <openssl/evp.h>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace CloudDrive {

std::string HashUtil::sha256(const std::string& input) {
    return sha256(reinterpret_cast<const unsigned char*>(input.data()), input.size());
}

std::string HashUtil::sha256(const unsigned char* data, size_t length) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return "";
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, data, length);
    EVP_DigestFinal_ex(ctx, hash, &hashLen);
    EVP_MD_CTX_free(ctx);
    
    std::stringstream ss;
    for (unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string HashUtil::sha256File(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) return "";
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return "";
    
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    
    char buffer[8192];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        EVP_DigestUpdate(ctx, buffer, file.gcount());
    }
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    EVP_DigestFinal_ex(ctx, hash, &hashLen);
    EVP_MD_CTX_free(ctx);
    
    std::stringstream ss;
    for (unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string HashUtil::sha256FromFile(const std::string& tmpDir, const std::string& uploadId, int totalChunks) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return "";

    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);

    for (int i = 0; i < totalChunks; i++) {
        // Match FileUtil::getChunkPath pattern: tmpDir/uploadId/i.chunk
        std::string chunkPath = tmpDir + "/" + uploadId + "/" + std::to_string(i) + ".chunk";
        std::ifstream file(chunkPath, std::ios::binary);
        if (!file) {
            EVP_MD_CTX_free(ctx);
            return "";
        }

        char buffer[8192];
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
            EVP_DigestUpdate(ctx, buffer, file.gcount());
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    EVP_DigestFinal_ex(ctx, hash, &hashLen);
    EVP_MD_CTX_free(ctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string HashUtil::md5(const std::string& input) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return "";

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;

    EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
    EVP_DigestUpdate(ctx, input.data(), input.size());
    EVP_DigestFinal_ex(ctx, hash, &hashLen);
    EVP_MD_CTX_free(ctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

} // namespace CloudDrive
