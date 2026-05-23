#include "friendrequestlistdialog.h"
#include "ui_friendrequestlistdialog.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

FriendRequestListDialog::FriendRequestListDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::FriendRequestListDialog)
{
    ui->setupUi(this);
    setFixedSize(420, 360);
}

FriendRequestListDialog::~FriendRequestListDialog()
{
    delete ui;
}

void FriendRequestListDialog::setRequests(const QList<_STRU_FRIEND_REQUEST_ITEM>& requests)
{
    m_requests = requests;
    ui->lw_requests->clear();

    for (const auto& req : m_requests) {
        QListWidgetItem* item = new QListWidgetItem();
        item->setSizeHint(QSize(0, 64));
        ui->lw_requests->addItem(item);

        QWidget* w = new QWidget();
        QHBoxLayout* row = new QHBoxLayout(w);
        row->setContentsMargins(16, 10, 16, 10);
        row->setSpacing(12);

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

        ui->lw_requests->setItemWidget(item, w);

        int requestId = req.requestId;
        int fromId = req.fromId;
        QString fromName = QString::fromUtf8(req.fromName);

        connect(acceptBtn, &QPushButton::clicked, this, [this, requestId, fromId, fromName]() {
            Q_EMIT sig_acceptRequest(requestId, fromId, fromName);
            for (int i = 0; i < ui->lw_requests->count(); ++i) {
                QListWidgetItem* it = ui->lw_requests->item(i);
                QWidget* wdg = ui->lw_requests->itemWidget(it);
                if (wdg) {
                    QList<QPushButton*> btns = wdg->findChildren<QPushButton*>();
                    for (auto* btn : btns) btn->setEnabled(false);
                }
                it->setBackground(QColor("#F0FFF0"));
            }
        });

        connect(refuseBtn, &QPushButton::clicked, this, [this, requestId]() {
            Q_EMIT sig_refuseRequest(requestId);
            for (int i = 0; i < ui->lw_requests->count(); ++i) {
                QListWidgetItem* it = ui->lw_requests->item(i);
                QWidget* wdg = ui->lw_requests->itemWidget(it);
                if (wdg) {
                    QList<QPushButton*> btns = wdg->findChildren<QPushButton*>();
                    for (auto* btn : btns) btn->setEnabled(false);
                }
                it->setBackground(QColor("#FFF0F0"));
            }
        });
    }
}
