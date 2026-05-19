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

MeetingDialog::MeetingDialog(QWidget *parent)
    : QWidget(parent), ui(new Ui::MeetingDialog),
      m_pTimer(nullptr), m_elapsedSec(0), m_meetingId(0), m_isCreator(false),
      m_pAudioRead(nullptr), m_pAudioWrite(nullptr),
      m_pVideoRead(nullptr), m_pVideoWrite(nullptr), m_pVideoWriteLocal(nullptr),
      m_isVideoOn(false), m_isMicMuted(false), m_pEndMeetingBtn(nullptr)
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
    //音频采集播放初始化
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
    connect(m_pAudioRead, &Audio_Read::sig_audioFrame, this, &MeetingDialog::sig_meetingAudio);

    //视频采集播放初始化
    if (m_pVideoRead) {
        m_pVideoRead->slot_closeVideo();
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
    m_pVideoRead = new Video_Read(this);
    m_pVideoWrite = new Video_Write(ui->wdg_videoArea);
    m_pVideoWriteLocal = new Video_Write(ui->wdg_videoArea);
    m_isVideoOn = false;
    m_isMicMuted = true;
    ui->pb_microphone->setText(QString::fromUtf8("麦克风"));
    ui->pb_camera->setText(QString::fromUtf8("摄像头"));
    ui->wdg_videoArea->setVisible(true);
    m_pVideoWriteLocal->setVisible(false);

    connect(m_pVideoRead, &Video_Read::sig_videoFrame, this, &MeetingDialog::sig_videoFrame);
    connect(m_pVideoRead, &Video_Read::sig_videoFrame,
            m_pVideoWriteLocal, &Video_Write::slot_recvVideoFrame);

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
    if (m_isMicMuted) {
        m_pAudioRead->slot_startRecord();
        ui->pb_microphone->setText(QString::fromUtf8("关闭麦克风"));
        m_isMicMuted = false;
    } else {
        m_pAudioRead->slot_pauseRecord();
        ui->pb_microphone->setText(QString::fromUtf8("麦克风"));
        m_isMicMuted = true;
    }
}

//摄像头
void MeetingDialog::on_pb_camera_clicked()
{
    if (m_isVideoOn) {
        m_pVideoRead->slot_closeVideo();
        m_pVideoWriteLocal->setVisible(false);
        ui->pb_camera->setText(QString::fromUtf8("摄像头"));
        m_isVideoOn = false;
        Q_EMIT sig_videoFrame(nullptr, 0); // 通知远端清空画面
    } else {
        m_pVideoRead->slot_openVideo();
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
