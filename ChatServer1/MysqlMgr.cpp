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

bool MysqlMgr::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userinfo) {
    return _dao.CheckPwd(email, pwd, userinfo);
}

std::shared_ptr<UserInfo> MysqlMgr::GetUser(int uid) {
    return _dao.GetUser(uid);
}

std::shared_ptr<UserInfo> MysqlMgr::GetUser(std::string name) {
    return _dao.GetUser(name);
}

bool MysqlMgr::AddFriendApply(const int& from, const int& to) {
    return _dao.AddFriendApply(from, to);
}

bool MysqlMgr::AddFriend(const int& from, const int& to, std::string backname) {
    return _dao.AddFriend(from, to, backname);
}

bool MysqlMgr::GetApplyList(int touid, 
    std::vector<std::shared_ptr<ApplyInfo>>& apply_list, int begin, int limit) {
    return _dao.GetApplyList(touid, apply_list, begin, limit);
}

bool MysqlMgr::GetFriendList(int self_id,
    std::vector<std::shared_ptr<UserInfo>>& user_list) {
    return _dao.GetFriendList(self_id, user_list);
}