#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include "Singleton.h"
#include <functional>	//回调
#include <map>
#include <unordered_map>
#include <json/json.h>		//json格式
#include <json/value.h>		//json值
#include <json/reader.h>	//解析
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp> //read_ini
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "hiredis.h"
#include <cassert>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

enum ErrorCodes {
    SUCCESS = 0,
    ERROR_JSON = 1001,              //JSON解析错误
    RPC_FAILED = 1002,              //RPC请求错误
    VARIFY_EXPIRED = 1003,          //验证码过期
    VARIFY_CODE_ERR = 1004,         //验证码错误
    USER_EXIST = 1005,              //用户已存在
    PASSWD_ERR = 1006,              //密码错误
    EMAIL_NOT_MATCH = 1007,         //邮箱不匹配
    PASSWD_UPDATE_FAILED = 1008,    //更新密码失败
    PASSWD_INVALID = 1009,          //密码无效
    //RPC_GET_FAILED = 1010,          //获取rpc请求失败
    TOKEN_INVALID = 1010,           //TOKEN无效
    UID_INVALID = 1011,             //uid无效
};

class Defer {
public:
    Defer(std::function<void()> func): func_(func) {}
    ~Defer() {  //在析构的时候执行目标函数, 类似RAII, 就是在离开作用域之前做一个固定动作
        func_();
    }
private:
    std::function<void()> func_;
};

#define MAX_LENGTH 1024*2
#define HEAD_TOTAL_LEN 4        //头部总长度
#define HEAD_ID_LEN 2           //头部id长度
#define HEAD_DATA_LEN 2         //头部数据长度
#define MAX_RECVQUE 10000       //
#define MAX_SENDQUE 1000        //

enum MSG_IDS {
    MSG_CHAT_LOGIN = 1005, //用户登陆
    MSG_CHAT_LOGIN_RSP = 1006, //用户登陆回包
    //ID_SEARCH_USER_REQ = 1007, //用户搜索请求
    //ID_SEARCH_USER_RSP = 1008, //搜索用户回包
    //ID_ADD_FRIEND_REQ = 1009, //申请添加好友请求
    //ID_ADD_FRIEND_RSP = 1010, //申请添加好友回复
    //ID_NOTIFY_ADD_FRIEND_REQ = 1011,  //通知用户添加好友申请
    //ID_AUTH_FRIEND_REQ = 1013,  //认证好友请求
    //ID_AUTH_FRIEND_RSP = 1014,  //认证好友回复
    //ID_NOTIFY_AUTH_FRIEND_REQ = 1015, //通知用户认证好友申请
    //ID_TEXT_CHAT_MSG_REQ = 1017, //文本聊天信息请求
    //ID_TEXT_CHAT_MSG_RSP = 1018, //文本聊天信息回复
    //ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //通知用户文本聊天信息
};

//class ConfigMgr;
//extern ConfigMgr gCfgMgr;
#define CODEPREFIX "code_"
#define USERTOKENPREFIX  "utoken_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT  "logincount"
#define NAME_INFO  "nameinfo_"
#define USERIPPREFIX  "uip_"
