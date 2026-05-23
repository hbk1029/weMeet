#pragma once
#include<string.h>

/*-------------数据库信息-----------------*/
#define _DEF_DB_NAME                ("wechat_db")
#define _DEF_DB_IP                  ("localhost")
#define _DEF_DB_USER                ("root")
#define _DEF_DB_PWD                 ("Hbk_20041029")
/*--------------------------------------*/

//缓冲区大小
#define _DEF_BUFFER_SIZE            (4096)
//epoll最大长度
#define _DEF_EPOLL_SIZE             (4096)
//线程池阈值
#define _DEF_THREAD_MAX             (100)
#define _DEF_THREAD_MIN             (10)
#define _DEF_QUEUE_MAX              (1000)
//IP地址长度
#define _DEF_IP_SIZE                (16)

//加载库的版本号
#define _DEF_VERSION_LOW            (2)
#define _DEF_VERSION_HIGH           (2)
//服务器IP
#define _DEF_SERVER_IP              ("192.168.74.135")
//服务器端口号
#define _DEF_SERVER_PORT            (8080)
//TCP协议监听队列的最大长度
#define _DEF_TCP_LISTEN_MAX         (128)

//手机号, 昵称, 密码的最大长度
#define _DEF_MAX_LENGTH             (30)
//个性签名最大长度
#define _DEF_FEELING_LENGTH         (100)
//聊天最大长度
#define _DEF_CONTENT_LENGTH         (1024 * 8)
//注册回复
#define _DEF_REGISTER_SUCCESS       (0)
#define _DEF_NAME_EXIST             (1)
#define _DEF_TEL_EXIST              (2)
//登录回复
#define _DEF_LOGIN_SUCCESS          (0)
#define _DEF_TEL_NOT_EXISTS         (1)
#define _DEF_PASSWORD_ERROR         (2)
//添加好友回复
#define _DEF_FRIEND_NOT_EXISTS      (0)
#define _DEF_CANNOT_ADD_SELF        (1)  //不能添加自己
#define _DEF_ALREADY_FRIEND         (2)
#define _DEF_REQUEST_ALREADY_SEND   (3)  //已经发送过请求，请勿重复
#define _DEF_REQUEST_SEND           (4)  //请求已发送，等待对方确认
#define _DEF_ADD_FRIEND_SUCCESS     (5)
#define _DEF_REQUEST_REFUSE         (6)  //对方拒绝了你的好友请求

//聊天回复
#define _DEF_SEND_SUCCESS           (0)
#define _DEF_SEND_FAIL              (1)
//在线状态
#define _DEF_STATUS_OFFLINE         (0)
#define _DEF_STATUS_ONLINE          (1)
#define _DEF_STATUS_SHADOW          (2)

//声明一个结构体类型的变量
typedef int packType;

//声明结构体类型的宏
#define _DEF_PROTOCOL_BASE          (1000)
//注册请求
#define _DEF_REGISTER_RQ            (_DEF_PROTOCOL_BASE + 1)
//注册回复
#define _DEF_REGISTER_RS            (_DEF_PROTOCOL_BASE + 2)
//登录请求
#define _DEF_LOGIN_RQ               (_DEF_PROTOCOL_BASE + 3)
//登录回复
#define _DEF_LOGIN_RS               (_DEF_PROTOCOL_BASE + 4)
//添加好友请求
#define _DEF_ADD_FRIEND_RQ          (_DEF_PROTOCOL_BASE + 5)
//添加好友回复
#define _DEF_ADD_FRIEND_RS          (_DEF_PROTOCOL_BASE + 6)
//聊天请求
#define _DEF_CHAT_RQ                (_DEF_PROTOCOL_BASE + 7)
//聊天回复
#define _DEF_CHAT_RS                (_DEF_PROTOCOL_BASE + 8)
//下线请求
#define _DEF_OFFLINE_RQ             (_DEF_PROTOCOL_BASE + 9)
//用户信息
#define _DEF_FRIEND_INFO            (_DEF_PROTOCOL_BASE + 10)
//好友请求通知
#define _DEF_FRIEND_REQUEST_NOTIFY  (_DEF_PROTOCOL_BASE + 11)

