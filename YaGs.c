//***************************
//* Yet Another Game Server *
//***************************

#define _DEFAULT_SOURCE                 // Required for usleep() in my development environment

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//$$ Includes
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include <arpa/inet.h>                  // This and sys/socket.h - a whole plethora of socket related stuff
#include <ctype.h>                      // isspace()
#include <errno.h>                      // EINTR
#include <fcntl.h>                      // fcntl(), F_SETFL, FNDELAY
#include <pwd.h>                        // getpwuid()
#include <stdbool.h>                    // bool data type
#include <stdio.h>                      // Standard I/O
#include <stdlib.h>                     // exit() malloc()
#include <string.h>                     // strcpy(), strcmp(), strlen()
#include <sys/socket.h>                 // This and arpa/inet - a whole plethora of socket related stuff
#include <time.h>                       // time(), ctime()
#include <unistd.h>                     // getpwuid(), getuid(), read(), usleep()

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//$$ Global variables
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Booleans
bool                GameShutDown;       // Set this to true to stop the game
bool                NoPlayers;          // True when we have no players

// Numbers
size_t              BufferLen;          // Length of the string stored in Buffer
long int            BytesRead;          // Number of bytes read
long int            CurrentTimeSec;     // Current time in seconds
socklen_t           LingerSize;         // Size of Linger stucture
int                 Listen;             // Listening socket
int                 MaxSocket;          // Maximum socket value
int                 OptVal;             // Set socket option value
socklen_t           OptValSize;         // Size of socket option value
int                 ReturnValue;        // Return value
long int            SendResult;         // Number of bytes sent to player
int                 Socket;             // Socket value
socklen_t           SocketSize;         // Size of Socket structure
extern int          errno;              // Error number set by fopen(), for example
size_t              i;                  // Just a number
size_t              j;                  // Just a number
size_t              x;                  // Just a number

//Pointers
char               *CurrentTime;        // Current timestamp
char               *HomeDir;            // Value of $HOME
FILE               *GreetFile;          // Greeting file
FILE               *LogFile;            // Log file
struct Players     *pPlayer;            // Pointer to player
struct Players     *pPlayerCurr;        // Pointer to current player in the player list
struct Players     *pPlayerHead;        // Pointer to head of player list
struct Players     *pPlayerTail;        // Pointer to tail of player list
struct passwd      *pw;                 // Password struct (used to get $HOME)

// Strings
char                Buffer[1024];       // Just a buffer
char                Command[1024];      // The command
char                GreetFileName[50];  // Greeting file name
char                LogFileName[50];    // Log file name
char                LogMsg[100];        // Log message

// Structures
fd_set              InpSet;             // File Descriptor Set structure
struct linger       Linger;             // Linger structure
struct sockaddr_in  SocketAddr;         // Socket Address structure
struct timeval      TimeOut;            // Time value structure

struct Players                          // Player structure
{
  int   Socket;   
  char  Name[50];
  char  Afk;
  char  Input[1024];
  char  Output[1024];
  struct Players *pPlayerNext;
};

// Messages
char               *GameSleepMsg = "No Connections: Going to sleep";      // Game sleeping message
char               *GameStartMsg = "YaGs v1.0.3 Starting";                // Game starting message
char               *GameStopMsg  = "YaGs has shutdown";                   // Game stop message
char               *GameWakeMsg  = "Waking up";                           // Game wake up message

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//$$ Macros
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#define DEBUGIT(dl)     if (DEBUGIT_LVL >= dl) {sprintf(LogMsg,"*** %s ***",__FUNCTION__);LogIt(LogMsg);} // dl = debug level
#define DEBUGIT_LVL     1               // Range of 0 to 5 with 0 = No debug messages and 5 = Maximum debug messages
#define GREET_FILE      "Greeting.txt"  // Greeting file
#define LIB_DIR         "Library"       // Library directory
#define LOG_DIR         "Logs"          // Log directory
#define LOG_FILE        "Log.txt"       // Log file
#define PORT            7777            // Port number
#define SLEEP_TIME      0400000         // Sleep for a short period of time
#define USE_USLEEP      'N'             // Use usleep() Y or N
#define YAGS_HOME_DIR   "YaGs"          // YaGs home directory

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//$$ Functions
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

