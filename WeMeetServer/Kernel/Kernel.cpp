#include"Kernel.h"
#include"Net/def.h"
#include"../common.h"
#include<list>
#include<algorithm>
#include"../Mediator/TCPServerMediator.h"
#include "bcrypt/BCrypt.hpp"

Kernel* Kernel::m_pKernel = nullptr;

Kernel::Kernel(): m_pMediator(nullptr) {
    m_pKernel = this;
    setProtocol();
}
Kernel::~Kernel() {
    endServer();
}

Kernel *Kernel::getInstance()
{
    static Kernel kernel;
    return &kernel;
}

void Kernel::setProtocol() {
    //先初始化为0
    memset(m_protocol, 0, sizeof(m_protocol));
    //存储协议
    m_protocol[_DEF_REGISTER_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealRegisterRq;
    m_protocol[_DEF_LOGIN_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealLoginRq;
    m_protocol[_DEF_ADD_FRIEND_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealAddFriendRq;
    m_protocol[_DEF_ADD_FRIEND_RS - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealAddFriendRs;
    //会议相关协议
    m_protocol[_DEF_MEETING_CREATE_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealMeetingCreateRq;
    m_protocol[_DEF_MEETING_JOIN_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealMeetingJoinRq;
    m_protocol[_DEF_MEETING_EXIT_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealMeetingExitRq;
    m_protocol[_DEF_MEETING_CHAT_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealMeetingChatRq;
    m_protocol[_DEF_MEETING_AUDIO_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealMeetingAudioRq;
    m_protocol[_DEF_MEETING_VIDEO_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealMeetingVideoRq;
    m_protocol[_DEF_MEETING_END_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealMeetingEndRq;
    m_protocol[_DEF_GET_FRIEND_REQUESTS_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealGetFriendRequestsRq;
    m_protocol[_DEF_FRIEND_REQUEST_ACTION_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealFriendRequestActionRq;
    m_protocol[_DEF_DELETE_FRIEND_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealDeleteFriendRq;
    //m_protocol[_DEF_CHAT_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealChatRq;
    //m_protocol[_DEF_OFFLINE_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealOfflineRq;
}

bool Kernel::startServer() {
    //打开网络
    m_pMediator = new TCPServerMediator;
    if (!m_pMediator->openNet()) {
        cout << "打开TCP服务端失败" << endl;
        return false;
    }
    //连接服务器
    char host[] = _DEF_DB_IP;
    char user[] = _DEF_DB_USER;
    char pass[] = _DEF_DB_PWD;
    char db[] = _DEF_DB_NAME;

    if (!m_sql.ConnectMysql(host, user, pass, db)) {
        cout << "打开数据库失败" << endl;
        return false;
    }
    return true;
}

void Kernel::runServer()
{
    if(m_pMediator) {
        m_pMediator->run();
    }
}
void Kernel::endServer() {
    if (m_pMediator) {
        //关闭网络
        m_pMediator->closeNet();
        //回收资源
        delete m_pMediator;
        m_pMediator = nullptr;
    }
    //断开与数据库的连接
    m_sql.DisConnect();
}



void Kernel::dealData(char* data, int len, int sockfd) {

    cout << __func__ << endl;
    //取出协议头
    packType type = *(packType*)data;
    //根据协议头判断是哪一个结构体
    int index = type - _DEF_PROTOCOL_BASE - 1;
    if (index >= 0 && index < _DEF_PROTOCOL_COUNT) {
        PFUN pf = m_protocol[index];
        //调用不同的处理函数
        if (pf) {
            (m_pKernel->*pf)(data, len, sockfd);
        }
        else {
            //错误原因:
            //1.定义结构体时, type值写错了
            //2.对端发送的结构体错误
            cout << "type2:" << type << endl;
        }
    }
    else {
        //错误原因
        //1.声明结构体时, packType没有放在第一位
        //2.接收数据时, offset没清零
        cout << "type1:" << type << endl;
    }
}
//处理注册请求
void Kernel::dealRegisterRq(char* data, int len, int sockfd) {
    cout << __func__ << endl;
    //拆包
    _STRU_REGISTER_RQ* rq = (_STRU_REGISTER_RQ*)data;
    cout << "name:" << rq->name << " tel:" << rq->tel  << " pass:" << rq->pass << endl;
    //校验电话号是否被注册
    _STRU_REGISTER_RS rs;
    char sql[1024] = "";
    sprintf(sql, "select tel from t_user where tel = '%s';", rq->tel);
    list<string> lstStr;
    if (!m_sql.SelectMysql(sql, 1, lstStr)) {
        cout << "查询数据库失败" << sql << endl;
        return;
    }
    //判断查询结果是否为空
    if (!lstStr.empty()) {
        //电话号已存在, 返回注册的错误信息
        rs.result = _DEF_TEL_EXIST;
        m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
        return;
    }
    //校验昵称是否被注册
    sprintf(sql, "select name from t_user where name = '%s';", rq->name);
    if (!m_sql.SelectMysql(sql, 1, lstStr)) {
        cout << "查询数据库失败" << sql << endl;
        return;
    }
    if (!lstStr.empty()) {
        //电话号已存在, 返回注册的错误信息
        rs.result = _DEF_NAME_EXIST;
        m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
        return;
    }
    //加密密码
    string password_hash = BCrypt::generateHash(rq->pass, 12);
    //把注册数据插入数据库
    sprintf(sql, "insert into t_user (name, tel, password_hash, iconid, feeling) values ('%s', '%s', '%s', '8', '好好学习');",
        rq->name, rq->tel, password_hash.c_str());
    if (!m_sql.UpdateMysql(sql)) {
        //原因: 没连接数据库, 或者sql语句错误
        cout << "插入数据库失败:" << sql << endl;
        return;
    }
    //注册成功
    rs.result = _DEF_REGISTER_SUCCESS;
    m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
}
//处理登录请求
void Kernel::dealLoginRq(char* data, int len, int sockfd) {
    cout << __func__ << endl;
    //拆包
    _STRU_LOGIN_RQ* rq = (_STRU_LOGIN_RQ*)data;
    cout << "tel:" << rq->tel << " password:" << rq->pass << endl;
    //校验
    //电话号是否存在
    char sql[1024] = "";
    sprintf(sql, "select password_hash, id, name from t_user where tel = '%s';", rq->tel);
    list<string> lstStr;
    if (!m_sql.SelectMysql(sql, 3, lstStr)) {
        //没连接数据库或有语法错误
        cout << "查询数据库失败:" << sql << endl;
        return;
    }
    //如果查不到, 说明手机号不存在
    _STRU_LOGIN_RS rs;
    if (lstStr.empty()) {
        rs.result = _DEF_TEL_NOT_EXISTS;
        m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
    }
    //密码是否正确 (结果链表第一个)
    else {
        string password_hash = lstStr.front(); //取出密码（哈希值）
        lstStr.pop_front();
        int userId = stoi(lstStr.front());
        lstStr.pop_front();
        string name = lstStr.front();
        lstStr.pop_front();
        //验证密码
        if (!BCrypt::validatePassword(rq->pass, password_hash)) {
            rs.result = _DEF_PASSWORD_ERROR;
            m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
            return;
        }
        //密码相同, 登录成功
        else {
            rs.result = _DEF_LOGIN_SUCCESS;
            rs.userId = userId;
            strcpy(rs.userName, name.c_str());
            //把当前用户的id和sockfd进行绑定
            std::lock_guard<std::mutex> lock(m_mutex);
            m_mapIdToSockfd[userId] = sockfd;
            //更新用户在线状态到数据库
            memset(sql, 0, sizeof(sql));
            sprintf(sql, "update t_user set status = %d where id = %d;", _DEF_STATUS_ONLINE, userId);
            m_sql.UpdateMysql(sql);
            //发登录回复
            m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
            //获取当前登录用户的信息以及好友的信息, 并发送给客户端
            getUserInfoAndFriendInfo(userId);
        }
        //锁外: 通知所有在线好友: 我上线了
        broadcastFriendStatus(userId, _DEF_STATUS_ONLINE);
    }
}

void Kernel::dealAddFriendRq(char* data, int len, int sockfd) {
    _STRU_ADD_FRIEND_RQ* rq = (_STRU_ADD_FRIEND_RQ*)data;
    cout << rq->userId << " " << rq->userName << " " << rq->friendName << endl;
    _STRU_ADD_FRIEND_RS rs;

    //校验请求者是否存在
    if(m_mapIdToSockfd.count(rq->userId) == 0) {
        cout << "userId not exist" << endl;
        return;
    }
    //根据昵称查找目标用户
    char sql[1024] = "";
    sprintf(sql, "select id, status from t_user where name = '%s' or tel = '%s';", rq->friendName, rq->friendName);
    list<string> lstStr;
    if (!m_sql.SelectMysql(sql, 2, lstStr)) {
        cout << "查询数据库失败:" << sql << endl;
        return;
    }
    if(lstStr.empty()) {
        //用户不存在
        rs.result = _DEF_FRIEND_NOT_EXISTS;
        //回复客户端
        m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
        return;
    }
    //校验是否已经是好友(避免重复添加)
    int target_id = stoi(lstStr.front());
    int target_status = stoi(lstStr.front());
    //不能添加自己
    if (target_id == rq->userId) {
        rs.result = _DEF_CANNOT_ADD_SELF;
        m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
        return;
    }
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select user_id from t_friend where (user_id = %d and friend_id = %d) or (user_id = %d and friend_id = %d);",
            rq->userId, target_id, target_id, rq->userId);
    lstStr.clear();  //清空 lstStr，防止残留数据
    if(!m_sql.SelectMysql(sql, 1, lstStr)) {
        cout << "查询数据库失败:" << sql << endl;
        return;
    }
    if(!lstStr.empty()) {
        //已经是好友了
        rs.result = _DEF_ALREADY_FRIEND;
        m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
        return;
    }
    //检查是否有待处理的相同请求
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select id from t_friend_requests where from_user_id = %d and to_user_id = %d and status = 0;",
            rq->userId, target_id);
    lstStr.clear();  //清空 lstStr，防止残留数据
    if(!m_sql.SelectMysql(sql, 1, lstStr)) {
        cout << "查询数据库失败:" << sql << endl;
        return;
    }
    if(!lstStr.empty()) {
        //有相同记录
        rs.result = _DEF_REQUEST_ALREADY_SEND;
        lstStr.pop_front();
        m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
        return;
    }
    //检查对方是否已经向我发过好友请求, 如果有, 直接建立好友关系
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select id from t_friend_requests where from_user_id = %d and to_user_id = %d and status = 0;",
            target_id, rq->userId);
    if (!m_sql.SelectMysql(sql, 1, lstStr)) {
        cout << "查询数据库失败:" << sql << endl;
        return;
    }
    if(!lstStr.empty()) {
        //存在反向请求，视为双方同意，直接建立好友关系
        sprintf(sql, "update t_friend_requests set status = 1 where from_user_id = %d and to_user_id = %d and status = 0;",
                target_id, rq->userId);
        if(!m_sql.UpdateMysql(sql)) {
            cout << "更新数据库失败:" << sql << endl;
            return;
        }
        //插入好友关系
        sprintf(sql, "insert into t_friend (user_id, friend_id) values (%d, %d), (%d, %d);",
                rq->userId, target_id, target_id, rq->userId);
        if(!m_sql.UpdateMysql(sql)) {
            cout << "插入数据库失败:" << sql << endl;
            return;
        }
        //删除请求消息
        memset(sql, 0, sizeof(sql));
        int requestId = stoi(lstStr.front());
        sprintf(sql, "delete from t_friend_requests where id = %d;", requestId);
        if(!m_sql.UpdateMysql(sql)) {
            cout << "删除失败" << endl;
            return;
        }
        //通知双方添加成功
        _STRU_FRIEND_INFO infoA, infoB;
        getInfoById(rq->userId, &infoA);
        getInfoById(target_id, &infoB);
        if (m_mapIdToSockfd.count(rq->userId))
            m_pMediator->sendData((char*)&infoB, sizeof(infoB), m_mapIdToSockfd[rq->userId]);
        if (m_mapIdToSockfd.count(target_id))
            m_pMediator->sendData((char*)&infoA, sizeof(infoA), m_mapIdToSockfd[target_id]);
        return;
    }
    //校验通过，插入好友请求记录
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "insert into t_friend_requests (from_user_id, to_user_id) values (%d, %d);", rq->userId, target_id);
    if(!m_sql.UpdateMysql(sql)) {
        cout << "插入数据库失败:" << sql << endl;
        return;
    }
    //回复请求者: 请求已发送，等待对方确认
    rs.result = _DEF_REQUEST_SEND;
    m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);

    //判断对方当前是否在线
    if(target_status == _DEF_STATUS_OFFLINE) {
        //不在线，数据库已保存，无需额外操作
    }
    else {
        //在线/隐身，直接发送好友请求通知
        if (m_mapIdToSockfd.count(target_id) > 0) {
            //查询请求记录的ID
            memset(sql, 0, sizeof(sql));
            sprintf(sql, "select id from t_friend_requests where from_user_id = %d and to_user_id = %d and status = 0;", rq->userId, target_id);
            list<string> lstStr;
    lstStr.clear();  //清空 lstStr，防止残留数据
            if(!m_sql.SelectMysql(sql, 1, lstStr)) {
                cout << "查询数据库失败:" << sql << endl;
                return;
            }
            if(!lstStr.empty()) {
                int requestId = stoi(lstStr.front());
                _STRU_FRIEND_REQUEST_NOTIFY notify;
                notify.requestId = requestId;
                notify.fromId = rq->userId;
                strcpy(notify.fromName, rq->userName);
                m_pMediator->sendData((char*)&notify, sizeof(notify), m_mapIdToSockfd[target_id]);
            }
        }
    }
}

void Kernel::dealAddFriendRs(char *data, int len, int sockfd) {
    //说明对方拒绝添加好友，需要删除数据库的对应记录
    _STRU_ADD_FRIEND_RS* rs = (_STRU_ADD_FRIEND_RS*)data;
    cout << rs->requestId << endl;
    if(rs->result == _DEF_REQUEST_REFUSE) {
        char sql[1024] = "";
        sprintf(sql, "delete from t_friend_requests where id = %d;", rs->requestId);
        if(!m_sql.UpdateMysql(sql)) {
            cout << "删除失败" << endl;
            return;
        }
        //不需要通知对方
    }
}

////处理聊天请求
//void Kernel::dealChatRq(char* data, int len, int sockfd) {
//    cout << __func__ << endl;
//    //拆包
//    _STRU_CHAT_RQ* rq = (_STRU_CHAT_RQ*)data;
//    //判断B客户端是否在线
//    if (m_mapIdToSocket.count(rq->friendId) > 0) {
//        //在线, 把数据转发给B客户端
//        m_pMediator->sendData(data, len, m_mapIdToSocket[rq->friendId]);
//    }
//    else {
//        //不在线, 给A客户端发一个聊天失败回复
//        _STRU_CHAT_RS rs;
//        rs.friendId = rq->friendId;
//        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
//    }
//}
//void Kernel::dealOfflineRq(char* data, int len, int sockfd) {
//    cout << __func__ << endl;
//    //拆包
//    _STRU_OFFLINE_RQ* rq = (_STRU_OFFLINE_RQ*)data;
//    //根据id取出好友id
//    char sql[1024] = "";
//    sprintf(sql, "select idB from t_friend where idA = %d;", rq->userId);
//    list<string> lstStr;
//    if (!m_sql.SelectMysql(sql, 1, lstStr)) {
//        cout << "查询数据库失败:" << sql << endl;
//        return;
//    }
//    //遍历好友id列表
//    while (!lstStr.empty()) {
//        //每次取出1个好友id
//        int friendId = stoi(lstStr.front());
//        lstStr.pop_front();
//        //判断好友是否在线
//        if (m_mapIdToSocket.count(friendId) > 0) {
//            //在线, 把当前下线请求转发给他
//            m_pMediator->sendData(data, len, m_mapIdToSocket[friendId]);
//        }
//        //关闭下线用户的socket并回收map空间
//        auto it = m_mapIdToSocket.find(friendId);
//        if (it != m_mapIdToSocket.end()) {
//            closesocket(it->second);
//        }
//        m_mapIdToSocket.erase(friendId);
//    }
//}
//删除好友请求
void Kernel::dealDeleteFriendRq(char* data, int len, int sockfd) {
    (void)len;
    _STRU_DELETE_FRIEND_RQ* rq = (_STRU_DELETE_FRIEND_RQ*)data;
    cout << "dealDeleteFriendRq userId=" << rq->userId << " friendId=" << rq->friendId << endl;
    _STRU_DELETE_FRIEND_RS rs;
    rs.friendId = rq->friendId;
    // 同时清理好友请求记录
    char sql[512] = "";
    sprintf(sql, "delete from t_friend_requests where (from_user_id = %d and to_user_id = %d) or (from_user_id = %d and to_user_id = %d);",
            rq->userId, rq->friendId, rq->friendId, rq->userId);
    m_sql.UpdateMysql(sql);
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select id from t_friend where (user_id = %d and friend_id = %d) or (user_id = %d and friend_id = %d);",
            rq->userId, rq->friendId, rq->friendId, rq->userId);
    list<string> lstStr;
    if (!m_sql.SelectMysql(sql, 1, lstStr) || lstStr.empty()) {
        rs.result = 1;
        if (m_mapIdToSockfd.count(rq->userId) > 0)
            m_pMediator->sendData((char*)&rs, sizeof(rs), m_mapIdToSockfd[rq->userId]);
        return;
    }
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "delete from t_friend where (user_id = %d and friend_id = %d) or (user_id = %d and friend_id = %d);",
            rq->userId, rq->friendId, rq->friendId, rq->userId);
    rs.result = m_sql.UpdateMysql(sql) ? 0 : 1;
    if (m_mapIdToSockfd.count(rq->userId) > 0)
        m_pMediator->sendData((char*)&rs, sizeof(rs), m_mapIdToSockfd[rq->userId]);
    if (m_mapIdToSockfd.count(rq->friendId) > 0) {
        _STRU_DELETE_FRIEND_RS rs2;
        rs2.result = 0;
        rs2.friendId = rq->userId;
        m_pMediator->sendData((char*)&rs2, sizeof(rs2), m_mapIdToSockfd[rq->friendId]);
    }
}

//向在线好友推送状态变更
void Kernel::broadcastFriendStatus(int userId, int status) {
    char sql[512] = "";
    sprintf(sql, "select friend_id from t_friend where user_id = %d;", userId);
    list<string> lstStr;
    if (!m_sql.SelectMysql(sql, 1, lstStr)) return;
    _STRU_FRIEND_STATUS_NOTIFY notify;
    notify.userId = userId;
    notify.status = status;
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!lstStr.empty()) {
        int friendId = stoi(lstStr.front());
        lstStr.pop_front();
        if (m_mapIdToSockfd.count(friendId) > 0)
            m_pMediator->sendData((char*)&notify, sizeof(notify), m_mapIdToSockfd[friendId]);
    }
}

void Kernel::getUserInfoAndFriendInfo(int userId) {
    //根据自己id获取自己信息
    _STRU_FRIEND_INFO userInfo;
    getInfoById(userId, &userInfo);
    //把用户信息发给客户端
    if (m_mapIdToSockfd.count(userId) > 0) {
        m_pMediator->sendData((char*)&userInfo, sizeof(userInfo), m_mapIdToSockfd[userId]);
    }
    else {
        cout << "map里没有这个键值:" << userId << endl;
        return;
    }
    //根据用户id获取朋友id, 存到朋友列表里
    char sql[1024] = "";
    sprintf(sql, "select friend_id from t_friend where user_id = %d;", userId);
    list<string> lstStr;
    if (!m_sql.SelectMysql(sql, 1, lstStr)) {
        cout << "查询数据库失败:" << sql << endl;
        return;
    }
    //遍历结果链表
    while (!lstStr.empty()) {
        //每次取出一个朋友id
        int friendId = stoi(lstStr.front());
        lstStr.pop_front();
        //根据朋友id获取朋友信息
        _STRU_FRIEND_INFO friendInfo;
        getInfoById(friendId, &friendInfo);
        //判断是否在线, 如果在线, 需要把自己的信息发给朋友
        if (_DEF_STATUS_ONLINE == friendInfo.status) {
            m_pMediator->sendData((char*)&userInfo, sizeof(userInfo), m_mapIdToSockfd[friendId]);
        }
        //把朋友信息发给客户端
        m_pMediator->sendData((char*)&friendInfo, sizeof(friendInfo), m_mapIdToSockfd[userId]);
    }
    //查询friend_request表，检查是否有未处理的请求
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select rq.id, rq.from_user_id, usr.name from t_friend_requests rq inner join t_user usr on rq.from_user_id = usr.id where rq.to_user_id = %d and rq.status = 0;",
            userId);
    //查询失败
    lstStr.clear();  //清空 lstStr，防止残留数据
    if(!m_sql.SelectMysql(sql, 3, lstStr)) {
        cout << "查询数据库失败:" << sql << endl;
        return;
    }
    //遍历所有待处理请求
    while(!lstStr.empty()) {
        //查询成功，取出id，from_id, from_name
        int rq_id = stoi(lstStr.front());
        lstStr.pop_front();
        int from_id = stoi(lstStr.front());
        lstStr.pop_front();
        string from_name = lstStr.front();
        lstStr.pop_front();
        //构造通知包
        _STRU_FRIEND_REQUEST_NOTIFY notify;
        notify.requestId = rq_id;
        notify.fromId = from_id;
        strcpy(notify.fromName, from_name.c_str());
        //发送给客户端
        if(m_mapIdToSockfd.count(userId) > 0) {
            m_pMediator->sendData((char*)&notify, sizeof(notify), m_mapIdToSockfd[userId]);
        }
    }
}

void Kernel::getInfoById(int userId, _STRU_FRIEND_INFO* info) {
    //获取id
    info->id = userId;
    //跟据id查询name, iconId, feeling，status
    char sql[1024] = "";
    sprintf(sql, "select name, iconId, feeling, status from t_user where id = %d;", userId);
    list<string> lstStr;
    m_sql.SelectMysql(sql, 4, lstStr);
    if (lstStr.size() != 4) {
        cout << "查询昵称, 签名, 头像id, 在线状态错误:" << sql << endl;
        return;
    }
    else {
        //取出查询结果
        string name = lstStr.front();
        lstStr.pop_front();
        int iconId = stoi(lstStr.front());
        lstStr.pop_front();
        string feeling = lstStr.front();
        lstStr.pop_front();
        int status = stoi(lstStr.front());
        lstStr.pop_front();
        //赋值给结构体
        strcpy(info->name, name.c_str());
        info->iconId = iconId;
        strcpy(info->feeling, feeling.c_str());
        //在线状态判断
        bool is_connect = (m_mapIdToSockfd.count(userId) > 0);
        if (status == _DEF_STATUS_SHADOW) {
            //隐身：无论Socket是否连接，对外都显示离线
            info->status = _DEF_STATUS_OFFLINE;
        } else if (status == _DEF_STATUS_ONLINE && is_connect) {
            //在线且有Socket连接
            info->status = _DEF_STATUS_ONLINE;
        } else {
            //其他情况：离线
            info->status = _DEF_STATUS_OFFLINE;
        }
    }
}

// 生成9位不重复会议号
int Kernel::generateMeetingCode() {
    int code;
    char sql[256] = "";
    list<string> lstStr;
    do {
        code = 100000000 + rand() % 900000000;
        memset(sql, 0, sizeof(sql));
        sprintf(sql, "select id from t_meeting where meeting_code = %d;", code);
        list<string> lstStr;
    lstStr.clear();
        if (!m_sql.SelectMysql(sql, 1, lstStr)) {
            continue;
        }
    } while (!lstStr.empty());
    return code;
}

// 广播给会议全体在线成员
void Kernel::broadcastToMeeting(int meetingCode, char* data, int len, int excludeSockfd) {
    std::set<int> targetFds;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_mapMeetingMembers.count(meetingCode) == 0) return;
        std::cout << "BROADCAST meeting=" << meetingCode << " members=[";
        for (int userId : m_mapMeetingMembers[meetingCode]) {
            std::cout << userId;
            if (m_mapIdToSockfd.count(userId) > 0) {
                int fd = m_mapIdToSockfd[userId];
                std::cout << "(fd=" << fd << ")";
                if (fd != excludeSockfd) {
                    auto r = targetFds.insert(fd);
                    if (!r.second) std::cout << "!DUP!";
                }
            }
            std::cout << " ";
        }
        std::cout << "] count=" << targetFds.size() << std::endl;
    }
    for (int fd : targetFds) {
        std::cout << "SEND fd=" << fd << std::endl;
        m_pMediator->sendData(data, len, fd);
    }
}
void Kernel::dealMeetingCreateRq(char* data, int len, int sockfd) {
    cout << __func__ << endl;
    _STRU_MEETING_CREATE_RQ* rq = (_STRU_MEETING_CREATE_RQ*)data;
    _STRU_MEETING_CREATE_RS rs;

    int meetingCode = generateMeetingCode();
    char sql[512] = "";
    sprintf(sql, "insert into t_meeting (meeting_code, creator_id, creator_name) values (%d, %d, '%s');",
            meetingCode, rq->userId, rq->userName);
    if (!m_sql.UpdateMysql(sql)) {
        rs.result = 1;
        m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
        return;
    }

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "insert into t_meeting_member (meeting_id, user_id, user_name) values (%d, %d, '%s');",
            meetingCode, rq->userId, rq->userName);
    if (!m_sql.UpdateMysql(sql)) {
        rs.result = 1;
        m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
        return;
    }

    rs.meetingId = meetingCode;
    rs.result = 0;
    m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);

    // 将创建者加入内存，并给自己发成员信息
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& v = m_mapMeetingMembers[meetingCode];
        if (std::find(v.begin(), v.end(), rq->userId) == v.end())
            v.push_back(rq->userId);
    }
    _STRU_MEETING_MEMBER_INFO memberInfo;
    memberInfo.meetingId = meetingCode;
    memberInfo.userId = rq->userId;
    strcpy(memberInfo.userName, rq->userName);
    m_pMediator->sendData((char*)&memberInfo, sizeof(memberInfo), sockfd);
}

