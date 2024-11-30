#ifndef LISTITEMBASE_H
#define LISTITEMBASE_H
#include <QWidget>
#include "global.h"

class ListItemBase : public QWidget
{
    Q_OBJECT
public:
    explicit ListItemBase(QWidget* parent = nullptr);
    void SetItemType(ListItemType itemType);

    ListItemType GetItemType();
protected://是给qt事件系统调用的, 只让子类调用不能让其他函数调用
    // 确保 QSS 样式在控件上正确渲染
    virtual void paintEvent(QPaintEvent *event) override;
private:
    ListItemType _itemType;
};

#endif // LISTITEMBASE_H
