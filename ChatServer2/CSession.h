#ifndef CSESSION_H
#define CSESSION_H
#include "const.h"
#include "MsgNode.h"
#include <string>
#include <memory>
#include <boost/asio.hpp>

namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
class CServer;
class LogicSystem;

class CSession : public std::enable_shared_from_this<CSession> {
public:
    CSession(net::io_context& ioc, CServer* server);
    ~CSession();
    std::shared_ptr<CSession> SharedSelf();
    tcp::socket& GetSocket();
    std::string GetSessionId();
    void SetUserId(int uid);
    int GetUserId();

    void Start();
    void AsyncReadHead(int total_len);
    void AsyncReadBody(int total_len);
    void Send(std::string msg, short msg_id);
    void Send(char* msg, short max_len, short msg_id);
    void Close();
private:
    void asyncReadFull(std::size_t max_len,
         std::function<void(const boost::system::error_code&, std::size_t)> handler);
    void asyncReadLen(std::size_t read_len, std::size_t total_len,
         std::function<void(const boost::system::error_code&, std::size_t)> handler);
    // net::io_context& _ioc;
    char _data[MAX_LENGTH];
    tcp::socket _socket;
    CServer* _server;
    bool _b_close;
    std::string _session_id;
    std::shared_ptr<RecvNode> _recv_msg_node;
    std::shared_ptr<MsgNode> _recv_head_node;
    int _user_uid;
};

class LogicNode {
    friend class LogicSystem;
public:
    LogicNode(std::shared_ptr<CSession>, std::shared_ptr<RecvNode>);
private:
    std::shared_ptr<CSession> _session;
    std::shared_ptr<RecvNode> _recvnode;
};
#endif // CSESSION_H