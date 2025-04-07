#ifndef MYSQLMGR_H
#define MYSQLMGR_H
#include "Singleton.h"
#include "MysqlDao.h"

class ApplyInfo;
class MysqlMgr : public Singleton<MysqlMgr> {
    friend class Singleton<MysqlMgr>;
public:
    ~MysqlMgr();
    int UserRegister(const std::string& name,
        const std::string& email, const std::string& passwd);
    bool CheckEmail(const std::string& name, const std::string& email);
    bool UpdatePwd(const std::string& name, const std::string& pwd);
    bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userinfo);
    std::shared_ptr<UserInfo> GetUser(int uid);
    std::shared_ptr<UserInfo> GetUser(std::string name);

    bool AddFriendApply(const int& from, const int& to);
    bool AddFriend(const int& from, const int& to, std::string backname);
    bool AuthFriendApply(const int& from, const int& to);
    bool GetApplyList(int touid, 
        std::vector<std::shared_ptr<ApplyInfo>>& apply_list, int begin, int limit = 10);
    bool GetFriendList(int self_id,
         std::vector<std::shared_ptr<UserInfo>>& user_list);
private:
    MysqlMgr();
    MysqlDao _dao;
};
#endif // MYSQLMGR_H