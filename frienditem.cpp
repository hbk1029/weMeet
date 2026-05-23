#include "frienditem.h"
#include "ui_frienditem.h"
#include "./Net/def.h"
#include <QStyle>

FriendItem::FriendItem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FriendItem),
    m_friendId(0),
    m_status(_DEF_STATUS_OFFLINE)
{
    ui->setupUi(this);
}

FriendItem::~FriendItem()
{
    delete ui;
}

void FriendItem::setFriendInfo(int friendId, QString name, int iconId, QString feeling, int status)
{
    Q_UNUSED(iconId);
    m_friendId = friendId;
    m_name = name;
    m_feeling = feeling;
    m_status = status;

    ui->lb_name->setText(name);
    ui->lb_feeling->setText(feeling);
    ui->lb_avatar->setText(name.isEmpty() ? QString("?") : QString(name.at(0)));

    setFriendStatus(status);
}

void FriendItem::setFriendStatus(int status)
{
    m_status = status;
    bool online = (_DEF_STATUS_ONLINE == status);
    ui->lb_avatar->setProperty("status", online ? "" : "offline");
    ui->lb_avatar->style()->unpolish(ui->lb_avatar);
    ui->lb_avatar->style()->polish(ui->lb_avatar);
}
