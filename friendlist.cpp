#include "friendlist.h"
#include "ui_friendlist.h"
#include <QAbstractItemView>
#include <QMenu>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QDateTime>

FriendList::FriendList(QWidget *parent)
    : QWidget(parent), ui(new Ui::FriendList), m_iconId(8)
{
    ui->setupUi(this);
    //初始化导航按钮激活状态，默认显示会议页
    setNavButtonActive(ui->pb_meeting, true);

    //添加好友按钮改为蓝色
    ui->pb_addFriend->setStyleSheet(
        "QPushButton { background: #1677FF; color: white; border-radius: 8px; font-size: 14px; font-family: Microsoft YaHei; border: none; padding: 8px 20px; }"
        "QPushButton:hover { background: #4096FF; }");

    //搜索过滤
    connect(ui->le_search, &QLineEdit::textChanged,
            this, &FriendList::on_le_search_textChanged);

    //好友列表右键菜单
    ui->lw_friendList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->lw_friendList, &QListWidget::customContextMenuRequested,
            this, &FriendList::on_lw_friendList_menuRequested);

    // --- 创建会议视图控件 ---
    // 快速会议卡片
    m_pQuickCard = new QWidget(ui->wdg_content);
    m_pQuickCard->setStyleSheet("QWidget#quickCard { background: white; border-radius: 12px; }");
    m_pQuickCard->setObjectName("quickCard");
    m_pQuickCard->setMinimumHeight(52);
    QVBoxLayout* quickLayout = new QVBoxLayout(m_pQuickCard);
    quickLayout->setContentsMargins(20, 8, 20, 8);
    QLabel* quickTitle = new QLabel(QString::fromUtf8("快速会议"), m_pQuickCard);
    quickTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #333; font-family: Microsoft YaHei;");
    QLabel* quickDesc = new QLabel(QString::fromUtf8("立即创建会议，邀请好友加入"), m_pQuickCard);
    quickDesc->setStyleSheet("font-size: 13px; color: #999; font-family: Microsoft YaHei;");
    m_pQuickMeetingBtn = new QPushButton(QString::fromUtf8("创建会议"), m_pQuickCard);
    m_pQuickMeetingBtn->setCursor(Qt::PointingHandCursor);
    m_pQuickMeetingBtn->setFixedSize(100, 36);
    m_pQuickMeetingBtn->setStyleSheet(
        "QPushButton { background: #1677FF; color: white; border-radius: 8px; font-size: 14px; font-family: Microsoft YaHei; border: none; }"
        "QPushButton:hover { background: #4096FF; }");
    connect(m_pQuickMeetingBtn, &QPushButton::clicked, this, &FriendList::on_pb_quickMeeting_clicked);
    QHBoxLayout* quickRow = new QHBoxLayout();
    quickRow->addWidget(quickTitle);
    quickRow->addStretch();
    quickRow->addWidget(m_pQuickMeetingBtn);
    quickLayout->addLayout(quickRow);
    quickLayout->addWidget(quickDesc);
    ui->vl_content->addWidget(m_pQuickCard);

    // 加入会议卡片
    m_pJoinCard = new QWidget(ui->wdg_content);
    m_pJoinCard->setStyleSheet("QWidget#joinCard { background: white; border-radius: 12px; }");
    m_pJoinCard->setObjectName("joinCard");
    m_pJoinCard->setMinimumHeight(52);
    QVBoxLayout* joinLayout = new QVBoxLayout(m_pJoinCard);
    joinLayout->setContentsMargins(20, 8, 20, 8);
    QLabel* joinTitle = new QLabel(QString::fromUtf8("加入会议"), m_pJoinCard);
    joinTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #333; font-family: Microsoft YaHei;");
    QLabel* joinDesc = new QLabel(QString::fromUtf8("输入9位会议号加入已有会议"), m_pJoinCard);
    joinDesc->setStyleSheet("font-size: 13px; color: #999; font-family: Microsoft YaHei;");
    m_pMeetingCodeInput = new QLineEdit(m_pJoinCard);
    m_pMeetingCodeInput->setPlaceholderText(QString::fromUtf8("请输入9位会议号"));
    m_pMeetingCodeInput->setMaxLength(9);
    m_pMeetingCodeInput->setFixedWidth(180);
    m_pMeetingCodeInput->setStyleSheet(
        "QLineEdit { border: 1px solid #E5E6EB; border-radius: 8px; padding: 6px 12px; font-size: 14px; font-family: Microsoft YaHei; }"
        "QLineEdit:focus { border-color: #1677FF; }");
    m_pJoinByCodeBtn = new QPushButton(QString::fromUtf8("加入"), m_pJoinCard);
    m_pJoinByCodeBtn->setCursor(Qt::PointingHandCursor);
    m_pJoinByCodeBtn->setFixedSize(72, 36);
    m_pJoinByCodeBtn->setStyleSheet(
        "QPushButton { background: #1677FF; color: white; border-radius: 8px; font-size: 14px; font-family: Microsoft YaHei; border: none; }"
        "QPushButton:hover { background: #4096FF; }");
    connect(m_pJoinByCodeBtn, &QPushButton::clicked, this, [this]() {
        if (m_pMeetingCodeInput) {
            QString code = m_pMeetingCodeInput->text().trimmed();
            if (code.length() == 9) {
                bool isAllDigit = true;
                for (const QChar& ch : code) {
                    if (!ch.isDigit()) { isAllDigit = false; break; }
                }
                if (isAllDigit) {
                    Q_EMIT sig_meetingJoinHistory(code);
                } else {
                    QMessageBox::about(this, QString::fromUtf8("提示"), QString::fromUtf8("会议号必须为9位数字"));
                }
            } else {
                QMessageBox::about(this, QString::fromUtf8("提示"), QString::fromUtf8("请输入9位数字会议号"));
            }
        }
    });
    QHBoxLayout* joinRow = new QHBoxLayout();
    joinRow->addWidget(joinTitle);
    joinRow->addStretch();
    joinRow->addWidget(m_pMeetingCodeInput);
    joinRow->addWidget(m_pJoinByCodeBtn);
    joinLayout->addLayout(joinRow);
    joinLayout->addWidget(joinDesc);
    ui->vl_content->addWidget(m_pJoinCard);

    // 会议历史
    QLabel* historyLabel = new QLabel(QString::fromUtf8("会议历史"), ui->wdg_content);
    historyLabel->setObjectName("lb_meetingHistory");
    historyLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #666; font-family: Microsoft YaHei; padding: 4px 0;");
    ui->vl_content->addWidget(historyLabel);

    // 会议历史容器（堆叠列表 + 空状态）
    m_pHistoryStack = new QStackedWidget(ui->wdg_content);
    m_pHistoryStack->setStyleSheet(
        "QStackedWidget { background: transparent; border: none; }");
    ui->vl_content->addWidget(m_pHistoryStack);

    m_pMeetingHistory = new QListWidget();
    m_pMeetingHistory->setStyleSheet(
        "QListWidget { background: transparent; border: none; font-family: Microsoft YaHei; }"
        "QListWidget::item { background: white; border-radius: 12px; margin: 0 0 6px 0; padding: 0; }"
        "QListWidget::item:hover { background: #F0F5FF; }");
    connect(m_pMeetingHistory, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (item) {
            QString id = item->data(Qt::UserRole).toString();
            if (!id.isEmpty()) Q_EMIT sig_meetingJoinHistory(id);
        }
    });
    m_pHistoryStack->addWidget(m_pMeetingHistory);

    m_pEmptyPlaceholder = new QLabel();
    m_pEmptyPlaceholder->setAlignment(Qt::AlignCenter);
    m_pEmptyPlaceholder->setText(QString::fromUtf8(
        "<p style='font-size:40px; color:#C0C4CC; margin:0;'>&#128466;</p>"
        "<p style='font-size:14px; color:#999; margin:8px 0 4px;'>暂无会议记录</p>"
        "<p style='font-size:12px; color:#BBB; margin:0;'>快速发起一场会议吧</p>"));
    m_pEmptyPlaceholder->setStyleSheet("border: none; background: transparent; font-family: Microsoft YaHei;");
    m_pHistoryStack->addWidget(m_pEmptyPlaceholder);

    loadMeetingHistory(); // 初始显示空状态占位

    // 好友请求按钮 + 好友数（加到通讯录操作栏，添加好友之后）
    m_pFriendRequestsBtn = new QPushButton(QString::fromUtf8("好友请求"), ui->wdg_content);
    m_pFriendRequestsBtn->setCursor(Qt::PointingHandCursor);
    m_pFriendRequestsBtn->setStyleSheet(
        "QPushButton { background: white; color: #1677FF; border: 1px solid #1677FF; border-radius: 8px;"
        "  font-size: 14px; font-family: Microsoft YaHei; padding: 8px 20px; }"
        "QPushButton:hover { background: #F0F5FF; }");
    m_pFriendRequestsBtn->setVisible(false);
    connect(m_pFriendRequestsBtn, &QPushButton::clicked, this, [this]() {
        Q_EMIT sig_showFriendRequests();
    });
    ui->hl_actions->insertWidget(3, m_pFriendRequestsBtn);

    m_pFriendCountLabel = new QLabel(QString::fromUtf8("共 0 位好友"), ui->wdg_content);
    m_pFriendCountLabel->setStyleSheet("color: #999; font-size: 13px; font-family: Microsoft YaHei;");
    m_pFriendCountLabel->setVisible(false);
    ui->hl_actions->insertWidget(4, m_pFriendCountLabel);

    // 初始隐藏通讯录控件，默认显示会议视图
    ui->wdg_profile->setVisible(false);
    ui->le_search->setVisible(false);
    ui->lw_friendList->setVisible(false);
    ui->pb_addFriend->setVisible(false);
    ui->pb_createMeeting->setVisible(false);
    ui->pb_joinMeeting->setVisible(false);
}

