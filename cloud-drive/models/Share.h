#ifndef CLOUD_DRIVE_SHARE_MODEL_H
#define CLOUD_DRIVE_SHARE_MODEL_H

#include <string>
#include <vector>
#include <json/json.h>
#include <drogon/orm/DbClient.h>

namespace CloudDrive {

class ShareModel {
public:
    struct ShareInfo {
        long id{0};
        std::string shareUrl;
        std::string shareCode;
        long userId{0};
        long fileId{0};
        bool needCode{true};
        std::string expireAt;
        int maxDownloadCount{0};
        int viewCount{0};
        int downloadCount{0};
        int saveCount{0};
        bool isActive{true};
        std::string createdAt;

        Json::Value toJson() const {
            Json::Value json;
            json["id"] = (Json::Value::Int64)id;
            json["share_url"] = shareUrl;
            json["share_code"] = shareCode;
            json["user_id"] = (Json::Value::Int64)userId;
            json["file_id"] = (Json::Value::Int64)fileId;
            json["need_code"] = needCode;
            json["expire_at"] = expireAt;
            json["max_download_count"] = maxDownloadCount;
            json["view_count"] = viewCount;
            json["download_count"] = downloadCount;
            json["save_count"] = saveCount;
            json["is_active"] = isActive;
            json["created_at"] = createdAt;
            return json;
        }
    };

    // Create a share record
    static std::pair<int, std::string> createShare(long userId, long fileId,
                                                     bool needCode,
                                                     const std::string& expireAt,
                                                     int maxDownloadCount);

    // Get share by share_url
    static ShareInfo getShareByUrl(const std::string& shareUrl);

    // Get share by ID
    static ShareInfo getShareById(long shareId, long userId);

    // List shares by user
    static std::vector<ShareInfo> listShares(long userId, int page = 1, int pageSize = 50);

    // Count shares by user
    static int countShares(long userId);

    // Cancel share (set is_active = 0)
    static bool cancelShare(long shareId, long userId);

    // Increment view count
    static bool incrementViewCount(const std::string& shareUrl);

    // Increment download count
    static bool incrementDownloadCount(const std::string& shareUrl);

    // Check if share is valid (active, not expired, within download limit)
    static bool isShareValid(const ShareInfo& share);

    // Generate a random share URL
    static std::string generateShareUrl();

    // Generate a random share code (4 digits)
    static std::string generateShareCode();
};

} // namespace CloudDrive

#endif
