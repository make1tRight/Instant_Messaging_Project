#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOContextPool.h"

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port):
    m_ioc(ioc), m_acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {

}

void CServer::Start() {
    //[5-12:00]��ֹ�ص��������õ�ʱ���౻������, ����ʹ������ָ��, �����ü�����ά����������������
    auto self = shared_from_this();
    auto& io_context = AsioIOContextPool::GetInstance()->GetIOContext();
    std::shared_ptr<HttpConnection> new_conn = std::make_shared<HttpConnection>(io_context);
    //�첽����
    m_acceptor.async_accept(new_conn->GetSocket(), [self, new_conn](beast::error_code ec) {    //���ﲶ����Ϊ���������ü���
        try {
            //���������ǰ����, ����������������
            if (ec) {
                self->Start(); //������������
                return;
            }

            //����������, ���Ҵ���HttpConnection������������
            //std::make_shared<HttpConnection>(std::move(self->m_socket))->Start();      //[5-17:56]������ֵ���ú��ƶ�����
            new_conn->Start(); //�̳߳��滻���߳� 
            //HttpConnection(std::move(m_socket));

            //��������
            self->Start();
        }
        catch (std::exception& exp) {
            std::cout << "exception is " << exp.what() << std::endl;
            self->Start();
        };
    });
}