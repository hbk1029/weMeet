#include "meetingdialog.h"
#include "ui_meetingdialog.h"
#include <QClipboard>
#include <QApplication>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QTextCursor>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QComboBox>
#include <QFileDialog>
#include <QDir>
#include "VideoApi/h264_encoder.h"
#include "AudioApi/wav_recorder.h"
#include "AudioApi/wav_player.h"

MeetingDialog::MeetingDialog(QWidget *parent)
    : QWidget(parent), ui(new Ui::MeetingDialog),
      m_pTimer(nullptr), m_elapsedSec(0), m_meetingId(0), m_isCreator(false),
#ifdef USE_SDL2
      m_pSDLAudioRead(nullptr), m_pSDLAudioWrite(nullptr), m_pOpusEncoder(nullptr),
#else
      m_pAudioRead(nullptr), m_pAudioWrite(nullptr),
#endif
      m_pVideoRead(nullptr), m_pVideoWrite(nullptr), m_pVideoWriteLocal(nullptr),
      m_isVideoOn(false), m_isMicMuted(false), m_pEndMeetingBtn(nullptr),
      m_pRecorder(nullptr), m_pRecordBtn(nullptr), m_pPlayer(nullptr), m_pPlayBtn(nullptr)
{
    ui->setupUi(this);
    // 创建结束会议按钮（仅主持人可见）
    m_pEndMeetingBtn = new QPushButton(QString::fromUtf8("结束会议"), ui->wdg_controlBar);
    m_pEndMeetingBtn->setCursor(Qt::PointingHandCursor);
    m_pEndMeetingBtn->setStyleSheet(
        "QPushButton { background: #FF4D4F; color: white; border-radius: 8px; font-size: 14px;"
        "  font-family: Microsoft YaHei; border: none; padding: 8px 28px; }"
        "QPushButton:hover { background: #FF7875; }");
    m_pEndMeetingBtn->setVisible(false);
    connect(m_pEndMeetingBtn, &QPushButton::clicked, this, &MeetingDialog::sig_meetingEnd);
    QHBoxLayout* ctrlLayout = ui->wdg_controlBar->findChild<QHBoxLayout*>();
    if (ctrlLayout) {
        int idx = ctrlLayout->indexOf(ui->pb_exit);
        if (idx >= 0) ctrlLayout->insertWidget(idx + 1, m_pEndMeetingBtn);
    }

    //萌拍下拉框（无/兔耳朵/帽子）
    QComboBox* cbFunny = new QComboBox(ui->wdg_controlBar);
    cbFunny->setCursor(Qt::PointingHandCursor);
    cbFunny->addItem(QString::fromUtf8("无"));
    cbFunny->addItem(QString::fromUtf8("兔耳朵"));
    cbFunny->addItem(QString::fromUtf8("帽子"));
    cbFunny->setStyleSheet(
        "QComboBox { border: 1px solid #E5E6EB; border-radius: 8px; padding: 6px 12px;"
        "  font-size: 14px; font-family: Microsoft YaHei; background: white; }"
        "QComboBox:hover { border-color: #1677FF; }");
    cbFunny->setCurrentIndex(0);
    if (ctrlLayout) {
        int idx = ctrlLayout->indexOf(ui->pb_camera);
        if (idx >= 0) ctrlLayout->insertWidget(idx + 1, cbFunny);
    }
    connect(cbFunny, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
        if (m_pVideoRead) {
            // 跨线程调用：Video_Read 在 Worker 线程
            QMetaObject::invokeMethod(m_pVideoRead, "slot_setFunnyPic", Qt::QueuedConnection, Q_ARG(int, index));
            QMetaObject::invokeMethod(m_pVideoRead, "slot_setFaceDetect", Qt::QueuedConnection, Q_ARG(bool, index != 0));
        }
    });

    // 录制按钮
    m_pRecordBtn = new QPushButton(QString::fromUtf8("开始录制"), ui->wdg_controlBar);
    m_pRecordBtn->setCursor(Qt::PointingHandCursor);
    m_pRecordBtn->setStyleSheet(
        "QPushButton { background: #E5E6EB; color: #333; border-radius: 8px; font-size: 14px;"
        "  font-family: Microsoft YaHei; border: none; padding: 8px 16px; }"
        "QPushButton:hover { background: #D3D4D9; }");
    connect(m_pRecordBtn, &QPushButton::clicked, this, &MeetingDialog::slot_recordToggle);
    if (ctrlLayout) {
        int idx = ctrlLayout->indexOf(ui->pb_exit);
        if (idx >= 0) ctrlLayout->insertWidget(idx + 2, m_pRecordBtn);
    }

    // 播放录制按钮
    m_pPlayBtn = new QPushButton(QString::fromUtf8("播放录制"), ui->wdg_controlBar);
    m_pPlayBtn->setCursor(Qt::PointingHandCursor);
    m_pPlayBtn->setStyleSheet(
        "QPushButton { background: #E5E6EB; color: #333; border-radius: 8px; font-size: 14px;"
        "  font-family: Microsoft YaHei; border: none; padding: 8px 16px; }"
        "QPushButton:hover { background: #D3D4D9; }");
    connect(m_pPlayBtn, &QPushButton::clicked, this, &MeetingDialog::slot_playRecording);
    if (ctrlLayout) {
        int idx = ctrlLayout->indexOf(ui->pb_exit);
        if (idx >= 0) ctrlLayout->insertWidget(idx + 3, m_pPlayBtn);
    }

    //禁止 pb_send 被 Enter 键触发（避免和 le_input 的 returnPressed 重复）
    ui->pb_send->setAutoDefault(false);

    //计时器
    m_pTimer = new QTimer(this);
    connect(m_pTimer, &QTimer::timeout, this, &MeetingDialog::slot_timerTick);

    //回车发送
    connect(ui->le_input, &QLineEdit::returnPressed, this, &MeetingDialog::on_pb_send_clicked);
}