FriendList::~FriendList()
{
    delete ui;
}

//设置导航按钮激活样式
void FriendList::setNavButtonActive(QPushButton* button, bool active)
{
    if (active) {
        button->setProperty("active", true);
    } else {
        button->setProperty("active", false);
    }
    //强制刷新样式
    button->style()->unpolish(button);
    button->style()->polish(button);
}

//通讯录导航
void FriendList::on_pb_contact_clicked()
{
    setNavButtonActive(ui->pb_contact, true);
    setNavButtonActive(ui->pb_meeting, false);
    setNavButtonActive(ui->pb_settings, false);
    // 显示通讯录控件
    ui->wdg_profile->setVisible(true);
    ui->le_search->setVisible(true);
    ui->lw_friendList->setVisible(true);
    ui->pb_addFriend->setVisible(true);
    ui->pb_createMeeting->setVisible(false);
    ui->pb_joinMeeting->setVisible(false);
    if (m_pFriendRequestsBtn) m_pFriendRequestsBtn->setVisible(true);
    if (m_pFriendCountLabel) m_pFriendCountLabel->setVisible(true);
    // 隐藏会议视图控件
    if (m_pQuickCard) m_pQuickCard->setVisible(false);
    if (m_pJoinCard) m_pJoinCard->setVisible(false);
    if (m_pHistoryStack) m_pHistoryStack->setVisible(false);
    QLabel* historyLabel = ui->wdg_content->findChild<QLabel*>("lb_meetingHistory");
    if (historyLabel) historyLabel->setVisible(false);
}

