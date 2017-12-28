#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <strings.h>
#include <sqlite3.h> 

/* portul folosit */
#define PORT 2908

/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData{
  int idThread; //id-ul thread-ului tinut in evidenta de acest program contorizeaza
  int cl; //descriptorul intors de accept
}thData;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *);
void login(void * arg);
void userRegister(void *arg);
sqlite3* openDatabase();
void closeDatabase(sqlite3* db) ;
int checkUsernamePassword(char *username, char *password);
int checkForValidUsername(char* username, char*password);
void addQuestion();
void addingQuestion();

int main()
{
  struct sockaddr_in server;	// structura folosita de server
  struct sockaddr_in from;	
  int nr;		//mesajul primit de trimis la client 
  int sd;		//descriptorul de socket 
  int pid;
  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
  int i=0;

  //addingQuestion();

  /* crearea unui socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror ("[server]Eroare la socket().\n");
    return errno;
  }
  /* utilizarea optiunii SO_REUSEADDR */
  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

  /* pregatirea structurilor de date */
  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));

  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
  server.sin_family = AF_INET;	
  /* acceptam orice adresa */
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  /* utilizam un port utilizator */
  server.sin_port = htons (PORT);

  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
  {
    perror ("[server]Eroare la bind().\n");
    return errno;
  }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, 2) == -1)
  {
    perror ("[server]Eroare la listen().\n");
    return errno;
  }
  /* servim in mod concurent clientii...folosind thread-uri */
  while (1)
  {
    int client;
    pthread_t thread;
    thData * td; //parametru functia executata de thread     
    int length = sizeof (from);

    printf ("[server]Asteptam la portul %d...\n",PORT);
    fflush (stdout);

    //client= malloc(sizeof(int));
    /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
    if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
    {
      perror ("[server]Eroare la accept().\n");
      continue;
    }

    /* s-a realizat conexiunea, se astepta mesajul */

    int idThread; //id-ul threadului
    int cl; //descriptorul intors de accept

    td=(struct thData*)malloc(sizeof(struct thData));	
    td->idThread=i++;
    td->cl=client;

    pthread_create(&thread, NULL, &treat, td);	      

  }//while    
};

static void *treat(void * arg)
{		
  thData tdL; 
  tdL= *((struct thData*)arg);	
  printf ("[thread]- %d - Asteptam mesajul...\n", tdL.idThread); ///////////
  fflush (stdout);		 
  pthread_detach(pthread_self());		
  raspunde((struct thData*)arg);
  /* am terminat cu acest client, inchidem conexiunea */
  close ((intptr_t)arg);
  return(NULL);	
};


void raspunde(void *arg)
{
  int ok, i=0;
  struct thData tdL; 
  tdL= *((struct thData*)arg);

  //finished: login(&tdL);
  userRegister(&tdL);
  
}

void closeDatabase(sqlite3* db) { sqlite3_close(db); }

sqlite3* openDatabase()
{
  sqlite3 *db;
  if( (sqlite3_open("quizzGameDataBase.db", &db)) ) 
  {
    fprintf(stderr, "[Database]Can't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
  } 
  else 
    fprintf(stderr, "[Database]Opened database successfully\n");
  return db;
}

int checkUsernamePassword(char *username, char *password)
{
  sqlite3 *db = openDatabase();
  sqlite3_stmt *res;
  char sql[1024];
  sprintf(sql, "SELECT username, password FROM users WHERE username = '%s' AND password = '%s';", username, password);

  if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK)
  {
    
    fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return -1;
  }
  closeDatabase(db);
  if (sqlite3_step(res) == SQLITE_ROW) 
  {
    return 1;
  }
  else
    return 0;

  sqlite3_finalize(res);
  return 0;
}

