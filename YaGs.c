//***************************
//* Yet another Game server *
//***************************


#define _DEFAULT_SOURCE                               // Required for a bunch of BSD socket stuff
#pragma GCC diagnostic push                           // Ignore warnings about sections
#pragma GCC diagnostic ignored "-Wunreachable-code"   //   of unreachable code.

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Includes
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include <arpa/inet.h>                                // This and sys/socket.h - a whole plethora of socket related stuff
#include <ctype.h>                                    // tolower(), isspace()
#include <errno.h>                                    // EINTR
#include <fcntl.h>                                    // fcntl(), F_SETFL, FNDELAY
#include <stdbool.h>                                  // bool data type
#include <stdio.h>                                    // Standard I/O
#include <stdlib.h>                                   // exit() malloc()
#include <string.h>                                   // strcat(), strcpy(), strcmp(), strlen()
#include <sys/socket.h>                               // This and arpa/inet - a whole plethora of socket related stuff
#include <time.h>                                     // time(), ctime()
#include <unistd.h>                                   // close(), read(), getuid(), usleep(), fsync()

#ifndef __builtin_free                                // Prevents E0020 identifier "_builtin_free" is undefined
#define __builtin_free free                           //   in stdio.hand in stdlib.h
#endif                                                // This doesn't seem to have anything to do with the code in YaGs.c

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Macros
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Configuration
#define DEBUGIT(dl)           if (DEBUGIT_LVL >= dl) {sprintf(LogMsg,"*** %s ***",__FUNCTION__);LogIt(LogMsg);} // dl = debug level
#define DEBUGIT_LVL           1                       // Range of 0 to 5 where 0 = No debug messages and 5 = Maximum debug messages
#define PORT                  7373                    // Port number
#define SLEEP_TIME            0400000                 // Sleep for a short period of time
#define USE_USLEEP            'N'                     // Use usleep() Y or N
// Directories
#define YAGS_DIR              "/mnt/c/YaGs"           // YaGs top level directory path
#define LIB_DIR               "Library"               // Library directory
#define WORLD_DIR             "World"                 // World directory
#define LOG_DIR               "Logs"                  // Log directory
// Library directory contents
#define GREETING_FILE         "Greeting.txt"          // Greeting file
#define HELP_FILE             "Help.txt"              // Help file
#define MOTD_FILE             "Motd.txt"              // Message of the day file
#define VALID_NAMES_FILE      "ValidNames.txt"        // Valid names file
// Log directory contents
#define LOG_FILE              "Log.txt"               // Log file
// World directory contents
#define PLAYER_FILE           "Player.yags"           // Player file
// Timer events
#define NO_INPUT_TICK         500                     // Ticks before checking if player is still there
#define NO_INPUT_COUNT_LIMIT  3                       // Triggers player disconnect after this limit is hit

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Globals
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Booleans
bool                EndFile;                          // File access - End of file
bool                Found;                            // File access - Record found
bool                GameShutDown;                     // Set this to true to stop the game
bool                NoPlayers;                        // True when we have no players

// Numbers
size_t              BufferLen;                        // Length of the string stored in Buffer
long int            BytesRead;                        // Number of bytes read
int                 CommandNbr;                       // Command number zero based
time_t              CurrentTimeSec;                   // Current time in seconds
socklen_t           LingerSize;                       // Size of Linger stucture
int                 Listen;                           // Listening socket
int                 MaxSocket;                        // Maximum socket value
long                Offset;                           // Offset for fseek()
int                 OptVal;                           // Set socket option value
socklen_t           OptValSize;                       // Size of socket option value
int                 PlayerNbr;                        // Plyayer number
int                 ReturnValue1;                     // Return value
size_t              ReturnValue2;                     // Return value
long int            SendResult;                       // Number of bytes sent to player
int                 Socket;                           // Socket value
socklen_t           SocketAddrSize;                   // Size of Socket structure
extern int          errno;                            // Error number set by fopen(), for example
size_t              i;                                // A non-negative integer
size_t              j;                                // A non-negative integer
size_t              k;                                // A non-negative integer
size_t              x;                                // A non-negative integer
size_t              y;                                // A non-negative integer
size_t              z;                                // A non-negative integer