void AbortIt();
void AcceptNewPlayer();
void CheckForNewPlayers();
void CloseLog();
void DoCmd();
void GetPlayerInput();
void GetTime();
void Greeting();
void HeartBeat();
void LogIt(char *LogMsg);
void LowerCase(char * Str);
void OpenLog();
void ProcessPlayerInput();
void SendPlayerOutput();
void ShutItDown();
void Sleep();
void SocketListen();
void StartItUp();
void Trim(char * Str);

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//$$ Main
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

int main(int argc, char **argv)
{
  StartItUp();
  while (!GameShutDown)
  {
    HeartBeat();
    CheckForNewPlayers();
    if (NoPlayers)
    {
      LogIt(GameSleepMsg);
      while (NoPlayers)
      {
        Sleep();
        CheckForNewPlayers();
      }
      LogIt(GameWakeMsg);
    }
    GetPlayerInput();
    ProcessPlayerInput();
    SendPlayerOutput();
    Sleep();
  }
  ShutItDown();
}

// Start the game up
void StartItUp()
{
  GameShutDown = false;
  NoPlayers    = true;
  pw = getpwuid(getuid());
  HomeDir = pw->pw_dir;
  OpenLog();
  SocketListen();
  pPlayerHead = NULL;
  pPlayerTail = NULL;
  pPlayerCurr = NULL;
}

// Heart beat
void HeartBeat()
{
  DEBUGIT(1)
}

// Check for new players
void CheckForNewPlayers()
{
  DEBUGIT(1)
  FD_ZERO(&InpSet);
  FD_SET(Listen, &InpSet);
  MaxSocket = Listen;
  pPlayerCurr = pPlayerHead;
  while (pPlayerCurr != NULL)
  {
    Socket = pPlayerCurr->Socket;
    FD_SET(Socket, &InpSet);
    if (Socket > MaxSocket)
    {
      MaxSocket = Socket;
    }
    pPlayerCurr = pPlayerCurr->pPlayerNext;
  }
  TimeOut.tv_sec  = 0;
  TimeOut.tv_usec = 1;
  ReturnValue = select(MaxSocket + 1, &InpSet, NULL, NULL, &TimeOut);
  if ((ReturnValue < 0) && (errno != EINTR))
  {
    sprintf(LogMsg,"Socket Error: select - check connections - failed with error: %s", strerror(errno));
    AbortIt();
  }
  if (FD_ISSET(Listen, &InpSet))
  {
    AcceptNewPlayer();
  }
}

// Accept new player
void AcceptNewPlayer()
{
  Socket = accept(Listen, (struct sockaddr *) &SocketAddr, (socklen_t *) &SocketSize);
  if (Socket < 0)
  {
    sprintf(LogMsg,"Socket Error: accept - new connection - failed with error: %s", strerror(errno));
    AbortIt();
  }
  sprintf(LogMsg,"New connection, socket fd is %d , ip is : %s , port : %d", Socket, inet_ntoa(SocketAddr.sin_addr), ntohs(SocketAddr.sin_port));
  LogIt(LogMsg);
  pPlayer = malloc(sizeof(struct Players));
  pPlayer->Socket = Socket;
  pPlayer->Name[0]     = '*';
  pPlayer->Name[1]     = '\0';
  pPlayer->Afk         = 'N';
  pPlayer->Input[0]    = '\0';
  pPlayer->Output[0]   = '\0';
  pPlayer->pPlayerNext = NULL;
  pPlayerCurr = pPlayer;
  if (pPlayerHead == NULL)
  {
    pPlayerHead = pPlayerCurr;
    pPlayerTail = pPlayerCurr;
  }
  else
  {
    pPlayerTail->pPlayerNext = pPlayerCurr;
    pPlayerTail = pPlayerCurr;
  }
  NoPlayers = false;
  Greeting();
}

