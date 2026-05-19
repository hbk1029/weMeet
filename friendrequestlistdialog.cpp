#include "friendrequestlistdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

FriendRequestListDialog::FriendRequestListDialog(QWidget* parent)
    : QDialog(parent), m_pListWidget(nullptr)
{
    setWindowTitle(QString::fromUtf8("好友请求"));
    setFixedSize(420, 360);
    setStyleSheet(
        "QDialog { background: #F5F6FA; font-family: Microsoft YaHei; }");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    QLabel* titleLabel = new QLabel(QString::fromUtf8("待处理的好友请求"), this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #333;");
    mainLayout->addWidget(titleLabel);

    m_pListWidget = new QListWidget(this);
    m_pListWidget->setStyleSheet(
        "QListWidget { background: transparent; border: none; }"
        "QListWidget::item { background: white; border-radius: 10px; margin: 0 0 8px 0; padding: 0; }");
    mainLayout->addWidget(m_pListWidget);

    QPushButton* closeBtn = new QPushButton(QString::fromUtf8("关闭"), this);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedHeight(36);
    closeBtn->setStyleSheet(
        "QPushButton { background: #E5E6EB; color: #333; border-radius: 8px; font-size: 14px; border: none; }"
        "QPushButton:hover { background: #D3D4D9; }");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(closeBtn);
}

FriendRequestListDialog::~FriendRequestListDialog()
{
}

void FriendRequestListDialog::setRequests(const QList<_STRU_FRIEND_REQUEST_ITEM>& requests)
{
    m_requests = requests;
    m_pListWidget->clear();

    for (const auto& req : m_requests) {
        QListWidgetItem* item = new QListWidgetItem();
        item->setSizeHint(QSize(0, 64));
        m_pListWidget->addItem(item);

        QWidget* w = new QWidget();
        QHBoxLayout* row = new QHBoxLayout(w);
        row->setContentsMargins(16, 10, 16, 10);
        row->setSpacing(12);

        // 头像占位
        QLabel* avatar = new QLabel(QString(req.fromName).left(1));
        avatar->setFixedSize(40, 40);
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setStyleSheet(
            "background: #1677FF; color: white; border-radius: 20px;"
            "font-size: 16px; font-weight: bold;");

        QLabel* nameLabel = new QLabel(QString::fromUtf8("%1").arg(req.fromName));
        nameLabel->setStyleSheet("font-size: 14px; color: #333;");
        QLabel* hintLabel = new QLabel(QString::fromUtf8("请求添加你为好友"));
        hintLabel->setStyleSheet("font-size: 12px; color: #999;");
        QVBoxLayout* textCol = new QVBoxLayout();
        textCol->setSpacing(2);
        textCol->addWidget(nameLabel);
        textCol->addWidget(hintLabel);

        QPushButton* acceptBtn = new QPushButton(QString::fromUtf8("同意"));
        acceptBtn->setCursor(Qt::PointingHandCursor);
        acceptBtn->setFixedSize(60, 32);
        acceptBtn->setStyleSheet(
            "QPushButton { background: #1677FF; color: white; border-radius: 6px; font-size: 13px; border: none; }"
            "QPushButton:hover { background: #4096FF; }");

        QPushButton* refuseBtn = new QPushButton(QString::fromUtf8("拒绝"));
        refuseBtn->setCursor(Qt::PointingHandCursor);
        refuseBtn->setFixedSize(60, 32);
        refuseBtn->setStyleSheet(
            "QPushButton { background: #F5F6FA; color: #666; border: 1px solid #E5E6EB; border-radius: 6px; font-size: 13px; }"
            "QPushButton:hover { background: #FFE7E7; color: #FF4D4F; border-color: #FF4D4F; }");

        row->addWidget(avatar);
        row->addLayout(textCol);
        row->addStretch();
        row->addWidget(acceptBtn);
        row->addWidget(refuseBtn);

        m_pListWidget->setItemWidget(item, w);

        int requestId = req.requestId;
        int fromId = req.fromId;
        QString fromName = QString::fromUtf8(req.fromName);

        connect(acceptBtn, &QPushButton::clicked, this, [this, requestId, fromId, fromName]() {
            Q_EMIT sig_acceptRequest(requestId, fromId, fromName);
            // 从列表中移除该条目
            for (int i = 0; i < m_pListWidget->count(); ++i) {
                QListWidgetItem* it = m_pListWidget->item(i);
                QWidget* wdg = m_pListWidget->itemWidget(it);
                if (wdg) {
                    QList<QPushButton*> btns = wdg->findChildren<QPushButton*>();
                    for (auto* btn : btns) btn->setEnabled(false);
                }
                it->setBackground(QColor("#F0FFF0"));
            }
        });

        connect(refuseBtn, &QPushButton::clicked, this, [this, requestId]() {
            Q_EMIT sig_refuseRequest(requestId);
            for (int i = 0; i < m_pListWidget->count(); ++i) {
                QListWidgetItem* it = m_pListWidget->item(i);
                QWidget* wdg = m_pListWidget->itemWidget(it);
                if (wdg) {
                    QList<QPushButton*> btns = wdg->findChildren<QPushButton*>();
                    for (auto* btn : btns) btn->setEnabled(false);
                }
                it->setBackground(QColor("#FFF0F0"));
            }
        });
    }
}
