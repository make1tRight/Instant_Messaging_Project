#include "RedisMgr.h"
#include "ConfigMgr.h"
#include "const.h"
#include <string.h>
#include <mutex>

RedisConnPool::RedisConnPool(std::size_t poolSize, const char* host,
     int port, const char* passwd)
    : _poolSize(poolSize), _host(host), _port(port), _b_stop(false) {
    for (int i = 0; i < _poolSize; ++i) {
        redisContext* conn = redisConnect(_host, _port);
        if (conn == nullptr) {
            continue;
        }
        if (conn->err != 0) {
            redisFree(conn);
            continue;
        }

        auto reply = (redisReply*)redisCommand(conn, "AUTH %s", passwd);
        if (reply == nullptr) {
            std::cout << "Authentication failed." << std::endl;
            continue;
        }
        if (reply->type == REDIS_REPLY_ERROR) {
            std::cout << "Authentication failed." << std::endl;
            freeReplyObject(reply);
            continue;
        }

        std::cout << "Authentication successful." << std::endl;
        freeReplyObject(reply);
        _conns.push(conn);
    }
}

RedisConnPool::~RedisConnPool() {
    std::lock_guard<std::mutex> lock(_mutex);
    while (!_conns.empty()) {
        _conns.pop();
    }
}

redisContext* RedisConnPool::GetConnection() {
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock, [this]() {
        if (_b_stop) {
            return true;
        }
        return !_conns.empty();
    });
    if (_b_stop) {
        return nullptr;
    }
    redisContext* c = _conns.front();
    _conns.pop();
    return c;
}

void RedisConnPool::ReturnConnection(redisContext* conn) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_b_stop) {
        return;
    }
    _conns.push(conn);
    _cond.notify_one();
}

void RedisConnPool::Close() {
    _b_stop = true;
    _cond.notify_all();
}

RedisMgr::~RedisMgr() {
    Close();
}

bool RedisMgr::Connect(const std::string& host, int port) {
    _context = redisConnect(host.c_str(), port);
    if (_context == nullptr) {
        std::cout << "_context is nullptr" << std::endl;
        return false;
    }
    if (_context != nullptr && _context->err) {
        std::cout << "Redis connect err: " << _context->errstr << std::endl;
        return false;
    }
    return true;
}

bool RedisMgr::Auth(const std::string& passwd) {
    auto conn = _conn_pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _conn_pool->ReturnConnection(conn);
    });
    _reply = (redisReply*)redisCommand(conn, "AUTH %s", passwd.c_str());
    if (_reply == nullptr) {
        std::cout << "Authentication failed." << std::endl;
        return false;
    }
    if (_reply->type == REDIS_REPLY_ERROR) {
        std::cout << "Authentication failed." << std::endl;
        freeReplyObject(_reply);
        return false;
    }

    std::cout << "Authentication successful." << std::endl;
    freeReplyObject(_reply);   
    return true;
}

bool RedisMgr::Get(const std::string& key, std::string& value) {
    auto conn = _conn_pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _conn_pool->ReturnConnection(conn);
    });
    _reply = (redisReply*)redisCommand(conn, "GET %s", key.c_str());
    if (_reply == nullptr) {
        std::cout << "[ GET " << key << " ] failed." << std::endl;
        return false;
    }
    if (_reply->type != REDIS_REPLY_STRING) {
        std::cout << "[ GET " << key << " ] failed." << std::endl;
        freeReplyObject(_reply);
        return false;
    }

    value = _reply->str;
    std::cout << "[ GET " << key << " ] successful." << std::endl;
    freeReplyObject(_reply);
    return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& value) {
    auto conn = _conn_pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _conn_pool->ReturnConnection(conn);
    });
    _reply = (redisReply*)redisCommand(conn, "SET %s %s", key.c_str(), value.c_str());
    if (_reply == nullptr) {
        std::cout << "[ SET " << key << " " << value << " ] failed." << std::endl;
        return false;
    }
    if (!(_reply->type == REDIS_REPLY_STATUS 
        && (strcmp(_reply->str, "OK") == 0 || strcmp(_reply->str, "ok") == 0))) {
        std::cout << "[ SET " << key << " " << value << " ] failed." << std::endl;
        freeReplyObject(_reply);
        return false;
    }

    std::cout << "[ SET " << key << " " << value << " ] successful." << std::endl;
    freeReplyObject(_reply);
    return true;
}

