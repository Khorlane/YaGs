//***************************
//* Yet Another Game Server *
//***************************

#define _DEFAULT_SOURCE                     // Required for usleep() in my development environment

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Includes
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include <arpa/inet.h>                      // This and sys/socket.h - a whole plethora of socket related stuff
#include <ctype.h>                          // isspace()
#include <errno.h>                          // EINTR
#include <fcntl.h>                          // fcntl(), F_SETFL, FNDELAY
#include <pwd.h>                            // getpwuid()
#include <stdbool.h>                        // bool data type
#include <stdio.h>                          // Standard I/O
#include <stdlib.h>                         // exit() malloc()
#include <string.h>                         // strcpy(), strcmp(), strlen()
#include <sys/socket.h>                     // This and arpa/inet - a whole plethora of socket related stuff
#include <time.h>                           // time(), ctime()
#include <unistd.h>                         // getpwuid(), getuid(), read(), usleep()

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Globals
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Booleans
bool                EndFile;                // File access - End of file
bool                Found;                  // File access - Record found
bool                GameShutDown;           // Set this to true to stop the game
bool                NoPlayers;              // True when we have no players

// Numbers
size_t              BufferLen;              // Length of the string stored in Buffer
long int            BytesRead;              // Number of bytes read
time_t              CurrentTimeSec;         // Current time in seconds
socklen_t           LingerSize;             // Size of Linger stucture
int                 Listen;                 // Listening socket
int                 MaxSocket;              // Maximum socket value
long                Offset;                 // Offset for fseek()
int                 OptVal;                 // Set socket option value
socklen_t           OptValSize;             // Size of socket option value
int                 PlayerNbr;              // Plyayer number
int                 ReturnValue1;           // Return value
size_t              ReturnValue2;           // Return value
long int            SendResult;             // Number of bytes sent to player
int                 Socket;                 // Socket value
socklen_t           SocketAddrSize;         // Size of Socket structure
extern int          errno;                  // Error number set by fopen(), for example
size_t              i;                      // A non-negative integer
size_t              j;                      // A non-negative integer
size_t              k;                      // A non-negative integer
size_t              x;                      // A non-negative integer
size_t              y;                      // A non-negative integer
size_t              z;                      // A non-negative integer

//Pointers
char               *CurrentTime;            // Current timestamp
char               *HomeDir;                // Value of $HOME Linux Environment Variable
FILE               *GreetingFile;           // Greeting file
FILE               *LogFile;                // Log file
FILE               *PlayerFile;             // Player file
FILE               *ValidNamesFile;         // Valid names file
struct Players     *pPlayer;                // Pointer to player
struct Players     *pPlayerSave;            // Pointer to player - save
struct Players     *pPlayerCurr;            // Pointer to current player in the player list
struct Players     *pPlayerCurrSave;        // Pointer to current player in the player list - save
struct Players     *pPlayerHead;            // Pointer to head of player list
struct Players     *pPlayerTail;            // Pointer to tail of player list
struct Players     *pTarget;                // Pointer to target player
struct passwd      *pw;                     // Password struct (used to get $HOME environment variable)

// Strings
 char               Buffer[1024];           // Just a buffer
char                Command[1024];          // The command from the player
char                GreetingFileName[50];   // Greeting file name
char                LogFileName[50];        // Log file name
char                LogMsg[100];            // Log message
char                MsgTxt[100];            // Message text
char                MudCmd[10];             // Mud command
char                TheRest[50];            // The rest of the command
char                aTmpStr[50];            // Temp string
char                *TmpStr = aTmpStr;      // Temp sting too
char                PlayerFileName[50];     // Player file name
char                ValidNamesFileName[50]; // Valid names file name

// Structures
fd_set              InpSet;                 // File Descriptor Set structure
struct linger       Linger;                 // Linger structure
struct sockaddr_in  SocketAddr;             // Socket Address structure
struct timeval      TimeOut;                // Time value structure

// Messages
char               *GameSleepMsg = "No Connections: Going to sleep";      // Game sleeping message
char               *GameStartMsg = "YaGs v1.0.4 Starting";                // Game starting message
char               *GameStopMsg  = "YaGs has shutdown";                   // Game stop message
char               *GameWakeMsg  = "Waking up";                           // Game wake up message

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Player
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

typedef enum
{
  Send_Greeting,
  Wait_New_Player_YN,
  Wait_Player_Name,
  Wait_Password,
  Wait_Sex,
  Wait_New_Player_Name,
  Wait_Sure_YN,
  Wait_Password1,
  Wait_Password2,
  Online,
  Disconnect
} PlayerState;

