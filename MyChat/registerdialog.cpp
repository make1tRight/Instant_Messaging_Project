#include "registerdialog.h"
#include "ui_registerdialog.h"
#include "global.h"
#include "HttpMgr.h"
// #include "clickedlabel.h"

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RegisterDialog)
    , _countdown(5)
{
    ui->setupUi(this);
    ui->password_edit->setEchoMode(QLineEdit::Password);
    ui->confirm_edit->setEchoMode(QLineEdit::Password);
    ui->err_tip->setProperty("state", "normal");
    repolish(ui->err_tip);
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reg_mod_finish, 
        this, &RegisterDialog::slot_reg_mod_finish);
    initHttpHandlers();
    ui->err_tip->clear();   //清除掉一开始的错误提示

    // 验证各个输入项是否合理
    connect(ui->user_edit, &QLineEdit::editingFinished, this, [this]() {
        checkUserValid();
    });
    connect(ui->email_edit, &QLineEdit::editingFinished, this, [this]() {
        checkEmailValid();
    });
    connect(ui->password_edit, &QLineEdit::editingFinished, this, [this]() {
        checkPassValid();
    });
    connect(ui->confirm_edit, &QLineEdit::editingFinished, this, [this]() {
        checkConfirmValid();
    });
    connect(ui->varify_edit, &QLineEdit::editingFinished, this, [this]() {
        checkVarifyValid();
    });
    ui->password_visible->setCursor(Qt::PointingHandCursor);
    ui->confirm_visible->setCursor(Qt::PointingHandCursor);

    ui->password_visible->SetState("unvisible", "unvisible_hover", "",
                                   "visible", "visible_hover", "");
    ui->confirm_visible->SetState("unvisible", "unvisible_hover", "",
                                   "visible", "visible_hover", "");

    // 建立按键与密码显示按钮的关系
    connect(ui->password_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->password_visible->GetCurState();
        if (state == ClickLbState::Normal) {
            ui->password_edit->setEchoMode(QLineEdit::Password);
        } else {
            ui->password_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";
    });
    connect(ui->confirm_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->confirm_visible->GetCurState();
        if (state == ClickLbState::Normal) {
            ui->confirm_edit->setEchoMode(QLineEdit::Password);
        } else {
            ui->confirm_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";
    });

    // 创建定时器
    _countdown_timer = new QTimer(this);
    //连接槽和信号
    connect(_countdown_timer, &QTimer::timeout, [this]() {
        if (_countdown == 0) {
            _countdown_timer->stop();
            emit sigSwitchLogin();
            return;
        }
        _countdown--;
        auto str = QString("Registration is successful \n return to the login interface after %1s").arg(_countdown);
        ui->tip_lb->setText(str);
    });
}


RegisterDialog::~RegisterDialog()
{
    qDebug() << "destruct RegisterDialog";
    delete ui;
}

void RegisterDialog::on_getCode_btn_clicked()
{
    auto email = ui->email_edit->text();
    // \w表示a-z或者是数字, ?表示可有可无, ._防备用户邮箱里有这些特殊字符
    // +表示任意个相同括号内容
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch();

    if (match) {
        // 发送验证码
        QJsonObject json_obj;
        json_obj["email"] = email;
        HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/get_varifycode"),
                                            json_obj, ReqId::ID_GET_VARIFY_CODE, Modules::REGISTERMOD);

    } else {
        showTip(tr("err email address"), false);
    }
    // qDebug() << "Button object name: " << ui->pushButton->objectName();
}

void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err) {
    if (err != ErrorCodes::SUCCESS) {
        showTip(tr("Network request error"), false);
        return;
    }

    //解析JSON 字符串, res 转化为QByteArray(网络链路层的0101组包形成的字节流)
    //字节流变成类对象叫做反序列化, 类对象变成字节流叫做序列化
    //QByteArray -> QJsonDocument(.json文件)
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if (jsonDoc.isNull()) {
        showTip(tr("json parsing failed"), false);
        return;
    }
    //json 解析错误(没有办法转换成对象)
    if (!jsonDoc.isObject()) {
        showTip(tr("json parsing failed"), false);
        return;
    }
    //json解析成功(转换成json对象)
    // jsonDoc.object();
    m_handlers[id](jsonDoc.object());
    return;
}

void RegisterDialog::initHttpHandlers() {
    //注册获取验证码回包
    m_handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](const QJsonObject& jsonObj) {
        int error = jsonObj["error"].toInt(); //json是个键值对, 根据error类型获取错误码
        if (error != ErrorCodes::SUCCESS) {
            showTip(tr("Parameter error"), false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("Verification code has been sent to your email!"),
            true);
        qDebug() << "email is " << email;
    });

    //注册用户回包逻辑
    m_handlers.insert(ReqId::ID_REG_USER, [this](QJsonObject jsonObj) {
        int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            showTip(tr("Parameter error"), false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("User registration successful"), true);
        qDebug() << "user uuid is " << jsonObj["uid"].toString();
        qDebug() << "email is " << email;
        ChangeTipPage();    //注册成功切换页面
    });
}

