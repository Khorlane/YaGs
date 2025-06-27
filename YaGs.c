//***************************
//* Yet another Game server *
//***************************

#define VERSION "Version 1.0.5"                       // Version number
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
#include <math.h>                                     // fmod()
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
#define YAGS_DIR              "/mnt/c/Projects/YaGs"  // YaGs top level directory path
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
#define MOBILES_FILE          "Mobiles.txt"           // Mobiles file
#define OBJECTS_FILE          "Objects.txt"           // Objects file
#define ROOMS_FILE            "Rooms.txt"             // Rooms file
#define PLAYER_FILE           "Player.yags"           // Player file
#define PLAYER_START_ROOM     120                     // Player start room
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
char                *CmdParm3 = aTmpStr3;             // Command Parameter 3
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
fd_set             InpSet;                            // File Descriptor Set structure
struct linger      Linger;                            // Linger structure
struct sockaddr_in SocketAddr;                        // Socket Address structure
struct timeval     TimeOut;                           // Time value structure

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
char               *GameStartMsg = "YaGs is starting";                    // Game starting message
char               *GameStopMsg  = "YaGs has shutdown";                   // Game stop message
char               *GameWakeMsg  = "Waking up";                           // Game wake up message

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Player
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

//PlayerStates is an enumeration that defines various states in a player interaction process,
// such as sending greetings, waiting for player input, and managing online connectivity.
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

// The PlayerList struct represents a data structure for managing a list of connected players,
// containing various attributes such as socket information, player state, name, password,
// experience, and pointers to the next and previous players in the list.
struct PlayerList
{
  int                Socket;                          // Socket number returned from accept()
  PlayerState        State;                           // Player state
  char               Name[50];                        // Player name
  char               Password[50];                    // Player password
  char               Afk;                             // Away from keyboard flag (Y/N)
  char               Admin;                           // Admin flag (Y/N) - Controls which commands are available to the player
  time_t             Born;                            // Time player was created
  char               Color;                           // Color code (Y/N) Y means that player output is run through the Color() function
  int                Experience;                      // Experience points
  char               Level;                           // Player level
  char               Sex;                             // Player sex (M/F)
  int                RoomNbr;                         // Room number
  char               Input[1024];                     // Player input buffer
  char               Output[2048];                    // Player output buffer
  int                BadPswdCount;                    // Number of bad passwords entered
  int                PlayerNbr;                       // Player number
  int                NoInputTick;                     // Ticks before checking if player is still there
  int                NoInputCount;                    // Number of no input ticks
  struct PlayerList *pPlayerNext;                     // Pointer to next player in the player list
  struct PlayerList *pPlayerPrev;                     // Pointer to previous player in the player list
};

// The Player structure represents a player in a game, encapsulating attributes such as
// name, password, status flags, creation time, color preference, experience points, level, and sex.
struct sPlayer
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
  int             RoomNbr;                            // Room number
} Player;

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Functions
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

void    AbortIt();
void    AcceptNewPlayer();
void    AddPlayerToFile();
void    AddToPlayerList();
void    CheckForNewPlayers();
void    CloseLog();
void    ClosePlayerFile();
void    Color();
void    CopyPlayerListToPlayer();
void    CopyPlayerToPlayerList();
void    DelFromPlayerList();
void    DisconnectPlayers();
void    DoAdvance();
void    DoColor();
void    DoHelp();
void    DoLook();
void    DoPlayed();
void    DoPlayerfile();
void    DoQuit();
void    DoShutdown();
void    DoWho();
void    DoStatus();
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
void    OpenLog();
void    OpenPlayerFile();
bool    PlayerNameValid();
bool    PlayerNameValidNew();
bool    PlayerNameValidOld();
void    ProcessCommand();
void    ProcessPlayerInput();
void    Prompt(struct PlayerList *pPlayer);
void    ReadPlayerFromFile();
void    SendGreeting();
void    SendMotd();
void    SendPlayerOutput();
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