//Pointers
char               *CurrentTime;                      // Current timestamp
struct PlayerList  *pActor;                           // Pointer to acting player in the player list
struct PlayerList  *pPlayer;                          // Pointer to a player in the player list - generic usage
struct PlayerList  *pPlayerSave;                      // Pointer to a player in the player list - save
struct PlayerList  *pPlayerCurr;                      // Pointer to current player in the player list
struct PlayerList  *pPlayerCurrSave;                  // Pointer to current player in the player list - save
struct PlayerList  *pPlayerHead;                      // Pointer to the head of player list
struct PlayerList  *pPlayerTail;                      // Pointer to the tail of player list
struct PlayerList  *pTarget;                          // Pointer to target player in the player list

// Strings
char                aTmpStr[1024];                    // Temp string
char                *TmpStr   = aTmpStr;              // Temp string too
char                aTmpStr1[1024];                   // Temp string 1
char                *TmpStr1  = aTmpStr1;             // Temp string 1 too
char                *CmdParm1 = aTmpStr1;             // Command Parameter 1
char                aTmpStr2[1024];                   // Temp string 2
char                *TmpStr2  = aTmpStr2;             // Temp string 2 too
char                *CmdParm2 = aTmpStr2;             // Command Parameter 2
char                aTmpStr3[1024];                   // Temp string 3
char                *TmpStr3  = aTmpStr3;             // Temp string 3 too
char                *CmdParm3 = aTmpStr3;             // Command Parameter 2
char                Buffer[2048];                     // Just a buffer
char                Command[1024];                    // The command from the player
char                LogMsg[100];                      // Log message
char                MsgTxt[100];                      // Message text
char                MudCmd[10];                       // Mud command
char                TheRest[50];                      // The rest of the command

// Files
char               *GreetingFileName   = aTmpStr;     // Greeting file name
char               *HelpFileName       = aTmpStr;     // Help file name
char               *LogFileName        = aTmpStr;     // Log file name
char               *MotdFileName       = aTmpStr;     // Message of the day file name
char               *PlayerFileName     = aTmpStr;     // Player file name
char               *ValidNamesFileName = aTmpStr;     // Valid names file name
FILE               *GreetingFile;                     // Greeting file
FILE               *HelpFile;                         // Help file
FILE               *LogFile;                          // Log file
FILE               *MotdFile;                         // Message of the day file
FILE               *PlayerFile;                       // Player file
FILE               *ValidNamesFile;                   // Valid names file

// Structures
fd_set              InpSet;                           // File Descriptor Set structure
struct linger       Linger;                           // Linger structure
struct sockaddr_in  SocketAddr;                       // Socket Address structure
struct timeval      TimeOut;                          // Time value structure

// Color codes
char              *Normal        = "\x1B[0;m";        // NORMAL     &N
char              *BrightBlack   = "\x1B[1;30m";      // BBLACK     &K
char              *BrightRed     = "\x1B[1;31m";      // BRED       &R
char              *BrightGreen   = "\x1B[1;32m";      // BGREEN     &G
char              *BrightYellow  = "\x1B[1;33m";      // BYELLOW    &Y
char              *BrightBlue    = "\x1B[1;34m";      // BBLUE      &B
char              *BrightMagenta = "\x1B[1;35m";      // BMAGENTA   &M
char              *BrightCyan    = "\x1B[1;36m";      // BCYAN      &C
char              *BrightWhite   = "\x1B[1;37m";      // BWHITE     &W
char              *None          = "";                // No Color

// Messages
char               *GameSleepMsg = "No Connections: Going to sleep";      // Game sleeping message
char               *GameStartMsg = "YaGs v1.0.4 Starting";                // Game starting message
char               *GameStopMsg  = "YaGs has shutdown";                   // Game stop message
char               *GameWakeMsg  = "Waking up";                           // Game wake up message

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Player
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

typedef enum PlayerStates
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

struct PlayerList                                        // Players structure - list of connected players
{
  int                Socket;
  PlayerState        State;
  char               Name[50];
  char               Password[50];
  char               Afk;
  char               Admin;
  time_t             Born;
  char               Color;
  int                Experience;
  char               Level;
  char               Sex;
  char               Input[1024];
  char               Output[2048];
  int                BadPswdCount;
  int                PlayerNbr;
  int                NoInputTick;
  int                NoInputCount;
  struct PlayerList *pPlayerNext;                     // Pointer to next player in the player list
  struct PlayerList *pPlayerPrev;                     // Pointer to previous player in the player list
};

