#include "File.h"
#include <drogon/drogon.h>
#include <sstream>

namespace CloudDrive {

using namespace drogon::orm;

std::pair<int, std::string> FileModel::createFile(long userId, long parentId,
                                                    const std::string& filename,
                                                    const std::string& fileHash,
                                                    long fileSize,
                                                    const std::string& filePath,
                                                    bool isDir) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return {9001, "database connection failed"};

    try {
        db->execSqlSync(
            "INSERT INTO file (user_id, parent_id, filename, file_hash, file_size, file_path, is_dir) "
            "VALUES (?, ?, ?, ?, ?, ?, ?)",
            userId, parentId, filename,
            fileHash,
            fileSize,
            filePath,
            isDir ? 1 : 0);
        return {0, "success"};
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Create file failed: " << e.base().what();
        return {9001, "internal error"};
    }
}

FileModel::FileInfo FileModel::getFileById(long fileId, long userId) {
    FileInfo info;
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return info;

    try {
        auto result = db->execSqlSync(
            "SELECT id, user_id, parent_id, filename, file_hash, file_size, file_path, "
            "is_dir, is_deleted, ref_count, "
            "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') as created_at, "
            "DATE_FORMAT(updated_at, '%Y-%m-%d %H:%i:%s') as updated_at, "
            "DATE_FORMAT(deleted_at, '%Y-%m-%d %H:%i:%s') as deleted_at "
            "FROM file WHERE id = ? AND user_id = ? AND is_deleted = 0",
            fileId, userId);

        if (result.size() > 0) {
            auto row = result[0];
            info.id = row["id"].as<long long>();
            info.userId = row["user_id"].as<long long>();
            info.parentId = row["parent_id"].isNull() ? 0 : row["parent_id"].as<long long>();
            info.filename = row["filename"].as<std::string>();
            info.fileHash = row["file_hash"].isNull() ? "" : row["file_hash"].as<std::string>();
            info.fileSize = row["file_size"].isNull() ? 0 : row["file_size"].as<long long>();
            info.filePath = row["file_path"].isNull() ? "" : row["file_path"].as<std::string>();
            info.isDir = row["is_dir"].isNull() ? false : (row["is_dir"].as<int>() != 0);
            info.isDeleted = row["is_deleted"].isNull() ? false : (row["is_deleted"].as<int>() != 0);
            info.refCount = row["ref_count"].isNull() ? 1 : row["ref_count"].as<int>();
            info.createdAt = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
            info.updatedAt = row["updated_at"].isNull() ? "" : row["updated_at"].as<std::string>();
            info.deletedAt = row["deleted_at"].isNull() ? "" : row["deleted_at"].as<std::string>();
        }
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Get file by id failed: " << e.base().what();
    }

    return info;
}

std::vector<FileModel::FileInfo> FileModel::listFiles(long userId, long parentId,
                                                        int page, int pageSize) {
    std::vector<FileInfo> files;
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return files;

    int offset = (page - 1) * pageSize;

    try {
        auto result = db->execSqlSync(
            "SELECT id, user_id, parent_id, filename, file_hash, file_size, file_path, "
            "is_dir, is_deleted, ref_count, "
            "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') as created_at, "
            "DATE_FORMAT(updated_at, '%Y-%m-%d %H:%i:%s') as updated_at, "
            "DATE_FORMAT(deleted_at, '%Y-%m-%d %H:%i:%s') as deleted_at "
            "FROM file WHERE user_id = ? AND parent_id = ? AND is_deleted = 0 "
            "ORDER BY is_dir DESC, filename ASC LIMIT ? OFFSET ?",
            userId, parentId, pageSize, offset);

        for (auto row : result) {
            FileInfo info;
            info.id = row["id"].as<long long>();
            info.userId = row["user_id"].as<long long>();
            info.parentId = row["parent_id"].isNull() ? 0 : row["parent_id"].as<long long>();
            info.filename = row["filename"].as<std::string>();
            info.fileHash = row["file_hash"].isNull() ? "" : row["file_hash"].as<std::string>();
            info.fileSize = row["file_size"].isNull() ? 0 : row["file_size"].as<long long>();
            info.filePath = row["file_path"].isNull() ? "" : row["file_path"].as<std::string>();
            info.isDir = row["is_dir"].isNull() ? false : (row["is_dir"].as<int>() != 0);
            info.isDeleted = row["is_deleted"].isNull() ? false : (row["is_deleted"].as<int>() != 0);
            info.refCount = row["ref_count"].isNull() ? 1 : row["ref_count"].as<int>();
            info.createdAt = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
            info.updatedAt = row["updated_at"].isNull() ? "" : row["updated_at"].as<std::string>();
            info.deletedAt = row["deleted_at"].isNull() ? "" : row["deleted_at"].as<std::string>();
            files.push_back(std::move(info));
        }
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "List files failed: " << e.base().what();
    }

    return files;
}

