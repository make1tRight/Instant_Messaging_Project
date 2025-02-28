#include "LogicSystem.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "StatusGrpcClient.h"
#include "data.h"
#include "ConfigMgr.h"
#include "UserMgr.h"



LogicSystem::LogicSystem()
    : _b_stop(false) {
    RegisterCallBacks();
    m_worker_thread = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem() {
    _b_stop = true;
    m_consume.notify_one();
    m_worker_thread.join();
}

//[19-56:15]生产者消费者模型以及放太快的处理方法
void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg) {
    std::unique_lock<std::mutex> unique_lk(m_mutex);
    m_msg_que.push(msg);
    //由0变1则发送通知信号
    if (m_msg_que.size() == 1) {
        unique_lk.unlock();
        m_consume.notify_one();
    }
}

void LogicSystem::DealMsg() {
    for (;;) {
        std::unique_lock<std::mutex> unique_lk(m_mutex);
        //判断队列为空则用条件变量阻塞等待并释放锁
        while (m_msg_que.empty() && !_b_stop) {
            m_consume.wait(unique_lk);
        }
        //判断是否为关闭状态, 如果为关闭状态
        //把所有逻辑执行完后退出
        if (_b_stop) {
            while (!m_msg_que.empty()) {
                auto msg_node = m_msg_que.front();
                std::cout << "recv_msg is " << msg_node->_recvnode->_msg_id << std::endl;
                auto call_back_iter = m_funCallbacks.find(msg_node->_recvnode->_msg_id);
                if (call_back_iter == m_funCallbacks.end()) {
                    m_msg_que.pop();
                    continue;
                }
                //调用回调函数, 后面3个是传入的参数
                call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
                    std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
                m_msg_que.pop();
            }
            break;
        }
        //如果没有停服, 说明队列中有数据
        auto msg_node = m_msg_que.front();
        std::cout << "recv_msg id is " << msg_node->_recvnode->_msg_id << std::endl;
        auto call_back_iter = m_funCallbacks.find(msg_node->_recvnode->_msg_id);
        if (call_back_iter == m_funCallbacks.end()) {
            m_msg_que.pop();
            std::cout << "msg id [" << msg_node->_recvnode->_msg_id << "] handler not found" << std::endl;
            continue;
        }
        call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
            std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
        m_msg_que.pop();
    }
}

void LogicSystem::RegisterCallBacks() {
    m_funCallbacks[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto uid = root["uid"].asInt();
    auto token = root["token"].asString();
    std::cout << "user login uid is " << uid << " user token is "
        << token << std::endl;
    //从状态服务器获取token匹配是否准确
    //auto rsp = StatusGrpcClient::GetInstance()->Login(uid, root["token"].asString());
    Json::Value  rtvalue;
    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, MSG_CHAT_LOGIN_RSP);
    });
    //从redis获取用户token是否正确
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    std::string token_value = "";
    bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
    if (!success) {
        rtvalue["error"] = ErrorCodes::UID_INVALID;
        return;
    }
    if (token_value != token) {
        rtvalue["error"] = ErrorCodes::TOKEN_INVALID;
        return;
    }
    rtvalue["error"] = ErrorCodes::SUCCESS;

    std::string base_key = USER_BASE_INFO + uid_str;
    auto user_info = std::make_shared<UserInfo>();
    bool b_base = GetBaseInfo(base_key, uid, user_info);
    if (!b_base) {
        rtvalue["error"] = ErrorCodes::UID_INVALID;
        return;
    }
    rtvalue["uid"] = uid;
    rtvalue["pwd"] = user_info->pwd;
    rtvalue["name"] = user_info->name;
    //从数据库获取申请列表
    //获取好友列表
    //用户登录数量增加
    auto server_name = ConfigMgr::Inst().GetValue("SelfServer", "Name");
    auto rd_res = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server_name);
    int count = 0;
    if (!rd_res.empty()) {
        count = std::stoi(rd_res);
    }
    ++count;
    auto count_str = std::to_string(count);
    RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, count_str);//将更新后的数量写回redis
    
    //session绑定用户uid
    session->SetUserId(uid);
    //为用户设置登录ipserver的名字(有可能用户在其他server登录上去了)
    std::string ipkey = USERIPPREFIX + uid_str;
    RedisMgr::GetInstance()->Set(ipkey, server_name);
    //为后续实现踢人操作做准备, uid和session绑定管理
    UserMgr::GetInstance()->SetUserSession(uid, session);
    return;
}

bool LogicSystem::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
    std::string info_str = "";
    bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
    if (b_base) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        userinfo->uid = root["uid"].asInt();
        userinfo->name = root["name"].asString();
        userinfo->pwd = root["pwd"].asString();
        userinfo->email = root["email"].asString();
        userinfo->nick = root["nick"].asString();
        userinfo->desc = root["desc"].asString();
        userinfo->sex = root["sex"].asInt();
        userinfo->icon = root["icon"].asString();
        std::cout << "user login uid is  " << userinfo->uid << " name  is "
            << userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email << std::endl;
    }
    else {
        //redis中没有数据就查询MySQL
        std::shared_ptr<UserInfo> user_info = nullptr;
        user_info = MysqlMgr::GetInstance()->GetUser(uid);
        if (user_info == nullptr) {
            return false;
        }
        userinfo = user_info;
        //将数据库内容写入redis缓存
        Json::Value redis_root;
        redis_root["uid"] = uid;
        redis_root["pwd"] = userinfo->pwd;
        redis_root["name"] = userinfo->name;
        redis_root["email"] = userinfo->email;
        redis_root["nick"] = userinfo->nick;
        redis_root["desc"] = userinfo->desc;
        redis_root["sex"] = userinfo->sex;
        redis_root["icon"] = userinfo->icon;
        RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
    }
    return true;
}