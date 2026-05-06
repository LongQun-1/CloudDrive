#ifndef CLOUD_DRIVE_SHARE_CONTROLLER_H
#define CLOUD_DRIVE_SHARE_CONTROLLER_H

#include <drogon/HttpController.h>

namespace CloudDrive {

class ShareController : public drogon::HttpController<ShareController> {
public:
    METHOD_LIST_BEGIN
        // Create share link
        ADD_METHOD_TO(ShareController::createShare, "/api/share/create", drogon::Post, drogon::Options, "CloudDrive::AuthMiddleware");
        // Verify and access share (public - no auth)
        ADD_METHOD_TO(ShareController::verifyShare, "/api/share/verify", drogon::Post, drogon::Options);
        // Get share info (public - no auth)
        ADD_METHOD_TO(ShareController::getShareInfo, "/api/share/info/{shareUrl}", drogon::Get, drogon::Options);
        // List shared folder files (public - no auth, needs code if required)
        ADD_METHOD_TO(ShareController::listSharedFiles, "/api/share/files/{shareUrl}", drogon::Get, drogon::Options);
        // Download shared file (public - no auth, needs code if required)
        ADD_METHOD_TO(ShareController::downloadSharedFile, "/api/share/download/{shareUrl}", drogon::Get, drogon::Options, "CloudDrive::RateLimitMiddleware");
        // List my shares
        ADD_METHOD_TO(ShareController::listMyShares, "/api/share/list", drogon::Get, drogon::Options, "CloudDrive::AuthMiddleware");
        // Cancel share
        ADD_METHOD_TO(ShareController::cancelShare, "/api/share/cancel", drogon::Put, drogon::Options, "CloudDrive::AuthMiddleware");
    METHOD_LIST_END

    // POST /api/share/create
    void createShare(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // POST /api/share/verify
    void verifyShare(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // GET /api/share/info/{shareUrl}
    void getShareInfo(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const std::string& shareUrl);

    // GET /api/share/files/{shareUrl}?code=xxx&parent_id=0
    void listSharedFiles(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                          const std::string& shareUrl);

    // GET /api/share/download/{shareUrl}?code=xxx&file_id=0
    void downloadSharedFile(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                             const std::string& shareUrl);

    // GET /api/share/list?page=1&page_size=50
    void listMyShares(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // PUT /api/share/cancel
    void cancelShare(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace CloudDrive

#endif
