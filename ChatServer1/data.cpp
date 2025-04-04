#include "data.h"

UserInfo::UserInfo()
     : _uid(0), _name(""), _passwd(""), _email(""), 
     _nick(""), _desc(""), _sex(0), _icon(""), _back("") {}

ApplyInfo::ApplyInfo(int uid, std::string name, std::string desc,
     std::string icon, std::string nick, int sex, int status)
    : _uid(uid), _name(name), _desc(desc),
    _icon(icon), _nick(nick), _sex(sex), _status(status) {
}