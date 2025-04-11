#include "StatusGrpcClient.h"
#include <grpcpp/grpcpp.h>
#include "ConfigMgr.h"


StatusConnPool::~StatusConnPool() {
    std::unique_lock<std::mutex> lock(_mutex);
    Close();
    while (!_pool.empty()) {
        _pool.pop();
    }
}

std::unique_ptr<StatusService::Stub> StatusConnPool::GetConnection() {
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock, [this]() {
        if (_b_stop) {
            return true;
        }
        return !_pool.empty();
    });
    if (_b_stop) {
        return nullptr;
    }
    std::unique_ptr<StatusService::Stub> conn(std::move(_pool.front()));
    if (conn == nullptr) {
        std::cout << "Failed to get conn." << std::endl;
        return nullptr;
    }
    _pool.pop();
    return conn;
}

void StatusConnPool::ReturnConnection(std::unique_ptr<StatusService::Stub> conn) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_b_stop) {
        return;
    }
    _pool.push(std::move(conn));
    _cond.notify_one();
}


void StatusConnPool::Close() {
    _b_stop = true;
    _cond.notify_all();
}

StatusConnPool::StatusConnPool(int poolSize, std::string host, std::string port) 
     : _poolSize(poolSize), _host(host), _port(port), _b_stop(false) {
    for (int i = 0; i < _poolSize; ++i) {
        std::shared_ptr<Channel> channel = 
            grpc::CreateChannel(host+":"+port, grpc::InsecureChannelCredentials());
        _pool.push(StatusService::NewStub(channel));
    }
}

GetChatServerRsp StatusGrpcClient::GetChatServer(int uid) {
    ClientContext context;
    GetChatServerReq request;
    GetChatServerRsp response;

    request.set_uid(uid);
    auto stub = _pool->GetConnection();
    Status status = stub->GetChatServer(&context, request, &response);
    Defer defer([&stub, this]() {
        _pool->ReturnConnection(std::move(stub));
    });

    if (!status.ok()) {
        response.set_error(ERROR_CODES::RPC_FAILED);
    }
    return response;
}

StatusGrpcClient::~StatusGrpcClient() {

}

StatusGrpcClient::StatusGrpcClient() {
    std::shared_ptr<ConfigMgr> cfg = ConfigMgr::GetInstance();
    std::string host = (*cfg)["StatusServer"]["Host"];
    std::string port = (*cfg)["StatusServer"]["Port"];
    _pool.reset(new StatusConnPool(5, host, port));
}
