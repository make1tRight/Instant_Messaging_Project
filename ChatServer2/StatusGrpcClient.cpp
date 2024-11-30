#include "StatusGrpcClient.h"

GetChatServerRsp StatusGrpcClient::GetChatServer(int uid) {
	ClientContext context;
	GetChatServerRsp reply;
	GetChatServerReq request;
	request.set_uid(uid);
	auto stub = pool_->getConnection();
	Status status = stub->GetChatServer(&context, request, &reply);	//根据reply返回不同的情况
	Defer defer([&stub, this]() {	//用完自动返回连接, 因为他的析构函数就是我们在这里定义的这个东西
		pool_->returnConnection(std::move(stub));
	});
	if (status.ok()) {
		return reply;
	}
	else {
		reply.set_error(ErrorCodes::RPC_FAILED);
		return reply;
	}
}

StatusGrpcClient::StatusGrpcClient() {
	auto& gCfgMgr = ConfigMgr::Inst();
	std::string host = gCfgMgr["StatusServer"]["Host"];
	std::string port = gCfgMgr["StatusServer"]["Port"];
	pool_.reset(new StatusConPool(5, host, port));
}
