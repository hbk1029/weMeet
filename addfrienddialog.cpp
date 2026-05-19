#include "addfrienddialog.h"
#include "ui_addfrienddialog.h"

AddFriendDialog::AddFriendDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::AddFriendDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    ui->le_input->setFocus();
}

AddFriendDialog::~AddFriendDialog()
{
    delete ui;
}

QString AddFriendDialog::getInputText() const
{
    return ui->le_input->text().trimmed();
}

void AddFriendDialog::on_pb_send_clicked()
{
    if (ui->le_input->text().trimmed().isEmpty()) {
        return;
    }
    accept();
}

void AddFriendDialog::on_pb_cancel_clicked()
{
    reject();
}

void AddFriendDialog::on_pb_close_clicked()
{
    reject();
}
