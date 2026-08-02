//***************************
//* Yet another Game server *
//***************************

#define VERSION "Version 1.0.5"                          // Version number
#define _DEFAULT_SOURCE                                  // Required for a bunch of BSD socket stuff
#pragma GCC diagnostic push                              // Ignore warnings about sections
#pragma GCC diagnostic ignored "-Wunreachable-code"      //   of unreachable code.

#ifdef __INTELLISENSE__                                  // Visual Studio does not recognize this GCC built-in
static inline void __builtin_free(void *Ptr)             //   while parsing newer glibc headers.
{                                                        //   __builtin_free is in stdlib.h and Intellisense
  (void)Ptr;                                             //   doesn't understand and throws warning:
}                                                        //   Warning VCR001  Function definition for '__builtin_free' not found.
#endif                                                   // This block is here to shut Intellisense up

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Includes
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include <arpa/inet.h>                                   // This and sys/socket.h - a whole plethora of socket related stuff
#include <ctype.h>                                       // isspace(), tolower(), toupper()
#include <errno.h>                                       // errno, EINTR
#include <fcntl.h>                                       // fcntl(), F_SETFL, FNDELAY
#include <math.h>                                        // fmod()
#include <stdbool.h>                                     // bool, true, false
#include <stdio.h>                                       // a whole bunch of i/o functions
#include <stdlib.h>                                      // atoi(), exit(), free(), malloc()
#include <string.h>                                      // a whole bunch of string functions
#include <sys/socket.h>                                  // This and arpa/inet - a whole plethora of socket related stuff
#include <time.h>                                        // ctime(), difftime(), time(), time_t
#include <unistd.h>                                      // close(), fsync(), read(), usleep()

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Macros
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Debugging
#define DEBUGIT(dl)             if (DEBUGIT_LVL >= dl) {sprintf(LogMsg,"*** %s ***",__FUNCTION__);LogIt(LogMsg);} // dl = debug level
#define DEBUGIT_LVL             1                        // Range of 0 to 5 where 0 = No debug messages and 5 = Maximum debug messages
// Configuration
#define BUFFER_LIMIT            2048                     // Max size of Buffer including '\0'
#define PORT                    3737                     // Port number
#define SLEEP_TIME              0400000                  // Sleep for a short period of time
#define STRING_LIMIT            1024                     // Max size of string including '\0'
#define USE_USLEEP              'N'                      // Use usleep() Y or N
// Directories
#define YAGS_DIR                "/mnt/c/Projects/YaGs"   // YaGs top level directory path
#define LIB_DIR                 "Library"                // Library directory
#define WORLD_DIR               "World"                  // World directory
#define LOG_DIR                 "Logs"                   // Log directory
// Library directory contents
#define GREETING_FILE           "Greeting.txt"           // Greeting file
#define HELP_FILE               "Help.txt"               // Help file
#define MOTD_FILE               "Motd.txt"               // Message of the day file
#define VALID_NAMES_FILE        "ValidNames.txt"         // Valid names file
// Log directory contents
#define LOG_FILE                "Log.txt"                // Log file
// World directory contents
#define MOBILES_FILE            "Mobiles.txt"            // Mobiles file
#define OBJECTS_FILE            "Objects.txt"            // Objects file
#define ROOMS_FILE              "Rooms.txt"              // Rooms file
#define PLAYER_FILE             "Player.yags"            // Player file
#define PLAYER_START_ROOM       120                      // Player start room
// Timer events
#define NO_INPUT_TICK           500                      // Ticks before checking if player is still there
#define NO_INPUT_COUNT_LIMIT    3                        // Triggers player disconnect after this limit is hit
#define PLAYER_AUTOSAVE_SECONDS 60                       // Seconds between dirty player saves

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Globals
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Booleans
bool                  EndFile;                           // File access - End of file
bool                  Found;                             // File access - Record found
bool                  GameShutDown;                      // Set this to true to stop the game
bool                  NoPlayers;                         // True when we have no players

// Numbers
size_t                BufferLen;                         // Length of the string stored in Buffer
long int              BytesRead;                         // Number of bytes read
size_t                CmdDoCount;                        // Count of function pointers in the DoCommand array
size_t                CmdTableCount;                     // Count of entries in the CommandTable array
int                   CommandNbr;                        // Command number zero based
time_t                CurrentTime;                       // Current time for played calculation
time_t                CurrentTimeSec;                    // Current time in seconds
time_t                NextPlayerAutosave;                // Time of the next dirty player save
int                   Days;                              // Played time in days
int                   DestRoomNbr;                       // Room number player is moving into
int                   DirectionNbr;                      // The DirectionTable index of the direction
double                ElapsedTime;                       // Elapsed player time
int                   Hours;                             // Played time in hours
socklen_t             LingerSize;                        // Size of Linger stucture
int                   LineNbr;                           // Line number
int                   Listen;                            // Listening socket
int                   MaxSocket;                         // Maximum socket value
int                   Minutes;                           // Played time in minutes
long                  Offset;                            // Offset for fseek()
int                   OptVal;                            // Set socket option value
socklen_t             OptValSize;                        // Size of socket option value
int                   PlayerRcdNbr;                      // Player record number within Player.yags
int                   ReturnValue1;                      // Return value
size_t                ReturnValue2;                      // Return value
long int              SendResult;                        // Number of bytes sent to player
int                   Seconds;                           // Played time in seconds
int                   Socket;                            // Socket value
socklen_t             SocketAddrSize;                    // Size of Socket structure
size_t                StrLen;                            // String length
int                   WordState;                         // Tracks WordState: NotWord | InWord
extern int            errno;                             // Error number set by fopen(), for example
size_t                i;                                 // A non-negative integer
size_t                j;                                 // A non-negative integer
size_t                k;                                 // A non-negative integer
size_t                x;                                 // A non-negative integer
size_t                y;                                 // A non-negative integer
size_t                z;                                 // A non-negative integer

//Pointers
char                 *pColor;                            // Selected color code string
char                 *CurrentTimeTxt;                    // Current timestamp text
char                 *pExitsCopy;                        // Working copy of Room Exits
char                 *pOutput;                           // Pointer into Player->Output
char                 *pOutPlus1;                         // Pointer to pOutput + 1
char                 *pTmpStr;                           // Pointer into TmpStr
char                 *pToken;                            // It's a token
struct ConnList      *pActor;                            // Pointer to acting player in the connection list
struct ConnList      *pConn;                             // Pointer to a connection in the connection list - generic usage
struct ConnList      *pConnSave;                         // Pointer to a connection in the connection list - save
struct ConnList      *pConnCurr;                         // Pointer to current connection in the connection list
struct ConnList      *pConnCurrSave;                     // Pointer to current connection in the connection list - save
struct ConnList      *pConnHead;                         // Pointer to the head of connection list
struct ConnList      *pConnTail;                         // Pointer to the tail of connection list
struct ConnList      *pTarget;                           // Pointer to target player in the connection list

// Strings
char                  aTmpStr[STRING_LIMIT];             // Temp string
char                 *TmpStr   = aTmpStr;                // Temp string too
char                  aTmpStr1[STRING_LIMIT];            // Temp string 1
char                 *TmpStr1  = aTmpStr1;               // Temp string 1 too
char                 *CmdParm1 = aTmpStr1;               // Command Parameter 1
char                  aTmpStr2[STRING_LIMIT];            // Temp string 2
char                 *TmpStr2  = aTmpStr2;               // Temp string 2 too
char                 *CmdParm2 = aTmpStr2;               // Command Parameter 2
char                  aTmpStr3[STRING_LIMIT];            // Temp string 3
char                 *TmpStr3  = aTmpStr3;               // Temp string 3 too
char                 *CmdParm3 = aTmpStr3;               // Command Parameter 3
char                  Buffer[BUFFER_LIMIT];              // Just a buffer
char                  Command[STRING_LIMIT];             // The command from the player
char                  LogMsg[100];                       // Log message
char                  MsgTxt[100];                       // Message text
char                  MudCmd[10];                        // Mud command
char                 *Parameters;                        // Command parameters
char                  TheRest[50];                       // The rest of the command
char                 *RoomExits;                         // Formatted room exits

// Files
char                 *GreetingFileName   = aTmpStr;      // Greeting file name
char                 *HelpFileName       = aTmpStr;      // Help file name
char                 *LogFileName        = aTmpStr;      // Log file name
char                 *MotdFileName       = aTmpStr;      // Message of the day file name
char                 *PlayerFileName     = aTmpStr;      // Player file name
char                 *RoomFileName       = aTmpStr;      // Room file name
char                 *ValidNamesFileName = aTmpStr;      // Valid names file name
FILE                 *GreetingFile;                      // Greeting file
FILE                 *HelpFile;                          // Help file
FILE                 *LogFile;                           // Log file
FILE                 *MobileFile;                        // Mobile file
FILE                 *MotdFile;                          // Message of the day file
FILE                 *ObjectFile;                        // Object file
FILE                 *PlayerFile;                        // Player file
FILE                 *RoomFile;                          // Room file
FILE                 *ValidNamesFile;                    // Valid names file

// Structures
fd_set                InpSet;                            // File Descriptor Set structure
struct linger         Linger;                            // Linger structure
struct sockaddr_in    SocketAddr;                        // Socket Address structure
struct timeval        TimeOut;                           // Time value structure

// Color codes
char                 *Normal        = "\x1B[0;m";        // NORMAL     &N
char                 *BrightBlack   = "\x1B[1;30m";      // BBLACK     &K
char                 *BrightRed     = "\x1B[1;31m";      // BRED       &R
char                 *BrightGreen   = "\x1B[1;32m";      // BGREEN     &G
char                 *BrightYellow  = "\x1B[1;33m";      // BYELLOW    &Y
char                 *BrightBlue    = "\x1B[1;34m";      // BBLUE      &B
char                 *BrightMagenta = "\x1B[1;35m";      // BMAGENTA   &M
char                 *BrightCyan    = "\x1B[1;36m";      // BCYAN      &C
char                 *BrightWhite   = "\x1B[1;37m";      // BWHITE     &W
char                 *None          = "";                // No Color

// Messages
char                 *GameSleepMsg = "No Connections: Going to sleep";      // Game sleeping message
char                 *GameStartMsg = "YaGs is starting";                    // Game starting message
char                 *GameStopMsg  = "YaGs has shutdown";                   // Game stop message
char                 *GameWakeMsg  = "Waking up";                           // Game wake up message

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

