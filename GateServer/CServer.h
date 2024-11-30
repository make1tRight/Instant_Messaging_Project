#pragma once
#include "const.h"


class CServer : public std::enable_shared_from_this<CServer>
{
public:
    //io_context是上下文的意思, 这里用来通信的(建立连接)
    CServer(boost::asio::io_context& ioc, unsigned short& port);
    void Start();

private:
    tcp::acceptor m_acceptor;
    net::io_context& m_ioc;
    //tcp::socket m_socket;
};