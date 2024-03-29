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

#define PORT 2018
#define QUERY_SIZE 1024
#define USER_DATA_SIZE 64
#define MAX_CLIENTS_PER_TABLE 2

extern int errno;

int indexGlobal;
int playersConnected;

typedef struct thData{
  pthread_t idThread; 
  int cl; 
  char* username;
  int points;
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

static void *treat(void *arg); 
void respond(void *arg);
int checkUsernamePassword(char *username, char *password);
void login(void *arg);
int checkForValidUsername(char *username, char *password);
void userRegister(void *arg);
void addUsernameToStruct(void *arg, char *username);
sqlite3* openDatabase();
void closeDatabase(sqlite3 *db);
void addQuestion();
void addingQuestion();
struct questionAndSize getQuestion(int questionID);
struct answersAndSizes getAnswers(int questionID);
void sendQuestion(void *arg, int questionID);
void sendAnswers(void *arg, int questionID);
void sendAnswer(void *arg, int size, const unsigned char *answer);
int isAnswerCorrect(const unsigned char *answer, int questionID);
void answerFromClient(void *arg, unsigned char **answer);
void sendAnswerCorrectness(void *arg, int questionID);
int getNumberOfQuesitons();
void runningTheGame(void *arg);
void checkClientStateForWrite(pthread_t idThread, int sendRtn, int size);
void checkClientStateForRead(pthread_t idThread, int readRtn);
void sendWinner(thData *tds);

int main()
{
  struct sockaddr_in server;
  struct sockaddr_in from;	
  int sd;		

  addingQuestion();

  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror ("[server]Eroare la socket().\n");
    return errno;
  }
 
  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));

  server.sin_family = AF_INET;	
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  server.sin_port = htons (PORT);

  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
  {
    perror ("[server]Eroare la bind().\n");
    return errno;
  }
 
  if (listen (sd, 2) == -1)
  {
    perror ("[server]Eroare la listen().\n");
    return errno;
  }

  indexGlobal = 0;
  thData tds[25];
  while (1)
  {
    int client;
    unsigned int length = sizeof (from);

    printf ("[server] Port: %d...\n",PORT);
    fflush (stdout);

    if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
    {
      perror ("[server]Eroare la accept().\n");
      continue;
    }

    tds[indexGlobal].cl = client;

    pthread_create(&tds[indexGlobal].idThread, NULL, &treat, &tds[indexGlobal]);  
    indexGlobal++;

    if(indexGlobal == MAX_CLIENTS_PER_TABLE)
    {   
      sendWinner(tds); 
      indexGlobal = 0;
    }
  }    
};

static void *treat(void *arg)
{		
  thData *tdL = (struct thData*)arg; 
  printf ("[thread]- %d - connected\n", (int) tdL->idThread); 
  respond(arg);
  close ((intptr_t)arg);
  return(NULL);	
};

void checkClientStateForRead(pthread_t idThread, int readRtn)
{
  if (readRtn == 0)
  {
    if(playersConnected > 0)
      playersConnected--;
    indexGlobal--;
    printf("[Thread %d] This client exited earlier!\n", (int) idThread);
    pthread_exit(0);
  }
}

void checkClientStateForWrite(pthread_t idThread, int sendRtn, int size)
{
  if (sendRtn != size)
  {
    if(playersConnected > 0)
      playersConnected--;
    indexGlobal--;
    printf("[Thread %d] This client exited earlier!\n", (int) idThread);
    pthread_exit(0);
  }
}


void checkForSufficentClients(void * arg)
{
  thData *tdL = (struct thData*)arg;
  int sendRtn, ok = 0;

  //printf("[infunctie]Players connected: %d\n", playersConnected);
  
  while(playersConnected != MAX_CLIENTS_PER_TABLE) ;

  ok = 1;
  sendRtn = send(tdL->cl, &ok, sizeof(ok), MSG_NOSIGNAL);
  checkClientStateForWrite(tdL->idThread, sendRtn, sizeof(ok));
}


