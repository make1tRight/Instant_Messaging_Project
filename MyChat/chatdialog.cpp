#include "chatdialog.h"
#include "ui_chatdialog.h"
#include <QAction>
#include <QRandomGenerator>
#include "chatuserwid.h"
#include "loadingdlg.h"

ChatDialog::ChatDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChatDialog), _mode(ChatUIMode::ChatMode), _state(ChatUIMode::ChatMode), _b_loading(false)
{
    ui->setupUi(this);
    ui->add_btn->SetState("normal", "hover", "press");
    ui->search_edit->SetMaxLength(15);

    QAction* searchAction = new QAction(ui->search_edit);
    searchAction->setIcon(QIcon(":/res/search.png"));
    ui->search_edit->addAction(searchAction,QLineEdit::LeadingPosition);//在头部的位置加放大镜
    ui->search_edit->setPlaceholderText(QStringLiteral("search"));

    //创建清除动作并设置图标
    QAction* clearAction = new QAction(ui->search_edit);
    clearAction->setIcon(QIcon(":/res/close_transparent.png"));
    ui->search_edit->addAction(clearAction,QLineEdit::TrailingPosition);//在尾部的位置加清除
    //当需要显示清除图标的时候更改为实际的清除图标
    connect(ui->search_edit, &QLineEdit::textChanged, [clearAction](const QString &text) {
        if (!text.isEmpty()) {
            clearAction->setIcon(QIcon(":/res/close_search.png"));
        } else {
            clearAction->setIcon(QIcon(":/res/close_transparent.png")); // 文本为空时，切换回透明图标
        }
    });
    // 连接清除动作的触发信号到槽函数，用于清除文本
    connect(clearAction, &QAction::triggered, [this, clearAction]() {
        ui->search_edit->clear();
        clearAction->setIcon(QIcon(":/res/close_transparent.png")); // 清除文本后，切换回透明图标
        ui->search_edit->clearFocus();
        //清除按钮被按下则不显示搜索框

        ShowSearch(false);
    });
    ui->search_edit->SetMaxLength(15);
    ShowSearch(false);
    //连接加载信号和槽
    connect(ui->chat_user_list, &ChatUserList::sig_loading_chat_user,
            this, &ChatDialog::slot_loading_chat_user);
    //显示我们写的widget
    addChatUserList();

    //加载sidebar的图标
    QPixmap pixmap(":/res/head_1.png");
    // ui->side_head_lb->setPixmap(pixmap);
    QPixmap scaledPixmap = pixmap.scaled(ui->side_head_lb->size(), Qt::KeepAspectRatio);
    ui->side_head_lb->setPixmap(scaledPixmap);
    ui->side_head_lb->setScaledContents(true);

    ui->side_chat_lb->setProperty("state","normal");
    ui->side_chat_lb->SetState("normal","hover","pressed",
                               "selected_normal","selected_hover","selected_pressed");
    ui->side_contact_lb->SetState("normal","hover","pressed",
                                  "selected_normal","selected_hover","selected_pressed");

    AddLBGroup(ui->side_chat_lb);
    AddLBGroup(ui->side_contact_lb);

    connect(ui->side_chat_lb, &StateWidget::clicked, this, &ChatDialog::slot_side_chat);
    connect(ui->side_contact_lb, &StateWidget::clicked, this, &ChatDialog::slot_side_contact);

    //链接搜索框输入变化 -> 搜索框里面输入文字以后, 就显示搜索用的框(隐藏联系人的widget)
    connect(ui->search_edit, &QLineEdit::textChanged, this, &ChatDialog::slot_text_changed);
    //检测鼠标点击位置 -> 判断是否要清空搜索框
    this->installEventFilter(this);
}

ChatDialog::~ChatDialog()
{
    delete ui;
}

