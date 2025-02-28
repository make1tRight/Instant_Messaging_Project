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
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

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

#define LOGIN_COUNT  "logincount"
#define USERTOKENPREFIX  "utoken_"