void respond(void *arg)
{
  thData *tdL = (struct thData*)arg; 
  int code;
  int readRtn = read (tdL->cl, &code, sizeof(code));

  checkClientStateForRead(tdL->idThread, readRtn);

  if (readRtn < 0)
  {
    printf("[Thread %d]\n", (int) tdL->idThread);
    perror ("respond: read() - 1 error from client.\n");
  }

  if(code == 1)
  {
    login(arg);
  }
  else if(code == 2)
  {
    userRegister((struct thData*)arg);
  }

  playersConnected++; 
  checkForSufficentClients(arg);
  runningTheGame(arg);
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

void login(void *arg)
{
  thData *tdL = (struct thData*)arg; 
  int ok = 0;
  char username[USER_DATA_SIZE], password[USER_DATA_SIZE];

  while(!ok)
  {
    int readRtn = read (tdL->cl, username, USER_DATA_SIZE);
    checkClientStateForRead(tdL->idThread, readRtn);

    if (readRtn <= 0)
    {
      printf("[Thread %d]\n", (int) tdL->idThread);
      perror ("login: read() - 1 error from client.\n");
    }

    readRtn = read (tdL->cl, password, USER_DATA_SIZE);
    checkClientStateForRead(tdL->idThread, readRtn);

    if (readRtn < 0)
    {
      printf("[Thread %d]\n", (int) tdL->idThread);
      perror ("login: read() - 2 error from client.\n");
    }

    ok = checkUsernamePassword(username, password);
    if(ok)
      addUsernameToStruct(arg, username);

    int sendRtn = send(tdL->cl, &ok, sizeof(ok), MSG_NOSIGNAL);
    checkClientStateForWrite(tdL->idThread, sendRtn, sizeof(ok));

    if (sendRtn < 0)
    {
      printf("[Thread %d] ", (int) tdL->idThread);
      perror ("[Thread]login: write() - 1 error sending to client.\n");
    }
  }

  printf ("[Thread %d] %s logged in successfully.\n", (int) tdL->idThread, tdL->username);
}

int checkForValidUsername(char *username, char *password)
{
  sqlite3 *db = openDatabase();
  char *errMsg = 0;

  char sql[QUERY_SIZE];
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

void userRegister(void *arg)
{
  thData *tdL = (struct thData*)arg;   
  int ok = 0;
  char username[USER_DATA_SIZE], password[USER_DATA_SIZE];
  while(!ok)
  {
    int readRtn = read (tdL->cl, username, USER_DATA_SIZE);
    checkClientStateForRead(tdL->idThread, readRtn);

    if (readRtn <= 0)
    {
      printf("[Thread %d]\n",(int) tdL->idThread);
      perror ("register: read() - 1 error from client.\n");
    }

    readRtn = read (tdL->cl, password, USER_DATA_SIZE);
    checkClientStateForRead(tdL->idThread, readRtn);

    if (readRtn < 0)
    {
      printf("[Thread %d]\n", (int) tdL->idThread);
      perror ("register: read() - 2 error from client.\n");
    }

    ok = checkForValidUsername(username, password);
    if (ok)
      addUsernameToStruct((struct thData*)arg, username);

    int sendRtn = send(tdL->cl, &ok, sizeof(ok), MSG_NOSIGNAL);
    checkClientStateForWrite(tdL->idThread, sendRtn, sizeof(ok));

    if (sendRtn < 0)
    {
      printf("[Thread %d] ", (int) tdL->idThread);
      perror ("register write() - 1 error sending to client.\n");
    }
  }

  printf ("[Thread %d]%s registered successfully.\n", (int) tdL->idThread, tdL->username);
}

void addUsernameToStruct(void *arg, char *username)
{
  thData *tdL = (struct thData*)arg;
  int length = strlen(username);
  tdL->username = malloc(length);
  strcpy(tdL->username, username);
  tdL->username[length] = '\0';
  tdL->points = 0;
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

  return db;
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

  char sql[QUERY_SIZE];
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
  char sql[QUERY_SIZE];
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
  thData *tdL = (struct thData*)arg;
  struct questionAndSize temp = getQuestion(questionID);

  if(temp.size == 0)
    exit(0);

  int sendRtn = send(tdL->cl, &temp.size, sizeof(temp.size), MSG_NOSIGNAL);
  checkClientStateForWrite(tdL->idThread, sendRtn, sizeof(temp.size));

  if (sendRtn < 0)
  {
    printf("[Thread %d] ", (int) tdL->idThread);
    perror ("sendQuestion: write() - 1 error sending to client.\n");
  }

  sendRtn = send(tdL->cl, temp.question, temp.size, MSG_NOSIGNAL);
  checkClientStateForWrite(tdL->idThread, sendRtn, temp.size);

  if (sendRtn < 0)
  {
    printf("[Thread %d] ", (int) tdL->idThread);
    perror ("sendQuestion: write() - 2 error sending to client.\n");
  }
}

struct answersAndSizes getAnswers(int questionID)
{
  sqlite3 *db = openDatabase();
  sqlite3_stmt *res;
  struct answersAndSizes temp;
  char sql[QUERY_SIZE];
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
  thData *tdL = (struct thData*)arg;

  int sendRtn = send(tdL->cl, &size, sizeof(size), MSG_NOSIGNAL);
  checkClientStateForWrite(tdL->idThread, sendRtn, sizeof(size));
  
  if (sendRtn < 0)
  {
    printf("[Thread %d] ", (int) tdL->idThread);
    perror ("sendAnswer: write() - 1  error sending to client.\n");
  }

  sendRtn = send(tdL->cl, answer, size, MSG_NOSIGNAL);
  checkClientStateForWrite(tdL->idThread, sendRtn, size);

  if (sendRtn < 0)
  {
    printf("[Thread %d] ", (int) tdL->idThread);
    perror ("sendAnswer: write() - 2 error sending to client.\n");
  }
}

void sendAnswers(void *arg, int questionID)
{
  struct answersAndSizes temp = getAnswers(questionID);

  sendAnswer(arg, temp.sizeA, temp.answerA);
  sendAnswer(arg, temp.sizeB, temp.answerB);
  sendAnswer(arg, temp.sizeC, temp.answerC);
  sendAnswer(arg, temp.sizeD, temp.answerD);
}

void answerFromClient(void *arg, unsigned char **answer)
{
  thData *tdL = (struct thData*)arg;
  int length;
  int readRtn = read (tdL->cl, &length, sizeof(length));
  checkClientStateForRead(tdL->idThread, readRtn);

  if (readRtn < 0)
  {
    printf("[Thread %d]\n",(int) tdL->idThread);
    perror ("answerFromClient: read() - 1 error from client.\n");
  }
  
  *answer = malloc(length);
  readRtn = read (tdL->cl, *answer, length);
  checkClientStateForRead(tdL->idThread, readRtn);

  if (readRtn < 0)
  {
    printf("[Thread %d]\n",(int) tdL->idThread);
    perror ("answerFromClient: read() - 2 error from client.\n");
  }

  answer[0][length] ='\0';
}

int isAnswerCorrect(const unsigned char *answer, int questionID)
{
  sqlite3 *db = openDatabase();
  sqlite3_stmt *res;
  int ok;
  char sql[QUERY_SIZE];
  sprintf(sql, "SELECT correct_answer FROM questions WHERE id_question = %d;", questionID);

  if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK)
  {
    
    fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(-1);
  }
  closeDatabase(db);
  
  if (sqlite3_step(res) == SQLITE_ROW) 
  {
    if(!strcmp((char*) answer, (char*) sqlite3_column_text(res, 0)))
    {
      return 1;
    }
    else
      return 0;
  }
  else
  {
    sqlite3_finalize(res);
    return 0;
  }
}

int questionsPoints(int questionID)
{
  sqlite3 *db = openDatabase();
  sqlite3_stmt *res;
  int ok;
  char sql[QUERY_SIZE];
  sprintf(sql, "SELECT points FROM questions WHERE id_question = %d;", questionID);

  if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK)
  {
    
    fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(-1);
  }
  closeDatabase(db);
  
  if (sqlite3_step(res) == SQLITE_ROW) 
  {
    return atoi((const char*) sqlite3_column_text(res, 0));
  }
  else
  {
    sqlite3_finalize(res);
    fprintf(stderr, "questionsPoints: error database");
    return 0;
  }
}


