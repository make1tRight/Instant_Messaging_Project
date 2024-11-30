#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOContextPool.h"

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port):
    m_ioc(ioc), m_acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {

}

void CServer::Start() {
    //[5-12:00]防止回调函数调用的时候类被析构掉, 这里使用智能指针, 靠引用计数来维持类对象的生命周期
    auto self = shared_from_this();
    auto& io_context = AsioIOContextPool::GetInstance()->GetIOContext();
    std::shared_ptr<HttpConnection> new_conn = std::make_shared<HttpConnection>(io_context);
    //异步接收
    m_acceptor.async_accept(new_conn->GetSocket(), [self, new_conn](beast::error_code ec) {    //这里捕获是为了增加引用计数
        try {
            //出错放弃当前连接, 继续监听其他连接
            if (ec) {
                self->Start(); //监听其他连接
                return;
            }

            //创建新连接, 并且创建HttpConnection类管理这个连接
            //std::make_shared<HttpConnection>(std::move(self->m_socket))->Start();      //[5-17:56]关于右值引用和移动构造
            new_conn->Start(); //线程池替换单线程 
            //HttpConnection(std::move(m_socket));

            //继续监听
            self->Start();
        }
        catch (std::exception& exp) {
            std::cout << "exception is " << exp.what() << std::endl;
            self->Start();
        };
    });
}