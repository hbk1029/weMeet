#ifndef FRIENDREQUESTDIALOG_H
#define FRIENDREQUESTDIALOG_H

#include <QDialog>

namespace Ui {
class FriendRequestDialog;
}

class FriendRequestDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FriendRequestDialog(QWidget* parent = nullptr);
    ~FriendRequestDialog();

    //设置请求信息
    void setRequestInfo(const QString& fromName);
    //是否同意
    bool isAccepted() const { return m_accepted; }

private slots:
    void on_pb_accept_clicked();
    void on_pb_refuse_clicked();

private:
    Ui::FriendRequestDialog* ui;
    bool m_accepted;
};

#endif // FRIENDREQUESTDIALOG_H