// 处理加入会议请求
void Kernel::dealMeetingJoinRq(char* data, int len, int sockfd) {
    cout << __func__ << endl;
    _STRU_MEETING_JOIN_RQ* rq = (_STRU_MEETING_JOIN_RQ*)data;
    _STRU_MEETING_JOIN_RS rs;
    rs.meetingId = rq->meetingId;

    char sql[512] = "";
    sprintf(sql, "select status from t_meeting where meeting_code = %d;", rq->meetingId);
    list<string> lstStr;
    if (!m_sql.SelectMysql(sql, 1, lstStr) || lstStr.empty()) {
        rs.result = 1;
        m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
        return;
    }
    int status = stoi(lstStr.front());
    if (status != 1) {
        rs.result = 2;
        m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
        return;
    }

    bool isRejoin = false;
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select id from t_meeting_member where meeting_id = %d and user_id = %d;",
            rq->meetingId, rq->userId);
    lstStr.clear();
    if (m_sql.SelectMysql(sql, 1, lstStr) && !lstStr.empty()) {
        isRejoin = true;
    }
    else {
        memset(sql, 0, sizeof(sql));
        sprintf(sql, "insert into t_meeting_member (meeting_id, user_id, user_name) values (%d, %d, '%s');",
                rq->meetingId, rq->userId, rq->userName);
        if (!m_sql.UpdateMysql(sql)) {
            rs.result = 1;
            m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
            return;
        }
    }

    rs.result = 0;
    m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select user_id, user_name from t_meeting_member where meeting_id = %d;",
            rq->meetingId);
    lstStr.clear();
    if (m_sql.SelectMysql(sql, 2, lstStr)) {
        while (!lstStr.empty()) {
            int memberId = stoi(lstStr.front());
            lstStr.pop_front();
            string memberName = lstStr.front();
            lstStr.pop_front();
            _STRU_MEETING_MEMBER_INFO memberInfo;
            memberInfo.meetingId = rq->meetingId;
            memberInfo.userId = memberId;
            strcpy(memberInfo.userName, memberName.c_str());
            m_pMediator->sendData((char*)&memberInfo, sizeof(memberInfo), sockfd);
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& v = m_mapMeetingMembers[rq->meetingId];
        if (std::find(v.begin(), v.end(), rq->userId) == v.end())
            v.push_back(rq->userId);
    }
    if (!isRejoin) {
        _STRU_MEETING_JOIN_NOTIFY notify;
        notify.meetingId = rq->meetingId;
        notify.userId = rq->userId;
        strcpy(notify.userName, rq->userName);
        broadcastToMeeting(rq->meetingId, (char*)&notify, sizeof(notify), sockfd);
    }
}