void RegisterDialog::showTip(QString str, bool b_ok)
{
    if (b_ok) {
        ui->err_tip->setProperty("state", "normal");
    } else {
        ui->err_tip->setProperty("state", "err");
    }
    ui->err_tip->setText(str);
    repolish(ui->err_tip);
}

// 显示错误
void RegisterDialog::AddTipErr(TipErr te, QString tips) {
    _tip_errs[te] = tips;
    showTip(tips, false);
}

// 没有错误就要删除
void RegisterDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if (_tip_errs.empty()) {    //没有错误就直接返回
        ui->err_tip->clear();
        return;
    }
    showTip(_tip_errs.first(), false);   //还有错误就返回第一条
}

void RegisterDialog::ChangeTipPage()
{
    // stackedWidget从page切换到page_2
    _countdown_timer->stop();
    ui->stackedWidget->setCurrentWidget(ui->page_2);

    // 启动定时器, 设置间隔为1000ms
    _countdown_timer->start(1000);
}

bool RegisterDialog::checkUserValid()
{
    if (ui->user_edit->text() == "") {
        AddTipErr(TipErr::TIP_USER_ERR, tr("username cannot be empty"));
        return false;
    }
    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}

bool RegisterDialog::checkEmailValid()
{
    // 验证邮箱地址的正则表达式
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

bool RegisterDialog::checkPassValid()
{
    auto pass = ui->password_edit->text();
    auto confirm = ui->confirm_edit->text();
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
    if (pass != confirm) {
        //提示密码和确认密码不匹配
        AddTipErr(TipErr::TIP_PWD_CONFIRM, tr("Password and confirm password do not match"));
        return false;
    } else {
        DelTipErr(TipErr::TIP_PWD_CONFIRM);
    }
    return true;
}

bool RegisterDialog::checkConfirmValid()
{
    auto pass = ui->password_edit->text();
    auto confirm = ui->confirm_edit->text();
    if (pass.length() < 6 || pass.length() > 15) {
        //提示长度不正确
        AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("Confirm password length should be 6~15"));
        return false;
    }
    // 创造正则表达式对象, 按照上述密码要求
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");  //必须是[]里面的字符
    bool match = regExp.match(pass).hasMatch();
    if (!match) {
        //提示字符非法
        AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("Cannot contain illegal characters"));
        return false;
    }
    DelTipErr(TipErr::TIP_CONFIRM_ERR);
    if (pass != confirm) {
        //提示密码和确认密码不匹配
        AddTipErr(TipErr::TIP_PWD_CONFIRM, tr("Password and confirm password do not match"));
        return false;
    } else {
        DelTipErr(TipErr::TIP_PWD_CONFIRM);
    }
    return true;
}

bool RegisterDialog::checkVarifyValid()
{
    auto pass = ui->varify_edit->text();
    if (pass.isEmpty()) {
        AddTipErr(TipErr::TIP_VARIFY_ERR, tr("Verification code cannot be empty"));
        return false;
    }
    DelTipErr(TipErr::TIP_VARIFY_ERR);
    return true;
}

void RegisterDialog::on_sure_btn_clicked()
{
    if(ui->user_edit->text() == "") {
        showTip(tr("Username cannot be empty"), false);
        return;
    }
    if(ui->email_edit->text() == "") {
        showTip(tr("Email cannot be empty"), false);
        return;
    }
    if(ui->password_edit->text() == "") {
        showTip(tr("Password cannot be empty"), false);
        return;
    }
    if(ui->confirm_edit->text() == "") {
        showTip(tr("Confirm password cannot be empty"), false);
        return;
    }
    if(ui->confirm_edit->text() != ui->password_edit->text()) {
        showTip(tr("Password and confirm password do not match"), false);
        return;
    }
    if(ui->varify_edit->text() == "") {
        showTip(tr("Verification code cannot be empty"), false);
        return;
    }

    QJsonObject json_obj;
    json_obj["user"] = ui->user_edit->text();
    json_obj["email"] = ui->email_edit->text();
    json_obj["passwd"] = xorString(ui->password_edit->text());
    json_obj["confirm"] = xorString(ui->confirm_edit->text());
    json_obj["varifycode"] = ui->varify_edit->text();
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/user_register"),
                                        json_obj, ReqId::ID_REG_USER, Modules::REGISTERMOD);
    // qDebug() << "Button object name: " << ui->pushButton->objectName();
}

void RegisterDialog::on_cancel_btn_clicked()
{
    _countdown_timer->stop();
    emit sigSwitchLogin();
}

