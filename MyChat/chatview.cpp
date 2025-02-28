#include "chatview.h"
#include <QScrollBar>
#include <QEvent>
#include <QStyleOption>
#include <QPainter>

ChatView::ChatView(QWidget *parent) : QWidget(parent), isAppended(false)
{
    //相当于背板
    QVBoxLayout *pMainLayout = new QVBoxLayout();//vertical box
    this->setLayout(pMainLayout);
    // pMainLayout->setMargins(0);
    pMainLayout->unsetContentsMargins();
    //滚动区域
    m_pScrollArea = new QScrollArea();
    m_pScrollArea->setObjectName("chat_area");
    pMainLayout->addWidget(m_pScrollArea);

    QWidget *w = new QWidget(this);
    w->setObjectName("chat_bg");
    w->setAutoFillBackground(true);//自动填充背景
    //在Widget里面创建子垂直布局
    QVBoxLayout *pHLayout_1 = new QVBoxLayout();
    //10000是伸缩因子, 让w尽可能大, w尽可能大滚动区域就会尽可能大
    pHLayout_1->addWidget(new QWidget(), 100000);//用一个widget更好管理聊天的每条信息
    w->setLayout(pHLayout_1);
    m_pScrollArea->setWidget(w);
    //默认关闭垂直滚动条
    m_pScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QScrollBar* pVScrollBar = m_pScrollArea->verticalScrollBar();
    //范围变化触发槽函数
    connect(pVScrollBar, &QScrollBar::rangeChanged, this, &ChatView::onVScrollBarMoved);

    //把垂直ScrollBar放到上边 而不是原来的并排
    QHBoxLayout *pHLayout_2 = new QHBoxLayout();
    pHLayout_2->addWidget(pVScrollBar, 0, Qt::AlignRight);//滚动条放在滚动区域的右边
    // pHLayout_2->setMargin(0);
    pMainLayout->unsetContentsMargins();
    m_pScrollArea->setLayout(pHLayout_2);//将垂直布局(滚动条)设置到滚动区域中
    pVScrollBar->setHidden(true);
    //允许widget重新设置大小, 安装事件过滤器
    m_pScrollArea->setWidgetResizable(true);
    m_pScrollArea->installEventFilter(this);
    initStyleSheet();
}

void ChatView::appendChatItem(QWidget *item)
{
    QVBoxLayout *vl = qobject_cast<QVBoxLayout *>(m_pScrollArea->widget()->layout());
    // qDebug() << "vl->count() is " << vl->count();
    vl->insertWidget(vl->count()-1, item);//在第0个位置, 也就是w里面的widget上方
    isAppended = true;//调整滚动条
}

void ChatView::prependChatItem(QWidget *item)
{

}

void ChatView::insertChatItem(QWidget *before, QWidget *item)
{

}

bool ChatView::eventFilter(QObject *o, QEvent *e)
{
    if(e->type() == QEvent::Enter && o == m_pScrollArea)
    {
        m_pScrollArea->verticalScrollBar()->setHidden(m_pScrollArea->verticalScrollBar()->maximum() == 0);//鼠标进入的时候如果没达到最大值就不显示bar
    }
    else if(e->type() == QEvent::Leave && o == m_pScrollArea)//鼠标离开的时候直接隐藏滚动条
    {
        m_pScrollArea->verticalScrollBar()->setHidden(true);
    }
    return QWidget::eventFilter(o, e);
}

void ChatView::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    // opt.init(this);可能新版本被删掉了
    opt.initFrom(this);
    QPainter p(this);
    //使样式表生效
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ChatView::onVScrollBarMoved(int min, int max)
{
    if(isAppended) //添加item可能调用多次
    {
        QScrollBar *pVScrollBar = m_pScrollArea->verticalScrollBar();
        pVScrollBar->setSliderPosition(pVScrollBar->maximum());//设置最大的位置
        //500毫秒内可能调用多次, 防止一直拉数据
        QTimer::singleShot(500, [this]() {
           isAppended = false;
        });
    }
}

void ChatView::initStyleSheet()
{

}
