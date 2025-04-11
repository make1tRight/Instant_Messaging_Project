#ifndef REDISMGR_H
#define REDISMGR_H
#include "Singleton.h"
#include <hiredis/hiredis.h>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <memory>

class RedisConnPool {
public:
    RedisConnPool(std::size_t poolSize, const char* host,
         int port, const char* passwd);
    ~RedisConnPool();

    redisContext* GetConnection();
    void ReturnConnection(redisContext* conn);
    void Close();
private:
    std::atomic<bool> _b_stop;
    std::size_t _poolSize;
    const char* _host;
    int _port;
    std::mutex _mutex;
    std::queue<redisContext*> _conns;
    std::condition_variable _cond;
};

class RedisMgr : public Singleton<RedisMgr>,
     std::enable_shared_from_this<RedisMgr> {
    friend class Singleton<RedisMgr>;
public:
    ~RedisMgr();
    bool Auth(const std::string& passwd);
    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value);
    bool HSet(const std::string& key, const std::string& hkey,
         const std::string& value);
    bool HSet(const char* key, const char* hkey,
         const char* value, std::size_t value_len);
    std::string HGet(const std::string& key, const std::string& hkey);
    bool LPush(const std::string& key, const std::string& value);
    bool LPop(const std::string& key, std::string& value);
    bool RPush(const std::string& key, const std::string& value);
    bool RPop(const std::string& key, std::string& value);
    bool Del(const std::string& key);
    bool ExistsKey(const std::string& key);

    void Close();
private:
    RedisMgr();
    std::unique_ptr<RedisConnPool> _conn_pool;
};
#endif // REDISMGR_H