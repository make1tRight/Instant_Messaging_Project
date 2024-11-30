#ifndef GROUPTIPITEM_H
#define GROUPTIPITEM_H

#include <QWidget>
#include "listitembase.h"

namespace Ui {
class GroupTipItem;
}

class GroupTipItem : public ListItemBase
{
    Q_OBJECT

public:
    explicit GroupTipItem(QWidget *parent = nullptr);
    ~GroupTipItem();

    QSize sizeHint() const override;
    void SetGroupTip(QString str);
private:
    Ui::GroupTipItem *ui;
    QString _tip;
};

#endif // GROUPTIPITEM_H
