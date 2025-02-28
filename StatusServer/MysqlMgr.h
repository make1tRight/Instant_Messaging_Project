#pragma once
#include "const.h"
#include "MysqlDao.h"


class MysqlMgr : public Singleton<MysqlMgr> //数据库管理者, 用于对接逻辑层的调用
{
    friend class Singleton<MysqlMgr>;
public:
    ~MysqlMgr();
    int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
    bool CheckEmail(const std::string& name, const std::string& email);
    bool UpdatePwd(const std::string& name, const std::string& pwd);
    bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userinfo);
private:
    MysqlMgr();
    MysqlDao _dao;
};