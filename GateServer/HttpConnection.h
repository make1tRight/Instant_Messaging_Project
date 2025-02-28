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
    void CheckDeadline();                                   //��ⳬʱ
    void WriteResponse();                                   //Ӧ��(���ֻ��ֹ���)
    void HandleReq();                                       //��������ͷ(��������)
    void PreParseGetParam();                                //����post��������Ĳ���
    tcp::socket m_socket;
    beast::flat_buffer m_buffer{ 8192 };                    //8k�ֽڻ��������ڽ���http���͹���������
    http::request<http::dynamic_body> m_request;            //���նԷ�������(�ַ���/ͼƬ(������)/��), dynamic_body�Ƕ�̬������
    http::response<http::dynamic_body> m_response;          //��Ӧ�Է�������
    net::steady_timer deadline_{                            //���ó�ʱʱ����60s
        m_socket.get_executor(), std::chrono::seconds(60)   //������ֱ�ӳ�ʼ��
    };

    std::string m_getUrl;
    std::unordered_map<std::string, std::string> m_getParams;
};