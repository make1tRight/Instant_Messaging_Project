#include "VarifyGrpcClient.h"
#include "const.h"
#include "ConfigMgr.h"

std::unique_ptr<VarifyService::Stub> RPConnPool::GetConnection() {
    std::unique_lock<std::mutex> lock(_mtx);
    _cond.wait(lock, [this]() {
        if (_b_stop) {
            return true;
        }
        return !_conns.empty();
    });
    if (_b_stop) {
        return nullptr;
    }
    auto conn = std::move(_conns.front());
    _conns.pop();
    return conn;
}

void RPConnPool::ReturnConnection(std::unique_ptr<VarifyService::Stub> conn) {
    std::unique_lock<std::mutex> lock(_mtx);
    if (_b_stop) {
        return;
    }
    _conns.push(std::move(conn));
    _cond.notify_one();
}

RPConnPool::~RPConnPool() {
    Stop();
}

void RPConnPool::Stop() {
    std::lock_guard<std::mutex> lock(_mtx);
    _b_stop = true;

    while (!_conns.empty()) {
        _conns.pop();
    }
    _cond.notify_all();
}

RPConnPool::RPConnPool(std::size_t size, std::string host, std::string port)
     : _max_poolsize(size), _host(host), _port(port), _b_stop(false)  {
    for (int i = 0; i < size; ++i) {
        std::shared_ptr<Channel>  channel = 
            grpc::CreateChannel(_host + ":" +_port, grpc::InsecureChannelCredentials());
        _conns.push(VarifyService::NewStub(channel));
    }
}

GetVarifyRsp VarifyGrpcClient::GetVarifyCode(std::string email) {
    ClientContext context;
    GetVarifyReq request;
    GetVarifyRsp response;
    request.set_email(email);
    auto stub = _pool->GetConnection();

    Status status = stub->GetVarifyCode(&context, request, &response);
    if (!status.ok()) {
        response.set_error(ERROR_CODES::RPC_FAILED);
    }
    _pool->ReturnConnection(std::move(stub));
    return response;
}

VarifyGrpcClient::VarifyGrpcClient() {
    auto cfg = ConfigMgr::GetInstance();
    std::string host = (*cfg)["VarifyServer"]["Host"];
    std::string port = (*cfg)["VarifyServer"]["Port"];
    // std::shared_ptr<Channel> channel = grpc::CreateChannel(
    //     "0.0.0.0:50051", grpc::InsecureChannelCredentials());
    // _stub = VarifyService::NewStub(channel);
    std::cout << "host=" << host << ", port=" << port << std::endl;
    _pool.reset(new RPConnPool(5, host, port));
}