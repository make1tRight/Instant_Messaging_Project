#include "VarifyGrpcClient.h"
#include "const.h"


GetVarifyRsp VarifyGrpcClient::GetVarifyCode(std::string email) {
    ClientContext context;
    GetVarifyReq request;
    GetVarifyRsp response;
    request.set_email(email);
    
    Status status = _stub->GetVarifyCode(&context, request, &response);
    if (!status.ok()) {
        response.set_error(ERROR_CODES::RPC_FAILED);
    }
    return response;
}

VarifyGrpcClient::VarifyGrpcClient() {
    std::shared_ptr<Channel> channel = grpc::CreateChannel(
        "0.0.0.0:50051", grpc::InsecureChannelCredentials());
    _stub = VarifyService::NewStub(channel);
}