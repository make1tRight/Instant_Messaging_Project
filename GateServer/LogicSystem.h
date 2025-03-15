#ifndef LOGICSYSTEM_H
#define LOGICSYSTEM_H
#include "Singleton.h"
#include <functional>
#include <unordered_map>

class HttpConnection;
typedef std::function<
    void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem : public Singleton<LogicSystem> {
    friend class Singleton<LogicSystem>; //加了_instance才有权限new
public:
    ~LogicSystem();
    bool HandleGet(std::string path,
         std::shared_ptr<HttpConnection> conn);
    void RegisterGet(std::string url, HttpHandler handler);
private:
    LogicSystem();
    std::unordered_map<std::string, HttpHandler> _get_handler;
    std::unordered_map<std::string, HttpHandler> _post_handler;
};
#endif // LOGICSYSTEM_H