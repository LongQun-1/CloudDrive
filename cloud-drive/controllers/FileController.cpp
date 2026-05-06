#include "FileController.h"
#include "middlewares/AuthMiddleware.h"
#include "models/File.h"
#include "models/User.h"
#include "utils/ResponseUtil.h"
#include "utils/FileUtil.h"
#include "utils/ValidatorUtil.h"
#include "utils/HashUtil.h"
#include "plugins/RedisPlugin.h"
#include <drogon/drogon.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace CloudDrive {

namespace fs = std::filesystem;

// Helper: get query parameter with default value (drogon 1.9.12 getParameter has no default)
static std::string getParam(const drogon::HttpRequestPtr& req, const std::string& key, const std::string& defaultValue = "") {
    auto& params = req->getParameters();
    auto it = params.find(key);
    if (it != params.end() && !it->second.empty()) {
        return it->second;
    }
    return defaultValue;
}

// POST /api/file/folder
void FileController::createFolder(const drogon::HttpRequestPtr& req,
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

    std::string folderName = (*json).get("folder_name", "").asString();
    long parentId = (*json).get("parent_id", 0).asInt64();

    if (folderName.empty() || !ValidatorUtil::isValidFilename(folderName)) {
        callback(ResponseUtil::error(1001, "invalid folder name"));
        return;
    }

    // Check if folder with same name exists
    if (FileModel::fileExistsInDir(userId, parentId, folderName)) {
        callback(ResponseUtil::error(3001, "file or folder with same name already exists"));
        return;
    }

    auto [code, message] = FileModel::createFile(userId, parentId, folderName, "", 0, "", true);
    if (code != 0) {
        callback(ResponseUtil::error(code, message));
        return;
    }

    LOG_INFO << "Folder created: " << folderName << " by user " << userId;
    callback(ResponseUtil::success(Json::Value(), "folder created"));
}

// POST /api/file/upload
// Simple upload: client sends file as raw binary body, metadata in query params or JSON
void FileController::uploadFile(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    long userId = AuthMiddleware::getUserId(req);
    if (userId < 0) {
        callback(ResponseUtil::error(1002, "unauthorized"));
        return;
    }

    // Get file data from request body
    std::string_view body = req->getBody();
    if (body.empty()) {
        callback(ResponseUtil::error(1001, "no file data in request body"));
        return;
    }

    // Get filename and parent_id from query parameters
    std::string filename = getParam(req, "filename", "");
    if (filename.empty() || !ValidatorUtil::isValidFilename(filename)) {
        filename = "unnamed_file";
    }
    long parentId = std::atol(getParam(req, "parent_id", "0").c_str());

    size_t fileSize = body.size();

    // Check storage limit
    auto userInfo = UserModel::getUserById(userId);
    long currentUsed = FileModel::calculateStorageUsed(userId);
    if (currentUsed + (long)fileSize > userInfo.storageLimit) {
        callback(ResponseUtil::error(3002, "storage limit exceeded"));
        return;
    }

    // Calculate hash
    std::string fileHash = HashUtil::sha256(reinterpret_cast<const unsigned char*>(body.data()), body.size());

    // Check for dedup (same hash already uploaded by this user)
    auto existingFile = FileModel::findFileByHash(userId, fileHash);
    if (existingFile.id > 0) {
        // Instant upload: reuse existing file's storage path
        auto [code, message] = FileModel::createFile(userId, parentId, filename, fileHash, (long)fileSize, existingFile.filePath, false);
        if (code == 0) {
            FileModel::incrementRefCount(fileHash);
            UserModel::updateStorageUsed(userId, (long)fileSize);

            Json::Value data;
            data["instant_upload"] = true;
            data["file_hash"] = fileHash;
            callback(ResponseUtil::success(data, "file uploaded (instant)"));
            return;
        }
    }

    // Save to disk
    auto& config = drogon::app().getCustomConfig();
    std::string storageDir = config.get("storage", Json::Value()).get("path", "/data/cloud-drive/files").asString();
    std::string storagePath = FileUtil::getStoragePath(storageDir, fileHash);

    if (!FileUtil::ensureDir(fs::path(storagePath).parent_path().string())) {
        callback(ResponseUtil::error(9001, "failed to create storage directory"));
        return;
    }

    // Write file
    std::ofstream ofs(storagePath, std::ios::binary);
    if (!ofs.is_open()) {
        callback(ResponseUtil::error(9001, "failed to save file"));
        return;
    }
    ofs.write(body.data(), body.size());
    ofs.close();

    // Create file record
    auto [code, message] = FileModel::createFile(userId, parentId, filename, fileHash, (long)fileSize, storagePath, false);
    if (code != 0) {
        fs::remove(storagePath);
        callback(ResponseUtil::error(code, message));
        return;
    }

    // Update user storage
    UserModel::updateStorageUsed(userId, (long)fileSize);

    Json::Value data;
    data["file_hash"] = fileHash;
    data["file_size"] = (Json::Value::Int64)fileSize;
    data["instant_upload"] = false;
    callback(ResponseUtil::success(data, "file uploaded"));
}

