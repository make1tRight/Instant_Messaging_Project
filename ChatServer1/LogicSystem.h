#ifndef LOGICSYSTEM_H
#define LOGICSYSTEM_H
#include "Singleton.h"
#include "CSession.h"
#include <queue>
#include <thread>
#include <functional>
#include <unordered_map>
typedef std::function<
    void(std::shared_ptr<CSession>, const short&, const std::string&)> FuncCallback;


class UserInfo;
class ApplyInfo;
class LogicSystem : public Singleton<LogicSystem> {
    friend class Singleton<LogicSystem>; //加了_instance才有权限new
public:
    ~LogicSystem();
    void PostMsgToQue(std::shared_ptr<LogicNode> msg);
private:
    LogicSystem();
    void RegisterCallBacks();
    void DealMsg();
    void LoginHandler(std::shared_ptr<CSession>, const short&, const std::string&);
    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
    bool GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& apply_list);
    bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& friend_list);

    std::mutex _mutex;
    std::thread _worker;
    std::condition_variable _cond;
    bool _b_stop;
    std::unordered_map<short, FuncCallback> _funcCallbacks;
    std::queue<std::shared_ptr<LogicNode>> _msg_que;
};
#endif // LOGICSYSTEM_H