//会议导航
void FriendList::on_pb_meeting_clicked()
{
    setNavButtonActive(ui->pb_contact, false);
    setNavButtonActive(ui->pb_meeting, true);
    setNavButtonActive(ui->pb_settings, false);
    // 显示会议视图控件
    ui->wdg_profile->setVisible(false);
    if (m_pQuickCard) m_pQuickCard->setVisible(true);
    if (m_pJoinCard) m_pJoinCard->setVisible(true);
    if (m_pHistoryStack) m_pHistoryStack->setVisible(true);
    QLabel* historyLabel = ui->wdg_content->findChild<QLabel*>("lb_meetingHistory");
    if (historyLabel) historyLabel->setVisible(true);
    // 隐藏通讯录控件
    ui->le_search->setVisible(false);
    ui->lw_friendList->setVisible(false);
    ui->pb_addFriend->setVisible(false);
    ui->pb_createMeeting->setVisible(false);
    ui->pb_joinMeeting->setVisible(false);
    if (m_pFriendRequestsBtn) m_pFriendRequestsBtn->setVisible(false);
    if (m_pFriendCountLabel) m_pFriendCountLabel->setVisible(false);
}

//设置导航
void FriendList::on_pb_settings_clicked()
{
    setNavButtonActive(ui->pb_contact, false);
    setNavButtonActive(ui->pb_meeting, false);
    setNavButtonActive(ui->pb_settings, true);
    // 显示通讯录视图（设置页暂用通讯录占位）
    ui->wdg_profile->setVisible(true);
    ui->le_search->setVisible(true);
    ui->lw_friendList->setVisible(true);
    ui->pb_addFriend->setVisible(true);
    ui->pb_createMeeting->setVisible(false);
    ui->pb_joinMeeting->setVisible(false);
    if (m_pFriendRequestsBtn) m_pFriendRequestsBtn->setVisible(true);
    if (m_pFriendCountLabel) m_pFriendCountLabel->setVisible(true);
    if (m_pQuickCard) m_pQuickCard->setVisible(false);
    if (m_pJoinCard) m_pJoinCard->setVisible(false);
    if (m_pHistoryStack) m_pHistoryStack->setVisible(false);
    QLabel* historyLabel = ui->wdg_content->findChild<QLabel*>("lb_meetingHistory");
    if (historyLabel) historyLabel->setVisible(false);
}

