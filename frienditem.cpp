#include "frienditem.h"
#include "ui_frienditem.h"
#include "./Net/def.h"

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

//设置好友信息
void FriendItem::setFriendInfo(int friendId, QString name, int iconId, QString feeling, int status)
{
    Q_UNUSED(iconId);
    //保存到成员变量
    m_friendId = friendId;
    m_name = name;
    m_feeling = feeling;
    m_status = status;

    //设置昵称和签名
    ui->lb_name->setText(name);
    ui->lb_feeling->setText(feeling);

    //设置圆形文字头像：取名字首字符
    QString firstChar = name.isEmpty() ? QString("?") : QString(name.at(0));
    ui->lb_avatar->setText(firstChar);

    //在线: 深蓝头像, 离线: 灰色头像
    if (_DEF_STATUS_ONLINE == status) {
        ui->lb_avatar->setStyleSheet(
            "background: #1677FF; color: white; border-radius: 22px;"
            "font-size: 18px; font-weight: bold; font-family: Microsoft YaHei;");
    } else {
        ui->lb_avatar->setStyleSheet(
            "background: #B0B0B0; color: white; border-radius: 22px;"
            "font-size: 18px; font-weight: bold; font-family: Microsoft YaHei;");
    }
}

//只更新在线状态 (不覆盖昵称和签名)
void FriendItem::setFriendStatus(int status)
{
    m_status = status;
    if (_DEF_STATUS_ONLINE == status) {
        ui->lb_avatar->setStyleSheet(
            "background: #1677FF; color: white; border-radius: 22px;"
            "font-size: 18px; font-weight: bold; font-family: Microsoft YaHei;");
    } else {
        ui->lb_avatar->setStyleSheet(
            "background: #B0B0B0; color: white; border-radius: 22px;"
            "font-size: 18px; font-weight: bold; font-family: Microsoft YaHei;");
    }
}
