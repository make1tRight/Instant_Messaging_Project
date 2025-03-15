#include "CServer.h"
#include <iostream>

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port)
    : _ioc(ioc), _socket(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(), port))  {

}

void CServer::Start() {
    auto self = shared_from_this();

    _acceptor.async_accept([&self](const boost::system::error_code& ec) {
        try {
            if (ec) {
                self->Start();
                return;
            }

            // session->start();
            self->Start();
        }
        catch (std::exception& e) {
            std::cout << "Exception: " << e.what() << std::endl;
            self->Start();
        }
    });
}