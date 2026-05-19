#ifndef KERNEL_H
#define KERNEL_H
#include"logindialog.h"
#include"friendlist.h"
#include"frienditem.h"
#include"./Net/def.h"
#include"./Mediator/INetMediator.h"
#include<QObject>
#include"common.h"
#include"meetingdialog.h"
#include"addfrienddialog.h"
#include"friendrequestdialog.h"

class Kernel;
typedef void (Kernel::*PFUN)(char* data, int len, long from);

class Kernel : public QObject
{
    Q_OBJECT
public:
    explicit Kernel(QObject *parent = nullptr);
    static Kernel* getSingleInstance();
    //初始化配置
    void initConfig();
    //初始化数组
    void setProtocol();
    //处理注册回复
    void dealRegisterRs(char* data, int len, long from);
    //处理登录回复
    void dealLoginRs(char* data, int len, long from);
    //处理用户信息
    void dealFriendInfo(char* data, int len, long from);
    //处理添加好友回复
    void dealAddFriendRs(char* data, int len, long from);
    //处理好友请求通知
    void dealFriendRequestNotify(char* data, int len, long from);
    //处理聊天请求
    void dealChatRq(char* data, int len, long from);
    //处理聊天回复
    void dealChatRs(char* data, int len, long from);
    //处理下线请求
    void dealOfflineRq(char* data, int len, long from);
    //处理音频帧接收
    void dealMeetingAudioNotify(char* data, int len, long from);
    //处理视频帧接收
    void dealMeetingVideoNotify(char* data, int len, long from);
    //处理创建会议回复
    void dealMeetingCreateRs(char* data, int len, long from);
    //处理加入会议回复
    void dealMeetingJoinRs(char* data, int len, long from);
    //处理新成员加入通知
    void dealMeetingJoinNotify(char* data, int len, long from);
    //处理成员退出通知
    void dealMeetingExitNotify(char* data, int len, long from);
    //处理会议聊天通知
    void dealMeetingChatNotify(char* data, int len, long from);
    //处理会议成员信息
    void dealMeetingMemberInfo(char* data, int len, long from);
    //处理删除好友回复
    void dealDeleteFriendRs(char* data, int len, long from);
    //处理好友状态推送
    void dealFriendStatusNotify(char* data, int len, long from);
    //处理获取好友请求列表回复
    void dealGetFriendRequestsRs(char* data, int len, long from);
    //处理好友请求操作回复
    void dealFriendRequestActionRs(char* data, int len, long from);
signals:
    void sig_showMeetingDialog(int meetingId, bool isCreator);
    void sig_friendRequestCountChanged(int count);
private:
    INetMediator* m_pMediator;
    LoginDialog* m_pLoginDlg;
    FriendList* m_pFriendList;
    QString m_serverIP;
    PFUN m_protocol[_DEF_PROTOCOL_COUNT];
    map<int, FriendItem*> m_mapIdToFriendItem;
    int m_myId;
    QString m_name;
    MeetingDialog* m_pMeetingDlg;
    int m_currentMeetingId;
    QList<_STRU_FRIEND_REQUEST_ITEM> m_pendingRequests; //待处理好友请求列表


public slots:
    void slot_closeProcess();
    void slot_dealData(char* data, int len, long from);
    void slot_registerData(QString name, QString tel, QString pass);
    void slot_loginData(QString tel, QString pass);
    void slot_addFriendItem();
    void slot_meetingCreate();
    void slot_meetingJoin();
    void slot_meetingJoinHistory(QString meetingId);
    void slot_meetingExit();
    void slot_meetingEnd();
    void slot_meetingChat(QString content);
    void slot_meetingAudio(const char* data, int len);
    void slot_meetingVideo(const char* data, int len);
    void slot_deleteFriend(int friendId);
    void slot_showFriendRequests();
    void slot_acceptFriendRequest(int requestId, int fromId, const QString& fromName);
    void slot_refuseFriendRequest(int requestId);
    void slot_fetchPendingRequests();
};

#endif // KERNEL_H