//创建会议
void FriendList::on_pb_createMeeting_clicked()
{
    Q_EMIT sig_meetingCreate();
}

//加入会议
void FriendList::on_pb_joinMeeting_clicked()
{
    Q_EMIT sig_meetingJoin();
}

//添加好友
void FriendList::on_pb_addFriend_clicked()
{
    Q_EMIT sig_addFriendItem();
}

//设置用户信息
void FriendList::setUserInfo(QString name, int iconId, QString feeling)
{
    ui->lb_userName->setText(name);
    ui->lb_userFeeling->setText(feeling);
    m_iconId = iconId;
}

//添加好友项
void FriendList::addFriendItem(FriendItem* item)
{
    QListWidgetItem* listItem = new QListWidgetItem(ui->lw_friendList);
    listItem->setSizeHint(QSize(0, 64));
    item->setStyleSheet("");
    ui->lw_friendList->setItemWidget(listItem, item);
    if (m_pFriendCountLabel) {
        m_pFriendCountLabel->setText(QString::fromUtf8("共 %1 位好友").arg(ui->lw_friendList->count()));
    }
}

//移除好友项
void FriendList::removeFriendItem(FriendItem* item)
{
    for (int i = 0; i < ui->lw_friendList->count(); ++i) {
        QListWidgetItem* listItem = ui->lw_friendList->item(i);
        if (ui->lw_friendList->itemWidget(listItem) == item) {
            ui->lw_friendList->takeItem(i);
            delete listItem;
            if (m_pFriendCountLabel) {
                m_pFriendCountLabel->setText(QString::fromUtf8("共 %1 位好友").arg(ui->lw_friendList->count()));
            }
            return;
        }
    }
}

//搜索过滤好友
void FriendList::on_le_search_textChanged(const QString& text)
{
    for (int i = 0; i < ui->lw_friendList->count(); ++i) {
        QListWidgetItem* listItem = ui->lw_friendList->item(i);
        QWidget* widget = ui->lw_friendList->itemWidget(listItem);
        FriendItem* friendItem = qobject_cast<FriendItem*>(widget);
        if (friendItem) {
            bool match = text.isEmpty() ||
                friendItem->getFriendName().contains(text, Qt::CaseInsensitive);
            listItem->setHidden(!match);
        }
    }
}

//右键菜单删除好友
void FriendList::on_lw_friendList_menuRequested(const QPoint& pos)
{
    QListWidgetItem* listItem = ui->lw_friendList->itemAt(pos);
    if (!listItem) return;

    QWidget* widget = ui->lw_friendList->itemWidget(listItem);
    FriendItem* friendItem = qobject_cast<FriendItem*>(widget);
    if (!friendItem) return;

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background: white; border: 1px solid #E5E6EB; border-radius: 8px; padding: 4px; }"
        "QMenu::item { padding: 8px 24px; font-size: 14px; font-family: Microsoft YaHei; color: #333; }"
        "QMenu::item:hover { background: #F0F5FF; color: #FF4D4F; }");
    QAction* deleteAction = menu.addAction(QString::fromUtf8("删除好友"));
    QAction* selected = menu.exec(ui->lw_friendList->mapToGlobal(pos));
    if (selected == deleteAction) {
        int ret = QMessageBox::question(this, QString::fromUtf8("删除好友"),
            QString::fromUtf8("确定要删除好友 %1 吗？").arg(friendItem->getFriendName()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            Q_EMIT sig_deleteFriend(friendItem->getFriendId());
        }
    }
}

// 快速会议
void FriendList::on_pb_quickMeeting_clicked()
{
    Q_EMIT sig_meetingCreate();
}