// 处理退出会议请求
void Kernel::dealMeetingExitRq(char* data, int len, int sockfd) {
    cout << __func__ << endl;
    _STRU_MEETING_EXIT_RQ* rq = (_STRU_MEETING_EXIT_RQ*)data;

    char sql[256] = "";
    sprintf(sql, "delete from t_meeting_member where meeting_id = %d and user_id = %d;",
            rq->meetingId, rq->userId);
    m_sql.UpdateMysql(sql);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_mapMeetingMembers.count(rq->meetingId) > 0) {
            auto& vec = m_mapMeetingMembers[rq->meetingId];
            vec.erase(std::remove(vec.begin(), vec.end(), rq->userId), vec.end());
        }
    }

    _STRU_MEETING_EXIT_NOTIFY notify;
    notify.meetingId = rq->meetingId;
    notify.userId = rq->userId;
    notify.isMeetingEnd = 0;
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select name from t_user where id = %d;", rq->userId);
    list<string> lstStr;
    if (m_sql.SelectMysql(sql, 1, lstStr) && !lstStr.empty()) {
        strcpy(notify.userName, lstStr.front().c_str());
    } else {
        memset(notify.userName, 0, _DEF_MAX_LENGTH);
    }
    broadcastToMeeting(rq->meetingId, (char*)&notify, sizeof(notify), -1);
}

