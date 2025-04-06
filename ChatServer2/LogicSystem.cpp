#include "LogicSystem.h"
#include "const.h"
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "data.h"
#include "ConfigMgr.h"
#include "UserMgr.h"
#include "ChatGrpcClient.h"
#include "CSession.h"

using message::AddFriendReq;
using message::TextChatData;

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
    _funcCallbacks[ID_SEARCH_USER_REQ] = std::bind(&LogicSystem::SearchInfo, this, 
         std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _funcCallbacks[ID_ADD_FRIEND_REQ] = std::bind(&LogicSystem::AddFriendApply, this,
         std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _funcCallbacks[ID_AUTH_FRIEND_REQ] = std::bind(&LogicSystem::AuthFriendApply, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _funcCallbacks[ID_TEXT_CHAT_MSG_REQ] = std::bind(&LogicSystem::DealChatTextMsg, this,
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

void LogicSystem::SearchInfo(std::shared_ptr<CSession> session,
     const short& msg_id, const std::string& msg_data) {
    Json::Value root;
    Json::Reader reader;
    reader.parse(msg_data, root);
    std::string uid_str = root["uid"].asString();
    std::cout << "SearchInfo called by uid: " << uid_str << std::endl;
    
    Json::Value rtvalue;
    // 这里按值捕获session可以增加shared_ptr的引用计数
    // 避免session提前释放
    Defer defer([session, &rtvalue, &msg_id]() {
        std::string jsonstr = rtvalue.toStyledString();
        session->Send(jsonstr, ID_SEARCH_USER_RSP);
    });
    
    bool is_digit = isPureDigit(uid_str);
    if (is_digit) {
        GetUserByUid(uid_str, rtvalue);
    } else {
        GetUserByName(uid_str, rtvalue);
    }
}

void LogicSystem::AddFriendApply(std::shared_ptr<CSession> session,
     const short& msg_id, const std::string& msg_data) {
    // 好友申请持久化到mysql中
    Json::Value root;
    Json::Reader reader;
    reader.parse(msg_data, root);
    int fromuid = root["uid"].asInt();
    std::string applyname = root["applyname"].asString();
    std::string backname = root["backname"].asString();
    int touid = root["touid"].asInt();
    std::cout << "user login uid: " << fromuid
        << " applyname: " << applyname
        << " backname: " << backname
        << " touid: " << touid << std::endl;
    Json::Value rtvalue;
    rtvalue["error"] = ERROR_CODES::SUCCESS;
    Defer defer([this, session, &rtvalue]() {
        std::string jsonstr = rtvalue.toStyledString();
        session->Send(jsonstr, ID_ADD_FRIEND_RSP);
    });
    MysqlMgr::GetInstance()->AddFriendApply(fromuid, touid);
    
    std::string to_ip_key = USER_IP_PREFIX + std::to_string(touid);
    std::string to_ip_value = "";
    bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
    if (!b_ip) {
        rtvalue["error"] = ERROR_CODES::UID_INVALID;
        return;
    }
    // 如果在同一服务上, 直接发送tcp请求给接收方
    std::shared_ptr<ConfigMgr> cfg = ConfigMgr::GetInstance();
    std::string self_server_name = (*cfg)["SelfServer"]["Name"];

    std::string base_key = USER_BASE_INFO + std::to_string(fromuid);
    auto apply_info = std::make_shared<UserInfo>();
    auto b_info = GetBaseInfo(base_key, fromuid, apply_info);
    if (self_server_name == to_ip_value) {
        auto to_session = UserMgr::GetInstance()->GetSession(touid);
        // 如果用户在线
        if (to_session) {
            Json::Value notify;
            notify["error"] = ERROR_CODES::SUCCESS;
            notify["applyuid"] = fromuid;
            notify["applyname"] = applyname;
            if (b_info) {
                notify["email"] = apply_info->_email;
                notify["nick"] = apply_info->_nick;
                notify["desc"] = apply_info->_desc;
                notify["sex"] = apply_info->_sex;
                notify["icon"] = apply_info->_icon;
            }
            std::string return_str = notify.toStyledString();
            to_session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
        }
        return;
    }
    // 如果不在同一服务, 通过grpc发送给对应服务, 由对应服务发送tcp请求给接收方
    AddFriendReq add_req;
    add_req.set_applyuid(fromuid);
    add_req.set_name(applyname);
    add_req.set_touid(touid);
    if (b_info) {
        add_req.set_desc(apply_info->_desc);
        add_req.set_icon(apply_info->_icon);
        add_req.set_nick(apply_info->_nick);
        add_req.set_sex(apply_info->_sex);
    }
    ChatGrpcClient::GetInstance()->NotifyAddFriend(to_ip_value, add_req);
}

void LogicSystem::AuthFriendApply(std::shared_ptr<CSession> session,
    const short& msg_id, const std::string& msg_data) {
    // 打印客户端发送过来的数据
    Json::Value root;
    Json::Reader reader;
    reader.parse(msg_data, root);
    int fromuid = root["fromuid"].asInt();
    int touid = root["touid"].asInt();
    std::string backname = root["back"].asString();
    std::cout << "user login uid: " << fromuid
        << " backname: " << backname
        << " touid: " << touid << std::endl;
    Json::Value rtvalue;
    rtvalue["error"] = ERROR_CODES::SUCCESS;
    Defer defer([&rtvalue, this, session]() {
        std::string jsonstr = rtvalue.toStyledString();
        session->Send(jsonstr, ID_AUTH_FRIEND_RSP);
    });
    // 返回发出申请方的数据给认证方用户
    std::string base_key = USER_BASE_INFO + std::to_string(touid);
    std::shared_ptr<UserInfo> applyinfo = std::make_shared<UserInfo>();
    bool b_info = GetBaseInfo(base_key, touid, applyinfo);
    if (b_info) {
        rtvalue["email"] = applyinfo->_name;
        rtvalue["nick"] = applyinfo->_nick;
        rtvalue["desc"] = applyinfo->_desc;
        rtvalue["sex"] = applyinfo->_sex;
        rtvalue["icon"] = applyinfo->_icon;
    } else {
        rtvalue["error"] = ERROR_CODES::UID_INVALID;
    }
    // 将好友通过以后持久化状态位
    MysqlMgr::GetInstance()->AddFriendApply(fromuid, touid);
    // 增加好友关系
    MysqlMgr::GetInstance()->AddFriend(fromuid, touid, backname);
    // 如果在同一服务且在线则通过tcp发送给对方
    
    std::string to_ip_key = USER_IP_PREFIX + std::to_string(touid);
    std::string to_ip_value = "";
    bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
    if (!b_ip) {
        rtvalue["error"] = ERROR_CODES::UID_INVALID;
        return;
    }
    std::shared_ptr<ConfigMgr> cfg = ConfigMgr::GetInstance();
    std::string self_server_name = (*cfg)["SelfServer"]["Name"];
    if (self_server_name == to_ip_value) {
        std::shared_ptr<CSession> to_session =
             UserMgr::GetInstance()->GetSession(touid);
        if (to_session) {
            // 这里是给对方的认证通过数据
            Json::Value notify;
            notify["error"] = ERROR_CODES::SUCCESS;
            notify["fromuid"] = fromuid;
            notify["touid"] = touid;
            std::string base_key = USER_BASE_INFO + std::to_string(fromuid);
            std::shared_ptr<UserInfo> userinfo = std::make_shared<UserInfo>();
            bool b_base = GetBaseInfo(base_key, fromuid, userinfo);
            if (b_info) {
                notify["email"] = userinfo->_email;
                notify["nick"] = userinfo->_nick;
                notify["desc"] = userinfo->_desc;
                notify["sex"] = userinfo->_sex;
                notify["icon"] = userinfo->_icon;
            } else {
                notify["error"] = ERROR_CODES::UID_INVALID;
            }
            std::string return_str = notify.toStyledString();
            to_session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
        }
        return;
    }
    // 如果不在同一服务则通过grpc发送给对应服务
    AuthFriendReq auth_req;
    auth_req.set_fromuid(fromuid);
    auth_req.set_touid(touid);
    ChatGrpcClient::GetInstance()->NotifyAuthFriend(to_ip_value, auth_req);
}

void LogicSystem::DealChatTextMsg(std::shared_ptr<CSession> session,
    const short& msg_id, const std::string& msg_data) {
    Json::Value root;
    Json::Reader reader;
    reader.parse(msg_data, root);
    int fromuid = root["fromuid"].asInt();
    int touid = root["touid"].asInt();
    const Json::Value arrays = root["text_array"];

    Json::Value rtvalue;
    rtvalue["error"] = ERROR_CODES::SUCCESS;
    rtvalue["fromuid"] = fromuid;
    rtvalue["touid"] = touid;
    rtvalue["text_array"] = arrays;
    // 这里是发给自己的
    Defer defer([&rtvalue, session, this]() {
        std::string jsonstr = rtvalue.toStyledString();
        session->Send(jsonstr, ID_TEXT_CHAT_MSG_RSP);
    });

    // 获取己方所在服务
    std::shared_ptr<ConfigMgr> cfg = ConfigMgr::GetInstance();
    std::string self_server_name = (*cfg)["SelfServer"]["Name"];
    // 获取对方所在服务
    std::string to_ip_key = USER_BASE_INFO + std::to_string(touid);
    std::string to_ip_value = "";
    bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
    if (!b_ip) {
        rtvalue["error"] = ERROR_CODES::UID_INVALID;
        return;
    }
    if (self_server_name == to_ip_value) {
        std::shared_ptr<CSession> to_session = 
            UserMgr::GetInstance()->GetSession(touid);
        // 双方在同一服务上, 且对方在线则直接发送
        if (to_session) {
            std::string jsonstr = rtvalue.toStyledString();
            to_session->Send(jsonstr, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
        }
        return;
    }

    TextChatMsgReq text_msg_req;
    text_msg_req.set_fromuid(fromuid);
    text_msg_req.set_touid(touid);
    for (const auto& txt_obj : arrays) {
        std::string msgid = txt_obj["msgid"].asString();
        std::string msgcontent = txt_obj["msgcontent"].asString();
        std::cout << "msgid: " << msgid << std::endl;
        std::cout << "msgcontent: " << msgcontent << std::endl;
        TextChatData* txt_msg = text_msg_req.add_testmsgs();
        txt_msg->set_msgid(msgid);
        txt_msg->set_msgcontent(msgcontent);
    }
    ChatGrpcClient::GetInstance()->NotifyTextChatMsg(to_ip_value, text_msg_req, rtvalue);
}

bool LogicSystem::GetBaseInfo(std::string base_key, int uid,
     std::shared_ptr<UserInfo>& userinfo) {
    // 先查redis
    std::string info_str = "";
    bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
    if (b_base) {
        Json::Value root;
        Json::Reader reader;
        reader.parse(info_str, root);
        userinfo->_uid = root["uid"].asInt();
        userinfo->_name = root["name"].asString();
        userinfo->_passwd = root["passwd"].asString();
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
        redis_root["passwd"] = userinfo->_passwd;
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

void LogicSystem::GetUserByUid(std::string uid_str, Json::Value& rtvalue) {
    rtvalue["error"] = ERROR_CODES::SUCCESS;
    std::string base_key = USER_BASE_INFO + uid_str;
    std::string info_str = "";
    bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
    if (b_base) {
        Json::Value root;
        Json::Reader reader;
        reader.parse(info_str, root);
        auto uid = root["uid"].asInt();
        auto name = root["name"].asString();
        auto passwd = root["passwd"].asString();
        auto email = root["email"].asString();
        auto nick = root["nick"].asString();
        auto desc = root["desc"].asString();
        auto sex = root["sex"].asInt();
        auto icon = root["icon"].asString();
        std::cout << "user login uid: " << uid 
            << " name: " << name 
            << " password: " << passwd
            << " email: " << email << std::endl;
        
        rtvalue["uid"] = uid;
        rtvalue["name"] = name;
        rtvalue["passwd"] = passwd;
        rtvalue["email"] = email;
        rtvalue["nick"] = nick;
        rtvalue["desc"] = desc;
        rtvalue["sex"] = sex;
        rtvalue["icon"] = icon;
        return;
    }
    std::shared_ptr<UserInfo> userinfo = nullptr;
    userinfo = MysqlMgr::GetInstance()->GetUser(std::stoi(uid_str));
    if (userinfo == nullptr) {
        rtvalue["error"] = ERROR_CODES::UID_INVALID;
        return;
    }
    //将数据库内容写入redis缓存
    Json::Value redis_root;
    redis_root["uid"] = userinfo->_uid;
    redis_root["passwd"] = userinfo->_passwd;
    redis_root["name"] = userinfo->_name;
    redis_root["email"] = userinfo->_email;
    redis_root["nick"] = userinfo->_nick;
    redis_root["desc"] = userinfo->_desc;
    redis_root["sex"] = userinfo->_sex;
    redis_root["icon"] = userinfo->_icon;
    std::string jsonstr = redis_root.toStyledString();
    RedisMgr::GetInstance()->Set(base_key, jsonstr);
    // 返回数据
    rtvalue["uid"] = userinfo->_uid;
    rtvalue["passwd"] = userinfo->_passwd;
    rtvalue["name"] = userinfo->_name;
    rtvalue["email"] = userinfo->_email;
    rtvalue["nick"] = userinfo->_nick;
    rtvalue["desc"] = userinfo->_desc;
    rtvalue["sex"] = userinfo->_sex;
    rtvalue["icon"] = userinfo->_icon;
}

void LogicSystem::GetUserByName(std::string name, Json::Value& rtvalue) {
    rtvalue["error"] = ERROR_CODES::SUCCESS;
    std::string base_key = NAME_INFO + name;
    std::string info_str = "";
    bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
    if (b_base) {
        Json::Value root;
        Json::Reader reader;
        reader.parse(info_str, root);
        auto uid = root["uid"].asInt();
        auto name = root["name"].asString();
        auto passwd = root["passwd"].asString();
        auto email = root["email"].asString();
        auto nick = root["nick"].asString();
        auto desc = root["desc"].asString();
        auto sex = root["sex"].asInt();
        auto icon = root["icon"].asString();
        std::cout << "user login uid: " << uid 
            << " name: " << name 
            << " password: " << passwd
            << " email: " << email << std::endl;
        
        rtvalue["uid"] = uid;
        rtvalue["name"] = name;
        rtvalue["passwd"] = passwd;
        rtvalue["email"] = email;
        rtvalue["nick"] = nick;
        rtvalue["desc"] = desc;
        rtvalue["sex"] = sex;
        rtvalue["icon"] = icon;
        return;
    }
    std::shared_ptr<UserInfo> userinfo = nullptr;
    userinfo = MysqlMgr::GetInstance()->GetUser(base_key);
    if (userinfo == nullptr) {
        rtvalue["error"] = ERROR_CODES::UID_INVALID;
        return;
    }
    //将数据库内容写入redis缓存
    Json::Value redis_root;
    redis_root["uid"] = userinfo->_uid;
    redis_root["passwd"] = userinfo->_passwd;
    redis_root["name"] = userinfo->_name;
    redis_root["email"] = userinfo->_email;
    redis_root["nick"] = userinfo->_nick;
    redis_root["desc"] = userinfo->_desc;
    redis_root["sex"] = userinfo->_sex;
    redis_root["icon"] = userinfo->_icon;
    std::string jsonstr = redis_root.toStyledString();
    RedisMgr::GetInstance()->Set(base_key, jsonstr);
    // 返回数据
    rtvalue["uid"] = userinfo->_uid;
    rtvalue["passwd"] = userinfo->_passwd;
    rtvalue["name"] = userinfo->_name;
    rtvalue["email"] = userinfo->_email;
    rtvalue["nick"] = userinfo->_nick;
    rtvalue["desc"] = userinfo->_desc;
    rtvalue["sex"] = userinfo->_sex;
    rtvalue["icon"] = userinfo->_icon;
}

bool LogicSystem::isPureDigit(std::string str) {
    for (const char& c : str) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}
