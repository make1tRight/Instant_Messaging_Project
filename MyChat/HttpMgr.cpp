#include "HttpMgr.h"

HttpMgr::~HttpMgr() {

}

HttpMgr::HttpMgr() {
    connect(this, &HttpMgr::sig_http_finish, this, &HttpMgr::slot_http_finish); //建立信号和槽之间的关系
}

//qt发送http请求(是一个异步的过程)
void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod) { //利用qt去发送http报文
    QByteArray data = QJsonDocument(json).toJson(); //转成字节数组
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));
    auto self = shared_from_this();     //继承了enable_shared_from_this<T>这个模板类
    QNetworkReply* reply = m_httpManager.post(request, data);       //qt的http管理类中post是用于响应客户端请求的一个方法
    //防止捕获的对象被delete掉, 所以这里获取的是self, self是一个智能指针(用shared_from_this生成的)
    QObject::connect(reply, &QNetworkReply::finished, [self, reply, req_id, mod]() {    //[3-41:10]这里为什么传的是self而不是this
        // 处理错误情况
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << reply->errorString();
            //发送信号通知其他模块 -> 已完成
            emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
            reply->deleteLater();   //不用以后再回收
            return;
        }
            
        //没有错误的情况
        QString res = reply->readAll();
        //发送信号通知完成
        emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS, mod);
        reply->deleteLater();
        return;
    });
}

void HttpMgr::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod) {
    if (mod == Modules::REGISTERMOD) {
        //发送信号通知指定模块 -> http响应结束了
        emit sig_reg_mod_finish(id, res, err);
    }
    if (mod == Modules::RESETMOD) {
        //发送信号通知指定模块 -> http响应结束了
        emit sig_reset_mod_finish(id, res, err);
    }
    if (mod == Modules::LOGINMOD) {
        emit sig_login_mod_finish(id, res, err);
    }
}
