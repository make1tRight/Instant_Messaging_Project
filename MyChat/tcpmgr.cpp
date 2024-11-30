#include "tcpmgr.h"
#include <QAbstractSocket>
#include <QJsonDocument>
#include "usermgr.h"

TcpMgr::~TcpMgr()
{

}

TcpMgr::TcpMgr()
    : _host(""), _port(0), _b_recv_pending(false), _message_id(0), _message_len(0)
{
    QObject::connect(&_socket, &QTcpSocket::connected, [&]() {
        qDebug() << "Connected to server!";
        //建立连接后发送消息
        emit sig_con_success(true);
    });
    QObject::connect(&_socket, &QTcpSocket::readyRead, [&]() {      //数据可读的时候触发readyRead信号
        // 当有数据可读的时候, 读取所有数据
        // 并加载到缓冲区
        _buffer.append(_socket.readAll());
        QDataStream stream(&_buffer, QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_5_0);
        forever {
            //解析头部[id-2][len-2][data...], 也就是id和len
            if (!_b_recv_pending) {
                //检查缓冲区中的数据是否足够解析出一个消息头(id+len)
                if (_buffer.size() <static_cast<int>(sizeof(quint16) * 2)) {
                    return;
                }
                //预读取消息ID和消息长度, [id-2][len-2][data...]
                stream >> _message_id >> _message_len;
                //将buffer中的前四个字节移除
                _buffer = _buffer.mid(sizeof(quint16) * 2);
                //输出读取的数据
                qDebug() << "Message ID:" << _message_id << ", Length" << _message_len;
            }
            //buffer剩余长度是否满足消息体长度, 不满足则退出继续接收剩余字符
            if (_buffer.size() < _message_len) {
                _b_recv_pending = true;
                return;
            }
            //确定收全了
            _b_recv_pending = false;
            //再读取消息体
            QByteArray messageBody = _buffer.mid(0, _message_len);
            qDebug() << "receive body msg is " << messageBody;
            //读多了要截断
            _buffer = _buffer.mid(_message_len);
            //调用handlers去处理消息体
            handleMsg(ReqId(_message_id), _message_id, messageBody);
        }
    });
    //处理错误的逻辑
    QObject::connect(&_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
                                                                           [&](QAbstractSocket::SocketError socketError) {
        Q_UNUSED(socketError)
        qDebug() << "Error: " << _socket.errorString();
        switch (socketError) {
        case QTcpSocket::ConnectionRefusedError:
            qDebug() << "Connection Refused!";
            emit sig_con_success(false);
            break;
        case QTcpSocket::RemoteHostClosedError:
            qDebug() << "Remote Host Closed Connection!";
            break;
        case QTcpSocket::HostNotFoundError:
            qDebug() << "Host Not Found!";
            emit sig_con_success(false);
            break;
        case QTcpSocket::SocketTimeoutError:
            qDebug() << "Connection Timeout!";
            emit sig_con_success(false);
            break;
        case QTcpSocket::NetworkError:
            qDebug() << "Network Error!";
            break;
        default:
            qDebug() << "Other Error!";
            break;
        }
    });
    // 处理错误（适用于Qt 5.15之前的版本）
    // QObject::connect(&_socket, static_cast<void (QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error),
    //                  [&](QTcpSocket::SocketError socketError) {
    //                      qDebug() << "Error:" << _socket.errorString();
    //                  });

    //处理连接断开
    QObject::connect(&_socket, &QTcpSocket::disconnected, [&]() {
        qDebug() << "Disconnected from server.";
    });
    //连接发送信号用来发送数据
    QObject::connect(this, &TcpMgr::sig_send_data, this, &TcpMgr::slot_send_data);  //发送完信号调用槽函数
    //注册消息
    initHandlers();
}

void TcpMgr::initHandlers()
{
    //auto self = shared_from_this();[18-53:04]为什么不能用shared_from_this, 类没构造完
    _handlers.insert(ReqId::ID_CHAT_LOGIN_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        //将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        //检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {   //一定要有错误标记, 因为SUCCESS是0
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Login Failed, err is Json Parse Err" << err;
            emit sig_login_failed(err);
            return;
        }
        int err = jsonObj["error"].toInt(); //取出错误标记
        if (err != ErrorCodes::SUCCESS) {   //如果不是SUCCESS就发送信号告知其他界面
            qDebug() << "Login Failed, err is " << err;
            emit sig_login_failed(err);
            return;
        }
        //没有问题就可以切换到聊天界面了
        //UserMgr是用户数据管理类
        UserMgr::GetInstance()->SetUid(jsonObj["uid"].toInt());
        UserMgr::GetInstance()->SetName(jsonObj["name"].toString());
        UserMgr::GetInstance()->SetToken(jsonObj["token"].toString());
        emit sig_switch_chatdlg();
    });
}

//处理消息的逻辑
void TcpMgr::handleMsg(ReqId id, int len, QByteArray data)
{
    auto find_iter = _handlers.find(id);
    //如果没找到就报错
    if (find_iter == _handlers.end()) {
        qDebug() << "not found id [" << id << "] to handle";
        return;
    }
    //QMap可以调用value()
    //调用回调函数
    find_iter.value()(id, len, data);
}

void TcpMgr::slot_tcp_connect(ServerInfo si)
{
    qDebug() << "receive tcp connect signal";
    //尝试连接到服务器
    qDebug() << "Connecting to server...";
    _host = si.Host;
    _port = static_cast<uint16_t>(si.Port.toUInt());
    _socket.connectToHost(si.Host, _port);//[18-44:32]通过构造函数里面的sig_con_success来异步告知成功或者是失败
}

void TcpMgr::slot_send_data(ReqId reqId, QString data)
{
    // uint16_t id = reqId;
    // //将字符串转换为UTF-8编码的字节数组
    // QByteArray dataBytes = data.toUtf8();
    // //计算长度(使用网络字节序转换)
    // quint16 len = static_cast<quint16>(data.size());
    // //创建一个QByteArray用户存储要发送的所有数据
    // QByteArray block;
    // QDataStream out(&block, QIODevice::WriteOnly);
    // //设置数据流使用网络(大端)字节序
    // out.setByteOrder(QDataStream::BigEndian);
    // //写入ID和长度
    // out << id << len;
    // //添加字符串数据
    // block.append(data);
    // //发送数据
    // _socket.write(block);
    // qDebug() << "tcp mgr send byte data is " << block;
    uint16_t id = reqId;
    QByteArray dataBytes = data.toUtf8();
    // 计算长度（使用网络字节序转换）
    quint16 len = static_cast<quint16>(dataBytes.length());
    // 创建一个QByteArray用于存储要发送的所有数据
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    // 设置数据流使用网络字节序
    out.setByteOrder(QDataStream::BigEndian);
    // 写入ID和长度
    out << id << len;
    // 添加字符串数据
    block.append(dataBytes);
    // 发送数据
    _socket.write(block);
    qDebug() << "tcp mgr send byte data is " << block ;
}