MeetingDialog::~MeetingDialog()
{
    delete ui;
}

void MeetingDialog::setUserName(const QString& name)
{
    setWindowTitle(QString::fromUtf8("会议 - %1").arg(name));
}

//设置会议信息
void MeetingDialog::setMeetingInfo(int meetingId, bool isCreator)
{
    m_meetingId = meetingId;
    m_isCreator = isCreator;
    ui->lb_meetingId->setText(QString::number(meetingId));
    ui->lw_memberList->clear();
    m_mapUserIdToItem.clear();
    ui->tb_messages->clear();
    m_elapsedSec = 0;
    ui->lb_timer->setText("00:00");
    // 萌拍重置为"无"
    QComboBox* cbFunny = ui->wdg_controlBar->findChild<QComboBox*>();
    if (cbFunny) cbFunny->setCurrentIndex(0);
    //音频采集播放初始化
#ifdef USE_SDL2
    if (m_pSDLAudioRead) {
        m_pSDLAudioRead->slot_closeAudio();
        delete m_pSDLAudioRead;
        m_pSDLAudioRead = nullptr;
    }
    if (m_pSDLAudioWrite) {
        delete m_pSDLAudioWrite;
        m_pSDLAudioWrite = nullptr;
    }
    if (m_pOpusEncoder) {
        delete m_pOpusEncoder;
        m_pOpusEncoder = nullptr;
    }
    m_pSDLAudioRead = new SDLAudioRead(this);
    m_pSDLAudioWrite = new SDLAudioWrite(this);
    m_pSDLAudioWrite->slot_openAudio();  // 播放设备始终打开，随时准备接收远端音频
    m_pOpusEncoder = new OpusEncoder(this);
    // 原始 PCM → WAV 录制（录制需要原始PCM，非编码后数据）
    connect(m_pSDLAudioRead, &SDLAudioRead::SIG_sendAudioFrame, this,
            [this](QByteArray data) {
        if (m_pRecorder && m_pRecorder->isRecording()) {
            m_pRecorder->writePcmData(data.data(), data.size());
        }
    });
    // 原始 PCM → Opus 编码 → 网络发送
    connect(m_pSDLAudioRead, &SDLAudioRead::SIG_sendAudioFrame, this,
            [this](QByteArray data) {
        m_pOpusEncoder->slot_encode(data.data(), data.size());
    });
    connect(m_pOpusEncoder, &OpusEncoder::sig_encodedData, this,
            [this](const char* data, int len) {
        static int sigCount = 0;
        if (++sigCount <= 3 || sigCount % 100 == 0)
            qDebug() << "[AUDIO_SEND] sig_meetingAudio frame#" << sigCount << " len=" << len;
        Q_EMIT sig_meetingAudio(data, len);
    });
#else
    if (m_pAudioRead) {
        m_pAudioRead->slot_pauseRecord();
        delete m_pAudioRead;
        m_pAudioRead = nullptr;
    }
    if (m_pAudioWrite) {
        delete m_pAudioWrite;
        m_pAudioWrite = nullptr;
    }
    m_pAudioRead = new Audio_Read(this);
    m_pAudioWrite = new Audio_Write(this);
    // 非 SDL2 路径也需在录制时写入 WAV
    connect(m_pAudioRead, &Audio_Read::sig_audioFrame, this,
            [this](const char* data, int len) {
        Q_EMIT sig_meetingAudio(data, len);
        if (m_pRecorder && m_pRecorder->isRecording()) {
            m_pRecorder->writePcmData(data, len);
        }
    });
#endif

    //视频采集播放初始化
    if (m_pVideoRead) {
        // 跨线程调用：Video_Read 在 Worker 线程，必须通过 invokeMethod
        QMetaObject::invokeMethod(m_pVideoRead, "slot_closeVideo", Qt::BlockingQueuedConnection);
        delete m_pVideoRead;
        m_pVideoRead = nullptr;
    }
    if (m_pVideoWrite) {
        delete m_pVideoWrite;
        m_pVideoWrite = nullptr;
    }
    if (m_pVideoWriteLocal) {
        delete m_pVideoWriteLocal;
        m_pVideoWriteLocal = nullptr;
    }
    // 不传 parent：Video_Read 通过 moveToThread 移到 Worker 线程，有 parent 会导致 moveToThread 静默失败
    m_pVideoRead = new Video_Read(nullptr);
#ifdef USE_OPENGL
    m_pVideoWrite = new OpenGLRender(ui->wdg_videoArea);
    m_pVideoWriteLocal = new OpenGLRender(ui->wdg_videoArea);
#else
    m_pVideoWrite = new Video_Write(ui->wdg_videoArea);
    m_pVideoWriteLocal = new Video_Write(ui->wdg_videoArea);
#endif
    m_isVideoOn = false;
    m_isMicMuted = true;
    ui->pb_microphone->setText(QString::fromUtf8("麦克风"));
    ui->pb_camera->setText(QString::fromUtf8("摄像头"));
    ui->wdg_videoArea->setVisible(true);
    m_pVideoWriteLocal->setVisible(false);

    connect(m_pVideoRead, &Video_Read::sig_videoFrame, this, &MeetingDialog::sig_videoFrame);

    // 本地预览: QImage 直通, 绕过 H.264 色度丢失
#ifdef USE_OPENGL
    connect(m_pVideoRead, &Video_Read::sig_localPreviewFrame,
            m_pVideoWriteLocal, &OpenGLRender::slot_updateImage);
#else
    connect(m_pVideoRead, &Video_Read::sig_localPreviewFrame,
            m_pVideoWriteLocal, [this](QImage img) {
        m_pVideoWriteLocal->slot_recvVideoFrame(nullptr, 0, 0);  // 先触发 JPEG 路径
        // Video_Write 无直接 setImage, 用 QImage 编码 JPEG 再走 JPEG 路径
        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        img.save(&buf, "JPG", 90);
        m_pVideoWriteLocal->slot_recvVideoFrame(ba.data(), ba.size(), 0);
    });
#endif

    // 暂停 H.264 本地预览: 改用 sig_localPreviewFrame QImage 直通
    // 网络传输 sig_videoFrame 保持 H.264

    // wdg_videoArea 布局
    QHBoxLayout* videoLayout = ui->wdg_videoArea->findChild<QHBoxLayout*>();
    if (!videoLayout) {
        videoLayout = new QHBoxLayout(ui->wdg_videoArea);
        videoLayout->setContentsMargins(0, 0, 0, 0);
        videoLayout->setSpacing(4);
    }
    videoLayout->addWidget(m_pVideoWrite, 3);
    m_pVideoWriteLocal->setMaximumSize(160, 120);
    m_pVideoWriteLocal->setMinimumSize(160, 120);
    videoLayout->addWidget(m_pVideoWriteLocal, 1);

    // 初始化录制/回放
    if (!m_pRecorder) {
        m_pRecorder = new WavRecorder(this);
    }
    if (!m_pPlayer) {
        m_pPlayer = new WavPlayer(this);
    }
    // 确保 records 目录存在
    QString recordsDir = QCoreApplication::applicationDirPath() + "/records/";
    QDir().mkpath(recordsDir);

    if (m_pEndMeetingBtn) m_pEndMeetingBtn->setVisible(m_isCreator);

    m_pTimer->start(1000);
}

