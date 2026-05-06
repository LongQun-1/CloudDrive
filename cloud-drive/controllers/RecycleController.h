#ifndef CLOUD_DRIVE_RECYCLE_CONTROLLER_H
#define CLOUD_DRIVE_RECYCLE_CONTROLLER_H

#include <drogon/HttpController.h>

namespace CloudDrive {

class RecycleController : public drogon::HttpController<RecycleController> {
public:
    METHOD_LIST_BEGIN
        // List deleted files in recycle bin
        ADD_METHOD_TO(RecycleController::listRecycleBin, "/api/recycle/list", drogon::Get, drogon::Options, "CloudDrive::AuthMiddleware");
        // Restore file from recycle bin
        ADD_METHOD_TO(RecycleController::restoreFile, "/api/recycle/restore", drogon::Put, drogon::Options, "CloudDrive::AuthMiddleware");
        // Permanently delete file
        ADD_METHOD_TO(RecycleController::permanentDelete, "/api/recycle/delete", drogon::Delete, drogon::Options, "CloudDrive::AuthMiddleware");
        // Empty recycle bin
        ADD_METHOD_TO(RecycleController::emptyRecycleBin, "/api/recycle/empty", drogon::Delete, drogon::Options, "CloudDrive::AuthMiddleware");
    METHOD_LIST_END

    // GET /api/recycle/list?page=1&page_size=50
    void listRecycleBin(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // PUT /api/recycle/restore
    void restoreFile(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // DELETE /api/recycle/delete
    void permanentDelete(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // DELETE /api/recycle/empty
    void emptyRecycleBin(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace CloudDrive

#endif
