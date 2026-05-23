#include "friendlist.h"
#include "ui_friendlist.h"
#include <QMenu>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QCoreApplication>
#include <QDir>

FriendList::FriendList(QWidget *parent)
    : QWidget(parent), ui(new Ui::FriendList), m_iconId(8)
{
    ui->setupUi(this);
    setNavButtonActive(ui->pb_meeting, true);

    // 搜索过滤
    connect(ui->le_search, &QLineEdit::textChanged,
            this, &FriendList::on_le_search_textChanged);

    // 好友列表右键菜单（auto-connect 绑定 on_lw_friendList_customContextMenuRequested）
    ui->lw_friendList->setContextMenuPolicy(Qt::CustomContextMenu);

    // 会议历史列表点击
    connect(ui->lw_meetingHistory, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (item) {
            QString id = item->data(Qt::UserRole).toString();
            if (!id.isEmpty()) Q_EMIT sig_meetingJoinHistory(id);
        }
    });

    loadMeetingHistory();

    // 好友请求按钮 + 好友计数（动态插入到 hl_actions）
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

void FriendList::setNavButtonActive(QPushButton* button, bool active)
{
    button->setProperty("active", active);
    button->style()->unpolish(button);
    button->style()->polish(button);
}

void FriendList::on_pb_contact_clicked()
{
    setNavButtonActive(ui->pb_contact, true);
    setNavButtonActive(ui->pb_meeting, false);
    setNavButtonActive(ui->pb_settings, false);
    ui->wdg_profile->setVisible(true);
    ui->le_search->setVisible(true);
    ui->lw_friendList->setVisible(true);
    ui->pb_addFriend->setVisible(true);
    ui->pb_createMeeting->setVisible(false);
    ui->pb_joinMeeting->setVisible(false);
    if (m_pFriendRequestsBtn) m_pFriendRequestsBtn->setVisible(true);
    if (m_pFriendCountLabel) m_pFriendCountLabel->setVisible(true);
    ui->wdg_quickCard->setVisible(false);
    ui->wdg_joinCard->setVisible(false);
    ui->stk_history->setVisible(false);
    ui->lb_meetingHistory->setVisible(false);
}

void FriendList::on_pb_meeting_clicked()
{
    setNavButtonActive(ui->pb_contact, false);
    setNavButtonActive(ui->pb_meeting, true);
    setNavButtonActive(ui->pb_settings, false);
    ui->wdg_profile->setVisible(false);
    ui->wdg_quickCard->setVisible(true);
    ui->wdg_joinCard->setVisible(true);
    ui->stk_history->setVisible(true);
    ui->lb_meetingHistory->setVisible(true);
    ui->le_search->setVisible(false);
    ui->lw_friendList->setVisible(false);
    ui->pb_addFriend->setVisible(false);
    ui->pb_createMeeting->setVisible(false);
    ui->pb_joinMeeting->setVisible(false);
    if (m_pFriendRequestsBtn) m_pFriendRequestsBtn->setVisible(false);
    if (m_pFriendCountLabel) m_pFriendCountLabel->setVisible(false);
}

void FriendList::on_pb_settings_clicked()
{
    setNavButtonActive(ui->pb_contact, false);
    setNavButtonActive(ui->pb_meeting, false);
    setNavButtonActive(ui->pb_settings, true);
    ui->wdg_profile->setVisible(true);
    ui->le_search->setVisible(true);
    ui->lw_friendList->setVisible(true);
    ui->pb_addFriend->setVisible(true);
    ui->pb_createMeeting->setVisible(false);
    ui->pb_joinMeeting->setVisible(false);
    if (m_pFriendRequestsBtn) m_pFriendRequestsBtn->setVisible(true);
    if (m_pFriendCountLabel) m_pFriendCountLabel->setVisible(true);
    ui->wdg_quickCard->setVisible(false);
    ui->wdg_joinCard->setVisible(false);
    ui->stk_history->setVisible(false);
    ui->lb_meetingHistory->setVisible(false);
}

void FriendList::on_pb_createMeeting_clicked()
{
    Q_EMIT sig_meetingCreate();
}

void FriendList::on_pb_joinMeeting_clicked()
{
    Q_EMIT sig_meetingJoin();
}

