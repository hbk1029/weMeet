#pragma once
#include"../Mediator/INetMediator.h"
#include"../MySQL/MySQL.h"
#include<map>
#include<vector>
#include <set>
#include<algorithm>
#include <cstdlib>
#include <mutex>

class Kernel;
typedef void (Kernel::*PFUN)(char* data, int len, int sockfd);

class Kernel {
public:
    INetMediator* m_pMediator;
    MySQL m_sql;
    PFUN m_protocol[_DEF_PROTOCOL_COUNT];
    map<int, int> m_mapIdToSockfd;
    std::mutex m_mutex;
    map<int, vector<int>> m_mapMeetingMembers;

private:
    Kernel();
    ~Kernel();

public:
    static Kernel* m_pKernel;

    static Kernel* getInstance();
    void setProtocol();
    bool startServer();
    void runServer();
    void endServer();
    void dealData(char* data, int len, int sockfd);

    void dealRegisterRq(char* data, int len, int sockfd);
    void dealLoginRq(char* data, int len, int sockfd);
    void dealAddFriendRq(char* data, int len, int sockfd);
    void dealAddFriendRs(char* data, int len, int sockfd);

    void getUserInfoAndFriendInfo(int userId);
    void getInfoById(int userId, _STRU_FRIEND_INFO* info);
    // --- 会议处理函数 ---
    void dealMeetingCreateRq(char* data, int len, int sockfd);
    void dealMeetingJoinRq(char* data, int len, int sockfd);
    void dealMeetingExitRq(char* data, int len, int sockfd);
    void dealMeetingChatRq(char* data, int len, int sockfd);
    void dealMeetingAudioRq(char* data, int len, int sockfd);
    void dealMeetingVideoRq(char* data, int len, int sockfd);
    void dealGetFriendRequestsRq(char* data, int len, int sockfd);
    void dealFriendRequestActionRq(char* data, int len, int sockfd);
    void dealMeetingEndRq(char* data, int len, int sockfd);
    void dealDeleteFriendRq(char* data, int len, int sockfd);
    void broadcastFriendStatus(int userId, int status);
    void handleDisconnect(int sockfd);

    int generateMeetingCode();
    void broadcastToMeeting(int meetingCode, char* data, int len, int excludeSockfd);
};