// 处理结束会议请求（仅主持人）
void Kernel::dealMeetingEndRq(char* data, int len, int sockfd) {
    cout << __func__ << endl;
    _STRU_MEETING_EXIT_RQ* rq = (_STRU_MEETING_EXIT_RQ*)data;

    // 验证是否为主持人
    bool isCreator = false;
    {
        char csql[256] = "";
        sprintf(csql, "select creator_id from t_meeting where meeting_code = %d;", rq->meetingId);
        list<string> clst;
        if (m_sql.SelectMysql(csql, 1, clst) && !clst.empty()) {
            isCreator = (stoi(clst.front()) == rq->userId);
        }
    }
    if (!isCreator) return;

    // 标记会议结束
    {
        char sql[256] = "";
        sprintf(sql, "update t_meeting set status = 0 where meeting_code = %d;", rq->meetingId);
        m_sql.UpdateMysql(sql);
    }

    // 获取用户名
    char userName[_DEF_MAX_LENGTH] = {0};
    {
        char sql[256] = "";
        sprintf(sql, "select name from t_user where id = %d;", rq->userId);
        list<string> lstStr;
        if (m_sql.SelectMysql(sql, 1, lstStr) && !lstStr.empty()) {
            strcpy(userName, lstStr.front().c_str());
        }
    }

    _STRU_MEETING_EXIT_NOTIFY notify;
    notify.meetingId = rq->meetingId;
    notify.userId = rq->userId;
    strcpy(notify.userName, userName);
    notify.isMeetingEnd = 1;
    broadcastToMeeting(rq->meetingId, (char*)&notify, sizeof(notify), -1);
}