// POST /api/file/chunk/init
void FileController::initChunkedUpload(const drogon::HttpRequestPtr& req,
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

    std::string filename = (*json).get("filename", "").asString();
    long fileSize = (*json).get("file_size", 0).asInt64();
    std::string fileHash = (*json).get("file_hash", "").asString();
    int totalChunks = (*json).get("total_chunks", 0).asInt();
    long parentId = (*json).get("parent_id", 0).asInt64();

    if (filename.empty() || fileSize <= 0 || totalChunks <= 0) {
        callback(ResponseUtil::error(1001, "invalid parameters"));
        return;
    }

    // Check storage limit
    auto userInfo = UserModel::getUserById(userId);
    long currentUsed = FileModel::calculateStorageUsed(userId);
    if (currentUsed + fileSize > userInfo.storageLimit) {
        callback(ResponseUtil::error(3002, "storage limit exceeded"));
        return;
    }

    // Check for instant upload
    if (!fileHash.empty()) {
        auto existingFile = FileModel::findFileByHash(userId, fileHash);
        if (existingFile.id > 0) {
            auto [code, message] = FileModel::createFile(userId, parentId, filename, fileHash, fileSize, existingFile.filePath, false);
            if (code == 0) {
                FileModel::incrementRefCount(fileHash);
                UserModel::updateStorageUsed(userId, fileSize);

                Json::Value data;
                data["instant_upload"] = true;
                data["file_hash"] = fileHash;
                callback(ResponseUtil::success(data, "file uploaded (instant)"));
                return;
            }
        }
    }

    // Generate upload ID
    std::string uploadId = HashUtil::md5(std::to_string(userId) + filename + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));

    auto db = drogon::app().getDbClient("cloud_drive");
    try {
        db->execSqlSync(
            "INSERT INTO chunk (upload_id, user_id, file_hash, filename, file_size, total_chunks, parent_id) "
            "VALUES (?, ?, ?, ?, ?, ?, ?)",
            uploadId, userId, fileHash.empty() ? "" : fileHash, filename, fileSize, totalChunks, parentId);
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Init chunked upload failed: " << e.base().what();
        callback(ResponseUtil::error(9001, "internal error"));
        return;
    }

    // Create tmp directory for chunks
    auto& config = drogon::app().getCustomConfig();
    std::string tmpDir = config.get("storage", Json::Value()).get("tmp_path", "/data/cloud-drive/tmp").asString();
    FileUtil::ensureDir(FileUtil::getUploadTmpDir(tmpDir, uploadId));

    Json::Value data;
    data["upload_id"] = uploadId;
    data["chunk_size"] = 5 * 1024 * 1024; // 5MB per chunk
    data["instant_upload"] = false;
    callback(ResponseUtil::success(data, "chunked upload initialized"));
}

