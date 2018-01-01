#include "quizzgame.h"
#include <QApplication>

class ServerComunicator
{
public:
    int GetSocketDescriptor();
    int ConnectToServer(const char* ipAdress, int port);
private:
    int sd;
};

int ServerComunicator::ConnectToServer(const char* ipAdress, int port)
{
    struct sockaddr_in server;

    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror ("[client]socket() error.\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ipAdress);
    server.sin_port = htons(port);

    if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
        perror ("[client]connect() error.\n");
        return 0;
    }

    return 1;
}

int ServerComunicator::GetSocketDescriptor() { return sd; }

int main(int argc, char *argv[])
{
    ServerComunicator sc;
    QApplication a(argc, argv);
    QuizzGame w(0, sc.ConnectToServer("127.0.0.1", 2018));
    w.SetSocketDescriptor(sc.GetSocketDescriptor());
    w.show();
    a.exec();
}