// 处理会议聊天请求
void Kernel::dealMeetingChatRq(char* data, int len, int sockfd) {
    cout << __func__ << endl;
    _STRU_MEETING_CHAT_RQ* rq = (_STRU_MEETING_CHAT_RQ*)data;

    _STRU_MEETING_CHAT_NOTIFY notify;
    memset(&notify, 0, sizeof(notify));
    notify.type = _DEF_MEETING_CHAT_NOTIFY;
    notify.meetingId = rq->meetingId;
    notify.userId = rq->userId;
    char sql[256] = "";
    sprintf(sql, "select name from t_user where id = %d;", rq->userId);
    list<string> lstStr;
    if (m_sql.SelectMysql(sql, 1, lstStr) && !lstStr.empty()) {
        strcpy(notify.userName, lstStr.front().c_str());
    }
    strcpy(notify.content, rq->content);

    broadcastToMeeting(rq->meetingId, (char*)&notify, sizeof(notify), -1);
}

void Kernel::dealMeetingAudioRq(char* data, int len, int sockfd) {
    cout << __func__ << endl;
    _STRU_MEETING_AUDIO_RQ* rq = (_STRU_MEETING_AUDIO_RQ*)data;
    broadcastToMeeting(rq->meetingId, data, len, sockfd);
}

