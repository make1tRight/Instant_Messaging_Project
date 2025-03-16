#ifndef VARIFYGRPCCLIENT_H
#define VARIFYGRPCCLIENT_H
#include "Singleton.h"
#include "message.grpc.pb.h"
#include <memory>
#include <atomic>
#include <queue>
#include <mutex>
#include <thread>
#include <string>
#include <condition_variable>
#include <grpcpp/grpcpp.h>
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::VarifyService;
using message::GetVarifyReq;
using message::GetVarifyRsp;

class RPConnPool : public Singleton<RPConnPool> {
    friend class Singleton<RPConnPool>;
    friend class VarifyGrpcClient;
public:
    std::unique_ptr<VarifyService::Stub> GetConnection();
    void ReturnConnection(std::unique_ptr<VarifyService::Stub> conn);
    ~RPConnPool();
    void Stop();
private:
    RPConnPool(std::size_t size, std::string host, std::string port);
    std::atomic<bool> _b_stop;
    std::size_t _max_poolsize;
    std::string _host;
    std::string _port;
    std::queue<std::unique_ptr<VarifyService::Stub>> _conns;
    std::mutex _mtx;
    std::condition_variable _cond;
};


class VarifyGrpcClient : public Singleton<VarifyGrpcClient> {
    friend class Singleton<VarifyGrpcClient>;
public:
    GetVarifyRsp GetVarifyCode(std::string email);
private:
    VarifyGrpcClient();
    std::unique_ptr<VarifyService::Stub> _stub;
    std::unique_ptr<RPConnPool> _pool;
};
#endif // VARIFYGRPCCLIENT_H