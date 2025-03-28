#ifndef MYSQLMGR_H
#define MYSQLMGR_H
#include "Singleton.h"
#include "MysqlDao.h"

class MysqlMgr : public Singleton<MysqlMgr> {
    friend class Singleton<MysqlMgr>;
public:
    ~MysqlMgr();
    int UserRegister(const std::string& name,
        const std::string& email, const std::string& passwd);
    bool CheckEmail(const std::string& name, const std::string& email);
    bool UpdatePwd(const std::string& name, const std::string& pwd);
    bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userinfo);
private:
    MysqlMgr();
    MysqlDao _dao;
};
#endif // MYSQLMGR_H