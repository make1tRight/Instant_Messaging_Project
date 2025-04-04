#include "LogicSystem.h"
#include "const.h"
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
// #include "StatusGrpcClient.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "data.h"
#include "ConfigMgr.h"
#include "UserMgr.h"

LogicSystem::LogicSystem() : _b_stop(false) {
    RegisterCallBacks();
    _worker = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem() {
    _b_stop = true;
    _cond.notify_one();
    _worker.join();
}

void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg) {
    std::unique_lock<std::mutex> lock(_mutex);
    _msg_que.push(msg);
    if (_msg_que.size() == 1) {
        lock.unlock();
        _cond.notify_one();
    }
}

void LogicSystem::RegisterCallBacks() {
    _funcCallbacks[MSG_CHAT_LOGIN_REQ] = std::bind(&LogicSystem::LoginHandler, this,
         std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::DealMsg() {
    for (;;) {
        std::unique_lock<std::mutex> unique_lk(_mutex);
        while (_msg_que.empty() && !_b_stop) {
            _cond.wait(unique_lk);
        }
        if (_b_stop) {
            while (!_msg_que.empty()) {
                std::shared_ptr<LogicNode> msgnode = _msg_que.front();
                std::cout << "Receive message: " << msgnode->_recvnode->_msg_id << std::endl;
                auto callback = _funcCallbacks.find(msgnode->_recvnode->_msg_id);
                if (callback == _funcCallbacks.end()) {
                    std::cout << "msg id[" << msgnode->_recvnode->_msg_id 
                        << "] handler not found." << std::endl;
                    _msg_que.pop();
                    continue;
                }
                callback->second(msgnode->_session, msgnode->_recvnode->_msg_id,
                     std::string(msgnode->_recvnode->_data, msgnode->_recvnode->_cur_len));
                _msg_que.pop();
            }
            break;
        }
        std::shared_ptr<LogicNode> msgnode = _msg_que.front();
        std::cout << "Receive message: " << msgnode->_recvnode->_msg_id << std::endl;
        auto callback = _funcCallbacks.find(msgnode->_recvnode->_msg_id);
        if (callback == _funcCallbacks.end()) {
            std::cout << "msg id[" << msgnode->_recvnode->_msg_id 
                << "] handler not found." << std::endl;
            _msg_que.pop();
            continue;
        }
        callback->second(msgnode->_session, msgnode->_recvnode->_msg_id,
            std::string(msgnode->_recvnode->_data, msgnode->_recvnode->_cur_len));
        _msg_que.pop();
    }
}

void LogicSystem::LoginHandler(std::shared_ptr<CSession> session,
     const short& msg_id, const std::string& msg_data) {
    Json::Reader reader;
    Json::Value root;
    Json::Value rtvalue;
    reader.parse(msg_data, root);
    int uid = root["uid"].asInt();
    std::string token = root["token"].asString();
    std::cout << "User logining, uid: " << uid
        << ", token: " << token << std::endl;
    Defer defer([session, &rtvalue]() {
        std::string jsonstr = rtvalue.toStyledString();
        session->Send(jsonstr, MSG_CHAT_LOGIN_RSP);
    });
    // 先查redis
    std::string uid_str = std::to_string(uid);
    std::string token_key = USER_TOKEN_PREFIX + uid_str;
    std::string token_value;
    bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
    if (!success) {
        rtvalue["error"] = ERROR_CODES::UID_INVALID;
        return;
    }
    if (token_value != token) {
        rtvalue["error"] = ERROR_CODES::TOKEN_INVALID;
        return;
    }
    rtvalue["error"] = ERROR_CODES::SUCCESS;
    std::shared_ptr<UserInfo> userinfo = std::make_shared<UserInfo>();
    std::string base_key = USER_BASE_INFO + uid_str;
    bool b_base = GetBaseInfo(base_key, uid, userinfo);
    if (!b_base) {
        rtvalue["error"] = ERROR_CODES::UID_INVALID;
        return;
    }
    rtvalue["uid"] = uid;
    rtvalue["name"] = userinfo->_name;
    rtvalue["passwd"] = userinfo->_passwd;
    rtvalue["email"] = userinfo->_email;
    rtvalue["nick"] = userinfo->_nick;
    rtvalue["desc"] = userinfo->_desc;
    rtvalue["sex"] = userinfo->_sex;
    rtvalue["icon"] = userinfo->_icon;
    rtvalue["back"] = userinfo->_back;
    // 从数据库中获取申请列表
    std::vector<std::shared_ptr<ApplyInfo>> apply_list;
    auto b_apply = GetFriendApplyInfo(uid, apply_list);
    if (b_apply) {
        for (auto& apply : apply_list) {
            Json::Value obj;
            obj["name"] = apply->_name;
            obj["uid"] = apply->_uid;
            obj["icon"] = apply->_icon;
            obj["nick"] = apply->_nick;
            obj["sex"] = apply->_sex;
            obj["desc"] = apply->_desc;
            obj["status"] = apply->_status;
            rtvalue["apply_list"].append(obj);
        }
    }
    // 从数据库中获取好友列表
    std::vector<std::shared_ptr<UserInfo>> friend_list;
    auto b_friend_list = GetFriendList(uid, friend_list);
    if (b_friend_list) {
        for (auto& friend_ele : friend_list) {
            Json::Value obj;
            obj["name"] = friend_ele->_name;
            obj["uid"] = friend_ele->_uid;
            obj["icon"] = friend_ele->_icon;
            obj["nick"] = friend_ele->_nick;
            obj["sex"] = friend_ele->_sex;
            obj["desc"] = friend_ele->_desc;
            rtvalue["friend_list"].append(obj);
        }
    }
    // 将用户登录数量加入到redis缓存, 便于状态服务进行负载均衡决策
    auto server_name = ConfigMgr::GetInstance()->GetValue("SelfServer", "Name");
    auto rd_res = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server_name);
    int count = 0;
    if (!rd_res.empty()) {
        count = std::stoi(rd_res);
    }
    ++count;
    std::string count_str = std::to_string(count);
    RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, count_str);

    session->SetUserId(uid);
    std::string ipkey = USER_IP_PREFIX + uid_str;
    RedisMgr::GetInstance()->Set(ipkey, server_name);
    UserMgr::GetInstance()->SetSession(uid, session);
}

