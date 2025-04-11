#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "RedisMgr.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

ChatServer::ChatServer()
     : _host(""), _port(""), _name(""), _conn_count(0) {

}

ChatServer::ChatServer(const ChatServer& cs)
     : _host(cs._host), _port(cs._port), _name(cs._name), _conn_count(cs._conn_count) {

}

ChatServer& ChatServer::operator=(const ChatServer& cs) {
    if (this == &cs) {
        return *this;
    }

    _host = cs._host;
    _port = cs._port;
    _name = cs._name;
    _conn_count = cs._conn_count;
    return *this;
}

std::string generate_unique_string() {
    boost::uuids::uuid uuid =  boost::uuids::random_generator()();
    std::string unique_string = boost::uuids::to_string(uuid);
    return unique_string;
}

StatusServiceImpl::StatusServiceImpl() {
    std::shared_ptr<ConfigMgr> cfg = ConfigMgr::GetInstance();
    std::vector<std::string> words;
    std::string server_list =  (*cfg)["chatservers"]["Name"];
    std::stringstream ss(server_list);
    std::string word;
    while (std::getline(ss, word, ',')) {
        words.push_back(word);
    }
    for (auto& word : words) {
        if ((*cfg)[word]["Name"].empty()) {
            continue;
        }
        ChatServer cserver;
        cserver._name = (*cfg)[word]["Name"];
        cserver._host = (*cfg)[word]["Host"];
        cserver._port = (*cfg)[word]["Port"];
        _servers[cserver._name] = cserver;
    }
}

Status StatusServiceImpl::GetChatServer(ServerContext* context,
     const GetChatServerReq* request, GetChatServerRsp* response) {
    ChatServer server = getChatServer();

    response->set_host(server._host);
    response->set_port(server._port);
    response->set_error(ERROR_CODES::SUCCESS);
    response->set_token(generate_unique_string());
    insertToken(request->uid(), response->token());
    return Status::OK;
}

void StatusServiceImpl::insertToken(int uid, std::string token) {
    std::string uid_str = std::to_string(uid);
    std::string token_key = USER_TOKEN_PREFIX + uid_str;
    RedisMgr::GetInstance()->Set(token_key, token);
}

ChatServer StatusServiceImpl::getChatServer() {
    std::lock_guard<std::mutex> lock(_mutex);
    ChatServer min_server = _servers.begin()->second;
    std::string count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, min_server._name);
    if (count_str.empty()) {
        min_server._conn_count = INT_MAX;
    } else {
        min_server._conn_count = std::stoi(count_str);
    }
    for (auto& server : _servers) {
        if (server.second._name == min_server._name) {
            continue;
        }
        std::string cnt_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.second._name);
        if (cnt_str.empty()) {
            server.second._conn_count = INT_MAX;
        } else {
            server.second._conn_count = std::stoi(cnt_str);
        }
        if (server.second._conn_count < min_server._conn_count) {
            min_server = server.second;
        }
    }
    return min_server;
}