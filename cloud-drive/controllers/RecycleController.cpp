#include "RecycleController.h"
#include "middlewares/AuthMiddleware.h"
#include "models/File.h"
#include "models/User.h"
#include "utils/ResponseUtil.h"
#include <drogon/drogon.h>
#include <filesystem>

namespace CloudDrive {

namespace fs = std::filesystem;

// Helper: get query parameter with default value
static std::string getParam(const drogon::HttpRequestPtr& req, const std::string& key, const std::string& defaultValue = "") {
    auto& params = req->getParameters();
    auto it = params.find(key);
    if (it != params.end() && !it->second.empty()) {
        return it->second;
    }
    return defaultValue;
}

// GET /api/recycle/list?page=1&page_size=50
void RecycleController::listRecycleBin(const drogon::HttpRequestPtr& req,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    long userId = AuthMiddleware::getUserId(req);
    if (userId < 0) {
        callback(ResponseUtil::error(1002, "unauthorized"));
        return;
    }

    int page = std::atoi(getParam(req, "page", "1").c_str());
    int pageSize = std::atoi(getParam(req, "page_size", "50").c_str());

    if (page < 1) page = 1;
    if (pageSize < 1 || pageSize > 100) pageSize = 50;

    auto files = FileModel::listDeletedFiles(userId, page, pageSize);
    int total = FileModel::countDeletedFiles(userId);

    Json::Value fileList(Json::arrayValue);
    for (const auto& f : files) {
        fileList.append(f.toJson());
    }

    callback(ResponseUtil::paginated(fileList, total, page, pageSize));
}

// PUT /api/recycle/restore
void RecycleController::restoreFile(const drogon::HttpRequestPtr& req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    long userId = AuthMiddleware::getUserId(req);
    if (userId < 0) {
        callback(ResponseUtil::error(1002, "unauthorized"));
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        callback(ResponseUtil::error(1001, "request body must be JSON"));
        return;
    }

    // Support single or batch restore
    std::vector<long> fileIds;
    if ((*json).isMember("file_id")) {
        fileIds.push_back((*json).get("file_id", 0).asInt64());
    } else if ((*json).isMember("file_ids") && (*json)["file_ids"].isArray()) {
        for (auto& id : (*json)["file_ids"]) {
            fileIds.push_back(id.asInt64());
        }
    }

    if (fileIds.empty()) {
        callback(ResponseUtil::error(1001, "file_id or file_ids is required"));
        return;
    }

    // Calculate total size to restore
    long totalSize = 0;
    auto db = drogon::app().getDbClient("cloud_drive");
    for (long fileId : fileIds) {
        try {
            auto result = db->execSqlSync(
                "SELECT file_size FROM file WHERE id = ? AND user_id = ? AND is_deleted = 1",
                fileId, userId);
            if (!result.empty()) {
                totalSize += result[0]["file_size"].as<long long>();
            }
        } catch (...) {}
    }

    // Restore files
    for (long fileId : fileIds) {
        if (!FileModel::restoreFile(fileId, userId)) {
            callback(ResponseUtil::error(9001, "restore failed for file " + std::to_string(fileId)));
            return;
        }
    }

    // Update user storage
    if (totalSize > 0) {
        UserModel::updateStorageUsed(userId, totalSize);
    }

    callback(ResponseUtil::success(Json::Value(), "file(s) restored"));
}

// DELETE /api/recycle/delete
void RecycleController::permanentDelete(const drogon::HttpRequestPtr& req,
                                          std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    long userId = AuthMiddleware::getUserId(req);
    if (userId < 0) {
        callback(ResponseUtil::error(1002, "unauthorized"));
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        callback(ResponseUtil::error(1001, "request body must be JSON"));
        return;
    }

    long fileId = (*json).get("file_id", 0).asInt64();
    if (fileId <= 0) {
        callback(ResponseUtil::error(1001, "file_id is required"));
        return;
    }

    // Verify file is in recycle bin
    auto db = drogon::app().getDbClient("cloud_drive");
    try {
        auto result = db->execSqlSync(
            "SELECT id, file_path, file_hash, is_dir FROM file WHERE id = ? AND user_id = ? AND is_deleted = 1",
            fileId, userId);

        if (result.empty()) {
            callback(ResponseUtil::error(1004, "file not found in recycle bin"));
            return;
        }

        auto row = result[0];
        std::string filePath = row["file_path"].isNull() ? "" : row["file_path"].as<std::string>();
        std::string fileHash = row["file_hash"].isNull() ? "" : row["file_hash"].as<std::string>();
        bool isDir = row["is_dir"].isNull() ? false : (row["is_dir"].as<int>() != 0);

        // Delete from database
        if (!FileModel::hardDeleteFile(fileId, userId)) {
            callback(ResponseUtil::error(9001, "permanent delete failed"));
            return;
        }

        // Delete physical file (only if not a directory and file exists)
        if (!isDir && !filePath.empty() && fs::exists(filePath)) {
            // Check if other files reference the same hash
            bool otherReferences = false;
            try {
                auto refResult = db->execSqlSync(
                    "SELECT COUNT(*) as cnt FROM file WHERE file_hash = ? AND is_deleted = 0 AND id != ?",
                    fileHash, fileId);
                if (!refResult.empty() && refResult[0]["cnt"].as<int>() > 0) {
                    otherReferences = true;
                }
            } catch (...) {}

            if (!otherReferences) {
                fs::remove(filePath);
            }
        }

        callback(ResponseUtil::success(Json::Value(), "file permanently deleted"));

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Permanent delete failed: " << e.base().what();
        callback(ResponseUtil::error(9001, "internal error"));
    }
}

// DELETE /api/recycle/empty
void RecycleController::emptyRecycleBin(const drogon::HttpRequestPtr& req,
                                          std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    long userId = AuthMiddleware::getUserId(req);
    if (userId < 0) {
        callback(ResponseUtil::error(1002, "unauthorized"));
        return;
    }

    auto db = drogon::app().getDbClient("cloud_drive");
    try {
        // Get all deleted files for this user
        auto result = db->execSqlSync(
            "SELECT id, file_path, file_hash, is_dir FROM file WHERE user_id = ? AND is_deleted = 1",
            userId);

        int deletedCount = 0;
        for (auto row : result) {
            long fileId = row["id"].as<long long>();
            std::string filePath = row["file_path"].isNull() ? "" : row["file_path"].as<std::string>();
            std::string fileHash = row["file_hash"].isNull() ? "" : row["file_hash"].as<std::string>();
            bool isDir = row["is_dir"].isNull() ? false : (row["is_dir"].as<int>() != 0);

            // Hard delete from database
            if (FileModel::hardDeleteFile(fileId, userId)) {
                deletedCount++;

                // Delete physical file
                if (!isDir && !filePath.empty() && fs::exists(filePath)) {
                    bool otherReferences = false;
                    try {
                        auto refResult = db->execSqlSync(
                            "SELECT COUNT(*) as cnt FROM file WHERE file_hash = ? AND is_deleted = 0",
                            fileHash);
                        if (!refResult.empty() && refResult[0]["cnt"].as<int>() > 0) {
                            otherReferences = true;
                        }
                    } catch (...) {}

                    if (!otherReferences) {
                        fs::remove(filePath);
                    }
                }
            }
        }

        Json::Value data;
        data["deleted_count"] = deletedCount;
        callback(ResponseUtil::success(data, "recycle bin emptied"));

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Empty recycle bin failed: " << e.base().what();
        callback(ResponseUtil::error(9001, "internal error"));
    }
}

} // namespace CloudDrive
