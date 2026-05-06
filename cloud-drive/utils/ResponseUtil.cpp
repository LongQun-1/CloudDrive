#include "ResponseUtil.h"
#include <random>
#include <sstream>
#include <iomanip>

namespace CloudDrive {

std::string ResponseUtil::generateRequestId() {
    uuid_t uuid;
    char uuid_str[37];
    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_str);
    return std::string(uuid_str);
}

Json::Value ResponseUtil::makeBase(int code, const std::string& message, const std::string& requestId) {
    Json::Value root;
    root["code"] = code;
    root["message"] = message;
    root["request_id"] = requestId.empty() ? generateRequestId() : requestId;
    return root;
}

drogon::HttpResponsePtr ResponseUtil::success(const Json::Value& data, const std::string& message) {
    Json::Value root = makeBase(0, message, "");
    root["data"] = data;
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(root);
    resp->setStatusCode(drogon::k200OK);
    return resp;
}

drogon::HttpResponsePtr ResponseUtil::error(int code, const std::string& message, const std::string& requestId) {
    Json::Value root = makeBase(code, message, requestId);
    root["data"] = Json::Value(Json::objectValue);
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(root);
    
    if (code == 1002) {
        resp->setStatusCode(drogon::k401Unauthorized);
    } else if (code == 1003) {
        resp->setStatusCode(drogon::k403Forbidden);
    } else if (code == 1004) {
        resp->setStatusCode(drogon::k404NotFound);
    } else if (code == 1006) {
        resp->setStatusCode(drogon::k429TooManyRequests);
    } else if (code >= 9001) {
        resp->setStatusCode(drogon::k500InternalServerError);
    } else {
        resp->setStatusCode(drogon::k400BadRequest);
    }
    
    return resp;
}

drogon::HttpResponsePtr ResponseUtil::paginated(const Json::Value& data,
                                                  int total,
                                                  int page,
                                                  int pageSize,
                                                  const std::string& message) {
    Json::Value root = makeBase(0, message, "");
    root["data"] = data;
    root["total"] = total;
    root["page"] = page;
    root["page_size"] = pageSize;
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(root);
    resp->setStatusCode(drogon::k200OK);
    return resp;
}

} // namespace CloudDrive
