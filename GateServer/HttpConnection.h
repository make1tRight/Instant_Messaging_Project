#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <memory>
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>

namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;

class LogicSystem;
class HttpConnection : public std::enable_shared_from_this<HttpConnection> {
    friend class LogicSystem;
public:
    HttpConnection(tcp::socket socket);
    void Start();
private:
    void CheckDeadline();
    void HandleRequest();
    void WriteResponse();
    void PreParseGetParam();
    tcp::socket _socket;
    beast::flat_buffer _buffer{8192};
    http::request<http::dynamic_body> _request;
    http::response<http::dynamic_body> _response;
    asio::steady_timer _deadline{
        _socket.get_executor(), std::chrono::seconds(60)
    };
    std::string _url_string;
    std::map<std::string, std::string> _get_params;
};
#endif // HTTPCONNECTION_H