//添加成员
void MeetingDialog::addMember(int userId, QString userName)
{
    if (m_mapUserIdToItem.count(userId) > 0) return;

    QListWidgetItem* item = new QListWidgetItem(ui->lw_memberList);
    item->setSizeHint(QSize(0, 44));

    QWidget* memberWidget = new QWidget();
    QVBoxLayout* outerLayout = new QVBoxLayout(memberWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addStretch();

    QHBoxLayout* innerLayout = new QHBoxLayout();
    innerLayout->setContentsMargins(8, 0, 8, 0);
    innerLayout->setSpacing(12);

    //头像圆片
    QLabel* avatar = new QLabel(memberWidget);
    avatar->setFixedSize(32, 32);
    avatar->setAlignment(Qt::AlignCenter);
    QString firstChar = userName.isEmpty() ? QString("?") : QString(userName.at(0));
    avatar->setText(firstChar);
    avatar->setStyleSheet(
        "background: #1677FF; color: white; border-radius: 16px;"
        "font-size: 14px; font-weight: bold; font-family: Microsoft YaHei;");

    QLabel* nameLabel = new QLabel(userName, memberWidget);
    nameLabel->setStyleSheet("font-size: 14px; color: #333; font-family: Microsoft YaHei;");

    innerLayout->addWidget(avatar);
    innerLayout->addWidget(nameLabel);
    innerLayout->addStretch();
    outerLayout->addLayout(innerLayout);
    outerLayout->addStretch();

    ui->lw_memberList->setItemWidget(item, memberWidget);
    m_mapUserIdToItem[userId] = item;
}

void MeetingDialog::removeMember(int userId)
{
    if (m_mapUserIdToItem.count(userId) == 0) return;
    QListWidgetItem* item = m_mapUserIdToItem[userId];
    int row = ui->lw_memberList->row(item);
    ui->lw_memberList->takeItem(row);
    delete item;
    m_mapUserIdToItem.erase(userId);
    //远端视频画面清空
    if (m_pVideoWrite) {
        m_pVideoWrite->slot_clearFrame();
    }
}

//清空远端视频
void MeetingDialog::clearRemoteVideo()
{
    if (m_pVideoWrite) {
        m_pVideoWrite->slot_clearFrame();
    }
}

//添加系统消息
void MeetingDialog::appendSystemMsg(QString msg)
{
    QTextCursor cursor = ui->tb_messages->textCursor();
    cursor.movePosition(QTextCursor::End);

    bool isEmpty = ui->tb_messages->document()->isEmpty();

    QTextBlockFormat blockFmt;
    blockFmt.setAlignment(Qt::AlignCenter);
    blockFmt.setTopMargin(3);
    blockFmt.setBottomMargin(3);
    blockFmt.setLineHeight(160, QTextBlockFormat::ProportionalHeight);

    QTextCharFormat charFmt;
    charFmt.setForeground(QColor("#999999"));
    charFmt.setFontPointSize(12);

    if (!isEmpty) {
        cursor.insertBlock(blockFmt, charFmt);
    } else {
        cursor.setBlockFormat(blockFmt);
        cursor.setCharFormat(charFmt);
    }

    cursor.insertText(msg, charFmt);

    QScrollBar* sb = ui->tb_messages->verticalScrollBar();
    if (sb) sb->setValue(sb->maximum());
}

//添加聊天消息
void MeetingDialog::appendChatMsg(int userId, QString userName, QString content)
{
    Q_UNUSED(userId);
    QFile f(QString("C:\\temp\\chat_debug_%1.log").arg(QCoreApplication::applicationPid()));
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream s(&f);
        s << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
          << " APPEND_MSG user=" << userId << " content=" << content << "\n";
        f.close();
    }

    QTextCursor cursor = ui->tb_messages->textCursor();
    cursor.movePosition(QTextCursor::End);

    bool isEmpty = ui->tb_messages->document()->isEmpty();

    QTextBlockFormat blockFmt;
    blockFmt.setAlignment(Qt::AlignLeft);
    blockFmt.setTopMargin(3);
    blockFmt.setBottomMargin(3);
    blockFmt.setLineHeight(160, QTextBlockFormat::ProportionalHeight);

    QTextCharFormat nameFmt;
    nameFmt.setForeground(QColor("#1677FF"));
    nameFmt.setFontWeight(QFont::Bold);

    QTextCharFormat defaultFmt;

    if (!isEmpty) {
        cursor.insertBlock(blockFmt, defaultFmt);
    } else {
        cursor.setBlockFormat(blockFmt);
        cursor.setCharFormat(defaultFmt);
    }

    cursor.insertText(userName, nameFmt);
    cursor.insertText(": " + content, defaultFmt);

    QScrollBar* sb = ui->tb_messages->verticalScrollBar();
    if (sb) sb->setValue(sb->maximum());
}