#define _DEF_MEETING_CREATE_RQ      (_DEF_PROTOCOL_BASE + 12)

#define _DEF_MEETING_CREATE_RS      (_DEF_PROTOCOL_BASE + 13)

#define _DEF_MEETING_JOIN_RQ        (_DEF_PROTOCOL_BASE + 14)

#define _DEF_MEETING_JOIN_RS        (_DEF_PROTOCOL_BASE + 15)

#define _DEF_MEETING_EXIT_RQ        (_DEF_PROTOCOL_BASE + 16)

#define _DEF_MEETING_JOIN_NOTIFY    (_DEF_PROTOCOL_BASE + 17)

#define _DEF_MEETING_EXIT_NOTIFY    (_DEF_PROTOCOL_BASE + 18)

#define _DEF_MEETING_CHAT_RQ        (_DEF_PROTOCOL_BASE + 19)

#define _DEF_MEETING_CHAT_NOTIFY    (_DEF_PROTOCOL_BASE + 20)

#define _DEF_MEETING_MEMBER_INFO    (_DEF_PROTOCOL_BASE + 21)
//结构体宏的个数
#define _DEF_MEETING_END_RQ         (_DEF_PROTOCOL_BASE + 27)
#define _DEF_GET_FRIEND_REQUESTS_RQ  (_DEF_PROTOCOL_BASE + 30)
#define _DEF_GET_FRIEND_REQUESTS_RS  (_DEF_PROTOCOL_BASE + 31)
#define _DEF_FRIEND_REQUEST_ACTION_RQ (_DEF_PROTOCOL_BASE + 32)
#define _DEF_FRIEND_REQUEST_ACTION_RS (_DEF_PROTOCOL_BASE + 33)
#define _DEF_PROTOCOL_COUNT         (40)

//结构体声明
//注册请求
typedef struct _STRU_REGISTER_RQ {
    packType type;
    char name[_DEF_MAX_LENGTH];
    char tel[_DEF_MAX_LENGTH];
    char pass[_DEF_MAX_LENGTH];
    _STRU_REGISTER_RQ() : type(_DEF_REGISTER_RQ) {
        memset(name, 0, _DEF_MAX_LENGTH);
        memset(tel, 0, _DEF_MAX_LENGTH);
        memset(pass, 0, _DEF_MAX_LENGTH);
    }
}_STRU_REGISTER_RQ;
//注册回复
typedef struct _STRU_REGISTER_RS {
    packType type;
    int result;
    _STRU_REGISTER_RS() : type(_DEF_REGISTER_RS), result(_DEF_NAME_EXIST) {
    }
}_STRU_REGISTER_RS;
//登录请求
typedef struct _STRU_LOGIN_RQ {
    packType type;
    char tel[_DEF_MAX_LENGTH];
    char pass[_DEF_MAX_LENGTH];
    _STRU_LOGIN_RQ() : type(_DEF_LOGIN_RQ) {
        memset(tel, 0, _DEF_MAX_LENGTH);
        memset(pass, 0, _DEF_MAX_LENGTH);
    }
}_STRU_LOGIN_RQ;
//登录回复
typedef struct _STRU_LOGIN_RS {
    packType type;
    int userId;
    char userName[_DEF_MAX_LENGTH];
    int result;
    _STRU_LOGIN_RS() : type(_DEF_LOGIN_RS), userId(0), result(_DEF_PASSWORD_ERROR) {
        memset(userName, 0, _DEF_MAX_LENGTH);
    }
}_STRU_LOGIN_RS;
//添加好友请求
typedef struct _STRU_ADD_FRIEND_RQ {
    packType type;
    int userId;
    char userName[_DEF_MAX_LENGTH];
    char friendName[_DEF_MAX_LENGTH];
    _STRU_ADD_FRIEND_RQ() : type(_DEF_ADD_FRIEND_RQ), userId(0) {
        memset(userName, 0, _DEF_MAX_LENGTH);
        memset(friendName, 0, _DEF_MAX_LENGTH);
    }
}_STRU_ADD_FRIEND_RQ;
//添加好友回复
typedef struct _STRU_ADD_FRIEND_RS {
    packType type;
    int result;
    int requestId; //请求记录在数据库中的ID（同意/拒绝回传）
    _STRU_ADD_FRIEND_RS() : type(_DEF_ADD_FRIEND_RS), result(_DEF_FRIEND_NOT_EXISTS), requestId(0) {

    }
}_STRU_ADD_FRIEND_RS;
//聊天请求
typedef struct _STRU_CHAT_RQ {
    packType type;
    int userId;
    int friendId;
    char content[_DEF_CONTENT_LENGTH];
    _STRU_CHAT_RQ() : type(_DEF_CHAT_RQ), userId(0), friendId(0) {
        memset(content, 0, _DEF_CONTENT_LENGTH);
    }
}_STRU_CHAT_RQ;
//聊天回复
typedef struct _STRU_CHAT_RS {
    packType type;
    int result;
    int friendId;
    _STRU_CHAT_RS() : type(_DEF_CHAT_RS), result(_DEF_SEND_FAIL), friendId(0) {
    }
}_STRU_CHAT_RS;
//下线请求
typedef struct _STRU_OFFLINE_RQ {
    packType type;
    int userId;
    _STRU_OFFLINE_RQ() : type(_DEF_OFFLINE_RQ), userId(0) {
    }
}_STRU_OFFLINE_RQ;
//用户信息
typedef struct _STRU_FRIEND_INFO {
    packType type;
    int id;
    char name[_DEF_MAX_LENGTH];
    int iconId;
    char feeling[_DEF_FEELING_LENGTH];
    int status;
    _STRU_FRIEND_INFO() :type(_DEF_FRIEND_INFO), id(0), iconId(0), status(0) {
        memset(name, 0, _DEF_MAX_LENGTH);
        memset(feeling, 0, _DEF_FEELING_LENGTH);
    }
}_STRU_FRIEND_INFO;

