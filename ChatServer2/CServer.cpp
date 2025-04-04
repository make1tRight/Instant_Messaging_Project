#include "CServer.h"
#include <iostream>
#include "AsioIOContextPool.h"
#include "CSession.h"
#include "UserMgr.h"


CServer::CServer(boost::asio::io_context& ioc, short port)
    : _ioc(ioc),  _port(port), _acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {
    std::cout << "ChatServer listening on port: " << _port << std::endl;
    StartAccept();
}

CServer::~CServer() {
    std::cout << "ChatServer stops listening on port: " << _port << std::endl;
}

void CServer::ClearSession(std::string session_id) {
    if (_sessions.find(session_id) != _sessions.end()) {
        UserMgr::GetInstance()->RmvSession(_sessions[session_id]->GetUserId());
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions.erase(session_id);
    }
}

// 为什么Start()用于建立http短连接, StartAccept()用于建立tcp长连接
// void CServer::Start() {
//     auto self = shared_from_this();
//     auto& ioc = AsioIOContextPool::GetInstance()->GetIOContext();
//     auto new_conn = std::make_shared<HttpConnection>(ioc);
//     _acceptor.async_accept(new_conn->GetSocket(), [self, new_conn](beast::error_code ec) {
//         try {
//             if (ec) {
//                 self->Start();
//                 return;
//             }
//             new_conn->Start();
//             // http请求后连接通常会关闭, 服务器需要重新Start()来接受新的连接
//             self->Start();
//         }
//         catch (std::exception& e) {
//             std::cout << "CServer::Start() Exception: " << e.what() << std::endl;
//             // self->Start();
//         }
//     });
// }

void CServer::StartAccept() {
    auto& ioc = AsioIOContextPool::GetInstance()->GetIOContext();
    std::shared_ptr<CSession> new_session = std::make_shared<CSession>(ioc, this);
    if (!new_session) {
        std::cout << "new_session is nullptr." << std::endl;
    }
    // 通过HandleAccept来控制下一次接受连接的时机
    _acceptor.async_accept(new_session->GetSocket(), std::bind(
        &CServer::HandleAccept, this, new_session, std::placeholders::_1));
}

void CServer::HandleAccept(std::shared_ptr<CSession> new_session,
    const boost::system::error_code& error) {
    if (!error) {
        new_session->Start();
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions.insert(std::make_pair(new_session->GetSessionId(), new_session));
        std::cout << "HandleAccept() called." << std::endl;
    } else {
        std::cout << "Failed to HandleAccept(), error: " 
            << error.message() << std::endl;
    }

    StartAccept();
}