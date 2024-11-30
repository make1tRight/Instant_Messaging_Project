#pragma once
#include "const.h"

class HttpConnection; //先声明防止循环依赖
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    bool HandleGet(std::string, std::shared_ptr<HttpConnection>);   //处理get请求
    bool HandlePost(std::string, std::shared_ptr<HttpConnection>);   //处理post请求
    void RegGet(std::string, HttpHandler handler);      //注册get请求
    void RegPost(std::string, HttpHandler handler);     //post请求
private:
    LogicSystem();
    std::map<std::string, HttpHandler> m_postHandlers;
    std::map<std::string, HttpHandler> m_getHandlers;
};