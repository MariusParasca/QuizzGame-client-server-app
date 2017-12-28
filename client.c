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

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

int main (int argc, char *argv[])
{
	int sd;			// descriptorul de socket
	struct sockaddr_in server;	// structura folosita pentru conectare 
		// mesajul trimis
	int nr=0;
	char username[50], password[50];
	char buf[10];

	/* exista toate argumentele in linia de comanda? */
	if (argc != 3)
	{
		printf ("Syntax: %s <server_adress> <port>\n", argv[0]);
		return -1;
	}

	/* stabilim portul */
	port = atoi (argv[2]);

	/* cream socketul */
	if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror ("[client]socket() error.\n");
		return errno;
	}

	/* umplem structura folosita pentru realizarea conexiunii cu serverul */
	/* familia socket-ului */
	server.sin_family = AF_INET;
	/* adresa IP a serverului */
	server.sin_addr.s_addr = inet_addr(argv[1]);
	/* portul de conectare */
	server.sin_port = htons (port);

	/* ne conectam la server */
	if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
	{
		perror ("[client]connect() error.\n");
		return errno;
	}

	/* citirea mesajului */
	printf ("[client]Enter the username: ");
	scanf("%s", username);
	printf ("[client]Enter the password: ");
	scanf("%s", password);

	/* trimiterea mesajului la server */
	if (write (sd,username,sizeof(username)) <= 0)
	{
		perror ("[client]write() to server error.\n");
		return errno;
	}

	if (write (sd,password,sizeof(password)) <= 0)
	{
		perror ("[client]write() to server error.\n");
		return errno;
	}

	/* citirea raspunsului dat de server 
	(apel blocant pina cind serverul raspunde) */
	char *string;
	int ok;
	if (read (sd, &ok, sizeof(int)) < 0)
	{
		perror ("[client]read() error from server.\n");
		return errno;
	}

	if(ok)
	{
		printf("Register succesfully");
	}
	else
	{
		printf("Register again");
	}

	/* inchidem conexiunea, am terminat */
	close (sd);
}
