#include "VarifyGrpcClient.h"
#include "ConfigMgr.h"

RPCConnPool::RPCConnPool(size_t poolsize, std::string host, std::string port) : 
poolSize_(poolsize), host_(host), port_(port), b_stop_(false) {
    for (size_t i = 0; i < poolSize_; ++i) {
        std::shared_ptr<Channel> channel = grpc::CreateChannel(host_+ ":"+ port,
            grpc::InsecureChannelCredentials()); //创建一个Insecure的channel
        //auto m = VarifyService::NewStub(channel);
        //connections_.push(m); [10-42:06]这样写有什么问题
        connections_.push(VarifyService::NewStub(channel));
    }
}

RPCConnPool::~RPCConnPool() {
    std::lock_guard<std::mutex> lock(mutex_);
    Close();
    while (!connections_.empty()) {
        connections_.pop();
    }
}

std::unique_ptr<VarifyService::Stub> RPCConnPool::getConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this]() { //根据lambda表达式决定要不要解锁 [wait的用法看并发编程和网络编程]
        if (b_stop_) {
            return true;
        }
        return !connections_.empty();
    });

    if (b_stop_) {
        return nullptr;
    }
    auto context = std::move(connections_.front());
    connections_.pop();
    return context;
}

void RPCConnPool::returnConnection(std::unique_ptr<VarifyService::Stub> context) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (b_stop_) {
        return;
    }

    connections_.push(std::move(context));
    cond_.notify_one();
}

void RPCConnPool::Close() {     //[10-44:11]告知其他模块, 要关闭连接了
    b_stop_ = true;             //判断这个b_stop_
    cond_.notify_all();
}

GetVarifyRsp VarifyGrpcClient::GetVarifyCode(std::string email) {
    ClientContext context;
    GetVarifyRsp reply;
    GetVarifyReq request;
    request.set_email(email);
    auto stub = pool_->getConnection();

    Status status = stub->GetVarifyCode(&context, request, &reply);
    if (status.ok()) {
        pool_->returnConnection(std::move(stub));
        return reply;
    }
    else {
        pool_->returnConnection(std::move(stub));
        reply.set_error(ErrorCodes::RPC_FAILED);
        return reply;
    }
}

VarifyGrpcClient::VarifyGrpcClient() {
    //std::shared_ptr<Channel> channel = grpc::CreateChannel("127.0.0.1:50051",
    //    grpc::InsecureChannelCredentials()); //创建一个Insecure的channel
    //m_stub = VarifyService::NewStub(channel);//stub需要一个channel用于通信
    auto& gCfgMgr = ConfigMgr::Inst();
    std::string host = gCfgMgr["VarifyServer"]["Host"]; //和config.ini关联的
    std::string port = gCfgMgr["VarifyServer"]["Port"];
    pool_.reset(new RPCConnPool(5, host, port));
}