int FileModel::countFiles(long userId, long parentId) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return 0;

    try {
        auto result = db->execSqlSync(
            "SELECT COUNT(*) as cnt FROM file WHERE user_id = ? AND parent_id = ? AND is_deleted = 0",
            userId, parentId);
        if (result.size() > 0) {
            return result[0]["cnt"].as<int>();
        }
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Count files failed: " << e.base().what();
    }
    return 0;
}

std::vector<FileModel::FileInfo> FileModel::searchFiles(long userId, const std::string& keyword,
                                                          int page, int pageSize) {
    std::vector<FileInfo> files;
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return files;

    int offset = (page - 1) * pageSize;
    std::string pattern = "%" + keyword + "%";

    try {
        auto result = db->execSqlSync(
            "SELECT id, user_id, parent_id, filename, file_hash, file_size, file_path, "
            "is_dir, is_deleted, ref_count, "
            "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') as created_at, "
            "DATE_FORMAT(updated_at, '%Y-%m-%d %H:%i:%s') as updated_at, "
            "DATE_FORMAT(deleted_at, '%Y-%m-%d %H:%i:%s') as deleted_at "
            "FROM file WHERE user_id = ? AND is_deleted = 0 AND filename LIKE ? "
            "ORDER BY is_dir DESC, updated_at DESC LIMIT ? OFFSET ?",
            userId, pattern, pageSize, offset);

        for (auto row : result) {
            FileInfo info;
            info.id = row["id"].as<long long>();
            info.userId = row["user_id"].as<long long>();
            info.parentId = row["parent_id"].isNull() ? 0 : row["parent_id"].as<long long>();
            info.filename = row["filename"].as<std::string>();
            info.fileHash = row["file_hash"].isNull() ? "" : row["file_hash"].as<std::string>();
            info.fileSize = row["file_size"].isNull() ? 0 : row["file_size"].as<long long>();
            info.filePath = row["file_path"].isNull() ? "" : row["file_path"].as<std::string>();
            info.isDir = row["is_dir"].isNull() ? false : (row["is_dir"].as<int>() != 0);
            info.isDeleted = row["is_deleted"].isNull() ? false : (row["is_deleted"].as<int>() != 0);
            info.refCount = row["ref_count"].isNull() ? 1 : row["ref_count"].as<int>();
            info.createdAt = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
            info.updatedAt = row["updated_at"].isNull() ? "" : row["updated_at"].as<std::string>();
            info.deletedAt = row["deleted_at"].isNull() ? "" : row["deleted_at"].as<std::string>();
            files.push_back(std::move(info));
        }
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Search files failed: " << e.base().what();
    }

    return files;
}

int FileModel::countSearchFiles(long userId, const std::string& keyword) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return 0;

    std::string pattern = "%" + keyword + "%";
    try {
        auto result = db->execSqlSync(
            "SELECT COUNT(*) as cnt FROM file WHERE user_id = ? AND is_deleted = 0 AND filename LIKE ?",
            userId, pattern);
        if (result.size() > 0) {
            return result[0]["cnt"].as<int>();
        }
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Count search files failed: " << e.base().what();
    }
    return 0;
}