void sendAnswerCorrectness(void *arg, int questionID)
{
  thData *tdL = (struct thData*)arg;
  unsigned char *answer;
  answerFromClient(arg, &answer);
  int ok = isAnswerCorrect((const unsigned char*) answer, questionID);
  free(answer);

  if(ok)
    tdL->points += questionsPoints(questionID);

  int sendRtn = send(tdL->cl, &ok, sizeof(ok), MSG_NOSIGNAL);
  checkClientStateForWrite(tdL->idThread, sendRtn, sizeof(ok));

  if (sendRtn < 0)
  {
    printf("[Thread %d] ", (int) tdL->idThread);
    perror ("sendAnswerCorrectness: write() - 1 error sending to client.\n");
  }
}


int getNumberOfQuesitons()
{
  sqlite3 *db = openDatabase();
  sqlite3_stmt *res;
  int ok;
  char sql[QUERY_SIZE];
  sprintf(sql, "SELECT COUNT(id_question) FROM questions");

  if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK)
  {
    
    fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(-1);
  }
  closeDatabase(db);
  
  if (sqlite3_step(res) == SQLITE_ROW) 
  {
    return atoi( (const char*) sqlite3_column_text(res, 0));
  }
  else
  {
    sqlite3_finalize(res);
    return -1;
  }
}

