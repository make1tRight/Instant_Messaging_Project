#include "AsioIOContextPool.h"

AsioIOContextPool::AsioIOContextPool(std::size_t size/*std::thread::hardware_concurrency()*/):
    m_ioContexts(size), m_works(size), m_nextIOContext(0) {
    // m_works是监工, 有work才能够保证线程执行完run以后不会直接退出
    // 线程不直接退出, 后续才能够继续进行读写等逻辑操作
    // m_works绑定iocontext
    for (std::size_t i = 0; i < size; ++i) {
        m_works[i] = std::unique_ptr<Work>(new Work(m_ioContexts[i]));
    }

    // 一个线程跑一个ioContext
    for (std::size_t i = 0; i < m_ioContexts.size(); ++i) {
        m_threads.emplace_back([this, i]() {
            m_ioContexts[i].run();
        });
    }
}

AsioIOContextPool::~AsioIOContextPool() {
    Stop(); //RAII; 如果不放在这里就要在调用Pool的逻辑里调用
    std::cout << "AsioIOContextPool destruct" << std::endl;
}

AsioIOContextPool::IOContext& AsioIOContextPool::GetIOContext() {
    // 有一个负载均衡的作用
    auto& context = m_ioContexts[m_nextIOContext++];
    if (m_nextIOContext == m_ioContexts.size()) {
        m_nextIOContext = 0;
    }
    return context;
}

void AsioIOContextPool::Stop() {
    for (auto& work : m_works) {
        work->get_io_context().stop();
        work.reset();   //变成空指针, 回收上下文; 线程执行完run以后就能够返回了
    }

    for (auto& t : m_threads) {
        t.join();       //等待线程回收完毕才真正stop, 避免线程没回收完毕就退出导致系统崩溃
    }
}