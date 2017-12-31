#ifndef QUIZZGAME_H
#define QUIZZGAME_H

#include <QMainWindow>

namespace Ui {
class QuizzGame;
}

class QuizzGame : public QMainWindow
{
    Q_OBJECT

public:
    explicit QuizzGame(QWidget *parent = 0);
    ~QuizzGame();

private:
    Ui::QuizzGame *ui;
};

#endif // QUIZZGAME_H
