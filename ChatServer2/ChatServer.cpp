#include "CServer.h"
#include "LogicSystem.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "AsioIOContextPool.h"
#include "ChatServiceImpl.h"
#include <boost/asio.hpp>
#include <grpcpp/grpcpp.h>

int main() {
    std::shared_ptr<ConfigMgr> cfg = ConfigMgr::GetInstance();
    std::string server_name = (*cfg)["SelfServer"]["Name"];
    try {
        std::shared_ptr<AsioIOContextPool> pool = AsioIOContextPool::GetInstance();
        std::string server_address =
             (*cfg)["SelfServer"]["Host"] + ":" + (*cfg)["SelfServer"]["RPCPort"];
        
        RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");
        ChatServiceImpl service;
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        std::unique_ptr<grpc::Server> grpc_server(builder.BuildAndStart());

        std::thread grpc_server_thread([&grpc_server]() {
            grpc_server->Wait();
        });

        // tcp服务创建的逻辑
        boost::asio::io_context ioc;
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([pool, &ioc, &grpc_server] (boost::system::error_code, int) {
            ioc.stop();
            pool->Stop();
            grpc_server->Shutdown();
        });
        std::string port = (*cfg)["SelfServer"]["Port"];
        CServer server(ioc, atoi(port.c_str()));
        ioc.run();

        RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
        RedisMgr::GetInstance()->Close();
        grpc_server_thread.join();
    }
    catch (std::exception& e) {
        std::cout << "main() Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}