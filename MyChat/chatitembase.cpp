#include "chatitembase.h"

ChatItemBase::ChatItemBase(ChatRole role, QWidget *parent) : QWidget(parent), m_role(role)
{
    //初始化+起名字
    m_pNameLabel = new QLabel();
    m_pNameLabel->setObjectName("chat_user_name");
    //设置字体
    QFont font("Microsoft YaHei");
    font.setPointSize(9);
    m_pNameLabel->setFont(font);
    //设置高度
    m_pNameLabel->setFixedHeight(20);

    //图标
    m_pIconLabel = new QLabel();
    m_pIconLabel->setScaledContents(true);
    m_pIconLabel->setFixedSize(42, 42);

    //聊天气泡
    m_pBubble = new QWidget();
    QGridLayout *pGLayout = new QGridLayout();
    pGLayout->setVerticalSpacing(3);    //控件之间的垂直间距
    pGLayout->setHorizontalSpacing(3);  //控件之间的水平间距
    // pGLayout->setMargin(3);
    pGLayout->setContentsMargins(3, 3, 3, 3);
    QSpacerItem* pSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    if (m_role == ChatRole::Self) {                     //自己靠右显示
        m_pNameLabel->setContentsMargins(0,0,8,0);      //图片与名字间距是8
        m_pNameLabel->setAlignment(Qt::AlignRight);
        pGLayout->addWidget(m_pNameLabel, 0, 1, 1, 1);
        pGLayout->addWidget(m_pIconLabel, 0, 2, 2, 1, Qt::AlignTop);//icon占2行1列, 靠上显示
        pGLayout->addItem(pSpacer, 1, 0, 1, 1);
        pGLayout->addWidget(m_pBubble, 1, 1, 1, 1);
        //设置拉伸比例
        pGLayout->setColumnStretch(0, 2);//第一列占比2
        pGLayout->setColumnStretch(1, 3);//第二列占比3
    } else {                                            //对方靠左显示
        m_pNameLabel->setContentsMargins(8,0,0,0);      //图片与名字间距是8
        m_pNameLabel->setAlignment(Qt::AlignLeft);
        pGLayout->addWidget(m_pNameLabel, 0, 1, 1, 1);
        pGLayout->addWidget(m_pIconLabel, 0, 0, 2, 1, Qt::AlignTop);//icon占2行1列, 靠上显示
        pGLayout->addItem(pSpacer, 2, 2, 1, 1);
        pGLayout->addWidget(m_pBubble, 1, 1, 1, 1);
        pGLayout->setColumnStretch(2, 2);//第三列占比2
        pGLayout->setColumnStretch(1, 3);//第二列占比3
    }
    //使得布局生效
    this->setLayout(pGLayout);
}

void ChatItemBase::setUserName(const QString &name)
{
    m_pNameLabel->setText(name);
}

void ChatItemBase::setUserIcon(const QPixmap &icon)
{
    m_pIconLabel->setPixmap(icon);
}

void ChatItemBase::setWidget(QWidget *w)
{
    QGridLayout* pGLayout = (qobject_cast<QGridLayout*>)(this->layout());
    //将原来占位用的bubble替换成新的
    pGLayout->replaceWidget(m_pBubble, w);
    delete m_pBubble;//这个delete不写会造成内存泄漏
    m_pBubble = w;
}
