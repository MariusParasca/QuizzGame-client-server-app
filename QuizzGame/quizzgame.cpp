#include "quizzgame.h"
#include "ui_quizzgame.h"

QuizzGame::QuizzGame(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::QuizzGame)
{
    ui->setupUi(this);
}

QuizzGame::~QuizzGame()
{
    delete ui;
}
