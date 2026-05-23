#ifndef FRIENDREQUESTLISTDIALOG_H
#define FRIENDREQUESTLISTDIALOG_H

#include <QDialog>
#include <QList>
#include "./Net/def.h"

namespace Ui { class FriendRequestListDialog; }

class FriendRequestListDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FriendRequestListDialog(QWidget* parent = nullptr);
    ~FriendRequestListDialog();

    void setRequests(const QList<_STRU_FRIEND_REQUEST_ITEM>& requests);

signals:
    void sig_acceptRequest(int requestId, int fromId, const QString& fromName);
    void sig_refuseRequest(int requestId);

private:
    Ui::FriendRequestListDialog* ui;
    QList<_STRU_FRIEND_REQUEST_ITEM> m_requests;
};

#endif // FRIENDREQUESTLISTDIALOG_H
