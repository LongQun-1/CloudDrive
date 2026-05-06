#ifndef CLOUD_DRIVE_HASH_UTIL_H
#define CLOUD_DRIVE_HASH_UTIL_H

#include <string>

namespace CloudDrive {

class HashUtil {
public:
    // Calculate SHA256 hash of a string
    static std::string sha256(const std::string& input);

    // Calculate SHA256 hash of a file
    static std::string sha256File(const std::string& filePath);

    // Calculate SHA256 hash of binary data
    static std::string sha256(const unsigned char* data, size_t length);

    // Calculate SHA256 hash from chunk files (for chunked upload merge)
    static std::string sha256FromFile(const std::string& tmpDir, const std::string& uploadId, int totalChunks);

    // Calculate MD5 hash of a string
    static std::string md5(const std::string& input);
};

} // namespace CloudDrive

#endif