struct Players                              // Players structure - list of connected players
{
  int             Socket;
  PlayerState     State;
  char            Name[50];
  char            Password[50];
  char            Afk;
  char            Admin;
  time_t          Born;
  int             Experience;
  char            Level;
  char            Sex;
  char            Input[1024];
  char            Output[1024];
  int             BadPswdCount;
  int             PlayerNbr;
  struct Players *pPlayerNext;
  struct Players *pPlayerPrev;
};

struct sPlayer                              // Player structure - used when reading and writing player file
{
  char            Name[50];
  char            Password[50];
  char            Afk;
  char            Admin;
  time_t          Born;
  int             Experience;
  char            Level;
  char            Sex;
} Player;

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Macros
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#define DEBUGIT(dl)      if (DEBUGIT_LVL >= dl) {sprintf(LogMsg,"*** %s ***",__FUNCTION__);LogIt(LogMsg);} // dl = debug level
#define DEBUGIT_LVL      1                // Range of 0 to 5 with 0 = No debug messages and 5 = Maximum debug messages
#define GREETING_FILE    "Greeting.txt"   // Greeting file
#define LIB_DIR          "Library"        // Library directory
#define LOG_DIR          "Logs"           // Log directory
#define LOG_FILE         "Log.txt"        // Log file
#define PLAYER_FILE      "Player.yags"    // Player file
#define PORT             7777             // Port number
#define SLEEP_TIME       0400000          // Sleep for a short period of time
#define USE_USLEEP       'N'              // Use usleep() Y or N
#define VALID_NAMES_FILE "ValidNames.txt" // Valid names file
#define WORLD_DIR        "World"          // World directory
#define YAGS_HOME_DIR    "YaGs"           // YaGs home directory

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Functions
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
void    AbortIt();
void    AcceptNewPlayer();
void    AddPlayerToFile();
void    AddToPlayerList();
void    CheckForNewPlayers();
void    CloseFiles();
void    CloseLog();
bool    MudCmdOk();
void    CopyPlayerListToPlayer();
void    CopyPlayerToPlayerList();
void    DelFromPlayerList();
void    DoAdvance();
void    DoPlayerfile();
void    DoShutdown();
bool    Equal(char* S1, char* S2);
void    GetNextPlayerNbr();
long    GetPlayerFileOffset();
void    GetPlayerInput();
void    GetPlayerOnline();
void    GetTime();
void    HeartBeat();
void    InitalizeNewPlayer();
void    Initialization();
void    LogIt(char* LogMsg);
void    LowerCase(char* Str);
void    OpenFiles();
void    OpenLog();
bool    PlayerNameValid();
bool    PlayerNameValidNew();
bool    PlayerNameValidOld();
void    ProcessCommand();
void    ProcessPlayerInput();
void    Prompt(struct Players* pPlayer);
void    ReadPlayerFromFile();
void    SendGreeting();
void    SendPlayerOutput();
void    SendToAll();
void    ShutItDown();
void    Sleep();
void    SocketListen();
void    StartItUp();
void    Trim(char* Str);
void    Word(size_t Nbr, char* S1, char* S2);
size_t  Words(char* Str);
void    WritePlayerToFile();

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Main
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

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
    ProcessPlayerInput();
    SendPlayerOutput();
    Sleep();
  }
  ShutItDown();
}

void HeartBeat()
{
  DEBUGIT(2)
}

void ProcessPlayerInput()
{
  DEBUGIT(2)
  GetPlayerInput();
  pPlayerCurr = pPlayerHead;
  while (pPlayerCurr != NULL)
  {
    pPlayer = pPlayerCurr;
    if (pPlayer->Input[0] != '\0')
    {
      strcpy(Command, pPlayer->Input);
      pPlayer->Input[0] = '\0';
      ProcessCommand();
    }
    pPlayerCurr = pPlayerCurr->pPlayerNext;
  }
}

