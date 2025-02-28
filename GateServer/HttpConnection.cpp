#include "HttpConnection.h"
#include "LogicSystem.h"

HttpConnection::HttpConnection(boost::asio::io_context& ioc):
    m_socket(ioc) {   //[5-33:16]使用move移动构造

}

void HttpConnection::Start() {
    auto self = shared_from_this();
    http::async_read(m_socket, m_buffer, m_request, [self](beast::error_code ec, std::size_t bytes_transferred) {
        try {
            if (ec) {
                std::cout << "http read err is" << ec.what() << std::endl;
                return;
            }
            boost::ignore_unused(bytes_transferred);//忽略掉已发送的字节数
            self->HandleReq();
            self->CheckDeadline();
        }
        catch (std::exception& exp) {
            std::cout << "exception is " << exp.what() << std::endl;
        }
    });
}

unsigned char ToHex(unsigned char x) {
    return x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    else assert(0);
    return y;
}

std::string UrlEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        //判断是否仅有数字和字母构成
        if (isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ') //为空字符
            strTemp += "+";
        else
        {
            //其他字符(比如说汉字)需要提前加%并且高四位和低四位分别转为16进制
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);   //取高4位
            strTemp += ToHex((unsigned char)str[i] & 0x0F); //取低4位
        }
    }
    return strTemp;
}

std::string UrlDecode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        //还原+为空
        if (str[i] == '+') strTemp += ' ';
        //遇到%将后面的两个字符从16进制转为char再拼接
        else if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high * 16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}


void HttpConnection::PreParseGetParam() {
    // 提取 URI  
    auto uri = m_request.target();
    // 查找查询字符串的开始位置（即 '?' 的位置）  
    auto query_pos = uri.find('?');
    if (query_pos == std::string::npos) {
        m_getUrl = uri;
        return;
    }

    m_getUrl = uri.substr(0, query_pos);
    std::string query_string = uri.substr(query_pos + 1);
    std::string key;
    std::string value;
    size_t pos = 0;
    while ((pos = query_string.find('&')) != std::string::npos) {
        auto pair = query_string.substr(0, pos);
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码  
            value = UrlDecode(pair.substr(eq_pos + 1));
            m_getParams[key] = value;
        }
        query_string.erase(0, pos + 1);
    }
    // 处理最后一个参数对（如果没有 & 分隔符）  
    if (!query_string.empty()) {
        size_t eq_pos = query_string.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(query_string.substr(0, eq_pos));    //以免有特殊字符, 进行url解码
            value = UrlDecode(query_string.substr(eq_pos + 1));
            m_getParams[key] = value;
        }
    }
}

void HttpConnection::HandleReq() {
    //设置版本(返回请求中的版本)
    m_response.version(m_request.version());
    m_response.keep_alive(false);
    if (m_request.method() == http::verb::get) {        //处理get请求
        PreParseGetParam();
        bool success = LogicSystem::GetInstance()->HandleGet(m_getUrl, shared_from_this());
        if (!success) {
            m_response.result(http::status::not_found); //返回404
            m_request.set(http::field::content_type, "text/plain");     //设置包体内容的类型
            beast::ostream(m_response.body()) << "url not found\r\n";   //往响应body里面写数据
            WriteResponse();
            return;
        }
        m_response.result(http::status::ok);
        m_response.set(http::field::server, "GateServer");              //告诉对方, 这个报文是哪个服务器响应的
        WriteResponse();
        return;
    }

    if (m_request.method() == http::verb::post) {        //处理get请求
        bool success = LogicSystem::GetInstance()->HandlePost(m_request.target(), shared_from_this());
        if (!success) {
            m_response.result(http::status::not_found); //返回404
            m_request.set(http::field::content_type, "text/plain");     //设置包体内容的类型
            beast::ostream(m_response.body()) << "url not found\r\n";   //往响应body里面写数据
            WriteResponse();
            return;
        }
        m_response.result(http::status::ok);
        m_response.set(http::field::server, "GateServer");              //告诉对方, 这个报文是哪个服务器响应的
        WriteResponse();
        return;
    }
}

void HttpConnection::WriteResponse() {
    auto self = shared_from_this();
    m_response.content_length(m_response.body().size()); //告知包体长度, 用于粘包处理
    http::async_write(m_socket, m_response, [self](beast::error_code ec, std::size_t bytes_transferred) {
        self->m_socket.shutdown(tcp::socket::shutdown_send, ec);//向对端发送信号, 我关闭自己的send端了, 不再发送数据了
        self->deadline_.cancel();
    });
}

void HttpConnection::CheckDeadline() {
    auto self = shared_from_this();
    deadline_.async_wait([self](beast::error_code ec) {
        if (!ec) {  //出错了直接关socket就好, 这里是因为出错了
            self->m_socket.close(ec);   //服务器尽量不要主动去关客户端(TIME_WAIT)
        }
    });
}