bool FileModel::renameFile(long fileId, long userId, const std::string& newName) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return false;

    try {
        auto result = db->execSqlSync(
            "UPDATE file SET filename = ? WHERE id = ? AND user_id = ? AND is_deleted = 0",
            newName, fileId, userId);
        return true;
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Rename file failed: " << e.base().what();
        return false;
    }
}

bool FileModel::moveFile(long fileId, long userId, long newParentId) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return false;

    try {
        db->execSqlSync(
            "UPDATE file SET parent_id = ? WHERE id = ? AND user_id = ? AND is_deleted = 0",
            newParentId, fileId, userId);
        return true;
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Move file failed: " << e.base().what();
        return false;
    }
}

bool FileModel::softDeleteFile(long fileId, long userId) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return false;

    try {
        db->execSqlSync(
            "UPDATE file SET is_deleted = 1, deleted_at = NOW() WHERE id = ? AND user_id = ? AND is_deleted = 0",
            fileId, userId);
        return true;
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Soft delete file failed: " << e.base().what();
        return false;
    }
}

bool FileModel::softDeleteFiles(const std::vector<long>& fileIds, long userId) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return false;

    try {
        for (long fileId : fileIds) {
            db->execSqlSync(
                "UPDATE file SET is_deleted = 1, deleted_at = NOW() WHERE id = ? AND user_id = ? AND is_deleted = 0",
                fileId, userId);
        }
        return true;
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Batch soft delete failed: " << e.base().what();
        return false;
    }
}

bool FileModel::restoreFile(long fileId, long userId) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return false;

    try {
        db->execSqlSync(
            "UPDATE file SET is_deleted = 0, deleted_at = NULL WHERE id = ? AND user_id = ? AND is_deleted = 1",
            fileId, userId);
        return true;
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Restore file failed: " << e.base().what();
        return false;
    }
}

bool FileModel::hardDeleteFile(long fileId, long userId) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return false;

    try {
        db->execSqlSync(
            "DELETE FROM file WHERE id = ? AND user_id = ?",
            fileId, userId);
        return true;
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Hard delete file failed: " << e.base().what();
        return false;
    }
}

std::vector<FileModel::FileInfo> FileModel::listDeletedFiles(long userId, int page, int pageSize) {
    std::vector<FileInfo> files;
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return files;

    int offset = (page - 1) * pageSize;

    try {
        auto result = db->execSqlSync(
            "SELECT id, user_id, parent_id, filename, file_hash, file_size, file_path, "
            "is_dir, is_deleted, ref_count, "
            "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') as created_at, "
            "DATE_FORMAT(updated_at, '%Y-%m-%d %H:%i:%s') as updated_at, "
            "DATE_FORMAT(deleted_at, '%Y-%m-%d %H:%i:%s') as deleted_at "
            "FROM file WHERE user_id = ? AND is_deleted = 1 "
            "ORDER BY deleted_at DESC LIMIT ? OFFSET ?",
            userId, pageSize, offset);

        for (auto row : result) {
            FileInfo info;
            info.id = row["id"].as<long long>();
            info.userId = row["user_id"].as<long long>();
            info.parentId = row["parent_id"].isNull() ? 0 : row["parent_id"].as<long long>();
            info.filename = row["filename"].as<std::string>();
            info.fileHash = row["file_hash"].isNull() ? "" : row["file_hash"].as<std::string>();
            info.fileSize = row["file_size"].isNull() ? 0 : row["file_size"].as<long long>();
            info.filePath = row["file_path"].isNull() ? "" : row["file_path"].as<std::string>();
            info.isDir = row["is_dir"].isNull() ? false : (row["is_dir"].as<int>() != 0);
            info.isDeleted = row["is_deleted"].isNull() ? false : (row["is_deleted"].as<int>() != 0);
            info.refCount = row["ref_count"].isNull() ? 1 : row["ref_count"].as<int>();
            info.createdAt = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
            info.updatedAt = row["updated_at"].isNull() ? "" : row["updated_at"].as<std::string>();
            info.deletedAt = row["deleted_at"].isNull() ? "" : row["deleted_at"].as<std::string>();
            files.push_back(std::move(info));
        }
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "List deleted files failed: " << e.base().what();
    }

    return files;
}

