#ifndef ADDFRIENDDIALOG_H
#define ADDFRIENDDIALOG_H

#include <QDialog>

namespace Ui {
class AddFriendDialog;
}

class AddFriendDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddFriendDialog(QWidget *parent = nullptr);
    ~AddFriendDialog();

    QString getInputText() const;

private slots:
    void on_pb_send_clicked();
    void on_pb_cancel_clicked();
    void on_pb_close_clicked();

private:
    Ui::AddFriendDialog *ui;
};

#endif // ADDFRIENDDIALOG_H
