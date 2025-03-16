#include "LogicSystem.h"
#include "HttpConnection.h"
#include "const.h"
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>

LogicSystem::LogicSystem() {
    RegisterGet("/get_test", [](std::shared_ptr<HttpConnection> conn) {
        beast::ostream(conn->_response.body()) 
            << "get request received." << std::endl;
        int i = 0;
        for (const auto& elem : conn->_get_params) {
            ++i;
            beast::ostream(conn->_response.body()) 
                << "======Params[" << i << "]======" << std::endl; 
            beast::ostream(conn->_response.body()) << "key: " << elem.first 
                << ", value: " << elem.second << std::endl;
        }
    });

    RegisterPost("/get_varifycode", [](std::shared_ptr<HttpConnection> conn) {
        auto body_str = beast::buffers_to_string(conn->_request.body().data());
        std::cout << "body string: " << body_str << std::endl;
        conn->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value root_src;
        bool success = reader.parse(body_str, root_src);
        if (!success) {
            std::cout << "Failed to parse JSON data." << std::endl;
            root["error"] = ERROR_CODES::JSON_ERROR;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->_response.body()) << jsonstr;
            return;
        }
        auto email = root_src["email"].asString();
        std::cout << "email: " << email << std::endl;
        root["email"] = email;
        root["error"] = ERROR_CODES::SUCCESS;
        std::string jsonstr = root.toStyledString();
        beast::ostream(conn->_response.body()) << jsonstr;
    });
}

LogicSystem::~LogicSystem() {

}

bool LogicSystem::HandleGet(std::string path,
    std::shared_ptr<HttpConnection> conn) {
    if (_get_handler.find(path) == _get_handler.end()) {
        return false;
    }

    _get_handler[path](conn);
    return true;
}

bool LogicSystem::HandlePost(std::string path,
    std::shared_ptr<HttpConnection> conn) {
    if (_post_handler.find(path) == _post_handler.end()) {
        return false;
    }

    _post_handler[path](conn);
    return true;
}    

void LogicSystem::RegisterGet(std::string url, HttpHandler handler) {
    _get_handler.insert(std::make_pair(url, handler));
}

void LogicSystem::RegisterPost(std::string url, HttpHandler handler) {
    _post_handler.insert(std::make_pair(url, handler));
}
