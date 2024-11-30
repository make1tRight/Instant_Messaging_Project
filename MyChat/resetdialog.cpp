#include "resetdialog.h"
#include "ui_resetdialog.h"
#include <QJsonObject>
#include "HttpMgr.h"
// #include <QRegularExpression>


ResetDialog::ResetDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ResetDialog)
{
    //输入完就检测, 这里是判断cursor有没有click
    ui->setupUi(this);
    ui->err_tip->clear();   //清除掉一开始的错误提示

    connect(ui->user_edit, &QLineEdit::editingFinished, this, [this]() {
        checkUserValid();
    });
    connect(ui->email_edit, &QLineEdit::editingFinished, this, [this]() {
        checkEmailValid();
    });
    connect(ui->newPass_edit, &QLineEdit::editingFinished, this, [this]() {
        checkPassValid();
    });
    connect(ui->varify_edit, &QLineEdit::editingFinished, this, [this]() {
        checkVarifyValid();
    });

    //连接reset相关信号和注册处理逻辑
    initHandlers();
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reset_mod_finish, this,
            &ResetDialog::slot_reset_mod_finish);
}

ResetDialog::~ResetDialog()
{
    delete ui;
}

void ResetDialog::on_cancel_btn_clicked()
{
    qDebug() << "cancel btn clicked";
    emit switchLogin();
}


void ResetDialog::on_varify_btn_clicked()
{
    qDebug() << "receive varify btn clicked";
    auto email = ui->email_edit->text();
    auto bcheck = checkEmailValid();
    if (!bcheck) {
        return;
    }
    //发送http请求获取验证码
    QJsonObject json_obj;
    json_obj["email"] = email;
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/get_varifycode"),
                                        json_obj, ReqId::ID_GET_VARIFY_CODE, Modules::RESETMOD);
}

bool ResetDialog::checkEmailValid()
{
    auto email = ui->email_edit->text();
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch();
    if (!match) {
        //提示邮箱不正确
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("Email address error"));
        return false;
    }
    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool ResetDialog::checkPassValid()
{
    auto pass = ui->newPass_edit->text();
    if (pass.length() < 6 || pass.length() > 15) {
        //提示长度不正确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("Password length should be 6~15"));
        return false;
    }
    // 创造正则表达式对象, 按照上述密码要求
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");  //必须是[]里面的字符
    bool match = regExp.match(pass).hasMatch();
    if (!match) {
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("Cannot contain illegal characters"));
        return false;
    }
    DelTipErr(TipErr::TIP_PWD_ERR);
    return true;
}

bool ResetDialog::checkVarifyValid()
{
    auto pass = ui->varify_edit->text();
    if (pass.isEmpty()) {
        AddTipErr(TipErr::TIP_VARIFY_ERR, tr("Verification code cannot be empty"));
        return false;
    }
    DelTipErr(TipErr::TIP_VARIFY_ERR);
    return true;
}

void ResetDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void ResetDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if (_tip_errs.empty()) {
        ui->err_tip->clear();
        return;
    }
    showTip(_tip_errs.first(), false);
}

void ResetDialog::showTip(QString str, bool b_ok)
{
    if(b_ok) {
        ui->err_tip->setProperty("state", "normal");
    } else {
        ui->err_tip->setProperty("state", "err");
    }

    ui->err_tip->setText(str);
    repolish(ui->err_tip);
}

void ResetDialog::initHandlers()
{
    //注册获取验证码回包逻辑
    _handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](QJsonObject jsonObj) {
        int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            showTip(tr("Parameter error"), false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("Verification code has been sent to your email!"), true);
        qDebug() << "email is " << email;
    });

    //注册成功用户回报逻辑
    _handlers.insert(ReqId::ID_RESET_PWD, [this](QJsonObject jsonObj) {
        int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            showTip(tr("Parameter error"), false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("User registration successful"), true);
        qDebug() << "user uuid is " << jsonObj["uid"].toString();
        qDebug() << "email is " << email;
    });
}


void ResetDialog::on_confirm_btn_clicked()
{
    bool valid = checkUserValid();
    if (!valid) {
        return;
    }
    valid = checkEmailValid();
    if (!valid) {
        return;
    }
    valid = checkPassValid();
    if (!valid) {
        return;
    }
    valid = checkVarifyValid();
    if (!valid) {
        return;
    }

    //发送http重置用户请求
    QJsonObject json_obj;
    json_obj["user"] = ui->user_edit->text();
    json_obj["email"] = ui->email_edit->text();
    json_obj["passwd"] = xorString(ui->newPass_edit->text());
    json_obj["varifycode"] = ui->varify_edit->text();
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/reset_pwd"),
                                        json_obj, ReqId::ID_RESET_PWD, Modules::RESETMOD);

}

void ResetDialog::slot_reset_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS) {
        showTip(tr("Network request error"), false);
        return;
    }
    //解析JSON字符串, res需转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    //json解析错误
    if (jsonDoc.isNull()) {
        showTip(tr("json parsing failed"), false);
        return;
    }
    if (!jsonDoc.isObject()) {
        showTip(tr("json parsing failed"), false);
        return;
    }

    //调用对应逻辑, 也就是根据ReqId回调
    _handlers[id](jsonDoc.object());
    return;
}

bool ResetDialog::checkUserValid()
{
    if (ui->user_edit->text() == "") {
        AddTipErr(TipErr::TIP_USER_ERR, tr("username cannot be empty"));
        return false;
    }
    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}


