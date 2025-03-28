#ifndef STATUSSERVICEIMPL_H
#define STATUSSERVICEIMPL_H
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <string>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;

struct ChatServer {
    std::string _host;
    std::string _port;
};

class StatusServiceImpl final : public StatusService::Service {
public:
    StatusServiceImpl();

    virtual Status GetChatServer(ServerContext* context,
         const GetChatServerReq* request, GetChatServerRsp* response) override;
public:
    std::vector<ChatServer> _servers;
    int _server_index;
};



#endif // STATUSSERVICEIMPL_H