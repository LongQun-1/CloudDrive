#include "ShareController.h"
#include "middlewares/AuthMiddleware.h"
#include "models/Share.h"
#include "models/File.h"
#include "utils/ResponseUtil.h"
#include "utils/ValidatorUtil.h"
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

// POST /api/share/create
void ShareController::createShare(const drogon::HttpRequestPtr& req,
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
    bool needCode = (*json).get("need_code", true).asBool();
    std::string expireAt = (*json).get("expire_at", "").asString();
    int maxDownloadCount = (*json).get("max_download_count", 0).asInt();

    if (fileId <= 0) {
        callback(ResponseUtil::error(1001, "file_id is required"));
        return;
    }

    // Verify file exists and belongs to user
    auto fileInfo = FileModel::getFileById(fileId, userId);
    if (fileInfo.id == 0) {
        callback(ResponseUtil::error(1004, "file not found"));
        return;
    }

    auto [code, shareUrl] = ShareModel::createShare(userId, fileId, needCode, expireAt, maxDownloadCount);
    if (code != 0) {
        callback(ResponseUtil::error(code, shareUrl));
        return;
    }

    // Get the created share to return share_code
    auto shareInfo = ShareModel::getShareByUrl(shareUrl);

    Json::Value data;
    data["share_url"] = shareUrl;
    data["share_code"] = shareInfo.shareCode;
    data["need_code"] = shareInfo.needCode;
    callback(ResponseUtil::success(data, "share created"));
}

// POST /api/share/verify
void ShareController::verifyShare(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json) {
        callback(ResponseUtil::error(1001, "request body must be JSON"));
        return;
    }

    std::string shareUrl = (*json).get("share_url", "").asString();
    std::string shareCode = (*json).get("share_code", "").asString();

    if (shareUrl.empty()) {
        callback(ResponseUtil::error(1001, "share_url is required"));
        return;
    }

    auto shareInfo = ShareModel::getShareByUrl(shareUrl);
    if (shareInfo.id == 0) {
        callback(ResponseUtil::error(1004, "share not found"));
        return;
    }

    if (!ShareModel::isShareValid(shareInfo)) {
        callback(ResponseUtil::error(4001, "share has expired or been cancelled"));
        return;
    }

    // Verify code if needed
    if (shareInfo.needCode && shareCode != shareInfo.shareCode) {
        callback(ResponseUtil::error(4002, "invalid share code"));
        return;
    }

    // Increment view count
    ShareModel::incrementViewCount(shareUrl);

    // Get file info (don't expose full path)
    auto fileInfo = FileModel::getFileById(shareInfo.fileId, shareInfo.userId);

    Json::Value data;
    data["file_name"] = fileInfo.filename;
    data["file_size"] = (Json::Value::Int64)fileInfo.fileSize;
    data["is_dir"] = fileInfo.isDir;
    data["need_code"] = shareInfo.needCode;
    data["share_url"] = shareUrl;
    callback(ResponseUtil::success(data, "share verified"));
}

// GET /api/share/info/{shareUrl}
void ShareController::getShareInfo(const drogon::HttpRequestPtr& req,
                                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                    const std::string& shareUrl) {
    auto shareInfo = ShareModel::getShareByUrl(shareUrl);
    if (shareInfo.id == 0) {
        callback(ResponseUtil::error(1004, "share not found"));
        return;
    }

    if (!ShareModel::isShareValid(shareInfo)) {
        callback(ResponseUtil::error(4001, "share has expired or been cancelled"));
        return;
    }

    // Increment view count
    ShareModel::incrementViewCount(shareUrl);

    // Get file info (limited fields)
    auto fileInfo = FileModel::getFileById(shareInfo.fileId, shareInfo.userId);

    Json::Value data;
    data["file_name"] = fileInfo.filename;
    data["file_size"] = (Json::Value::Int64)fileInfo.fileSize;
    data["is_dir"] = fileInfo.isDir;
    data["need_code"] = shareInfo.needCode;
    data["share_url"] = shareUrl;
    data["file_id"] = (Json::Value::Int64)shareInfo.fileId;
    callback(ResponseUtil::success(data));
}