void login(void * arg)
{
  thData tdL; 
  int ok;
  char username[64], password[64];
  tdL= *((struct thData*)arg);

  if (read (tdL.cl, username,sizeof(username)) <= 0)
  {
    printf("[Thread %d]\n",tdL.idThread);
    perror ("read() error from client.\n");
  }
  if (read (tdL.cl, password,sizeof(password)) <= 0)
  {
    printf("[Thread %d]\n",tdL.idThread);
    perror ("read() error from client.\n");
  }

  printf ("[Thread %d]Mesajul a fost receptionat...%s %s\n",tdL.idThread, username, password); ///////////

   ok = checkUsernamePassword(username, password);

  if (write (tdL.cl, &ok, sizeof(ok)) <= 0)
  {
    printf("[Thread %d] ",tdL.idThread);
    perror ("[Thread]write() error sending to client.\n");
  }
  else
    printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",tdL.idThread); ////////
}

void userRegister(void *arg)
{
  thData tdL; 
  int ok;
  char *string;
  char username[64], password[64];
  tdL= *((struct thData*)arg);

  if (read (tdL.cl, username,sizeof(username)) <= 0)
  {
    printf("[Thread %d]\n",tdL.idThread);
    perror ("read() error from client.\n");
  }
  if (read (tdL.cl, password,sizeof(password)) <= 0)
  {
    printf("[Thread %d]\n",tdL.idThread);
    perror ("read() error from client.\n");
  }

  ok = checkForValidUsername(username, password);

  if (write (tdL.cl, &ok, sizeof(ok)) <= 0)
  {
    printf("[Thread %d] ",tdL.idThread);
    perror ("[Thread]write() error sending to client.\n");
  }
  else
    printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",tdL.idThread);
}

int checkForValidUsername(char* username, char*password)
{
  sqlite3 *db = openDatabase();
  char *errMsg = 0;

  char sql[1024];
  sprintf(sql, "INSERT INTO users VALUES(NULL, '%s', '%s');", username, password);
  
  if( sqlite3_exec(db, sql, NULL, 0, &errMsg) != SQLITE_OK )
  {
    fprintf(stderr, "SQL error: %s\n", errMsg);
    sqlite3_free(errMsg);
    closeDatabase(db);
    return 0;
  }
  else 
  {
    closeDatabase(db);
    return 1;
  }

  return 0;
}

void addQuestion()
{
  char question[1024];
  char a[128], b[128], c[128], d[128], correctAnswer[128];
  char *errMsg = 0;
  int points;
  sqlite3 *db = openDatabase();

  printf("Enter question: \n");
  scanf("%[^\n]%*c", question);
  printf("Enter points: \n");
  scanf("%d", &points); getchar();
  printf("Enter answer a): \n");
  scanf("%[^\n]%*c", a);
  printf("Enter answer b): \n");
  scanf("%[^\n]%*c", b);
  printf("Enter answer c): \n");
  scanf("%[^\n]%*c", c);
  printf("Enter answer d): \n");
  scanf("%[^\n]%*c", d);
  printf("Enter correct answer: \n");
  scanf("%[^\n]%*c", correctAnswer);

  //printf("%s cu punctele: %d\n Raspunsurile: a)%s\n b)%s\n c)%s\n d)%s\n corect: %s\n", question, points, a, b, c, d, correctAnswer);

  char sql[1024];
  sprintf(sql, "INSERT INTO questions VALUES(NULL, '%s', %d, '%s', '%s','%s','%s','%s');", question, points, a, b, c, d, correctAnswer);
  
  if( sqlite3_exec(db, sql, NULL, 0, &errMsg) != SQLITE_OK )
  {
    fprintf(stderr, "SQL error: %s\n", errMsg);
    sqlite3_free(errMsg);
  }
  else 
  {
    printf("Question added successfully.");
  }
  closeDatabase(db);
}

void addingQuestion()
{
  while(1)
  {
    char answer;
    printf("Do you want to add a question? y/n\n"); 
    scanf("%c", &answer); getchar();
    if(answer == 'y')
    {
      addQuestion();
      break;
    }
    else
      break;
  }
}