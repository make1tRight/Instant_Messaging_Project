#ifndef CHATUSERWID_H
#define CHATUSERWID_H

#include "listitembase.h"

namespace Ui {
class ChatUserWid;
}

class ChatUserWid : public ListItemBase//让listitembase去继承QWidget
{
    Q_OBJECT

public:
    explicit ChatUserWid(QWidget *parent = nullptr);
    ~ChatUserWid();

    QSize sizeHint() const override {
        return QSize(250, 70);//返回自定义尺寸
    }
    void SetInfo(QString name, QString head, QString msg);

private:
    Ui::ChatUserWid *ui;
    QString _name;
    QString _head;
    QString _msg;
};

#endif // CHATUSERWID_H