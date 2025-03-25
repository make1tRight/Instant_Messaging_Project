#ifndef CONST_H
#define CONST_H
#include <functional>
#define CODEPREFIX "code_"

enum ERROR_CODES {
    SUCCESS = 0,                    
    JSON_ERROR = 1001,              //JSON解析错误
    RPC_FAILED = 1002,              //RPC请求错误
    VARIFY_EXPIRED = 1003,          //验证码过期
    VARIFY_CODE_ERROR = 1004,       //验证码错误
    USER_EXIST = 1005,              //用户已存在
    PASSWD_ERR = 1006,              //密码错误
    EMAIL_NOT_MATCH = 1007,         //邮箱不匹配
    PWD_UPDATE_FAILED = 1008        //更新密码失败
};

class Defer {
public:
    Defer(std::function<void()> func) : _func(func) {

    }
    ~Defer() {
        _func();
    }
private:
    std::function<void()> _func;
};


#endif // CONST_H