// Player and World structure typedefs
typedef struct Mobile        Mobile;
typedef struct MobileList    MobileList;
typedef struct Object        Object;
typedef struct ObjectList    ObjectList;
typedef struct PlayerEquList PlayerEquList;
typedef struct PlayerInvList PlayerInvList;
typedef struct ConnList      ConnList;
typedef struct Player        Player;
typedef struct Room          Room;
typedef struct RoomList      RoomList;

// The ConnList struct represents a connected player session, including socket state,
// input and output buffers, player data, and connection list pointers.
struct ConnList
{
  int                 Socket;                         // Socket number returned from accept()
  PlayerState         State;                          // Player state
  char                Input[1024];                    // Player input buffer
  char                Output[2048];                   // Player output buffer
  int                 BadPswdCount;                   // Number of bad passwords entered
  int                 PlayerRcdNbr;                   // Player record number within Player.yags
  int                 NoInputTick;                    // Ticks before checking if player is still there
  int                 NoInputCount;                   // Number of no input ticks
  bool                PlayerDirty;                    // Player record has unsaved changes
  Player             *pPlayer;                        // Pointer to the connected player data
  PlayerEquList      *pPlayerEquHead;                 // Pointer to the head of the player equipment list
  PlayerEquList      *pPlayerEquTail;                 // Pointer to the tail of the player equipment list
  PlayerInvList      *pPlayerInvHead;                 // Pointer to the head of the player inventory list
  PlayerInvList      *pPlayerInvTail;                 // Pointer to the tail of the player inventory list
  ConnList           *pConnNext;                      // Pointer to next connection in the connection list
  ConnList           *pConnPrev;                      // Pointer to previous connection in the connection list
};

// The Player structure represents a player, encapsulating attributes such as
// name, password, status flags, creation time, color preference, experience points, level, and sex.
struct Player
{
  char                Name[50];                       // Player name
  char                Password[50];                   // Player password
  char                Afk;                            // Away from keyboard flag (Y/N)
  char                Admin;                          // Admin flag (Y/N) - Controls which commands are available to the player
  time_t              Born;                           // Time player was created
  char                Color;                          // Color code (Y/N) Y means that player output is run through the Color() function
  int                 Experience;                     // Experience points
  char                Level;                          // Player level
  char                Sex;                            // Player sex (M/F)
  int                 RoomNbr;                        // Room number
};

struct PlayerEquList
{
  Object             *pObject;                        // Pointer to an equipped Object struct
  char               *Slot;                           // Equipment slot occupied by the object
  PlayerEquList      *pNextPlayerEqu;                 // Pointer to the next equipment list node
};

struct PlayerInvList
{
  Object             *pObject;                        // Pointer to an inventory Object struct
  int                 Quantity;                       // Number of identical objects carried
  PlayerInvList      *pNextPlayerInv;                 // Pointer to the next inventory list node
};

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// World
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

struct Mobile
{
  char               *Id;                             // Unique mobile identifier
  char               *Desc1;                          // Short mobile description
  char               *Desc2;                          // Description shown in a room
  char               *Desc3;                          // Detailed mobile description
  char               *Attack;                         // Mobile attack description
  int                 Level;                          // Mobile level
  int                 Hit;                            // Mobile hit value
  int                 Exp;                            // Experience award
  char               *Loot;                           // Space-separated object identifiers
};

struct MobileList
{
  Mobile             *pMobile;                        // Pointer to a Mobile struct
  MobileList         *pNextMobile;                    // Pointer to the next node in the list
};

MobileList            *pMobileListHead = NULL;        // Pointer to the head of the mobile list
MobileList            *pMobileListTail = NULL;        // Pointer to the tail of the mobile list

struct Object
{
  char               *Id;                             // Unique object identifier
  char               *Desc1;                          // Short object description
  char               *Desc2;                          // Description shown in a room
  char               *Desc3;                          // Detailed object description
  int                 Weight;                         // Object weight
  int                 Cost;                           // Object purchase cost
  char               *Type;                           // General object type
  char               *Subtype;                        // Specific object type
  int                 Value;                          // Type-specific object value
};

struct ObjectList
{
  Object             *pObject;                        // Pointer to an Object struct
  ObjectList         *pNextObject;                    // Pointer to the next list node
};

ObjectList            *pObjectListHead = NULL;        // Pointer to the head of the object list
ObjectList            *pObjectListTail = NULL;        // Pointer to the tail of the object list

struct Room
{
  int                 RoomNbr;                        // Room number (e.g., 101)
  char               *Name;                           // Room name (e.g., "Back Porch")
  char               *Desc;                           // Room description (multi-line text)
  char               *Terrain;                        // Terrain type (e.g., "Concrete", "Indoor")
  char               *Flags;                          // Flags (e.g., "None", "NoFight")
  char               *Exits;                          // Exits as a single string (e.g., "xxxxx xxxxx 00106 xxxxx xxxxx")
};

struct RoomList
{
  Room               *pRoom;                          // Pointer to a Room struct
  RoomList           *pNextRoom;                      // Pointer to the next node in the list
};

Player                PlayerRcd;                      // Player record used for player file reads
Room                  SingleRoom;
Room                 *pCurrentRoom;
Room                 *pDestinationRoom;
Room                 *pNewRoom;
Room                 *pRoom;
RoomList             *pNewRoomListNode;
RoomList             *pRoomListCurr = NULL;
RoomList             *pRoomListHead = NULL;
RoomList             *pRoomListNext = NULL;
RoomList             *pRoomListTail = NULL;

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Functions
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

void    AbortIt();
void    AddPlayerToFile();
void    AddToConnList();
void    SocketCheckForNewPlayers();
void    CloseLog();
void    ClosePlayerFile();
void    Color();
void    DelFromConnList();
int     DirectionLookUp(char *Direction);
void    DoAdvance();
void    DoColor();
void    DoEquipment();
void    DoGo();
void    DoHelp();
void    DoInventory();
void    DoKill();
void    DoLook();
void    DoPlayed();
void    DoPlayerfile();
void    DoQuit();
void    DoShutdown();
void    DoWho();
void    DoStatus();
bool    Equal(char *Str1, char *Str2);
void    GetNextPlayerRcdNbr();
long    GetPlayerFileOffset();
void    GetPlayerOnline();
void    GetTime();
void    HeartBeat();
void    InitalizeNewPlayer();
void    Initialization();
void    LogIt(char *LogMsg);
void    LowerCase(char *Str);
void    MobileReadFile();
void    NormalizePlayerName(char *Name);
bool    MudCmdOk();
void    ObjectReadFile();
void    OpenLog();
void    OpenPlayerFile();
bool    PlayerNameValid();
bool    PlayerNameValidNew();
bool    PlayerNameValidOld();
void    PlayerAutoSave();
void    ProcessCommandAlias();
void    ProcessCommand();
void    ProcessPlayerInput();
void    Prompt(ConnList *pConn);
void    ReadPlayerFromFile();
void    RoomAddToRoomList();
Room   *RoomAllocateAndCopy(const Room *SourceRoom);
void    RoomFreeList();
char   *RoomGetExits(const Room *pRoom);
Room   *RoomLookUp(int RoomNbr);
void    RoomReadFile();
void    SendGreeting();
void    SendMotd();
void    SendToAll();
void    ShutItDown();
void    Sleep();
void    SocketAcceptNewPlayer();
void    SocketGetPlayerInput();
void    SocketDisconnectPlayers();
void    SocketListen();
void    SocketSendPlayerOutput();
void    StartItUp();
void    StrAppend(char *Str1, char *Str2);
void    StripTrailingNlCr(char* Buffer);
void    Trim(char *Str);
void    Up1stChar(char *Str);
void    ValidateCommandTable();
void    Word(size_t Nbr, char *Str1, char *Str2);
size_t  Words(char *Str);
void    WritePlayerToFile();
//void    zTestStuff();

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Commands
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Commands is a structure that holds various character pointers representing different attributes
// related to commands, such as name, admin status, level, position, social interactions,
// fight commands, words, parts, and messages.
struct sCommands
{
  char               *Name;                           // Command name
  char               *Admin;                          // This is an admin command
  char               *Level;                          // Player must be at this level to use the command
  char               *Position;                       // Player must be, at least, in this position to use the command
  char               *Social;                         // Is this a social command
  char               *Fight;                          // Can this command be issued during a fight
  char               *MinWords;                       // Minimum number of words in the command
  char               *MaxWords;                       // Maximum number of words in the command
  char               *Message;                        // Message to display if command is invalid
} Commands;

// Command aliases provide abbreviated forms of commands.
struct sCommandAlias
{
  char               *Alias;                          // Command alias
  char               *Command;                        // Full command name
};

struct sCommandAlias CommandAliasTable[] =
{
  {"l",  "look"},
  {"k",  "kill"},
  {"i",  "inventory"},
  {"eq", "equipment"},
  {NULL,  NULL}
};

// Direction aliases and names are ordered to match the exits in Rooms.txt.
struct sDirection
{
  char               *ShortName;                      // Abbreviated direction
  char               *LongName;                       // Full direction name
  char               *DisplayName;                    // Direction shown in room exits
};

struct sDirection DirectionTable[] =
{
  {"n",  "north",     "North"},
  {"ne", "northeast", "NorthEast"},
  {"e",  "east",      "East"},
  {"se", "southeast", "SouthEast"},
  {"s",  "south",     "South"},
  {"sw", "southwest", "SouthWest"},
  {"w",  "west",      "West"},
  {"nw", "northwest", "NorthWest"},
  {"u",  "up",        "Up"},
  {"d",  "down",      "Down"},
  {NULL, NULL,        NULL}
};

// CommandTable column indexes
#define CMD_NAME       0
#define CMD_ADMIN      1
#define CMD_LEVEL      2
#define CMD_POSITION   3
#define CMD_SOCIAL     4
#define CMD_FIGHT      5
#define CMD_MIN_WORDS  6
#define CMD_MAX_WORDS  7
#define CMD_MESSAGE    8

