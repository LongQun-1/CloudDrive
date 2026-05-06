#include "FileUtil.h"
#include <sys/statvfs.h>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace CloudDrive {

std::string FileUtil::safeFilePath(const std::string& baseDir, const std::string& filename) {
    // 1. Filter ../
    if (filename.find("..") != std::string::npos) {
        throw std::runtime_error("invalid filename: path traversal detected");
    }
    
    // 2. Use filesystem to normalize path
    std::filesystem::path fullPath = std::filesystem::path(baseDir) / filename;
    std::filesystem::path normalized = std::filesystem::weakly_canonical(fullPath);
    
    // 3. Verify path is still under baseDir
    std::string baseStr = std::filesystem::weakly_canonical(baseDir).string();
    if (normalized.string().find(baseStr) != 0) {
        throw std::runtime_error("path traversal detected");
    }
    
    return normalized.string();
}

std::string FileUtil::getStoragePath(const std::string& storageDir, const std::string& hash) {
    if (hash.length() < 2) {
        throw std::runtime_error("hash too short");
    }
    return storageDir + "/" + hash.substr(0, 2) + "/" + hash;
}

std::string FileUtil::getChunkPath(const std::string& tmpDir, const std::string& uploadId, int chunkIndex) {
    return tmpDir + "/" + uploadId + "/" + std::to_string(chunkIndex) + ".chunk";
}

std::string FileUtil::getUploadTmpDir(const std::string& tmpDir, const std::string& uploadId) {
    return tmpDir + "/" + uploadId;
}

bool FileUtil::ensureDir(const std::string& dir) {
    try {
        std::filesystem::create_directories(dir);
        return true;
    } catch (...) {
        return false;
    }
}

bool FileUtil::fileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

uint64_t FileUtil::getFileSize(const std::string& path) {
    try {
        return std::filesystem::file_size(path);
    } catch (...) {
        return 0;
    }
}

bool FileUtil::deleteFile(const std::string& path) {
    try {
        return std::filesystem::remove(path);
    } catch (...) {
        return false;
    }
}

bool FileUtil::deleteDir(const std::string& dir) {
    try {
        return std::filesystem::remove_all(dir) > 0;
    } catch (...) {
        return false;
    }
}

uint64_t FileUtil::getAvailableDiskSpace(const std::string& path) {
    struct statvfs stat;
    if (statvfs(path.c_str(), &stat) != 0) {
        return 0;
    }
    return (uint64_t)stat.f_bsize * (uint64_t)stat.f_bavail;
}

bool FileUtil::isValidFilename(const std::string& filename) {
    if (filename.empty() || filename.length() > 255) return false;
    
    // Disallow path traversal and control characters
    for (char c : filename) {
        if (c == '/' || c == '\\' || c == '\0' || c == ':' || c == '*') {
            return false;
        }
        if (static_cast<unsigned char>(c) < 32) return false;
    }
    
    return true;
}

std::string FileUtil::getFileExtension(const std::string& filename) {
    size_t pos = filename.find_last_of('.');
    if (pos == std::string::npos) return "";
    std::string ext = filename.substr(pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool FileUtil::isAllowedExtension(const std::string& extension) {
    const auto& allowed = allowedExtensions();
    return std::find(allowed.begin(), allowed.end(), extension) != allowed.end();
}

const std::vector<std::string>& FileUtil::allowedExtensions() {
    static const std::vector<std::string> extensions = {
        "jpg", "jpeg", "png", "gif", "webp", "bmp",
        "mp4", "webm",
        "pdf",
        "doc", "docx", "xls", "xlsx", "ppt", "pptx",
        "txt",
        "zip", "rar", "7z", "tar", "gz",
        "md", "csv", "json", "xml",
        "mp3", "wav", "flac",
        "cpp", "h", "hpp", "py", "js", "ts", "html", "css"
    };
    return extensions;
}

} // namespace CloudDrive