void FriendList::on_pb_addFriend_clicked()
{
    Q_EMIT sig_addFriendItem();
}

void FriendList::on_pb_joinByCode_clicked()
{
    QString code = ui->le_meetingCode->text().trimmed();
    if (code.length() != 9) {
        QMessageBox::about(this, QString::fromUtf8("提示"), QString::fromUtf8("请输入9位数字会议号"));
        return;
    }
    for (const QChar& ch : code) {
        if (!ch.isDigit()) {
            QMessageBox::about(this, QString::fromUtf8("提示"), QString::fromUtf8("会议号必须为9位数字"));
            return;
        }
    }
    Q_EMIT sig_meetingJoinHistory(code);
}

void FriendList::setUserInfo(QString name, int iconId, QString feeling)
{
    ui->lb_userName->setText(name);
    ui->lb_userFeeling->setText(feeling);
    m_iconId = iconId;
}

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

void FriendList::on_lw_friendList_customContextMenuRequested(const QPoint& pos)
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
    saveMeetingHistory();
    rebuildHistoryList();
}

void FriendList::updateMeetingStatus(QString meetingId, bool isActive)
{
    for (int i = 0; i < m_meetingHistory.size(); ++i) {
        if (m_meetingHistory[i].id == meetingId) {
            m_meetingHistory[i].isActive = isActive;
            saveMeetingHistory();
            break;
        }
    }
    rebuildHistoryList();
}

void FriendList::loadMeetingHistory()
{
    QString filePath = QCoreApplication::applicationDirPath() + "/records/meeting_history.json";
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;  // 首次启动无历史文件，正常情况
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isArray()) {
        return;
    }

    m_meetingHistory.clear();
    for (const QJsonValue& val : doc.array()) {
        QJsonObject obj = val.toObject();
        MeetingRecord rec;
        rec.id = obj["id"].toString();
        rec.timeStr = obj["timeStr"].toString();
        rec.isActive = obj["isActive"].toBool(false);  // 文件中的记录默认视为已结束
        m_meetingHistory.append(rec);
    }
    rebuildHistoryList();
}

void FriendList::saveMeetingHistory()
{
    QString dirPath = QCoreApplication::applicationDirPath() + "/records/";
    QDir().mkpath(dirPath);  // 确保目录存在

    QJsonArray arr;
    for (const auto& rec : m_meetingHistory) {
        QJsonObject obj;
        obj["id"] = rec.id;
        obj["timeStr"] = rec.timeStr;
        obj["isActive"] = rec.isActive;
        arr.append(obj);
    }

    QJsonDocument doc(arr);
    QString filePath = dirPath + "meeting_history.json";
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(doc.toJson());
        file.close();
    }
}

void FriendList::rebuildHistoryList()
{
    QListWidget* list = ui->lw_meetingHistory;
    QStackedWidget* stack = ui->stk_history;
    list->clear();
    if (m_meetingHistory.isEmpty()) {
        stack->setCurrentIndex(1);
        return;
    }
    stack->setCurrentIndex(0);
    const char* kColors[] = {"#1677FF","#52C41A","#722ED1","#FA8C16","#13C2C2","#EB2F96"};
    for (const auto& rec : m_meetingHistory) {
        int ci = rec.id.toInt() % 6;
        QListWidgetItem* item = new QListWidgetItem();
        item->setSizeHint(QSize(0, 62));
        item->setData(Qt::UserRole, rec.id);
        list->addItem(item);

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

        QLabel* tag;
        if (rec.isActive) {
            tag = new QLabel(QString::fromUtf8("加入"));
            tag->setStyleSheet(
                "background: #E6F7FF; color: #1677FF; border-radius: 8px; padding: 2px 10px;"
                "font-size: 11px; font-family: Microsoft YaHei;");
        } else {
            tag = new QLabel(QString::fromUtf8("已结束"));
            tag->setStyleSheet(
                "background: #F5F6FA; color: #999; border-radius: 8px; padding: 2px 10px;"
                "font-size: 11px; font-family: Microsoft YaHei;");
        }
        row->addWidget(tag);
        list->setItemWidget(item, w);
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
