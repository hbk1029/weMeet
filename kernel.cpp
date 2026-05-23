#include "kernel.h"
#include "tracelog.h"
#include"./Net/def.h"
#include"./Mediator/TCPClientMediator.h"
#include"friendrequestlistdialog.h"
#include"VideoApi/h264_encoder.h"
#include"VideoApi/video_decoder.h"
#include<QSettings>
#include<QApplication>
#include<QFileInfo>
#include<QInputDialog>
#include<QFile>
#include<QTextStream>
#include<QDateTime>

Kernel::Kernel(QObject *parent) :
    QObject(parent), m_pMediator(nullptr), m_pLoginDlg(nullptr),
    m_pFriendList(nullptr), m_pMeetingDlg(nullptr), m_currentMeetingId(0),
    m_myId(0), m_name()
{
    //initConfig();
    setProtocol();
    m_pMediator = new TCPClientMediator;
    m_pLoginDlg = new LoginDialog;
    m_pFriendList = new FriendList;
    m_pLoginDlg->show();

    connect(m_pLoginDlg, &LoginDialog::sig_closeProcess, this, &Kernel::slot_closeProcess);
    connect(m_pMediator, &TCPClientMediator::sig_dealData, this, &Kernel::slot_dealData, Qt::BlockingQueuedConnection);
    connect(m_pLoginDlg, &LoginDialog::sig_registerData, this, &Kernel::slot_registerData);
    connect(m_pLoginDlg, &LoginDialog::sig_loginData, this, &Kernel::slot_loginData);
    connect(m_pFriendList, &FriendList::sig_addFriendItem, this, &Kernel::slot_addFriendItem);
    connect(m_pFriendList, &FriendList::sig_meetingCreate, this, &Kernel::slot_meetingCreate);
    connect(m_pFriendList, &FriendList::sig_meetingJoin, this, &Kernel::slot_meetingJoin);
    connect(m_pFriendList, &FriendList::sig_meetingJoinHistory, this, &Kernel::slot_meetingJoinHistory);
    connect(m_pFriendList, &FriendList::sig_deleteFriend, this, &Kernel::slot_deleteFriend);
    connect(m_pFriendList, &FriendList::sig_showFriendRequests, this, &Kernel::slot_showFriendRequests);
    connect(this, &Kernel::sig_friendRequestCountChanged, m_pFriendList, &FriendList::updateFriendRequestBadge);
    connect(this, &Kernel::sig_showMeetingDialog, this, [this](int meetingId, bool isCreator) {
        if (!m_pMeetingDlg) {
            m_pMeetingDlg = new MeetingDialog(nullptr);
            connect(m_pMeetingDlg, &MeetingDialog::sig_meetingExit, this, &Kernel::slot_meetingExit);
            connect(m_pMeetingDlg, &MeetingDialog::sig_meetingChat, this, &Kernel::slot_meetingChat);
            connect(m_pMeetingDlg, &MeetingDialog::sig_meetingAudio, this, &Kernel::slot_meetingAudio);
            connect(m_pMeetingDlg, &MeetingDialog::sig_videoFrame, this, &Kernel::slot_meetingVideo);
            connect(m_pMeetingDlg, &MeetingDialog::sig_meetingEnd, this, &Kernel::slot_meetingEnd);
        }
        m_pMeetingDlg->setMeetingInfo(meetingId, isCreator);
        m_pMeetingDlg->setUserName(m_name);
        m_pMeetingDlg->show();
    });

    if(!m_pMediator->openNet()) {
        QMessageBox::about(m_pLoginDlg, "提示", "客户端打开失败");
        exit(1);
    }
}

Kernel* Kernel::getSingleInstance()
{
    static Kernel kernel;
    return &kernel;
}

//初始化配置
void Kernel::initConfig()
{
    m_serverIP = _DEF_SERVER_IP;
    //路径设置: 在exe同级config.ini
    QString path = QApplication::applicationDirPath() + "/config.ini";
    //判断是否存在
    QFileInfo info(path);
    //加载配置文件, ip设置为配置文件中的IP
    QSettings settings(path, QSettings::IniFormat, nullptr); //有就打开, 没有就创建
    if(info.exists()) {
        //打开配置文件, 移动到ip组
        settings.beginGroup("");
        //读取ip -> addr
        QVariant ip = settings.value("ip");
        QString strIp = ip.toString();
        //结束
        settings.endGroup();
        if(!strIp.isEmpty()) {
            m_serverIP = strIp;

        }
    }
    //如果没有配置文件, 就使用默认的
    else {
        settings.beginGroup("Net");
        settings.setValue("ip", m_serverIP);
        settings.endGroup();
    }
    qDebug()<<"Server ip:"<<m_serverIP;
}


