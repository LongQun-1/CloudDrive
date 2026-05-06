#include "Share.h"
#include <drogon/drogon.h>
#include <random>
#include <chrono>

namespace CloudDrive {

using namespace drogon::orm;

std::string ShareModel::generateShareUrl() {
    static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(chars) - 2);

    std::string result;
    result.reserve(12);
    for (int i = 0; i < 12; i++) {
        result += chars[dis(gen)];
    }
    return result;
}

std::string ShareModel::generateShareCode() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    return std::to_string(dis(gen));
}

std::pair<int, std::string> ShareModel::createShare(long userId, long fileId,
                                                      bool needCode,
                                                      const std::string& expireAt,
                                                      int maxDownloadCount) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return {9001, "database connection failed"};

    std::string shareUrl = generateShareUrl();
    std::string shareCode = needCode ? generateShareCode() : "";

    try {
        if (expireAt.empty()) {
            db->execSqlSync(
                "INSERT INTO share (share_url, share_code, user_id, file_id, need_code, expire_at, max_download_count) "
                "VALUES (?, ?, ?, ?, ?, NULL, ?)",
                shareUrl, shareCode, userId, fileId,
                needCode ? 1 : 0,
                maxDownloadCount);
        } else {
            db->execSqlSync(
                "INSERT INTO share (share_url, share_code, user_id, file_id, need_code, expire_at, max_download_count) "
                "VALUES (?, ?, ?, ?, ?, ?, ?)",
                shareUrl, shareCode, userId, fileId,
                needCode ? 1 : 0,
                expireAt,
                maxDownloadCount);
        }

        return {0, shareUrl};
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Create share failed: " << e.base().what();
        return {9001, "internal error"};
    }
}

ShareModel::ShareInfo ShareModel::getShareByUrl(const std::string& shareUrl) {
    ShareInfo info;
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return info;

    try {
        auto result = db->execSqlSync(
            "SELECT id, share_url, share_code, user_id, file_id, need_code, expire_at, "
            "max_download_count, view_count, download_count, save_count, is_active, "
            "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') as created_at "
            "FROM share WHERE share_url = ?",
            shareUrl);

        if (result.size() > 0) {
            auto row = result[0];
            info.id = row["id"].as<long long>();
            info.shareUrl = row["share_url"].as<std::string>();
            info.shareCode = row["share_code"].isNull() ? "" : row["share_code"].as<std::string>();
            info.userId = row["user_id"].as<long long>();
            info.fileId = row["file_id"].as<long long>();
            info.needCode = row["need_code"].isNull() ? false : (row["need_code"].as<int>() != 0);
            info.expireAt = row["expire_at"].isNull() ? "" : row["expire_at"].as<std::string>();
            info.maxDownloadCount = row["max_download_count"].isNull() ? 0 : row["max_download_count"].as<int>();
            info.viewCount = row["view_count"].isNull() ? 0 : row["view_count"].as<int>();
            info.downloadCount = row["download_count"].isNull() ? 0 : row["download_count"].as<int>();
            info.saveCount = row["save_count"].isNull() ? 0 : row["save_count"].as<int>();
            info.isActive = row["is_active"].isNull() ? false : (row["is_active"].as<int>() != 0);
            info.createdAt = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
        }
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Get share by url failed: " << e.base().what();
    }

    return info;
}

ShareModel::ShareInfo ShareModel::getShareById(long shareId, long userId) {
    ShareInfo info;
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return info;

    try {
        auto result = db->execSqlSync(
            "SELECT id, share_url, share_code, user_id, file_id, need_code, expire_at, "
            "max_download_count, view_count, download_count, save_count, is_active, "
            "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') as created_at "
            "FROM share WHERE id = ? AND user_id = ?",
            shareId, userId);

        if (result.size() > 0) {
            auto row = result[0];
            info.id = row["id"].as<long long>();
            info.shareUrl = row["share_url"].as<std::string>();
            info.shareCode = row["share_code"].isNull() ? "" : row["share_code"].as<std::string>();
            info.userId = row["user_id"].as<long long>();
            info.fileId = row["file_id"].as<long long>();
            info.needCode = row["need_code"].isNull() ? false : (row["need_code"].as<int>() != 0);
            info.expireAt = row["expire_at"].isNull() ? "" : row["expire_at"].as<std::string>();
            info.maxDownloadCount = row["max_download_count"].isNull() ? 0 : row["max_download_count"].as<int>();
            info.viewCount = row["view_count"].isNull() ? 0 : row["view_count"].as<int>();
            info.downloadCount = row["download_count"].isNull() ? 0 : row["download_count"].as<int>();
            info.saveCount = row["save_count"].isNull() ? 0 : row["save_count"].as<int>();
            info.isActive = row["is_active"].isNull() ? false : (row["is_active"].as<int>() != 0);
            info.createdAt = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
        }
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Get share by id failed: " << e.base().what();
    }

    return info;
}

