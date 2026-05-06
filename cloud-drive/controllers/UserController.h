#ifndef CLOUD_DRIVE_USER_CONTROLLER_H
#define CLOUD_DRIVE_USER_CONTROLLER_H

#include <drogon/HttpController.h>

namespace CloudDrive {

class UserController : public drogon::HttpController<UserController> {
public:
    METHOD_LIST_BEGIN
        // User registration (no auth)
        ADD_METHOD_TO(UserController::registerUser, "/api/user/register", drogon::Post, drogon::Options, "CloudDrive::RateLimitMiddleware");
        // User login (no auth)
        ADD_METHOD_TO(UserController::login, "/api/user/login", drogon::Post, drogon::Options, "CloudDrive::RateLimitMiddleware");
        // Refresh token (no auth, uses refresh_token in Authorization)
        ADD_METHOD_TO(UserController::refreshToken, "/api/user/refresh", drogon::Post, drogon::Options);
        // Logout (requires auth)
        ADD_METHOD_TO(UserController::logout, "/api/user/logout", drogon::Post, drogon::Options, "CloudDrive::AuthMiddleware");
        // Get user info (requires auth)
        ADD_METHOD_TO(UserController::getUserInfo, "/api/user/info", drogon::Get, drogon::Options, "CloudDrive::AuthMiddleware", "CloudDrive::RateLimitMiddleware");
        // Update user info (requires auth)
        ADD_METHOD_TO(UserController::updateUserInfo, "/api/user/info", drogon::Put, drogon::Options, "CloudDrive::AuthMiddleware");
        // Change password (requires auth)
        ADD_METHOD_TO(UserController::changePassword, "/api/user/password", drogon::Put, drogon::Options, "CloudDrive::AuthMiddleware");
    METHOD_LIST_END

    // Registration
    void registerUser(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Login
    void login(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Refresh access token
    void refreshToken(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Logout
    void logout(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Get user info
    void getUserInfo(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Update user info
    void updateUserInfo(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Change password
    void changePassword(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace CloudDrive

#endif
