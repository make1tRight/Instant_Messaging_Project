#include "MysqlMgr.h"

MysqlMgr::~MysqlMgr() {

}

int MysqlMgr::UserRegister(const std::string& name,
    const std::string& email, const std::string& passwd) {
    return _dao.UserRegister(name, email, passwd);
}

MysqlMgr::MysqlMgr() {

}

bool MysqlMgr::CheckEmail(const std::string& name, const std::string& email) {
    return _dao.CheckEmail(name, email);
}
bool MysqlMgr::UpdatePwd(const std::string& name, const std::string& pwd) {
    return _dao.UpdatePwd(name, pwd);
}