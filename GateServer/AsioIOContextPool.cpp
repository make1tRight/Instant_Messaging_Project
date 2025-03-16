#include "AsioIOContextPool.h"

AsioIOContextPool::~AsioIOContextPool() {
    Stop();
}

io_context& AsioIOContextPool::GetIOContext() {
    auto& ioc = _io_contexts[_next_iocontext++];
    if (_next_iocontext == _max_poolsize) {
        _next_iocontext = 0;
    }
    return ioc;
}

void AsioIOContextPool::Stop() {
    _b_stop = true;
    for (auto& work : _works) {
        work->get_io_context().stop();
        work.reset();
    }

    for (auto& t : _threads) {
        t.join();
    }
}

AsioIOContextPool::AsioIOContextPool(std::size_t size)
     : _io_contexts(size), _works(size), _b_stop(false),
         _max_poolsize(size), _next_iocontext(0) {
    for (std::size_t i = 0; i < size; ++i) {
        _works[i] = std::unique_ptr<Work>(new Work(_io_contexts[i]));
    }
    
    for (std::size_t i = 0; i < _io_contexts.size(); ++i) {
        _threads.emplace_back([this, i]() {
            _io_contexts[i].run();
        });
    } 
}