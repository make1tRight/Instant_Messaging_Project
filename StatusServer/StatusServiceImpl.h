#ifndef STATUSSERVICEIMPL_H
#define STATUSSERVICEIMPL_H
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <string>
#include <mutex>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;

using message::LoginReq;
using message::LoginRsp;

struct ChatServer {
    ChatServer();
    ChatServer(const ChatServer& cs);
    ChatServer& operator=(const ChatServer& cs);

    std::string _host;
    std::string _port;
    std::string _name;
    int _conn_count;
};

class StatusServiceImpl final : public StatusService::Service {
public:
    StatusServiceImpl();

    virtual Status GetChatServer(ServerContext* context,
         const GetChatServerReq* request, GetChatServerRsp* response) override;
private:
    void insertToken(int uid, std::string token);
    ChatServer getChatServer();
    std::unordered_map<std::string, ChatServer> _servers;
    std::mutex _mutex;
};
#endif // STATUSSERVICEIMPL_H