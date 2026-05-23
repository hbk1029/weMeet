#ifndef FRIENDLIST_H
#define FRIENDLIST_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include "frienditem.h"
#include <QStringList>
#include <QList>
#include <QStackedWidget>

struct MeetingRecord {
    QString id;
    QString timeStr;
    bool isActive; // true=进行中, false=已结束
};

namespace Ui {
class FriendList;
}

class FriendList : public QWidget
{
    Q_OBJECT
public:
    explicit FriendList(QWidget *parent = nullptr);
    ~FriendList();
    void setUserInfo(QString name, int iconId, QString feeling);
    void addFriendItem(FriendItem* item);
    void removeFriendItem(FriendItem* item);
    void addMeetingHistory(QString meetingId);
    void updateMeetingStatus(QString meetingId, bool isActive);
    void loadMeetingHistory();
    void saveMeetingHistory();         // 持久化到 JSON 文件
    void updateFriendRequestBadge(int count);

signals:
    void sig_addFriendItem();
    void sig_meetingCreate();
    void sig_meetingJoin();
    void sig_meetingJoinHistory(QString meetingId);
    void sig_deleteFriend(int friendId);
    void sig_showFriendRequests();

private slots:
    void on_pb_contact_clicked();
    void on_pb_meeting_clicked();
    void on_pb_settings_clicked();
    void on_pb_createMeeting_clicked();
    void on_pb_joinMeeting_clicked();
    void on_pb_quickMeeting_clicked();
    void on_pb_addFriend_clicked();
    void on_pb_joinByCode_clicked();
    void on_le_search_textChanged(const QString& text);
    void on_lw_friendList_customContextMenuRequested(const QPoint& pos);
    void rebuildHistoryList();

private:
    void setNavButtonActive(QPushButton* button, bool active);

    Ui::FriendList *ui;
    int m_iconId;
    QList<MeetingRecord> m_meetingHistory;       // 历史会议号+时间+状态
    QPushButton* m_pFriendRequestsBtn;           // 好友请求按钮(动态insert到hl_actions)
    QLabel* m_pFriendCountLabel;                 // 好友数量标签(同上)
};

#endif // FRIENDLIST_H
