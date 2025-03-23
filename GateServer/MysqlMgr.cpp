#include "MysqlMgr.h"

MysqlMgr::~MysqlMgr() {

}

int MysqlMgr::UserRegister(const std::string& name,
    const std::string& email, const std::string& passwd) {
    return _dao.UserRegister(name, email, passwd);
}

MysqlMgr::MysqlMgr() {

}