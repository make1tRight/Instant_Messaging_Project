#include "logindialog.h"
#include "ui_logindialog.h"
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include "HttpMgr.h"
#include "tcpmgr.h"

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    connect(ui->reg_btn, &QPushButton::clicked, this, &LoginDialog::switchRegister);
    ui->forget_label->SetState("normal", "hover", "", "selected", "selected_hover", "");
    connect(ui->forget_label, &ClickedLabel::clicked, this, &LoginDialog::slot_forget_pwd);
    initHead();
    initHttpHandlers();
    //连接登录回包信号
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_login_mod_finish, this,
            &LoginDialog::slot_login_mod_finish);
    //连接tcp连接请求信号和槽函数(什么信号的时候, 希望做什么事(槽函数))
    connect(this, &LoginDialog::sig_connect_tcp, TcpMgr::GetInstance().get(), &TcpMgr::slot_tcp_connect);
    //连接tcp管理者发出的登录成功信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_con_success, this, &LoginDialog::slot_tcp_con_finish);
    //连接tcp管理者发出的登录失败信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_login_failed, this, &LoginDialog::slot_login_failed);
}

LoginDialog::~LoginDialog()
{
    qDebug() << "destruct LoginDialog";
    delete ui;
}

void LoginDialog::initHttpHandlers()
{
    _handlers.insert(ReqId::ID_LOGIN_USER, [this](QJsonObject jsonObj) {
        int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            showTip(tr("Parameter error"), false);
            enableBtn(true);
            return;
        }
        auto email = jsonObj["email"].toString();
        //发送信号通知tcpMgr发送长连接
        ServerInfo si;
        si.Uid = jsonObj["uid"].toInt();
        si.Host = jsonObj["host"].toString();
        si.Port = jsonObj["port"].toString();
        si.Token = jsonObj["token"].toString();

        _uid = si.Uid;
        _token = si.Token;
        qDebug() << "email is " << email << " uid is " << si.Uid << " host is "
                 << si.Host << "Port is " << si.Port << " Token is " << si.Token;
        emit sig_connect_tcp(si);
    });
}

void LoginDialog::initHead()
{
    //加载图片
    QPixmap originalPixmap(":/res/head_1.png");
    //设置图片自动缩放
    qDebug() << originalPixmap.size() << ui->head_label->size();
    originalPixmap = originalPixmap.scaled(ui->head_label->size(),
                                           Qt::KeepAspectRatio, Qt::SmoothTransformation);  //等比例缩放, 平滑转移
    //创建一个和原始图片相同大小的QPixmap, 用于绘制圆角图片
    QPixmap roundedPixmap(originalPixmap.size());
    roundedPixmap.fill(Qt::transparent);//用透明颜色填充

    QPainter painter(&roundedPixmap);
    painter.setRenderHint(QPainter::Antialiasing);          //设置抗锯齿
    painter.setRenderHint(QPainter::SmoothPixmapTransform); //使得圆角更平滑

    //使用QPainterPath设置圆角
    QPainterPath path;
    path.addRoundedRect(0, 0, originalPixmap.width(), originalPixmap.height(), 10, 10); //10是圆角的弧度
    painter.setClipPath(path);
    //将原始图片绘制到roundedPixmap上
    painter.drawPixmap(0, 0, originalPixmap);
    //设置绘制好的圆角图片到QLabel
    ui->head_label->setPixmap(roundedPixmap);

}

bool LoginDialog::checkUserValid()
{
    auto email = ui->email_edit->text();
    if (email.isEmpty()) {
        qDebug() << "email empty ";
        AddTipErr(TipErr::TIP_USER_ERR, tr("email cannot be empty"));
        return false;
    }
    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}

bool LoginDialog::checkPwdValid()
{
    auto pwd = ui->password_edit->text();
    if (pwd.length() < 6 || pwd.length() > 15) {
        qDebug() << "Password length invalid";
        //提示长度不正确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("Password length should be 6~15"));
        return false;
    }
    // 创造正则表达式对象, 按照上述密码要求
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");  //必须是[]里面的字符
    bool match = regExp.match(pwd).hasMatch();
    if (!match) {
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("Cannot contain illegal characters"));
        return false;
    }
    DelTipErr(TipErr::TIP_PWD_ERR);
    return true;
}

void LoginDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void LoginDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if (_tip_errs.empty()) {    //没有错误就直接返回
        ui->err_tip->clear();
        return;
    }
    showTip(_tip_errs.first(), false);   //还有错误就返回第一条
}

void LoginDialog::showTip(QString str, bool b_ok)
{
    if (b_ok) {
        ui->err_tip->setProperty("state", "normal");
    } else {
        ui->err_tip->setProperty("state", "err");
    }
    ui->err_tip->setText(str);
    repolish(ui->err_tip);
}

bool LoginDialog::enableBtn(bool enabled)
{
    ui->login_btn->setEnabled(enabled);
    ui->reg_btn->setEnabled(enabled);
    return true;
}

void LoginDialog::slot_forget_pwd()
{
    qDebug() << "slot forget pwd";
    emit switchReset();
}

void LoginDialog::on_login_btn_clicked()
{
    qDebug() << "login btn clicked";
    if (checkUserValid() == false) {
        return;
    }
    if (checkPwdValid() == false) {
        return;
    }
    enableBtn(false);
    auto email = ui->email_edit->text();
    auto pwd = ui->password_edit->text();
    //发送http请求登录
    QJsonObject json_obj;
    json_obj["email"] = email;
    json_obj["passwd"] = xorString(pwd);
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/user_login"),
                                        json_obj, ReqId::ID_LOGIN_USER, Modules::LOGINMOD);
}

void LoginDialog::slot_login_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS) {
        showTip(tr("Network request error"), false);
        return;
    }
    //解析JSON字符串, res需要转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    //json解析错误
    if (jsonDoc.isNull()) {
        showTip(tr("json parsing failed"), false);
        return;
    }
    //json解析错误
    if (!jsonDoc.isObject()) {
        showTip(tr("json parsing failed"), false);
        return;
    }
    //调用ReqId对应回调
    _handlers[id](jsonDoc.object());
    return;
}

void LoginDialog::slot_tcp_con_finish(bool b_success)
{
    if (b_success) {
        showTip(tr("ChatServer connection successful, logging in..."), true);
        QJsonObject jsonObj;
        jsonObj["uid"] = _uid;
        jsonObj["token"] = _token;
        QJsonDocument doc(jsonObj);
        QString jsonString = doc.toJson(QJsonDocument::Indented);
        //发送tcp请求给chatserver
        emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_CHAT_LOGIN, jsonString);
    } else {
        showTip(tr("Network request error"), false);
        enableBtn(true);
    }
}

void LoginDialog::slot_login_failed(int err)
{
    QString result = QString("Login failed, err is %1").arg(err);
    showTip(result, false);
    enableBtn(true);
}
