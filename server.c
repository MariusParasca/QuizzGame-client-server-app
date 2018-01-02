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
#define PORT 2018

/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData{
  pthread_t idThread; //id-ul thread-ului tinut in evidenta de acest program contorizeaza
  int cl; //descriptorul intors de accept
}thData;

struct questionAndSize
{
  const unsigned char* question;
  int size;
};

struct answersAndSizes 
{
  const unsigned char* answerA;
  const unsigned char* answerB;
  const unsigned char* answerC;
  const unsigned char* answerD;
  int sizeA;
  int sizeB;
  int sizeC;
  int sizeD;
};

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void respond(void *);
void login(void * arg);
void userRegister(void *arg);
sqlite3* openDatabase();
void closeDatabase(sqlite3* db);
int checkUsernamePassword(char *username, char *password);
int checkForValidUsername(char *username, char *password);
void addQuestion();
void addingQuestion();
struct questionAndSize getQuestion(int questionID);
struct answersAndSizes getAnswers(int questionID);
void sendQuestion(void *arg, int questionID);
void sendAnswers(void *arg, int questionID);
void sendAnswer(void *arg, int size, const unsigned char* answer);

int main()
{
  struct sockaddr_in server;	// structura folosita de server
  struct sockaddr_in from;	
  int nr;		//mesajul primit de trimis la client 
  int sd;		//descriptorul de socket 
  int pid;
  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
  int i=0;

  addingQuestion();

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
    unsigned int length = sizeof (from);

    printf ("[server]Asteptam la portul %d...\n",PORT);
    fflush (stdout);

    //client= malloc(sizeof(int));
    /* acceptam un client (stare blocanta pina la realizarea conexiunii)  */
    if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
    {
      perror ("[server]Eroare la accept().\n");
      continue;
    }

    /// s-a realizat conexiunea, se astepta mesajul 

    int idThread; //id-ul threadului
    int cl; //descriptorul intors de accept

    td=(struct thData*)malloc(sizeof(struct thData));	
    td->cl=client;

    pthread_create(&td->idThread, NULL, &treat, td);	     

  }//while    
};

static void *treat(void * arg)
{		
  thData tdL; 
  tdL= *((struct thData*)arg);	
  printf ("[thread]- %d - Asteptam mesajul...\n", (int) tdL.idThread); ///////////
  fflush (stdout);		 
  pthread_detach(pthread_self());		
  respond((struct thData*)arg);
  /* am terminat cu acest client, inchidem conexiunea */
  close ((intptr_t)arg);
  return(NULL);	
};


void respond(void *arg)
{
  thData tdL; 
  tdL= *((struct thData*)arg);
  int code;

  if (read (tdL.cl, &code, sizeof(code)) <= 0)
  {
    printf("[Thread %d]\n", (int) tdL.idThread);
    perror ("read() error from client.\n");
  }

  if(code == 1)
  {
    login((struct thData*)arg);
    //sendQuestion((struct thData*)arg);
    //sendAnswers((struct thData*)arg);
  }
  else if(code == 2)
  {
    userRegister((struct thData*)arg);
  }

  //sendQuestion((struct thData*) &tdL);
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
    sqlite3_finalize(res);
    return 1;
  }
  else
  {
    sqlite3_finalize(res);
    return 0;
  }

  return 0;
}

void login(void * arg)
{
  thData tdL; 
  int ok = 0;
  char username[64], password[64];
  tdL= *((struct thData*)arg);

  while(!ok)
  {
    if (read (tdL.cl, username, sizeof(username)) <= 0)
    {
      printf("[Thread %d]\n", (int) tdL.idThread);
      perror ("read() error from client.\n");
    }
    if (read (tdL.cl, password, sizeof(password)) <= 0)
    {
      printf("[Thread %d]\n", (int) tdL.idThread);
      perror ("read() error from client.\n");
    }

    ok = checkUsernamePassword(username, password);

    if (write (tdL.cl, &ok, sizeof(ok)) <= 0)
    {
      printf("[Thread %d] ", (int) tdL.idThread);
      perror ("[Thread]write() error sending to client.\n");
    }
  }

  printf ("[Thread %d]Log in successfully.\n", (int) tdL.idThread);
}

