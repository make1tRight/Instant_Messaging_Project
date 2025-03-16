#include "CServer.h"
#include "HttpConnection.h"
#include <iostream>
#include "AsioIOContextPool.h"

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port)
    : _ioc(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(), port)), _socket(ioc) {

}

void CServer::Start() {
    auto self = shared_from_this();
    auto& ioc = AsioIOContextPool::GetInstance()->GetIOContext();
    auto new_conn = std::make_shared<HttpConnection>(ioc);
    _acceptor.async_accept(new_conn->GetSocket(), [self, new_conn](beast::error_code ec) {
        try {
            if (ec) {
                self->Start();
                return;
            }
            new_conn->Start();
            self->Start();
        }
        catch (std::exception& e) {
            std::cout << "CServer::Start() Exception: " << e.what() << std::endl;
            // self->Start();
        }
    });
}