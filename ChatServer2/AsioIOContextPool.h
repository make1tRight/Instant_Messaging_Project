#ifndef ASIOIOCONTEXTPOOL_H
#define ASIOIOCONTEXTPOOL_H
#include "Singleton.h"
#include <vector>
#include <thread>
#include <memory>
#include <boost/asio.hpp>
using io_context = boost::asio::io_context;
using Work = boost::asio::io_context::work;
using WorkPtr = std::unique_ptr<Work>;

class CServer;
class AsioIOContextPool : public Singleton<AsioIOContextPool> {
    friend class Singleton<AsioIOContextPool>;
    friend class CServer;
public:
    ~AsioIOContextPool();

    io_context& GetIOContext();
    void Stop();
private:
    AsioIOContextPool(std::size_t size = std::thread::hardware_concurrency());
    AsioIOContextPool(const AsioIOContextPool&) = delete;
    AsioIOContextPool& operator=(const AsioIOContextPool&) = delete;

    std::vector<io_context> _io_contexts;
    std::vector<WorkPtr> _works;
    std::vector<std::thread> _threads;
    std::size_t _next_iocontext;
    std::size_t _max_poolsize;
    bool _b_stop;
};
#endif // ASIOIOCONTEXTPOOL_H