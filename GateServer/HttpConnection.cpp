#include "HttpConnection.h"
#include "LogicSystem.h"
#include <iostream>

// 10进制的char转16进制的ASCII码
unsigned char ToHex(unsigned char x) {
    return x > 9 ? x + 55 : x + 48;
}

// 16进制的char字符转10进制
unsigned char FromHex(unsigned char x) {
    unsigned char y;
    if (x >= 'A' && x <= 'Z') {
        y = x - 'A' + 10;
    } else if (x >= 'a' && x <= 'z') {
        y = x - 'a' + 10;
    } else if (x >= '0' && x <= '9') {
        y = x - '0';
    } else {
        assert(0);
    }
    return y;
}

std::string UrlEncode(const std::string& str) {
    std::string return_str = "";
    for (unsigned char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            return_str += c;
        } else if (c == ' ') {
            return_str += '+';
        } else {
            return_str += '%';
            return_str += ToHex(c >> 4);
            return_str += ToHex(c & 0x0f);
        }
    }
    return return_str;
}

std::string UrlDecode(const std::string& str) {
    std::string return_str = "";
    std::size_t size = str.size();
    for (int i = 0; i < size; ++i) {
        if (str[i] == '+') {
            return_str += ' ';
        } else if (str[i] == '%') {
            assert(i + 2 < size);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            return_str += high*16 + low; 
        } else {
            return_str += str[i];
        }
    }
    return return_str;
}

HttpConnection::HttpConnection(asio::io_context& ioc)
     : _socket(ioc) {

}

void HttpConnection::Start() {
    auto self = shared_from_this();
    http::async_read(_socket, _buffer, _request, [self](
        beast::error_code ec, std::size_t bytes_transferred) {
        try {
            if (ec) {
                std::cout << "http read error, code: " << ec.message() << std::endl;
                return;
            }
            boost::ignore_unused(bytes_transferred);
            self->HandleRequest();
            self->CheckDeadline();
        }
        catch (std::exception& e) {
            std::cout << "HttpConnection::Start() Exception: " << ec.message() << std::endl;
        }
    });
}

tcp::socket& HttpConnection::GetSocket() {
    return _socket;
}

void HttpConnection::CheckDeadline() {
    auto self = shared_from_this();
    _deadline.async_wait([self](beast::error_code ec) {
        if (!ec) {
            self->_socket.close(ec);
        }
    });
}

void HttpConnection::HandleRequest() {
    _response.version(_request.version());
    _response.keep_alive(false);

    if (_request.method() == http::verb::get) {
        PreParseGetParam();
        bool success = LogicSystem::GetInstance()->HandleGet(
            _url_string, shared_from_this());
        if (!success) {
            _response.result(http::status::not_found);
            _response.set(http::field::content_type, "text/plain");
            beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return;
        }

        _response.result(http::status::ok);
        _response.set(http::field::server, "GateServer");
        WriteResponse();
        return;
    }
    if (_request.method() == http::verb::post) {
        bool success = LogicSystem::GetInstance()->HandlePost(
            _request.target().data(), shared_from_this());
        if (!success) {
            _response.result(http::status::not_found);
            _response.set(http::field::content_type, "text/plain");
            beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return;
        }

        _response.result(http::status::ok);
        _response.set(http::field::server, "GateServer");
        WriteResponse();
        return;
    }
}

void HttpConnection::WriteResponse() {
    auto self = shared_from_this();
    _response.content_length(_response.body().size());
    http::async_write(_socket, _response, [self](
        beast::error_code ec, std::size_t) {
        self->_socket.shutdown(tcp::socket::shutdown_send, ec);
        self->_deadline.cancel();// 取消超时检测定时器
    });
}

void HttpConnection::PreParseGetParam() {
    auto url = _request.target().to_string();
    auto qchar = url.find('?');
    if (qchar == std::string::npos) {
        _url_string = url;
        return;
    }

    _url_string = url.substr(0, qchar);
    std::string query_string = url.substr(qchar + 1);
    std::size_t pos = 0;
    std::string key;
    std::string value;
    while ((pos = query_string.find('&')) != std::string::npos) {
        auto uri = query_string.substr(0, pos);
        auto eq = uri.find('=');
        if (eq == std::string::npos) {
            std::cout << "parameters lose." << std::endl;
            return;
        }
        key = UrlDecode(uri.substr(0, eq));
        value = UrlDecode(uri.substr(eq + 1));
        _get_params[key] = value;
        query_string.erase(0, pos + 1);
    }
    if (!query_string.empty()) {
        auto eq = query_string.find('=');
        if (eq == std::string::npos) {
            std::cout << "parameters lose." << std::endl;
            return;
        }
        key = UrlDecode(query_string.substr(0, eq));
        value = UrlDecode(query_string.substr(eq + 1));
        _get_params[key] = value;
    }
}