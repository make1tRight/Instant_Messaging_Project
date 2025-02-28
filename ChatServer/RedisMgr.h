#pragma once
#include "const.h"

class RedisConPool {
public:
    RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
    : poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
        for (size_t i = 0; i < poolSize_; ++i) {    //初始化连接池, 创建连接池里面的连接
            auto* context = redisConnect(host, port);
            if (context == nullptr || context->err != 0) {
                if (context != nullptr) {
                    redisFree(context);             //初始化失败就释放掉redisContext对象的内存(这个是Redis连接上下文的内存)
                }
                continue;                           //成功就不用管他
            }
            auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
            if (reply->type == REDIS_REPLY_ERROR) {
                std::cout << "Authentification failed" << std::endl;
                //释放redisCommand执行后返回的redisReply所占用的内存(这个是释放命令执行结果的内存)
                freeReplyObject(reply);
                redisFree(context);
                continue;
            }
            freeReplyObject(reply);
            std::cout << "Authentification successful" << std::endl;
            connections_.push(context);             //放回到池子里
        }
    }

    ~RedisConPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty()) {
            connections_.pop();
        }
    }

    redisContext* getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]() {
            if (b_stop_) {
                return true;//关机了 -> true -> 直接返回(代码逻辑继续往下运行)
            }
            return !connections_.empty();   //如果线程池还能继续访问, 但是线程池里面的线程被取光了 -> false -> 挂起等待锁
        });
        //停止状态直接返回空指针
        if (b_stop_) {
            return nullptr;
        }
        auto* context = connections_.front();
        connections_.pop();
        return context;
    }

    void returnConnection(redisContext* context) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        connections_.push(context);
        cond_.notify_one();                 //唤醒还在等待锁的线程
    }

    void Close() {
        b_stop_ = true;
        cond_.notify_all();                 //这个cond_是怎么和线程交互的呢?
    }

private:
    std::atomic<bool> b_stop_;
    size_t poolSize_;
    const char* host_;
    int port_;
    std::queue<redisContext*> connections_; //[12-42:36]连接池, 可以换成list使用双互斥量提高并发效率
    std::mutex mutex_;
    std::condition_variable cond_;
};

class RedisMgr : public Singleton<RedisMgr>
{
    friend class Singleton<RedisMgr>;
public:
    ~RedisMgr();
    //bool Connect(const std::string& host, int port);
    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value);
    bool Auth(const std::string& password);
    bool LPush(const std::string& key, const std::string& value);
    bool LPop(const std::string& key, std::string& value);
    bool RPush(const std::string& key, const std::string& value);
    bool RPop(const std::string& key, std::string& value);
    bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
    bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
    bool HDel(const std::string& key, const std::string& field);
    std::string HGet(const std::string& key, const std::string& hkey);
    bool Del(const std::string& key);
    bool ExistsKey(const std::string& key);
    void Close();
private:
    RedisMgr();
    //redisContext* m_connect;
    //redisReply* m_reply;
    std::unique_ptr<RedisConPool> m_conPool;
};