void runningTheGame(void *arg)
{
  thData *tdL = (struct thData*)arg;
  int n = getNumberOfQuesitons();
  int ok = 1;
  int sendRtn;
  for(int i = 1; i <= n; i++)
  {
    if(i > 1)
    {
      sendRtn = send(tdL->cl, &ok, sizeof(ok), MSG_NOSIGNAL);
      checkClientStateForWrite(tdL->idThread, sendRtn, sizeof(ok));

      if (sendRtn < 0)
      {
        printf("[Thread %d] ", (int) tdL->idThread);
        perror ("runningTheGame write() - 1 error sending to client.\n");
      }
    }
    sendQuestion((struct thData*)arg, i);
    sendAnswers((struct thData*)arg, i);
    sendAnswerCorrectness((struct thData*)arg, i);
  }

  ok = 0;
  sendRtn = send(tdL->cl, &ok, sizeof(ok), MSG_NOSIGNAL);
  checkClientStateForWrite(tdL->idThread, sendRtn, sizeof(ok));
  playersConnected--;
  if (sendRtn < 0)
  {
    printf("[Thread %d] ", (int) tdL->idThread);
    perror ("runningTheGame write() - 2 error sending to client.\n");
  }
}

void sendWinner(thData *tds)
{
  thData winnner;
  winnner.points = 0;

  for(int i = 0; i < indexGlobal; i++)
  {
    pthread_join(tds[i].idThread, NULL);
    if(tds[i].points >= winnner.points)
      winnner = tds[i];
  }

  for(int i = 0; i < indexGlobal; i++)
  {
    int length = strlen(winnner.username);

    int sendRtn = send(tds[i].cl, &length, sizeof(length), MSG_NOSIGNAL);
    checkClientStateForWrite(tds[i].idThread, sendRtn, sizeof(length));

    if (sendRtn < 0)
    {
      printf("[Thread %d] ", (int) tds[i].idThread);
      perror ("sendQuestion: write() - 1 error sending to client.\n");
    }

    sendRtn = send(tds[i].cl, winnner.username, length, MSG_NOSIGNAL);
    checkClientStateForWrite(tds[i].idThread, sendRtn, length);

    if (sendRtn < 0)
    {
      printf("[Thread %d] ", (int) tds[i].idThread);
      perror ("sendQuestion: write() - 2 error sending to client.\n");
    }
  }
  printf("Winner: %s\n", winnner.username);
}