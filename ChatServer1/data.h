#ifndef DATA_H
#define DATA_H
#include <string>

struct UserInfo {
    UserInfo();
    int _uid;
    std::string _name;
    std::string _passwd;
    std::string _email;
    std::string _nick;
    std::string _desc;
    int _sex;
    std::string _icon;
    std::string _back;
};

struct ApplyInfo {
    ApplyInfo(int uid, std::string name, std::string desc,
        std::string icon, std::string nick, int sex, int status);
    int _uid;
    std::string _name;
    std::string _desc;
    std::string _icon;
    std::string _nick;
    int _sex;
    int _status;
};
#endif // DATA_H