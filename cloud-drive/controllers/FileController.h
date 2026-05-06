#ifndef CLOUD_DRIVE_FILE_CONTROLLER_H
#define CLOUD_DRIVE_FILE_CONTROLLER_H

#include <drogon/HttpController.h>

namespace CloudDrive {

class FileController : public drogon::HttpController<FileController> {
public:
    METHOD_LIST_BEGIN
        // Create folder
        ADD_METHOD_TO(FileController::createFolder, "/api/file/folder", drogon::Post, drogon::Options, "CloudDrive::AuthMiddleware");
        // Upload file (simple upload)
        ADD_METHOD_TO(FileController::uploadFile, "/api/file/upload", drogon::Post, drogon::Options, "CloudDrive::AuthMiddleware");
        // Init chunked upload
        ADD_METHOD_TO(FileController::initChunkedUpload, "/api/file/chunk/init", drogon::Post, drogon::Options, "CloudDrive::AuthMiddleware");
        // Upload chunk
        ADD_METHOD_TO(FileController::uploadChunk, "/api/file/chunk/upload", drogon::Post, drogon::Options, "CloudDrive::AuthMiddleware");
        // Complete chunked upload (merge)
        ADD_METHOD_TO(FileController::completeChunkedUpload, "/api/file/chunk/complete", drogon::Post, drogon::Options, "CloudDrive::AuthMiddleware");
        // Download file
        ADD_METHOD_TO(FileController::downloadFile, "/api/file/download/{fileId}", drogon::Get, drogon::Options, "CloudDrive::AuthMiddleware", "CloudDrive::RateLimitMiddleware");
        // List files in directory
        ADD_METHOD_TO(FileController::listFiles, "/api/file/list", drogon::Get, drogon::Options, "CloudDrive::AuthMiddleware");
        // Get file info
        ADD_METHOD_TO(FileController::getFileInfo, "/api/file/info/{fileId}", drogon::Get, drogon::Options, "CloudDrive::AuthMiddleware");
        // Rename file
        ADD_METHOD_TO(FileController::renameFile, "/api/file/rename", drogon::Put, drogon::Options, "CloudDrive::AuthMiddleware");
        // Move file
        ADD_METHOD_TO(FileController::moveFile, "/api/file/move", drogon::Put, drogon::Options, "CloudDrive::AuthMiddleware");
        // Delete file (soft delete to recycle bin)
        ADD_METHOD_TO(FileController::deleteFile, "/api/file/delete", drogon::Delete, drogon::Options, "CloudDrive::AuthMiddleware");
        // Search files
        ADD_METHOD_TO(FileController::searchFiles, "/api/file/search", drogon::Get, drogon::Options, "CloudDrive::AuthMiddleware");
        // Get upload progress
        ADD_METHOD_TO(FileController::getUploadProgress, "/api/file/chunk/progress/{uploadId}", drogon::Get, drogon::Options, "CloudDrive::AuthMiddleware");
    METHOD_LIST_END

    // Create folder
    void createFolder(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Upload file (simple, small files)
    void uploadFile(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Init chunked upload
    void initChunkedUpload(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Upload a chunk
    void uploadChunk(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Complete chunked upload
    void completeChunkedUpload(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Download file
    void downloadFile(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       long fileId);

    // List files in directory
    void listFiles(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Get file info
    void getFileInfo(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      long fileId);

    // Rename file
    void renameFile(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Move file
    void moveFile(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Delete file
    void deleteFile(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Search files
    void searchFiles(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Get upload progress
    void getUploadProgress(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                            const std::string& uploadId);
};

} // namespace CloudDrive

#endif
