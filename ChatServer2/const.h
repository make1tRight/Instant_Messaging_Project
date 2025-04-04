#ifndef CONST_H
#define CONST_H
#include <functional>
#include "message.pb.h"
#include "message.grpc.pb.h"
#define CODEPREFIX "code_"
#define USER_TOKEN_PREFIX "utoken_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT "logincount"
#define USER_IP_PREFIX  "uip_"


#define HEAD_TOTAL_LEN 4    //头部总长度
#define MAX_LENGTH 1024*2   //报文总长度
#define HEAD_ID_LENGTH 2    //消息id长度
#define HEAD_DATA_LENGTH 2  //数据长度说明
#define MAX_RECVNODE 10000  
#define MAX_SENDNODE 10000  


enum ERROR_CODES {
    SUCCESS = 0,                    
    JSON_ERROR = 1001,              //JSON解析错误
    RPC_FAILED = 1002,              //RPC请求错误
    VARIFY_EXPIRED = 1003,          //验证码过期
    VARIFY_CODE_ERROR = 1004,       //验证码错误
    USER_EXIST = 1005,              //用户已存在
    PASSWD_ERR = 1006,              //密码错误
    EMAIL_NOT_MATCH = 1007,         //邮箱不匹配
    PWD_UPDATE_FAILED = 1008,       //更新密码失败
    PASSWD_INVALID = 1009,          //密码无效
    TOKEN_INVALID = 1010,           //token无效
    UID_INVALID = 1011,             //uid无效
};

enum MSG_IDS {
    MSG_CHAT_LOGIN_REQ = 1005,          //用户登录请求
    MSG_CHAT_LOGIN_RSP = 1006,          //用户登录回报
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