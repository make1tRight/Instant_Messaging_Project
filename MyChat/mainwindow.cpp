#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tcpmgr.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_loginDialog = new LoginDialog(this);
    m_loginDialog->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(m_loginDialog);
    //m_loginDialog->show();

    // 创建和注册消息链接
    connect(m_loginDialog, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    // 创建登录界面忘记密码信号的连接
    connect(m_loginDialog, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    // 创建与聊天界面的连接
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_switch_chatdlg, this, &MainWindow::SlotSwitchChat);

    // emit TcpMgr::GetInstance()->sig_switch_chatdlg();//测试用, 自己发sig_switch_chatdlg就不用开3个后端让后端发

    // m_registerDialog = new RegisterDialog(this);
    // // 设置通过代码完全控制窗口的外观和行为 | 无边框
    // m_loginDialog->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    // m_registerDialog->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    // m_registerDialog->hide();


}

MainWindow::~MainWindow()
{
    delete ui;
    //if (m_loginDialog) {
    //    delete m_loginDialog;
    //    m_loginDialog = nullptr;
    //}

    //if (m_registerDialog) {
    //    delete m_registerDialog;
    //    m_registerDialog = nullptr;
    //}
}

void MainWindow::SlotSwitchReg()
{
    //[15-1:18:28] 界面显示逻辑
    m_registerDialog = new RegisterDialog(this);
    // 设置通过代码完全控制窗口的外观和行为 | 无边框
    m_registerDialog->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    // 连接注册界面返回登录信号
    connect(m_registerDialog, &RegisterDialog::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);


    setCentralWidget(m_registerDialog);
    m_loginDialog->hide();
    m_registerDialog->show();
}

void MainWindow::SlotSwitchLogin()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    m_loginDialog = new LoginDialog(this);
    m_loginDialog->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(m_loginDialog);

    m_registerDialog->hide();
    m_loginDialog->show();

    //连接登录界面注册信号
    connect(m_loginDialog, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(m_loginDialog, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
}

void MainWindow::SlotSwitchReset()
{
    m_resetDialog = new ResetDialog(this);
    m_resetDialog->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);//不能最大最小化, 无边框
    setCentralWidget(m_resetDialog);

    m_loginDialog->hide(); //对象树系统会自动回收, 释放资源
    m_resetDialog->show();
    //修改密码以后返回登录界面的信号和槽函数
    connect(m_resetDialog, &ResetDialog::switchLogin, this, &MainWindow::SlotSwitchLogin2);
}

void MainWindow::SlotSwitchLogin2()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    m_loginDialog = new LoginDialog(this);
    m_loginDialog->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(m_loginDialog);

    m_resetDialog->hide();
    m_loginDialog->show();

    //连接登录界面忘记密码信号
    connect(m_loginDialog, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    //连接登录界面注册信号
    connect(m_loginDialog, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
}

void MainWindow::SlotSwitchChat()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    m_chatDialog = new ChatDialog(this);
    m_chatDialog->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(m_chatDialog);

    m_loginDialog->hide();
    m_chatDialog->show();
    qDebug() << "SlotSwitchChat called";
    this->setMinimumSize(QSize(1050, 900));
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}
