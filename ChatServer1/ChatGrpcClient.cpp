#include "ChatGrpcClient.h"
#include <grpcpp/grpcpp.h>
#include "ConfigMgr.h"
#include <istream>
#include "data.h"

ChatConnPool::ChatConnPool(std::size_t poolSize, std::string host, std::string port)
     : _poolSize(poolSize), _host(host), _port(port) {
    for (int i = 0; i < _poolSize; ++i) {
        std::shared_ptr<grpc::Channel> channel(
            grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials()));
        _conns.push(ChatService::NewStub(channel));
    }
}

ChatConnPool::~ChatConnPool() {
    std::lock_guard<std::mutex> lock(_mutex);
    Close();
    while (!_conns.empty()) {
        _conns.pop();
    }
}

std::unique_ptr<ChatService::Stub> ChatConnPool::GetConnection() {
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock, [this]() {
        if (_b_stop) {
            return true;
        }
        return !_conns.empty();
    });
    if (_b_stop) {
        return nullptr;
    }
    std::unique_ptr<ChatService::Stub> conn(std::move(_conns.front()));
    _conns.pop();
    return conn;
}

void ChatConnPool::ReturnConnection(std::unique_ptr<ChatService::Stub> conn) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_b_stop) {
        return;
    }
    _conns.push(std::move(conn));
    _cond.notify_one();
}

void ChatConnPool::Close() {
    _b_stop = true;
    _cond.notify_all();
}

ChatGrpcClient::~ChatGrpcClient() {

}

ChatGrpcClient::ChatGrpcClient() {
    auto cfg = ConfigMgr::GetInstance();
    auto server_list = (*cfg)["PeerServer"]["Servers"];
    std::stringstream ss(server_list);
    std::string word;
    std::vector<std::string> words;
    while (std::getline(ss, word, ',')) {
        words.push_back(word);
    }
    for (auto& word : words) {
        if ((*cfg)[word]["Name"].empty()) {
            continue;
        }
        _pool[(*cfg)[word]["Name"]] = 
            std::make_unique<ChatConnPool>(5, (*cfg)[word]["Host"], (*cfg)[word]["Port"]);
    }
}

AddFriendRsp ChatGrpcClient::NotifyAddFriend(std::string server_ip, const AddFriendReq& req) {
    AddFriendRsp rsp;
    return rsp;
}

AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req) {
    AuthFriendRsp rsp;
    return rsp;
}

bool ChatGrpcClient::GetBaseInfo(std::string base_key,
     int uid, std::shared_ptr<UserInfo>& userinfo) {
    return true;
}

TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(std::string server_ip, 
    const TextChatMsgReq& req, const Json::Value& rtvalue) {
    TextChatMsgRsp rsp;
    return rsp;
}