// CommandTable is a two - dimensional array of character pointers that stores command information,
// including command names, admin levels, positions, social interactions, fight options, word counts,
// and associated messages.
char *CommandTable[][9] =
{
  //                                                   MIN  MAX
  // Name          Admin Level Position  Social Fight Words Words Message
    {"advance",    "Y",  "1",  "sleep",  "N",   "N",  "3",  "3",  "Advance who and to what level?"} ,
    {"color",      "N",  "1",  "sleep",  "N",   "N",  "1",  "2",  "None"},
    {"equipment",  "N",  "1",  "sit",    "N",   "N",  "1",  "1",  "None"},
    {"go",         "N",  "1",  "stand",  "N",   "N",  "2",  "2",  "Go where?"},
    {"help",       "N",  "1",  "sleep",  "N",   "N",  "1",  "2",  "None"},
    {"inventory",  "N",  "1",  "sit",    "N",   "N",  "1",  "1",  "None"},
    {"kill",       "N",  "1",  "stand",  "N",   "Y",  "1",  "2",  "None"},
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
  DoEquipment,
  DoGo,
  DoHelp,
  DoInventory,
  DoKill,
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
  //zTestStuff();
  StartItUp();
  while (!GameShutDown)
  {
    HeartBeat();
    SocketCheckForNewPlayers();
    if (NoPlayers)
    {
      LogIt(GameSleepMsg);
      while (NoPlayers)
      {
        Sleep();
        SocketCheckForNewPlayers();
      }
      LogIt(GameWakeMsg);
    }
    ProcessPlayerInput();
    SocketSendPlayerOutput();
    Sleep();
  }
  ShutItDown();
}

// The HeartBeat function handles the heartbeat of the game server,
// which can be used for periodic tasks or checks.
void HeartBeat()
{
  DEBUGIT(2)
  CurrentTimeSec = time(NULL);
  if (CurrentTimeSec >= NextPlayerAutosave)
  {
    PlayerAutoSave();
    NextPlayerAutosave = CurrentTimeSec + PLAYER_AUTOSAVE_SECONDS;
  }
}

// Save online player records that have deferred changes.
void PlayerAutoSave()
{
  DEBUGIT(1)
  pConnSave     = pConn;
  pConnCurrSave = pConnCurr;
  pConnCurr     = pConnHead;
  while (pConnCurr != NULL)
  {
    pConn = pConnCurr;
    if (pConn->State == Online && pConn->PlayerDirty)
    {
      WritePlayerToFile();
    }
    pConnCurr = pConnCurr->pConnNext;
  }
  pConn = pConnSave;
  pConnCurr = pConnCurrSave;
}

// The ProcessPlayerInput function processes input from all players in a linked list, executing
// commands based on the input received and clearing the input buffer after processing.
void ProcessPlayerInput()
{
  DEBUGIT(2)
  SocketGetPlayerInput();
  pConnCurr = pConnHead;
  while (pConnCurr != NULL)
  {
    pConn = pConnCurr;
    if (pConn->Input[0] != '\0')
    {
      strcpy(Command, pConn->Input);
      pConn->Input[0] = '\0';
      ProcessCommand();
    }
    pConnCurr = pConnCurr->pConnNext;
  }
}

// The ProcessCommand function processes a command by trimming and logging it,
// checking the player's online status, and executing the command if it is valid.
void ProcessCommand()
{
  DEBUGIT(1)
  Trim(Command);
  LogIt(Command);
  if (pConn->State != Online)
  {
    GetPlayerOnline();
    return;
  }
  Word(1, Command, MudCmd);
  LowerCase(MudCmd);
  ProcessCommandAlias();

  if (MudCmdOk())
  {
    DoCommand[CommandNbr]();
  }
}

// Expand command aliases and normalize movement commands before validation.
void ProcessCommandAlias()
{
  DEBUGIT(1)
  i = 0;
  while (CommandAliasTable[i].Alias != NULL)
  {
    if (Equal(MudCmd, CommandAliasTable[i].Alias))
    {
      Parameters = strchr(Command, ' ');
      if (Parameters == NULL)
      {
        strcpy(Command, CommandAliasTable[i].Command);
      }
      else
      {
        sprintf(TmpStr, "%s%s", CommandAliasTable[i].Command, Parameters);
        strcpy(Command, TmpStr);
      }
      strcpy(MudCmd, CommandAliasTable[i].Command);
      break;
    }
    i++;
  }
  if (Words(Command) == 1)
  {
    strcpy(CmdParm1, MudCmd);
  }
  else
  if (Equal(MudCmd, "go") && Words(Command) == 2)
  {
    Word(2, Command, CmdParm1);
    LowerCase(CmdParm1);
  }
  else
  {
    return;
  }
  DirectionNbr = DirectionLookUp(CmdParm1);
  if (DirectionNbr < 0)
  {
    return;
  }
  sprintf(Command, "go %s", DirectionTable[DirectionNbr].LongName);
  strcpy(MudCmd, "go");
}

// Return the index of a direction by its short or long name.
int DirectionLookUp(char *Direction)
{
  DEBUGIT(1)
  i = 0;
  while (DirectionTable[i].ShortName != NULL)
  {
    if (Equal(Direction, DirectionTable[i].ShortName) ||
        Equal(Direction, DirectionTable[i].LongName))
    {
      return (int)i;
    }
    i++;
  }
  return -1;
}

// The MudCmdOk function checks if a given command is valid by comparing it against a command table,
// verifying user permissions, and ensuring the command meets the required word count, returning true
// if the command is valid and false otherwise.
bool MudCmdOk()
{
  DEBUGIT(1)
  i = 0;
  while (CommandTable[i][CMD_NAME] != NULL)
  {
    if (Equal(MudCmd, (char*)CommandTable[i][CMD_NAME]))
    {
      CommandNbr = (int)i;
      if ((size_t)CommandNbr >= CmdDoCount)
      { // If CommandNbr is out of range for DoCommand array, something is really wrong
        sprintf(Buffer, "FATAL: CommandNbr out of range (%d) for DoCommand size %zu", CommandNbr, CmdDoCount);
        LogIt(Buffer);
        AbortIt();
      }
      Commands.Name     = (char*)CommandTable[i][CMD_NAME];
      Commands.Admin    = (char*)CommandTable[i][CMD_ADMIN];
      Commands.Level    = (char*)CommandTable[i][CMD_LEVEL];
      Commands.Position = (char*)CommandTable[i][CMD_POSITION];
      Commands.Social   = (char*)CommandTable[i][CMD_SOCIAL];
      Commands.Fight    = (char*)CommandTable[i][CMD_FIGHT];
      Commands.MinWords = (char*)CommandTable[i][CMD_MIN_WORDS];
      Commands.MaxWords = (char*)CommandTable[i][CMD_MAX_WORDS];
      Commands.Message  = (char*)CommandTable[i][CMD_MESSAGE];
      if (Equal(Commands.Admin, "Y"))
      { // Admin command?
        if (pConn->pPlayer->Admin == 'N')
        {
          break;
        }
      }
      if (Words(Command) < atoi(Commands.MinWords) || Words(Command) > atoi(Commands.MaxWords))
      { // Too many or too few words
        if (Equal(Commands.Message, "None"))
        {
          sprintf(Buffer, "%s %s %s %s %s %s %s %s", "Too many or too few words in command,", "Min:", Commands.MinWords, "Max:", Commands.MaxWords, "Refer to help", Commands.Name, "\r\n");
          strcat(pConn->Output, Buffer);
        }
        else
        {
          strcat(pConn->Output, Commands.Message);
        }
        strcat(pConn->Output, "\r\n\r\n");
        Prompt(pConn);
        return false;
      }
      // Command is OK!
      return true;
    }
    i++;
  }
  // Command is none of the above
  strcat(pConn->Output, "Huh?\r\n\r\n");
  Prompt(pConn);
  return false;
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// All Do functions
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Advance a player's level in a game, updating their experience and notifying
// both the target player and the acting player of the change.
void DoAdvance()
{
  DEBUGIT(1)
  pTarget       = NULL;
  pConnCurrSave = pConnCurr;
  pConnCurr     = pConnHead;
  Word(2, Command, CmdParm1);
  NormalizePlayerName(CmdParm1);
  while (pConnCurr != NULL)
  {
    if (Equal(pConnCurr->pPlayer->Name, CmdParm1))
    {
      pTarget = pConnCurr;
      break;
    }
    pConnCurr = pConnCurr->pConnNext;
  }
  pConnCurr = pConnCurrSave;
  if (pTarget == NULL)
  {
    sprintf(Buffer, "%s %s", CmdParm1, "is not online\r\n");
    strcat(pConn->Output, Buffer);
    strcat(pConn->Output, "\r\n");
    Prompt(pConn);
    return;
  }
  Word(3, Command, CmdParm2);
  x = (size_t)atoi(CmdParm2);
  y = (size_t)(pTarget->pPlayer->Level);
  if (x == y)
  { // New level same as current level
    sprintf(Buffer, "%s %s %s %s", CmdParm1, "is already at level", CmdParm2, "\r\n");
    strcat(pConn->Output, Buffer);
    strcat(pConn->Output, "\r\n");
    Prompt(pConn);
    return;
  }
  if (x > 127)
  { // Level is type char and max value is 127
    sprintf(Buffer, "%s %s %s", "Level", CmdParm2, "is invalid\r\n");
    strcat(pConn->Output, Buffer);
    strcat(pConn->Output, "\r\n");
    Prompt(pConn);
    return;
  }
  // Advance the target player
  pActor = pConn;
  if (x > y)
  {
    sprintf(TmpStr, "%s", "promoted");
  }
  else
  {
    sprintf(TmpStr, "%s", "demoted");
  }
  pTarget->pPlayer->Level      = (char)atoi(CmdParm2);
  pTarget->pPlayer->Experience = (int)(pTarget->pPlayer->Level) * 100;
  // Message to target player
  strcat(pTarget->Output,"\r\n");
  sprintf(Buffer, "%s %s %s %s %s", pActor->pPlayer->Name, "has", TmpStr, "you to level", CmdParm2);
  strcat(pTarget->Output, Buffer);
  strcat(pTarget->Output, "\r\n\r\n");
  Prompt(pTarget);
  // Message to player
  sprintf(Buffer, "%s %s %s %s %s, ", pTarget->pPlayer->Name, "has been", TmpStr, "to level", CmdParm2);
  strcat(pActor->Output, Buffer);
  strcat(pActor->Output, "\r\n\r\n");
  Prompt(pActor);
  // Save target player
  pConn = pTarget;
  WritePlayerToFile();
  pConn = pActor;
}

// Manage the player's color settings in a game, allowing them to toggle color
// output on or off.
void DoColor()
{
  DEBUGIT(1)
  if (Words(Command) == 1)
  {
    if (pConn->pPlayer->Color == 'Y')
    {
      strcat(pConn->Output, "&CColor&N is &Mon&N.\r\n\r\n");
      Prompt(pConn);
      return;
    }
    if (pConn->pPlayer->Color == 'N')
    {
      strcat(pConn->Output, "Color is off.\r\n\r\n");
      Prompt(pConn);
      return;
    }
  };
  Word(2, Command, CmdParm1);
  LowerCase(CmdParm1);
  if (Equal(CmdParm1, "on"))
  {
    pConn->pPlayer->Color = 'Y';
    strcat(pConn->Output, "You will now see &RP&Gr&Ye&Bt&Mt&Cy&N &RC&Go&Yl&Bo&Mr&Cs&N.\r\n\r\n");
    Prompt(pConn);
  }
  if (Equal(CmdParm1, "off"))
  {
    pConn->pPlayer->Color = 'N';
    strcat(pConn->Output, "Color is off.\r\n\r\n");
    Prompt(pConn);
  }
  WritePlayerToFile();
}

// Display the player's equipment.
void DoEquipment()
{
  DEBUGIT(1)
  strcat(pConn->Output, "You have absolutely no equipment!\r\n\r\n");
  Prompt(pConn);
}

// Go in a specified direction
void DoGo()
{
  DEBUGIT(1)
  Word(2, Command, CmdParm1);
  LowerCase(CmdParm1);
  DirectionNbr = DirectionLookUp(CmdParm1);
  if (DirectionNbr < 0)
  {
    strcat(pConn->Output, "You go nowhere\r\n\r\n");
    Prompt(pConn);
    return;
  }
  pCurrentRoom = RoomLookUp(pConn->pPlayer->RoomNbr);
  if (pCurrentRoom == NULL || pCurrentRoom->Exits == NULL)
  {
    sprintf(LogMsg, "ERROR: Player is in missing room %d", pConn->pPlayer->RoomNbr);
    LogIt(LogMsg);
    strcat(pConn->Output, "You go nowhere\r\n\r\n");
    Prompt(pConn);
    return;
  }
  Word((size_t)DirectionNbr + 1, pCurrentRoom->Exits, CmdParm2);
  if (Equal(CmdParm2, "xxxxx"))
  {
    strcat(pConn->Output, "You go nowhere\r\n\r\n");
    Prompt(pConn);
    return;
  }
  DestRoomNbr = atoi(CmdParm2);
  pDestinationRoom = RoomLookUp(DestRoomNbr);
  if (pDestinationRoom == NULL)
  {
    sprintf(LogMsg, "ERROR: Room %d exit points to missing room %d", pConn->pPlayer->RoomNbr, DestRoomNbr);
    LogIt(LogMsg);
    strcat(pConn->Output, "You go nowhere\r\n\r\n");
    Prompt(pConn);
    return;
  }
  pConn->pPlayer->RoomNbr = DestRoomNbr;
  pConn->PlayerDirty = true;
  sprintf(Buffer, "You go %s\r\n", DirectionTable[DirectionNbr].LongName);
  strcat(pConn->Output, Buffer);
  DoLook();
}

// Retrieve and display help information from the help file.
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
    if (Words(Command) == 1)
    { // Just 'Help' was entered
      if (Equal(Buffer, "Help:"))
      {
        strcpy(Buffer, "\r\n");
      }
      strcat(pConn->Output, Buffer);
      strcat(pConn->Output,"\r\n");
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
    strcat(pConn->Output, Buffer);
    strcat(pConn->Output, "\r\n");
    if (strstr(Buffer, "Related help:"))
    {
      break;
    }
  }
  if (!Found)
  {
    strcat(pConn->Output, "Help topic not found\r\n");
  }
  fclose(HelpFile);
  strcat(pConn->Output, "\r\n");
  Prompt(pConn);
}

