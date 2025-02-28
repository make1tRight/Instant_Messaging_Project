#pragma once
#include "const.h"
#include "CSession.h"


class CServer : public std::enable_shared_from_this<CServer>
{
public:
    //io_context是上下文的意思, 这里用来通信的(建立连接)
    CServer(net::io_context& ioc, unsigned short port);
    ~CServer();
    void Start();
    void ClearSession(std::string);
private:
    void HandleAccept(std::shared_ptr<CSession> new_session, const boost::system::error_code& error);
    void StartAccept();
    net::io_context& m_ioc;
    unsigned short m_port;
    tcp::acceptor m_acceptor;
    std::map<std::string, std::shared_ptr<CSession>> m_sessions;
    std::mutex m_mutex;
};