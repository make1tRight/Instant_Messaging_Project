#include "LogicSystem.h"
#include "HttpConnection.h"

LogicSystem::LogicSystem() {
    RegisterGet("/get_test", [](std::shared_ptr<HttpConnection> conn) {
        beast::ostream(conn->_response.body()) << "get request received.";
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

void LogicSystem::RegisterGet(std::string url, HttpHandler handler) {
    _get_handler.insert(std::make_pair(url, handler));
}