// Display the player's inventory.
void DoInventory()
{
  DEBUGIT(1)
  strcat(pConn->Output, "You look into your bag and find it empty\r\n\r\n");
  Prompt(pConn);
}

// Attack something.
void DoKill()
{
  DEBUGIT(1)
  strcat(pConn->Output, "You kill something\r\n\r\n");
  Prompt(pConn);
}

// Display the current room and its contents to the player.
void DoLook()
{
  DEBUGIT(1)
  pRoom = RoomLookUp(pConn->pPlayer->RoomNbr);
  if (pConn->pPlayer->Admin == 'N')
  {
    sprintf(Buffer, "\r\n&C%s&N\r\n", pRoom->Name);
  }
  else
  {
    sprintf(Buffer, "\r\n&C%s&N &M[&N%d %s&M]&N\r\n", pRoom->Name, pConn->pPlayer->RoomNbr, pRoom->Terrain);
  }
  strcat(pConn->Output, Buffer);
  sprintf(Buffer, "%s", pRoom->Desc);
  strcat(pConn->Output, Buffer);
  RoomExits = RoomGetExits(pRoom);
  sprintf(Buffer, "&CExits: %s&N\r\n\r\n", RoomExits);
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
}

// Calculate the elapsed time since a player's birth in days, hours, minutes,
// and seconds. Format this information into a string and append it to the
// player's output.
void DoPlayed()
{
  DEBUGIT(1)
  CurrentTime = time(NULL);
  ElapsedTime = difftime(CurrentTime, pConn->pPlayer->Born);
  // Calculate days, hours, minutes, and seconds
  Days        = (int)(ElapsedTime / (24 * 3600));
  ElapsedTime = fmod(ElapsedTime, (24 * 3600));
  Hours       = (int)(ElapsedTime / 3600);
  ElapsedTime = fmod(ElapsedTime, 3600);
  Minutes     = (int)(ElapsedTime / 60);
  Seconds     = (int)fmod(ElapsedTime, 60);
  sprintf(Buffer, "Your age is : %d days, %d hours, %d minutes, %d seconds\n", Days, Hours, Minutes, Seconds);
  strcat(pConn->Output, Buffer);
  strcat(pConn->Output, "\r\n");
  Prompt(pConn);
}

// Generate a formatted player file listing by reading player data from the
// player file.
void DoPlayerfile()
{
  DEBUGIT(1)
  strcat(pConn->Output, "\r\n");
  strcat(pConn->Output, "&C");
  strcat(pConn->Output, "Player file listing\r\n");
  strcat(pConn->Output, "&N");
  strcat(pConn->Output, "-------------------\r\n");
  strcat(pConn->Output, "Name       Admin Color Level Experience\r\n");
  EndFile = false;
  PlayerRcdNbr = 1;
  ReadPlayerFromFile();
  while (EndFile == false)
  {
    sprintf(Buffer, "%-10s %1s %c %3s %c %4s %2i %8s %i", PlayerRcd.Name, " ", PlayerRcd.Admin, " ", PlayerRcd.Color, " ", PlayerRcd.Level, " ", PlayerRcd.Experience);
    strcat(pConn->Output, Buffer);
    strcat(pConn->Output, "\r\n");
    PlayerRcdNbr++;
    ReadPlayerFromFile();
  }
  strcat(pConn->Output, "\r\n");
  Prompt(pConn);
}

// Handle the process of disconnecting a player by saving their data to the
// player file and changing their state to "Disconnect."
void DoQuit()
{
  DEBUGIT(1)
  WritePlayerToFile();
  strcat(pConn->Output, "Bye Bye");
  strcat(pConn->Output, "\r\n");
  pConn->State = Disconnect;
}

// Initiate the shutdown process of the game by setting a shutdown flag,
// displaying a shutdown message, and notifying all connected users.
void DoShutdown()
{
  DEBUGIT(1)
  GameShutDown = true;
  strcpy(MsgTxt, "\r\nYaGs is shutting down!\r\n");
  SendToAll();
}

// Generates a status report for the player.
void DoStatus()
{
  DEBUGIT(1)
  strcat(pConn->Output, "\r\n");
  // Name
  sprintf(Buffer, "Name: %s\r\n", pConn->pPlayer->Name);
  strcat(pConn->Output, Buffer);
  // Afk
  sprintf(Buffer, "AFK: %c\r\n", pConn->pPlayer->Afk);
  strcat(pConn->Output, Buffer);
  // Born
  strcpy(TmpStr, ctime(&pConn->pPlayer->Born));
  TmpStr[strlen(TmpStr) - 1] = '\0';
  sprintf(Buffer, "Born: %s\r\n", TmpStr);
  strcat(pConn->Output, Buffer);
  // Color
  sprintf(Buffer, "Color: %c\r\n", pConn->pPlayer->Color);
  strcat(pConn->Output, Buffer);
  // Experience
  sprintf(Buffer, "Experience: %i\r\n", pConn->pPlayer->Experience);
  strcat(pConn->Output, Buffer);
  // Level
  sprintf(Buffer, "Level: %i\r\n", pConn->pPlayer->Level);
  strcat(pConn->Output, Buffer);
  //  Sex
  sprintf(Buffer, "Sex: %c\r\n", pConn->pPlayer->Sex);
  strcat(pConn->Output, Buffer);
  // Admin
  if (pConn->pPlayer->Admin == 'Y')
  {
    strcat(pConn->Output, "Your are an Admin!\r\n");
  }
  strcat(pConn->Output, "\r\n");
  Prompt(pConn);
}

