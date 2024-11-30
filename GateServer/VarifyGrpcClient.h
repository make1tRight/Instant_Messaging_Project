#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"

using grpc::Channel;
using grpc::Status;                 //状态
using grpc::ClientContext;          //上下文
using message::GetVarifyReq;        //request
using message::GetVarifyRsp;        //response
using message::VarifyService;       //认证服务

class RPCConnPool {
public:
    RPCConnPool(size_t poolsize, std::string host, std::string port);
    ~RPCConnPool();
    void Close();   //[10-44:11]告知其他模块, 要关闭连接了

    std::unique_ptr<VarifyService::Stub> getConnection();
    void returnConnection(std::unique_ptr<VarifyService::Stub> context);    //回收连接

private:
    std::atomic<bool> b_stop_; //ai生成的代码一般在后面加下划线
    size_t poolSize_;
    std::string host_;
    std::string port_;
    std::queue<std::unique_ptr<VarifyService::Stub>> connections_;
    std::condition_variable cond_;
    std::mutex mutex_;
};

class VarifyGrpcClient : public Singleton<VarifyGrpcClient>
{
    friend class Singleton<VarifyGrpcClient>;
public:
    GetVarifyRsp GetVarifyCode(std::string email);
private:
    VarifyGrpcClient();
    //std::unique_ptr<VarifyService::Stub> m_stub; //Stub是一个媒介
    std::unique_ptr<RPCConnPool> pool_;
};