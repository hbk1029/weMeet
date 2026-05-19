#ifndef MEETINGDIALOG_H
#define MEETINGDIALOG_H

#include <QWidget>
#include <QListWidget>
#include <QTextBrowser>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <map>
#include"AudioApi/audio_read.h"
#include"AudioApi/audio_write.h"
#include"VideoApi/video_read.h"
#include"VideoApi/video_write.h"

using namespace std;

namespace Ui {
class MeetingDialog;
}

class MeetingDialog : public QWidget
{
    Q_OBJECT
public:
    explicit MeetingDialog(QWidget *parent = nullptr);
    ~MeetingDialog();
    //设置会议信息
    void setMeetingInfo(int meetingId, bool isCreator);
    //设置窗口标题显示用户名
    void setUserName(const QString& name);
    //添加成员
    void addMember(int userId, QString userName);
    //获取音频播放对象
    Audio_Write* getAudioWrite() { return m_pAudioWrite; }
    //获取视频显示对象
    Video_Write* getVideoWrite() { return m_pVideoWrite; }
    //移除成员
    void removeMember(int userId);
    //清空远端视频
    void clearRemoteVideo();
    //添加系统消息
    void appendSystemMsg(QString msg);
    //添加聊天消息
    void appendChatMsg(int userId, QString userName, QString content);

signals:
    //退出会议信号
    void sig_meetingExit();
    //结束会议信号（仅主持人）
    void sig_meetingEnd();
    //发送聊天消息信号
    void sig_meetingChat(QString content);
    //发送音频帧信号
    void sig_meetingAudio(const char* data, int len);
    //发送视频帧信号
    void sig_videoFrame(const char* data, int len);

private slots:
    void on_pb_send_clicked();
    void on_pb_exit_clicked();
    void on_pb_copy_clicked();
    void on_pb_microphone_clicked();
    void on_pb_camera_clicked();
    //计时器
    void slot_timerTick();

private:
    Ui::MeetingDialog *ui;
    QTimer* m_pTimer;
    int m_elapsedSec;
    int m_meetingId;
    bool m_isCreator;
    map<int, QListWidgetItem*> m_mapUserIdToItem;
    Audio_Read*  m_pAudioRead;
    Audio_Write* m_pAudioWrite;
    Video_Read*  m_pVideoRead;
    Video_Write* m_pVideoWrite;       // 对方画面
    Video_Write* m_pVideoWriteLocal;  // 本地画中画
    bool m_isVideoOn;
    bool m_isMicMuted;
    QPushButton* m_pEndMeetingBtn;
};

#endif // MEETINGDIALOG_H
