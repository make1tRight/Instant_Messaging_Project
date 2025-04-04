#ifndef USERMGR_H
#define USERMGR_H
#include "Singleton.h"
#include <unordered_map>

class CSession;
class UserMgr : public Singleton<UserMgr> {
    friend class Singleton<UserMgr>;
public:
    ~UserMgr();
    std::shared_ptr<CSession> GetSession(int uid);
    void SetSession(int uid, std::shared_ptr<CSession> session);
    void RmvSession(int uid);
private:
    UserMgr();
    std::mutex _mutex;
    std::unordered_map<int, std::shared_ptr<CSession>> _uid2session;
};
#endif // USERMGR_H