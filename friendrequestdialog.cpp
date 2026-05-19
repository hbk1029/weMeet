#include "friendrequestdialog.h"
#include "ui_friendrequestdialog.h"

FriendRequestDialog::FriendRequestDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::FriendRequestDialog), m_accepted(false)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
}

FriendRequestDialog::~FriendRequestDialog()
{
    delete ui;
}

//设置请求信息
void FriendRequestDialog::setRequestInfo(const QString& fromName)
{
    ui->lb_msg->setText(QString::fromUtf8(
        "<b>%1</b> 请求添加你为好友").arg(fromName));
}

//同意
void FriendRequestDialog::on_pb_accept_clicked()
{
    m_accepted = true;
    accept();
}

//拒绝
void FriendRequestDialog::on_pb_refuse_clicked()
{
    m_accepted = false;
    reject();
}