void ProcessCommand() // TODO ProcessCommand()
{
  DEBUGIT(1)
  Trim(Command);
  LogIt(Command);
  if (pPlayer->State != Online)
  {
    GetPlayerOnline();
    return;
  }
  Word(1, Command, MudCmd);
  LowerCase(MudCmd);
  if (!MudCmdOk()) return;
  // Commands
  if (Equal(MudCmd, "advance"))
  {
    DoAdvance();
    return;
  }
  if (Equal(MudCmd, "playerfile"))
  {
    DoPlayerfile();
    return;
  }
  if (Equal(MudCmd, "shutdown"))
  {
    DoShutdown();
    return;
  }
  // if we get here something is broke bad!!
  strcat(pPlayer->Output, "\r\nYou should never see this!!\r\n");
  Prompt(pPlayer);
}

void DoAdvance()
{
  DEBUGIT(1)
  pPlayerCurrSave = pPlayerCurr;
  pTarget = NULL;
  Word(2, Command, TmpStr);
  while (pPlayerCurr != NULL)
  {
    if (Equal(pPlayerCurr->Name, TmpStr))
    {
      pTarget = pPlayerCurr;
      break;
    }
    pPlayerCurr = pPlayerCurr->pPlayerNext;
  }
  pPlayerCurr = pPlayerCurrSave;
  if (pTarget == NULL)
  {
    strcat(pPlayer->Output, "\r\n");
    sprintf(Buffer, "%s %s", TmpStr, "is not online\r\n");
    strcat(pPlayer->Output, Buffer);
    strcat(pPlayer->Output, "\r\n");
    Prompt(pPlayer);
    return;
  }
}

void DoPlayerfile()
{
  DEBUGIT(1)
  strcat(pPlayer->Output, "\r\n");
  strcat(pPlayer->Output, "Player file listing\r\n");
  strcat(pPlayer->Output, "-------------------\r\n");
  EndFile = false;
  PlayerNbr = 1;
  ReadPlayerFromFile();
  while (EndFile == false)
  {
    sprintf(Buffer, "%s %c %i", Player.Name, Player.Admin, Player.Level);
    strcat(pPlayer->Output, Buffer);
    strcat(pPlayer->Output, "\r\n");
    PlayerNbr++;
    ReadPlayerFromFile();
  }
  strcat(pPlayer->Output, "\r\n");
  Prompt(pPlayer);
}

