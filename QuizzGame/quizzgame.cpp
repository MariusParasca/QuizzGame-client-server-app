#include "quizzgame.h"
#include "ui_quizzgame.h"
#include <QMessageBox>

QuizzGame::QuizzGame(QWidget *parent, int status) : QMainWindow(parent), ui(new Ui::QuizzGame)
{
    ui->setupUi(this);
    if(status)
        ui->label_status->setText("Server online");
    else
        ui->label_status->setText("Server offline");
    ui->stackedWidget->setCurrentIndex(0);
}

QuizzGame::~QuizzGame() { delete ui; }

void QuizzGame::SetSocketDescriptor(int sd) { this->sd = sd; }

void QuizzGame::SendCode(int code)
{
    if (write (sd, &code, sizeof(code)) <= 0)
    {
        perror ("[client]write() to server error.\n");
        exit(errno);
    }
}

void QuizzGame::on_pushButton_singIn_clicked()
{
    SendCode(1);
    ui->stackedWidget->setCurrentIndex(1);
}

void QuizzGame::on_pushButton_registerWellcome_clicked()
{
    SendCode(2);
    ui->stackedWidget->setCurrentIndex(2);
}

void QuizzGame::SendUsernamePassword(QString qusername, QString qpassword, int succesfullyPage,
                                     int errorPage, QString title, QString succesfullyText, QString errorText)
{
    QByteArray toChar = qusername.toLatin1();
    char username[64];
    strcpy(username, toChar.data());
    QByteArray toChar2 = qpassword.toLatin1();
    char password[64];
    strcpy(password, toChar2.data());

    if (write (sd, username, sizeof(username)) <= 0)
    {
        perror ("[client]write() to server error.\n");
        exit(errno);
    }

    if (write (sd, password, sizeof(password)) <= 0)
    {
        perror ("[client]write() to server error.\n");
        exit(errno);
    }

    int ok;

    if (read (sd, &ok, sizeof(int)) < 0)
    {
        perror ("[client]read() error from server.\n");
        exit(errno);
    }

    if(ok)
    {
        QMessageBox::information(this, title , succesfullyText);
        ui->stackedWidget->setCurrentIndex(succesfullyPage);
    }
    else
    {
        QMessageBox::warning(this, title, errorText);
        ui->stackedWidget->setCurrentIndex(errorPage);
    }
}

void QuizzGame::on_pushButton_login_clicked()
{
    QString qusername = ui->lineEdit_username_login->text();
    QString qpassword = ui->lineEdit_password_login->text();
    SendUsernamePassword(qusername, qpassword, 3, 1, "Log in", "Log in succesfully",
                         "Username or password incorrect. Try again!");
}

void QuizzGame::on_pushButton_register_clicked()
{
    QString qusername = ui->lineEdit_username_register->text();
    QString qpassword = ui->lineEdit_password_register->text();
    SendUsernamePassword(qusername, qpassword, 3, 2, "Register", "Register succesfully",
                         "Username already taken. Try again!");
}