bool LogicSystem::GetBaseInfo(std::string base_key, int uid,
     std::shared_ptr<UserInfo>& userinfo) {
    // 先查redis
    std::string info_str;
    bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
    if (b_base) {
        Json::Value root;
        Json::Reader reader;
        reader.parse(info_str, root);
        userinfo->_uid = root["uid"].asInt();
        userinfo->_name = root["name"].asString();
        userinfo->_passwd = root["pwd"].asString();
        userinfo->_email = root["email"].asString();
        userinfo->_nick = root["nick"].asString();
        userinfo->_desc = root["desc"].asString();
        userinfo->_sex = root["sex"].asInt();
        userinfo->_icon = root["icon"].asString();
        std::cout << "User login, uid: " << userinfo->_uid 
                << " name: " << userinfo->_name 
                << " password: " << userinfo->_passwd 
                << " email: " << userinfo->_email 
                << std::endl;
    } else {
        std::shared_ptr<UserInfo> info = std::make_shared<UserInfo>();
        info =  MysqlMgr::GetInstance()->GetUser(uid);
        if (info == nullptr) {
            return false;
        }
        userinfo = info;
        // mysql查询出的数据缓存到redis
        Json::Value redis_root;
        redis_root["uid"] = uid;
        redis_root["pwd"] = userinfo->_passwd;
        redis_root["name"] = userinfo->_name;
        redis_root["email"] = userinfo->_email;
        redis_root["nick"] = userinfo->_nick;
        redis_root["desc"] = userinfo->_desc;
        redis_root["sex"] = userinfo->_sex;
        redis_root["icon"] = userinfo->_icon;
        std::string jsonstr = redis_root.toStyledString();
        RedisMgr::GetInstance()->Set(base_key, jsonstr);
    }
    return true;
}

bool LogicSystem::GetFriendApplyInfo(int to_uid,
     std::vector<std::shared_ptr<ApplyInfo>>& apply_list) {
    return MysqlMgr::GetInstance()->GetApplyList(to_uid, apply_list, 0, 10);
}
bool LogicSystem::GetFriendList(int self_id,
     std::vector<std::shared_ptr<UserInfo>>& friend_list) {
    return MysqlMgr::GetInstance()->GetFriendList(self_id, friend_list);
}