bool RedisMgr::HSet(const std::string& key, 
    const std::string& hkey, const std::string& value) {
    auto conn = _conn_pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _conn_pool->ReturnConnection(conn);
    });
    _reply = (redisReply*)redisCommand(conn, "HSET %s %s %s",
         key.c_str(), hkey.c_str(), value.c_str());
    if (_reply == nullptr) {
        std::cout << "[ HSET " << key 
            << " " << hkey 
            << " " << value 
            << " ] failed." << std::endl;
        return false;
    }
    if (_reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "[ HSET " << key 
            << " " << hkey 
            << " " << value 
            << " ] failed." << std::endl;
        freeReplyObject(_reply);
        return false;
    }

    std::cout << "[ HSET " << key << " " << hkey << " " << value << " ] successful." << std::endl;
    freeReplyObject(_reply);
    return true;
}
bool RedisMgr::HSet(const char* key, const char* hkey,
     const char* value, std::size_t value_len) {
    auto conn = _conn_pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _conn_pool->ReturnConnection(conn);
    });
    const char* argv[4];
    std::size_t argvlen[4];
    argv[0] = "HSET";
    argvlen[0] = 4;
    argv[1] = key;
    argvlen[1] = strlen(key);
    argv[2] = hkey;
    argvlen[2] = strlen(hkey);
    argv[3] = value;
    argvlen[3] = value_len;
    
    _reply = (redisReply*)redisCommandArgv(conn, 4, argv, argvlen);
    if (_reply == nullptr) {
        std::cout << "[ HSET " << key 
            << " " << hkey 
            << " " << value 
            << " ] failed." << std::endl;
        return false;
    }
    if (_reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "[ HSET " << key 
            << " " << hkey 
            << " " << value 
            << " ] failed." << std::endl;
        freeReplyObject(_reply);
        return false;
    }

    std::cout << "[ HSET " << key << " " << hkey << " " << value << " ] successful." << std::endl;
    freeReplyObject(_reply);
    return true;
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hkey) {
    auto conn = _conn_pool->GetConnection();
    if (conn == nullptr) {
        return "";
    }
    Defer defer([this, &conn]() {
        _conn_pool->ReturnConnection(conn);
    });
    
    const char* argv[3];
    std::size_t argvlen[3];

    argv[0] = "HGET";
    argvlen[0] = 4;
    argv[1] = key.c_str();
    argvlen[1] = key.size();
    argv[2] = hkey.c_str();
    argvlen[2] = hkey.size();
    _reply = (redisReply*)redisCommandArgv(conn, 3, argv, argvlen);//argv可防备参数带空格的情况
    if (_reply == nullptr) {
        std::cout << "[ HGET " << key << " " << hkey << " ] failed." << std::endl;
        return "";
    }
    if (_reply->type == REDIS_REPLY_NIL) {//防备哈希值不存在
        std::cout << "[ HGET " << key << " " << hkey << " ] failed." << std::endl;
        freeReplyObject(_reply);
        return "";
    }

    std::string value = _reply->str;
    std::cout << "[ HGET " << key << " " << hkey << " ] successful." << std::endl;
    freeReplyObject(_reply);
    return value;
}

bool RedisMgr::LPush(const std::string& key, const std::string& value) {
    auto conn = _conn_pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _conn_pool->ReturnConnection(conn);
    });
    _reply = (redisReply*)redisCommand(conn, "LPUSH %s %s", key.c_str(), value.c_str());
    if (_reply == nullptr) {
        std::cout << "[ LPUSH " << key << " " << value << " ] failed." << std::endl;
        return false;
    }
    if (_reply->type != REDIS_REPLY_INTEGER 
             || _reply->integer <= 0) {
        std::cout << "[ LPUSH " << key << " " << value << " ] failed." << std::endl;
        freeReplyObject(_reply);
        return false;
    }

    std::cout << "[ LPUSH " << key << " " << value << " ] successful." << std::endl;
    freeReplyObject(_reply);
    return true;
}

