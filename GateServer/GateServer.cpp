#include "CServer.h"
#include "HttpConnection.h"
#include "LogicSystem.h"

int main() {
    try {
        unsigned short port = static_cast<unsigned short>(8080);
        asio::io_context ioc{1};

        asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {
            if (error) {
                return;
            }
            ioc.stop();
        });
        std::make_shared<CServer>(ioc, port)->Start();
        std::cout << "GateServer listening on port: " << port << std::endl;
        ioc.run();
    }
    catch (std::exception& e) {
        std::cout << "main() Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}