//初始化数组
void Kernel::setProtocol() {
    qDebug()<<__func__;
    //初始化为0
    memset(m_protocol, 0, sizeof(m_protocol));
    //绑定协议
    m_protocol[_DEF_REGISTER_RS - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealRegisterRs;
    m_protocol[_DEF_LOGIN_RS - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealLoginRs;
    m_protocol[_DEF_FRIEND_INFO - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealFriendInfo;
    m_protocol[_DEF_ADD_FRIEND_RS - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealAddFriendRs;
    m_protocol[_DEF_FRIEND_REQUEST_NOTIFY - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealFriendRequestNotify;
    m_protocol[_DEF_CHAT_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealChatRq;
    m_protocol[_DEF_CHAT_RS - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealChatRs;
    m_protocol[_DEF_OFFLINE_RQ - _DEF_PROTOCOL_BASE - 1] = & Kernel::dealOfflineRq;
    //会议相关协议
    m_protocol[_DEF_MEETING_CREATE_RS - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealMeetingCreateRs;
    m_protocol[_DEF_MEETING_JOIN_RS - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealMeetingJoinRs;
    m_protocol[_DEF_MEETING_JOIN_NOTIFY - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealMeetingJoinNotify;
    m_protocol[_DEF_MEETING_EXIT_NOTIFY - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealMeetingExitNotify;
    m_protocol[_DEF_MEETING_CHAT_NOTIFY - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealMeetingChatNotify;
    m_protocol[_DEF_MEETING_MEMBER_INFO - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealMeetingMemberInfo;
    m_protocol[_DEF_MEETING_AUDIO_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealMeetingAudioNotify;
    m_protocol[_DEF_MEETING_VIDEO_RQ - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealMeetingVideoNotify;
    //好友相关协议
    m_protocol[_DEF_DELETE_FRIEND_RS - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealDeleteFriendRs;
    m_protocol[_DEF_FRIEND_STATUS_NOTIFY - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealFriendStatusNotify;
    //好友请求列表
    m_protocol[_DEF_GET_FRIEND_REQUESTS_RS - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealGetFriendRequestsRs;
    m_protocol[_DEF_FRIEND_REQUEST_ACTION_RS - _DEF_PROTOCOL_BASE - 1] = &Kernel::dealFriendRequestActionRs;
}
//处理注册回复
void Kernel::dealRegisterRs(char* data, int len, long from) {
    qDebug()<<__func__;
    //拆包
    _STRU_REGISTER_RS* rs = (_STRU_REGISTER_RS*)data;
    //根据信息提示用户
    switch (rs->result) {
    case _DEF_REGISTER_SUCCESS:
        QMessageBox::about(m_pLoginDlg, "提示", "注册成功");
        break;
    case _DEF_NAME_EXIST:
        QMessageBox::about(m_pLoginDlg, "提示", "昵称已注册");
        break;
    case _DEF_TEL_EXIST:
        QMessageBox::about(m_pLoginDlg, "提示", "电话号已注册");
        break;
    default:
        break;
    }
}
//处理登录回复
void Kernel::dealLoginRs(char* data, int len, long from) {
    qDebug()<<__func__;
    //拆包
    _STRU_LOGIN_RS* rs = (_STRU_LOGIN_RS*)data;
    switch (rs->result) {
    case _DEF_LOGIN_SUCCESS:
    {
        //登录成功, 先存一下id
        m_myId = rs->userId;
        m_name = rs->userName;
        qDebug() << "m_myId set to:" << m_myId;
        //隐藏登录界面
        m_pLoginDlg->hide();
        //显示好友列表界面
        m_pFriendList->show();
        //拉取离线好友请求
        slot_fetchPendingRequests();
        break;
    }
    case _DEF_TEL_NOT_EXISTS:
        QMessageBox::about(m_pLoginDlg, "提示", "账号不存在");
        break;
    case _DEF_PASSWORD_ERROR:
        QMessageBox::about(m_pLoginDlg, "提示", "密码错误");
        break;
    default:
        break;
    }
}
//处理用户信息
void Kernel::dealFriendInfo(char* data, int len, long from) {
    qDebug()<<__func__;
    //拆包
    _STRU_FRIEND_INFO* info = (_STRU_FRIEND_INFO*)data;
    qDebug() << "info->id:" << info->id << " m_myId:" << m_myId;  // 【添加调试
    //判断是自己的id还是朋友的id
    if(info->id == m_myId) {
        //设置自己信息
        m_pFriendList->setUserInfo(info->name, info->iconId, info->feeling);
    }
    else {
        //设置朋友信息
        if(m_mapIdToFriendItem.count(info->id) > 0) {
            //更新朋友信息
            FriendItem* item = m_mapIdToFriendItem[info->id];
            item->setFriendInfo(info->id, info->name, info->iconId, info->feeling, info->status);
        }
        else {
            //创建FriendItem, 并设置信息
            FriendItem* item = new FriendItem;
            item->setFriendInfo(info->id, info->name, info->iconId, info->feeling, info->status);
            //存到map里
            m_mapIdToFriendItem[info->id] = item;
            //添加到好友列表
            m_pFriendList->addFriendItem(item);
            //TODO:创建一个好友, 就要创建聊天窗口

        }
    }
}
//处理添加好友回复
void Kernel::dealAddFriendRs(char *data, int len, long from) {
    qDebug()<<__func__;
    _STRU_ADD_FRIEND_RS* rs = (_STRU_ADD_FRIEND_RS*)data;
    switch (rs->result) {
    case _DEF_ADD_FRIEND_SUCCESS:
        QMessageBox::about(m_pLoginDlg, "提示", "添加好友成功");
        break;
    case _DEF_FRIEND_NOT_EXISTS:
        QMessageBox::about(m_pLoginDlg, "提示", "好友不存在");
        break;
    case _DEF_ALREADY_FRIEND:
        QMessageBox::about(m_pLoginDlg, "提示", "已经是好友了");
        break;
    case _DEF_REQUEST_SEND:
        QMessageBox::about(m_pLoginDlg, "提示", "请求已发送");
        break;
    case _DEF_REQUEST_ALREADY_SEND:
        QMessageBox::about(m_pLoginDlg, "提示", "已经发送过请求, 请勿重复");
        break;
    case _DEF_REQUEST_REFUSE:
        QMessageBox::about(m_pLoginDlg, "提示", "对方拒绝添加你为好友");
        break;
    case _DEF_CANNOT_ADD_SELF:
        QMessageBox::about(m_pLoginDlg, "提示", "不能添加自己");
        break;
    default:
        break;
    }
}

void Kernel::dealFriendRequestNotify(char *data, int len, long from) {
    qDebug()<<__func__;
    Q_UNUSED(len);
    Q_UNUSED(from);
    _STRU_FRIEND_REQUEST_NOTIFY* notify = (_STRU_FRIEND_REQUEST_NOTIFY*)data;
    // 检查是否已存在相同请求
    for (const auto& req : m_pendingRequests) {
        if (req.requestId == notify->requestId) return;
    }
    // 存入待处理列表
    _STRU_FRIEND_REQUEST_ITEM item;
    item.requestId = notify->requestId;
    item.fromId = notify->fromId;
    strcpy(item.fromName, notify->fromName);
    m_pendingRequests.append(item);
    // 通知UI更新角标
    Q_EMIT sig_friendRequestCountChanged(m_pendingRequests.size());
}
//处理聊天请求
void Kernel::dealChatRq(char* data, int len, long from) {

}
//处理聊天回复
void Kernel::dealChatRs(char* data, int len, long from) {

}
//处理下线请求
void Kernel::dealOfflineRq(char* data, int len, long from) {

}

void Kernel::slot_closeProcess() {
    qDebug()<<__func__;
    //回收资源
    if(m_pLoginDlg){
        m_pLoginDlg->hide();
        delete m_pLoginDlg;
        m_pLoginDlg = nullptr;
    }
    if(m_pFriendList) {
        m_pFriendList->hide();
        delete m_pFriendList;
        m_pFriendList = nullptr;
    }
    if(m_pMeetingDlg) {
        m_pMeetingDlg->hide();
        delete m_pMeetingDlg;
        m_pMeetingDlg = nullptr;
    }
    if(m_pMediator) {
        m_pMediator->closeNet();
        delete m_pMediator;
        m_pMediator = nullptr;
    }
//    //清空map中new的对象
//    for(auto it = m_mapIdTOFriendItem.begin(); it!= m_mapIdTOFriendItem.end();){
//        //取出当前迭代器指向的对象
//        FriendItem* item = it.value();
//        //销毁
//        delete item;
//        //清除这个无效节点
//        it = m_mapIdTOFriendItem.erase(it);
//    }
//    for(auto it = m_mapIdToChatDiaglog.begin(); it!= m_mapIdToChatDiaglog.end();){
//        ChatDialog* chat = it.value();
//        delete chat;
//        it = m_mapIdToChatDiaglog.erase(it);
//    }
    //关闭进程
    exit(0);
}

void Kernel::slot_dealData(char *data, int len, long from)
{
    packType type = *(int*)data;
    // 仅对高频类型（音频/视频）抽样记录，避免日志爆炸
    static int dispCount = 0; dispCount++;
    bool isAV = (type == _DEF_MEETING_AUDIO_RQ || type == _DEF_MEETING_VIDEO_RQ);
    if (!isAV || dispCount <= 3 || dispCount % 100 == 0)
        TRACE("DISPATCH #%d type=%d len=%d isAV=%d", dispCount, type, len, isAV);

    int index = type - _DEF_PROTOCOL_BASE - 1;
    if(index >= 0 && index < _DEF_PROTOCOL_COUNT){
        PFUN pf = m_protocol[index];
        if(pf){
            (this->*pf)(data, len, from);
        }
    }
}

void Kernel::slot_registerData(QString name, QString tel, QString pass) {
    qDebug()<<__func__;
    _STRU_REGISTER_RQ rq;
    strcpy_s(rq.name, sizeof(rq.name), name.toStdString().c_str());
    strcpy_s(rq.tel, sizeof(rq.tel), tel.toStdString().c_str());
    strcpy_s(rq.pass, sizeof(rq.pass), pass.toStdString().c_str());
    //发送给服务端
    m_pMediator->sendData((char*)&rq, sizeof(rq), 0);
}

void Kernel::slot_loginData(QString tel, QString pass) {
    qDebug()<<__func__ << tel << pass;
    _STRU_LOGIN_RQ rq;
    strcpy_s(rq.tel, sizeof(rq.tel), tel.toStdString().c_str());
    strcpy_s(rq.pass, sizeof(rq.pass), pass.toStdString().c_str());
    qDebug() << "rq size:" << sizeof(rq) << "rq.tel:" << rq.tel << "rq.pass:" << rq.pass;
    //发送给服务端
    bool ok = m_pMediator->sendData((char*)&rq, sizeof(rq), 0);
    qDebug() << "sendData returned:" << ok;
}

void Kernel::slot_addFriendItem()
{
    qDebug() << __func__;
    //弹出添加好友对话框
    AddFriendDialog dlg(m_pFriendList);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    //获取用户输入的好友名
    QString name = dlg.getInputText();
    if (name.isEmpty()) {
        return;
    }
    //把添加好友的包发给服务器
    _STRU_ADD_FRIEND_RQ rq;
    rq.userId = m_myId;
    strcpy(rq.userName, m_name.toStdString().c_str());
    strcpy(rq.friendName, name.toStdString().c_str());
    m_pMediator->sendData((char*)&rq, sizeof(rq), 0);
}

//处理创建会议回复
void Kernel::dealMeetingCreateRs(char* data, int len, long from) {
    qDebug()<<__func__;
    _STRU_MEETING_CREATE_RS* rs = (_STRU_MEETING_CREATE_RS*)data;
    if (rs->result == 0) {
        m_currentMeetingId = rs->meetingId;
        m_pFriendList->addMeetingHistory(QString::number(rs->meetingId));
        Q_EMIT sig_showMeetingDialog(rs->meetingId, true);
    } else {
        QMessageBox::about(m_pFriendList, "提示", "创建会议失败");
    }
}

//处理加入会议回复
void Kernel::dealMeetingJoinRs(char* data, int len, long from) {
    qDebug()<<__func__;
    _STRU_MEETING_JOIN_RS* rs = (_STRU_MEETING_JOIN_RS*)data;
    if (rs->result == 0) {
        m_currentMeetingId = rs->meetingId;
        m_pFriendList->addMeetingHistory(QString::number(rs->meetingId));
        Q_EMIT sig_showMeetingDialog(rs->meetingId, rs->isCreator != 0);
    } else if (rs->result == 1) {
        m_pFriendList->updateMeetingStatus(QString::number(rs->meetingId), false);
        QMessageBox::about(m_pFriendList, "提示", "会议不存在");
    } else if (rs->result == 2) {
        m_pFriendList->updateMeetingStatus(QString::number(rs->meetingId), false);
        QMessageBox::about(m_pFriendList, "提示", "会议已结束");
    }
}

//处理新成员加入通知
void Kernel::dealMeetingJoinNotify(char* data, int len, long from) {
    Q_UNUSED(len); Q_UNUSED(from);
    _STRU_MEETING_JOIN_NOTIFY* notify = (_STRU_MEETING_JOIN_NOTIFY*)data;
    TRACE("JOIN_NOTIFY userId=%d userName=%s dlg=%d", notify->userId, notify->userName, (m_pMeetingDlg!=nullptr));
    if (m_pMeetingDlg) {
        m_pMeetingDlg->addMember(notify->userId, notify->userName);
        m_pMeetingDlg->appendSystemMsg(QString("%1 加入了会议").arg(notify->userName));
    }
}

//处理成员退出通知
void Kernel::dealMeetingExitNotify(char* data, int len, long from) {
    qDebug()<<__func__;
    _STRU_MEETING_EXIT_NOTIFY* notify = (_STRU_MEETING_EXIT_NOTIFY*)data;
    if (!m_pMeetingDlg) return;
    if (notify->isMeetingEnd) {
        m_pMeetingDlg->appendSystemMsg(QString::fromUtf8("主持人已结束会议"));
        m_pMeetingDlg->hide();
        m_pMeetingDlg->deleteLater();
        m_pMeetingDlg = nullptr;
        m_pFriendList->updateMeetingStatus(QString::number(notify->meetingId), false);
        m_currentMeetingId = 0;
    } else {
        m_pMeetingDlg->removeMember(notify->userId);
        m_pMeetingDlg->appendSystemMsg(QString("%1 离开了会议").arg(notify->userName));
    }
}

//处理会议聊天通知
void Kernel::dealMeetingChatNotify(char* data, int len, long from) {
    Q_UNUSED(len); Q_UNUSED(from);
    _STRU_MEETING_CHAT_NOTIFY* notify = (_STRU_MEETING_CHAT_NOTIFY*)data;
    TRACE("CHAT_NOTIFY userId=%d userName=%s content=%s dlg=%d", notify->userId, notify->userName, notify->content, (m_pMeetingDlg!=nullptr));
    QFile f(QString("C:\\temp\\chat_debug_%1.log").arg(QCoreApplication::applicationPid()));
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream s(&f);
        s << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
          << " RECV_NOTIFY user=" << notify->userId
          << " content=" << notify->content << "\n";
        f.close();
    }
    if (m_pMeetingDlg) {
        m_pMeetingDlg->appendChatMsg(notify->userId, notify->userName, notify->content);
    }
}

//处理会议成员信息
void Kernel::dealMeetingMemberInfo(char* data, int len, long from) {
    Q_UNUSED(len); Q_UNUSED(from);
    _STRU_MEETING_MEMBER_INFO* info = (_STRU_MEETING_MEMBER_INFO*)data;
    TRACE("MEMBER_INFO userId=%d userName=%s dlg=%d", info->userId, info->userName, (m_pMeetingDlg!=nullptr));
    if (m_pMeetingDlg) {
        m_pMeetingDlg->addMember(info->userId, info->userName);
    }
}

//创建会议
void Kernel::slot_meetingCreate() {
    qDebug()<<__func__;
    if (m_currentMeetingId != 0 && m_pMeetingDlg) {
        QMessageBox::about(m_pFriendList, "提示", "已在会议中，请先退出当前会议");
        return;
    }
    m_currentMeetingId = 0;
    _STRU_MEETING_CREATE_RQ rq;
    rq.userId = m_myId;
    strcpy(rq.userName, m_name.toStdString().c_str());
    m_pMediator->sendData((char*)&rq, sizeof(rq), 0);
}

//加入会议
void Kernel::slot_meetingJoin() {
    qDebug()<<__func__;
    if (m_currentMeetingId != 0 && m_pMeetingDlg) {
        QMessageBox::about(m_pFriendList, "提示", "已在会议中，请先退出当前会议");
        return;
    }
    m_currentMeetingId = 0;
    bool ok;
    int meetingCode = QInputDialog::getInt(m_pFriendList,
        "加入会议", "请输入9位会议号：", 0, 100000000, 999999999, 1, &ok);
    if (!ok) {
        return;
    }
    _STRU_MEETING_JOIN_RQ rq;
    rq.userId = m_myId;
    rq.meetingId = meetingCode;
    strcpy(rq.userName, m_name.toStdString().c_str());
    m_pMediator->sendData((char*)&rq, sizeof(rq), 0);
}

//从历史记录加入会议（无需弹输入框）
void Kernel::slot_meetingJoinHistory(QString meetingId) {
    qDebug()<<__func__;
    if (m_currentMeetingId != 0 && !m_pMeetingDlg) {
        m_currentMeetingId = 0;
    }
    if (m_currentMeetingId != 0) {
        QMessageBox::about(m_pFriendList, "提示", "已在会议中，请先退出当前会议");
        return;
    }
    bool ok;
    int meetingCode = meetingId.toInt(&ok);
    if (!ok) {
        return;
    }
    _STRU_MEETING_JOIN_RQ rq;
    rq.userId = m_myId;
    rq.meetingId = meetingCode;
    strcpy(rq.userName, m_name.toStdString().c_str());
    m_pMediator->sendData((char*)&rq, sizeof(rq), 0);
}

//退出会议
void Kernel::slot_meetingExit() {
    qDebug()<<__func__;
    if (m_currentMeetingId == 0) return;
    _STRU_MEETING_EXIT_RQ rq;
    rq.userId = m_myId;
    rq.meetingId = m_currentMeetingId;
    m_pMediator->sendData((char*)&rq, sizeof(rq), 0);
    if (m_pMeetingDlg) {
        m_pMeetingDlg->hide();
        m_pMeetingDlg->deleteLater();
        m_pMeetingDlg = nullptr;
    }
    m_currentMeetingId = 0;
}

//结束会议（仅主持人）
void Kernel::slot_meetingEnd() {
    qDebug()<<__func__;
    if (m_currentMeetingId == 0) return;
    _STRU_MEETING_EXIT_RQ rq;
    rq.type = _DEF_MEETING_END_RQ;
    rq.userId = m_myId;
    rq.meetingId = m_currentMeetingId;
    m_pMediator->sendData((char*)&rq, sizeof(rq), 0);
}

//发送会议聊天
void Kernel::slot_meetingChat(QString content) {
    qDebug()<<__func__;
    QFile f(QString("C:\\temp\\chat_debug_%1.log").arg(QCoreApplication::applicationPid()));
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream s(&f);
        s << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
          << " SEND_CHAT content=" << content << "\n";
        f.close();
    }
    if (m_currentMeetingId == 0) return;
    _STRU_MEETING_CHAT_RQ rq;
    rq.userId = m_myId;
    rq.meetingId = m_currentMeetingId;
    strcpy(rq.content, content.toStdString().c_str());
    m_pMediator->sendData((char*)&rq, sizeof(rq), 0);
}

//发送会议音频
void Kernel::slot_meetingAudio(const char* data, int len) {
    static int sendCount = 0; sendCount++;
    if (sendCount <= 3 || sendCount % 100 == 0)
        TRACE("AUDIO_SEND #%d len=%d meetingId=%d", sendCount, len, m_currentMeetingId);
    if (m_currentMeetingId == 0 || !data || len <= 0) {
        static int skipCount = 0;
        if (++skipCount <= 3)
            TRACE("AUDIO_SEND skip #%d meetingId=%d data=%d len=%d", skipCount, m_currentMeetingId, (data!=nullptr), len);
        return;
    }
    _STRU_MEETING_AUDIO_RQ rq;
    rq.userId = m_myId;
    rq.meetingId = m_currentMeetingId;
    rq.dataLen = len;
    if (len > (int)sizeof(rq.data)) {
        static int overflowCount = 0;
        if (++overflowCount <= 3)
            qDebug() << "[AUDIO_KERNEL_SEND] OVERFLOW! len=" << len << " > buf=" << sizeof(rq.data);
        return;
    }
    memcpy(rq.data, data, len);
    // 变长发送: 只发固定头 + 实际数据, 避免每帧发满 4KB
    int sendLen = (int)(sizeof(rq) - sizeof(rq.data) + len);
    if (sendCount <= 3 || sendCount % 100 == 0)
        TRACE("AUDIO_SEND data #%d sendLen=%d", sendCount, sendLen);
    m_pMediator->sendData((char*)&rq, sendLen, 0);
}

//处理会议音频接收
void Kernel::dealMeetingAudioNotify(char* data, int len, long from) {
    Q_UNUSED(from);
    _STRU_MEETING_AUDIO_RQ* rq = (_STRU_MEETING_AUDIO_RQ*)data;
    static int recvCount = 0; recvCount++;
    if (recvCount <= 3 || recvCount % 100 == 0)
        TRACE("AUDIO_NOTIFY #%d dataLen=%d dlg=%d", recvCount, rq->dataLen, (m_pMeetingDlg!=nullptr));
    if (m_pMeetingDlg && m_pMeetingDlg->getAudioWrite() && rq->dataLen > 0) {
        m_pMeetingDlg->getAudioWrite()->slot_net_rx(rq->data, rq->dataLen);
    }
}

//发送会议视频
void Kernel::slot_meetingVideo(QByteArray data)
{
    if (m_currentMeetingId == 0) return;
    int len = data.size();
    _STRU_MEETING_VIDEO_RQ rq;
    rq.userId = m_myId;
    rq.meetingId = m_currentMeetingId;
    rq.dataLen = len;
#ifdef USE_H264
    rq.codec = 1;
#endif
    static int sendCount = 0;
    if (++sendCount <= 3 || sendCount % 30 == 0)
        fprintf(stderr, "[SEND] video frame #%d dataLen=%d codec=%d\n", sendCount, len, rq.codec);
    if (len > (int)sizeof(rq.data)) return;
    if (len > 0) memcpy(rq.data, data.constData(), len);
    m_pMediator->sendData((char*)&rq, sizeof(rq), 0);
}

//处理会议视频接收
void Kernel::dealMeetingVideoNotify(char* data, int len, long from)
{
    Q_UNUSED(from);
    _STRU_MEETING_VIDEO_RQ* rq = (_STRU_MEETING_VIDEO_RQ*)data;
    if (!m_pMeetingDlg) return;
    static int recvCount = 0; recvCount++;
    if (recvCount <= 3 || recvCount % 30 == 0)
        TRACE("VIDEO_NOTIFY #%d dataLen=%d codec=%d", recvCount, rq->dataLen, rq->codec);
    if (rq->dataLen == 0) {
        TRACE("VIDEO_NOTIFY clear frame");
        m_pMeetingDlg->clearRemoteVideo();
        return;
    }
    // 通过 VideoDecoder Worker 线程解码（跨线程调用）
    VideoDecoder* decoder = m_pMeetingDlg->getVideoDecoder();
    if (decoder) {
        QByteArray packetData(rq->data, rq->dataLen);
        QMetaObject::invokeMethod(decoder, "slot_decodePacket", Qt::QueuedConnection,
                                  Q_ARG(QByteArray, packetData));
    }
}

//处理删除好友回复
void Kernel::dealDeleteFriendRs(char* data, int len, long from) {
    Q_UNUSED(len);
    Q_UNUSED(from);
    qDebug()<<__func__;
    _STRU_DELETE_FRIEND_RS* rs = (_STRU_DELETE_FRIEND_RS*)data;
    if (rs->result == 0) {
        //删除成功, 从列表移除
        if (m_mapIdToFriendItem.count(rs->friendId) > 0) {
            FriendItem* item = m_mapIdToFriendItem[rs->friendId];
            m_pFriendList->removeFriendItem(item);
            m_mapIdToFriendItem.erase(rs->friendId);
            delete item;
        }
    }
}

//处理好友状态推送
void Kernel::dealFriendStatusNotify(char* data, int len, long from) {
    Q_UNUSED(len);
    Q_UNUSED(from);
    _STRU_FRIEND_STATUS_NOTIFY* notify = (_STRU_FRIEND_STATUS_NOTIFY*)data;
    if (m_mapIdToFriendItem.count(notify->userId) > 0) {
        FriendItem* item = m_mapIdToFriendItem[notify->userId];
        //更新好友在线状态 (触发头像颜色变化)
        item->setFriendStatus(notify->status);
    }
}

//删除好友
void Kernel::slot_deleteFriend(int friendId) {
    qDebug() << __func__ << friendId;
    _STRU_DELETE_FRIEND_RQ rq;
    rq.userId = m_myId;
    rq.friendId = friendId;
    m_pMediator->sendData((char*)&rq, sizeof(rq), 0);
}

//拉取离线好友请求
void Kernel::slot_fetchPendingRequests() {
    qDebug() << __func__;
    _STRU_GET_FRIEND_REQUESTS_RQ rq;
    rq.userId = m_myId;
    m_pMediator->sendData((char*)&rq, sizeof(rq), 0);
}

//处理获取好友请求列表回复
void Kernel::dealGetFriendRequestsRs(char* data, int len, long from) {
    Q_UNUSED(len);
    Q_UNUSED(from);
    qDebug() << __func__;
    _STRU_GET_FRIEND_REQUESTS_RS* rs = (_STRU_GET_FRIEND_REQUESTS_RS*)data;
    m_pendingRequests.clear();
    for (int i = 0; i < rs->count; ++i) {
        m_pendingRequests.append(rs->items[i]);
    }
    Q_EMIT sig_friendRequestCountChanged(m_pendingRequests.size());
}

//处理好友请求操作回复
void Kernel::dealFriendRequestActionRs(char* data, int len, long from) {
    Q_UNUSED(len);
    Q_UNUSED(from);
    qDebug() << __func__;
    _STRU_FRIEND_REQUEST_ACTION_RS* rs = (_STRU_FRIEND_REQUEST_ACTION_RS*)data;
    if (rs->result == 0) {
        // 从待处理列表移除该请求
        for (int i = 0; i < m_pendingRequests.size(); ++i) {
            if (m_pendingRequests[i].requestId == rs->requestId) {
                m_pendingRequests.removeAt(i);
                break;
            }
        }
        Q_EMIT sig_friendRequestCountChanged(m_pendingRequests.size());
        if (rs->action == _DEF_REQUEST_ACCEPT) {
            QMessageBox::about(m_pFriendList, QString::fromUtf8("提示"),
                QString::fromUtf8("已同意好友请求"));
        }
    } else {
        QMessageBox::about(m_pFriendList, QString::fromUtf8("提示"),
            QString::fromUtf8("操作失败，请重试"));
    }
}

//显示好友请求列表对话框
void Kernel::slot_showFriendRequests() {
    qDebug() << __func__;
    if (m_pendingRequests.isEmpty()) {
        QMessageBox::about(m_pFriendList, QString::fromUtf8("好友请求"),
            QString::fromUtf8("暂无新的好友请求"));
        return;
    }
    FriendRequestListDialog* dlg = new FriendRequestListDialog(m_pFriendList);
    dlg->setRequests(m_pendingRequests);
    connect(dlg, &FriendRequestListDialog::sig_acceptRequest,
            this, &Kernel::slot_acceptFriendRequest);
    connect(dlg, &FriendRequestListDialog::sig_refuseRequest,
            this, &Kernel::slot_refuseFriendRequest);
    dlg->exec();
    dlg->deleteLater();
}

//同意好友请求
void Kernel::slot_acceptFriendRequest(int requestId, int fromId, const QString& fromName) {
    qDebug() << __func__ << requestId << fromId << fromName;
    _STRU_FRIEND_REQUEST_ACTION_RQ rq;
    rq.userId = m_myId;
    rq.requestId = requestId;
    rq.action = _DEF_REQUEST_ACCEPT;
    strcpy(rq.friendName, fromName.toStdString().c_str());
    m_pMediator->sendData((char*)&rq, sizeof(rq), 0);
}

//拒绝好友请求
void Kernel::slot_refuseFriendRequest(int requestId) {
    qDebug() << __func__ << requestId;
    _STRU_FRIEND_REQUEST_ACTION_RQ rq;
    rq.userId = m_myId;
    rq.requestId = requestId;
    rq.action = _DEF_REQUEST_REFUSE_ACTION;
    m_pMediator->sendData((char*)&rq, sizeof(rq), 0);
}
