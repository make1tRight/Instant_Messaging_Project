#include "ConfigMgr.h"
#include "RedisMgr.h"
#include <grpcpp/grpcpp.h>
#include <boost/asio.hpp>
#include "StatusServiceImpl.h"

void RunServer() {
    std::shared_ptr<ConfigMgr> cfg = ConfigMgr::GetInstance();
    std::string host = (*cfg)["StatusServer"]["Host"];
    std::string port = (*cfg)["StatusServer"]["Port"];
    std::string server_address = host + ":" + port;
    
    StatusServiceImpl service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on: " << server_address << std::endl;

    boost::asio::io_context ioc;
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&server](boost::system::error_code ec, int) {
        if (!ec) {
            std::cout << "Shutting down server." << std::endl;
            server->Shutdown();
        }
    });

    std::thread([&ioc]() {
        ioc.run();
    }).detach();

    // 阻塞当前线程, 直到server->Shutdown()被调用
    server->Wait();
    ioc.stop();
}


int main() {
    try {
        RunServer();
    }
    catch (std::exception& e) {
        std::cout << "main() Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}