void DoShutdown()
{
  DEBUGIT(1)
    GameShutDown = true;
  strcpy(MsgTxt, "\r\nYaGs is shutting down!\r\n");
  SendToAll();
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// General player communication
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

void Prompt(struct Players *pPlayer)
{
  DEBUGIT(1)
  //strcat(pPlayer->Output,"\r\n");
  strcat(pPlayer->Output,"> ");
}

void SendToAll()
{
  DEBUGIT(1)
  pPlayerSave     = pPlayer;
  pPlayerCurrSave = pPlayerCurr;
  pPlayerCurr     = pPlayerHead;
  while (pPlayerCurr != NULL)
  {
    pPlayer = pPlayerCurr;
    if (pPlayer != pPlayerSave)
    {
      strcat(pPlayer->Output,"\r\n");
    }
    strcat(pPlayer->Output, MsgTxt);
    Prompt(pPlayer);
    pPlayerCurr = pPlayerCurr->pPlayerNext;
  }
  pPlayer     = pPlayerSave;
  pPlayerCurr = pPlayerCurrSave;
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Get player online
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

void GetPlayerOnline()
{
  DEBUGIT(1)
  //***********************************
  // Send Greeting
  //***********************************
  if (pPlayer->State == Send_Greeting)
  {
    SendGreeting();
    pPlayer->State = Wait_New_Player_YN;
    return;
  }
  //***********************************
  // New player Y-N
  //***********************************
  if (pPlayer->State == Wait_New_Player_YN)
  {
    if (Equal(Command, "n"))
    { // Returning Player
      strcat(pPlayer->Output,"\r\nName?\r\n");
      pPlayer->State = Wait_Player_Name;
      Prompt(pPlayer);
      return;
    }
    if (Equal(Command, "y"))
    { // New Player
      strcat(pPlayer->Output,"\r\nSex M-F?\r\n");
      pPlayer->State = Wait_Sex;
      Prompt(pPlayer);
      return;
    }
    // Player didn't respond with a Y or N
    strcat(pPlayer->Output, "\r\nNew Player? Y or N\r\n");
    Prompt(pPlayer);
    return;
  }
  //***********************************
  // Returning player - Name
  //***********************************
  if (pPlayer->State == Wait_Player_Name)
  { // TODO Old player
    if (PlayerNameValid())
    { // Name is valid, ask for password
      CopyPlayerToPlayerList();
      strcat(pPlayer->Output, "\r\nPassword?\r\n");
      pPlayer->State = Wait_Password;
      Prompt(pPlayer);
      return;
    }
    // Didn't find player name, try again
    strcat(pPlayer->Output, "\r\n");
    strcat(pPlayer->Output, Command);
    strcat(pPlayer->Output, " not found, try again\r\n");
    strcat(pPlayer->Output, "\r\nName?\r\n");
    Prompt(pPlayer);
    return;
  }
  //***********************************
  // Returning player - Password
  //***********************************
  if (pPlayer->State == Wait_Password)
  {
    if (Equal(Command, pPlayer->Password))
    { // Password is valid
      strcat(pPlayer->Output, "\r\nWelcome to YaGs!!\r\n");
      pPlayer->State = Online;
      Prompt(pPlayer);
      return;
    }
    // Wrong password
    strcat(pPlayer->Output, "\r\nWrong password, try again\r\n");
    strcat(pPlayer->Output, "\r\nPassword?\r\n");
    Prompt(pPlayer);
    return;
  }
  //***********************************
  // New player - Sex
  //***********************************
  if (pPlayer->State == Wait_Sex)
  {
    if (Equal(Command, "m"))
    {
      pPlayer->Sex = 'M';
      strcat(pPlayer->Output, "\r\nName?\r\n");
      pPlayer->State = Wait_New_Player_Name;
      Prompt(pPlayer);
      return;
    }
    if (Equal(Command, "f"))
    {
      pPlayer->Sex = 'F';
      strcat(pPlayer->Output, "\r\nName?\r\n");
      pPlayer->State = Wait_New_Player_Name;
      Prompt(pPlayer);
      return;
    }
    strcat(pPlayer->Output, "\r\nSex not M or F, try again\r\n");
    strcat(pPlayer->Output, "\r\nSex M-F?\r\n");
    Prompt(pPlayer);
    return;
  }
  //***********************************
  // New player - Name
  //***********************************
  if (pPlayer->State == Wait_New_Player_Name)
  {
    if (PlayerNameValid())
    { // Name is valid, ask for password
      strcpy(pPlayer->Name, Command);
      strcat(pPlayer->Output, "\r\nPassword?\r\n");
      pPlayer->BadPswdCount = 0;
      pPlayer->State = Wait_Password1;
      Prompt(pPlayer);
      return;
    }
    // Didn't find player name, try again
    strcat(pPlayer->Output, "\r\n");
    strcat(pPlayer->Output, Command);
    strcat(pPlayer->Output, " not found, try again\r\n");
    strcat(pPlayer->Output, "\r\nName?\r\n");
    Prompt(pPlayer);
    return;
  }
  //***********************************
  // New player - Password1
  //***********************************
  if (pPlayer->State == Wait_Password1)
  {
    strcpy(pPlayer->Password, Command);
    strcat(pPlayer->Output, "\r\nRe-enter Password\r\n");
    pPlayer->State = Wait_Password2;
    Prompt(pPlayer);
    return;
  }
  //***********************************
  // New player - Password2
  //***********************************
  if (pPlayer->State == Wait_Password2)
  {
    if (Equal(pPlayer->Password, Command))
    { // TODO New player
      GetNextPlayerNbr();
      InitalizeNewPlayer();
      AddPlayerToFile();
      CopyPlayerToPlayerList();
      strcat(pPlayer->Output, "\r\nWelcome to YaGs!!\r\n");
      pPlayer->State = Online;
      Prompt(pPlayer);
      return;
    }
    // Check bad password count
    pPlayer->BadPswdCount++;
    if (pPlayer->BadPswdCount > 3)
    {
      strcat(pPlayer->Output, "\r\nThree tries, you're out!\r\n");
      pPlayer->State = Disconnect;
      return;
    }
    // Password don't match, try again
    strcat(pPlayer->Output, "\r\nPasswords don't match, try again\r\n");
    strcat(pPlayer->Output, "\r\nPassword?\r\n");
    pPlayer->State = Wait_Password1;
    Prompt(pPlayer);
    return;
  }
  strcpy(LogMsg,"ERROR: Logic - should never get here!");
  AbortIt();
}  

void SendGreeting()
{
  DEBUGIT(1)
  sprintf(GreetingFileName,"%s%s%s%s%s%s%s",HomeDir,"/",YAGS_HOME_DIR,"/",LIB_DIR,"/",GREETING_FILE);
  GreetingFile = fopen(GreetingFileName, "r");
  if (GreetingFile == NULL)
  {
    sprintf(LogMsg, "ERROR: Open %s failed: %s", GREETING_FILE, strerror(errno));
    AbortIt();
  }
  for (;;)
  {
    fgets(Buffer, sizeof(Buffer), GreetingFile);
    if (ferror(GreetingFile))
    {
      sprintf(LogMsg, "ERROR: Read %s failed: %s", GREETING_FILE, strerror(errno));
      AbortIt();
    }
    if (feof(GreetingFile))
    {
      break;
    }
    strcat(pPlayer->Output, Buffer);
  }
  fclose(GreetingFile);
  Prompt(pPlayer);
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Log
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

void OpenLog()
{
  sprintf(LogFileName,"%s%s%s%s%s%s%s",HomeDir,"/",YAGS_HOME_DIR,"/",LOG_DIR,"/",LOG_FILE);
  LogFile = fopen(LogFileName, "w");
  if (LogFile == NULL)
  {
    printf("Error opening %s : %s\r\n", LogFileName, strerror(errno));
    exit(1);
  }
  LogIt(GameStartMsg);
}

void LogIt(char *LogMsg)
{
  GetTime();
  fprintf(LogFile, "%s - %s\r\n", CurrentTime, LogMsg);
  fflush(LogFile);
}

void CloseLog()
{
  DEBUGIT(1)
  LogIt(GameStopMsg);
  fclose(LogFile);
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Sockets
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

void SocketListen()
{
  DEBUGIT(1)
  // Create listening socket
  Listen = socket(AF_INET, SOCK_STREAM, 0);
  if (Listen < 0)
  {
    close(Listen);
    sprintf(LogMsg,"Socket Error: socket - create listening socket - failed with error: %s", strerror(errno));
    AbortIt();
  }
  // Make Listen non-blocking
  ReturnValue1 = fcntl(Listen, F_SETFL, FNDELAY);
  if (ReturnValue1 < 0)
  {
    close(Listen);
    sprintf(LogMsg,"Socket Error: fcntl - make listening socket non-blocking - failed with return value: %d", ReturnValue1);
    AbortIt();
  }
  // Set SO_LINGER
  Linger.l_onoff  = 0;
  Linger.l_linger = 0;
  LingerSize      = sizeof(Linger);
  ReturnValue1 = setsockopt(Listen, SOL_SOCKET, SO_LINGER, &Linger, LingerSize);
  if (ReturnValue1 < 0)
  {
    close(Listen);
    sprintf(LogMsg,"Socket Error: setsockopt - SO_LINGER - failed with error: %s", strerror(errno));
    AbortIt();
  }
  // Set SO_REUSEADDR
  OptVal = 1;
  OptValSize = sizeof(OptVal);
  ReturnValue1 = setsockopt(Listen, SOL_SOCKET, SO_REUSEADDR, &OptVal, OptValSize);
  if (ReturnValue1 < 0)
  {
    close(Listen);
    sprintf(LogMsg,"Socket Error: setsockopt - SO_REUSEADDR - failed with error: %s", strerror(errno));
    AbortIt();
  }
  // Bind
  SocketAddr.sin_family      = AF_INET;
  SocketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  SocketAddr.sin_port        = htons(PORT);
  SocketAddrSize             = sizeof(SocketAddr);
  ReturnValue1 = bind(Listen, (struct sockaddr *) &SocketAddr, SocketAddrSize);
  if (ReturnValue1 < 0)
  {
    close(Listen);
    sprintf(LogMsg,"Socket Error: bind - listening port - failed with error: %s", strerror(errno));
    AbortIt();
  }
  // Listen on PORT for connections
  ReturnValue1 = listen(Listen, 20);
  if (ReturnValue1 < 0)
  {
    close(Listen);
    sprintf(LogMsg,"Socket Error: listen - listen on port - failed with error: %s", strerror(errno));
    AbortIt();
  }
  // YaGs is listening!!
  sprintf(LogMsg,"%s %d","YaGs is Listening on port", PORT);
  LogIt(LogMsg);
}

void CheckForNewPlayers()
{
  DEBUGIT(2)
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
  ReturnValue1 = select(MaxSocket + 1, &InpSet, NULL, NULL, &TimeOut);
  if ((ReturnValue1 < 0) && (errno != EINTR))
  {
    sprintf(LogMsg,"Socket Error: select - check connections - failed with error: %s", strerror(errno));
    AbortIt();
  }
  if (FD_ISSET(Listen, &InpSet))
  {
    AcceptNewPlayer();
  }
}

void AcceptNewPlayer()
{
  DEBUGIT(1)
  NoPlayers = false;
  Socket = accept(Listen, (struct sockaddr *) &SocketAddr, (socklen_t *) &SocketAddrSize);
  if (Socket < 0)
  {
    sprintf(LogMsg,"Socket Error: accept - new connection - failed with error: %s", strerror(errno));
    AbortIt();
  }
  sprintf(LogMsg,"New connection, socket fd is %d , ip is : %s , port : %d", Socket, inet_ntoa(SocketAddr.sin_addr), ntohs(SocketAddr.sin_port));
  LogIt(LogMsg);
  AddToPlayerList();
  GetPlayerOnline();
}

void GetPlayerInput()
{
  DEBUGIT(2)
  pPlayerCurr = pPlayerHead;
  while (pPlayerCurr != NULL)
  {
    pPlayer = pPlayerCurr;
    Socket = pPlayer->Socket;
    pPlayer->Input[0] = '\0';
    if (FD_ISSET(Socket, &InpSet))
    {
      BytesRead = read(Socket, Buffer, 1024);
      Buffer[BytesRead] = '\0';
      strcpy(pPlayer->Input, Buffer);
    }
    pPlayerCurr = pPlayerCurr->pPlayerNext;
  }
}

void SendPlayerOutput()
{
  DEBUGIT(2)
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
      strcpy(Buffer, "quit\0");
      perror("-- Send failed\r\n");
    }
    else
    {
      Buffer[0] = '\0';
    }
    pPlayerCurr = pPlayerCurr->pPlayerNext;
  }
}

void DisconnectPlayers()
{
  DEBUGIT(1)
  pPlayerCurr = pPlayerHead;
  while (pPlayerCurr != NULL)
  {
    pPlayer = pPlayerCurr;
    if (pPlayer->State == Disconnect)
    {
      close(pPlayer->Socket);
      DelFromPlayerList();
      continue;
    }
    pPlayerCurr = pPlayerCurr->pPlayerNext;
  }
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Start up and shutdown
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

void StartItUp()
{ // Do not add DEBUGIT
  Initialization();
  OpenLog();
  SocketListen();
  OpenFiles();
}

void Initialization()
{ // Do not add DEBUGIT
  pw           = getpwuid(getuid());
  HomeDir      = pw->pw_dir;
  GameShutDown = false;
  NoPlayers    = true;
  pPlayerHead  = NULL;
  pPlayerTail  = NULL;
  pPlayerCurr  = NULL;
}

void OpenFiles()
{
  DEBUGIT(1)
  sprintf(PlayerFileName,"%s%s%s%s%s%s%s",HomeDir,"/",YAGS_HOME_DIR,"/",WORLD_DIR,"/",PLAYER_FILE);
  PlayerFile = fopen(PlayerFileName, "r+");
  if (PlayerFile == NULL)
  {
    sprintf(LogMsg,"Error opening %s : %s", PlayerFileName, strerror(errno));
    AbortIt();
  }
}

void ShutItDown()
{
  DEBUGIT(1)
  CloseFiles();
  CloseLog();
}

void CloseFiles()
{
  DEBUGIT(1)
  fclose(PlayerFile);
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Strings
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

bool Equal(char *S1, char *S2)
{
  DEBUGIT(2)
  return (!strcmp(S1, S2));
}

void LowerCase(char *Str)
{
  DEBUGIT(1)
  for(i = 0; Str[i]; i++)
  {
    Str[i] = (char) tolower(Str[i]);
  }
}

void Trim(char *Str)
{
  DEBUGIT(1)
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

void Word(size_t Nbr, char *S1, char *S2)
{ // TODO Word()
  j = 0;
  x = 1;
  for (i = 0; S1[i]; i++)
  {
    if (x == Nbr)
    {
      break;
    }
    if (isspace(S1[i]))
    {
      x++;
    }
  }
  while (!isspace(S1[i]))
  {
    if (S1[i] == '\0')
    {
      break;
    }
    S2[j] = S1[i];
    i++;
    j++;
  }
  S2[j] = '\0';
}

size_t Words(char *Str)
{
  DEBUGIT(1)
  #define NotWord 0
  #define InWord  1
  int State;
  State = 0;
  x = 0;
  for (i = 0; Str[i]; i++)
  {
    if (isspace(Str[i]))
    {
      State = NotWord;
    }
    else if (State == NotWord)
    {
      State = InWord;
      x++;
    }
  }
  return x;
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Miscellaneous
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

void AbortIt()
{ // Do not add DEBUGIT
  LogIt(LogMsg);
  CloseLog();
  exit(1);
}

bool MudCmdOk()
{
  DEBUGIT(1)
  if (Equal(MudCmd, "advance"))    return true;
  if (Equal(MudCmd, "playerfile")) return true;
  if (Equal(MudCmd, "shutdown"))   return true;
  strcat(pPlayer->Output, "\r\nHuh?\r\n");
  Prompt(pPlayer);
  return false;
}

void GetTime()
{ // Do not add DEBUGIT
  CurrentTimeSec = time(NULL);              // Seconds since Epoch, 1970-01-01 00:00:00 +0000 (UTC)
  CurrentTime = ctime(&CurrentTimeSec);     // Convert to human readable
  x = strlen(CurrentTime);                  // Get rid of the '\n'
  CurrentTime[x-1] = '\0';                  //   at the end of string returned by ctime()
}

void Sleep()
{
  DEBUGIT(2)
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

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Player list
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

void AddToPlayerList()
{
  DEBUGIT(1)  
  pPlayer = malloc(sizeof(struct Players));
  pPlayerCurr = pPlayer;
  if (pPlayerHead != NULL)
  { // Not 1st Node
    pPlayerTail->pPlayerNext = pPlayerCurr;
    pPlayerCurr->pPlayerPrev = pPlayerTail;
    pPlayerTail = pPlayerCurr;
  }
  else
  { // 1st Node
    pPlayerHead = pPlayerCurr;
    pPlayerTail = pPlayerCurr;
  }
  pPlayer->Socket      = Socket;
  pPlayer->State       = Send_Greeting;
  pPlayer->Output[0]   = '\0';
  pPlayer->pPlayerNext = NULL;
  pPlayer->pPlayerPrev = NULL;
}

void DelFromPlayerList()
{
  DEBUGIT(1) 
  // Delete head node
  if (pPlayerCurr == pPlayerHead)
  {
    pPlayerHead = pPlayerCurr->pPlayerNext;
    pPlayerCurr->pPlayerNext->pPlayerPrev = NULL;
  }
  // Delete tail node
  if (pPlayerCurr == pPlayerTail)
  {
    pPlayerTail = pPlayerCurr->pPlayerPrev;
    pPlayerCurr->pPlayerPrev->pPlayerNext = NULL;
  }
  // Delete middle node
  pPlayerCurr->pPlayerPrev->pPlayerNext = pPlayerCurr->pPlayerNext;
  pPlayerCurr->pPlayerNext->pPlayerPrev = pPlayerCurr->pPlayerPrev;
  // Free node
  pPlayerCurr = pPlayer->pPlayerNext;
  free(pPlayer);
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Player file
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

long GetPlayerFileOffset()
{
  DEBUGIT(1)
  x = (size_t)sizeof(Player);
  y = (size_t)PlayerNbr - 1;
  Offset = (long)(x * y);
  return Offset;
}

bool PlayerNameValid()
{
  DEBUGIT(1)
  if (pPlayer->State == Wait_Player_Name)
  {
    return PlayerNameValidOld();
  }
  if (pPlayer->State == Wait_New_Player_Name)
  {
    return PlayerNameValidNew();
  }
  strcpy(LogMsg,"ERROR: Logic - should never get here!");
  AbortIt();
  return false;
}

bool PlayerNameValidOld()
{  
  DEBUGIT(1)
  Found     = false;
  EndFile   = false;
  PlayerNbr = 1;
  ReadPlayerFromFile();
  while (!EndFile)
  {
    if (Equal(Player.Name, Command))
    { // Match!
      Found = true;
      break;
    }
    PlayerNbr++;
    ReadPlayerFromFile();
  }
  return Found;
}

bool PlayerNameValidNew()
{
  DEBUGIT(1)
  sprintf(ValidNamesFileName,"%s%s%s%s%s%s%s",HomeDir,"/",YAGS_HOME_DIR,"/",LIB_DIR,"/",VALID_NAMES_FILE);
  ValidNamesFile = fopen(ValidNamesFileName, "r");
  if (ValidNamesFile == NULL)
  {
    sprintf(LogMsg, "ERROR: Open %s failed: %s", VALID_NAMES_FILE, strerror(errno));
    AbortIt();
  }
  Found = false;
  for (;;)
  {
    fgets(Buffer, sizeof(Buffer), ValidNamesFile);
    if (ferror(ValidNamesFile))
    {
      sprintf(LogMsg, "ERROR: Read %s failed: %s", VALID_NAMES_FILE, strerror(errno));
      AbortIt();
    }
    if (feof(ValidNamesFile))
    {
      Found = false;
      break;
    }
    Trim(Buffer);
    if (Equal(Command, Buffer))
    { // Match!
      Found = true;
      break;
    }
  }
  fclose(ValidNamesFile);
  return Found;
}

void ReadPlayerFromFile()
{
  DEBUGIT(1)
  fseek(PlayerFile, GetPlayerFileOffset(), SEEK_SET);
  fread(&Player, sizeof(Player), 1, PlayerFile);
  if (feof(PlayerFile))
  {
    EndFile = true;
    return;
  }
  if (ferror(PlayerFile))
  {
    sprintf(LogMsg,"ERROR: Reading %s", PLAYER_FILE);
    AbortIt();
  }
}

void WritePlayerToFile()
{
  DEBUGIT(1)
  ReturnValue1 = fseek(PlayerFile, GetPlayerFileOffset(), SEEK_SET);
  if (ReturnValue1 != 0)
  {
    sprintf(LogMsg,"ERROR: fseek %s", PLAYER_FILE);
    AbortIt();
  }
  ReturnValue2 = fwrite(&Player, sizeof(Player), 1, PlayerFile);
  if (ReturnValue2 != 1)
  {
    sprintf(LogMsg,"ERROR: fwrite %s", PLAYER_FILE);
    AbortIt();
  }
  ReturnValue1 = fsync(fileno(PlayerFile));
  if (ReturnValue1 != 0)
  {
    sprintf(LogMsg, "ERROR: fsync %s", PLAYER_FILE);
    AbortIt();
  }
}

void AddPlayerToFile()
{
  DEBUGIT(1)
  ReturnValue1 = fseek(PlayerFile, 0, SEEK_END);
  if (ReturnValue1 != 0)
  {
    sprintf(LogMsg,"ERROR: fseek %s", PLAYER_FILE);
    AbortIt();
  }
  ReturnValue2 = fwrite(&Player, sizeof(Player), 1, PlayerFile);
  if (ReturnValue2 != 1)
  {
    sprintf(LogMsg,"ERROR: fwrite %s", PLAYER_FILE);
    AbortIt();
  }
  ReturnValue1 = fsync(fileno(PlayerFile));
  if (ReturnValue1 != 0)
  {
    sprintf(LogMsg, "ERROR: fsync %s", PLAYER_FILE);
    AbortIt();
  }
}

void CopyPlayerToPlayerList()
{
  DEBUGIT(1)
  strcpy(pPlayer->Name,     Player.Name);
  strcpy(pPlayer->Password, Player.Password);
  pPlayer->Admin          = Player.Admin;
  pPlayer->Afk            = Player.Afk;
  pPlayer->Born           = Player.Born;
  pPlayer->Experience     = Player.Experience;
  pPlayer->Level          = Player.Level;
  pPlayer->Sex            = Player.Sex;
  pPlayer->PlayerNbr      = PlayerNbr;
  pPlayer->BadPswdCount   = 0;
}

void CopyPlayerListToPlayer()
{
  DEBUGIT(1)
  strcpy(Player.Name,     pPlayer->Name);
  strcpy(Player.Password, pPlayer->Password);
  Player.Admin          = pPlayer->Admin;
  Player.Afk            = pPlayer->Afk;
  Player.Born           = pPlayer->Born;
  Player.Experience     = pPlayer->Experience;
  Player.Level          = pPlayer->Level;
  Player.Sex            = pPlayer->Sex;
}

void InitalizeNewPlayer()
{
  DEBUGIT(1)
  strcpy(Player.Name,     pPlayer->Name);
  strcpy(Player.Password, pPlayer->Password);
  Player.Sex            = pPlayer->Sex;
  Player.Admin          = 'N';
  Player.Afk            = 'N';
  Player.Born           = time(NULL);
  Player.Experience     = 0;
  Player.Level          = 1;
}

void GetNextPlayerNbr()
{
  DEBUGIT(1)
  EndFile = false;
  PlayerNbr = 1;
  ReadPlayerFromFile();
  while (EndFile == false)
  {
    PlayerNbr++;
    ReadPlayerFromFile();
  }
}