//发送消息
void MeetingDialog::on_pb_send_clicked()
{
    QString text = ui->le_input->text().trimmed();
    if (text.isEmpty()) return;
    ui->le_input->clear();
    Q_EMIT sig_meetingChat(text);
}

//退出会议
void MeetingDialog::on_pb_exit_clicked()
{
    m_pTimer->stop();
    // 正在录制则自动停止
    if (m_pRecorder && m_pRecorder->isRecording()) {
        m_pRecorder->stopRecording();
        m_pRecordBtn->setText(QString::fromUtf8("开始录制"));
        m_pRecordBtn->setStyleSheet(
            "QPushButton { background: #E5E6EB; color: #333; border-radius: 8px; font-size: 14px;"
            "  font-family: Microsoft YaHei; border: none; padding: 8px 16px; }"
            "QPushButton:hover { background: #D3D4D9; }");
    }
    Q_EMIT sig_meetingExit();
}

//复制会议号
void MeetingDialog::on_pb_copy_clicked()
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(QString::number(m_meetingId));
    ui->pb_copy->setText("已复制");
    QTimer::singleShot(2000, this, [this]() {
        ui->pb_copy->setText("复制");
    });
}

//麦克风
void MeetingDialog::on_pb_microphone_clicked()
{
#ifdef USE_SDL2
    if (m_isMicMuted) {
        m_pSDLAudioRead->slot_openAudio();
        ui->pb_microphone->setText(QString::fromUtf8("关闭麦克风"));
        m_isMicMuted = false;
    } else {
        m_pSDLAudioRead->slot_closeAudio();
        ui->pb_microphone->setText(QString::fromUtf8("麦克风"));
        m_isMicMuted = true;
    }
#else
    if (m_isMicMuted) {
        m_pAudioRead->slot_startRecord();
        ui->pb_microphone->setText(QString::fromUtf8("关闭麦克风"));
        m_isMicMuted = false;
    } else {
        m_pAudioRead->slot_pauseRecord();
        ui->pb_microphone->setText(QString::fromUtf8("麦克风"));
        m_isMicMuted = true;
    }
#endif
}