// POST /api/file/chunk/upload
// Chunk data sent as raw binary body, metadata in query params
void FileController::uploadChunk(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    long userId = AuthMiddleware::getUserId(req);
    if (userId < 0) {
        callback(ResponseUtil::error(1002, "unauthorized"));
        return;
    }

    std::string uploadId = getParam(req, "upload_id", "");
    int chunkIndex = std::atoi(getParam(req, "chunk_index", "-1").c_str());

    if (uploadId.empty() || chunkIndex < 0) {
        callback(ResponseUtil::error(1001, "upload_id and chunk_index are required"));
        return;
    }

    // Get chunk data from body
    std::string_view body = req->getBody();
    if (body.empty()) {
        callback(ResponseUtil::error(1001, "no chunk data in request body"));
        return;
    }

    // Verify upload task
    auto db = drogon::app().getDbClient("cloud_drive");
    try {
        auto result = db->execSqlSync(
            "SELECT user_id, uploaded_chunks, is_completed FROM chunk WHERE upload_id = ?",
            uploadId);

        if (result.empty()) {
            callback(ResponseUtil::error(3003, "upload task not found"));
            return;
        }

        auto row0 = result[0];
        if (row0["user_id"].as<long long>() != userId) {
            callback(ResponseUtil::error(1003, "forbidden"));
            return;
        }

        if (row0["is_completed"].as<int>() != 0) {
            callback(ResponseUtil::error(3004, "upload already completed"));
            return;
        }

        // Save chunk to disk
        auto& config = drogon::app().getCustomConfig();
        std::string tmpDir = config.get("storage", Json::Value()).get("tmp_path", "/data/cloud-drive/tmp").asString();
        std::string chunkPath = FileUtil::getChunkPath(tmpDir, uploadId, chunkIndex);

        std::ofstream ofs(chunkPath, std::ios::binary);
        if (!ofs.is_open()) {
            callback(ResponseUtil::error(9001, "failed to save chunk"));
            return;
        }
        ofs.write(body.data(), body.size());
        ofs.close();

        // Update uploaded_chunks
        std::string uploadedChunks = row0["uploaded_chunks"].isNull() ? "" : row0["uploaded_chunks"].as<std::string>();
        if (!uploadedChunks.empty()) uploadedChunks += ",";
        uploadedChunks += std::to_string(chunkIndex);

        db->execSqlSync(
            "UPDATE chunk SET uploaded_chunks = ? WHERE upload_id = ?",
            uploadedChunks, uploadId);

        Json::Value data;
        data["chunk_index"] = chunkIndex;
        data["uploaded_chunks"] = uploadedChunks;
        callback(ResponseUtil::success(data, "chunk uploaded"));

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Upload chunk failed: " << e.base().what();
        callback(ResponseUtil::error(9001, "internal error"));
    }
}

