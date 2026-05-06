#ifndef CLOUD_DRIVE_RESPONSE_UTIL_H
#define CLOUD_DRIVE_RESPONSE_UTIL_H

#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <string>
#include <uuid/uuid.h>

namespace CloudDrive {

class ResponseUtil {
public:
    // Generate a UUID request_id
    static std::string generateRequestId();

    // Create a success response
    static drogon::HttpResponsePtr success(const Json::Value& data = Json::Value(), 
                                            const std::string& message = "success");

    // Create an error response
    static drogon::HttpResponsePtr error(int code, 
                                          const std::string& message,
                                          const std::string& requestId = "");

    // Create a paginated response
    static drogon::HttpResponsePtr paginated(const Json::Value& data,
                                              int total,
                                              int page,
                                              int pageSize,
                                              const std::string& message = "success");

private:
    static Json::Value makeBase(int code, const std::string& message, const std::string& requestId);
};

} // namespace CloudDrive

#endif
