#ifndef HTTPMGR_H
#define HTTPMGR_H
#include "singleton.h"
#include <QString>
#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>

//CRTP[3-18:57]奇异递归, 类能够继承以自己为基础的模板类
class HttpMgr : public QObject, 
    public Singleton<HttpMgr>, 
    public std::enable_shared_from_this<HttpMgr> {
    Q_OBJECT
public:
    ~HttpMgr(); //[3-21:08]这里设置成公有析构的目的是为了让智能指针能够调用
    void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod);
private:
    friend class Singleton<HttpMgr>; //这里的友元声明是为了让单例类能够访问HttpMgr的构造函数
    HttpMgr();
    QNetworkAccessManager m_httpManager;
private slots://qt的槽函数
    void slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);
signals:
    void sig_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);
    void sig_reg_mod_finish(ReqId id, QString res, ErrorCodes err);
    void sig_reset_mod_finish(ReqId id, QString res, ErrorCodes err);
    void sig_login_mod_finish(ReqId id, QString res, ErrorCodes err);
};

#endif // HTTPMGR_H