std::vector<ShareModel::ShareInfo> ShareModel::listShares(long userId, int page, int pageSize) {
    std::vector<ShareInfo> shares;
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return shares;

    int offset = (page - 1) * pageSize;

    try {
        auto result = db->execSqlSync(
            "SELECT id, share_url, share_code, user_id, file_id, need_code, expire_at, "
            "max_download_count, view_count, download_count, save_count, is_active, "
            "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') as created_at "
            "FROM share WHERE user_id = ? AND is_active = 1 "
            "ORDER BY created_at DESC LIMIT ? OFFSET ?",
            userId, pageSize, offset);

        for (auto row : result) {
            ShareInfo info;
            info.id = row["id"].as<long long>();
            info.shareUrl = row["share_url"].as<std::string>();
            info.shareCode = row["share_code"].isNull() ? "" : row["share_code"].as<std::string>();
            info.userId = row["user_id"].as<long long>();
            info.fileId = row["file_id"].as<long long>();
            info.needCode = row["need_code"].isNull() ? false : (row["need_code"].as<int>() != 0);
            info.expireAt = row["expire_at"].isNull() ? "" : row["expire_at"].as<std::string>();
            info.maxDownloadCount = row["max_download_count"].isNull() ? 0 : row["max_download_count"].as<int>();
            info.viewCount = row["view_count"].isNull() ? 0 : row["view_count"].as<int>();
            info.downloadCount = row["download_count"].isNull() ? 0 : row["download_count"].as<int>();
            info.saveCount = row["save_count"].isNull() ? 0 : row["save_count"].as<int>();
            info.isActive = row["is_active"].isNull() ? false : (row["is_active"].as<int>() != 0);
            info.createdAt = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
            shares.push_back(std::move(info));
        }
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "List shares failed: " << e.base().what();
    }

    return shares;
}

int ShareModel::countShares(long userId) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return 0;

    try {
        auto result = db->execSqlSync(
            "SELECT COUNT(*) as cnt FROM share WHERE user_id = ? AND is_active = 1",
            userId);
        if (result.size() > 0) {
            return result[0]["cnt"].as<int>();
        }
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Count shares failed: " << e.base().what();
    }
    return 0;
}

bool ShareModel::cancelShare(long shareId, long userId) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return false;

    try {
        db->execSqlSync(
            "UPDATE share SET is_active = 0 WHERE id = ? AND user_id = ?",
            shareId, userId);
        return true;
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Cancel share failed: " << e.base().what();
        return false;
    }
}

bool ShareModel::incrementViewCount(const std::string& shareUrl) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return false;

    try {
        db->execSqlSync(
            "UPDATE share SET view_count = view_count + 1 WHERE share_url = ?",
            shareUrl);
        return true;
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Increment view count failed: " << e.base().what();
        return false;
    }
}

bool ShareModel::incrementDownloadCount(const std::string& shareUrl) {
    auto db = drogon::app().getDbClient("cloud_drive");
    if (!db) return false;

    try {
        db->execSqlSync(
            "UPDATE share SET download_count = download_count + 1 WHERE share_url = ?",
            shareUrl);
        return true;
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Increment download count failed: " << e.base().what();
        return false;
    }
}

bool ShareModel::isShareValid(const ShareInfo& share) {
    if (!share.isActive) return false;

    // Check expiration
    if (!share.expireAt.empty()) {
        // Compare with current time
        auto now = std::chrono::system_clock::now();
        auto now_time = std::chrono::system_clock::to_time_t(now);
        char now_str[20];
        std::strftime(now_str, sizeof(now_str), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
        if (share.expireAt < std::string(now_str)) {
            return false;
        }
    }

    // Check download limit
    if (share.maxDownloadCount > 0 && share.downloadCount >= share.maxDownloadCount) {
        return false;
    }

    return true;
}

} // namespace CloudDrive
