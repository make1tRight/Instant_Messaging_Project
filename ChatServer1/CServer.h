#ifndef CSERVER_H
#define CSERVER_H
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <memory>
#include <string>
#include <map>

namespace beast = boost::beast;
namespace net = boost::asio;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

class CSession;

class CServer : public std::enable_shared_from_this<CServer> {
public:
    CServer(boost::asio::io_context& ioc, short port);
    ~CServer();
    void ClearSession(std::string session_id);
    // void Start();
private:
    void StartAccept();
    void HandleAccept(std::shared_ptr<CSession> session, const boost::system::error_code& error);
    short _port;
    tcp::acceptor _acceptor;
    net::io_context& _ioc;
    // tcp::socket _socket;
    std::mutex _mutex;
    std::map<std::string, std::shared_ptr<CSession>> _sessions;
};
#endif // CSERVER_H