// Greeting
void Greeting()
{
  sprintf(GreetFileName,"%s%s%s%s%s%s%s",HomeDir,"/",YAGS_HOME_DIR,"/",LIB_DIR,"/",GREET_FILE);
  GreetFile = fopen(GreetFileName, "r");
  if (GreetFile == NULL)
  {
    sprintf(LogMsg,"Error opening %s : %s", GREET_FILE, strerror(errno));
    AbortIt();
  }
  for (;;)
  {
    fgets(Buffer, sizeof(Buffer), GreetFile);
    if (ferror(GreetFile))
    {
      sprintf(LogMsg,"%s","Error reading %s : %s", GREET_FILE, strerror(errno));
      AbortIt();
    }
    if (feof(GreetFile))
    {
      break;
    }
    strcat(pPlayer->Output,Buffer);
  }
}

// Get player input
void GetPlayerInput()
{
  DEBUGIT(1)
  pPlayerCurr = pPlayerHead;
  while (pPlayerCurr != NULL)
  {
    pPlayer = pPlayerCurr;
    Socket = pPlayer->Socket;
    if (FD_ISSET(Socket, &InpSet))
    {
      BytesRead = read(Socket, Buffer, 1024);
      Buffer[BytesRead] = '\0';
      strcpy(pPlayer->Input, Buffer);
    }
    pPlayerCurr = pPlayerCurr->pPlayerNext;
  }
}

// Process player input
void ProcessPlayerInput()
{
  pPlayerCurr = pPlayerHead;
  while (pPlayerCurr != NULL)
  {
    pPlayer = pPlayerCurr;
    if (pPlayer->Input[0] != '\0')
    {
      strcpy(Command, pPlayer->Input);
      pPlayer->Input[0] = '\0';
      DoCmd();
    }
    pPlayerCurr = pPlayerCurr->pPlayerNext;
  }
}

// Send player output
void SendPlayerOutput()
{
  DEBUGIT(1)
  pPlayerCurr = pPlayerHead;
  while (pPlayerCurr != NULL)
  {
    pPlayer = pPlayerCurr;
    if (pPlayer->Output[0] == '\0')
    {
      pPlayerCurr = pPlayerCurr->pPlayerNext;
      continue;
    }
    strcpy(Buffer, pPlayer->Output);
    pPlayer->Output[0] = '\0';
    BufferLen = strlen(Buffer);
    Socket = pPlayer->Socket;
    SendResult = send(Socket, Buffer, BufferLen, 0);
    if (SendResult != BufferLen)
    {
      strcpy(Buffer,"quit\0");
      perror("-- Send failed\r\n");
    }
    else
    {
      Buffer[0] = '\0';
    }
    pPlayerCurr = pPlayerCurr->pPlayerNext;
  }
}

// Shut the game down
void ShutItDown()
{
  DEBUGIT(1)
  CloseLog();
}

// Open log
void OpenLog()
{
  sprintf(LogFileName,"%s%s%s%s%s%s%s",HomeDir,"/",YAGS_HOME_DIR,"/",LOG_DIR,"/",LOG_FILE);
  LogFile = fopen(LogFileName, "w");
  if (LogFile == NULL)
  {
    printf("Error opening %s : %s\r\n", LOG_FILE, strerror(errno));
    exit(1);
  }
  LogIt(GameStartMsg);
}

// Write to log
void LogIt(char *LogMsg)
{
  GetTime();
  fprintf(LogFile, "%s - %s\r\n", CurrentTime, LogMsg);
  fflush(LogFile);
}

// Close log
void CloseLog()
{
  LogIt(GameStopMsg);
  fclose(LogFile);
}

// Get current time
void GetTime()
{
  CurrentTimeSec = time(NULL);              // Seconds since Epoch, 1970-01-01 00:00:00 +0000 (UTC)
  CurrentTime = ctime(&CurrentTimeSec);     // Convert to human readable
  x = strlen(CurrentTime);                  // Get rid of the '\n'
  CurrentTime[x-1] = '\0';                  //   at the end of string returned by ctime()
}