//摄像头
void MeetingDialog::on_pb_camera_clicked()
{
    if (m_isVideoOn) {
        // 跨线程调用：Video_Read 在 Worker 线程
        QMetaObject::invokeMethod(m_pVideoRead, "slot_closeVideo", Qt::BlockingQueuedConnection);
        m_pVideoWriteLocal->setVisible(false);
        ui->pb_camera->setText(QString::fromUtf8("摄像头"));
        m_isVideoOn = false;
        Q_EMIT sig_videoFrame(QByteArray()); // 空帧通知远端清空画面
    } else {
        // 跨线程调用：确保 QTimer::start() 在 Worker 线程执行
        QMetaObject::invokeMethod(m_pVideoRead, "slot_openVideo", Qt::QueuedConnection);
        m_pVideoWriteLocal->setVisible(true);
        ui->pb_camera->setText(QString::fromUtf8("关闭摄像头"));
        m_isVideoOn = true;
    }
}

//计时器
void MeetingDialog::slot_timerTick()
{
    m_elapsedSec++;
    int min = m_elapsedSec / 60;
    int sec = m_elapsedSec % 60;
    ui->lb_timer->setText(QString("%1:%2")
        .arg(min, 2, 10, QChar('0'))
        .arg(sec, 2, 10, QChar('0')));
}

