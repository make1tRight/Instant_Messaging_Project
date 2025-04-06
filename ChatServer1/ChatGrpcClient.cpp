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
        // 多个stub底层复用同一个channel, 底层是同一个tcp连接
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
        _pools[(*cfg)[word]["Name"]] = 
            std::make_unique<ChatConnPool>(5, (*cfg)[word]["Host"], (*cfg)[word]["Port"]);
    }
}

AddFriendRsp ChatGrpcClient::NotifyAddFriend(std::string server_ip, const AddFriendReq& req) {
    AddFriendRsp rsp;
    Defer defer([&rsp, &req]() {
        rsp.set_error(ERROR_CODES::SUCCESS);
        rsp.set_applyuid(req.applyuid());
        rsp.set_touid(req.touid());
    });

    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        return rsp;
    }

    auto& _pool = find_iter->second;
    ClientContext context;
    auto stub = _pool->GetConnection();
    Status status = stub->NotifyAddFriend(&context, req, &rsp);
    Defer deferconn([&_pool, this, &stub]() {
        _pool->ReturnConnection(std::move(stub));
    });
    if (!status.ok()) {
        rsp.set_error(ERROR_CODES::RPC_FAILED);
        return rsp;
    }
    return rsp;
}

AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req) {
    AuthFriendRsp rsp;
    rsp.set_error(ERROR_CODES::SUCCESS);
    Defer defer([&rsp, &req]() {
        rsp.set_fromuid(req.fromuid());
        rsp.set_touid(req.touid());
    }); 
    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        rsp.set_error(ERROR_CODES::RPC_FAILED);
        return rsp;
    }
    ClientContext context;
    std::unique_ptr<ChatConnPool>& pool = find_iter->second;
    auto stub = pool->GetConnection();
    Status status = stub->NotifyAuthFriend(&context, req, &rsp);
    Defer deferconn([&pool, this, &stub]() {
        pool->ReturnConnection(std::move(stub));
    });
    if (!status.ok()) {
        rsp.set_error(ERROR_CODES::RPC_FAILED);
        return rsp;
    }
    return rsp;
}

bool ChatGrpcClient::GetBaseInfo(std::string base_key,
     int uid, std::shared_ptr<UserInfo>& userinfo) {
    return true;
}

TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(std::string server_ip, 
    const TextChatMsgReq& req, const Json::Value& rtvalue) {
    TextChatMsgRsp rsp;
    rsp.set_error(ERROR_CODES::SUCCESS);

    Defer defer([&rsp, &req]() {
        rsp.set_fromuid(req.fromuid());
        rsp.set_touid(req.touid());

        for (const TextChatData& txt_msg : req.testmsgs()) {
            TextChatData* new_msg = rsp.add_testmsgs();
            new_msg->set_msgid(txt_msg.msgid());
            new_msg->set_msgcontent(txt_msg.msgcontent());
        }
    });

    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        rsp.set_error(ERROR_CODES::RPC_FAILED);
        return rsp;
    }

    auto& pool = find_iter->second;
    ClientContext context;
    auto stub = pool->GetConnection();
    Status status = stub->NotifyTextChatMsg(&context, req, &rsp);
    Defer deferconn([&stub, &pool]() {
        pool->ReturnConnection(std::move(stub));
    });

    if (!status.ok()) {
        rsp.set_error(ERROR_CODES::RPC_FAILED);
    }
    return rsp;
}