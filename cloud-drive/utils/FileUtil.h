#ifndef CLOUD_DRIVE_FILE_UTIL_H
#define CLOUD_DRIVE_FILE_UTIL_H

#include <string>
#include <filesystem>
#include <vector>

namespace CloudDrive {

class FileUtil {
public:
    // Safely resolve a file path, preventing path traversal
    // Returns the normalized path if safe, throws std::runtime_error if traversal detected
    static std::string safeFilePath(const std::string& baseDir, const std::string& filename);
    
    // Get file storage path based on hash
    static std::string getStoragePath(const std::string& storageDir, const std::string& hash);
    
    // Get temporary chunk path
    static std::string getChunkPath(const std::string& tmpDir, const std::string& uploadId, int chunkIndex);
    
    // Get temporary upload directory
    static std::string getUploadTmpDir(const std::string& tmpDir, const std::string& uploadId);
    
    // Ensure directory exists
    static bool ensureDir(const std::string& dir);
    
    // Check if file exists
    static bool fileExists(const std::string& path);
    
    // Get file size
    static uint64_t getFileSize(const std::string& path);
    
    // Delete file
    static bool deleteFile(const std::string& path);
    
    // Delete directory recursively
    static bool deleteDir(const std::string& dir);
    
    // Get available disk space in bytes
    static uint64_t getAvailableDiskSpace(const std::string& path);
    
    // Validate filename (no path traversal characters)
    static bool isValidFilename(const std::string& filename);
    
    // Get file extension (lowercase)
    static std::string getFileExtension(const std::string& filename);
    
    // Allowed file extensions for upload
    static bool isAllowedExtension(const std::string& extension);
    
    // Allowed extensions whitelist
    static const std::vector<std::string>& allowedExtensions();
};

} // namespace CloudDrive

#endif
