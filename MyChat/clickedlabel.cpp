#include "clickedlabel.h"
#include <QMouseEvent>



ClickedLabel::ClickedLabel(QWidget *parent)
    : QLabel(parent), _curstate(ClickLbState::Normal)
{
    this->setCursor(Qt::PointingHandCursor);
}

void ClickedLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (_curstate == ClickLbState::Normal) {    //如果是normal状态, 点击鼠标左键就切换成selected
            qDebug() << "clicked, change to selected hover: " << _selected_hover;
            _curstate = ClickLbState::Selected;
            setProperty("state", _selected_press);
            repolish(this);
            update();
        } else {                                    //如果是selected状态, 点击鼠标左键就切换成normal
            qDebug() << "clicked, change to normal hover: " << _normal_hover;
            _curstate = ClickLbState::Normal;
            setProperty("state", _normal_press);    //根据qss文件重新选择显示的效果
            repolish(this);                         //重新刷新显示的效果
            update();       //[15-56:39]update强制刷新效果
        }
        // emit clicked();
        return;//如果是自己处理的就不需要基类处理了
    }
    //调用基类的mousePressEvent保证正常事件的处理
    QLabel::mousePressEvent(event);
}

void ClickedLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (_curstate == ClickLbState::Normal) {    //如果是normal状态, 点击鼠标左键就切换成selected
            // qDebug() << "clicked, change to selected hover: " << _selected_hover;
            setProperty("state", _normal_hover);
            repolish(this);
            update();
        } else {                                    //如果是selected状态, 点击鼠标左键就切换成normal
            // qDebug() << "clicked, change to normal hover: " << _normal_hover;
            setProperty("state", _selected_hover);    //根据qss文件重新选择显示的效果
            repolish(this);                         //重新刷新显示的效果
            update();       //[15-56:39]update强制刷新效果
        }
        emit clicked(this->text(), _curstate);
        return;//如果是自己处理的就不需要基类处理了
    }
    //调用基类的mousePressEvent保证正常事件的处理
    QLabel::mousePressEvent(event);
}

void ClickedLabel::enterEvent(QEnterEvent *event)
{
    // 处理鼠标悬停进入的逻辑
    if (_curstate == ClickLbState::Normal) {
        qDebug() << "enter, change to normal hover: " << _normal_hover;
        setProperty("state", _normal_hover);    //切换成qss里面配置的悬浮图标
        repolish(this);
        update();
    } else {
        qDebug() << "enter, change to selected hover: " << _selected_hover;
        setProperty("state", _selected_hover);
        repolish(this);
        update();
    }
    QLabel::enterEvent(event);
}

void ClickedLabel::leaveEvent(QEvent *event)
{
    // 处理鼠标悬停离开的逻辑
    if (_curstate == ClickLbState::Normal) {
        qDebug() << "leave, change to normal: " << _normal;
        setProperty("state", _normal);    //切换成qss里面配置的悬浮图标
        repolish(this);
        update();
    } else {
        qDebug() << "leave, change to selected: " << _selected;
        setProperty("state", _selected);
        repolish(this);
        update();
    }
    QLabel::leaveEvent(event);
}

void ClickedLabel::SetState(QString normal, QString hover, QString press, QString select, QString select_hover, QString select_press)
{
    _normal = normal;
    _normal_hover = hover;
    _normal_press = press;

    _selected = select;
    _selected_hover = select_hover;
    _selected_press = select_press;

    // 这两行代码的作用是什么?
    setProperty("state", normal);
    repolish(this);
}

ClickLbState ClickedLabel::GetCurState()
{
    return _curstate;
}

bool ClickedLabel::SetCurState(ClickLbState state)
{
    _curstate = state;
    if (_curstate == ClickLbState::Normal) {
        setProperty("state", _normal);
        repolish(this);
    }
    else if (_curstate == ClickLbState::Selected) {
        setProperty("state", _selected);
        repolish(this);
    }
    return true;

}

void ClickedLabel::ResetNormalState()
{
    _curstate = ClickLbState::Normal;
    setProperty("state", _normal);
    repolish(this);
}
