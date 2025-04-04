#include "CSession.h"
#include "CServer.h"
#include <string.h>
#include <boost/beast.hpp>
#include "LogicSystem.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

CSession::CSession(net::io_context& ioc, CServer* server) 
     : _socket(ioc), _server(server), _b_close(false), _user_uid(0) {
    boost::uuids::uuid auuid = boost::uuids::random_generator()();
    _session_id = boost::uuids::to_string(auuid);
    _recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
}

CSession::~CSession() {
    std::cout << "~CSession() destructed." << std::endl;
}

std::shared_ptr<CSession> CSession::SharedSelf() {
    return shared_from_this();
}

tcp::socket& CSession::GetSocket() {
    return _socket;
}

std::string CSession::GetSessionId() {
    return _session_id;
}

void CSession::SetUserId(int uid) {
    _user_uid = uid;
}

int CSession::GetUserId() {
    return _user_uid;
}

void CSession::Start() {
    AsyncReadHead(HEAD_TOTAL_LEN);
}

void CSession::AsyncReadHead(int total_len) {
    auto self = SharedSelf();
    asyncReadFull(HEAD_TOTAL_LEN,
         [self, this, total_len]
            (const boost::system::error_code& ec, std::size_t bytes_transferred) {
        try {
            if (ec) {
                std::cout << "Failed to AsyncReadHead, error: "
                     << ec.message() << std::endl;
                Close();
                _server->ClearSession(_session_id);
                return;
            }
            if (bytes_transferred < total_len) {
                std::cout 
                    << "Head len do not match, read[" 
                    << bytes_transferred << "], total["
                    << HEAD_TOTAL_LEN << "]." << std::endl;
                Close();
                _server->ClearSession(_session_id);
                return;
            }
            _recv_head_node->Clear();
            memcpy(_recv_head_node->_data, _data, bytes_transferred);
            short msg_id;
            memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LENGTH);
            msg_id = net::detail::socket_ops::network_to_host_short(msg_id);
            if (msg_id > MAX_LENGTH) {
                std::cout << "Invalid msg_id: " << msg_id << std::endl;
                Close();
                _server->ClearSession(_session_id);
                return;
            }
            std::cout << "msg_id: " << msg_id << std::endl;

            short msg_len;
            memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LENGTH, HEAD_DATA_LENGTH);
            msg_len = net::detail::socket_ops::network_to_host_short(msg_len);
            if (msg_len > MAX_LENGTH) {
                std::cout << "Invalid msg_len: " << msg_len << std::endl;
                Close();
                _server->ClearSession(_session_id);
                return;
            }
            std::cout << "msg_len: " << msg_len << std::endl;
            _recv_msg_node = std::make_shared<RecvNode>(msg_len, msg_id);
            AsyncReadBody(msg_len);
        } 
        catch (std::exception& e) {
            std::cout << "Exception code: " << e.what() << std::endl;
        }
    });
}

void CSession::AsyncReadBody(int total_len) {
    auto self = SharedSelf();
    asyncReadFull(total_len, [self, this, total_len]
        (const boost::system::error_code& ec, std::size_t bytes_transferred) {
        try {
            if (ec) {
                std::cout << "Failed to AsyncReadBody, error: "
                    << ec.message() << std::endl;
                Close();
                _server->ClearSession(_session_id);
                return;
            }
            if (bytes_transferred < total_len) {
                std::cout 
                    << "Message length do not match, read[" 
                    << bytes_transferred << "], total["
                    << total_len << "]." << std::endl;
                Close();
                _server->ClearSession(_session_id);
                return;
            }
            memcpy(_recv_msg_node->_data, _data, bytes_transferred);
            _recv_msg_node->_cur_len += bytes_transferred;
            _recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
            std::cout << "receive data: " << _recv_msg_node->_data << std::endl;
            LogicSystem::GetInstance()->PostMsgToQue(
                std::make_shared<LogicNode>(SharedSelf(), _recv_msg_node));
            AsyncReadHead(HEAD_TOTAL_LEN);
        }
        catch(std::exception& e) {
            std::cout << "Exception code: " << e.what() << std::endl;
        }
    });
}

void CSession::Send(std::string msg, short msg_id) {

}

void CSession::Close() {
    _b_close = true;
    _socket.close();
}


void CSession::asyncReadFull(std::size_t max_len,
    std::function<void(const boost::system::error_code& ec, std::size_t)> handler) {
    ::memset(_data, 0, MAX_LENGTH);
    asyncReadLen(0, max_len, handler);
}

void CSession::asyncReadLen(std::size_t read_len, std::size_t total_len,
    std::function<void(const boost::system::error_code& ec, std::size_t bytes_transferred)> handler) {
    auto self = SharedSelf();

    _socket.async_read_some(boost::asio::buffer(_data + read_len, total_len - read_len),
         [read_len, total_len, handler, self]
            (const boost::system::error_code& ec, std::size_t bytes_transferred) {
        if (ec) {
            handler(ec, read_len + bytes_transferred);
            return;
        }
        if (read_len + bytes_transferred >= total_len) {
            handler(ec, read_len + bytes_transferred);
            return;
        }
        self->asyncReadLen(read_len + bytes_transferred, total_len, handler);
    });
}

LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode)
     : _session(session), _recvnode(recvnode) {

}