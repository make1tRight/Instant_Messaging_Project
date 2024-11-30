#pragma once
#include "const.h"

class HttpConnection; //��������ֹѭ������
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    bool HandleGet(std::string, std::shared_ptr<HttpConnection>);   //����get����
    bool HandlePost(std::string, std::shared_ptr<HttpConnection>);   //����post����
    void RegGet(std::string, HttpHandler handler);      //ע��get����
    void RegPost(std::string, HttpHandler handler);     //post����
private:
    LogicSystem();
    std::map<std::string, HttpHandler> m_postHandlers;
    std::map<std::string, HttpHandler> m_getHandlers;
};