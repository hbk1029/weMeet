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
#include"AudioApi/audio_common.h"
#include"AudioApi/audio_read.h"
#include"AudioApi/audio_write.h"
#include"VideoApi/video_read.h"
#include"VideoApi/video_write.h"
#include "AudioApi/sdl_audio_read.h"
#include "AudioApi/sdl_audio_write.h"
#include "AudioApi/opus_encoder.h"
#include "AudioApi/audio_dsp.h"
#include "VideoApi/opengl_render.h"
#include "VideoApi/video_decoder.h"

using namespace std;

namespace Ui {
class MeetingDialog;
}

class WavRecorder;
class WavPlayer;
class AudioDSP;

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
#ifdef USE_SDL2
    SDLAudioWrite* getAudioWrite() { return m_pSDLAudioWrite; }
#else
    Audio_Write* getAudioWrite() { return m_pAudioWrite; }
#endif
    //获取视频显示对象
#ifdef USE_OPENGL
    OpenGLRender* getVideoWrite() { return m_pVideoWrite; }
#else
    Video_Write* getVideoWrite() { return m_pVideoWrite; }
#endif
    VideoDecoder* getVideoDecoder() { return m_pVideoDecoder; }
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
    void sig_videoFrame(QByteArray data);

private slots:
    void on_pb_send_clicked();
    void on_pb_exit_clicked();
    void on_pb_copy_clicked();
    void on_pb_microphone_clicked();
    void on_pb_camera_clicked();
    //计时器
    void slot_timerTick();
    //录制/回放
    void slot_recordToggle();
    void slot_playRecording();

private:
    Ui::MeetingDialog *ui;
    QTimer* m_pTimer;
    int m_elapsedSec;
    int m_meetingId;
    bool m_isCreator;
    map<int, QListWidgetItem*> m_mapUserIdToItem;
#ifdef USE_SDL2
    SDLAudioRead*  m_pSDLAudioRead;
    SDLAudioWrite* m_pSDLAudioWrite;
    OpusEncoder*   m_pOpusEncoder;
#else
    Audio_Read*  m_pAudioRead;
    Audio_Write* m_pAudioWrite;
#endif
    Video_Read*  m_pVideoRead;
    VideoDecoder* m_pVideoDecoder;   // H.264 解码 Worker 线程
#ifdef USE_OPENGL
    OpenGLRender* m_pVideoWrite;       // 对方画面
    OpenGLRender* m_pVideoWriteLocal;  // 本地画中画
#else
    Video_Write* m_pVideoWrite;       // 对方画面
    Video_Write* m_pVideoWriteLocal;  // 本地画中画
#endif
    bool m_isVideoOn;
    bool m_isMicMuted;
    QPushButton* m_pEndMeetingBtn;
    // 录制
    WavRecorder* m_pRecorder;
    QPushButton* m_pRecordBtn;
    // 回放
    WavPlayer* m_pPlayer;
    QPushButton* m_pPlayBtn;
    // 音频 DSP（AEC + 降噪 + AGC）
    AudioDSP* m_pAudioDSP;
};

#endif // MEETINGDIALOG_H