//录制切换
void MeetingDialog::slot_recordToggle()
{
    if (!m_pRecorder) return;

    if (m_pRecorder->isRecording()) {
        m_pRecorder->stopRecording();
        m_pRecordBtn->setText(QString::fromUtf8("开始录制"));
        m_pRecordBtn->setStyleSheet(
            "QPushButton { background: #E5E6EB; color: #333; border-radius: 8px; font-size: 14px;"
            "  font-family: Microsoft YaHei; border: none; padding: 8px 16px; }"
            "QPushButton:hover { background: #D3D4D9; }");
        appendSystemMsg(QString::fromUtf8("录制已停止"));
    } else {
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString filePath = QCoreApplication::applicationDirPath()
                           + "/records/recording_" + timestamp + ".wav";
        if (m_pRecorder->startRecording(filePath)) {
            m_pRecordBtn->setText(QString::fromUtf8("停止录制"));
            m_pRecordBtn->setStyleSheet(
                "QPushButton { background: #FF4D4F; color: white; border-radius: 8px; font-size: 14px;"
                "  font-family: Microsoft YaHei; border: none; padding: 8px 16px; }"
                "QPushButton:hover { background: #FF7875; }");
            appendSystemMsg(QString::fromUtf8("开始录制"));
        }
    }
}

//播放录制文件
void MeetingDialog::slot_playRecording()
{
    if (!m_pPlayer) return;

    QString filePath = QFileDialog::getOpenFileName(this,
        QString::fromUtf8("选择录制文件"),
        QCoreApplication::applicationDirPath() + "/records/",
        QString::fromUtf8("WAV 文件 (*.wav)"));

    if (filePath.isEmpty()) return;

    m_pPlayer->play(filePath);
    appendSystemMsg(QString::fromUtf8("播放录制: ") + filePath);
}