void ChatDialog::addChatUserList()
{
    //模拟测试条目
    // 创建QListWidgetItem，并设置自定义的widget
    for(int i = 0; i < 13; i++){
        int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
        //让消息, 头像, 名字随机显示
        int str_i = randomValue%strs.size();
        int head_i = randomValue%heads.size();
        int name_i = randomValue%names.size();

        auto *chat_user_wid = new ChatUserWid();
        chat_user_wid->SetInfo(names[name_i], heads[head_i], strs[str_i]);
        QListWidgetItem *item = new QListWidgetItem;
        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(chat_user_wid->sizeHint());
        //将item加入到chat_user_list中, 并且以chat_user_wid显示
        ui->chat_user_list->addItem(item);
        ui->chat_user_list->setItemWidget(item, chat_user_wid);

        // connect(ui->side_chat_lb, &StateWidget::clicked, this, &ChatDialog::slot_side_chat);
        // connect(ui->side_contact_lb, &StateWidget::clicked, this, &ChatDialog::slot_side_contact);

        // //链接搜索框输入变化
        // connect(ui->search_edit, &QLineEdit::textChanged, this, &ChatDialog::slot_text_changed);
    }
}

void ChatDialog::ClearLabelState(StateWidget *lb)
{
    //组内的按钮一次只能激活一个
    for (auto& ele : _lb_list) {
        if (ele == lb) {
            continue;
        }
        ele->ClearState();
    }
}

void ChatDialog::ShowSearch(bool b_search)
{
    if(b_search){
        ui->chat_user_list->hide();
        ui->con_user_list->hide();
        //显示search_list
        ui->search_list->show();
        _mode = ChatUIMode::SearchMode;
    }else if(_state == ChatUIMode::ChatMode){
        //显示chat_user_list
        ui->chat_user_list->show();
        ui->con_user_list->hide();
        ui->search_list->hide();
        _mode = ChatUIMode::ChatMode;
    }else if(_state == ChatUIMode::ContactMode){
        ui->chat_user_list->hide();
        ui->search_list->hide();
        //显示con_user_list
        ui->con_user_list->show();
        _mode = ChatUIMode::ContactMode;
    }
}

void ChatDialog::AddLBGroup(StateWidget *lb)
{
    _lb_list.push_back(lb);
}

bool ChatDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        handleGlobalMousePress(mouseEvent);
    }
    return QDialog::eventFilter(watched, event);
}

void ChatDialog::handleGlobalMousePress(QMouseEvent *event)
{
    //实现点击位置的判断和逻辑处理
    //不处于搜索模式直接返回
    if (_mode != ChatUIMode::SearchMode) {
        return;
    }
    // QPoint posInSearchList = ui->search_list->mapFromGlobal(event->globalPos());
    QPoint posInSearchList = ui->search_list->mapFromGlobal(event->globalPosition().toPoint());
    if (!ui->search_list->rect().contains(posInSearchList)) {
        //如果不在聊天列表内, 清空输入框
        ui->search_list->clear();
        ShowSearch(false);
    }
}

void ChatDialog::slot_loading_chat_user()
{
    if(_b_loading){
        return;
    }

    _b_loading = true;
    LoadingDlg *loadingDialog = new LoadingDlg(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    qDebug() << "add new data to list.....";
    addChatUserList();
    // 加载完成后关闭对话框
    loadingDialog->deleteLater();

    _b_loading = false;
}

void ChatDialog::slot_side_chat()
{
    qDebug()<< "receive side chat clicked";
    ClearLabelState(ui->side_chat_lb);
    ui->stackedWidget->setCurrentWidget(ui->chat_page);
    _state = ChatUIMode::ChatMode;
    ShowSearch(false);
}

void ChatDialog::slot_side_contact()
{
    qDebug()<< "receive side contact clicked";
    ClearLabelState(ui->side_contact_lb);
    ui->stackedWidget->setCurrentWidget(ui->friend_apply_page);
    _state = ChatUIMode::ContactMode;
    ShowSearch(false);
}

void ChatDialog::slot_text_changed(const QString &str)
{
    //字符串不为空就展示
    if (!str.isEmpty()) {
        ShowSearch(true);
    }
}
