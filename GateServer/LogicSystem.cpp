#include "LogicSystem.h"
#include "HttpConnection.h" //[5-1:08:54]避免循环依赖, 分别在对方的cpp文件里面包含.h; 在头文件里面只做声明
#include "VarifyGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "StatusGrpcClient.h"

LogicSystem::LogicSystem() {
    RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
        beast::ostream(connection->m_response.body()) << "receive get_test req " << std::endl;
        int i = 0;
        for (auto& elem : connection->m_getParams) {
            i++;
            beast::ostream(connection->m_response.body()) << "param " << i << "key is " << elem.first;
            beast::ostream(connection->m_response.body()) << ", " << i << "value is " << elem.second << std::endl;
        }
    });
    RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str = boost::beast::buffers_to_string(connection->m_request.body().data());
        std::cout << "get_varifycode receive body is " << body_str << std::endl;
        connection->m_response.set(http::field::content_type, "text/json"); //报文格式设定位json类型
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool parse_success = reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data" << std::endl;
            root["error"] = ErrorCodes::ERROR_JSON;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->m_response.body()) << jsonstr;
            return true;
        }
        if (!src_root.isMember("email")) {
            std::cout << "Failed to parse JSON data" << std::endl;
            root["error"] = ErrorCodes::ERROR_JSON;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->m_response.body()) << jsonstr;
            return true;
        }
        auto email = src_root["email"].asString();
        //添加通过gRPC向varifyserver通讯的部分
        GetVarifyRsp rsp = VarifyGrpcClient::GetInstance()->GetVarifyCode(email);
        std::cout << "email is " << email << std::endl;
        //root["error"] = 0; //表示没有错误
        root["error"] = rsp.error();//表示没有错误
        root["email"] = src_root["email"];
        std::string jsonstr = root.toStyledString(); //转换成字符串
        beast::ostream(connection->m_response.body()) << jsonstr;
        return true;
    });
    RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str = boost::beast::buffers_to_string(connection->m_request.body().data());
        std::cout << "user_register receive body is " << body_str << std::endl;
        connection->m_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool parse_success = reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::ERROR_JSON;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->m_response.body()) << jsonstr;
            return true;
        }
        auto email = src_root["email"].asString();
        auto name = src_root["user"].asString();
        auto pwd = src_root["passwd"].asString();
        auto confirm = src_root["confirm"].asString();
        if (pwd != confirm) {   //确保两次输入的密码是一致的
            std::cout << "password err" << std::endl;
            root["error"] = ErrorCodes::PASSWD_ERR;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->m_response.body()) << jsonstr;
            return true;
        }
        //先查找redis中email对应验证码是否合理
        std::string varify_code;
        bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);    //email-varify_code
        if (!b_get_varify) {
            std::cout << " get varify code expired" << std::endl;
            root["error"] = ErrorCodes::VARIFY_EXPIRED;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->m_response.body()) << jsonstr;
            return true;
        }
        if (varify_code != src_root["varifycode"].asString()) {
            std::cout << " varify code error" << std::endl;
            root["error"] = ErrorCodes::VARIFY_CODE_ERR;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->m_response.body()) << jsonstr;
            return true;
        }
        //访问redis查找
        //bool b_usr_exist = RedisMgr::GetInstance()->ExistsKey(src_root["user"].asString());
        //if (b_usr_exist) {
        //    std::cout << " user exist" << std::endl;
        //    root["error"] = ErrorCodes::USER_EXIST;
        //    std::string jsonstr = root.toStyledString();
        //    beast::ostream(connection->m_response.body()) << jsonstr;
        //    return true;
        //}
        //查找数据库判断用户是否存在
        //root["error"] = 0;
        //root["email"] = src_root["email"];
        //root["user"] = src_root["user"].asString();
        //root["passwd"] = src_root["passwd"].asString();
        //root["confirm"] = src_root["confirm"].asString();
        int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd);
        if (uid == 0 || uid == -1) {
            std::cout << " user or email exist" << std::endl;
            root["error"] = ErrorCodes::USER_EXIST;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->m_response.body()) << jsonstr;
            return true;
        }
        root["error"] = 0;
        root["uid"] = uid;
        root["email"] = email;
        root["user"] = name;
        root["passwd"] = pwd;
        root["confirm"] = confirm;
        root["varifycode"] = src_root["varifycode"].asString();
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->m_response.body()) << jsonstr;    //返回给前端
        return true;
    });
    //重置回调逻辑
    RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str = boost::beast::buffers_to_string(connection->m_request.body().data());
        std::cout << "reset_pwd receive body is " << body_str << std::endl;
        connection->m_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool parse_success = reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::ERROR_JSON;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->m_response.body()) << jsonstr;
            return true;
        }
        auto email = src_root["email"].asString();
        auto name = src_root["user"].asString();
        auto pwd = src_root["passwd"].asString();
        //匹配Redis中email对应验证码
        std::string varify_code;
        bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);
        if (!b_get_varify) {
            std::cout << " get varify code expired" << std::endl;
            root["error"] = ErrorCodes::VARIFY_EXPIRED;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->m_response.body()) << jsonstr;
            return true;
        }
        if (varify_code != src_root["varifycode"].asString()) {
            std::cout << " varify code error" << std::endl;
            root["error"] = ErrorCodes::VARIFY_CODE_ERR;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->m_response.body()) << jsonstr;
            return true;
        }
        //查询数据库判断用户名和邮箱是否匹配
        bool email_valid = MysqlMgr::GetInstance()->CheckEmail(name, email);
        if (!email_valid) {
            std::cout << " user email do not match" << std::endl;
            root["error"] = ErrorCodes::EMAIL_NOT_MATCH;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->m_response.body()) << jsonstr;
            return true;
        }
        //更新密码为最近密码
        bool b_up = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
        if (!b_up) {
            std::cout << " update pwd failed" << std::endl;
            root["error"] = ErrorCodes::PASSWD_UPDATE_FAILED;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->m_response.body()) << jsonstr;
            return true;
        }

        std::cout << "succeed to update password" << std::endl;
        root["error"] = 0;
        root["email"] = email;
        root["user"] = name;
        root["passwd"] = pwd;
        root["varifycode"] = src_root["varifycode"].asString(); //将json一个值转化为string
        std::string jsonstr = root.toStyledString();            //将json结构转化为string
        beast::ostream(connection->m_response.body()) << jsonstr;
        return true;
    });
    //用户登录逻辑
    RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str = boost::beast::buffers_to_string(connection->m_request.body().data());
        std::cout << "reset_pwd receive body is " << body_str << std::endl;
        connection->m_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool parse_success = reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::ERROR_JSON;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->m_response.body()) << jsonstr;
            return true;
        }
        auto email = src_root["email"].asString();
        auto pwd = src_root["passwd"].asString();
        UserInfo userInfo;
        //查询数据库判断用户名和邮箱是否匹配
        bool pwd_valid = MysqlMgr::GetInstance()->CheckPwd(email, pwd, userInfo);
        if (!pwd_valid) {
            std::cout << " email pwd do not match" << std::endl;
            root["error"] = ErrorCodes::PASSWD_INVALID;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->m_response.body()) << jsonstr;
            return true;
        }
        //查询StatusServer找到合适的连接
        auto reply = StatusGrpcClient::GetInstance()->GetChatServer(userInfo.uid);
        if (reply.error()) {
            std::cout << " grpc get ChatServer failed, error is " << reply.error() << std::endl;
            root["error"] = ErrorCodes::RPC_FAILED;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->m_response.body()) << jsonstr;
            return true;
        }
        //登录成功
        std::cout << "succeed to load userinfo uid is " << userInfo.uid << std::endl;
        root["error"] = 0;
        root["email"] = email;
        root["uid"] = userInfo.uid;
        root["token"] = reply.token();
        root["host"] = reply.host();
        root["port"] = reply.port();
        std::string jsonstr = root.toStyledString();            //将json结构转化为string
        beast::ostream(connection->m_response.body()) << jsonstr;
        return true;
    });

}

LogicSystem::~LogicSystem() {}

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> conn) {
    if (m_getHandlers.find(path) == m_getHandlers.end()) {//找不到对应的handler返回false
        return false;
    }

    m_getHandlers[path](conn);  //返回对应的handler
    return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> conn) {
    if (m_postHandlers.find(path) == m_postHandlers.end()) {//找不到对应的handler返回false
        return false;
    }

    m_postHandlers[path](conn);  //返回对应的handler
    return true;
}

void LogicSystem::RegGet(std::string url, HttpHandler handler) {
    m_getHandlers.insert(make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, HttpHandler handler) {
    m_postHandlers.insert(make_pair(url, handler));
}