//好友请求通知（服务器 → 目标用户）
typedef struct _STRU_FRIEND_REQUEST_NOTIFY {
    packType type;
    int fromId;
    char fromName[_DEF_MAX_LENGTH];
    int requestId; //请求记录在数据库中的ID（同意/拒绝回传）
    _STRU_FRIEND_REQUEST_NOTIFY(): type(_DEF_FRIEND_REQUEST_NOTIFY), fromId(0), requestId(0) {
        memset(fromName, 0, sizeof(fromName));
    }
} _STRU_FRIEND_REQUEST_NOTIFY;

// 创建会议请求
typedef struct _STRU_MEETING_CREATE_RQ {
    packType type;
    int userId;
    char userName[_DEF_MAX_LENGTH];
    _STRU_MEETING_CREATE_RQ() : type(_DEF_MEETING_CREATE_RQ), userId(0) {
        memset(userName, 0, _DEF_MAX_LENGTH);
    }
}_STRU_MEETING_CREATE_RQ;

// 创建会议回复
typedef struct _STRU_MEETING_CREATE_RS {
    packType type;
    int meetingId;
    int result;
    _STRU_MEETING_CREATE_RS() : type(_DEF_MEETING_CREATE_RS), meetingId(0), result(0) {
    }
}_STRU_MEETING_CREATE_RS;

// 加入会议请求
typedef struct _STRU_MEETING_JOIN_RQ {
    packType type;
    int userId;
    char userName[_DEF_MAX_LENGTH];
    int meetingId;
    _STRU_MEETING_JOIN_RQ() : type(_DEF_MEETING_JOIN_RQ), userId(0), meetingId(0) {
        memset(userName, 0, _DEF_MAX_LENGTH);
    }
}_STRU_MEETING_JOIN_RQ;

// 加入会议回复
typedef struct _STRU_MEETING_JOIN_RS {
    packType type;
    int meetingId;
    int result;
    int isCreator; // 告知客户端是否为会议主持人
    _STRU_MEETING_JOIN_RS() : type(_DEF_MEETING_JOIN_RS), meetingId(0), result(0), isCreator(0) {
    }
}_STRU_MEETING_JOIN_RS;