// Display a formatted list of online players, with level, etc.
void DoWho()
{
  DEBUGIT(1)
  strcat(pConn->Output, "\r\n");
  strcat(pConn->Output, "&C");
  strcat(pConn->Output, "Players online\r\n");
  strcat(pConn->Output, "&N");
  strcat(pConn->Output, "-------------------\r\n");
  strcat(pConn->Output, "Name       Level \r\n");
  pConnCurrSave = pConnCurr;
  pConnCurr     = pConnHead;
  while (pConnCurr != NULL)
  {
    if (pConnCurr->State == Online)
    {
      sprintf(Buffer, "%-10s %2s %2i", pConnCurr->pPlayer->Name, " ", pConnCurr->pPlayer->Level);
      strcat(pConn->Output, Buffer);
      strcat(pConn->Output, "\r\n");
    }
    pConnCurr = pConnCurr->pConnNext;
  }
  strcat(pConn->Output, "\r\n");
  Prompt(pConn);
  pConnCurr = pConnCurrSave;
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// General player communication
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Append a prompt string ("> ") to the player output.
void Prompt(ConnList *pConn)
{
  DEBUGIT(1)
  strcat(pConn->Output,"> ");
}

// Send a message to all players that are online.
void SendToAll()
{
  DEBUGIT(1)
  pConnSave     = pConn;
  pConnCurrSave = pConnCurr;
  pConnCurr     = pConnHead;
  while (pConnCurr != NULL)
  {
    if (pConnCurr->State != Online)
    {
      pConnCurr = pConnCurr->pConnNext;
      continue;
    }
    pConn = pConnCurr;
    if (pConn != pConnSave)
    {
      strcat(pConn->Output,"\r\n");
    }
    strcat(pConn->Output, MsgTxt);
    Prompt(pConn);
    pConnCurr = pConnCurr->pConnNext;
  }
  pConn   = pConnSave;
  pConnCurr = pConnCurrSave;
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Get player online
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Manage the online status and interactions of players in a game, handling
// various states such as greeting new players, validating names and passwords,
// and prompting for input based on the player's current state.
void GetPlayerOnline()
{
  DEBUGIT(1)
  //***********************************
  // Send Greeting
  //***********************************
  if (pConn->State == Send_Greeting)
  {
    SendGreeting();
    pConn->State = Wait_New_Player_YN;
    Prompt(pConn);
    return;
  }
  //***********************************
  // New player Y-N
  //***********************************
  if (pConn->State == Wait_New_Player_YN)
  {
    if (Equal(Command, "n"))
    { // Returning Player
      strcat(pConn->Output,"\r\nName?\r\n");
      pConn->State = Wait_Player_Name;
      Prompt(pConn);
      return;
    }
    if (Equal(Command, "y"))
    { // New Player
      strcat(pConn->Output,"\r\nSex M-F?\r\n");
      pConn->State = Wait_Sex;
      Prompt(pConn);
      return;
    }
    // Player didn't respond with a Y or N
    strcat(pConn->Output, "\r\nNew Player? Y or N\r\n");
    Prompt(pConn);
    return;
  }
  //***********************************
  // Returning player - Name
  //***********************************
  if (pConn->State == Wait_Player_Name)
  {
    NormalizePlayerName(Command);
    if (PlayerNameValid())
    { // Name is valid, ask for password
      strcat(pConn->Output, "\r\nPassword?\r\n");
      pConn->State = Wait_Password;
      Prompt(pConn);
      return;
    }
    // Didn't find player name, try again
    strcat(pConn->Output, "\r\n");
    strcat(pConn->Output, Command);
    strcat(pConn->Output, " not found, try again\r\n");
    strcat(pConn->Output, "\r\nName?\r\n");
    Prompt(pConn);
    return;
  }
  //***********************************
  // Returning player - Password
  //***********************************
  if (pConn->State == Wait_Password)
  {
    if (Equal(Command, pConn->pPlayer->Password))
    { // Password is valid
      SendMotd();
      pConn->State = Online;
      DoLook();
      return;
    }
    // Wrong password
    strcat(pConn->Output, "\r\nWrong password, try again\r\n");
    strcat(pConn->Output, "\r\nPassword?\r\n");
    Prompt(pConn);
    return;
  }
  //***********************************
  // New player - Sex
  //***********************************
  if (pConn->State == Wait_Sex)
  {
    if (Equal(Command, "m"))
    {
      pConn->pPlayer->Sex = 'M';
      strcat(pConn->Output, "\r\nName?\r\n");
      pConn->State = Wait_New_Player_Name;
      Prompt(pConn);
      return;
    }
    if (Equal(Command, "f"))
    {
      pConn->pPlayer->Sex = 'F';
      strcat(pConn->Output, "\r\nName?\r\n");
      pConn->State = Wait_New_Player_Name;
      Prompt(pConn);
      return;
    }
    strcat(pConn->Output, "\r\nSex not M or F, try again\r\n");
    strcat(pConn->Output, "\r\nSex M-F?\r\n");
    Prompt(pConn);
    return;
  }
  //***********************************
  // New player - Name
  //***********************************
  if (pConn->State == Wait_New_Player_Name)
  {
    NormalizePlayerName(Command);
    if (PlayerNameValid())
    { // Name is valid, ask for password
      strcpy(pConn->pPlayer->Name, Command);
      strcat(pConn->Output, "\r\nPassword?\r\n");
      pConn->BadPswdCount = 0;
      pConn->State = Wait_Password1;
      Prompt(pConn);
      return;
    }
    // Didn't find player name, try again
    strcat(pConn->Output, "\r\n");
    strcat(pConn->Output, Command);
    strcat(pConn->Output, " not found, try again\r\n");
    strcat(pConn->Output, "\r\nName?\r\n");
    Prompt(pConn);
    return;
  }
  //***********************************
  // New player - Password1
  //***********************************
  if (pConn->State == Wait_Password1)
  {
    strcpy(pConn->pPlayer->Password, Command);
    strcat(pConn->Output, "\r\nRe-enter Password\r\n");
    pConn->State = Wait_Password2;
    Prompt(pConn);
    return;
  }
  //***********************************
  // New player - Password2
  //***********************************
  if (pConn->State == Wait_Password2)
  {
    if (Equal(pConn->pPlayer->Password, Command))
    {
      GetNextPlayerRcdNbr();
      pConn->PlayerRcdNbr = PlayerRcdNbr;
      InitalizeNewPlayer();
      AddPlayerToFile();
      SendMotd();
      pConn->State = Online;
      DoLook();
      return;
    }
    // Check bad password count
    pConn->BadPswdCount++;
    if (pConn->BadPswdCount > 3)
    {
      strcat(pConn->Output, "\r\nThree tries, you're out!\r\n");
      pConn->State = Disconnect;
      return;
    }
    // Password don't match, try again
    strcat(pConn->Output, "\r\nPasswords don't match, try again\r\n");
    strcat(pConn->Output, "\r\nPassword?\r\n");
    pConn->State = Wait_Password1;
    Prompt(pConn);
    return;
  }
  strcpy(LogMsg,"ERROR: Logic - should never get here!");
  AbortIt();
}

// Read a greeting message from the greeting file and append its contents to
// a player's output.
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
    strcat(pConn->Output, Buffer);
  }
  fclose(GreetingFile);
}

// Read a message of the day (MOTD) from the MOTD file and append its contents
// to a player's output buffer.
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
    strcat(pConn->Output, Buffer);
  }
  fclose(MotdFile);
  strcat(pConn->Output, "\r\n");
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Log
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Initialize a log file and write the game start message.
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

// Log a message to a specified log file, prepending the current time to the
// message for accurate timestamping.
void LogIt(char *LogMsg)
{
  GetTime();
  fprintf(LogFile, "%s - %s\r\n", CurrentTimeTxt, LogMsg);
  fflush(LogFile);
}

// Close the log file and write the game stop message.
void CloseLog()
{
  DEBUGIT(1)
  LogIt(GameStopMsg);
  fclose(LogFile);
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Sockets
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// The SocketListen function initializes a listening socket for incoming TCP
// connections, configures it with various socket options, binds it to a
// specified port, and begins listening for connections.
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

// Monitor network sockets for new player connections and accept them if
// available, handling any errors that may occur during the process.
void SocketCheckForNewPlayers()
{
  DEBUGIT(2)
  FD_ZERO(&InpSet);
  FD_SET(Listen, &InpSet);
  MaxSocket = Listen;
  pConnCurr = pConnHead;
  while (pConnCurr != NULL)
  {
    Socket = pConnCurr->Socket;
    FD_SET(Socket, &InpSet);
    if (Socket > MaxSocket)
    {
      MaxSocket = Socket;
    }
    pConnCurr = pConnCurr->pConnNext;
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
    SocketAcceptNewPlayer();
  }
}

// Handle the acceptance of a new player connection by logging the connection
// details, updating the connection list, and managing the player's online status.
void SocketAcceptNewPlayer()
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
  AddToConnList();
  GetPlayerOnline();
}

// Process input from connected players by reading data from their respective
// sockets and updating their input state accordingly.
void SocketGetPlayerInput()
{
  DEBUGIT(2)
  pConnCurr = pConnHead;
  while (pConnCurr != NULL)
  {
    pConn = pConnCurr;
    Socket = pConn->Socket;
    pConn->Input[0] = '\0';
    if (FD_ISSET(Socket, &InpSet))
    {
      BytesRead = read(Socket, Buffer, 1024);
      Buffer[BytesRead] = '\0';
      strcpy(pConn->Input, Buffer);
      if (strlen(pConn->Input) > 0)
      {
        pConn->NoInputTick = 0;
        pConn->NoInputCount = 0;
      }
      else
      {
        pConn->NoInputTick++;
      }
    }
    pConnCurr = pConnCurr->pConnNext;
  }
}

// Process and send output messages to connected players, handling disconnection
// for those who have not provided input within a specified time limit.
void SocketSendPlayerOutput()
{
  DEBUGIT(2)
  pConnCurr = pConnHead;
  while (pConnCurr != NULL)
  {
    pConn = pConnCurr;
    if (pConn->NoInputTick > NO_INPUT_TICK)
    {
      pConn->NoInputTick = 0;
      pConn->NoInputCount++;
      if (pConn->NoInputCount > NO_INPUT_COUNT_LIMIT)
      {
        pConn->State = Disconnect;
        strcat(pConn->Output, "You are being disconnected. Bye Bye.\r\n\r\n");
        Prompt(pConn);
      }
      else
      {
        strcat(pConn->Output, "Are you still there?\r\n\r\n");
        Prompt(pConn);
      }
    }
    if (pConn->Output[0] == '\0')
    {
      pConnCurr = pConnCurr->pConnNext;
      continue;
    }
    Color();
    strcpy(Buffer, pConn->Output);
    pConn->Output[0] = '\0';
    BufferLen = strlen(Buffer);
    Socket = pConn->Socket;
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
    pConnCurr = pConnCurr->pConnNext;
  }
  SocketDisconnectPlayers();
}