void userRegister(void *arg)
{
  thData tdL; 
  int ok = 0;
  char username[64], password[64];
  tdL= *((struct thData*)arg);

  while(!ok)
  {
    if (read (tdL.cl, username, sizeof(username)) <= 0)
    {
      printf("[Thread %d]\n",(int) tdL.idThread);
      perror ("read() error from client.\n");
    }
    if (read (tdL.cl, password, sizeof(password)) <= 0)
    {
      printf("[Thread %d]\n", (int) tdL.idThread);
      perror ("read() error from client.\n");
    }

    ok = checkForValidUsername(username, password);

    if (write (tdL.cl, &ok, sizeof(ok)) <= 0)
    {
      printf("[Thread %d] ", (int) tdL.idThread);
      perror ("[Thread]write() error sending to client.\n");
    }
  }

  printf ("[Thread %d]Register successfully.\n", (int) tdL.idThread);
}

int checkForValidUsername(char *username, char *password)
{
  sqlite3 *db = openDatabase();
  char *errMsg = 0;

  char sql[1024];
  sprintf(sql, "INSERT INTO users VALUES(NULL, '%s', '%s');", username, password);
  
  if(( sqlite3_exec(db, sql, NULL, 0, &errMsg) ) != SQLITE_OK)
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
      addingQuestion();  
      break;
    }
    else
      break;
  }
}

struct questionAndSize getQuestion(int questionID)
{
  sqlite3 *db = openDatabase();
  sqlite3_stmt *res;
  struct questionAndSize temp;
  char sql[1024];
  sprintf(sql, "SELECT question FROM questions WHERE id_question = %d;", questionID);

  if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK)
  {
    
    fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(-1);
  }
  closeDatabase(db);
  
  if (sqlite3_step(res) == SQLITE_ROW) 
  {
    temp.question = sqlite3_column_text(res, 0);
    temp.size = sqlite3_column_bytes(res, 0);
    sqlite3_finalize(res);
    return temp;
  }
  else
  {
    temp.size = 0;
    sqlite3_finalize(res);
    return temp;
  }
}

void sendQuestion(void *arg, int questionID)
{
  thData tdL; 
  tdL= *((struct thData*)arg);
  struct questionAndSize temp = getQuestion(1);

  if(temp.size == 0)
    exit(0);

  if (write (tdL.cl, &temp.size, sizeof(temp.size)) <= 0)
  {
    printf("[Thread %d] ", (int) tdL.idThread);
    perror ("[Thread]write() error sending to client.\n");
  }

  if (write (tdL.cl, temp.question, temp.size) <= 0)
  {
    printf("[Thread %d] ", (int) tdL.idThread);
    perror ("[Thread]write() error sending to client.\n");
  }
}

struct answersAndSizes getAnswers(int questionID)
{
  sqlite3 *db = openDatabase();
  sqlite3_stmt *res;
  struct answersAndSizes temp;
  char sql[1024];
  sprintf(sql, "SELECT answer_a, answer_b, answer_c, answer_d FROM questions WHERE id_question = %d;", questionID);

  if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK)
  {
    
    fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(-1);
  }
  closeDatabase(db);
  
  if (sqlite3_step(res) == SQLITE_ROW) 
  {
    temp.answerA = sqlite3_column_text(res, 0);
    temp.sizeA = sqlite3_column_bytes(res, 0);
    temp.answerB = sqlite3_column_text(res, 1);
    temp.sizeB = sqlite3_column_bytes(res, 1);
    temp.answerC = sqlite3_column_text(res, 2);
    temp.sizeC = sqlite3_column_bytes(res, 2);
    temp.answerD = sqlite3_column_text(res, 3);
    temp.sizeD = sqlite3_column_bytes(res, 3);
    return temp;
  }
  else
  {
    temp.sizeA = 0; temp.sizeB = 0; temp.sizeC = 0; temp.sizeD = 0;
    sqlite3_finalize(res);
    return temp;
  }
}

void sendAnswer(void *arg, int size, const unsigned char* answer)
{
  thData tdL; 
  tdL= *((struct thData*)arg);
  
  if (write (tdL.cl, &size, sizeof(size)) <= 0)
  {
    printf("[Thread %d] ", (int) tdL.idThread);
    perror ("[Thread]write() error sending to client.\n");
  }

  if (write (tdL.cl, answer, size) <= 0)
  {
    printf("[Thread %d] ", (int) tdL.idThread);
    perror ("[Thread]write() error sending to client.\n");
  }
}

void sendAnswers(void *arg, int questionID)
{
  thData tdL; 
  tdL= *((struct thData*)arg);
  struct answersAndSizes temp = getAnswers(1);

  sendAnswer((struct thData*)arg, temp.sizeA, temp.answerA);
  sendAnswer((struct thData*)arg, temp.sizeB, temp.answerB);
  sendAnswer((struct thData*)arg, temp.sizeC, temp.answerC);
  sendAnswer((struct thData*)arg, temp.sizeD, temp.answerD);
}

