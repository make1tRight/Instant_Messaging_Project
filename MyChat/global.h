﻿#ifndef GLOBAL_H
#define GLOBAL_H
#include <QWidget>
#include <functional>
#include <QRegularExpression>
#include "QStyle"
#include <memory>
#include <iostream>
#include <mutex>
#include <QByteArray>
#include <QNetworkReply>
#include <QJsonObject>
#include <QDir>
#include <QSettings>

/*
* @brief repolish 用于刷新qss
*/
extern std::function<void(QWidget*)> repolish;

extern std::function<QString(QString)> xorString;

enum ReqId {
    ID_GET_VARIFY_CODE = 1001,  //获取验证码
    ID_REG_USER = 1002,         //注册用户
    ID_RESET_PWD = 1003,        //重置密码
    ID_LOGIN_USER = 1004,       //用户登录
    ID_CHAT_LOGIN = 1005,       //登录聊天服务器
    ID_CHAT_LOGIN_RSP = 1006,   //登录聊天服务器回包
};

enum Modules {
    REGISTERMOD = 0,            //注册模块
    RESETMOD = 1,               //重置模块
    LOGINMOD = 2,               //登录模块
};

// 注册界面的各种错误
enum TipErr {
    TIP_SUCCESS = 0,
    TIP_EMAIL_ERR = 1,
    TIP_PWD_ERR = 2,
    TIP_CONFIRM_ERR = 3,
    TIP_PWD_CONFIRM = 4,
    TIP_VARIFY_ERR = 5,
    TIP_USER_ERR = 6
};

enum ErrorCodes {
    SUCCESS = 0, 
    ERR_JSON = 1,               //JSON解析失败
    ERR_NETWORK = 2,            //网络错误
};

enum ClickLbState {
    Normal = 0,
    Selected = 1
};

extern QString gate_url_prefix;

struct ServerInfo {
    QString Host;
    QString Port;
    QString Token;
    int Uid;
};

//聊天界面几种模式
enum ChatUIMode{
    SearchMode, //搜索模式
    ChatMode, //聊天模式
    ContactMode, //联系模式
};

//自定义QListWidgetItem的几种类型
enum ListItemType{
    CHAT_USER_ITEM, //聊天用户
    CONTACT_USER_ITEM, //联系人用户
    SEARCH_USER_ITEM, //搜索到的用户
    ADD_USER_TIP_ITEM, //提示添加用户
    INVALID_ITEM,  //不可点击条目
    GROUP_TIP_ITEM, //分组提示条目
    LINE_ITEM,  //分割线
    APPLY_FRIEND_ITEM, //好友申请
};

enum ChatRole {
    Self,
    Other
};

//暂时用
struct MsgInfo {
    QString msgFlag;    //"text, image, file"
    QString content;    //表示文件和图像的url, 文本信息
    QPixmap pixmap;     //文件和图片缩略图
};

//申请好友标签输入框最低长度
const int MIN_APPLY_LABEL_ED_LEN = 40;

const QString add_prefix = "add tags: ";

const int  tip_offset = 5;


const std::vector<QString> strs = {
    "hello world!",
    "nice to meet you",
    "New year, new life",
    "You have to love yourself",
    "My love is written in the wind ever since the whole world is you"
};


const std::vector<QString> heads = {
    ":/res/head_1.png",
    ":/res/head_2.png",
    ":/res/head_3.png",
    ":/res/head_4.png",
    ":/res/head_5.png"
};

const std::vector<QString> names = {
    "Lily",
    "Ben",
    "Max",
    "Chandler",
    "Hank"
};

#endif // GLOBAL_H