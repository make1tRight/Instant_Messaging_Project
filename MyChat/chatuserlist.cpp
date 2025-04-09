#include "chatuserlist.h"
#include <QScrollBar>
#include "usermgr.h"

ChatUserList::ChatUserList(QWidget *parent) : QListWidget(parent) {
    Q_UNUSED(parent);
    //关闭横向和纵向滚动条(因为要等鼠标进来再显示)
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 安装事件过滤器
    this->viewport()->installEventFilter(this);
}

bool ChatUserList::eventFilter(QObject *watched, QEvent *event)
{
    // 检查事件是否是鼠标悬浮进入或离开
    if (watched == this->viewport()) {
        if (event->type() == QEvent::Enter) {
            // 鼠标悬浮，显示滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        } else if (event->type() == QEvent::Leave) {
            // 鼠标离开，隐藏滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }
    // 检查事件是否是鼠标滚轮事件
    if (watched == this->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        //y是上下滚动
        int numDegrees = wheelEvent->angleDelta().y() / 8;
        int numSteps = numDegrees / 15; // 计算滚动步数

        // 设置滚动幅度(numSteps是正->往上走, numSteps是负->--得正往下走)
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);

        // 检查是否滚动到底部
        QScrollBar *scrollBar = this->verticalScrollBar();
        int maxScrollValue = scrollBar->maximum();
        int currentValue = scrollBar->value();
        //int pageSize = 10; // 每页加载的联系人数量

        //加载更多内容
        if (maxScrollValue - currentValue <= 0) {
            // auto b_loaded = UserMgr::GetInstance()->IsLoadChatFin();
            // if(b_loaded){
            //     return true;
            // }

            // if(_load_pending){
            //     return true;
            // }
            // 滚动到底部，加载新的联系人
            qDebug()<<"load more chat user";
            // _load_pending = true;

            // QTimer::singleShot(100, [this](){
            //     _load_pending = false;
            //     QCoreApplication::quit(); // 完成后退出应用程序
            // });
            //发送信号通知聊天界面加载更多聊天内容
            emit sig_loading_chat_user();
        }

        return true; // 停止事件传递
    }
    //其他事件走基类的方法
    return QListWidget::eventFilter(watched, event);
}