void Kernel::dealMeetingVideoRq(char* data, int len, int sockfd) {
    cout << __func__ << endl;
    _STRU_MEETING_VIDEO_RQ* rq = (_STRU_MEETING_VIDEO_RQ*)data;
    //cout << "video meetingId=" << rq->meetingId << " map_has=" << (m_mapMeetingMembers.count(rq->meetingId) ? "yes" : "no") << " count=" << (m_mapMeetingMembers.count(rq->meetingId) ? (int)m_mapMeetingMembers[rq->meetingId].size() : 0) << endl;
    broadcastToMeeting(rq->meetingId, data, len, sockfd);
}

void Kernel::handleDisconnect(int sockfd) {
    // 先在锁内找到 userId、收集受影响的会议并清理内存数据
    int userId = -1;
    std::vector<int> affectedMeetings;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pair : m_mapIdToSockfd) {
            if (pair.second == sockfd) {
                userId = pair.first;
                break;
            }
        }
        if (userId == -1) return;
        cout << "handleDisconnect userId=" << userId << " sockfd=" << sockfd << endl;

        // 遍历所有会议，移除该用户
        for (auto& meetingPair : m_mapMeetingMembers) {
            int meetingId = meetingPair.first;
            auto& members = meetingPair.second;
            if (std::find(members.begin(), members.end(), userId) != members.end()) {
                members.erase(std::remove(members.begin(), members.end(), userId), members.end());
                affectedMeetings.push_back(meetingId);
            }
        }
        m_mapIdToSockfd.erase(userId);
    }

    // 锁外: 从数据库获取用户名
    char userName[_DEF_MAX_LENGTH] = {0};
    {
        char sql[256];
        sprintf(sql, "select name from t_user where id = %d;", userId);
        list<string> lstStr;
        if (m_sql.SelectMysql(sql, 1, lstStr) && !lstStr.empty()) {
            strcpy(userName, lstStr.front().c_str());
        }
    }

    // 锁外: 广播退出通知 (broadcastToMeeting 内部会自己加锁)
    for (int meetingId : affectedMeetings) {
        _STRU_MEETING_EXIT_NOTIFY notify;
        notify.meetingId = meetingId;
        notify.userId = userId;
        strcpy(notify.userName, userName);
        broadcastToMeeting(meetingId, (char*)&notify, sizeof(notify), sockfd);
    }
}

