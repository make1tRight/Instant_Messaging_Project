#ifndef CUSTOMIZEEDIT_H
#define CUSTOMIZEEDIT_H
#include <QLineEdit>
#include <QDebug>

class CustomizeEdit : public QLineEdit
{
    Q_OBJECT
public:
    CustomizeEdit(QWidget *parent = nullptr);
    void SetMaxLength(int maxLen);
protected:
    void focusOutEvent(QFocusEvent *event) override {
        //执行失去焦点时的处理逻辑(就是鼠标离开框框)
        QLineEdit::focusOutEvent(event);//调用基类的方法
        //发送失去焦点的信号
        emit sig_foucus_out();
    }
private:
    //限制输入长度
    void limitTextLength(QString text) {
        if (_max_len <= 0) {
            return;
        }
        QByteArray byteArray = text.toUtf8();       //转成字节流(中文2/3个字节, 英文一位一个字节)
        if (byteArray.size() > _max_len) {
            byteArray = byteArray.left(_max_len);   //截取左边_max_len的内容
            this->setText(QString::fromUtf8(byteArray));
        }
    }

    int _max_len;
signals:
    void sig_foucus_out();
};

#endif // CUSTOMIZEEDIT_H
