#ifndef TCPMGR_H
#define TCPMGR_H
#include <QTcpSocket>
#include "singleton.h"
#include "global.h"
#include <functional>
#include <QObject>
#include "userdata.h"

class TcpMgr : public QObject                       //为了能够发送、接收信号
    , public Singleton<TcpMgr>                      //单例基类的模板类
    , public std::enable_shared_from_this<TcpMgr>   //允许生成智能指针（QT可以自己管理）
{
    Q_OBJECT
public:
    ~TcpMgr();
private:
    friend class Singleton<TcpMgr>;
    TcpMgr();
    void initHandlers();
    void handleMsg(ReqId id, int len, QByteArray data);
    QTcpSocket _socket;
    QString _host;
    uint16_t _port;
    //处理粘包问题, 因为tcp不负责这个包是否完整, 所以要在http层检查
    QByteArray _buffer;
    bool _b_recv_pending;//true说明没有收全
    quint16 _message_id;
    quint16 _message_len;
    QMap<ReqId, std::function<void(ReqId id, int len, QByteArray data)>> _handlers;
//告知其他界面
public slots:
    void slot_tcp_connect(ServerInfo);
    void slot_send_data(ReqId reqId, QString data);
signals:
    void sig_con_success(bool b_success);
    void sig_send_data(ReqId reqId, QString data);
    void sig_switch_chatdlg();
    void sig_login_failed(int);
    void sig_user_search(std::shared_ptr<SearchInfo>);
    void sig_auth_rsp(std::shared_ptr<AuthRsp>);                //别人加自己好友
    void sig_add_auth_friend(std::shared_ptr<AuthInfo>);        //收到对方同意的消息
    void sig_friend_apply(std::shared_ptr<AddFriendApply>);     //申请加别人好友
};

#endif // TCPMGR_H