// POST /api/file/chunk/complete
void FileController::completeChunkedUpload(const drogon::HttpRequestPtr& req,
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

    std::string uploadId = (*json).get("upload_id", "").asString();
    if (uploadId.empty()) {
        callback(ResponseUtil::error(1001, "upload_id is required"));
        return;
    }

    auto db = drogon::app().getDbClient("cloud_drive");
    try {
        auto result = db->execSqlSync(
            "SELECT user_id, file_hash, filename, file_size, total_chunks, uploaded_chunks, parent_id "
            "FROM chunk WHERE upload_id = ? AND is_completed = 0",
            uploadId);

        if (result.empty()) {
            callback(ResponseUtil::error(3003, "upload task not found or already completed"));
            return;
        }

        auto row = result[0];
        if (row["user_id"].as<long long>() != userId) {
            callback(ResponseUtil::error(1003, "forbidden"));
            return;
        }

        std::string fileHash = row["file_hash"].isNull() ? "" : row["file_hash"].as<std::string>();
        std::string filename = row["filename"].as<std::string>();
        long fileSize = row["file_size"].as<long long>();
        int totalChunks = row["total_chunks"].as<int>();
        long parentId = row["parent_id"].isNull() ? 0 : row["parent_id"].as<long long>();
        std::string uploadedChunks = row["uploaded_chunks"].isNull() ? "" : row["uploaded_chunks"].as<std::string>();

        // Count uploaded chunks
        int uploadedCount = 0;
        if (!uploadedChunks.empty()) {
            std::istringstream iss(uploadedChunks);
            std::string token;
            while (std::getline(iss, token, ',')) {
                if (!token.empty()) uploadedCount++;
            }
        }

        if (uploadedCount < totalChunks) {
            callback(ResponseUtil::error(3005, "not all chunks uploaded: " + std::to_string(uploadedCount) + "/" + std::to_string(totalChunks)));
            return;
        }

        // Merge chunks
        auto& config = drogon::app().getCustomConfig();
        std::string storageDir = config.get("storage", Json::Value()).get("path", "/data/cloud-drive/files").asString();
        std::string tmpDir = config.get("storage", Json::Value()).get("tmp_path", "/data/cloud-drive/tmp").asString();

        // Calculate hash from chunks if not provided
        if (fileHash.empty()) {
            fileHash = HashUtil::sha256FromFile(tmpDir, uploadId, totalChunks);
        }

        std::string storagePath = FileUtil::getStoragePath(storageDir, fileHash);
        if (!FileUtil::ensureDir(fs::path(storagePath).parent_path().string())) {
            callback(ResponseUtil::error(9001, "failed to create storage directory"));
            return;
        }

        // Merge chunks into final file
        {
            std::ofstream ofs(storagePath, std::ios::binary);
            if (!ofs.is_open()) {
                callback(ResponseUtil::error(9001, "failed to create merged file"));
                return;
            }

            for (int i = 0; i < totalChunks; i++) {
                std::string chunkPath = FileUtil::getChunkPath(tmpDir, uploadId, i);
                std::ifstream ifs(chunkPath, std::ios::binary);
                if (!ifs.is_open()) {
                    ofs.close();
                    fs::remove(storagePath);
                    callback(ResponseUtil::error(9001, "chunk file missing: " + std::to_string(i)));
                    return;
                }
                ofs << ifs.rdbuf();
                ifs.close();
                fs::remove(chunkPath); // Clean up chunk
            }
        }

        // Create file record
        auto [code, message] = FileModel::createFile(userId, parentId, filename, fileHash, fileSize, storagePath, false);
        if (code != 0) {
            fs::remove(storagePath);
            callback(ResponseUtil::error(code, message));
            return;
        }

        // Mark upload as completed
        db->execSqlSync("UPDATE chunk SET is_completed = 1 WHERE upload_id = ?", uploadId);

        // Clean up tmp directory
        std::string uploadTmpDir = FileUtil::getUploadTmpDir(tmpDir, uploadId);
        fs::remove_all(uploadTmpDir);

        // Update user storage
        UserModel::updateStorageUsed(userId, fileSize);

        Json::Value data;
        data["file_hash"] = fileHash;
        data["file_size"] = (Json::Value::Int64)fileSize;
        callback(ResponseUtil::success(data, "file uploaded successfully"));

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Complete chunked upload failed: " << e.base().what();
        callback(ResponseUtil::error(9001, "internal error"));
    }
}

// GET /api/file/download/{fileId}
void FileController::downloadFile(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                   long fileId) {
    long userId = AuthMiddleware::getUserId(req);
    if (userId < 0) {
        callback(ResponseUtil::error(1002, "unauthorized"));
        return;
    }

    auto fileInfo = FileModel::getFileById(fileId, userId);
    if (fileInfo.id == 0) {
        callback(ResponseUtil::error(1004, "file not found"));
        return;
    }

    if (fileInfo.isDir) {
        callback(ResponseUtil::error(1001, "cannot download a folder"));
        return;
    }

    if (fileInfo.filePath.empty() || !fs::exists(fileInfo.filePath)) {
        callback(ResponseUtil::error(9002, "file data not found on disk"));
        return;
    }

    // Send file
    auto resp = drogon::HttpResponse::newFileResponse(fileInfo.filePath, fileInfo.filename);
    callback(resp);
}

