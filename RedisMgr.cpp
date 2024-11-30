#include "RedisMgr.h"
#include "ConfigMgr.h"

RedisMgr::~RedisMgr() {
    Close();
}

//bool RedisMgr::Connect(const std::string& host, int port) {
//    connect = redisConnect(host.c_str(), port);
//    if (nullptr == connect) {
//        return false;
//    }
//    if (connect != nullptr && connect->err) {
//        std::cout << "connect error " << connect->errstr << std::endl;
//        return false;
//    }
//    return true;
//}

bool RedisMgr::Get(const std::string& key, std::string& value) {
    auto connect = m_conPool->getConnection();
    if (connect == nullptr) {
        return false;
    }
    Defer defer([&connect, this]() {
        m_conPool->returnConnection(std::move(connect));
    });

    auto reply = (redisReply*)redisCommand(connect, "GET %s", key.c_str());  //这里一定要用c_str()转换一下才能传给redisCommand
    if (nullptr == connect) {
        std::cout << "Failed to execute [ GET " << key << " ]!" << std::endl;
        //freeReplyObject(reply);
        //m_conPool->returnConnection(connect);
        return false;
    }
    if (reply->type != REDIS_REPLY_STRING) {
        std::cout << "Failed to execute [ GET " << key << " ] failed" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    value = reply->str;
    freeReplyObject(reply);

    std::cout << "Execute command [ GET " << key << " ] successfully!" << std::endl;
    return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& value) {
    auto connect = m_conPool->getConnection();
    if (connect == nullptr) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "SET %s %s", key.c_str(), value.c_str());  //这里一定要用c_str()转换一下才能传给redisCommand
    if (nullptr == reply) {
        std::cout << "Failed to execute [ SET " << key << " " << value << " ]!" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    if (!(reply->type == REDIS_REPLY_STATUS && (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0))) {
        std::cout << "Failed to execute [ SET " << key << " " << value << " ]!" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    std::cout << "Execute command [ SET " << key << " " << value << " ] successfully!" << std::endl;
    return true;
}

bool RedisMgr::Auth(const std::string& password) {
    auto connect = m_conPool->getConnection();
    if (connect == nullptr) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "AUTH %s", password.c_str());  //这里一定要用c_str()转换一下才能传给redisCommand
    if (reply->type == REDIS_REPLY_ERROR) {
        std::cout << "Authentication failed" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    else {
        freeReplyObject(reply);
        std::cout << "Authentication successful" << std::endl;
        return true;
    }
}

bool RedisMgr::LPush(const std::string& key, const std::string& value) {
    auto connect = m_conPool->getConnection();
    if (connect == nullptr) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "LPUSH %s %s", key.c_str(), value.c_str());  //这里一定要用c_str()转换一下才能传给redisCommand
    if (nullptr == reply) {
        std::cout << "Failed to execute [ LPUSH " << key << " " << value << " ]!" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {    //插入成功INTEGER是1, 否则为0
        std::cout << "Failed to execute [ LPUSH " << key << " " << value << " ]!" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout << "Execute command [ LPUSH " << key << " " << value << " ] successfully!" << std::endl;
    freeReplyObject(reply);
    return true;
}

bool RedisMgr::LPop(const std::string& key, std::string& value) {
    auto connect = m_conPool->getConnection();
    if (connect == nullptr) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "LPOP %s", key.c_str());  //这里一定要用c_str()转换一下才能传给redisCommand
    if (nullptr == reply || reply->type == REDIS_REPLY_NIL) {
        std::cout << "Failed to execute [ LPOP " << key << " ]!" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    value = reply->str;
    std::cout << "Execute command [ LPOP " << key << " ] successfully!" << std::endl;
    freeReplyObject(reply);
    return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value) {
    auto connect = m_conPool->getConnection();
    if (connect == nullptr) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "RPUSH %s %s", key.c_str(), value.c_str());  //这里一定要用c_str()转换一下才能传给redisCommand
    if (nullptr == reply) {
        std::cout << "Failed to execute [ RPUSH " << key << " " << value << " ]!" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {    //插入成功INTEGER是1, 否则为0
        std::cout << "Failed to execute [ RPUSH " << key << " " << value << " ]!" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout << "Execute command [ RPUSH " << key << " " << value << " ] successfully!" << std::endl;
    freeReplyObject(reply);
    return true;
}

bool RedisMgr::RPop(const std::string& key, std::string& value) {
    auto connect = m_conPool->getConnection();
    if (connect == nullptr) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "RPOP %s", key.c_str());  //这里一定要用c_str()转换一下才能传给redisCommand
    if (nullptr == reply || reply->type == REDIS_REPLY_NIL) {
        std::cout << "Failed to execute [ RPOP " << key << " ]!" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    value = reply->str;
    std::cout << "Execute command [ RPOP " << key << " ] successfully!" << std::endl;
    freeReplyObject(reply);
    return true;
}

bool RedisMgr::HSet(const std::string& key, const std::string& hkey, const std::string& value) {    //二级缓存的map, 也就是有两层key
    auto connect = m_conPool->getConnection();
    if (connect == nullptr) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());  //这里一定要用c_str()转换一下才能传给redisCommand
    if (nullptr == reply || reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Failed to execute [ HSET " << key << " "  << hkey << " " << value << " ]!" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout << "Execute command [ HSET " << key << " " << hkey << " " << value << " ] successfully!" << std::endl;
    freeReplyObject(reply);
    return true;
}

bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen) {  //用于处理二进制数
    auto connect = m_conPool->getConnection();
    if (connect == nullptr) {
        return false;
    }
    const char* argv[4];
    size_t argvlen[4];
    argv[0] = "HSET";
    argvlen[0] = 4;
    argv[1] = key;
    argvlen[1] = strlen(key);
    argv[2] = hkey;
    argvlen[2] = strlen(hkey);
    argv[3] = hvalue;
    argvlen[3] = hvaluelen;
    auto reply = (redisReply*)redisCommandArgv(connect, 4, argv, argvlen);
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Failed to execute [ HSET " << key << "  " << hkey << "  " << hvalue << " ]!" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout << "Execute command [ HSET " << key << "  " << hkey << "  " << hvalue << " ] successfully!" << std::endl;
    freeReplyObject(reply);
    return true;
}

bool RedisMgr::HDel(const std::string& key, const std::string& field)
{
    auto connect = m_conPool->getConnection();
    if (connect == nullptr) {
        return false;
    }
    Defer defer([&connect, this]() {
        m_conPool->returnConnection(std::move(connect));
    });
    redisReply* reply = (redisReply*)redisCommand(connect, "HDEL %s %s", key.c_str(), field.c_str());
    if (reply == nullptr) {
        std::cerr << "HDEL command failed" << std::endl;
        return false;
    }
    bool success = false;
    if (reply->type == REDIS_REPLY_INTEGER) {//成功了会返回删除记录的数量
        success = reply->integer > 0;
    }
    freeReplyObject(reply);
    return success;
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hkey) {
    auto connect = m_conPool->getConnection();
    if (connect == nullptr) {
        return "";
    }
    const char* argv[3];
    size_t argvlen[3];
    argv[0] = "HGET";
    argvlen[0] = 4;
    argv[1] = key.c_str();
    argvlen[1] = key.length();
    argv[2] = hkey.c_str();
    argvlen[2] = hkey.length();
    auto reply = (redisReply*)redisCommandArgv(connect, 3, argv, argvlen);
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        std::cout << "Failed to execute [ HGET " << key << " " << hkey << " ]!" << std::endl;
        return "";
    }

    std::string value = reply->str;
    freeReplyObject(reply);
    std::cout << "Execute command [ HGET " << key << " " << hkey << " ] successfully!" << std::endl;
    return value;
}

bool RedisMgr::Del(const std::string& key) {
    auto connect = m_conPool->getConnection();
    if (connect == nullptr) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "DEL %s", key.c_str());  //这里一定要用c_str()转换一下才能传给redisCommand
    if (nullptr == reply || reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Failed to execute [ DEL " << key << " ]!" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout << "Execute command [ DEL " << key << " ] successfully!" << std::endl;
    freeReplyObject(reply);
    return true;
}

bool RedisMgr::ExistsKey(const std::string& key) {
    auto connect = m_conPool->getConnection();
    if (connect == nullptr) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "exists %s", key.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER || reply->integer == 0) {
        std::cout << "Not Found [ Key " << key << " ]!" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout << " Found [ Key " << key << " ] exists!" << std::endl;
    freeReplyObject(reply);
    return true;
}

void RedisMgr::Close() {
    //redisFree(m_connect);
    m_conPool->Close();
}

RedisMgr::RedisMgr() {
    auto& gCfrMgr = ConfigMgr::Inst();
    auto host = gCfrMgr["Redis"]["Host"];
    auto port= gCfrMgr["Redis"]["Port"];
    auto pwd = gCfrMgr["Redis"]["Passwd"];
    m_conPool.reset(new RedisConPool(5, host.c_str(), atoi(port.c_str()), pwd.c_str()));
}