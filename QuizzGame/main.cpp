#include "quizzgame.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QuizzGame w;
    w.show();

    return a.exec();
}
