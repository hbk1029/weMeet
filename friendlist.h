#ifndef FRIENDLIST_H
#define FRIENDLIST_H

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
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
    //设置用户信息
    void setUserInfo(QString name, int iconId, QString feeling);
    //添加好友项
    void addFriendItem(FriendItem* item);
    //移除好友项
    void removeFriendItem(FriendItem* item);
    //添加会议历史记录
    void addMeetingHistory(QString meetingId);
    //更新会议状态
    void updateMeetingStatus(QString meetingId, bool isActive);
    //从本地加载会议历史记录
    void loadMeetingHistory();
    //更新好友请求角标
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
    void on_pb_quickMeeting_clicked();  // 快速会议
    void on_pb_addFriend_clicked();
    //搜索过滤
    void on_le_search_textChanged(const QString& text);
    //右键菜单删除好友
    void on_lw_friendList_menuRequested(const QPoint& pos);

    void rebuildHistoryList();
private:
    void setNavButtonActive(QPushButton* button, bool active);

    Ui::FriendList *ui;
    int m_iconId;
    QListWidget* m_pMeetingHistory;  // 会议历史列表
    QList<MeetingRecord> m_meetingHistory; // 历史会议号+时间+状态
    QLineEdit* m_pMeetingCodeInput;   // 会议号输入框
    QPushButton* m_pJoinByCodeBtn;    // 加入按钮
    QPushButton* m_pQuickMeetingBtn;  // 快速会议按钮
    QWidget* m_pJoinCard;             // 加入会议卡片
    QWidget* m_pQuickCard;            // 快速会议卡片
    QStackedWidget* m_pHistoryStack;   // 会议历史+空状态堆叠
    QLabel* m_pEmptyPlaceholder;       // 空状态占位标签
    QPushButton* m_pFriendRequestsBtn; // 好友请求按钮
    QLabel* m_pFriendCountLabel;      // 好友数量标签
};

#endif // FRIENDLIST_H
