#ifndef STATUSGRPCCLIENT_H
#define STATUSGRPCCLIENT_H
#include "const.h"
#include "Singleton.h"
#include <mutex>
#include <queue>
#include <string>
#include <condition_variable>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::StatusService;
using message::GetChatServerReq;
using message::GetChatServerRsp;

class StatusConnPool : public Singleton<StatusConnPool> {
    friend class Singleton<StatusConnPool>;
    friend class StatusGrpcClient;
public:
    ~StatusConnPool();

    std::unique_ptr<StatusService::Stub> GetConnection();
    void ReturnConnection(std::unique_ptr<StatusService::Stub> conn);

    void Close();
private:
    StatusConnPool(int poolSize, std::string host, std::string port);
    std::string _host;
    std::string _port;
    std::mutex _mutex;
    std::condition_variable _cond;
    std::size_t _poolSize;
    std::atomic<bool> _b_stop;
    std::queue<std::unique_ptr<StatusService::Stub>> _pool;
};

class StatusGrpcClient : public Singleton<StatusGrpcClient> {
    friend class Singleton<StatusGrpcClient>;
public:
    ~StatusGrpcClient();
    GetChatServerRsp GetChatServer(int uid);
private:
    StatusGrpcClient();
    std::unique_ptr<StatusConnPool> _pool;
};
#endif // STATUSGRPCCLIENT_H