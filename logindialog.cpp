#include "logindialog.h"
#include "ui_logindialog.h"
#include <QMessageBox>
#include <QDebug>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::LoginDialog), m_isLoginMode(true)
{
    ui->setupUi(this);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

//设置标签按钮样式
void LoginDialog::setTabButtonStyle(QPushButton* button, bool active)
{
    if (active) {
        button->setStyleSheet(
            "QPushButton { background: #1677FF; color: white; border-radius: 8px;"
            "  font-size: 14px; font-weight: bold; font-family: Microsoft YaHei; border: none; }");
    } else {
        button->setStyleSheet(
            "QPushButton { background: transparent; color: #666; border-radius: 8px;"
            "  font-size: 14px; font-family: Microsoft YaHei; border: 1px solid #E5E6EB; }"
            "QPushButton:hover { color: #1677FF; border-color: #1677FF; }");
    }
}

//切换到登录
void LoginDialog::on_pb_login_clicked()
{
    m_isLoginMode = true;
    ui->le_name->setVisible(false);
    ui->le_passwordAgain->setVisible(false);
    ui->le_password->setGeometry(60, 299, 300, 40);
    ui->pb_submit->setGeometry(60, 370, 300, 44);
    ui->pb_submit->setText("登录");
    setTabButtonStyle(ui->pb_login, true);
    setTabButtonStyle(ui->pb_register, false);
}

//切换到注册
void LoginDialog::on_pb_register_clicked()
{
    m_isLoginMode = false;
    ui->le_name->setVisible(true);
    ui->le_passwordAgain->setVisible(true);
    ui->le_password->setGeometry(60, 299, 300, 40);
    ui->pb_submit->setGeometry(60, 410, 300, 44);
    ui->pb_submit->setText("注册");
    setTabButtonStyle(ui->pb_login, false);
    setTabButtonStyle(ui->pb_register, true);
}

//提交
void LoginDialog::on_pb_submit_clicked()
{
    qDebug() << "on_pb_submit_clicked called, loginMode:" << m_isLoginMode;
    QString tel = ui->le_tel->text().trimmed();
    QString pass = ui->le_password->text().trimmed();
    QString telTmp = tel;
    QString passTmp = pass;

    qDebug() << "tel:" << tel << "pass len:" << pass.length();
    if (tel.isEmpty() || telTmp.remove(" ").isEmpty()) {
        qDebug() << "validation failed: empty tel";
        QMessageBox::about(this, "提示", "输入电话号为空或全空格");
        return;
    }
    if (pass.isEmpty() || passTmp.remove(" ").isEmpty()) {
        qDebug() << "validation failed: empty pass";
        QMessageBox::about(this, "提示", "输入密码为空或全空格");
        return;
    }
    if (tel.length() != 11) {
        qDebug() << "validation failed: tel length" << tel.length();
        QMessageBox::about(this, "提示", "电话号必须为11位");
        return;
    }
    if (pass.length() < 8 || pass.length() > 16) {
        qDebug() << "validation failed: pass length" << pass.length();
        QMessageBox::about(this, "提示", "密码必须为8-16位");
        return;
    }

    if (m_isLoginMode) {
        qDebug() << "emitting sig_loginData";
        Q_EMIT sig_loginData(tel, pass);
    } else {
        QString name = ui->le_name->text().trimmed();
        QString nameTmp = name;
        if (name.isEmpty() || nameTmp.remove(" ").isEmpty()) {
            QMessageBox::about(this, "提示", "输入昵称为空或全空格");
            return;
        }
        QString passAgain = ui->le_passwordAgain->text();
        if (pass != passAgain) {
            QMessageBox::about(this, "提示", "两次输入密码必须一致");
            return;
        }
        Q_EMIT sig_registerData(name, tel, pass);
    }
}

void LoginDialog::closeEvent(QCloseEvent *event)
{
    qDebug()<<__func__;
    event->ignore();
    if (QMessageBox::question(this, "提示", "是否退出?") == QMessageBox::Yes) {
         Q_EMIT sig_closeProcess();
    }
}
