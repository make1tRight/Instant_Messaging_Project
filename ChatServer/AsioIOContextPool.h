#pragma once
#include "const.h"
#include "Singleton.h"

class AsioIOContextPool : public Singleton<AsioIOContextPool>
{
    friend Singleton<AsioIOContextPool>;
public:
    using IOContext = boost::asio::io_context;
    using Work = boost::asio::io_context::work; //保证ioc在没有事件以后也可以不退出
    using WorkPtr = std::unique_ptr<Work>;
    ~AsioIOContextPool();
    AsioIOContextPool(const AsioIOContextPool&) = delete;
    AsioIOContextPool& operator=(const AsioIOContextPool&) = delete;
    // 使用round-robin的方式返回一个io_context
    IOContext& GetIOContext();
    void Stop();
private:
    AsioIOContextPool(std::size_t size = std::thread::hardware_concurrency());
    std::vector<IOContext> m_ioContexts;
    std::vector<WorkPtr> m_works;
    std::vector<std::thread> m_threads; //有多少个上下文就有多少个线程, 因为上下文要跑在线程里
    std::size_t m_nextIOContext;        //返回下一个IOContext
};