int FileModel::countDeletedFiles(long userId) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return 0;

    try {
        auto result = db->execSqlSync(
            "SELECT COUNT(*) as cnt FROM file WHERE user_id = ? AND is_deleted = 1",
            userId);
        if (result.size() > 0) {
            return result[0]["cnt"].as<int>();
        }
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Count deleted files failed: " << e.base().what();
    }
    return 0;
}

long FileModel::calculateStorageUsed(long userId) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return 0;

    try {
        auto result = db->execSqlSync(
            "SELECT COALESCE(SUM(file_size), 0) as total FROM file "
            "WHERE user_id = ? AND is_deleted = 0 AND is_dir = 0",
            userId);
        if (result.size() > 0) {
            return result[0]["total"].as<long long>();
        }
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Calculate storage used failed: " << e.base().what();
    }
    return 0;
}

bool FileModel::fileExistsInDir(long userId, long parentId, const std::string& filename) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return false;

    try {
        auto result = db->execSqlSync(
            "SELECT COUNT(*) as cnt FROM file WHERE user_id = ? AND parent_id = ? AND filename = ? AND is_deleted = 0",
            userId, parentId, filename);
        return result.size() > 0 && result[0]["cnt"].as<int>() > 0;
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Check file exists failed: " << e.base().what();
        return false;
    }
}

FileModel::FileInfo FileModel::findFileByHash(long userId, const std::string& fileHash) {
    FileInfo info;
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return info;

    try {
        auto result = db->execSqlSync(
            "SELECT id, user_id, parent_id, filename, file_hash, file_size, file_path, "
            "is_dir, is_deleted, ref_count, "
            "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') as created_at, "
            "DATE_FORMAT(updated_at, '%Y-%m-%d %H:%i:%s') as updated_at, "
            "DATE_FORMAT(deleted_at, '%Y-%m-%d %H:%i:%s') as deleted_at "
            "FROM file WHERE user_id = ? AND file_hash = ? AND is_deleted = 0 LIMIT 1",
            userId, fileHash);

        if (result.size() > 0) {
            auto row = result[0];
            info.id = row["id"].as<long long>();
            info.userId = row["user_id"].as<long long>();
            info.parentId = row["parent_id"].isNull() ? 0 : row["parent_id"].as<long long>();
            info.filename = row["filename"].as<std::string>();
            info.fileHash = row["file_hash"].isNull() ? "" : row["file_hash"].as<std::string>();
            info.fileSize = row["file_size"].isNull() ? 0 : row["file_size"].as<long long>();
            info.filePath = row["file_path"].isNull() ? "" : row["file_path"].as<std::string>();
            info.isDir = row["is_dir"].isNull() ? false : (row["is_dir"].as<int>() != 0);
            info.isDeleted = row["is_deleted"].isNull() ? false : (row["is_deleted"].as<int>() != 0);
            info.refCount = row["ref_count"].isNull() ? 1 : row["ref_count"].as<int>();
            info.createdAt = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
            info.updatedAt = row["updated_at"].isNull() ? "" : row["updated_at"].as<std::string>();
            info.deletedAt = row["deleted_at"].isNull() ? "" : row["deleted_at"].as<std::string>();
        }
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Find file by hash failed: " << e.base().what();
    }

    return info;
}

bool FileModel::incrementRefCount(const std::string& fileHash) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return false;

    try {
        db->execSqlSync(
            "UPDATE file SET ref_count = ref_count + 1 WHERE file_hash = ? AND is_deleted = 0",
            fileHash);
        return true;
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Increment ref count failed: " << e.base().what();
        return false;
    }
}

} // namespace CloudDrive
