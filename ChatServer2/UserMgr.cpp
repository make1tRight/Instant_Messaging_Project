#include "UserMgr.h"


UserMgr::~UserMgr() {
    _uid2session.clear();
}

std::shared_ptr<CSession> UserMgr::GetSession(int uid) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto iter = _uid2session.find(uid);
    if (iter == _uid2session.end()) {
        return nullptr;
    }
    return iter->second;
}

void UserMgr::SetSession(int uid, std::shared_ptr<CSession> session) {
    std::lock_guard<std::mutex> lock(_mutex);
    _uid2session[uid] = session;
}
void UserMgr::RmvSession(int uid) {
    std::lock_guard<std::mutex> lock(_mutex);
    _uid2session.erase(uid);
}

UserMgr::UserMgr() {

}