// 退出会议请求
typedef struct _STRU_MEETING_EXIT_RQ {
    packType type;
    int userId;
    int meetingId;
    _STRU_MEETING_EXIT_RQ() : type(_DEF_MEETING_EXIT_RQ), userId(0), meetingId(0) {
    }
}_STRU_MEETING_EXIT_RQ;

// 新成员加入通知
typedef struct _STRU_MEETING_JOIN_NOTIFY {
    packType type;
    int meetingId;
    int userId;
    char userName[_DEF_MAX_LENGTH];
    _STRU_MEETING_JOIN_NOTIFY() : type(_DEF_MEETING_JOIN_NOTIFY), meetingId(0), userId(0) {
        memset(userName, 0, _DEF_MAX_LENGTH);
    }
}_STRU_MEETING_JOIN_NOTIFY;

// 成员退出通知
typedef struct _STRU_MEETING_EXIT_NOTIFY {
    packType type;
    int meetingId;
    int userId;
    char userName[_DEF_MAX_LENGTH];
    int isMeetingEnd;
    _STRU_MEETING_EXIT_NOTIFY() : type(_DEF_MEETING_EXIT_NOTIFY), meetingId(0), userId(0), isMeetingEnd(0) {
        memset(userName, 0, _DEF_MAX_LENGTH);
    }
}_STRU_MEETING_EXIT_NOTIFY;

// 会议聊天请求
typedef struct _STRU_MEETING_CHAT_RQ {
    packType type;
    int userId;
    int meetingId;
    char content[_DEF_CONTENT_LENGTH];
    _STRU_MEETING_CHAT_RQ() : type(_DEF_MEETING_CHAT_RQ), userId(0), meetingId(0) {
        memset(content, 0, _DEF_CONTENT_LENGTH);
    }
}_STRU_MEETING_CHAT_RQ;

// 会议聊天广播
typedef struct _STRU_MEETING_CHAT_NOTIFY {
    packType type;
    int userId;
    char userName[_DEF_MAX_LENGTH];
    int meetingId;
    char content[_DEF_CONTENT_LENGTH];
    _STRU_MEETING_CHAT_NOTIFY() : type(_DEF_MEETING_CHAT_NOTIFY), userId(0), meetingId(0) {
        memset(userName, 0, _DEF_MAX_LENGTH);
        memset(content, 0, _DEF_CONTENT_LENGTH);
    }
}_STRU_MEETING_CHAT_NOTIFY;

// 会议成员信息
typedef struct _STRU_MEETING_MEMBER_INFO {
    packType type;
    int meetingId;
    int userId;
    char userName[_DEF_MAX_LENGTH];
    _STRU_MEETING_MEMBER_INFO() : type(_DEF_MEETING_MEMBER_INFO), meetingId(0), userId(0) {
        memset(userName, 0, _DEF_MAX_LENGTH);
    }
}_STRU_MEETING_MEMBER_INFO;

// --- 音频协议 ---
#define _DEF_MEETING_AUDIO_RQ        (_DEF_PROTOCOL_BASE + 22)

typedef struct _STRU_MEETING_AUDIO_RQ {
    packType type;
    int userId;
    int meetingId;
    int dataLen;
    char data[200];
    _STRU_MEETING_AUDIO_RQ() : type(_DEF_MEETING_AUDIO_RQ), userId(0),
                               meetingId(0), dataLen(0) {
        memset(data, 0, sizeof(data));
    }
}_STRU_MEETING_AUDIO_RQ;

// --- 视频协议 ---
#define _DEF_MEETING_VIDEO_RQ        (_DEF_PROTOCOL_BASE + 23)
// --- 删除好友 ---
#define _DEF_DELETE_FRIEND_RQ        (_DEF_PROTOCOL_BASE + 24)
#define _DEF_DELETE_FRIEND_RS        (_DEF_PROTOCOL_BASE + 25)
// --- 好友状态推送 ---
#define _DEF_FRIEND_STATUS_NOTIFY    (_DEF_PROTOCOL_BASE + 26)