struct sPlayer                                        // Player structure - used when reading and writing player file
{
  char            Name[50];                           // Player name
  char            Password[50];                       // Player password
  char            Afk;                                // Away from keyboard flag (Y/N)
  char            Admin;                              // Admin flag (Y/N) - Controls which commands are available to the player
  time_t          Born;                               // Time player was created
  char            Color;                              // Color code (Y/N) Y means that player output is run through the Color() function
  int             Experience;                         // Experience points
  char            Level;                              // Player level
  char            Sex;                                // Player sex (M/F)
} Player;

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
void    Color();
void    CopyPlayerListToPlayer();
void    CopyPlayerToPlayerList();
void    DelFromPlayerList();
void    DisconnectPlayers();
void    DoAdvance();
void    DoColor();
void    DoHelp();
void    DoPlayerfile();
void    DoQuit();
void    DoShutdown();
bool    Equal(char *Str1, char *Str2);
void    GetNextPlayerNbr();
long    GetPlayerFileOffset();
void    GetPlayerInput();
void    GetPlayerOnline();
void    GetTime();
void    HeartBeat();
void    InitalizeNewPlayer();
void    Initialization();
void    LogIt(char *LogMsg);
void    LowerCase(char *Str);
bool    MudCmdOk();
void    OpenFiles();
void    OpenLog();
bool    PlayerNameValid();
bool    PlayerNameValidNew();
bool    PlayerNameValidOld();
void    ProcessCommand();
void    ProcessPlayerInput();
void    Prompt(struct PlayerList *pPlayer);
void    ReadPlayerFromFile();
void    SendGreeting();
void    SendPlayerOutput();
void    SendMotd();
void    SendToAll();
void    ShutItDown();
void    Sleep();
void    SocketListen();
void    StartItUp();
void    Trim(char *Str);
void    Up1stChar(char *Str);
void    Word(size_t Nbr, char *Str1, char *Str2);
size_t  Words(char *Str);
void    WritePlayerToFile();

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Commands
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

struct sCommands                                       // Command structure
{
  char *Name;
  char *Admin;
  char *Level;
  char *Position;
  char *Social;
  char *Fight;
  char *Words;
  char *Parts;
  char *Message;
} Commands;

