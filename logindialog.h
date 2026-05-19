#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QCloseEvent>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();
    void closeEvent(QCloseEvent* event);

signals:
    void sig_closeProcess();
    void sig_registerData(QString name, QString tel, QString pass);
    void sig_loginData(QString tel, QString pass);

private slots:
    void on_pb_login_clicked();
    void on_pb_register_clicked();
    void on_pb_submit_clicked();

private:
    void setTabButtonStyle(QPushButton* button, bool active);

    Ui::LoginDialog *ui;
    bool m_isLoginMode;
};

#endif // LOGINDIALOG_H