void FriendList::addMeetingHistory(QString meetingId)
{
    for (int i = 0; i < m_meetingHistory.size(); ++i) {
        if (m_meetingHistory[i].id == meetingId) {
            m_meetingHistory.removeAt(i);
            break;
        }
    }
    MeetingRecord rec;
    rec.id = meetingId;
    rec.timeStr = QDateTime::currentDateTime().toString("MM-dd hh:mm");
    rec.isActive = true;
    m_meetingHistory.prepend(rec);
    if (m_meetingHistory.size() > 20) m_meetingHistory.removeLast();
    rebuildHistoryList();
}

void FriendList::updateMeetingStatus(QString meetingId, bool isActive)
{
    for (int i = 0; i < m_meetingHistory.size(); ++i) {
        if (m_meetingHistory[i].id == meetingId) {
            m_meetingHistory[i].isActive = isActive;
            break;
        }
    }
    rebuildHistoryList();
}

void FriendList::loadMeetingHistory()
{
    rebuildHistoryList();
}

void FriendList::rebuildHistoryList()
{
    if (!m_pMeetingHistory || !m_pHistoryStack) return;
    m_pMeetingHistory->clear();
    if (m_meetingHistory.isEmpty()) {
        m_pHistoryStack->setCurrentIndex(1);
        return;
    }
    m_pHistoryStack->setCurrentIndex(0);
    const char* kColors[] = {"#1677FF","#52C41A","#722ED1","#FA8C16","#13C2C2","#EB2F96"};
    for (const auto& rec : m_meetingHistory) {
        int ci = rec.id.toInt() % 6;
        QListWidgetItem* item = new QListWidgetItem();
        item->setSizeHint(QSize(0, 62));
        item->setData(Qt::UserRole, rec.id);
        m_pMeetingHistory->addItem(item);
        QWidget* w = new QWidget();
        QHBoxLayout* row = new QHBoxLayout(w);
        row->setContentsMargins(14, 8, 14, 8);
        row->setSpacing(12);
        QLabel* avatar = new QLabel(QString::fromUtf8("会"));
        avatar->setFixedSize(36, 36);
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setStyleSheet(QString(
            "background: %1; color: white; border-radius: 18px;"
            "font-size: 14px; font-weight: bold; font-family: Microsoft YaHei;").arg(kColors[ci]));
        row->addWidget(avatar);
        QVBoxLayout* textCol = new QVBoxLayout();
        textCol->setSpacing(2);
        QLabel* idLabel = new QLabel(QString::fromUtf8("会议 %1").arg(rec.id));
        idLabel->setStyleSheet("font-size: 14px; color: #333; font-family: Microsoft YaHei;");
        QLabel* timeLabel = new QLabel(rec.timeStr);
        timeLabel->setStyleSheet("font-size: 12px; color: #999; font-family: Microsoft YaHei;");
        textCol->addWidget(idLabel);
        textCol->addWidget(timeLabel);
        row->addLayout(textCol);
        row->addStretch();
        if (rec.isActive) {
            QLabel* tag = new QLabel(QString::fromUtf8("加入"));
            tag->setStyleSheet(
                "background: #E6F7FF; color: #1677FF; border-radius: 8px; padding: 2px 10px;"
                "font-size: 11px; font-family: Microsoft YaHei;");
            row->addWidget(tag);
        } else {
            QLabel* tag = new QLabel(QString::fromUtf8("已结束"));
            tag->setStyleSheet(
                "background: #F5F6FA; color: #999; border-radius: 8px; padding: 2px 10px;"
                "font-size: 11px; font-family: Microsoft YaHei;");
            row->addWidget(tag);
        }
        m_pMeetingHistory->setItemWidget(item, w);
    }
}

void FriendList::updateFriendRequestBadge(int count)
{
    if (!m_pFriendRequestsBtn) return;
    if (count > 0) {
        m_pFriendRequestsBtn->setText(QString::fromUtf8("好友请求 (%1)").arg(count));
        m_pFriendRequestsBtn->setStyleSheet(
            "QPushButton { background: #FFF1F0; color: #FF4D4F; border: 1px solid #FF4D4F; border-radius: 8px;"
            "  font-size: 14px; font-family: Microsoft YaHei; padding: 8px 20px; }"
            "QPushButton:hover { background: #FFE7E7; }");
    } else {
        m_pFriendRequestsBtn->setText(QString::fromUtf8("好友请求"));
        m_pFriendRequestsBtn->setStyleSheet(
            "QPushButton { background: white; color: #1677FF; border: 1px solid #1677FF; border-radius: 8px;"
            "  font-size: 14px; font-family: Microsoft YaHei; padding: 8px 20px; }"
            "QPushButton:hover { background: #F0F5FF; }");
    }
}
