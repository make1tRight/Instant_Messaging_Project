#include "CServer.h"
#include "HttpConnection.h"
#include "LogicSystem.h"
#include "ConfigMgr.h"

int main() {
    try {
        ConfigMgr cfg;
        std::string gate_port_str = cfg["GateServer"]["Port"];
        unsigned short gate_port = atoi(gate_port_str.c_str());
        asio::io_context ioc{1};

        asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {
            if (error) {
                return;
            }
            ioc.stop();
        });
        std::make_shared<CServer>(ioc, gate_port)->Start();
        std::cout << "GateServer listening on port: " << gate_port << std::endl;
        ioc.run();
    }
    catch (std::exception& e) {
        std::cout << "main() Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}