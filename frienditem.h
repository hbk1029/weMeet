#ifndef FRIENDITEM_H
#define FRIENDITEM_H

#include <QWidget>

namespace Ui {
class FriendItem;
}

class FriendItem : public QWidget
{
    Q_OBJECT

public:
    explicit FriendItem(QWidget *parent = nullptr);
    ~FriendItem();

    void setFriendInfo(int friendId, QString name, int iconId, QString feeling, int status);
    //只更新在线状态 (不覆盖昵称签名)
    void setFriendStatus(int status);

    int getFriendId() const { return m_friendId; }
    QString getFriendName() const { return m_name; }
    int getFriendStatus() const { return m_status; }

private:
    Ui::FriendItem *ui;
    int m_friendId;
    QString m_name;
    QString m_feeling;
    int m_status;
};

#endif
