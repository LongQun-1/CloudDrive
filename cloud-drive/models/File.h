#ifndef CLOUD_DRIVE_FILE_MODEL_H
#define CLOUD_DRIVE_FILE_MODEL_H

#include <string>
#include <vector>
#include <json/json.h>
#include <drogon/orm/DbClient.h>

namespace CloudDrive {

class FileModel {
public:
    struct FileInfo {
        long id{0};
        long userId{0};
        long parentId{0};
        std::string filename;
        std::string fileHash;
        long fileSize{0};
        std::string filePath;    // actual storage path on disk
        bool isDir{false};
        bool isDeleted{false};
        int refCount{1};
        std::string createdAt;
        std::string updatedAt;
        std::string deletedAt;

        Json::Value toJson() const {
            Json::Value json;
            json["id"] = (Json::Value::Int64)id;
            json["user_id"] = (Json::Value::Int64)userId;
            json["parent_id"] = (Json::Value::Int64)parentId;
            json["filename"] = filename;
            json["file_hash"] = fileHash;
            json["file_size"] = (Json::Value::Int64)fileSize;
            json["file_path"] = filePath;
            json["is_dir"] = isDir;
            json["is_deleted"] = isDeleted;
            json["ref_count"] = refCount;
            json["created_at"] = createdAt;
            json["updated_at"] = updatedAt;
            json["deleted_at"] = deletedAt;
            return json;
        }
    };

    // Create a file record
    static std::pair<int, std::string> createFile(long userId, long parentId,
                                                    const std::string& filename,
                                                    const std::string& fileHash,
                                                    long fileSize,
                                                    const std::string& filePath,
                                                    bool isDir);

    // Get file by ID
    static FileInfo getFileById(long fileId, long userId);

    // List files in a directory
    static std::vector<FileInfo> listFiles(long userId, long parentId,
                                            int page = 1, int pageSize = 50);

    // Count files in a directory
    static int countFiles(long userId, long parentId);

    // Search files by name
    static std::vector<FileInfo> searchFiles(long userId, const std::string& keyword,
                                               int page = 1, int pageSize = 50);

    // Count search results
    static int countSearchFiles(long userId, const std::string& keyword);

    // Rename a file
    static bool renameFile(long fileId, long userId, const std::string& newName);

    // Move a file to a new parent
    static bool moveFile(long fileId, long userId, long newParentId);

    // Soft delete (move to recycle bin)
    static bool softDeleteFile(long fileId, long userId);

    // Batch soft delete
    static bool softDeleteFiles(const std::vector<long>& fileIds, long userId);

    // Restore from recycle bin
    static bool restoreFile(long fileId, long userId);

    // Hard delete (permanent)
    static bool hardDeleteFile(long fileId, long userId);

    // Get files in recycle bin
    static std::vector<FileInfo> listDeletedFiles(long userId, int page = 1, int pageSize = 50);

    // Count deleted files
    static int countDeletedFiles(long userId);

    // Calculate user storage used
    static long calculateStorageUsed(long userId);

    // Check if file exists in same directory (same parent + same name)
    static bool fileExistsInDir(long userId, long parentId, const std::string& filename);

    // Find file by hash (for dedup / instant upload)
    static FileInfo findFileByHash(long userId, const std::string& fileHash);

    // Increment ref count
    static bool incrementRefCount(const std::string& fileHash);
};

} // namespace CloudDrive

#endif