// Sleep
void Sleep()
{
  DEBUGIT(1)
  if (USE_USLEEP == 'Y')                    // Sleeping the game is one way to avoid needless
  {                                         //   consumption of CPU. Typically, usleep() works
    usleep(SLEEP_TIME);                     //   just fine for this purpose. Using select() is
  }                                         //   another (not recommended) means of sleeping a
  else                                      //   process.
  {                                         // The YaGs development environment is Windows 10,
    #include <sys/select.h>                 //   Visual Studio, and WSL (Windows Subsystem for Linux)
    struct timeval TimeOut;                 //   Ubuntu and for some strange reason, usleep() does not
    TimeOut.tv_sec = 0;                     //   does not actually sleep.
    TimeOut.tv_usec = SLEEP_TIME;           // So this messy function is the result. You should  
    select(0, NULL, NULL, NULL, &TimeOut);  //   adjust SLEEP_TIME until you are happy.
  }
}

// Establish listening socket
void SocketListen()
{
  DEBUGIT(1)
  // Create listening socket
  Listen = socket(AF_INET, SOCK_STREAM, 0);
  if (Listen < 0)
  {
    sprintf(LogMsg,"Socket Error: socket - create listening socket - failed with error: %s", strerror(errno));
    AbortIt();
  }
  // Make Listen non-blocking
  ReturnValue = fcntl(Listen, F_SETFL, FNDELAY);
  if (ReturnValue < 0)
  {
    sprintf(LogMsg,"Socket Error: fcntl - make listening socket non-blocking - failed with return value: %d", ReturnValue);
    AbortIt();
  }
  // Set SO_LINGER
  Linger.l_onoff  = 0;
  Linger.l_linger = 0;
  LingerSize      = sizeof(Linger);
  ReturnValue = setsockopt(Listen, SOL_SOCKET, SO_LINGER, &Linger, LingerSize);
  if (ReturnValue < 0)
  {
    sprintf(LogMsg,"Socket Error: setsockopt - SO_LINGER - failed with error: %s", strerror(errno));
    AbortIt();
  }
  // Set SO_REUSEADDR
  OptVal = 1;
  OptValSize = sizeof(OptVal);
  ReturnValue = setsockopt(Listen, SOL_SOCKET, SO_REUSEADDR, &OptVal, OptValSize);
  if (ReturnValue < 0)
  {
    sprintf(LogMsg,"Socket Error: setsockopt - SO_REUSEADDR - failed with error: %s", strerror(errno));
    AbortIt();
  }
  // Bind
  SocketAddr.sin_family      = AF_INET;
  SocketAddr.sin_addr.s_addr = INADDR_ANY;
  SocketAddr.sin_port        = htons(PORT);
  SocketSize                 = sizeof(SocketAddr);
  ReturnValue = bind(Listen, (struct sockaddr *) &SocketAddr, SocketSize);
  if (ReturnValue < 0)
  {
    sprintf(LogMsg,"Socket Error: bind - listening port - failed with error: %s", strerror(errno));
    AbortIt();
  }
  // Listen on PORT for connections
  ReturnValue = listen(Listen, 20);
  if (ReturnValue < 0)
  {
    sprintf(LogMsg,"Socket Error: listen - listen on port - failed with error: %s", strerror(errno));
    AbortIt();
  }
  // YaGs is listening!!
  sprintf(LogMsg,"%s %d","YaGs is Listening on port", PORT);
  LogIt(LogMsg);
}

// Abort YaGs
void AbortIt()
{
  LogIt(LogMsg);
  CloseLog();
  exit(1);
}

// Trim a string
void Trim(char * Str)
{
  i = strlen(Str);
  i--;
  while (isspace(Str[i]))
  {
    Str[i] = '\0';
    if (i == 0)
    { 
      break;
    }
    i--;
  }
  j = i;
  i = 0;
  while (Str[0] == ' ')
  {
    for (i = 0; i < j; i++)
    {
      Str[i] = Str[i+1];
    }
    Str[i] = '\0';
  }
}

// Force string to lowercase
void LowerCase(char * Str)
{
  for(i = 0; Str[i]; i++)
  {
    Str[i] = (char) tolower(Str[i]);
  }
}

// Do command
void DoCmd()
{
  Trim(Command);
  LowerCase(Command);
  if (strcmp(Command, "shutdown") == 0)
  {
    GameShutDown = true;
    return;
  }
  strcpy(pPlayer->Output, "Huh?\r\n");
}
