#include "chatpage.h"
#include "ui_chatpage.h"
#include <QStyleOption>
#include <QPainter>
#include "ChatItemBase.h"
#include "textbubble.h"
#include "picturebubble.h"

ChatPage::ChatPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatPage)
{
    ui->setupUi(this);
    //设置按钮样式
    ui->receive_btn->SetState("normal","hover","press");
    ui->send_btn->SetState("normal","hover","press");

    //设置图标样式
    ui->emo_lb->SetState("normal","hover","press","normal","hover","press");
    ui->file_lb->SetState("normal","hover","press","normal","hover","press");

}

ChatPage::~ChatPage()
{
    delete ui;
}

// 确保 QSS 样式在控件上正确渲染
void ChatPage::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    // opt.init(this);可能新版本被删掉了
    opt.initFrom(this);
    QPainter p(this);
    //使样式表生效
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ChatPage::on_send_btn_clicked()
{
    auto pTextEdit = ui->chatEdit;
    ChatRole role = ChatRole::Self;
    QString userName = QStringLiteral("make1tRight");
    QString userIcon = ":/res/head_1.png";

    const QVector<MsgInfo>& msgList = pTextEdit->getMsgList();
    for (int i = 0; i < msgList.size(); ++i) {
        QString type = msgList[i].msgFlag;
        ChatItemBase* pChatItem = new ChatItemBase(role);
        pChatItem->setUserIcon(QPixmap(userIcon));
        pChatItem->setUserName(userName);
        QWidget* pBubble = nullptr;
        if (type == "text") {
            pBubble = new TextBubble(role, msgList[i].content);
        } else if (type == "image") {
            pBubble = new PictureBubble(QPixmap(msgList[i].content), role);
        } else if (type == "file") {
            //下一季写文件
        }
        if (pBubble != nullptr) {
            pChatItem->setWidget(pBubble);
            ui->chat_data_list->appendChatItem(pChatItem);
        }
    }
}

