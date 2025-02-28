#pragma once
#include "const.h"

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
public:
    friend class LogicSystem;
    HttpConnection(boost::asio::io_context& ioc);
    void Start();
    tcp::socket& GetSocket() {
        return m_socket;
    }

private:
    void CheckDeadline();                                   //检测超时
    void WriteResponse();                                   //应答(握手挥手过程)
    void HandleReq();                                       //处理请求头(解析包体)
    void PreParseGetParam();                                //解析post报文里面的参数
    tcp::socket m_socket;
    beast::flat_buffer m_buffer{ 8192 };                    //8k字节缓冲区用于接收http发送过来的数据
    http::request<http::dynamic_body> m_request;            //接收对方的请求(字符串/图片(二进制)/表单), dynamic_body是动态的类型
    http::response<http::dynamic_body> m_response;          //响应对方的请求
    net::steady_timer deadline_{                            //设置超时时间是60s
        m_socket.get_executor(), std::chrono::seconds(60)   //这里是直接初始化
    };

    std::string m_getUrl;
    std::unordered_map<std::string, std::string> m_getParams;
};