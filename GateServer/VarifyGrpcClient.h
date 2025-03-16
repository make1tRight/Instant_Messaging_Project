#ifndef VARIFYGRPCCLIENT_H
#define VARIFYGRPCCLIENT_H
#include "Singleton.h"
#include "message.grpc.pb.h"
#include <memory>
#include <grpcpp/grpcpp.h>
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::VarifyService;
using message::GetVarifyReq;
using message::GetVarifyRsp;


class VarifyGrpcClient : public Singleton<VarifyGrpcClient> {
    friend class Singleton<VarifyGrpcClient>;
public:
    GetVarifyRsp GetVarifyCode(std::string email);
private:
    VarifyGrpcClient();
    std::unique_ptr<VarifyService::Stub> _stub;
};
#endif // VARIFYGRPCCLIENT_H