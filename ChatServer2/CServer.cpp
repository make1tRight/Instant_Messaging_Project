#include "CServer.h"
#include "AsioIOContextPool.h"
#include "UserMgr.h"

CServer::CServer(boost::asio::io_context& ioc, unsigned short port):
    m_ioc(ioc), m_port(port), m_acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {
    std::cout << "Server start success, listen on port: " << m_port << std::endl;
    StartAccept();
}

CServer::~CServer() {
    std::cout << "Server destruct listen on port: " << m_port << std::endl;
}

void CServer::ClearSession(std::string session_id) {
    if (m_sessions.find(session_id) != m_sessions.end()) {
        //移除用户和session之间的关联
        UserMgr::GetInstance()->RmvUserSession(m_sessions[session_id]->GetUserId());
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions.erase(session_id);
    }
}

void CServer::HandleAccept(std::shared_ptr<CSession> new_session, const boost::system::error_code& error) {
    if (!error) {
        new_session->Start();
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions.insert(std::make_pair(new_session->GetSessionId(), new_session));
    }
    else {
        std::cout << "session accept failed, error is " << error.what() << std::endl;
    }
    StartAccept();
}

void CServer::StartAccept() {
    auto& io_context = AsioIOContextPool::GetInstance()->GetIOContext();
    std::shared_ptr<CSession> new_session = std::make_shared<CSession>(io_context, this);//CSession用于实际与client通信
    m_acceptor.async_accept(new_session->GetSocket(), std::bind(&CServer::HandleAccept, this, new_session, std::placeholders::_1));
}
