#ifndef CONST_H
#define CONST_H
#include <functional>
#include "message.pb.h"
#include "message.grpc.pb.h"
#define CODEPREFIX "code_"
#define USER_TOKEN_PREFIX "utoken_"
#define USER_BASE_INFO "ubaseinfo_"
#define NAME_INFO "nameinfo_"
#define LOGIN_COUNT "logincount"
#define USER_IP_PREFIX  "uip_"


#define HEAD_TOTAL_LEN 4    //头部总长度
#define MAX_LENGTH 1024*2   //报文总长度
#define HEAD_ID_LENGTH 2    //消息id长度
#define HEAD_DATA_LENGTH 2  //数据长度说明
#define MAX_RECVQUE 10000  
#define MAX_SENDQUE 1000


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
    MSG_CHAT_LOGIN_RSP = 1006,          //用户登录回包
    ID_SEARCH_USER_REQ = 1007,          //用户搜索请求
    ID_SEARCH_USER_RSP = 1008,          //用户搜索回包
    ID_ADD_FRIEND_REQ = 1009,           //好友申请请求
    ID_ADD_FRIEND_RSP = 1010,           //好友申请回包
    ID_NOTIFY_ADD_FRIEND_REQ = 1011,    //通知用户添加好友申请

    ID_AUTH_FRIEND_REQ = 1013,          //好友认证请求
    ID_AUTH_FRIEND_RSP = 1014,          //好友认证回包
    ID_NOTIFY_AUTH_FRIEND_REQ = 1015,   //通知用户认证好友申请
    ID_TEXT_CHAT_MSG_REQ = 1017,        //聊天文本发送请求
    ID_TEXT_CHAT_MSG_RSP = 1018,        //聊天文本发送回包
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //通知用户有聊天消息到来
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