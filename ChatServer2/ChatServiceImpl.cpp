#include "ChatServiceImpl.h"
#include "data.h"
#include "UserMgr.h"
#include <jsoncpp/json/json.h>
#include "CSession.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"


ChatServiceImpl::ChatServiceImpl() {

}

Status ChatServiceImpl::NotifyAddFriend(ServerContext* context,
     const AddFriendReq* request, AddFriendRsp* response) {
    int uid = request->applyuid();
    std::shared_ptr<CSession> session = UserMgr::GetInstance()->GetSession(uid);
    Defer defer([response, request]() {
        response->set_error(ERROR_CODES::SUCCESS);
        response->set_applyuid(request->applyuid());
        response->set_touid(request->touid());
    });

    if (session == nullptr) {
        response->set_error(ERROR_CODES::RPC_FAILED);
        return Status::OK;
    }
    Json::Value rtvalue;
    rtvalue["error"] = ERROR_CODES::SUCCESS;
    rtvalue["applyuid"] = request->applyuid();
    rtvalue["name"] = request->name();
    rtvalue["desc"] = request->desc();
    rtvalue["icon"] = request->icon();
    rtvalue["nick"] = request->nick();
    rtvalue["sex"] = request->sex();
    std::string jsonstr = rtvalue.toStyledString();
    session->Send(jsonstr, ID_NOTIFY_ADD_FRIEND_REQ);
    return Status::OK;
}

Status ChatServiceImpl::NotifyAuthFriend(ServerContext* context,
     const AuthFriendReq* request, AuthFriendRsp* response) {
    int fromuid = request->fromuid();
    int touid = request->touid();
    std::shared_ptr<CSession> session =
         UserMgr::GetInstance()->GetSession(touid);
    response->set_error(ERROR_CODES::SUCCESS);
    Defer defer([&response, &request]() {
        response->set_fromuid(request->fromuid());
        response->set_touid(request->touid());
    });
    if (session == nullptr) {
        response->set_error(ERROR_CODES::RPC_FAILED);
        return Status::OK;
    }
    // 在内存中则直接发送通知给对方
    Json::Value rtvalue;
    rtvalue["error"] = ERROR_CODES::SUCCESS;
    rtvalue["fromuid"] = fromuid;
    rtvalue["touid"] = touid;
    std::string base_key = USER_BASE_INFO + std::to_string(fromuid);
    std::shared_ptr<UserInfo> userinfo = std::shared_ptr<UserInfo>();
    bool b_base = GetBaseInfo(base_key, fromuid, userinfo);
    if (b_base) {
        rtvalue["name"] = userinfo->_name;
        rtvalue["email"] = userinfo->_email;
        rtvalue["nick"] = userinfo->_nick;
        rtvalue["desc"] = userinfo->_desc;
        rtvalue["sex"] = userinfo->_sex;
        rtvalue["icon"] = userinfo->_icon;
    } else {
        rtvalue["error"] = ERROR_CODES::UID_INVALID;
    }

    std::string jsonstr = rtvalue.toStyledString();
    session->Send(jsonstr, ID_NOTIFY_AUTH_FRIEND_REQ);
    return Status::OK;
}

Status ChatServiceImpl::NotifyTextChatMsg(ServerContext* context,
     const TextChatMsgReq* request, TextChatMsgRsp* response) {
    int touid = request->touid();
    int fromuid = request->fromuid();
    std::shared_ptr<CSession> session = 
         UserMgr::GetInstance()->GetSession(touid);
    response->set_error(ERROR_CODES::SUCCESS);
    if (session == nullptr) {
        response->set_error(ERROR_CODES::RPC_FAILED);
        return Status::OK;
    }

    Json::Value rtvalue;
    rtvalue["error"] = ERROR_CODES::SUCCESS;
    rtvalue["fromuid"] = fromuid;
    rtvalue["touid"] = touid;
    // 将聊天数据组织为数组
    Json::Value text_array;
    for (const auto& txt_msg : request->testmsgs()) {
        Json::Value elem;
        elem["msgid"] = txt_msg.msgid();
        elem["msgcontent"] = txt_msg.msgcontent();
        text_array.append(elem);
    }
    rtvalue["text_array"] = text_array;

    std::string jsonstr = rtvalue.toStyledString();
    session->Send(jsonstr, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
    return Status::OK;
}

bool ChatServiceImpl::GetBaseInfo(std::string base_key,
     int uid, std::shared_ptr<UserInfo>& userinfo) {
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