char *CommandTable[][9] = {
  // Name          Admin Level Position  Social Fight Words Parts Message
    {"advance",    "Y",  "1",  "sleep",  "N",   "N",  "3",  "3",  "Advance who and to what level?"} ,
    {"color",      "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"help",       "N",  "1",  "sleep",  "N",   "N",  "1",  "2",  "None"},
    {"playerfile", "Y",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"quit",       "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"shutdown",   "Y",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {NULL,         NULL, NULL, NULL,     NULL,  NULL, NULL, NULL, NULL}
};

// Command jump table
void (*DoCommand[])(void) = 
{
  DoAdvance, 
  DoColor,
  DoHelp,
  DoPlayerfile,
  DoQuit,
  DoShutdown
};

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

void ProcessCommand()
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
  if (MudCmdOk())
  { 
    DoCommand[CommandNbr]();
  }
}

void DoAdvance()
{
  DEBUGIT(1)
  pTarget         = NULL;
  pPlayerCurrSave = pPlayerCurr;
  pPlayerCurr     = pPlayerHead;
  Word(2, Command, CmdParm1);
  while (pPlayerCurr != NULL)
  {
    if (Equal(pPlayerCurr->Name, CmdParm1))
    {
      pTarget = pPlayerCurr;
      break;
    }
    pPlayerCurr = pPlayerCurr->pPlayerNext;
  }
  pPlayerCurr = pPlayerCurrSave;
  if (pTarget == NULL)
  {
    sprintf(Buffer, "%s %s", CmdParm1, "is not online\r\n");
    strcat(pPlayer->Output, Buffer);
    strcat(pPlayer->Output, "\r\n");
    Prompt(pPlayer);
    return;
  }
  Word(3, Command, CmdParm2);
  x = (size_t)atoi(CmdParm2);
  y = (size_t)(pTarget->Level);
  if (x == y)
  { // New level same as current level
    sprintf(Buffer, "%s %s %s %s", CmdParm1, "is already at level", CmdParm2, "\r\n");
    strcat(pPlayer->Output, Buffer);
    strcat(pPlayer->Output, "\r\n");
    Prompt(pPlayer);
    return;
  }
  if (x > 127)
  { // Level is type char and max value is 127
    sprintf(Buffer, "%s %s %s", "Level", CmdParm2, "is invalid\r\n");
    strcat(pPlayer->Output, Buffer);
    strcat(pPlayer->Output, "\r\n");
    Prompt(pPlayer);
    return;
  }
  // Advance the target player
  pActor = pPlayer;
  if (x > y)
  {
    sprintf(TmpStr, "%s", "promoted");
  }
  else
  {
    sprintf(TmpStr, "%s", "demoted");
  }
  pTarget->Level      = (char)atoi(CmdParm2);
  pTarget->Experience = (int)(pTarget->Level) * 100;
  // Message to target player
  strcat(pTarget->Output,"\r\n");
  sprintf(Buffer, "%s %s %s %s %s", pActor->Name, "has", TmpStr, "you to level", CmdParm2);
  strcat(pTarget->Output, Buffer);
  strcat(pTarget->Output, "\r\n\r\n");
  Prompt(pTarget);
  // Message to player
  sprintf(Buffer, "%s %s %s %s %s, ", pTarget->Name, "has been", TmpStr, "to level", CmdParm2);
  strcat(pActor->Output, Buffer);
  strcat(pActor->Output, "\r\n\r\n");
  Prompt(pActor);
  // Save target player
  pPlayer = pTarget;
  CopyPlayerListToPlayer();
  WritePlayerToFile();
  pPlayer = pActor;
}

void DoColor()
{
  DEBUGIT(1)
  if (Words(Command) == 1)
  {
    if (pPlayer->Color == 'Y')
    {
      strcat(pPlayer->Output, "&CColor&N is &Mon&N.\r\n\r\n");
      Prompt(pPlayer);
      return;
    }
    if (pPlayer->Color == 'N')
    {
      strcat(pPlayer->Output, "Color is off.\r\n\r\n");
      Prompt(pPlayer);
      return;
    }
  };
  Word(2, Command, CmdParm1);
  LowerCase(CmdParm1);
  if (Equal(CmdParm1, "on"))
  {
    pPlayer->Color = 'Y';
    strcat(pPlayer->Output, "You will now see &RP&Gr&Ye&Bt&Mt&Cy&N &RC&Go&Yl&Bo&Mr&Cs&N.\r\n\r\n");
    Prompt(pPlayer);
  }
  if (Equal(CmdParm1, "off"))
  {
    pPlayer->Color = 'N';
    strcat(pPlayer->Output, "Color is off.\r\n\r\n");
    Prompt(pPlayer);
  }
  CopyPlayerListToPlayer();
  WritePlayerToFile();
}

void DoHelp()
{
  DEBUGIT(1)
  sprintf(HelpFileName,"%s%s%s%s%s",YAGS_DIR,"/",LIB_DIR,"/",HELP_FILE);
  HelpFile = fopen(HelpFileName, "r");
  if (HelpFile == NULL)
  {
    sprintf(LogMsg, "ERROR: Open %s failed: %s", HELP_FILE, strerror(errno));
    AbortIt();
  }
  LowerCase(Command);
  Word(2, Command, CmdParm1);
  Up1stChar(CmdParm1);
  strcpy(TmpStr, "Help:");
  strcat(TmpStr, CmdParm1);
  strcpy(CmdParm1, TmpStr);
  Found = false;
  for (;;)
  {
    fgets(Buffer, sizeof(Buffer), HelpFile);
    Trim(Buffer);
    if (ferror(HelpFile))
    {
      sprintf(LogMsg, "ERROR: Read %s failed: %s", HELP_FILE, strerror(errno));
      AbortIt();
    }
    if (feof(HelpFile))
    {
      break;
    }
    // Just 'Help' was entered
    if (Words(Command) == 1)
    {
      if (Equal(Buffer, "Help:"))
      {
        strcpy(Buffer, "\r\n");
      }
      strcat(pPlayer->Output, Buffer);
      strcat(pPlayer->Output,"\r\n");
      if (Equal(Buffer, "Related help: 'Help Help' Newbie NPC Object Room"))
      {
        Found = true;
        break;
      }
      continue;
    }
    // Help 'something' was entered
    if (!Found)
    {
      if (!Equal(Buffer, CmdParm1))
      {
        continue;
      }
      Found = true;
      continue;
    }
    strcat(pPlayer->Output, Buffer);
    strcat(pPlayer->Output, "\r\n");
    if (strstr(Buffer, "Related help:"))
    {
      break;
    }
  }
  if (!Found)
  {
    strcat(pPlayer->Output, "Help topic not found\r\n");
  }
  fclose(HelpFile);
  strcat(pPlayer->Output, "\r\n");
  Prompt(pPlayer);
}

void DoPlayerfile()
{
  DEBUGIT(1)
  strcat(pPlayer->Output, "\r\n");
  strcat(pPlayer->Output, "&C");
  strcat(pPlayer->Output, "Player file listing\r\n");
  strcat(pPlayer->Output, "&N");
  strcat(pPlayer->Output, "-------------------\r\n");
  strcat(pPlayer->Output, "Name       Admin Color Level Experience\r\n");
  EndFile = false;
  PlayerNbr = 1;
  ReadPlayerFromFile();
  while (EndFile == false)
  {
    sprintf(Buffer, "%-10s %1s %c %3s %c %4s %2i %8s %i", Player.Name, " ", Player.Admin, " ", Player.Color, " ", Player.Level, " ", Player.Experience);
    strcat(pPlayer->Output, Buffer);
    strcat(pPlayer->Output, "\r\n");
    PlayerNbr++;
    ReadPlayerFromFile();
  }
  strcat(pPlayer->Output, "\r\n");
  Prompt(pPlayer);
}

void DoQuit()
{
  DEBUGIT(1)
  CopyPlayerListToPlayer();
  WritePlayerToFile();
  strcat(pPlayer->Output, "Bye Bye");
  strcat(pPlayer->Output, "\r\n");
  pPlayer->State = Disconnect;
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

void Prompt(struct PlayerList *pPlayer)
{
  DEBUGIT(1)
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
    Prompt(pPlayer);
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
  {
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
      SendMotd();
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
    {
      GetNextPlayerNbr();
      InitalizeNewPlayer();
      AddPlayerToFile();
      CopyPlayerToPlayerList();
      SendMotd();
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
  sprintf(GreetingFileName,"%s%s%s%s%s",YAGS_DIR,"/",LIB_DIR,"/",GREETING_FILE);
  GreetingFile = fopen(GreetingFileName, "r");
  if (GreetingFile == NULL)
  {
    sprintf(LogMsg, "ERROR: Open %s failed: %s", GREETING_FILE, strerror(errno));
    sprintf(LogMsg, "ERROR: Full path name is: %s", GreetingFileName);
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
}

void SendMotd()
{
  DEBUGIT(1)
  sprintf(MotdFileName, "%s%s%s%s%s", YAGS_DIR, "/", LIB_DIR, "/", MOTD_FILE);
  MotdFile = fopen(MotdFileName, "r");
  if (MotdFile == NULL)
  {
    sprintf(LogMsg, "ERROR: Open %s failed: %s", MOTD_FILE, strerror(errno));
    AbortIt();
  }
  for (;;)
  {
    fgets(Buffer, sizeof(Buffer), MotdFile);
    if (ferror(MotdFile))
    {
      sprintf(LogMsg, "ERROR: Read %s failed: %s", MOTD_FILE, strerror(errno));
      AbortIt();
    }
    if (feof(MotdFile))
    {
      break;
    }
    strcat(pPlayer->Output, Buffer);
  }
  fclose(MotdFile);
  strcat(pPlayer->Output, "\r\n");
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Log
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

void OpenLog()
{ // Do not add DEBUGIT
  sprintf(LogFileName,"%s%s%s%s%s",YAGS_DIR,"/",LOG_DIR,"/",LOG_FILE);
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
      if (strlen(pPlayer->Input) > 0)
      {
        pPlayer->NoInputTick = 0;
        pPlayer->NoInputCount = 0;
      }
      else
      {
        pPlayer->NoInputTick++;
      }
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
    if (pPlayer->NoInputTick > NO_INPUT_TICK)
    {
      pPlayer->NoInputTick = 0;
      pPlayer->NoInputCount++;
      if (pPlayer->NoInputCount > NO_INPUT_COUNT_LIMIT)
      {
        pPlayer->State = Disconnect;
        strcat(pPlayer->Output, "You are being disconnected. Bye Bye.\r\n\r\n");
        Prompt(pPlayer);
      }
      else
      {
        strcat(pPlayer->Output, "Are you still there?\r\n\r\n");
        Prompt(pPlayer);
      }
    }
    if (pPlayer->Output[0] == '\0')
    {
      pPlayerCurr = pPlayerCurr->pPlayerNext;
      continue;
    }
    Color();
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
  DisconnectPlayers();
}

void Color()
{
  DEBUGIT(2)
  char *Str1; // Points to pPlayer->Output
  char *Str2; // Points to TmpStr
  char *Str3; // Points to 'next char' in Str1 (pPlayer->Output)
  char *Str4; // Points to the selected color code string
  if (strchr(pPlayer->Output, '&') == NULL) return;
  Str1 = &pPlayer->Output[0];
  Str2 = &TmpStr[0];
  while (*Str1)
  { // Loop here until we hit an '&'
    while (*Str1 != '&')
    {
      *Str2 = *Str1;
      if (*Str1 == '\0')
      { // We are done
        *Str2 = '\0';
        strcpy(pPlayer->Output, TmpStr);
        return;
      }
      Str1++;
      Str2++;
    }
    // We hit an '&'
    Str3 = Str1;
    Str3++;
    switch (*Str3)
    {
      case 'N':
        Str4 = Normal;
        break;
      case 'K':
        Str4 = BrightBlack;
        break;
      case 'R':
        Str4 = BrightRed;
        break;
      case 'G':
        Str4 = BrightGreen;
        break;
      case 'Y':
        Str4 = BrightYellow;
        break;
      case 'B':
        Str4 = BrightBlue;
        break;
      case 'M':
        Str4 = BrightMagenta;
        break;
      case 'C':
        Str4 = BrightCyan;
        break;
      case 'W':
        Str4 = BrightWhite;
        break;
    }
    if (pPlayer->Color == 'N')
    {
      Str4 = None;
    }
    while (*Str4 != '\0')
    { // Copy the color code string
      *Str2 = *Str4;
      Str2++;
      Str4++;
    }
    Str1++;
    Str1++;
  }
}

void DisconnectPlayers()
{
  DEBUGIT(2)
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
  GameShutDown = false;
  NoPlayers    = true;
  pPlayerHead  = NULL;
  pPlayerTail  = NULL;
  pPlayerCurr  = NULL;
}

void OpenFiles()
{
  DEBUGIT(1)
  sprintf(PlayerFileName,"%s%s%s%s%s",YAGS_DIR,"/",WORLD_DIR,"/",PLAYER_FILE);
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

bool Equal(char *Str1, char *Str2)
{
  DEBUGIT(2)
  return (!strcmp(Str1, Str2));
}

void LowerCase(char *Str)
{
  DEBUGIT(2)
  for(i = 0; Str[i]; i++)
  {
    Str[i] = (char) tolower(Str[i]);
  }
}

void Trim(char *Str)
{
  DEBUGIT(2)
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

void Up1stChar(char *Str)
{
  DEBUGIT(2)
  Str[0] = (char)toupper(Str[0]);
}

void Word(size_t Nbr, char *Str1, char *Str2)
{
  DEBUGIT(2)
  j = 0;
  x = 1;
  for (i = 0; Str1[i]; i++)
  {
    if (x == Nbr)
    {
      break;
    }
    if (isspace(Str1[i]))
    {
      x++;
    }
  }
  while (!isspace(Str1[i]))
  {
    if (Str1[i] == '\0')
    {
      break;
    }
    Str2[j] = Str1[i];
    i++;
    j++;
  }
  Str2[j] = '\0';
}

size_t Words(char *Str)
{
  DEBUGIT(2)
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
  i = 0;
  while (CommandTable[i][0] != NULL)
  {
    if (Equal(MudCmd, (char *)CommandTable[i][0]))
    {
      CommandNbr = (int)i;
      Commands.Name     = (char *)CommandTable[i][0];
      Commands.Admin    = (char *)CommandTable[i][1];
      Commands.Level    = (char *)CommandTable[i][2];
      Commands.Position = (char *)CommandTable[i][3];
      Commands.Social   = (char *)CommandTable[i][4];
      Commands.Fight    = (char *)CommandTable[i][5];
      Commands.Words    = (char *)CommandTable[i][6];
      Commands.Parts    = (char *)CommandTable[i][7];
      Commands.Message  = (char *)CommandTable[i][8];
      // Admin command?
      if (Equal(Commands.Admin, "Y"))
      {
        if (pPlayer->Admin == 'N')
        {
          break;
        }
      }
      // Check minimum words
      if (Words(Command) < atoi(Commands.Words))
      {
        strcat(pPlayer->Output, Commands.Message);
        strcat(pPlayer->Output, "\r\n\r\n");
        Prompt(pPlayer);
        return false;
      }
      // Command is OK!
      return true;
    }
    i++;
  }
  // Command is none of the above
  strcat(pPlayer->Output, "Huh?\r\n\r\n");
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
  {                                         // If the YaGs development environment is Windows 10,
    #include <sys/select.h>                 //   Visual Studio, and WSL (Windows Subsystem for Linux)
    struct timeval TimeOut;                 //   Ubuntu, then for some strange reason, usleep() does not
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
  pPlayer = (struct PlayerList *)malloc(sizeof(struct PlayerList));
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
  pPlayer->Socket       = Socket;
  pPlayer->State        = Send_Greeting;
  pPlayer->Output[0]    = '\0';
  pPlayer->NoInputTick  = 0;
  pPlayer->NoInputCount = 0;
  pPlayer->pPlayerNext = NULL;
}

void DelFromPlayerList()
{
  DEBUGIT(1)
  // Delete when only one node in list
  if (pPlayerCurr == pPlayerHead)
  {
    if (pPlayerCurr == pPlayerTail)
    { // We have no players
      pPlayerHead = NULL;
      pPlayerTail = NULL;
      NoPlayers   = true;
    }
  }
  else
  // Delete head node
  if (pPlayerCurr == pPlayerHead)
  {
    pPlayerHead = pPlayerCurr->pPlayerNext;
    pPlayerCurr->pPlayerNext->pPlayerPrev = NULL;
  }
  else
  // Delete tail node
  if (pPlayerCurr == pPlayerTail)
  {
    pPlayerTail = pPlayerCurr->pPlayerPrev;
    pPlayerCurr->pPlayerPrev->pPlayerNext = NULL;
  }
  else
  {
    // Delete middle node
    pPlayerCurr->pPlayerPrev->pPlayerNext = pPlayerCurr->pPlayerNext;
    pPlayerCurr->pPlayerNext->pPlayerPrev = pPlayerCurr->pPlayerPrev;
  }
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
  sprintf(ValidNamesFileName,"%s%s%s%s%s",YAGS_DIR,"/",LIB_DIR,"/",VALID_NAMES_FILE);
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
  PlayerNbr = pPlayer->PlayerNbr;
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
  if (ftell(PlayerFile) == 0)
  { // Player file has no records, so this MUST be an Admin!
    pPlayer->Admin = 'Y';
    Player.Admin   = 'Y';
  };
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
  pPlayer->Color          = Player.Color;
  pPlayer->Experience     = Player.Experience;
  pPlayer->Level          = Player.Level;
  pPlayer->Sex            = Player.Sex;
  pPlayer->PlayerNbr      = PlayerNbr;
  pPlayer->BadPswdCount   = 0;
  pPlayer->NoInputTick    = 0;
  pPlayer->NoInputCount   = 0;
}

void CopyPlayerListToPlayer()
{
  DEBUGIT(1)
  strcpy(Player.Name,     pPlayer->Name);
  strcpy(Player.Password, pPlayer->Password);
  Player.Admin          = pPlayer->Admin;
  Player.Afk            = pPlayer->Afk;
  Player.Born           = pPlayer->Born;
  Player.Color          = pPlayer->Color;
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
  Player.Color          = 'N';
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
