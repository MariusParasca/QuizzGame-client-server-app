#include "quizzgame.h"
#include "ui_quizzgame.h"
#include <QMessageBox>
#include <QCloseEvent>

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

void QuizzGame::SendUsernamePassword(QString qusername, QString qpassword, int errorPage,
                                     QString title, QString succesfullyText, QString errorText)
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
    }
    else
    {
        QMessageBox::warning(this, title, errorText);
        ui->stackedWidget->setCurrentIndex(errorPage);
    }
}

void QuizzGame::GetQuestion()
{
    int length;
    if (read (sd, &length, sizeof(int)) < 0)
    {
        perror ("[client]read() error from server.\n");
        exit(errno);
    }

    char *question = (char *) malloc(length);

    if (read (sd, question, length) < 0)
    {
        perror ("[client]read() error from server.\n");
        exit(errno);
    }

    question[length] ='\0';

    ui->label_question->setText(question);
}

void QuizzGame::GetAnswers()
{
    GetAnswer('a');
    GetAnswer('b');
    GetAnswer('c');
    GetAnswer('d');
}

void QuizzGame::GetAnswer(char subsection)
{
    int length;
    if (read (sd, &length, sizeof(int)) < 0)
    {
        perror ("[client]read() error from server.\n");
        exit(errno);
    }

    char *answer = (char *) malloc(length);

    if (read (sd, answer, length) < 0)
    {
        perror ("[client]read() error from server.\n");
        exit(errno);
    }

    answer[length] = '\0';

    switch (subsection) {
    case 'a':
        ui->radioButton_a->setText(answer);
        break;
    case 'b':
        ui->radioButton_b->setText(answer);
        break;
    case 'c':
        ui->radioButton_c->setText(answer);
        break;
    case 'd':
        ui->radioButton_d->setText(answer);
        break;
    default:
        ui->statusBar->showMessage("Error! Something went wrong on GetAnswer()!");
        break;
    }
}

void QuizzGame::on_pushButton_login_clicked()
{
    QString qusername = ui->lineEdit_username_login->text();
    QString qpassword = ui->lineEdit_password_login->text();
    SendUsernamePassword(qusername, qpassword, 1, "Log in", "Log in succesfully",
                         "Username or password incorrect. Try again!");
    PrepareGame();
}

void QuizzGame::on_pushButton_register_clicked()
{
    QString qusername = ui->lineEdit_username_register->text();
    QString qpassword = ui->lineEdit_password_register->text();
    SendUsernamePassword(qusername, qpassword, 2, "Register", "Register succesfully",
                         "Username already taken. Try again!");
    PrepareGame();
}

void QuizzGame::on_pushButton_check_clicked()
{
    PrepareGame();
}

void QuizzGame::PrepareGame()
{
    GetQuestion();
    GetAnswers();
    ui->stackedWidget->setCurrentIndex(3);
}

// modifica
void QuizzGame::closeEvent (QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "Quit",
                                                                tr("Are you sure?\n"),
                                                                QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        event->accept();
    }
}