// GET /api/share/files/{shareUrl}?code=xxx&parent_id=0
void ShareController::listSharedFiles(const drogon::HttpRequestPtr& req,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                       const std::string& shareUrl) {
    auto shareInfo = ShareModel::getShareByUrl(shareUrl);
    if (shareInfo.id == 0) {
        callback(ResponseUtil::error(1004, "share not found"));
        return;
    }

    if (!ShareModel::isShareValid(shareInfo)) {
        callback(ResponseUtil::error(4001, "share has expired or been cancelled"));
        return;
    }

    // Verify code if needed
    if (shareInfo.needCode) {
        std::string code = getParam(req, "code", "");
        if (code != shareInfo.shareCode) {
            callback(ResponseUtil::error(4002, "invalid share code"));
            return;
        }
    }

    auto sharedFile = FileModel::getFileById(shareInfo.fileId, shareInfo.userId);
    if (sharedFile.id == 0 || !sharedFile.isDir) {
        callback(ResponseUtil::error(1004, "shared resource is not a folder"));
        return;
    }

    // parent_id query param: 0 = root of shared folder, >0 = subfolder inside
    long parentId = std::atol(getParam(req, "parent_id", "0").c_str());

    // If parent_id is 0, list children of the shared folder itself
    long listParentId = (parentId == 0) ? sharedFile.id : parentId;

    // Verify the parent_id is within the shared folder tree (belongs to the share owner)
    if (parentId > 0) {
        auto parentFile = FileModel::getFileById(parentId, shareInfo.userId);
        if (parentFile.id == 0) {
            callback(ResponseUtil::error(1004, "folder not found"));
            return;
        }
    }

    auto files = FileModel::listFiles(shareInfo.userId, listParentId, 1, 200);
    int total = FileModel::countFiles(shareInfo.userId, listParentId);

    Json::Value fileList(Json::arrayValue);
    for (auto file : files) {
        Json::Value item;
        item["id"] = (Json::Value::Int64)file.id;
        item["filename"] = file.filename;
        item["file_size"] = (Json::Value::Int64)file.fileSize;
        item["is_dir"] = file.isDir;
        item["parent_id"] = (Json::Value::Int64)file.parentId;
        item["updated_at"] = file.updatedAt;
        fileList.append(item);
    }

    Json::Value data;
    data["files"] = fileList;
    data["total"] = total;
    data["folder_name"] = (parentId == 0) ? sharedFile.filename : "";
    callback(ResponseUtil::success(data));
}

// GET /api/share/download/{shareUrl}?code=xxx&file_id=0
void ShareController::downloadSharedFile(const drogon::HttpRequestPtr& req,
                                          std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                          const std::string& shareUrl) {
    auto shareInfo = ShareModel::getShareByUrl(shareUrl);
    if (shareInfo.id == 0) {
        callback(ResponseUtil::error(1004, "share not found"));
        return;
    }

    if (!ShareModel::isShareValid(shareInfo)) {
        callback(ResponseUtil::error(4001, "share has expired or been cancelled"));
        return;
    }

    // Verify code if needed
    if (shareInfo.needCode) {
        std::string code = getParam(req, "code", "");
        if (code != shareInfo.shareCode) {
            callback(ResponseUtil::error(4002, "invalid share code"));
            return;
        }
    }

    // Get shared root file info
    auto sharedFile = FileModel::getFileById(shareInfo.fileId, shareInfo.userId);

    // Determine which file to download
    long downloadFileId = std::atol(getParam(req, "file_id", "0").c_str());

    if (sharedFile.isDir) {
        // For shared folders: file_id is required to specify which file to download
        if (downloadFileId <= 0) {
            callback(ResponseUtil::error(1001, "file_id is required for folder shares"));
            return;
        }
        auto fileInfo = FileModel::getFileById(downloadFileId, shareInfo.userId);
        if (fileInfo.id == 0 || fileInfo.isDir) {
            callback(ResponseUtil::error(1004, "file not found or is a directory"));
            return;
        }
        if (fileInfo.filePath.empty() || !fs::exists(fileInfo.filePath)) {
            callback(ResponseUtil::error(9002, "file data not found on disk"));
            return;
        }
        ShareModel::incrementDownloadCount(shareUrl);
        auto resp = drogon::HttpResponse::newFileResponse(fileInfo.filePath, fileInfo.filename);
        callback(resp);
    } else {
        // For shared single file: download the shared file itself
        if (sharedFile.id == 0) {
            callback(ResponseUtil::error(1004, "shared file not found"));
            return;
        }
        if (sharedFile.filePath.empty() || !fs::exists(sharedFile.filePath)) {
            callback(ResponseUtil::error(9002, "file data not found on disk"));
            return;
        }
        ShareModel::incrementDownloadCount(shareUrl);
        auto resp = drogon::HttpResponse::newFileResponse(sharedFile.filePath, sharedFile.filename);
        callback(resp);
    }
}

// GET /api/share/list?page=1&page_size=50
void ShareController::listMyShares(const drogon::HttpRequestPtr& req,
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

    auto shares = ShareModel::listShares(userId, page, pageSize);
    int total = ShareModel::countShares(userId);

    // Enrich with file names
    Json::Value shareList(Json::arrayValue);
    for (const auto& s : shares) {
        Json::Value item = s.toJson();
        auto fileInfo = FileModel::getFileById(s.fileId, s.userId);
        item["file_name"] = fileInfo.filename;
        shareList.append(item);
    }

    callback(ResponseUtil::paginated(shareList, total, page, pageSize));
}

// PUT /api/share/cancel
void ShareController::cancelShare(const drogon::HttpRequestPtr& req,
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

    long shareId = (*json).get("share_id", 0).asInt64();
    if (shareId <= 0) {
        callback(ResponseUtil::error(1001, "share_id is required"));
        return;
    }

    // Verify ownership
    auto shareInfo = ShareModel::getShareById(shareId, userId);
    if (shareInfo.id == 0) {
        callback(ResponseUtil::error(1004, "share not found"));
        return;
    }

    if (!ShareModel::cancelShare(shareId, userId)) {
        callback(ResponseUtil::error(9001, "cancel share failed"));
        return;
    }

    callback(ResponseUtil::success(Json::Value(), "share cancelled"));
}

} // namespace CloudDrive
