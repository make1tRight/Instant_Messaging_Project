#ifndef CHATSERVICEIMPL_H
#define CHATSERVICEIMPL_H
#include "const.h"

using grpc::Channel;
using grpc::Status;
using grpc::ServerContext;

using message::ChatService;
using message::AddFriendReq;
using message::AddFriendRsp;
using message::AuthFriendReq;
using message::AuthFriendRsp;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;

class UserInfo;
class ChatServiceImpl final : public ChatService::Service {
public:
    ChatServiceImpl();
    virtual Status NotifyAddFriend(ServerContext* context,
         const AddFriendReq* request, AddFriendRsp* response) override;
    virtual Status NotifyAuthFriend(ServerContext* context,
         const AuthFriendReq* request, AuthFriendRsp* response) override;
    virtual Status NotifyTextChatMsg(ServerContext* context,
         const TextChatMsgReq* request, TextChatMsgRsp* response) override;

    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
private:
    
};
#endif // CHATSERVICEIMPL_H