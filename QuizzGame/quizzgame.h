#ifndef QUIZZGAME_H
#define QUIZZGAME_H

#include <QMainWindow>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <QTimer>
#include <QTime>

#define NUMBER_OF_CLIENTS 2

namespace Ui {
class QuizzGame;
}

class QuizzGame : public QMainWindow
{
    Q_OBJECT

public:
    explicit QuizzGame(QWidget *parent = 0, int status = 0);
    void SetSocketDescriptor(int sd);
    void SendCode(int code);
    int ReadInt();
    void SendUsernamePassword(QString qusername, QString qpassword, QString succesfullyText, QString errorText);
    void GetQuestion();
    void GetAnswers();
    void GetAnswer(char subsection);
    void GetNextQuestion();
    void SendAnswer(char* answer);
    void PrepareGame();
    void SetRaddioButtonsToFalse();
    void QuestionTimer();
    void NextQuestionTimer();
    void PrintWinner();
    void closeEvent (QCloseEvent *event);
    ~QuizzGame();

private slots:
    void on_pushButton_singIn_clicked();

    void on_pushButton_login_clicked();

    void on_pushButton_register_clicked();

    void on_pushButton_registerWellcome_clicked();

    void on_pushButton_check_clicked();

    void on_pushButton_nextQuestion_clicked();

    void questionTimerSlot();

    void nextQuestionTimerSlot();

private:
    Ui::QuizzGame *ui;
    int sd;
    char *answerA, *answerB, *answerC, *answerD;
    QTimer *questTimer, *nextQuestTimer;
    QTime *questTime, *nextQuestTime;
};



#endif // QUIZZGAME_H