//处理获取好友请求列表
void Kernel::dealGetFriendRequestsRq(char* data, int len, int sockfd) {
    (void)len;
    _STRU_GET_FRIEND_REQUESTS_RQ* rq = (_STRU_GET_FRIEND_REQUESTS_RQ*)data;
    cout << __func__ << " userId=" << rq->userId << endl;

    _STRU_GET_FRIEND_REQUESTS_RS rs;
    char sql[1024] = "";
    sprintf(sql, "select rq.id, rq.from_user_id, usr.name from t_friend_requests rq inner join t_user usr on rq.from_user_id = usr.id where rq.to_user_id = %d and rq.status = 0 limit %d;",
            rq->userId, _DEF_MAX_FRIEND_REQUESTS);
    list<string> lstStr;
    if (!m_sql.SelectMysql(sql, 3, lstStr)) {
        cout << "查询数据库失败:" << sql << endl;
        m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
        return;
    }
    rs.count = 0;
    while (!lstStr.empty()) {
        if (rs.count >= _DEF_MAX_FRIEND_REQUESTS) break;
        rs.items[rs.count].requestId = stoi(lstStr.front());
        lstStr.pop_front();
        rs.items[rs.count].fromId = stoi(lstStr.front());
        lstStr.pop_front();
        strcpy(rs.items[rs.count].fromName, lstStr.front().c_str());
        lstStr.pop_front();
        rs.count++;
    }
    m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
}