// Iterate through the connection list, disconnecting players set to the "Disconnect"
// state by closing their sockets and removing them from the connection list.
void SocketDisconnectPlayers()
{
  DEBUGIT(2)
  pConnCurr = pConnHead;
  while (pConnCurr != NULL)
  {
    pConn = pConnCurr;
    if (pConn->State == Disconnect)
    {
      if (pConn->PlayerRcdNbr > 0 && pConn->PlayerDirty)
      {
        WritePlayerToFile();
      }
      close(pConn->Socket);
      DelFromConnList();
      continue;
    }
    pConnCurr = pConnCurr->pConnNext;
  }
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Start up and shutdown
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Initialize the system by performing a series of setup tasks, including
// initialization, logging, socket listening, and file opening.
void StartItUp()
{ // Do not add DEBUGIT
  Initialization();
  OpenLog();
  ValidateCommandTable();
  SocketListen();
  OpenPlayerFile();
  MobileReadFile();
  ObjectReadFile();
  RoomReadFile();
}

// Set up the initial state of a game by resetting various player-related
// variables and setting the game state.
void Initialization()
{ // Do not add DEBUGIT
  GameShutDown       = false;
  NoPlayers          = true;
  NextPlayerAutosave = time(NULL) + PLAYER_AUTOSAVE_SECONDS;
  pConnHead          = NULL;
  pConnTail          = NULL;
  pConnCurr          = NULL;
}

// Gracefully shut down the game by closing files and logs.
void ShutItDown()
{
  DEBUGIT(1)
  PlayerAutoSave();
  RoomFreeList();
  ClosePlayerFile();
  CloseLog();
  close(Listen);
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Connection list
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Allocate memory for a new connection node, initialize its properties, and add it
// to a linked list of connections, handling both the first node and subsequent
// nodes appropriately.
void AddToConnList()
{
  DEBUGIT(1)
  pConn          = (ConnList *)calloc(1, sizeof(ConnList));
  pConn->pPlayer = (Player *)calloc(1, sizeof(Player));
  pConnCurr      = pConn;
  if (pConnHead != NULL)
  { // Not 1st Node
    pConnTail->pConnNext = pConnCurr;
    pConnCurr->pConnPrev = pConnTail;
    pConnTail            = pConnCurr;
  }
  else
  { // 1st Node
    pConnHead = pConnCurr;
    pConnTail = pConnCurr;
  }
  pConn->Socket       = Socket;
  pConn->State        = Send_Greeting;
  pConn->Output[0]    = '\0';
  pConn->NoInputTick  = 0;
  pConn->NoInputCount = 0;
  pConn->pConnNext    = NULL;
}

// Remove the current connection node from the doubly linked connection list, handling
// cases for deleting the head, tail, or a middle node, and update the list
// pointers accordingly.
void DelFromConnList()
{
  DEBUGIT(1)
  // Delete when only one node in list
  if (pConnCurr == pConnHead)
  {
    if (pConnCurr == pConnTail)
    { // We have no players
      pConnHead = NULL;
      pConnTail = NULL;
      NoPlayers   = true;
    }
  }
  else
  // Delete head node
  if (pConnCurr == pConnHead)
  {
    pConnHead = pConnCurr->pConnNext;
    pConnCurr->pConnNext->pConnPrev = NULL;
  }
  else
  // Delete tail node
  if (pConnCurr == pConnTail)
  {
    pConnTail = pConnCurr->pConnPrev;
    pConnCurr->pConnPrev->pConnNext = NULL;
  }
  else
  {
    // Delete middle node
    pConnCurr->pConnPrev->pConnNext = pConnCurr->pConnNext;
    pConnCurr->pConnNext->pConnPrev = pConnCurr->pConnPrev;
  }
  // Free node
  pConnCurr = pConn->pConnNext;
  free(pConn->pPlayer);
  free(pConn);
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Player file
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Open the player file for reading and writing.
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

// Close the player file.
void ClosePlayerFile()
{
  DEBUGIT(1)
  fclose(PlayerFile);
}

// Calculate and return the file offset for a player based on the size of the
// Player structure and current value of PlayerRcdNbr, effectively determining
// the byte position in a file where a specific player's data would be stored.
long GetPlayerFileOffset()
{
  DEBUGIT(1)
  x = (size_t)sizeof(Player);
  y = (size_t)PlayerRcdNbr - 1;
  Offset = (long)(x * y);
  return Offset;
}

// Check the validity of the player's name.
bool PlayerNameValid()
{
  DEBUGIT(1)
  if (pConn->State == Wait_Player_Name)
  {
    return PlayerNameValidOld();
  }
  if (pConn->State == Wait_New_Player_Name)
  {
    return PlayerNameValidNew();
  }
  strcpy(LogMsg,"ERROR: Logic - should never get here!");
  AbortIt();
  return false;
}

// Check if a player's name matches an existing player in the player file.
bool PlayerNameValidOld()
{
  DEBUGIT(1)
  Found        = false;
  EndFile      = false;
  PlayerRcdNbr = 1;
  ReadPlayerFromFile();
  while (!EndFile)
  {
    if (Equal(PlayerRcd.Name, Command))
    { // Match!
      Found = true;
      pConn->PlayerRcdNbr = PlayerRcdNbr;
      *pConn->pPlayer = PlayerRcd;
      break;
    }
    PlayerRcdNbr++;
    ReadPlayerFromFile();
  }
  return Found;
}

// Check if a player name exists in a predefined list of valid names.
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
    Trim(Buffer);
    if (Equal(Command, Buffer))
    { // Match!
      Found = true;
      break;
    }
    if (feof(ValidNamesFile))
    {
      Found = false;
      break;
    }
  }
  fclose(ValidNamesFile);
  return Found;
}

// Read player data from the player file.
void ReadPlayerFromFile()
{
  DEBUGIT(1)
  fseek(PlayerFile, GetPlayerFileOffset(), SEEK_SET);
  fread(&PlayerRcd, sizeof(PlayerRcd), 1, PlayerFile);
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

// Write player data to the player file.
void WritePlayerToFile()
{
  DEBUGIT(1)
  PlayerRcdNbr = pConn->PlayerRcdNbr;
  ReturnValue1 = fseek(PlayerFile, GetPlayerFileOffset(), SEEK_SET);
  if (ReturnValue1 != 0)
  {
    sprintf(LogMsg,"ERROR: fseek %s", PLAYER_FILE);
    AbortIt();
  }
  ReturnValue2 = fwrite(pConn->pPlayer, sizeof(Player), 1, PlayerFile);
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
  pConn->PlayerDirty = false;
}

// Add a player record to a file, initializing the player as an admin if the
// file is empty.
void AddPlayerToFile()
{
  DEBUGIT(1)
  if (ftell(PlayerFile) == 0)
  { // Player file has no records, so this MUST be an Admin!
    pConn->pPlayer->Admin = 'Y';
  };
  ReturnValue1 = fseek(PlayerFile, 0, SEEK_END);
  if (ReturnValue1 != 0)
  {
    sprintf(LogMsg,"ERROR: fseek %s", PLAYER_FILE);
    AbortIt();
  }
  ReturnValue2 = fwrite(pConn->pPlayer, sizeof(Player), 1, PlayerFile);
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

// Initializes a new player record.
void InitalizeNewPlayer()
{
  DEBUGIT(1)
  pConn->pPlayer->Admin      = 'N';
  pConn->pPlayer->Afk        = 'N';
  pConn->pPlayer->Born       = time(NULL);
  pConn->pPlayer->Color      = 'N';
  pConn->pPlayer->Experience = 0;
  pConn->pPlayer->Level      = 1;
  pConn->pPlayer->RoomNbr    = PLAYER_START_ROOM;
}

// Determines the next PlayerRcdNbr by reading the player file until EOF.
void GetNextPlayerRcdNbr()
{
  DEBUGIT(1)
  EndFile      = false;
  PlayerRcdNbr = 1;
  ReadPlayerFromFile();
  while (EndFile == false)
  {
    PlayerRcdNbr++;
    ReadPlayerFromFile();
  }
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Strings
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Compare two C-style strings for equality and return true if they are identical
// and false otherwise.
bool Equal(char *Str1, char *Str2)
{
  DEBUGIT(2)
  return (!strcmp(Str1, Str2));
}

// Convert all characters in the input C-style string Str to lowercase.
void LowerCase(char *Str)
{
  DEBUGIT(2)
  for (i = 0; Str[i]; i++)
  {
    Str[i] = (char)tolower(Str[i]);
  }
}

// Convert a player name to lowercase, then capitalize the first character.
void NormalizePlayerName(char *Name)
{
  DEBUGIT(2)
  LowerCase(Name);
  Up1stChar(Name);
}

// Append Str2 to Str1
void StrAppend(char *Str1, char *Str2)
{
  DEBUGIT(2)
  x = strlen(Str1);
  y = strlen(Str2);
  if (x + y >= STRING_LIMIT)
  {
    sprintf(LogMsg, "ERROR: STRING_LIMIT exceeded in StrAppend");
    AbortIt();
  }
  memcpy(Str1 + x, Str2, y);
  Str1[x + y] = '\0';
}

// Remove the trailing new line or carriage return character from a C-style string,
void StripTrailingNlCr(char *Buffer)
{
  StrLen = strlen(Buffer);
  if (StrLen > 1 && Buffer[StrLen - 2] == '\r' && Buffer[StrLen - 1] == '\n')
  { // Remove "\r\n"
    Buffer[StrLen - 2] = '\0';
  }
  else
  if (StrLen > 1 && Buffer[StrLen - 2] == '\n' && Buffer[StrLen - 1] == '\r')
  { // Remove "\n\r"
    Buffer[StrLen - 2] = '\0';
  }
  else
  if (StrLen > 0 && Buffer[StrLen - 1] == '\n')
  { // Remove "\n"
    Buffer[StrLen - 1] = '\0';
  }
  else
  if (StrLen > 0 && Buffer[StrLen - 1] == '\r')
  { // Remove "\r"
    Buffer[StrLen - 1] = '\0';
  }
}

// Remove leading and trailing whitespace characters from a given C-style string.
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

// Convert the first character of a string to uppercase.
void Up1stChar(char *Str)
{
  DEBUGIT(2)
  Str[0] = (char)toupper(Str[0]);
}

// Extract the N-th word from the input string Str1 and copy it into the output
// string Str2, where Nbr specifies the word position to extract.
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

// Return the number of words in a string.
size_t Words(char *Str)
{
  DEBUGIT(2)
  #define NotWord 0
  #define InWord  1
  WordState = 0;
  x = 0;
  for (i = 0; Str[i]; i++)
  {
    if (isspace(Str[i]))
    {
      WordState = NotWord;
    }
    else if (WordState == NotWord)
    {
      WordState = InWord;
      x++;
    }
  }
  return x;
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Miscellaneous
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Log a message, close the log, and then terminate the program.
void AbortIt()
{ // Do not add DEBUGIT
  LogIt(LogMsg);
  CloseLog();
  exit(1);
}

// Process a player's output string to replace color codes indicated by '&' with
// corresponding ANSI color codes, modifying the output accordingly.
void Color()
{
  DEBUGIT(2)
  if (strchr(pConn->Output, '&') == NULL) return;
  pOutput = &pConn->Output[0];
  pTmpStr = &TmpStr[0];
  while (*pOutput)
  { // Loop here until we hit an '&'
    while (*pOutput != '&')
    {
      *pTmpStr = *pOutput;
      if (*pOutput == '\0')
      { // We are done
        *pTmpStr = '\0';
        strcpy(pConn->Output, TmpStr);
        return;
      }
      pOutput++;
      pTmpStr++;
    }
    // We hit an '&'
    pOutPlus1 = pOutput;
    pOutPlus1++;
    switch (*pOutPlus1)
    {
    case 'N':
      pColor = Normal;
      break;
    case 'K':
      pColor = BrightBlack;
      break;
    case 'R':
      pColor = BrightRed;
      break;
    case 'G':
      pColor = BrightGreen;
      break;
    case 'Y':
      pColor = BrightYellow;
      break;
    case 'B':
      pColor = BrightBlue;
      break;
    case 'M':
      pColor = BrightMagenta;
      break;
    case 'C':
      pColor = BrightCyan;
      break;
    case 'W':
      pColor = BrightWhite;
      break;
    }
    if (pConn->pPlayer->Color == 'N')
    {
      pColor = None;
    }
    while (*pColor != '\0')
    { // Copy the color code string
      *pTmpStr = *pColor;
      pTmpStr++;
      pColor++;
    }
    pOutput++;
    pOutput++;
  }
}

// Retrieve the current time in a human-readable format.
void GetTime()
{ // Do not add DEBUGIT
  CurrentTimeSec = time(NULL);              // Seconds since Epoch, 1970-01-01 00:00:00 +0000 (UTC)
  CurrentTimeTxt = ctime(&CurrentTimeSec);  // Convert to human readable
  x = strlen(CurrentTimeTxt);               // Get rid of the '\n'
  CurrentTimeTxt[x - 1] = '\0';             //   at the end of string returned by ctime()
}

// Pause the game execution for SLEEP_TIME to manage CPU usage effectively.
void Sleep()
{
  DEBUGIT(2)
  if (USE_USLEEP == 'Y')                    // Sleeping the game is one way to avoid needless
  {                                         //   consumption of CPU. Typically, usleep() works
    usleep(SLEEP_TIME);                     //   just fine for this purpose. Using select() is
  }                                         //   another (not recommended) means of sleeping a process
  else                                      // If the YaGs development environment is Windows 11,
  {                                         //   Visual Studio, and WSL (Windows Subsystem for Linux)
    #include <sys/select.h>                 //   Ubuntu, then for some strange reason, usleep() does not
    TimeOut.tv_sec = 0;                     //   does not actually sleep.
    TimeOut.tv_usec = SLEEP_TIME;           // So this messy function is the result. You should
    select(0, NULL, NULL, NULL, &TimeOut);  //   adjust SLEEP_TIME until you are happy.
  }
}

// Validate CommandTable & DoCommand alignment
void ValidateCommandTable()
{
  DEBUGIT(1)
  CmdDoCount    = sizeof(DoCommand) / sizeof(DoCommand[0]);
  CmdTableCount = 0;
  while (CommandTable[CmdTableCount][0] != NULL)
  {
    CmdTableCount++;
  }
  if (CmdDoCount != CmdTableCount)
  {
    sprintf(Buffer, "FATAL: Command table/handler mismatch. CommandTable=%zu DoCommand=%zu", CmdTableCount, CmdDoCount);
    LogIt(Buffer);
    AbortIt();
  }
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Mobiles
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Read mobile definitions from Mobiles.txt and build the permanent mobile list.
void MobileReadFile()
{
  DEBUGIT(1)
  sprintf(TmpStr, "%s/%s/%s", YAGS_DIR, WORLD_DIR, MOBILES_FILE);
  MobileFile = fopen(TmpStr, "r");
  if (MobileFile == NULL)
  {
    sprintf(LogMsg, "ERROR: Open %s failed: %s", MOBILES_FILE, strerror(errno));
    AbortIt();
  }
  while (fgets(Buffer, sizeof(Buffer), MobileFile) != NULL)
  {
    // Add Mobile node
    StripTrailingNlCr(Buffer);
    if (Buffer[0] == '\0')
    {
      continue;
    }
    if (pMobileListHead == NULL)
    {
      pMobileListHead = (MobileList *)calloc(1, sizeof(MobileList));
      pMobileListTail = pMobileListHead;
    }
    else
    {
      pMobileListTail->pNextMobile = (MobileList *)calloc(1, sizeof(MobileList));
      pMobileListTail = pMobileListTail->pNextMobile;
    }
    if (pMobileListTail == NULL)
    {
      sprintf(LogMsg, "ERROR: Memory allocation failed for MobileList node");
      AbortIt();
    }
    pMobileListTail->pMobile = (Mobile *)calloc(1, sizeof(Mobile));
    if (pMobileListTail->pMobile == NULL)
    {
      sprintf(LogMsg, "ERROR: Memory allocation failed for Mobile");
      AbortIt();
    }
    // Mobile Id
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Id = strdup(TmpStr);
    // Desc1
    fgets(Buffer, sizeof(Buffer), MobileFile);
    StripTrailingNlCr(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Desc1 = strdup(TmpStr);
    // Desc2
    fgets(Buffer, sizeof(Buffer), MobileFile);
    StripTrailingNlCr(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Desc2 = strdup(TmpStr);
    // Desc3
    fgets(Buffer, sizeof(Buffer), MobileFile);
    StripTrailingNlCr(Buffer);
    TmpStr[0] = '\0';
    while (fgets(Buffer, sizeof(Buffer), MobileFile) != NULL)
    {
      StripTrailingNlCr(Buffer);
      if (strncmp(Buffer, "Attack:", 7) == 0)
      {
        break;
      }
      StrAppend(TmpStr, Buffer);
      StrAppend(TmpStr, "\n");
    }
    pMobileListTail->pMobile->Desc3 = strdup(TmpStr);
    // Attack
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Attack = strdup(TmpStr);
    // Level
    fgets(Buffer, sizeof(Buffer), MobileFile);
    StripTrailingNlCr(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Level = atoi(TmpStr);
    // Hit
    fgets(Buffer, sizeof(Buffer), MobileFile);
    StripTrailingNlCr(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Hit = atoi(TmpStr);
    // Exp
    fgets(Buffer, sizeof(Buffer), MobileFile);
    StripTrailingNlCr(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Exp = atoi(TmpStr);
    // Loot
    fgets(Buffer, sizeof(Buffer), MobileFile);
    StripTrailingNlCr(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Loot = strdup(TmpStr);
  }
  fclose(MobileFile);
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Objects
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Read object definitions from Objects.txt and build the permanent object list.
void ObjectReadFile()
{
  DEBUGIT(1)
  sprintf(TmpStr, "%s/%s/%s", YAGS_DIR, WORLD_DIR, OBJECTS_FILE);
  ObjectFile = fopen(TmpStr, "r");
  if (ObjectFile == NULL)
  {
    sprintf(LogMsg, "ERROR: Open %s failed: %s", OBJECTS_FILE, strerror(errno));
    AbortIt();
  }
  while (fgets(Buffer, sizeof(Buffer), ObjectFile) != NULL)
  {
    // Add Object node
    StripTrailingNlCr(Buffer);
    if (Buffer[0] == '\0')
    {
      continue;
    }
    if (pObjectListHead == NULL)
    {
      pObjectListHead = (ObjectList *)calloc(1, sizeof(ObjectList));
      pObjectListTail = pObjectListHead;
    }
    else
    {
      pObjectListTail->pNextObject = (ObjectList *)calloc(1, sizeof(ObjectList));
      pObjectListTail = pObjectListTail->pNextObject;
    }
    if (pObjectListTail == NULL)
    {
      sprintf(LogMsg, "ERROR: Memory allocation failed for ObjectList node");
      AbortIt();
    }
    pObjectListTail->pObject = (Object *)calloc(1, sizeof(Object));
    if (pObjectListTail->pObject == NULL)
    {
      sprintf(LogMsg, "ERROR: Memory allocation failed for Object");
      AbortIt();
    }
    // Object Id
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pObjectListTail->pObject->Id = strdup(TmpStr);
    // Desc1
    fgets(Buffer, sizeof(Buffer), ObjectFile);
    StripTrailingNlCr(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pObjectListTail->pObject->Desc1 = strdup(TmpStr);
    // Desc2
    fgets(Buffer, sizeof(Buffer), ObjectFile);
    StripTrailingNlCr(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pObjectListTail->pObject->Desc2 = strdup(TmpStr);
    // Desc3
    fgets(Buffer, sizeof(Buffer), ObjectFile);
    StripTrailingNlCr(Buffer);
    TmpStr[0] = '\0';
    while (fgets(Buffer, sizeof(Buffer), ObjectFile) != NULL)
    {
      StripTrailingNlCr(Buffer);
      if (strncmp(Buffer, "Weight:", 7) == 0)
      {
        break;
      }
      StrAppend(TmpStr, Buffer);
      StrAppend(TmpStr, "\n");
    }
    pObjectListTail->pObject->Desc3 = strdup(TmpStr);
    // Weight
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pObjectListTail->pObject->Weight = atoi(TmpStr);
    // Cost
    fgets(Buffer, sizeof(Buffer), ObjectFile);
    StripTrailingNlCr(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pObjectListTail->pObject->Cost = atoi(TmpStr);
    // Type
    fgets(Buffer, sizeof(Buffer), ObjectFile);
    StripTrailingNlCr(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pObjectListTail->pObject->Type = strdup(TmpStr);
    // Subtype
    fgets(Buffer, sizeof(Buffer), ObjectFile);
    StripTrailingNlCr(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pObjectListTail->pObject->Subtype = strdup(TmpStr);
    // Value
    fgets(Buffer, sizeof(Buffer), ObjectFile);
    StripTrailingNlCr(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pObjectListTail->pObject->Value = atoi(TmpStr);
  }
  fclose(ObjectFile);
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Rooms
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Add a new Room to the linked list of rooms
void RoomAddToRoomList()
{
  DEBUGIT(1)
  // Allocate memory for a new RoomList node
  pNewRoomListNode = (RoomList *)malloc(sizeof(RoomList));
  if (pNewRoomListNode == NULL)
  {
    sprintf(LogMsg, "ERROR: Memory allocation failed for RoomList node");
    AbortIt();
  }
  // Initialize the new node
  pNewRoomListNode->pRoom     = pNewRoom;
  pNewRoomListNode->pNextRoom = NULL;
  if (pRoomListHead != NULL)
  {
    // Use pRoomTail to append the new node to the end of the list
    pRoomListTail->pNextRoom = pNewRoomListNode;
    pRoomListTail = pNewRoomListNode;
  }
  else
  { // First room being added
    pRoomListHead = pNewRoomListNode;
    pRoomListTail = pNewRoomListNode;
  }
}

// Dynamically allocate a Room structure, copy the contents of SingleRoom, and
// return a pointer to the newly allocated Room structure.
Room *RoomAllocateAndCopy(const Room *SourceRoom)
{
  // Allocate memory for the new Room
  pNewRoom = (Room*)malloc(sizeof(Room));
  if (pNewRoom == NULL) {
    sprintf(LogMsg, "ERROR: Memory allocation failed for Room");
    AbortIt();
  }
  // Copy the RoomNumber
  pNewRoom->RoomNbr = SourceRoom->RoomNbr;
  // Allocate and copy the Name
  if (SourceRoom->Name != NULL)
  {
    pNewRoom->Name = strdup(SourceRoom->Name);
    if (pNewRoom->Name == NULL)
    {
      sprintf(LogMsg, "ERROR: Memory allocation failed for Room Name");
      AbortIt();
    }
  }
  else
  {
    pNewRoom->Name = NULL;
  }
  // Allocate and copy the Description
  if (SourceRoom->Desc != NULL)
  {
    pNewRoom->Desc = strdup(SourceRoom->Desc);
    if (pNewRoom->Desc == NULL)
    {
      sprintf(LogMsg, "ERROR: Memory allocation failed for Room Description");
      AbortIt();
    }
  }
  else
  {
    pNewRoom->Desc = NULL;
  }
  // Allocate and copy the Terrain
  if (SourceRoom->Terrain != NULL)
  {
    pNewRoom->Terrain = strdup(SourceRoom->Terrain);
    if (pNewRoom->Terrain == NULL)
    {
      sprintf(LogMsg, "ERROR: Memory allocation failed for Room Terrain");
      AbortIt();
    }
  }
  else
  {
    pNewRoom->Terrain = NULL;
  }
  // Allocate and copy the Flags
  if (SourceRoom->Flags != NULL)
  {
    pNewRoom->Flags = strdup(SourceRoom->Flags);
    if (pNewRoom->Flags == NULL)
    {
      sprintf(LogMsg, "ERROR: Memory allocation failed for Room Flags");
      AbortIt();
    }
  }
  else {
    pNewRoom->Flags = NULL;
  }
  // Allocate and copy the Exits
  if (SourceRoom->Exits != NULL)
  {
    pNewRoom->Exits = strdup(SourceRoom->Exits);
    if (pNewRoom->Exits == NULL)
    {
      sprintf(LogMsg, "ERROR: Memory allocation failed for Room Exits");
      AbortIt();
    }
  }
  else
  {
    pNewRoom->Exits = NULL;
  }
  return pNewRoom;
}

// Free all dynamically allocated memory for the linked list of rooms
void RoomFreeList()
{
  DEBUGIT(1)
  pRoomListCurr = pRoomListHead;
  while (pRoomListCurr != NULL)
  {
    pRoomListNext = pRoomListCurr->pNextRoom;
    if (pRoomListCurr->pRoom != NULL)
    {
      free(pRoomListCurr->pRoom->Name);
      free(pRoomListCurr->pRoom->Desc);
      free(pRoomListCurr->pRoom->Terrain);
      free(pRoomListCurr->pRoom->Flags);
      free(pRoomListCurr->pRoom->Exits);
      free(pRoomListCurr->pRoom);
    }
    free(pRoomListCurr);
    pRoomListCurr = pRoomListNext;
  }
  pRoomListHead = NULL;
  pRoomListTail = NULL;
}

// Parse the pRoom->Exits string and return a formatted string of available exits.
char *RoomGetExits(const Room *pRoom)
{
  if (pRoom == NULL || pRoom->Exits == NULL)
  {
    return strdup("");
  }
  TmpStr[0] = '\0';
  // Tokenize the Exits string and map valid room numbers to directions.
  pExitsCopy = strdup(pRoom->Exits);
  if (pExitsCopy == NULL)
  {
    sprintf(LogMsg, "ERROR: Memory allocation failed for exitsCopy in RoomGetExits");
    AbortIt();
  }
  pToken = strtok(pExitsCopy, " ");
  for (i = 0; pToken != NULL && i < 10; i++)
  {
    if (strcmp(pToken, "xxxxx") != 0)
    {
      if (strlen(TmpStr) > 0)
      {
        strcat(TmpStr, " ");
      }
      strcat(TmpStr, DirectionTable[i].DisplayName);
    }
    pToken = strtok(NULL, " ");
  }
  free(pExitsCopy);
  return TmpStr;
}

// Search for a room in the linked list of rooms by its RoomNbr. Return a pointer
// to the Room structure if found, or NULL if not found.
Room *RoomLookUp(int RoomNbr)
{
  DEBUGIT(1)
  pRoomListCurr = pRoomListHead;
  while (pRoomListCurr != NULL)
  {
    if (pRoomListCurr->pRoom != NULL && pRoomListCurr->pRoom->RoomNbr == RoomNbr)
    {
      return pRoomListCurr->pRoom; // Return the matching Room pointer
    }
    pRoomListCurr = pRoomListCurr->pNextRoom;
  }
  return NULL;
}

// Read room data from a file, parsing the room number, name, etc, store it in
// the SingleRoom structure, which is then added to the linked list of rooms.
void RoomReadFile()
{
  sprintf(RoomFileName, "%s/%s/%s", YAGS_DIR, WORLD_DIR, ROOMS_FILE);
  RoomFile = fopen(RoomFileName, "r");
  if (RoomFile == NULL)
  {
    sprintf(LogMsg, "ERROR: Open %s failed: %s", ROOMS_FILE, strerror(errno));
    AbortIt();
  }
  // Read rooms until $End is found
  while (true)
  {
    // Read Room Number and Name
    if (fgets(Buffer, sizeof(Buffer), RoomFile) == NULL)
    {
      sprintf(LogMsg, "ERROR: Failed to read Room Number and Name from %s at line %d", ROOMS_FILE, LineNbr);
      AbortIt();
    }
    StripTrailingNlCr(Buffer);
    LineNbr++;
    Buffer[strcspn(Buffer, "\n")] = '\0';
    // Stop processing if $End is found
    if (strcmp(Buffer, "$End") == 0)
    {
      sprintf(LogMsg, "INFO: End of room data reached at line %d in %s", LineNbr, ROOMS_FILE);
      LogIt(LogMsg);
      break;
    }
    // Extract Room Number (first word)
    pToken = strtok(Buffer, " ");
    if (pToken == NULL)
    {
      sprintf(LogMsg, "ERROR: Failed to parse Room Number from %s at line %d", ROOMS_FILE, LineNbr);
      AbortIt();
    }
    SingleRoom.RoomNbr = atoi(pToken);
    // Extract Room Name (rest of the line)
    pToken = strtok(NULL, "");
    if (pToken == NULL)
    {
      sprintf(LogMsg, "ERROR: Failed to parse Room Name from %s at line %d", ROOMS_FILE, LineNbr);
      AbortIt();
    }
    SingleRoom.Name = strdup(pToken);
    // Read Description (multi-line until "Terrain" label is found)
    TmpStr[0] = '\0';
    while (true)
    {
      if (fgets(Buffer, sizeof(Buffer), RoomFile) == NULL)
      {
        sprintf(LogMsg, "ERROR: Failed while reading Description from %s at line %d", ROOMS_FILE, LineNbr);
        AbortIt();
      }
      StripTrailingNlCr(Buffer);
      LineNbr++;
      Buffer[strcspn(Buffer, "\n")] = '\0';
      if (strncmp(Buffer, "Terrain: ", 9) == 0)
      {
        break;
      }
      StrAppend(TmpStr, Buffer);
      StrAppend(TmpStr, "\n");
    }
    if (TmpStr[0] != '\0')
    {
      SingleRoom.Desc = TmpStr;
    }
    else
    {
      sprintf(LogMsg, "ERROR: No description found for room in %s at line %d", ROOMS_FILE, LineNbr);
      AbortIt();
    }
    // Read Terrain
    if (!strncmp(Buffer, "Terrain: ", 9) == 0)
    {
      sprintf(LogMsg, "ERROR: Invalid Terrain format in %s at line %d", ROOMS_FILE, LineNbr);
      AbortIt();
    }
    SingleRoom.Terrain = strdup(Buffer + 9);
    // Read Flags
    if (fgets(Buffer, sizeof(Buffer), RoomFile) == NULL)
    {
      sprintf(LogMsg, "ERROR: Failed to read Flags from %s at line %d", ROOMS_FILE, LineNbr);
      AbortIt();
    }
    StripTrailingNlCr(Buffer);
    LineNbr++;
    Buffer[strcspn(Buffer, "\n")] = '\0';
    if (!strncmp(Buffer, "Flags: ", 7) == 0)
    {
      sprintf(LogMsg, "ERROR: Invalid Flags format in %s at line %d", ROOMS_FILE, LineNbr);
      AbortIt();
    }
    SingleRoom.Flags = strdup(Buffer + 7);
    // Read Exits
    if (fgets(Buffer, sizeof(Buffer), RoomFile) == NULL)
    {
      sprintf(LogMsg, "ERROR: Failed to skip exits header line in %s at line %d", ROOMS_FILE, LineNbr);
      AbortIt();
    }
    LineNbr++;
    if (fgets(Buffer, sizeof(Buffer), RoomFile) == NULL)
    {
      sprintf(LogMsg, "ERROR: Failed to read Exits from %s at line %d", ROOMS_FILE, LineNbr);
      AbortIt();
    }
    StripTrailingNlCr(Buffer);
    LineNbr++;
    Buffer[strcspn(Buffer, "\n")] = '\0';
    SingleRoom.Exits = strdup(Buffer);
    if (fgets(Buffer, sizeof(Buffer), RoomFile) == NULL)
    {
      sprintf(LogMsg, "ERROR: Failed to skip blank line after exits in %s at line %d", ROOMS_FILE, LineNbr);
      AbortIt();
    }
    LineNbr++;
    pNewRoom = RoomAllocateAndCopy(&SingleRoom);
    RoomAddToRoomList();
  }
  fclose(RoomFile);
}

/*
void zTestStuff()
{
  OpenLog();
  RoomReadFile();
  RoomFreeList();
  CloseLog();
  exit(0);
}
*/