typedef struct _STRU_MEETING_VIDEO_RQ {
    packType type;
    int userId;
    int meetingId;
    int dataLen;
    char data[32768];
    _STRU_MEETING_VIDEO_RQ() : type(_DEF_MEETING_VIDEO_RQ), userId(0),
                               meetingId(0), dataLen(0) {
        memset(data, 0, sizeof(data));
    }
}_STRU_MEETING_VIDEO_RQ;
//删除好友请求
typedef struct _STRU_DELETE_FRIEND_RQ {
    packType type;
    int userId;
    int friendId;
    _STRU_DELETE_FRIEND_RQ() : type(_DEF_DELETE_FRIEND_RQ), userId(0), friendId(0) {}
}_STRU_DELETE_FRIEND_RQ;
//删除好友回复
typedef struct _STRU_DELETE_FRIEND_RS {
    packType type;
    int result;
    int friendId;
    _STRU_DELETE_FRIEND_RS() : type(_DEF_DELETE_FRIEND_RS), result(1), friendId(0) {}
}_STRU_DELETE_FRIEND_RS;
//好友状态推送
typedef struct _STRU_FRIEND_STATUS_NOTIFY {
    packType type;
    int userId;
    int status;
    _STRU_FRIEND_STATUS_NOTIFY() : type(_DEF_FRIEND_STATUS_NOTIFY), userId(0), status(0) {}
}_STRU_FRIEND_STATUS_NOTIFY;


//获取好友请求列表请求
typedef struct _STRU_GET_FRIEND_REQUESTS_RQ {
    packType type;
    int userId;
    _STRU_GET_FRIEND_REQUESTS_RQ() : type(_DEF_GET_FRIEND_REQUESTS_RQ), userId(0) {
    }
}_STRU_GET_FRIEND_REQUESTS_RQ;

//单条好友请求信息
typedef struct _STRU_FRIEND_REQUEST_ITEM {
    int requestId;
    int fromId;
    char fromName[_DEF_MAX_LENGTH];
    _STRU_FRIEND_REQUEST_ITEM() : requestId(0), fromId(0) {
        memset(fromName, 0, _DEF_MAX_LENGTH);
    }
}_STRU_FRIEND_REQUEST_ITEM;

//获取好友请求列表回复
#define _DEF_MAX_FRIEND_REQUESTS (50)
typedef struct _STRU_GET_FRIEND_REQUESTS_RS {
    packType type;
    int count;
    _STRU_FRIEND_REQUEST_ITEM items[_DEF_MAX_FRIEND_REQUESTS];
    _STRU_GET_FRIEND_REQUESTS_RS() : type(_DEF_GET_FRIEND_REQUESTS_RS), count(0) {
        memset(items, 0, sizeof(items));
    }
}_STRU_GET_FRIEND_REQUESTS_RS;

//好友请求操作（同意/拒绝）
#define _DEF_REQUEST_ACCEPT          (1)
#define _DEF_REQUEST_REFUSE_ACTION   (2)
typedef struct _STRU_FRIEND_REQUEST_ACTION_RQ {
    packType type;
    int userId;
    int requestId;
    int action;
    char friendName[_DEF_MAX_LENGTH];
    _STRU_FRIEND_REQUEST_ACTION_RQ() : type(_DEF_FRIEND_REQUEST_ACTION_RQ),
                                       userId(0), requestId(0), action(0) {
        memset(friendName, 0, _DEF_MAX_LENGTH);
    }
}_STRU_FRIEND_REQUEST_ACTION_RQ;

//好友请求操作回复
typedef struct _STRU_FRIEND_REQUEST_ACTION_RS {
    packType type;
    int requestId;
    int result;
    int action;
    _STRU_FRIEND_REQUEST_ACTION_RS() : type(_DEF_FRIEND_REQUEST_ACTION_RS),
                                       requestId(0), result(1), action(0) {
    }
}_STRU_FRIEND_REQUEST_ACTION_RS;
