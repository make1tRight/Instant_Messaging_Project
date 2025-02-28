#pragma once
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/grpcpp.h>
#include "data.h"
#include <mutex>


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::ChatService;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;


class ChatServiceImpl final : public ChatService::Service
{
public:
	ChatServiceImpl();
    virtual Status NotifyAddFriend(::grpc::ServerContext* context,
        const AddFriendReq* request, AddFriendRsp* response) override;
    virtual Status NotifyAuthFriend(::grpc::ServerContext* context,
        const AuthFriendReq* request, AuthFriendRsp* response) override;
    virtual Status NotifyTextChatMsg(::grpc::ServerContext* context,
        const TextChatMsgReq* request, TextChatMsgRsp* response) override;
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>* userinfo);//查用户信息
private:
};

