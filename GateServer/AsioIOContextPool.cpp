#include "AsioIOContextPool.h"

AsioIOContextPool::AsioIOContextPool(std::size_t size/*std::thread::hardware_concurrency()*/):
    m_ioContexts(size), m_works(size), m_nextIOContext(0) {
    // m_works�Ǽ๤, ��work���ܹ���֤�߳�ִ����run�Ժ󲻻�ֱ���˳�
    // �̲߳�ֱ���˳�, �������ܹ��������ж�д���߼�����
    // m_works��iocontext
    for (std::size_t i = 0; i < size; ++i) {
        m_works[i] = std::unique_ptr<Work>(new Work(m_ioContexts[i]));
    }

    // һ���߳���һ��ioContext
    for (std::size_t i = 0; i < m_ioContexts.size(); ++i) {
        m_threads.emplace_back([this, i]() {
            m_ioContexts[i].run();
        });
    }
}

AsioIOContextPool::~AsioIOContextPool() {
    Stop(); //RAII; ��������������Ҫ�ڵ���Pool���߼������
    std::cout << "AsioIOContextPool destruct" << std::endl;
}

AsioIOContextPool::IOContext& AsioIOContextPool::GetIOContext() {
    // ��һ�����ؾ��������
    auto& context = m_ioContexts[m_nextIOContext++];
    if (m_nextIOContext == m_ioContexts.size()) {
        m_nextIOContext = 0;
    }
    return context;
}

void AsioIOContextPool::Stop() {
    for (auto& work : m_works) {
        work->get_io_context().stop();
        work.reset();   //��ɿ�ָ��, ����������; �߳�ִ����run�Ժ���ܹ�������
    }

    for (auto& t : m_threads) {
        t.join();       //�ȴ��̻߳�����ϲ�����stop, �����߳�û������Ͼ��˳�����ϵͳ����
    }
}