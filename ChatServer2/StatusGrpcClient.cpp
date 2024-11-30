#include "StatusGrpcClient.h"

GetChatServerRsp StatusGrpcClient::GetChatServer(int uid) {
	ClientContext context;
	GetChatServerRsp reply;
	GetChatServerReq request;
	request.set_uid(uid);
	auto stub = pool_->getConnection();
	Status status = stub->GetChatServer(&context, request, &reply);	//����reply���ز�ͬ�����
	Defer defer([&stub, this]() {	//�����Զ���������, ��Ϊ�������������������������ﶨ����������
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
