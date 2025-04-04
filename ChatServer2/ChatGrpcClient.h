#ifndef CHATGRPCCLIENT_H
#define CHATGRPCCLIENT_H
#include <memory>
#include "const.h"
#include <queue>
#include <string>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <unordered_map>
#include "Singleton.h"
#include <jsoncpp/json/json.h>
using grpc::Channel;

using message::ChatService;
using message::AddFriendReq;
using message::AddFriendRsp;
using message::AuthFriendReq;
using message::AuthFriendRsp;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;


class UserInfo;
class ChatConnPool {
public:
    ChatConnPool(std::size_t poolSize, std::string host, std::string port);
    ~ChatConnPool();

    std::unique_ptr<ChatService::Stub> GetConnection();
    void ReturnConnection(std::unique_ptr<ChatService::Stub> conn);
    void Close();
private:
    std::size_t _poolSize;
    std::string _host;
    std::string _port;
    std::queue<std::unique_ptr<ChatService::Stub>> _conns;
    std::mutex _mutex;
    std::condition_variable _cond;
    // atomic线程安全, 写入以后所有线程立即可见
    std::atomic<bool> _b_stop;
};

class ChatGrpcClient : public Singleton<ChatGrpcClient> {
    friend class Singleton<ChatGrpcClient>;
public:
    ~ChatGrpcClient();

    AddFriendRsp NotifyAddFriend(std::string server_ip, const AddFriendReq& request);
    AuthFriendRsp NotifyAuthFriend(std::string server_ip, const AuthFriendReq& request);
    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
    TextChatMsgRsp NotifyTextChatMsg(std::string server_ip,
         const TextChatMsgReq& request, const Json::Value& rtvalue);

private:
    ChatGrpcClient();
    std::unordered_map<std::string, std::unique_ptr<ChatConnPool>> _pool;
};
#endif // CHATGRPCCLIENT_H