bool RedisMgr::LPop(const std::string& key, std::string& value) {
    auto conn = _conn_pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _conn_pool->ReturnConnection(conn);
    });
    _reply = (redisReply*)redisCommand(conn, "LPOP %s", key.c_str());
    if (_reply == nullptr) {
        std::cout << "[ LPOP " << key << " ] failed." << std::endl;
        return false;
    }
    if (_reply->type == REDIS_REPLY_NIL) {
        std::cout << "[ LPOP " << key << " ] failed." << std::endl;
        freeReplyObject(_reply);
        return false;
    }
    value = _reply->str;
    std::cout << "[ LPOP " << key << " ] successful." << std::endl;
    freeReplyObject(_reply);
    return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value) {
    auto conn = _conn_pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _conn_pool->ReturnConnection(conn);
    });
    _reply = (redisReply*)redisCommand(conn, "RPUSH %s %s", key.c_str(), value.c_str());
    if (_reply == nullptr) {
        std::cout << "[ RPUSH " << key << " " << value << " ] failed." << std::endl;
        return false;
    }
    if (_reply->type != REDIS_REPLY_INTEGER 
             || _reply->integer <= 0) {
        std::cout << "[ RPUSH " << key << " " << value << " ] failed." << std::endl;
        freeReplyObject(_reply);
        return false;
    }

    std::cout << "[ RPUSH " << key << " " << value << " ] successful." << std::endl;
    freeReplyObject(_reply);
    return true;
}

bool RedisMgr::RPop(const std::string& key, std::string& value) {
    auto conn = _conn_pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _conn_pool->ReturnConnection(conn);
    });
    _reply = (redisReply*)redisCommand(conn, "RPOP %s", key.c_str());
    if (_reply == nullptr) {
        std::cout << "[ RPOP " << key << " ] failed." << std::endl;
        return false;
    }
    if (_reply->type == REDIS_REPLY_NIL) {
        std::cout << "[ RPOP " << key << " ] failed." << std::endl;
        freeReplyObject(_reply);
        return false;
    }
    value = _reply->str;
    std::cout << "[ RPOP " << key << " ] successful." << std::endl;
    freeReplyObject(_reply);
    return true;
}

bool RedisMgr::Del(const std::string& key) {
    auto conn = _conn_pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _conn_pool->ReturnConnection(conn);
    });
    _reply = (redisReply*)redisCommand(conn, "DEL %s", key.c_str());
    if (_reply == nullptr) {
        std::cout << "[ DEL " << key << " ] failed." << std::endl;
        return false;
    }
    if (_reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "[ DEL " << key << " ] failed." << std::endl;
        freeReplyObject(_reply);
        return false;
    }
    std::cout << "[ DEL " << key << " ] successful." << std::endl;
    freeReplyObject(_reply);
    return true;
}

bool RedisMgr::HDel(const std::string& key, const std::string& field) {
    auto conn = _conn_pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _conn_pool->ReturnConnection(conn);
    });
    _reply = (redisReply*)redisCommand(conn, "HDEL %s %s", key.c_str(), field.c_str());
    if (_reply == nullptr) {
        std::cout << "[ HDEL " << key << " " << field << " ] failed." << std::endl;
        return false;
    }
    bool success = false;
    if (_reply->type == REDIS_REPLY_INTEGER) {
        success = _reply->integer > 0;
    }
    std::cout << "[ HDEL " << key << " " << field << " ] successful." << std::endl;
    freeReplyObject(_reply);
    return success;
}

bool RedisMgr::ExistsKey(const std::string& key) {
    auto conn = _conn_pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _conn_pool->ReturnConnection(conn);
    });
    _reply = (redisReply*)redisCommand(conn, "EXISTS %s", key.c_str());
    if (_reply == nullptr) {
        std::cout << "Not found key [ " << key << " ]." << std::endl;
        return false;
    }
    if (_reply->type != REDIS_REPLY_INTEGER || _reply->integer <= 0) {
        std::cout << "Not found key [ " << key << " ]." << std::endl;
        freeReplyObject(_reply);
        return false;
    }
    std::cout << "Found key [ " << key << " ] successful." << std::endl;
    freeReplyObject(_reply);
    return true;
}

void RedisMgr::Close() {
    // redisFree(_context);
    _conn_pool->Close();
}

RedisMgr::RedisMgr() {
    auto cfg = ConfigMgr::GetInstance();
    auto host = (*cfg)["Redis"]["Host"];
    auto port = (*cfg)["Redis"]["Port"];
    auto passwd = (*cfg)["Redis"]["Passwd"];
    // _conn_pool.reset(new RedisConnPool(5,
            // host.c_str(), atoi(port.c_str()), passwd.c_str()));
    
    _conn_pool = std::make_unique<RedisConnPool>(5,
            host.c_str(), atoi(port.c_str()), passwd.c_str());
}