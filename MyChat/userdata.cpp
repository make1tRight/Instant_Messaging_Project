#include "userdata.h"


SearchInfo::SearchInfo(int uid, QString name, QString nick, QString desc, int sex)
    : _uid(uid) ,_name(name), _nick(nick),_desc(desc),_sex(sex) {

}

AddFriendApply::AddFriendApply(int from_uid, QString name, QString desc, QString icon, QString nick, int sex)
    :_from_uid(from_uid),_name(name), _desc(desc),_icon(icon),_nick(nick),_sex(sex)
{

}