// Commands is a structure that holds various character pointers representing different attributes
// related to commands, such as name, admin status, level, position, social interactions,
// fight commands, words, parts, and messages.
struct sCommands
{
  char *Name;                                         // Command name
  char *Admin;                                        // This is an admin command
  char *Level;                                        // Player must be at this level to use the command
  char *Position;                                     // Player must be, at least, in this position to use the command
  char *Social;                                       // Is this a social command
  char *Fight;                                        // Can this command be issued during a fight
  char *MinWords;                                     // Minimum number of words in the command
  char *MaxWords;                                     // Maximum number of words in the command
  char *Message;                                      // Message to display if command is invalid
} Commands;

// CommandTable is a two - dimensional array of character pointers that stores command information,
// including command names, admin levels, positions, social interactions, fight options, word counts,
// and associated messages.
char *CommandTable[][9] = {
  //                                                   MIN  MAX
  // Name          Admin Level Position  Social Fight Words Words Message
    {"advance",    "Y",  "1",  "sleep",  "N",   "N",  "3",  "3",  "Advance who and to what level?"} ,
    {"color",      "N",  "1",  "sleep",  "N",   "N",  "1",  "2",  "None"},
    {"help",       "N",  "1",  "sleep",  "N",   "N",  "1",  "2",  "None"},
    {"look",       "N",  "1",  "sit",    "N",   "N",  "1",  "1",  "None"},
    {"played",     "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"playerfile", "Y",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"quit",       "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"shutdown",   "Y",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"status",     "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"who",        "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {NULL,         NULL, NULL, NULL,     NULL,  NULL, NULL, NULL, NULL}
};

// DoCommand is an array of function pointers, each pointing to a function that takes no parameters
// and returns void, allowing for the execution of various commands such as DoAdvance, DoColor, and others.
void (*DoCommand[])(void) =
{ // This list and the CommandTable MUST BE in the same order
  DoAdvance,
  DoColor,
  DoHelp,
  DoLook,
  DoPlayed,
  DoPlayerfile,
  DoQuit,
  DoShutdown,
  DoStatus,
  DoWho
};

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Main
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// The main function initializes the game server, enters a loop to handle player interactions,
// checks for new players, processes player input, sends output to players, and handles game shutdown.
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

// The HeartBeat function handles the heartbeat of the game server,
// which can be used for periodic tasks or checks.
void HeartBeat()
{
  DEBUGIT(2)
}

// The ProcessPlayerInput function processes input from all players in a linked list, executing
// commands based on the input received and clearing the input buffer after processing.
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

// The ProcessCommand function processes a command by trimming and logging it,
// checking the player's online status, and executing the command if it is valid.
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

// The MudCmdOk function checks if a given command is valid by comparing it against a command table,
// verifying user permissions, and ensuring the command meets the required word count, returning true
// if the command is valid and false otherwise.
bool MudCmdOk()
{
  DEBUGIT(1)
  i = 0;
  while (CommandTable[i][0] != NULL)
  {
    if (Equal(MudCmd, (char*)CommandTable[i][0]))
    {
      CommandNbr        = (int)i;
      Commands.Name     = (char*)CommandTable[i][0];
      Commands.Admin    = (char*)CommandTable[i][1];
      Commands.Level    = (char*)CommandTable[i][2];
      Commands.Position = (char*)CommandTable[i][3];
      Commands.Social   = (char*)CommandTable[i][4];
      Commands.Fight    = (char*)CommandTable[i][5];
      Commands.MinWords = (char*)CommandTable[i][6];
      Commands.MaxWords = (char*)CommandTable[i][7];
      Commands.Message  = (char*)CommandTable[i][8];
      // Admin command?
      if (Equal(Commands.Admin, "Y"))
      {
        if (pPlayer->Admin == 'N')
        {
          break;
        }
      }
      // Check minimum/maximum words
      if (Words(Command) < atoi(Commands.MinWords) || Words(Command) > atoi(Commands.MaxWords))
      {
        if (Equal(Commands.Message, "None"))
        {
          sprintf(Buffer, "%s %s %s %s %s %s %s %s", "Too many or too few words in command,", "Min:", Commands.MinWords, "Max:", Commands.MaxWords, "Refer to help", Commands.Name, "\r\n");
          strcat(pPlayer->Output, Buffer);
        }
        else
        {
          strcat(pPlayer->Output, Commands.Message);
        }
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

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// All Do functions
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// The DoAdvance function advances a player's level in a game, updating their experience and notifying
// both the target player and the acting player of the change, while also handling various conditions
// such as invalid levels and ensuring the target player is online.
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

// The DoColor function manages the player's color settings in a game, allowing them to toggle
// color output on or off and providing appropriate feedback based on their current setting.
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

// The DoHelp function retrieves and displays help information from a specified help file based on
// user input, handling errors and formatting the output for the player.
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

// The DoLook function displays the current room and its contents to the player.
void DoLook()
{
  DEBUGIT(1)
  sprintf(Buffer, "You are in room %d, looking around, you see nothing special.\r\n", pPlayer->RoomNbr);
  strcat(pPlayer->Output, Buffer);
  Prompt(pPlayer);
}

// The DoPlayed function calculates the elapsed time since a player's birth in days, hours, minutes,
// and seconds, formats this information into a string, and appends it to the player's output.
void DoPlayed()
{
  DEBUGIT(1)
  time_t CurrentTime;
  double ElapsedTime;
  int Days, Hours, Minutes, Seconds;

  CurrentTime = time(NULL); // Get the current time
  ElapsedTime = difftime(CurrentTime, pPlayer->Born); // Calculate the difference in seconds
  // Calculate days, hours, minutes, and seconds
  Days        = (int)(ElapsedTime / (24 * 3600));
  ElapsedTime = fmod(ElapsedTime, (24 * 3600));
  Hours       = (int)(ElapsedTime / 3600);
  ElapsedTime = fmod(ElapsedTime, 3600);
  Minutes     = (int)(ElapsedTime / 60);
  Seconds     = (int)fmod(ElapsedTime, 60);
  sprintf(Buffer, "Your age is : %d days, %d hours, %d minutes, %d seconds\n", Days, Hours, Minutes, Seconds);
  strcat(pPlayer->Output, Buffer);
  strcat(pPlayer->Output, "\r\n");
  Prompt(pPlayer);
}

// The DoPlayerfile function generates a formatted player file listing by reading player data from
// the player file and appending it to the output buffer of a player structure.
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

// The DoQuit function handles the process of disconnecting a player by copying the player list, writing
// player data to a file, updating the output message, and changing the player's state to "Disconnect."
void DoQuit()
{
  DEBUGIT(1)
  CopyPlayerListToPlayer();
  WritePlayerToFile();
  strcat(pPlayer->Output, "Bye Bye");
  strcat(pPlayer->Output, "\r\n");
  pPlayer->State = Disconnect;
}

// The DoShutdown function initiates the shutdown process of the game by setting a shutdown flag,
// displaying a shutdown message, and notifying all connected users.
void DoShutdown()
{
  DEBUGIT(1)
  GameShutDown = true;
  strcpy(MsgTxt, "\r\nYaGs is shutting down!\r\n");
  SendToAll();
}

void DoStatus()
{
  DEBUGIT(1)
  strcat(pPlayer->Output, "\r\n");
  // Name
  sprintf(Buffer, "Name: %s\r\n", pPlayer->Name);
  strcat(pPlayer->Output, Buffer);
  // Afk
  sprintf(Buffer, "AFK: %c\r\n", pPlayer->Afk);
  strcat(pPlayer->Output, Buffer);
  // Born
  char *TimeStr = ctime(&pPlayer->Born); // Convert the birth time to a date string
  TimeStr[strlen(TimeStr) - 1] = '\0';   //   and remove the newline character
  sprintf(Buffer, "Born: %s\r\n", TimeStr);
  strcat(pPlayer->Output, Buffer);
  // Color
  sprintf(Buffer, "Color: %c\r\n", pPlayer->Color);
  strcat(pPlayer->Output, Buffer);
  // Experience
  sprintf(Buffer, "Experience: %i\r\n", pPlayer->Experience);
  strcat(pPlayer->Output, Buffer);
  // Level
  sprintf(Buffer, "Level: %i\r\n", pPlayer->Level);
  strcat(pPlayer->Output, Buffer);
  //  Sex
  sprintf(Buffer, "Sex: %c\r\n", pPlayer->Sex);
  strcat(pPlayer->Output, Buffer);
  // Admin
  if (pPlayer->Admin == 'Y')
  {
    strcat(pPlayer->Output, "Your are an Admin!\r\n");
  }

  strcat(pPlayer->Output, "\r\n");
  Prompt(pPlayer);
}

// The DoWho function outputs a formatted list of online players, including their names and levels, to the player's output buffer.
void DoWho()
{
  DEBUGIT(1)
  strcat(pPlayer->Output, "\r\n");
  strcat(pPlayer->Output, "&C");
  strcat(pPlayer->Output, "Players online\r\n");
  strcat(pPlayer->Output, "&N");
  strcat(pPlayer->Output, "-------------------\r\n");
  strcat(pPlayer->Output, "Name       Level \r\n");
  pPlayerCurrSave = pPlayerCurr;
  pPlayerCurr     = pPlayerHead;
  while (pPlayerCurr != NULL)
  {
    sprintf(Buffer, "%-10s %2s %2i", pPlayerCurr->Name, " ", pPlayerCurr->Level);
    strcat(pPlayer->Output, Buffer);
    strcat(pPlayer->Output, "\r\n");
    pPlayerCurr = pPlayerCurr->pPlayerNext;
  }
  strcat(pPlayer->Output, "\r\n");
  Prompt(pPlayer);
  pPlayerCurr = pPlayerCurrSave;
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// General player communication
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// The Prompt function appends a prompt string ("> ") to the Output member of a PlayerList structure, which is passed as a pointer.
void Prompt(struct PlayerList *pPlayer)
{
  DEBUGIT(1)
  strcat(pPlayer->Output,"> ");
}

// The SendToAll function sends a message to all players in a linked list, ensuring that the
// original player context is preserved during the operation.
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

// The GetPlayerOnline function manages the online status and interactions of players in a game, handling
// various states such as greeting new players, validating names and passwords, and prompting for input
// based on the player's current state.
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

// The SendGreeting function reads a greeting message from a specified file and appends its contents
// to a player's output, handling errors related to file operations.
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

// The SendMotd function reads a message of the day (MOTD) from a specified file and appends its
// contents to a player's output buffer, handling errors related to file operations.
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

// The OpenLog function initializes a log file by constructing its name from predefined directory and
// file constants, attempts to open it for writing, and handles any errors that occur during this process.
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
  LogIt(VERSION);
}

// The LogIt function logs a message to a specified log file, prepending the current time to the
// message for timestamping.
void LogIt(char *LogMsg)
{
  GetTime();
  fprintf(LogFile, "%s - %s\r\n", CurrentTime, LogMsg);
  fflush(LogFile);
}

// The CloseLog function is responsible for closing the log file and logging a game stop message,
// ensuring proper cleanup of logging resources.
void CloseLog()
{
  DEBUGIT(1)
  LogIt(GameStopMsg);
  fclose(LogFile);
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Sockets
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// The SocketListen function initializes a listening socket for incoming TCP connections, configures
// it with various socket options, binds it to a specified port, and begins listening for connections.
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

// The CheckForNewPlayers function monitors network sockets for new player connections and accepts
// them if available, handling any errors that may occur during the process.
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

// The AcceptNewPlayer function handles the acceptance of a new player connection by accepting a socket
// connection, logging the connection details, and updating the player list and online status.
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

// The GetPlayerInput function processes input from connected players by reading data from their
// respective sockets and updating their input state accordingly.
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

// The SendPlayerOutput function processes and sends output messages to connected players,
// handling disconnection for those who have not provided input within a specified time limit.
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

// The DisconnectPlayers function iterates through a list of players, disconnecting those whose state
// is set to "Disconnect" by closing their sockets and removing them from the player list.
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

// The StartItUp function initializes the system by performing a series of setup tasks,
// including initialization, logging, socket listening, and file opening.
void StartItUp()
{ // Do not add DEBUGIT
  Initialization();
  OpenLog();
  SocketListen();
  OpenPlayerFile();
}

// The Initialization function sets up the initial state of a game by resetting various
// player-related variables and ensuring that the game is not shut down.
void Initialization()
{ // Do not add DEBUGIT
  GameShutDown = false;
  NoPlayers    = true;
  pPlayerHead  = NULL;
  pPlayerTail  = NULL;
  pPlayerCurr  = NULL;
}

// The ShutItDown function is responsible for gracefully shutting down the game by closing files and logs.
void ShutItDown()
{
  DEBUGIT(1)
  ClosePlayerFile();
  CloseLog();
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Player list
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// The AddToPlayerList function allocates memory for a new player node, initializes its properties,
// and adds it to a linked list of players, handling both the first node and subsequent nodes appropriately.
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

// The DelFromPlayerList function removes the current player node from a doubly linked list of players,
// handling cases for deleting the head, tail, or a middle node, and updates the list pointers accordingly.
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

// The CopyPlayerToPlayerList function copies the attributes of a player from a source player object
// to a destination player object, initializing various fields such as name, password, admin status, and experience level.
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
  pPlayer->RoomNbr        = Player.RoomNbr;
  pPlayer->PlayerNbr      = PlayerNbr;
  pPlayer->BadPswdCount   = 0;
  pPlayer->NoInputTick    = 0;
  pPlayer->NoInputCount   = 0;
}

// The CopyPlayerListToPlayer function copies the attributes of a player from a source player structure (pPlayer) to a target player structure (Player).
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
  Player.RoomNbr        = pPlayer->RoomNbr;
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Player file
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// The OpenPlayerFile function attempts to open a player file for reading and writing, constructing
// the file path from predefined directory and file name constants, and handles errors by
// logging a message and aborting the operation if the file cannot be opened.
void OpenPlayerFile()
{
  DEBUGIT(1)
  sprintf(PlayerFileName, "%s%s%s%s%s", YAGS_DIR, "/", WORLD_DIR, "/", PLAYER_FILE);
  PlayerFile = fopen(PlayerFileName, "r+");
  if (PlayerFile == NULL)
  {
    sprintf(LogMsg, "Error opening %s : %s", PlayerFileName, strerror(errno));
    AbortIt();
  }
}

// The ClosePlayerFile function is responsible for closing the PlayerFile file stream, ensuring that
// any resources associated with it are properly released.
void ClosePlayerFile()
{
  DEBUGIT(1)
  fclose(PlayerFile);
}

// The GetPlayerFileOffset function calculates and returns the file offset for a player based on the
// size of the Player structure and current value of PlayerNbr, effectively determining the byte position
// in a file where a specific player's data would be stored.
long GetPlayerFileOffset()
{
  DEBUGIT(1)
  x = (size_t)sizeof(Player);
  y = (size_t)PlayerNbr - 1;
  Offset = (long)(x * y);
  return Offset;
}

// The PlayerNameValid function checks the validity of a player's name based on the current state of
// the player, returning true if valid or triggering an error if an unexpected state is encountered.
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

// The PlayerNameValidOld function checks if a player's name matches a specified command by reading
// player data from a file until a match is found or the end of the file is reached, returning
// a boolean value indicating whether a match was found.
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

// The PlayerNameValidNew function checks if a given player name exists in a predefined list of
// valid names stored in a file, returning true if a match is found and false otherwise.
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

// The ReadPlayerFromFile function reads player data from a file, handling potential errors
// and end-of-file conditions during the read operation.
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

// The WritePlayerToFile function writes the current player's data to a file,
// ensuring proper file positioning and error handling during the write operation.
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

// The AddPlayerToFile function adds a player record to a file,
// initializing the player as an admin if the file is empty,
// and includes error handling for file operations.
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

// The InitalizeNewPlayer function initializes a new player by copying their name and password from
// a provided player structure, setting default values for various player attributes, and recording
// the current time as their birth timestamp.
void InitalizeNewPlayer()
{
  DEBUGIT(1)
  strcpy(Player.Name,     pPlayer->Name);
  strcpy(Player.Password, pPlayer->Password);
  Player.Admin          = 'N';
  Player.Afk            = 'N';
  Player.Born           = time(NULL);
  Player.Color          = 'N';
  Player.Experience     = 0;
  Player.Level          = 1;
  Player.Sex            = pPlayer->Sex;
  Player.RoomNbr        = PLAYER_START_ROOM;
}

// The GetNextPlayerNbr function increments the PlayerNbr variable starting from 1 and continues
// to read player data from a file until the end of the file is reached.
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

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Strings
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// The Equal function compares two C-style strings for equality and returns true if they are identical and false otherwise.
bool Equal(char *Str1, char *Str2)
{
  DEBUGIT(2)
  return (!strcmp(Str1, Str2));
}

// The LowerCase function converts all characters in the input C-style string Str to their lowercase equivalents.
void LowerCase(char *Str)
{
  DEBUGIT(2)
  for (i = 0; Str[i]; i++)
  {
    Str[i] = (char)tolower(Str[i]);
  }
}

// The Trim function removes leading and trailing whitespace characters from a given C-style string.
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
      Str[i] = Str[i + 1];
    }
    Str[i] = '\0';
  }
}

// The Up1stChar function takes a pointer to a character string and converts the first character of the string to uppercase.
void Up1stChar(char *Str)
{
  DEBUGIT(2)
  Str[0] = (char)toupper(Str[0]);
}

// The Word function extracts the N-th word from the input string Str1 and copies it into the output string Str2,
// where Nbr specifies the word position to extract.
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

// The Words function counts and returns the number of words in a given null-terminated string,
// where words are defined as sequences of non-space characters separated by whitespace.
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

// The AbortIt function logs a message, closes the log, and then terminates the program with an exit status of 1.
void AbortIt()
{ // Do not add DEBUGIT
  LogIt(LogMsg);
  CloseLog();
  exit(1);
}

// The Color function processes a player's output string to replace color codes indicated
// by '&' with corresponding ANSI color codes, modifying the output accordingly.
void Color()
{
  DEBUGIT(2)
  char* Str1; // Points to pPlayer->Output
  char* Str2; // Points to TmpStr
  char* Str3; // Points to 'next char' in Str1 (pPlayer->Output)
  char* Str4; // Points to the selected color code string
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

// The GetTime function retrieves the current time in a human-readable format, converting
// it from seconds since the Unix epoch and removing the trailing newline character.
void GetTime()
{ // Do not add DEBUGIT
  CurrentTimeSec = time(NULL);              // Seconds since Epoch, 1970-01-01 00:00:00 +0000 (UTC)
  CurrentTime = ctime(&CurrentTimeSec);     // Convert to human readable
  x = strlen(CurrentTime);                  // Get rid of the '\n'
  CurrentTime[x - 1] = '\0';                  //   at the end of string returned by ctime()
}

// The Sleep function is designed to pause the execution of a program for a specified duration,
// utilizing either usleep or select based on the environment configuration to manage CPU usage effectively.
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