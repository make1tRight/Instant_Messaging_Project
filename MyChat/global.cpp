#include "global.h"


QString gate_url_prefix = "";


std::function<void(QWidget*)> repolish = [](QWidget* w) {
    w->style()->unpolish(w);    //去掉原来的样式
    w->style()->polish(w);      //更新为新样式
};

std::function<QString(QString)> xorString = [](QString input) {
    QString result = input;
    int length = input.length();
    length = length % 255;
    for (int i = 0; i < length; ++i) {
        // 对每个字符进行异或操作（相当于一个加密算法）
        // QChar就是一个字符
        result[i] = QChar(static_cast<ushort>(input[i].unicode()) ^ static_cast<ushort>(length));
    }
    return result;
};