// GET /api/file/list?parent_id=0&page=1&page_size=50
void FileController::listFiles(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    long userId = AuthMiddleware::getUserId(req);
    if (userId < 0) {
        callback(ResponseUtil::error(1002, "unauthorized"));
        return;
    }

    long parentId = std::atol(getParam(req, "parent_id", "0").c_str());
    int page = std::atoi(getParam(req, "page", "1").c_str());
    int pageSize = std::atoi(getParam(req, "page_size", "50").c_str());

    if (page < 1) page = 1;
    if (pageSize < 1 || pageSize > 100) pageSize = 50;

    auto files = FileModel::listFiles(userId, parentId, page, pageSize);
    int total = FileModel::countFiles(userId, parentId);

    Json::Value fileList(Json::arrayValue);
    for (const auto& f : files) {
        fileList.append(f.toJson());
    }

    callback(ResponseUtil::paginated(fileList, total, page, pageSize));
}

// GET /api/file/info/{fileId}
void FileController::getFileInfo(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                  long fileId) {
    long userId = AuthMiddleware::getUserId(req);
    if (userId < 0) {
        callback(ResponseUtil::error(1002, "unauthorized"));
        return;
    }

    auto fileInfo = FileModel::getFileById(fileId, userId);
    if (fileInfo.id == 0) {
        callback(ResponseUtil::error(1004, "file not found"));
        return;
    }

    callback(ResponseUtil::success(fileInfo.toJson()));
}

// PUT /api/file/rename
void FileController::renameFile(const drogon::HttpRequestPtr& req,
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
    std::string newName = (*json).get("new_name", "").asString();

    if (fileId <= 0 || newName.empty()) {
        callback(ResponseUtil::error(1001, "file_id and new_name are required"));
        return;
    }

    if (!ValidatorUtil::isValidFilename(newName)) {
        callback(ResponseUtil::error(1001, "invalid filename"));
        return;
    }

    // Check file exists and belongs to user
    auto fileInfo = FileModel::getFileById(fileId, userId);
    if (fileInfo.id == 0) {
        callback(ResponseUtil::error(1004, "file not found"));
        return;
    }

    // Check duplicate name in same directory
    if (FileModel::fileExistsInDir(userId, fileInfo.parentId, newName)) {
        callback(ResponseUtil::error(3001, "file with same name already exists in this directory"));
        return;
    }

    if (!FileModel::renameFile(fileId, userId, newName)) {
        callback(ResponseUtil::error(9001, "rename failed"));
        return;
    }

    callback(ResponseUtil::success(Json::Value(), "file renamed"));
}

// PUT /api/file/move
void FileController::moveFile(const drogon::HttpRequestPtr& req,
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
    long newParentId = (*json).get("new_parent_id", 0).asInt64();

    if (fileId <= 0) {
        callback(ResponseUtil::error(1001, "file_id is required"));
        return;
    }

    // Can't move to self
    if (fileId == newParentId) {
        callback(ResponseUtil::error(1001, "cannot move file to itself"));
        return;
    }

    // Verify source file
    auto fileInfo = FileModel::getFileById(fileId, userId);
    if (fileInfo.id == 0) {
        callback(ResponseUtil::error(1004, "file not found"));
        return;
    }

    // Verify target directory exists (if not root)
    if (newParentId != 0) {
        auto targetDir = FileModel::getFileById(newParentId, userId);
        if (targetDir.id == 0 || !targetDir.isDir) {
            callback(ResponseUtil::error(1004, "target directory not found"));
            return;
        }
    }

    // Check duplicate name in target directory
    if (FileModel::fileExistsInDir(userId, newParentId, fileInfo.filename)) {
        callback(ResponseUtil::error(3001, "file with same name already exists in target directory"));
        return;
    }

    if (!FileModel::moveFile(fileId, userId, newParentId)) {
        callback(ResponseUtil::error(9001, "move failed"));
        return;
    }

    callback(ResponseUtil::success(Json::Value(), "file moved"));
}