//处理好友请求操作（同意/拒绝）
void Kernel::dealFriendRequestActionRq(char* data, int len, int sockfd) {
    (void)len;
    _STRU_FRIEND_REQUEST_ACTION_RQ* rq = (_STRU_FRIEND_REQUEST_ACTION_RQ*)data;
    cout << __func__ << " requestId=" << rq->requestId << " action=" << rq->action << endl;

    _STRU_FRIEND_REQUEST_ACTION_RS rs;
    rs.requestId = rq->requestId;
    rs.action = rq->action;

    char sql[1024] = "";
    //验证请求是否存在
    sprintf(sql, "select id, from_user_id, to_user_id from t_friend_requests where id = %d and status = 0;",
            rq->requestId);
    list<string> lstStr;
    if (!m_sql.SelectMysql(sql, 3, lstStr) || lstStr.empty()) {
        cout << "请求不存在或已处理" << endl;
        rs.result = 1;
        m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
        return;
    }
    int fromId = stoi(lstStr.front());
    lstStr.pop_front();
    int toId = stoi(lstStr.front());
    lstStr.pop_front();
    lstStr.clear();

    if (rq->action == _DEF_REQUEST_REFUSE_ACTION) {
        //拒绝：标记请求状态为2（已拒绝）
        memset(sql, 0, sizeof(sql));
        sprintf(sql, "update t_friend_requests set status = 2 where id = %d;", rq->requestId);
        m_sql.UpdateMysql(sql);
        rs.result = 0;
        m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
    } else if (rq->action == _DEF_REQUEST_ACCEPT) {
        //同意：建立好友关系
        //更新请求状态
        memset(sql, 0, sizeof(sql));
        sprintf(sql, "update t_friend_requests set status = 1 where id = %d;", rq->requestId);
        if (!m_sql.UpdateMysql(sql)) {
            rs.result = 1;
            m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
            return;
        }
        //检查是否已经是好友
        memset(sql, 0, sizeof(sql));
        sprintf(sql, "select id from t_friend where (user_id = %d and friend_id = %d) or (user_id = %d and friend_id = %d);",
                fromId, toId, toId, fromId);
        if (!m_sql.SelectMysql(sql, 1, lstStr)) {
            rs.result = 1;
            m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
            return;
        }
        if (lstStr.empty()) {
            //插入双向好友关系
            memset(sql, 0, sizeof(sql));
            sprintf(sql, "insert into t_friend (user_id, friend_id) values (%d, %d), (%d, %d);",
                    fromId, toId, toId, fromId);
            if (!m_sql.UpdateMysql(sql)) {
                rs.result = 1;
                m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
                return;
            }
        }
        rs.result = 0;
        m_pMediator->sendData((char*)&rs, sizeof(rs), sockfd);
        //通知双方添加成功，推送好友信息
        _STRU_FRIEND_INFO infoFrom, infoTo;
        getInfoById(fromId, &infoFrom);
        getInfoById(toId, &infoTo);
        if (m_mapIdToSockfd.count(fromId))
            m_pMediator->sendData((char*)&infoTo, sizeof(infoTo), m_mapIdToSockfd[fromId]);
        if (m_mapIdToSockfd.count(toId))
            m_pMediator->sendData((char*)&infoFrom, sizeof(infoFrom), m_mapIdToSockfd[toId]);
    }
}
