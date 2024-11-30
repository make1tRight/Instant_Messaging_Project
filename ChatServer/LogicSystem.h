#pragma once
#include "const.h"
#include "CSession.h"
#include "data.h"

typedef std::function<void(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data)> FunCallback;

class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    void PostMsgToQue(std::shared_ptr<LogicNode> msg);
private:
    LogicSystem();
    void DealMsg();
    void RegisterCallBacks();
    void LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
    std::thread m_worker_thread;
    std::mutex m_mutex;
    std::condition_variable m_consume;
    std::queue<std::shared_ptr<LogicNode>> m_msg_que;
    bool _b_stop;
    std::map<short, FunCallback> m_funCallbacks;
};