// DELETE /api/file/delete
void FileController::deleteFile(const drogon::HttpRequestPtr& req,
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

    // Support single or batch delete
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

    // Update storage used before deleting
    long totalSize = 0;
    for (long fileId : fileIds) {
        auto fileInfo = FileModel::getFileById(fileId, userId);
        if (fileInfo.id > 0) {
            totalSize += fileInfo.fileSize;
        }
    }

    if (!FileModel::softDeleteFiles(fileIds, userId)) {
        callback(ResponseUtil::error(9001, "delete failed"));
        return;
    }

    // Update user storage (subtract deleted files size)
    if (totalSize > 0) {
        UserModel::updateStorageUsed(userId, -totalSize);
    }

    callback(ResponseUtil::success(Json::Value(), "file(s) moved to recycle bin"));
}

// GET /api/file/search?keyword=xxx&page=1&page_size=50
void FileController::searchFiles(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    long userId = AuthMiddleware::getUserId(req);
    if (userId < 0) {
        callback(ResponseUtil::error(1002, "unauthorized"));
        return;
    }

    std::string keyword = getParam(req, "keyword", "");
    int page = std::atoi(getParam(req, "page", "1").c_str());
    int pageSize = std::atoi(getParam(req, "page_size", "50").c_str());

    if (keyword.empty()) {
        callback(ResponseUtil::error(1001, "keyword is required"));
        return;
    }

    if (page < 1) page = 1;
    if (pageSize < 1 || pageSize > 100) pageSize = 50;

    auto files = FileModel::searchFiles(userId, keyword, page, pageSize);
    int total = FileModel::countSearchFiles(userId, keyword);

    Json::Value fileList(Json::arrayValue);
    for (const auto& f : files) {
        fileList.append(f.toJson());
    }

    callback(ResponseUtil::paginated(fileList, total, page, pageSize));
}

// GET /api/file/chunk/progress/{uploadId}
void FileController::getUploadProgress(const drogon::HttpRequestPtr& req,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                        const std::string& uploadId) {
    long userId = AuthMiddleware::getUserId(req);
    if (userId < 0) {
        callback(ResponseUtil::error(1002, "unauthorized"));
        return;
    }

    auto db = drogon::app().getDbClient("cloud_drive");
    try {
        auto result = db->execSqlSync(
            "SELECT user_id, filename, file_size, total_chunks, uploaded_chunks, is_completed "
            "FROM chunk WHERE upload_id = ?",
            uploadId);

        if (result.empty()) {
            callback(ResponseUtil::error(3003, "upload task not found"));
            return;
        }

        auto row = result[0];
        if (row["user_id"].as<long long>() != userId) {
            callback(ResponseUtil::error(1003, "forbidden"));
            return;
        }

        std::string uploadedChunks = row["uploaded_chunks"].isNull() ? "" : row["uploaded_chunks"].as<std::string>();

        // Count uploaded chunks
        int uploadedCount = 0;
        if (!uploadedChunks.empty()) {
            std::istringstream iss(uploadedChunks);
            std::string token;
            while (std::getline(iss, token, ',')) {
                if (!token.empty()) uploadedCount++;
            }
        }

        int totalChunks = row["total_chunks"].as<int>();

        Json::Value data;
        data["upload_id"] = uploadId;
        data["filename"] = row["filename"].as<std::string>();
        data["file_size"] = (Json::Value::Int64)row["file_size"].as<long long>();
        data["total_chunks"] = totalChunks;
        data["uploaded_chunks"] = uploadedCount;
        data["is_completed"] = row["is_completed"].as<int>() != 0;
        data["progress"] = totalChunks > 0 ? (uploadedCount * 100 / totalChunks) : 0;

        callback(ResponseUtil::success(data));

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Get upload progress failed: " << e.base().what();
        callback(ResponseUtil::error(9001, "internal error"));
    }
}

} // namespace CloudDrive
