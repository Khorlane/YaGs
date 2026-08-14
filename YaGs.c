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
#include <ctype.h>                                       // isdigit(), isspace(), tolower(), toupper()
#include <errno.h>                                       // errno, EINTR
#include <fcntl.h>                                       // fcntl(), F_SETFL, FNDELAY
#include <math.h>                                        // fmod(), llround(), log10(), pow()
#include <stdbool.h>                                     // bool, true, false
#include <stdio.h>                                       // a whole bunch of i/o functions
#include <stdlib.h>                                      // atoi(), calloc(), exit(), free(), malloc(), rand(), srand()
#include <string.h>                                      // a whole bunch of string functions
#include <strings.h>                                     // strcasecmp()
#include <sys/socket.h>                                  // This and arpa/inet - a whole plethora of socket related stuff
#include <time.h>                                        // ctime(), difftime(), localtime(), mktime(), time(), time_t
#include <unistd.h>                                      // close(), fsync(), read(), usleep()

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Macros
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Debugging
#define DEBUGIT(dl)             if (DEBUGIT_LVL >= dl) {sprintf(LogMsg,"*** %s ***",__FUNCTION__);LogIt(LogMsg);} // dl = debug level
#define DEBUGIT_LVL             1                        // Range of 0 to 5 where 0 = No debug messages and 5 = Maximum debug messages
// Configuration
#define BASE_MOB_XP             50                       // Base mob xp per level
#define BASE_PLAYER_XP          1000                     // Base player xp per level
#define BUFFER_LIMIT            2048                     // Max size of Buffer including '\0'
#define DAMAGE_PER_LEVEL        10                       // Maximum base damage per attacker level
#define MOB_HPT_PER_LEVEL       31                       // Mobile hit points per level
#define PORT                    3737                     // Port number
#define PLAYER_HPT_PER_LEVEL    31                       // Player hit points per level
#define PLAYER_RECOVERY_AMOUNT  1                        // Base hit points recovered per recovery event
#define SLEEP_TIME              100000                   // Sleep for a short period of time
#define STRING_LIMIT            1024                     // Max size of string including '\0'
#define USE_USLEEP              'N'                      // Use usleep() Y or N
// Directories
#define YAGS_DIR                "/mnt/c/Projects/YaGs"   // YaGs top level directory path
#define LIB_DIR                 "Library"                // Library directory
#define WORLD_DIR               "World"                  // World directory
#define PLAYER_EQU_DIR          "PlayerEqu"              // Player equipment directory
#define PLAYER_INV_DIR          "PlayerInv"              // Player inventory directory
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
#define SHOPS_FILE              "Shops.txt"              // Shops file
#define SPAWN_FILE              "Spawn.txt"              // Spawn file
#define PLAYER_FILE             "Player.yags"            // Player file
#define PLAYER_START_ROOM       120                      // Player start room
// Timer events
#define COMBAT_TICKS            20                       // Heartbeat ticks between combat rounds
#define HUNGER_THIRST_RATE      10                       // Hunger and thirst percentage lost per metabolism event
#define HUNGER_THIRST_TICKS     1000                     // Heartbeat ticks between metabolism events
#define NO_INPUT_TICK           500                      // Ticks before checking if player is still there
#define NO_INPUT_COUNT_LIMIT    3                        // Triggers player disconnect after this limit is hit
#define MOBILE_MOVE_CHANCE      25                       // Percent chance a movable mobile changes rooms
#define MOBILE_MOVE_TICKS       50                       // Heartbeat ticks between mobile movement checks
#define MOBILE_RESPAWN_TICKS    10                       // Heartbeat ticks between mobile respawn checks
#define PLAYER_AUTOSAVE_SECONDS 60                       // Seconds between dirty player saves
#define PLAYER_RECOVERY_TICKS   10                       // Heartbeat ticks between player recovery events

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
int                   CombatTick;                        // Heartbeat ticks since the last combat round
int                   CommandNbr;                        // Command number zero based
time_t                CurrentTime;                       // Current time for player age calculation
time_t                CurrentTimeSec;                    // Current time in seconds
time_t                NextPlayerAutosave;                // Time of the next dirty player save
int                   Days;                              // Player age in days
int                   Damage;                            // Damage inflicted by the current attack
int                   DestRoomNbr;                       // Room number player is moving into
int                   DirectionNbr;                      // The DirectionTable index of the direction
int                   DestroyCount;                      // Number of objects to destroy
double                ElapsedTime;                       // Elapsed player time
double                ExpAdditional;                     // Additional experience required for a player level
int                   ExpAward;                          // Experience awarded for killing a mobile
long long             ExpBase;                           // Base experience required for a player level
int                   ExpCalcLevel;                      // Player level used for experience calculation
int                   ExpLevelDiff;                      // Player and mobile level difference for experience calculation
int                   ExpPercent;                        // Percentage of base mobile experience awarded
long long             ExpRequired;                       // Total experience required for a player level
int                   Hours;                             // Player age in hours
int                   HitPercent;                        // Remaining hit point percentage
int                   HitPointsRecovered;                // Whole hit points recovered during a recovery event
int                   HungerThirstTick;                  // Heartbeat ticks since the last metabolism event
socklen_t             LingerSize;                        // Size of Linger stucture
int                   LineNbr;                           // Line number
int                   Listen;                            // Listening socket
int                   MaxSocket;                         // Maximum socket value
int                   MaxHitPoints;                      // Maximum hit points for the current combatant
int                   Minutes;                           // Player age in minutes
int                   MobileMoveRoomCount;               // Number of eligible rooms for mobile movement
int                   MobileMoveTick;                    // Heartbeat ticks since the last mobile movement check
int                   MobileRespawnTick;                 // Heartbeat ticks since the last mobile respawn check
long                  Offset;                            // Offset for fseek()
int                   OptVal;                            // Set socket option value
socklen_t             OptValSize;                        // Size of socket option value
int                   PlayerRcdNbr;                      // Player record number within Player.yags
int                   PlayerRecoveryTick;                // Heartbeat ticks since the last player recovery event
double                RecoveryRate;                      // Hit points accumulated per player recovery event
int                   ReturnValue1;                      // Return value
size_t                ReturnValue2;                      // Return value
long int              SendResult;                        // Number of bytes sent to player
int                   Seconds;                           // Player age in seconds
int                   Socket;                            // Socket value
socklen_t             SocketAddrSize;                    // Size of Socket structure
size_t                StrLen;                            // String length
int                   WearPositionNbr;                   // WearPositionTable index
int                   WeaponDamage;                      // Damage bonus from the wielded weapon
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
struct Room          *pMobileMoveRooms[10];              // Eligible destination rooms for mobile movement
struct tm            *pSpawnTime;                        // Pointer to calendar time used for respawn scheduling

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
char                  HealthPct[20];                     // Color-coded remaining health percentage
char                  LogMsg[100];                       // Log message
char                  MsgTxt[BUFFER_LIMIT];              // Message text
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
FILE                 *PlayerEquFile;                     // Player equipment file
FILE                 *PlayerFile;                        // Player file
FILE                 *PlayerInvFile;                     // Player inventory file
FILE                 *RoomFile;                          // Room file
FILE                 *ShopFile;                          // Shop file
FILE                 *SpawnFile;                         // Spawn file
FILE                 *ValidNamesFile;                    // Valid names file

// Structures
fd_set                InpSet;                            // File Descriptor Set structure
struct linger         Linger;                            // Linger structure
struct sockaddr_in    SocketAddr;                        // Socket Address structure
struct timeval        TimeOut;                           // Time value structure
struct tm             SpawnTime;                         // Calendar time used for respawn scheduling

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

typedef enum PlayerPositions
{
  Sleeping,
  Sitting,
  Standing
} PlayerPosition;

// Player and World structure typedefs
typedef struct Mobile         Mobile;
typedef struct MobileInstance MobileInstance;
typedef struct MobileList     MobileList;
typedef struct Object         Object;
typedef struct ObjectList     ObjectList;
typedef struct PlayerEquList  PlayerEquList;
typedef struct PlayerInvList  PlayerInvList;
typedef struct ConnList       ConnList;
typedef struct Player         Player;
typedef struct Room           Room;
typedef struct RoomList       RoomList;
typedef struct RoomObjectList RoomObjectList;
typedef struct Shop           Shop;
typedef struct ShopList       ShopList;
typedef struct ShopObjectList ShopObjectList;
typedef struct Spawn          Spawn;
typedef struct SpawnList      SpawnList;

// The ConnList struct represents a connected player session, including socket state,
// input and output buffers, player data, and connection list pointers.
struct ConnList
{
  int                 Socket;                            // Socket number returned from accept()
  PlayerState         State;                             // Player state
  char                Input[1024];                       // Player input buffer
  char                Output[2048];                      // Player output buffer
  int                 BadPswdCount;                      // Number of bad passwords entered
  int                 PlayerRcdNbr;                      // Player record number within Player.yags
  int                 NoInputTick;                       // Ticks before checking if player is still there
  int                 NoInputCount;                      // Number of no input ticks
  int                 HitPoints;                         // Current player hit points
  double              HitPointRecovery;                  // Fractional hit points accumulated during recovery
  PlayerPosition      Position;                          // Current player position
  char                Afk;                               // Away from keyboard flag (Y/N)
  bool                PlayerDirty;                       // Player record has unsaved changes
  Player             *pPlayer;                           // Pointer to the connected player data
  MobileInstance     *pFightingMobile;                   // Pointer to the mobile currently fighting the player
  PlayerEquList      *pPlayerEquHead;                    // Pointer to the head of the player equipment list
  PlayerEquList      *pPlayerEquTail;                    // Pointer to the tail of the player equipment list
  PlayerInvList      *pPlayerInvHead;                    // Pointer to the head of the player inventory list
  PlayerInvList      *pPlayerInvTail;                    // Pointer to the tail of the player inventory list
  ConnList           *pConnNext;                         // Pointer to next connection in the connection list
  ConnList           *pConnPrev;                         // Pointer to previous connection in the connection list
};

// The Player structure represents a player, encapsulating attributes such as
// name, password, status flags, creation time, color preference, coins, experience points, hunger, level, sex, and thirst.
struct Player
{
  char                Name[50];                          // Player name
  char                Password[50];                      // Player password
  char                Admin;                             // Admin flag (Y/N) - Controls which commands are available to the player
  time_t              Born;                              // Time player was created
  char                Color;                             // Color code (Y/N) Y means that player output is run through the Color() function
  int                 Coins;                             // Player coin balance
  long long           Experience;                        // Experience points
  int                 Hunger;                            // Player fullness percentage
  int                 Level;                             // Player level
  char                Sex;                               // Player sex (M/F)
  int                 Thirst;                            // Player hydration percentage
  int                 RoomNbr;                           // Room number
};

struct PlayerEquList
{
  Object             *pObject;                           // Pointer to an equipped Object struct
  char               *Slot;                              // Equipment slot occupied by the object
  PlayerEquList      *pNextPlayerEqu;                    // Pointer to the next equipment list node
};

struct PlayerInvList
{
  Object             *pObject;                           // Pointer to an inventory Object struct
  int                 Quantity;                          // Number of identical objects carried
  PlayerInvList      *pNextPlayerInv;                    // Pointer to the next inventory list node
};

PlayerEquList        *pPlayerEquList     = NULL;         // Pointer to found player equipment list node
PlayerInvList        *pPlayerInvList     = NULL;         // Pointer to found player inventory list node
PlayerEquList        *pPlayerEquListCurr = NULL;         // Pointer to the current player equipment list node
PlayerEquList        *pPlayerEquListNew  = NULL;         // Pointer to a new player equipment list node
PlayerEquList        *pPlayerEquListPrev = NULL;         // Pointer to the previous player equipment list node
PlayerInvList        *pPlayerInvListCurr = NULL;         // Pointer to the current player inventory list node
PlayerInvList        *pPlayerInvListNew  = NULL;         // Pointer to a new player inventory list node
PlayerInvList        *pPlayerInvListPrev = NULL;         // Pointer to the previous player inventory list node

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// World
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

struct Mobile
{
  char               *Id;                                // Unique mobile identifier
  char               *Desc1;                             // Short mobile description
  char               *Desc2;                             // Description shown in a room
  char               *Desc3;                             // Detailed mobile description
  char               *Flags;                             // Mobile behavior flags
  char               *Attack;                            // Mobile attack description
  int                 Level;                             // Mobile level
  int                 Hit;                               // Mobile hit point adjustment
  int                 Exp;                               // Experience award
  char               *Loot;                              // Space-separated object identifiers
};

struct MobileList
{
  Mobile             *pMobile;                           // Pointer to a Mobile struct
  MobileList         *pNextMobile;                       // Pointer to the next node in the list
};

struct MobileInstance
{
  Mobile             *pMobile;                           // Pointer to the permanent mobile definition
  Spawn              *pSpawn;                            // Pointer to the spawn rule that created the mobile
  Room               *pRoom;                             // Pointer to the mobile's current room
  int                 HitPoints;                         // Current mobile hit points
  ConnList           *pFightingPlayer;                   // Pointer to the player currently fighting the mobile
  MobileInstance     *pNextMobileInstance;               // Pointer to the next mobile in the world
  MobileInstance     *pNextRoomMobile;                   // Pointer to the next mobile in the room
};

MobileInstance        *pMobileInstance     = NULL;       // Pointer to a found mobile instance
MobileInstance        *pMobileInstanceCurr = NULL;       // Pointer to the current mobile instance
MobileInstance        *pMobileInstanceHead = NULL;       // Pointer to the head of the world mobile list
MobileInstance        *pMobileInstanceNew  = NULL;       // Pointer to a new mobile instance
MobileInstance        *pMobileInstanceNext = NULL;       // Pointer to the next mobile instance
MobileInstance        *pMobileInstancePrev = NULL;       // Pointer to the previous mobile instance
MobileInstance        *pMobileInstanceTail = NULL;       // Pointer to the tail of the world mobile list
MobileList            *pMobileListCurr     = NULL;       // Pointer to the current mobile list node
MobileList            *pMobileListHead     = NULL;       // Pointer to the head of the mobile list
MobileList            *pMobileListTail     = NULL;       // Pointer to the tail of the mobile list

struct Object
{
  char               *Id;                                // Unique object identifier
  char               *Desc1;                             // Short object description
  char               *Desc2;                             // Description shown in a room
  char               *Desc3;                             // Detailed object description
  int                 Weight;                            // Object weight
  int                 Cost;                              // Object purchase cost
  char               *Type;                              // General object type
  char               *Subtype;                           // Specific object type
  int                 Value;                             // Type-specific object value
};

struct ObjectList
{
  Object             *pObject;                           // Pointer to an Object struct
  ObjectList         *pNextObject;                       // Pointer to the next list node
};

Object                *pDestroyObject  = NULL;           // Pointer to the object being destroyed
Object                *pExamineObject  = NULL;           // Pointer to the object being examined
Object                *pGiveObject     = NULL;           // Pointer to the object being given
Object                *pLoadObject     = NULL;           // Pointer to the object being loaded
Object                *pObject         = NULL;           // Pointer to object
ObjectList            *pObjectListCurr = NULL;           // Pointer to the current object list node
ObjectList            *pObjectListHead = NULL;           // Pointer to the head of the object list
ObjectList            *pObjectListTail = NULL;           // Pointer to the tail of the object list

struct Room
{
  int                 RoomNbr;                           // Room number (e.g., 101)
  char               *Name;                              // Room name (e.g., "Back Porch")
  char               *Desc;                              // Room description (multi-line text)
  char               *Terrain;                           // Terrain type (e.g., "Concrete", "Indoor")
  char               *Flags;                             // Flags (e.g., "None", "NoFight")
  char               *Exits;                             // Exits as a single string (e.g., "xxxxx xxxxx 00106 xxxxx xxxxx")
  MobileInstance     *pMobileInstanceHead;               // Pointer to the head of the room mobile list
  MobileInstance     *pMobileInstanceTail;               // Pointer to the tail of the room mobile list
  RoomObjectList     *pRoomObjectHead;                   // Pointer to the head of the room object list
  RoomObjectList     *pRoomObjectTail;                   // Pointer to the tail of the room object list
};

struct RoomList
{
  Room               *pRoom;                             // Pointer to a Room struct
  RoomList           *pNextRoom;                         // Pointer to the next node in the list
};

struct RoomObjectList
{
  Object             *pObject;                        // Pointer to an object on the ground
  int                 Quantity;                       // Number of identical objects on the ground
  RoomObjectList     *pNextRoomObject;                // Pointer to the next room object list node
};

struct Shop
{
  Room               *pRoom;                             // Pointer to the room containing the shop
  char               *Message;                           // Message shown when viewing the shop room
  ShopObjectList     *pShopObjectHead;                   // Pointer to the head of the shop object list
  ShopObjectList     *pShopObjectTail;                   // Pointer to the tail of the shop object list
};

struct ShopList
{
  Shop               *pShop;                             // Pointer to a Shop struct
  ShopList           *pNextShop;                         // Pointer to the next shop list node
};

struct ShopObjectList
{
  Object             *pObject;                           // Pointer to an object sold by the shop
  ShopObjectList     *pNextShopObject;                   // Pointer to the next shop object list node
};

struct Spawn
{
  Mobile             *pMobile;                           // Pointer to the mobile definition
  int                 MaxInWorld;                        // Maximum number of this mobile in the world
  int                 CurrentInWorld;                    // Current number of this mobile in the world
  bool                RespawnPending;                    // A replacement mobile is waiting to respawn
  time_t              NextSpawnTime;                     // Real time when the next mobile may respawn
  Room               *pRoom;                             // Pointer to the room where the mobile spawns
  int                 Seconds;                           // Respawn interval seconds
  int                 Minutes;                           // Respawn interval minutes
  int                 Hours;                             // Respawn interval hours
  int                 Days;                              // Respawn interval days
  int                 Weeks;                             // Respawn interval weeks
  int                 Months;                            // Respawn interval months
  int                 Years;                             // Respawn interval years
};

struct SpawnList
{
  Spawn              *pSpawn;                            // Pointer to a Spawn struct
  SpawnList          *pNextSpawn;                        // Pointer to the next spawn list node
};

Mobile               *pMobile;                           // Pointer to mobile
Player                PlayerRcd;                         // Player record used for player file reads
Room                  SingleRoom;                        // Temporary room read from Rooms.txt
Room                 *pCurrentRoom;                      // Pointer to player's current room during movement
Room                 *pDestinationRoom;                  // Pointer to player's destination room during movement
Room                 *pNewRoom;                          // Pointer to newly allocated room
Room                 *pRoom;                             // Pointer to room found by RoomLookUp()
Room                 *pMobileMoveRoom;                   // Pointer to eligible or selected mobile destination room
RoomList             *pNewRoomListNode;                  // Pointer to newly allocated room list node
RoomList             *pRoomListCurr        = NULL;       // Pointer to current room list node
RoomList             *pRoomListHead        = NULL;       // Pointer to head of room list
RoomList             *pRoomListNext        = NULL;       // Pointer to next room list node
RoomList             *pRoomListTail        = NULL;       // Pointer to tail of room list
RoomObjectList       *pRoomObjectList      = NULL;       // Pointer to found room object list node
RoomObjectList       *pRoomObjectListCurr  = NULL;       // Pointer to the current room object list node
RoomObjectList       *pRoomObjectListNew   = NULL;       // Pointer to a new room object list node
RoomObjectList       *pRoomObjectListNext  = NULL;       // Pointer to the next room object list node
RoomObjectList       *pRoomObjectListPrev  = NULL;       // Pointer to the previous room object list node
Shop                 *pShop                = NULL;       // Pointer to the current shop
ShopList             *pShopListCurr        = NULL;       // Pointer to the current shop list node
ShopList             *pShopListHead        = NULL;       // Pointer to the head of the shop list
ShopList             *pShopListTail        = NULL;       // Pointer to the tail of the shop list
ShopObjectList       *pShopObjectList      = NULL;       // Pointer to found shop object list node
ShopObjectList       *pShopObjectListCurr  = NULL;       // Pointer to the current shop object list node
Spawn                *pSpawn               = NULL;       // Pointer to the current spawn definition
SpawnList            *pSpawnListCurr       = NULL;       // Pointer to the current spawn list node
SpawnList            *pSpawnListHead       = NULL;       // Pointer to the head of the spawn list
SpawnList            *pSpawnListTail       = NULL;       // Pointer to the tail of the spawn list

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Functions
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

void           AbortIt();
void           AddPlayerToFile();
void           AddToConnList();
void           CalcHealthPct(int HitPoints, int HitPointsMax);
void           SocketCheckForNewPlayers();
void           CloseLog();
void           Color();
void           Combat();
void           CombatPlayerDeath();
void           CombatRound();
void           CombatStop();
void           CombatVictory();
void           DelFromConnList();
int            DirectionLookUp(char *Direction);
void           DoAdvance();
void           DoAfk();
void           DoBorn();
void           DoBuy();
void           DoChat();
void           DoColor();
void           DoDestroy();
void           DoDrink();
void           DoDrop();
void           DoEat();
void           DoEquipment();
void           DoExamine();
void           DoGet();
void           DoGive();
void           DoGo();
void           DoGoto();
void           DoHelp();
void           DoInventory();
void           DoKill();
void           DoList();
void           DoLoad();
void           DoLook();
void           DoMoney();
void           DoPlayerfile();
void           DoQuit();
void           DoRemove();
void           DoRestore();
void           DoSay();
void           DoSell();
void           DoShutdown();
void           DoSit();
void           DoSleep();
void           DoStand();
void           DoStatus();
void           DoTell();
void           DoTime();
void           DoWake();
void           DoWear();
void           DoWield();
void           DoWho();
bool           Equal(char *Str1, char *Str2);
void           GetNextPlayerRcdNbr();
long           GetPlayerFileOffset();
void           GetPlayerOnline();
void           GetTime();
void           HeartBeat();
void           InitalizeNewPlayer();
void           Initialization();
bool           IdMatch(char *PartialId, char *FullId);
void           LogIt(char *LogMsg);
void           LowerCase(char *Str);
void           MobileInstanceAdd();
void           MobileInstanceFreeList();
void           MobileInstanceLookUp(char *Id);
void           MobileInstanceMove();
void           MobileInstanceRemove();
void           MobileAttackVerb();
void           MobileExpCalc();
void           MobileLookUp(char *Id);
void           MobileMove();
void           MobileReadFile();
void           MobileRespawn();
void           NormalizePlayerName(char *Name);
bool           MudCmdOk();
void           ObjectLookUp(char *Id);
void           ObjectReadFile();
void           OpenLog();
void           PlayerAutoSave();
void           PlayerAttackVerb();
void           PlayerCloseFile();
void           PlayerEquAdd(Object *pObject, char *Slot);
void           PlayerEquLookUp(char *Id);
void           PlayerEquReadFile();
void           PlayerEquRemove();
void           PlayerEquSlotLookUp(char *Slot);
void           PlayerEquWriteFile();
void           PlayerExpCalc();
void           PlayerHungerThirst();
void           PlayerInvAdd(Object *pObject);
void           PlayerInvLookUp(char *Id);
void           PlayerInvReadFile();
void           PlayerInvRemoveOne();
void           PlayerInvWriteFile();
void           PlayerLevelUp();
bool           PlayerNameValid();
bool           PlayerNameValidNew();
bool           PlayerNameValidOld();
void           PlayerOpenFile();
void           PlayerReadFile();
void           PlayerRecoverHitPoints();
void           PlayerWriteFile();
void           ProcessCommandAlias();
void           ProcessCommand();
void           ProcessPlayerInput();
void           Prompt(ConnList *pConn);
void           RoomAddToRoomList();
void           RoomAllocateAndCopy(const Room *SourceRoom);
void           RoomFreeList();
char          *RoomGetExits(const Room *pRoom);
void           RoomLookUp(int RoomNbr);
void           RoomObjectAdd(Object *pObject);
void           RoomObjectLookUp(char *Id);
void           RoomObjectRemoveOne();
void           RoomReadFile();
void           SendGreeting();
void           SendMotd();
void           SendToAll();
void           SendToRoom(int RoomNbr, ConnList *pExclude);
void           ShopLookUp(int RoomNbr);
void           ShopObjectLookUp(char *Id);
void           ShopReadFile();
void           ShutItDown();
void           Sleep();
void           SocketAcceptNewPlayer();
void           SocketGetPlayerInput();
void           SocketDisconnectPlayers();
void           SocketListen();
void           SocketSendPlayerOutput();
void           SpawnMobiles();
void           SpawnReadFile();
void           SpawnScheduleNext();
void           StartItUp();
void           StrAppend(char *Str1, char *Str2);
void           Trim(char *Str);
void           TrimLeft(char *Str);
void           TrimRight(char *Str);
void           Up1stChar(char *Str);
void           ValidateCommandTable();
int            WearPositionLookUp(char *Subtype);
void           Word(size_t Nbr, char *Str1, char *Str2);
size_t         Words(char *Str);
//void           zTestStuff();

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Commands
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Commands is a structure that holds various character pointers representing different attributes
// related to commands, such as name, admin status, level, position, social interactions,
// fight commands, words, parts, and messages.
struct sCommands
{
  char               *Name;                              // Command name
  char               *Admin;                             // This is an admin command
  char               *Level;                             // Player must be at this level to use the command
  char               *Position;                          // Player must be, at least, in this position to use the command
  char               *Social;                            // Is this a social command
  char               *Fight;                             // Can this command be issued during a fight
  char               *MinWords;                          // Minimum number of words in the command
  char               *MaxWords;                          // Maximum number of words in the command
  char               *Message;                           // Message to display if command is invalid
} Commands;

// Command aliases provide abbreviated forms of commands.
struct sCommandAlias
{
  char               *Alias;                             // Command alias
  char               *Command;                           // Full command name
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
  char               *ShortName;                         // Abbreviated direction
  char               *LongName;                          // Full direction name
  char               *DisplayName;                       // Direction shown in room exits
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

// Equipment slots and their player-facing display labels.
struct sEquipmentSlot
{
  char               *Slot;                              // Equipment slot name
  char               *Label;                             // Label shown by equipment command
};

struct sEquipmentSlot EquipmentSlotTable[] =
{
  {"Head",        "Worn on head:"},
  {"LeftEar",     "Worn on left ear:"},
  {"RightEar",    "Worn on right ear:"},
  {"Neck",        "Worn around neck:"},
  {"Shoulders",   "Worn on shoulders:"},
  {"Chest",       "Worn on chest:"},
  {"Back",        "Worn on back:"},
  {"Arms",        "Worn on arms:"},
  {"LeftWrist",   "Worn on left wrist:"},
  {"RightWrist",  "Worn on right wrist:"},
  {"Hands",       "Worn on hands:"},
  {"LeftFinger",  "Worn on left finger:"},
  {"RightFinger", "Worn on right finger:"},
  {"Shield",      "Shield held:"},
  {"Waist",       "Worn around waist:"},
  {"Legs",        "Worn on legs:"},
  {"LeftAnkle",   "Worn on left ankle:"},
  {"RightAnkle",  "Worn on right ankle:"},
  {"Feet",        "Worn on feet:"},
  {"Wielded",     "Weapon wielded:"},
  {NULL,           NULL}
};

// Armor subtypes map to one or two actual equipment slots.
struct sWearPosition
{
  char               *Subtype;                           // Armor subtype from Objects.txt
  char               *Slot1;                             // First equipment slot
  char               *Slot2;                             // Optional second equipment slot
};

struct sWearPosition WearPositionTable[] =
{
  {"Head",      "Head",        NULL},
  {"Ear",       "LeftEar",     "RightEar"},
  {"Neck",      "Neck",        NULL},
  {"Shoulders", "Shoulders",   NULL},
  {"Chest",     "Chest",       NULL},
  {"Back",      "Back",        NULL},
  {"Arms",      "Arms",        NULL},
  {"Wrist",     "LeftWrist",   "RightWrist"},
  {"Hands",     "Hands",       NULL},
  {"Finger",    "LeftFinger",  "RightFinger"},
  {"Shield",    "Shield",      NULL},
  {"Waist",     "Waist",       NULL},
  {"Legs",      "Legs",        NULL},
  {"Ankle",     "LeftAnkle",   "RightAnkle"},
  {"Feet",      "Feet",        NULL},
  {NULL,        NULL,          NULL}
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
    {"afk",        "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"born",       "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"buy",        "N",  "1",  "sit",    "N",   "N",  "2",  "2",  "Buy what?"},
    {"chat",       "N",  "1",  "sleep",  "N",   "Y",  "2",  "999", "Chat what?"},
    {"color",      "N",  "1",  "sleep",  "N",   "N",  "1",  "2",  "None"},
    {"destroy",    "N",  "1",  "sit",    "N",   "N",  "2",  "3",  "Destroy what?"},
    {"drink",      "N",  "1",  "sit",    "N",   "N",  "2",  "2",  "Drink what?"},
    {"drop",        "N",  "1",  "sit",    "N",   "N",  "2",  "2",  "Drop what?"},
    {"eat",        "N",  "1",  "sit",    "N",   "N",  "2",  "2",  "Eat what?"},
    {"equipment",  "N",  "1",  "sit",    "N",   "N",  "1",  "1",  "None"},
    {"examine",    "N",  "1",  "sit",    "N",   "N",  "2",  "2",  "Examine what?"},
    {"get",         "N",  "1",  "sit",    "N",   "N",  "2",  "2",  "Get what?"},
    {"give",        "N",  "1",  "sit",    "N",   "N",  "3",  "3",  "Give what to whom?"},
    {"go",         "N",  "1",  "stand",  "N",   "N",  "2",  "2",  "Go where?"},
    {"goto",       "Y",  "1",  "sleep",  "N",   "N",  "2",  "2",  "Goto which room?"},
    {"help",       "N",  "1",  "sleep",  "N",   "N",  "1",  "2",  "None"},
    {"inventory",  "N",  "1",  "sit",    "N",   "N",  "1",  "1",  "None"},
    {"kill",       "N",  "1",  "stand",  "N",   "Y",  "2",  "2",  "Kill what?"},
    {"list",       "N",  "1",  "sit",    "N",   "N",  "1",  "1",  "None"},
    {"load",       "Y",  "1",  "sleep",  "N",   "N",  "2",  "2",  "Load what?"},
    {"look",       "N",  "1",  "sit",    "N",   "N",  "1",  "1",  "None"},
    {"money",      "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"playerfile", "Y",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"quit",       "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"remove",     "N",  "1",  "sit",    "N",   "N",  "2",  "2",  "Remove what?"},
    {"restore",    "Y",  "1",  "sleep",  "N",   "N",  "2",  "2",  "Restore whom?"},
    {"say",        "N",  "1",  "sit",    "N",   "Y",  "2",  "999", "Say what?"},
    {"sell",       "N",  "1",  "sit",    "N",   "N",  "2",  "2",  "Sell what?"},
    {"shutdown",   "Y",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"sit",        "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"sleep",      "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"stand",      "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"status",     "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"tell",       "N",  "1",  "sleep",  "N",   "Y",  "3",  "999", "Tell whom what?"},
    {"time",       "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"wake",       "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {"wear",       "N",  "1",  "sit",    "N",   "N",  "2",  "2",  "Wear what?"},
    {"wield",      "N",  "1",  "sit",    "N",   "N",  "2",  "2",  "Wield what?"},
    {"who",        "N",  "1",  "sleep",  "N",   "N",  "1",  "1",  "None"},
    {NULL,         NULL, NULL, NULL,     NULL,  NULL, NULL, NULL, NULL}
};

// DoCommand is an array of function pointers, each pointing to a function that takes no parameters
// and returns void, allowing for the execution of various commands such as DoAdvance, DoColor, and others.
void (*DoCommand[])(void) =
{ // This list and the CommandTable MUST BE in the same order
  DoAdvance,
  DoAfk,
  DoBorn,
  DoBuy,
  DoChat,
  DoColor,
  DoDestroy,
  DoDrink,
  DoDrop,
  DoEat,
  DoEquipment,
  DoExamine,
  DoGet,
  DoGive,
  DoGo,
  DoGoto,
  DoHelp,
  DoInventory,
  DoKill,
  DoList,
  DoLoad,
  DoLook,
  DoMoney,
  DoPlayerfile,
  DoQuit,
  DoRemove,
  DoRestore,
  DoSay,
  DoSell,
  DoShutdown,
  DoSit,
  DoSleep,
  DoStand,
  DoStatus,
  DoTell,
  DoTime,
  DoWake,
  DoWear,
  DoWield,
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
  CombatTick++;
  if (CombatTick >= COMBAT_TICKS)
  {
    CombatTick = 0;
    Combat();
  }
  HungerThirstTick++;
  if (HungerThirstTick >= HUNGER_THIRST_TICKS)
  {
    HungerThirstTick = 0;
    PlayerHungerThirst();
  }
  PlayerRecoveryTick++;
  if (PlayerRecoveryTick >= PLAYER_RECOVERY_TICKS)
  {
    PlayerRecoveryTick = 0;
    PlayerRecoverHitPoints();
  }
  MobileMoveTick++;
  if (MobileMoveTick >= MOBILE_MOVE_TICKS)
  {
    MobileMoveTick = 0;
    MobileMove();
  }
  MobileRespawnTick++;
  if (MobileRespawnTick >= MOBILE_RESPAWN_TICKS)
  {
    MobileRespawnTick = 0;
    MobileRespawn();
  }
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
      PlayerWriteFile();
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

// Return the index of an armor subtype in WearPositionTable.
int WearPositionLookUp(char *Subtype)
{
  DEBUGIT(1)
  i = 0;
  while (WearPositionTable[i].Subtype != NULL)
  {
    if (Equal(Subtype, WearPositionTable[i].Subtype))
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
      if (pConn->pFightingMobile != NULL && !Equal(Commands.Fight, "Y"))
      {
        strcat(pConn->Output, "You can't do that while fighting.\r\n\r\n");
        Prompt(pConn);
        return false;
      }
      if (Equal(Commands.Position, "stand") && pConn->Position != Standing)
      {
        strcat(pConn->Output, "You must be standing to do that.\r\n\r\n");
        Prompt(pConn);
        return false;
      }
      if (Equal(Commands.Position, "sit") && pConn->Position == Sleeping)
      {
        strcat(pConn->Output, "You must be sitting or standing to do that.\r\n\r\n");
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
  if (atoi(CmdParm2) < 1)
  { // Level must be greater than zero
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
  pTarget->pPlayer->Level      = atoi(CmdParm2);
  pTarget->HitPoints           = pTarget->pPlayer->Level * PLAYER_HPT_PER_LEVEL;
  ExpCalcLevel = pTarget->pPlayer->Level;
  PlayerExpCalc();
  pTarget->pPlayer->Experience = ExpRequired;
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
  PlayerWriteFile();
  pConn = pActor;
}

// Toggle the current connection's away from keyboard flag.
void DoAfk()
{
  DEBUGIT(1)
  if (pConn->Afk == 'Y')
  {
    pConn->Afk = 'N';
    strcat(pConn->Output, "You are no longer AFK.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  pConn->Afk = 'Y';
  strcat(pConn->Output, "You are now AFK.\r\n\r\n");
  Prompt(pConn);
}

// Display the current player's birthdate and age.
void DoBorn()
{
  DEBUGIT(1)
  CurrentTime = time(NULL);
  ElapsedTime = difftime(CurrentTime, pConn->pPlayer->Born);
  Days        = (int)(ElapsedTime / (24 * 3600));
  ElapsedTime = fmod(ElapsedTime, (24 * 3600));
  Hours       = (int)(ElapsedTime / 3600);
  ElapsedTime = fmod(ElapsedTime, 3600);
  Minutes     = (int)(ElapsedTime / 60);
  Seconds     = (int)fmod(ElapsedTime, 60);
  strcpy(TmpStr, ctime(&pConn->pPlayer->Born));
  TrimRight(TmpStr);
  sprintf(Buffer, "Your birthdate is: %s\r\nYour age is: %d days, %d hours, %d minutes, %d seconds\r\n\r\n", TmpStr, Days, Hours, Minutes, Seconds);
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
}

// Buy one object from the shop in the player's current room.
void DoBuy()
{
  DEBUGIT(1)
  ShopLookUp(pConn->pPlayer->RoomNbr);
  if (pShop == NULL)
  {
    strcat(pConn->Output, "Find a shop.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  Word(2, Command, CmdParm1);
  ShopObjectLookUp(CmdParm1);
  if (pShopObjectList == NULL)
  {
    strcat(pConn->Output, "That item is not for sale.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  if (pConn->pPlayer->Coins < pShopObjectList->pObject->Cost)
  {
    strcat(pConn->Output, "You don't have enough coins.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  sprintf(Buffer, "You buy %s for %d coins.\r\n\r\n", pShopObjectList->pObject->Desc1, pShopObjectList->pObject->Cost);
  pConn->pPlayer->Coins -= pShopObjectList->pObject->Cost;
  PlayerInvAdd(pShopObjectList->pObject);
  PlayerWriteFile();
  PlayerInvWriteFile();
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
}

// Send an out-of-character message to every online player.
void DoChat()
{
  DEBUGIT(1)
  Parameters = strchr(Command, ' ');
  Parameters++;
  Trim(Parameters);
  pActor       = pConn;
  pConnCurrSave = pConnCurr;
  pConnCurr     = pConnHead;
  while (pConnCurr != NULL)
  {
    if (pConnCurr != pActor && pConnCurr->State == Online)
    {
      pConn = pConnCurr;
      sprintf(Buffer, "\r\n&W%s chats, \"%s\"&N\r\n\r\n", pActor->pPlayer->Name, Parameters);
      strcat(pConn->Output, Buffer);
      Prompt(pConn);
    }
    pConnCurr = pConnCurr->pConnNext;
  }
  pConn     = pActor;
  pConnCurr = pConnCurrSave;
  sprintf(Buffer, "&WYou chat, \"%s\"&N\r\n\r\n", Parameters);
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
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
  PlayerWriteFile();
}

// Permanently destroy one or more objects from the player's inventory.
void DoDestroy()
{
  DEBUGIT(1)
  Word(2, Command, CmdParm1);
  PlayerInvLookUp(CmdParm1);
  if (pPlayerInvList == NULL)
  {
    strcat(pConn->Output, "You don't have that.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  pDestroyObject = pPlayerInvList->pObject;
  DestroyCount = 1;
  if (Words(Command) == 3)
  {
    Word(3, Command, CmdParm2);
    LowerCase(CmdParm2);
    if (Equal(CmdParm2, "all"))
    {
      DestroyCount = pPlayerInvList->Quantity;
    }
    else
    {
      DestroyCount = atoi(CmdParm2);
    }
  }
  if (DestroyCount < 1)
  {
    strcat(pConn->Output, "Destroy count must be greater than zero.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  if (DestroyCount > pPlayerInvList->Quantity)
  {
    strcat(pConn->Output, "You don't have that many.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  if (DestroyCount == 1)
  {
    sprintf(Buffer, "You destroy %s.\r\n\r\n", pDestroyObject->Desc1);
  }
  else
  {
    sprintf(Buffer, "You destroy %d of %s.\r\n\r\n", DestroyCount, pDestroyObject->Desc1);
  }
  for (i = 0; i < (size_t)DestroyCount; i++)
  {
    PlayerInvRemoveOne();
  }
  PlayerInvWriteFile();
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
}

// Drink one drink object from the player's inventory.
void DoDrink()
{
  DEBUGIT(1)
  Word(2, Command, CmdParm1);
  PlayerInvLookUp(CmdParm1);
  if (pPlayerInvList == NULL)
  {
    strcat(pConn->Output, "You don't have that.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  if (!Equal(pPlayerInvList->pObject->Type, "Drink"))
  {
    strcat(pConn->Output, "You can't drink that.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  if (pConn->pPlayer->Thirst >= 100)
  {
    strcat(pConn->Output, "You can't take another sip.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  sprintf(Buffer, "You drink %s.\r\n", pPlayerInvList->pObject->Desc1);
  strcat(pConn->Output, Buffer);
  pConn->pPlayer->Thirst += pPlayerInvList->pObject->Value;
  if (pConn->pPlayer->Thirst > 100)
  {
    pConn->pPlayer->Thirst = 100;
  }
  PlayerInvRemoveOne();
  PlayerInvWriteFile();
  PlayerWriteFile();
  if (pConn->pPlayer->Thirst >= 100)
  {
    strcat(pConn->Output, "You are no longer thirsty, not even a little bit.\r\n\r\n");
  }
  else if (pConn->pPlayer->Thirst > 80)
  {
    strcat(pConn->Output, "You are a little bit thirsty.\r\n\r\n");
  }
  else if (pConn->pPlayer->Thirst > 60)
  {
    strcat(pConn->Output, "You need some lip balm.\r\n\r\n");
  }
  else if (pConn->pPlayer->Thirst > 40)
  {
    strcat(pConn->Output, "You are thirsty.\r\n\r\n");
  }
  else if (pConn->pPlayer->Thirst > 20)
  {
    strcat(pConn->Output, "Your throat is parched!\r\n\r\n");
  }
  else
  {
    strcat(pConn->Output, "You are extremely thirsty!!!\r\n\r\n");
  }
  Prompt(pConn);
}

// Drop one inventory object on the ground in the current room.
void DoDrop()
{
  DEBUGIT(1)
  Word(2, Command, CmdParm1);
  PlayerInvLookUp(CmdParm1);
  if (pPlayerInvList == NULL)
  {
    strcat(pConn->Output, "You don't have that.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  RoomLookUp(pConn->pPlayer->RoomNbr);
  sprintf(Buffer, "You drop %s.\r\n\r\n", pPlayerInvList->pObject->Desc1);
  RoomObjectAdd(pPlayerInvList->pObject);
  PlayerInvRemoveOne();
  PlayerInvWriteFile();
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
}

// Eat one food object from the player's inventory.
void DoEat()
{
  DEBUGIT(1)
  Word(2, Command, CmdParm1);
  PlayerInvLookUp(CmdParm1);
  if (pPlayerInvList == NULL)
  {
    strcat(pConn->Output, "You don't have that.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  if (!Equal(pPlayerInvList->pObject->Type, "Food"))
  {
    strcat(pConn->Output, "You can't eat that.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  if (pConn->pPlayer->Hunger >= 100)
  {
    strcat(pConn->Output, "You can't take another bite.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  sprintf(Buffer, "You eat %s.\r\n", pPlayerInvList->pObject->Desc1);
  strcat(pConn->Output, Buffer);
  pConn->pPlayer->Hunger += pPlayerInvList->pObject->Value;
  if (pConn->pPlayer->Hunger > 100)
  {
    pConn->pPlayer->Hunger = 100;
  }
  PlayerInvRemoveOne();
  PlayerInvWriteFile();
  PlayerWriteFile();
  if (pConn->pPlayer->Hunger >= 100)
  {
    strcat(pConn->Output, "You are no longer hungry, not even a little bit.\r\n\r\n");
  }
  else if (pConn->pPlayer->Hunger > 80)
  {
    strcat(pConn->Output, "You are a little bit hungry.\r\n\r\n");
  }
  else if (pConn->pPlayer->Hunger > 60)
  {
    strcat(pConn->Output, "Your stomach is growling.\r\n\r\n");
  }
  else if (pConn->pPlayer->Hunger > 40)
  {
    strcat(pConn->Output, "You are hungry.\r\n\r\n");
  }
  else if (pConn->pPlayer->Hunger > 20)
  {
    strcat(pConn->Output, "You could eat a horse!\r\n\r\n");
  }
  else
  {
    strcat(pConn->Output, "You are extremely hungry!!!\r\n\r\n");
  }
  Prompt(pConn);
}

// Display the player's equipment.
void DoEquipment()
{
  DEBUGIT(1)
  strcat(pConn->Output, "\r\nEquipment\r\n---------\r\n");
  if (pConn->pPlayerEquHead == NULL)
  {
    strcat(pConn->Output, "You have absolutely no equipment!\r\n");
  }
  else
  {
    i = 0;
    while (EquipmentSlotTable[i].Slot != NULL)
    {
      PlayerEquSlotLookUp(EquipmentSlotTable[i].Slot);
      pPlayerEquListCurr = pPlayerEquList;
      if (pPlayerEquListCurr != NULL)
      {
        sprintf(Buffer, "%-25s%s\r\n", EquipmentSlotTable[i].Label, pPlayerEquListCurr->pObject->Desc1);
        strcat(pConn->Output, Buffer);
      }
      i++;
    }
  }
  strcat(pConn->Output, "\r\n");
  Prompt(pConn);
}

// Display the detailed description of a visible object or mobile.
void DoExamine()
{
  DEBUGIT(1)
  Word(2, Command, CmdParm1);
  pExamineObject = NULL;
  pMobileInstance = NULL;
  PlayerInvLookUp(CmdParm1);
  if (pPlayerInvList != NULL)
  {
    pExamineObject = pPlayerInvList->pObject;
  }
  if (pExamineObject == NULL)
  {
    PlayerEquLookUp(CmdParm1);
    if (pPlayerEquList != NULL)
    {
      pExamineObject = pPlayerEquList->pObject;
    }
  }
  if (pExamineObject == NULL)
  {
    RoomLookUp(pConn->pPlayer->RoomNbr);
    RoomObjectLookUp(CmdParm1);
    if (pRoomObjectList != NULL)
    {
      pExamineObject = pRoomObjectList->pObject;
    }
  }
  if (pExamineObject == NULL)
  {
    ShopLookUp(pConn->pPlayer->RoomNbr);
    if (pShop != NULL)
    {
      ShopObjectLookUp(CmdParm1);
      if (pShopObjectList != NULL)
      {
        pExamineObject = pShopObjectList->pObject;
      }
    }
  }
  if (pExamineObject == NULL)
  {
    MobileInstanceLookUp(CmdParm1);
  }
  if (pExamineObject == NULL && pMobileInstance == NULL)
  {
    strcat(pConn->Output, "You don't see that here.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  strcat(pConn->Output, "\r\n");
  if (pExamineObject != NULL)
  {
    strcat(pConn->Output, pExamineObject->Desc3);
  }
  else
  {
    strcat(pConn->Output, pMobileInstance->pMobile->Desc3);
  }
  strcat(pConn->Output, "\r\n");
  Prompt(pConn);
}

// Get one object from the ground in the current room.
void DoGet()
{
  DEBUGIT(1)
  Word(2, Command, CmdParm1);
  RoomLookUp(pConn->pPlayer->RoomNbr);
  RoomObjectLookUp(CmdParm1);
  if (pRoomObjectList == NULL)
  {
    strcat(pConn->Output, "You don't see that here.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  sprintf(Buffer, "You get %s.\r\n\r\n", pRoomObjectList->pObject->Desc1);
  PlayerInvAdd(pRoomObjectList->pObject);
  RoomObjectRemoveOne();
  PlayerInvWriteFile();
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
}

// Give one inventory object to an online player in the same room.
void DoGive()
{
  DEBUGIT(1)
  Word(2, Command, CmdParm1);
  PlayerInvLookUp(CmdParm1);
  if (pPlayerInvList == NULL)
  {
    strcat(pConn->Output, "You don't have that.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  pGiveObject   = pPlayerInvList->pObject;
  pTarget       = NULL;
  pConnCurrSave = pConnCurr;
  pConnCurr     = pConnHead;
  Word(3, Command, CmdParm2);
  NormalizePlayerName(CmdParm2);
  while (pConnCurr != NULL)
  {
    if (pConnCurr->State == Online && Equal(pConnCurr->pPlayer->Name, CmdParm2))
    {
      pTarget = pConnCurr;
      break;
    }
    pConnCurr = pConnCurr->pConnNext;
  }
  pConnCurr = pConnCurrSave;
  if (pTarget == NULL)
  {
    sprintf(Buffer, "%s is not online.\r\n\r\n", CmdParm2);
    strcat(pConn->Output, Buffer);
    Prompt(pConn);
    return;
  }
  if (pTarget == pConn)
  {
    strcat(pConn->Output, "You can't give something to yourself.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  if (pTarget->pPlayer->RoomNbr != pConn->pPlayer->RoomNbr)
  {
    sprintf(Buffer, "%s is not here.\r\n\r\n", pTarget->pPlayer->Name);
    strcat(pConn->Output, Buffer);
    Prompt(pConn);
    return;
  }
  pActor = pConn;
  PlayerInvRemoveOne();
  PlayerInvWriteFile();
  pConn = pTarget;
  PlayerInvAdd(pGiveObject);
  PlayerInvWriteFile();
  sprintf(Buffer, "\r\n%s gives you %s.\r\n\r\n", pActor->pPlayer->Name, pGiveObject->Desc1);
  strcat(pTarget->Output, Buffer);
  Prompt(pTarget);
  pConn = pActor;
  sprintf(Buffer, "You give %s to %s.\r\n\r\n", pGiveObject->Desc1, pTarget->pPlayer->Name);
  strcat(pActor->Output, Buffer);
  Prompt(pActor);
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
  RoomLookUp(pConn->pPlayer->RoomNbr);
  pCurrentRoom = pRoom;
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
  RoomLookUp(DestRoomNbr);
  pDestinationRoom = pRoom;
  if (pDestinationRoom == NULL)
  {
    sprintf(LogMsg, "ERROR: Room %d exit points to missing room %d", pConn->pPlayer->RoomNbr, DestRoomNbr);
    LogIt(LogMsg);
    strcat(pConn->Output, "You go nowhere\r\n\r\n");
    Prompt(pConn);
    return;
  }
  sprintf(MsgTxt, "%s leaves.\r\n", pConn->pPlayer->Name);
  SendToRoom(pConn->pPlayer->RoomNbr, pConn);
  pConn->pPlayer->RoomNbr = DestRoomNbr;
  sprintf(MsgTxt, "%s arrives.\r\n\r\n", pConn->pPlayer->Name);
  SendToRoom(pConn->pPlayer->RoomNbr, pConn);
  pConn->PlayerDirty = true;
  sprintf(Buffer, "You go %s\r\n", DirectionTable[DirectionNbr].LongName);
  strcat(pConn->Output, Buffer);
  DoLook();
}

// Magically transport an admin to a room and display the destination.
void DoGoto()
{
  DEBUGIT(1)
  Word(2, Command, CmdParm1);
  for (i = 0; CmdParm1[i] != '\0'; i++)
  {
    if (!isdigit((unsigned char)CmdParm1[i]))
    {
      sprintf(Buffer, "Room %s does not exist.\r\n\r\n", CmdParm1);
      strcat(pConn->Output, Buffer);
      Prompt(pConn);
      return;
    }
  }
  DestRoomNbr = atoi(CmdParm1);
  RoomLookUp(DestRoomNbr);
  pDestinationRoom = pRoom;
  if (pDestinationRoom == NULL)
  {
    sprintf(Buffer, "Room %s does not exist.\r\n\r\n", CmdParm1);
    strcat(pConn->Output, Buffer);
    Prompt(pConn);
    return;
  }
  sprintf(MsgTxt, "%s disappears.\r\n\r\n", pConn->pPlayer->Name);
  SendToRoom(pConn->pPlayer->RoomNbr, pConn);
  pConn->pPlayer->RoomNbr = DestRoomNbr;
  sprintf(MsgTxt, "%s appears.\r\n\r\n", pConn->pPlayer->Name);
  SendToRoom(pConn->pPlayer->RoomNbr, pConn);
  pConn->PlayerDirty = true;
  sprintf(Buffer, "You magically transport to room %d.\r\n", DestRoomNbr);
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
  strcat(pConn->Output, "\r\nInventory\r\n---------\r\n");
  if (pConn->pPlayerInvHead == NULL)
  {
    strcat(pConn->Output, "You look into your bag and find it empty\r\n");
  }
  else
  {
    pPlayerInvListCurr = pConn->pPlayerInvHead;
    while (pPlayerInvListCurr != NULL)
    {
      sprintf(Buffer, "(%d) %s\r\n", pPlayerInvListCurr->Quantity, pPlayerInvListCurr->pObject->Desc1);
      strcat(pConn->Output, Buffer);
      pPlayerInvListCurr = pPlayerInvListCurr->pNextPlayerInv;
    }
  }
  strcat(pConn->Output, "\r\n");
  Prompt(pConn);
}

// Start a fight with a mobile in the current room.
void DoKill()
{
  DEBUGIT(1)
  if (pConn->pFightingMobile != NULL)
  {
    strcat(pConn->Output, "You are already fighting.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  RoomLookUp(pConn->pPlayer->RoomNbr);
  if (strstr(pRoom->Flags, "NoFight") != NULL)
  {
    strcat(pConn->Output, "You can't fight here.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  Word(2, Command, CmdParm1);
  MobileInstanceLookUp(CmdParm1);
  if (pMobileInstance == NULL)
  {
    strcat(pConn->Output, "You don't see that here.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  if (pMobileInstance->pFightingPlayer != NULL)
  {
    strcat(pConn->Output, "That mobile is already fighting someone.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  pConn->pFightingMobile = pMobileInstance;
  pMobileInstance->pFightingPlayer = pConn;
  sprintf(Buffer, "You start a fight with %s!\r\n\r\n", pMobileInstance->pMobile->Desc1);
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
}

// Display the objects sold by the shop in the player's current room.
void DoList()
{
  DEBUGIT(1)
  ShopLookUp(pConn->pPlayer->RoomNbr);
  if (pShop == NULL)
  {
    strcat(pConn->Output, "Find a shop.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  strcat(pConn->Output, "\r\nItems for sale\r\n--------------\r\n\r\n");
  strcat(pConn->Output, "Cost  Item\r\n----  ----\r\n");
  pShopObjectListCurr = pShop->pShopObjectHead;
  while (pShopObjectListCurr != NULL)
  {
    sprintf(Buffer, "%4d  %s\r\n", pShopObjectListCurr->pObject->Cost, pShopObjectListCurr->pObject->Desc1);
    strcat(pConn->Output, Buffer);
    pShopObjectListCurr = pShopObjectListCurr->pNextShopObject;
  }
  strcat(pConn->Output, "\r\n");
  Prompt(pConn);
}

// Load one object into the issuing admin's inventory.
void DoLoad()
{
  DEBUGIT(1)
  Word(2, Command, CmdParm1);
  ObjectLookUp(CmdParm1);
  pLoadObject = pObject;
  if (pLoadObject == NULL)
  {
    sprintf(Buffer, "Object %s does not exist.\r\n\r\n", CmdParm1);
    strcat(pConn->Output, Buffer);
    Prompt(pConn);
    return;
  }
  sprintf(Buffer, "You load %s.\r\n\r\n", pLoadObject->Desc1);
  PlayerInvAdd(pLoadObject);
  PlayerInvWriteFile();
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
}

// Display the current room and its contents to the player.
void DoLook()
{
  DEBUGIT(1)
  RoomLookUp(pConn->pPlayer->RoomNbr);
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
  sprintf(Buffer, "&CExits: %s&N\r\n", RoomExits);
  strcat(pConn->Output, Buffer);
  pConnCurrSave = pConnCurr;
  pConnCurr = pConnHead;
  while (pConnCurr != NULL)
  {
    if (pConnCurr != pConn && pConnCurr->State == Online && pConnCurr->pPlayer->RoomNbr == pConn->pPlayer->RoomNbr)
    {
      sprintf(Buffer, "%s is here.\r\n", pConnCurr->pPlayer->Name);
      strcat(pConn->Output, Buffer);
    }
    pConnCurr = pConnCurr->pConnNext;
  }
  pConnCurr = pConnCurrSave;
  pMobileInstanceCurr = pRoom->pMobileInstanceHead;
  while (pMobileInstanceCurr != NULL)
  {
    sprintf(Buffer, "%s\r\n", pMobileInstanceCurr->pMobile->Desc2);
    strcat(pConn->Output, Buffer);
    pMobileInstanceCurr = pMobileInstanceCurr->pNextRoomMobile;
  }
  pRoomObjectListCurr = pRoom->pRoomObjectHead;
  while (pRoomObjectListCurr != NULL)
  {
    sprintf(Buffer, "(%d) %s\r\n", pRoomObjectListCurr->Quantity, pRoomObjectListCurr->pObject->Desc2);
    strcat(pConn->Output, Buffer);
    pRoomObjectListCurr = pRoomObjectListCurr->pNextRoomObject;
  }
  ShopLookUp(pConn->pPlayer->RoomNbr);
  if (pShop != NULL)
  {
    sprintf(Buffer, "%s\r\n", pShop->Message);
    strcat(pConn->Output, Buffer);
  }
  strcat(pConn->Output, "\r\n");
  Prompt(pConn);
}

// Display the player's coin balance.
void DoMoney()
{
  DEBUGIT(1)
  sprintf(Buffer, "You have %d coins\r\n\r\n", pConn->pPlayer->Coins);
  strcat(pConn->Output, Buffer);
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
  PlayerReadFile();
  while (EndFile == false)
  {
    sprintf(Buffer, "%-10s %1s %c %3s %c %4s %2i %8s %lld", PlayerRcd.Name, " ", PlayerRcd.Admin, " ", PlayerRcd.Color, " ", PlayerRcd.Level, " ", PlayerRcd.Experience);
    strcat(pConn->Output, Buffer);
    strcat(pConn->Output, "\r\n");
    PlayerRcdNbr++;
    PlayerReadFile();
  }
  strcat(pConn->Output, "\r\n");
  Prompt(pConn);
}

// Handle the process of disconnecting a player by saving their data to the
// player file and changing their state to "Disconnect."
void DoQuit()
{
  DEBUGIT(1)
  PlayerWriteFile();
  strcat(pConn->Output, "Bye Bye");
  strcat(pConn->Output, "\r\n");
  pConn->State = Disconnect;
}

// Remove an equipped object and return it to the player's inventory.
void DoRemove()
{
  DEBUGIT(1)
  Word(2, Command, CmdParm1);
  PlayerEquLookUp(CmdParm1);
  if (pPlayerEquList == NULL)
  {
    sprintf(Buffer, "You don't have a(n) %s.\r\n\r\n", CmdParm1);
    strcat(pConn->Output, Buffer);
    Prompt(pConn);
    return;
  }
  sprintf(Buffer, "You remove %s.\r\n\r\n", pPlayerEquList->pObject->Desc1);
  PlayerInvAdd(pPlayerEquList->pObject);
  PlayerEquRemove();
  PlayerEquWriteFile();
  PlayerInvWriteFile();
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
}

// Restore an online player to their maximum hit points.
void DoRestore()
{
  DEBUGIT(1)
  pTarget       = NULL;
  pConnCurrSave = pConnCurr;
  pConnCurr     = pConnHead;
  Word(2, Command, CmdParm1);
  NormalizePlayerName(CmdParm1);
  while (pConnCurr != NULL)
  {
    if (pConnCurr->State == Online && Equal(pConnCurr->pPlayer->Name, CmdParm1))
    {
      pTarget = pConnCurr;
      break;
    }
    pConnCurr = pConnCurr->pConnNext;
  }
  pConnCurr = pConnCurrSave;
  if (pTarget == NULL)
  {
    sprintf(Buffer, "%s is not online.\r\n\r\n", CmdParm1);
    strcat(pConn->Output, Buffer);
    Prompt(pConn);
    return;
  }
  pTarget->HitPoints = pTarget->pPlayer->Level * PLAYER_HPT_PER_LEVEL;
  if (pTarget == pConn)
  {
    strcat(pConn->Output, "You restore yourself to full health.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  sprintf(Buffer, "\r\n%s restores you to full health.\r\n\r\n", pConn->pPlayer->Name);
  strcat(pTarget->Output, Buffer);
  Prompt(pTarget);
  sprintf(Buffer, "You restore %s to full health.\r\n\r\n", pTarget->pPlayer->Name);
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
}

// Send a spoken message to every other player in the current room.
void DoSay()
{
  DEBUGIT(1)
  Parameters = strchr(Command, ' ');
  Parameters++;
  Trim(Parameters);
  sprintf(Buffer, "You say, \"%s\"\r\n\r\n", Parameters);
  strcat(pConn->Output, Buffer);
  sprintf(MsgTxt, "%s says, \"%s\"\r\n\r\n", pConn->pPlayer->Name, Parameters);
  SendToRoom(pConn->pPlayer->RoomNbr, pConn);
  Prompt(pConn);
}

// Sell one inventory object to the shop in the player's current room.
void DoSell()
{
  DEBUGIT(1)
  ShopLookUp(pConn->pPlayer->RoomNbr);
  if (pShop == NULL)
  {
    strcat(pConn->Output, "Find a shop.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  Word(2, Command, CmdParm1);
  PlayerInvLookUp(CmdParm1);
  if (pPlayerInvList == NULL)
  {
    strcat(pConn->Output, "You don't have that.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  sprintf(Buffer, "You sell %s for %d coins.\r\n\r\n", pPlayerInvList->pObject->Desc1, pPlayerInvList->pObject->Cost);
  pConn->pPlayer->Coins += pPlayerInvList->pObject->Cost;
  PlayerInvRemoveOne();
  PlayerWriteFile();
  PlayerInvWriteFile();
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
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

// Move the player from standing to sitting.
void DoSit()
{
  DEBUGIT(1)
  if (pConn->Position == Sitting)
  {
    strcat(pConn->Output, "You are already sitting.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  if (pConn->Position == Sleeping)
  {
    strcat(pConn->Output, "You must wake up before you can sit.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  pConn->Position = Sitting;
  strcat(pConn->Output, "You sit down.\r\n\r\n");
  sprintf(MsgTxt, "%s sits down.\r\n\r\n", pConn->pPlayer->Name);
  SendToRoom(pConn->pPlayer->RoomNbr, pConn);
  Prompt(pConn);
}

// Move the player from sitting to sleeping.
void DoSleep()
{
  DEBUGIT(1)
  if (pConn->Position == Sleeping)
  {
    strcat(pConn->Output, "You are already asleep.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  if (pConn->Position == Standing)
  {
    strcat(pConn->Output, "You must be sitting before you can sleep.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  pConn->Position = Sleeping;
  strcat(pConn->Output, "You fall asleep.\r\n\r\n");
  sprintf(MsgTxt, "%s falls asleep.\r\n\r\n", pConn->pPlayer->Name);
  SendToRoom(pConn->pPlayer->RoomNbr, pConn);
  Prompt(pConn);
}

// Move the player from sitting to standing.
void DoStand()
{
  DEBUGIT(1)
  if (pConn->Position == Standing)
  {
    strcat(pConn->Output, "You are already standing.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  if (pConn->Position == Sleeping)
  {
    strcat(pConn->Output, "You must wake up before you can stand.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  pConn->Position = Standing;
  strcat(pConn->Output, "You stand up.\r\n\r\n");
  sprintf(MsgTxt, "%s stands up.\r\n\r\n", pConn->pPlayer->Name);
  SendToRoom(pConn->pPlayer->RoomNbr, pConn);
  Prompt(pConn);
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
  sprintf(Buffer, "AFK: %c\r\n", pConn->Afk);
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
  ExpCalcLevel = pConn->pPlayer->Level + 1;
  PlayerExpCalc();
  sprintf(Buffer, "Experience: %lld / %lld\r\n", pConn->pPlayer->Experience, ExpRequired);
  strcat(pConn->Output, Buffer);
  // Level
  sprintf(Buffer, "Level: %i\r\n", pConn->pPlayer->Level);
  strcat(pConn->Output, Buffer);
  //  Sex
  sprintf(Buffer, "Sex: %c\r\n", pConn->pPlayer->Sex);
  strcat(pConn->Output, Buffer);
  // Hunger
  sprintf(Buffer, "Hunger: %d%%\r\n", pConn->pPlayer->Hunger);
  strcat(pConn->Output, Buffer);
  // Thirst
  sprintf(Buffer, "Thirst: %d%%\r\n", pConn->pPlayer->Thirst);
  strcat(pConn->Output, Buffer);
  // Admin
  if (pConn->pPlayer->Admin == 'Y')
  {
    strcat(pConn->Output, "Your are an Admin!\r\n");
  }
  strcat(pConn->Output, "\r\n");
  Prompt(pConn);
}

// Send a private message to another online player.
void DoTell()
{
  DEBUGIT(1)
  Word(2, Command, CmdParm1);
  NormalizePlayerName(CmdParm1);
  pTarget       = NULL;
  pConnCurrSave = pConnCurr;
  pConnCurr     = pConnHead;
  while (pConnCurr != NULL)
  {
    if (pConnCurr->State == Online && Equal(pConnCurr->pPlayer->Name, CmdParm1))
    {
      pTarget = pConnCurr;
      break;
    }
    pConnCurr = pConnCurr->pConnNext;
  }
  pConnCurr = pConnCurrSave;
  if (pTarget == NULL)
  {
    sprintf(Buffer, "%s is not online.\r\n\r\n", CmdParm1);
    strcat(pConn->Output, Buffer);
    Prompt(pConn);
    return;
  }
  if (pTarget == pConn)
  {
    strcat(pConn->Output, "You can't tell yourself.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  Parameters = strchr(Command, ' ');
  while (Parameters[0] == ' ')
  {
    Parameters++;
  }
  while (Parameters[0] != ' ' && Parameters[0] != '\0')
  {
    Parameters++;
  }
  while (Parameters[0] == ' ')
  {
    Parameters++;
  }
  sprintf(Buffer, "\r\n&M%s tells you, \"%s\"&N\r\n\r\n", pConn->pPlayer->Name, Parameters);
  strcat(pTarget->Output, Buffer);
  Prompt(pTarget);
  sprintf(Buffer, "&MYou tell %s, \"%s\"&N\r\n\r\n", pTarget->pPlayer->Name, Parameters);
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
}

// Display the current time on the server.
void DoTime()
{
  DEBUGIT(1)
  GetTime();
  sprintf(Buffer, "Server time: %s\r\n\r\n", CurrentTimeTxt);
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
}

// Wake the player from sleeping and leave them sitting.
void DoWake()
{
  DEBUGIT(1)
  if (pConn->Position != Sleeping)
  {
    strcat(pConn->Output, "You are already awake.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  pConn->Position = Sitting;
  strcat(pConn->Output, "You wake up.\r\n\r\n");
  sprintf(MsgTxt, "%s wakes up.\r\n\r\n", pConn->pPlayer->Name);
  SendToRoom(pConn->pPlayer->RoomNbr, pConn);
  Prompt(pConn);
}

// Wear an armor object from the player's inventory.
void DoWear()
{
  DEBUGIT(1)
  Word(2, Command, CmdParm1);
  PlayerInvLookUp(CmdParm1);
  if (pPlayerInvList == NULL)
  {
    strcat(pConn->Output, "You don't have that.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  if (!Equal(pPlayerInvList->pObject->Type, "Armor"))
  {
    strcat(pConn->Output, "You can't wear that.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  WearPositionNbr = WearPositionLookUp(pPlayerInvList->pObject->Subtype);
  if (WearPositionNbr < 0)
  {
    strcat(pConn->Output, "You can't wear that.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  strcpy(TmpStr, WearPositionTable[WearPositionNbr].Slot1);
  PlayerEquSlotLookUp(TmpStr);
  if (pPlayerEquList != NULL)
  {
    if (WearPositionTable[WearPositionNbr].Slot2 == NULL)
    {
      strcat(pConn->Output, "You are already wearing something there.\r\n\r\n");
      Prompt(pConn);
      return;
    }
    strcpy(TmpStr, WearPositionTable[WearPositionNbr].Slot2);
    PlayerEquSlotLookUp(TmpStr);
    if (pPlayerEquList != NULL)
    {
      strcat(pConn->Output, "You are already wearing something there.\r\n\r\n");
      Prompt(pConn);
      return;
    }
  }
  sprintf(Buffer, "You wear %s.\r\n\r\n", pPlayerInvList->pObject->Desc1);
  PlayerEquAdd(pPlayerInvList->pObject, TmpStr);
  PlayerInvRemoveOne();
  PlayerEquWriteFile();
  PlayerInvWriteFile();
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
}

// Wield a weapon object from the player's inventory.
void DoWield()
{
  DEBUGIT(1)
  Word(2, Command, CmdParm1);
  PlayerInvLookUp(CmdParm1);
  if (pPlayerInvList == NULL)
  {
    strcat(pConn->Output, "You don't have that.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  if (!Equal(pPlayerInvList->pObject->Type, "Weapon"))
  {
    strcat(pConn->Output, "You can't wield that.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  strcpy(TmpStr, "Wielded");
  PlayerEquSlotLookUp(TmpStr);
  if (pPlayerEquList != NULL)
  {
    strcat(pConn->Output, "You are already wielding something.\r\n\r\n");
    Prompt(pConn);
    return;
  }
  sprintf(Buffer, "You wield %s.\r\n\r\n", pPlayerInvList->pObject->Desc1);
  PlayerEquAdd(pPlayerInvList->pObject, TmpStr);
  PlayerInvRemoveOne();
  PlayerEquWriteFile();
  PlayerInvWriteFile();
  strcat(pConn->Output, Buffer);
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
  strcat(pConn->Output, "Name       Level AFK\r\n");
  pConnCurrSave = pConnCurr;
  pConnCurr     = pConnHead;
  while (pConnCurr != NULL)
  {
    if (pConnCurr->State == Online)
    {
      sprintf(Buffer, "%-10s %2s %2i   %c", pConnCurr->pPlayer->Name, " ", pConnCurr->pPlayer->Level, pConnCurr->Afk);
      strcat(pConn->Output, Buffer);
      strcat(pConn->Output, "\r\n");
    }
    pConnCurr = pConnCurr->pConnNext;
  }
  strcat(pConn->Output, "\r\n");
  Prompt(pConn);
  pConnCurr = pConnCurrSave;
}

// Process one combat round for every player who is fighting.
void Combat()
{
  DEBUGIT(1)
  pConnSave     = pConn;
  pConnCurrSave = pConnCurr;
  pConnCurr     = pConnHead;
  while (pConnCurr != NULL)
  {
    pConn = pConnCurr;
    if (pConn->State == Online && pConn->pFightingMobile != NULL)
    {
      CombatRound();
    }
    pConnCurr = pConnCurr->pConnNext;
  }
  pConn     = pConnSave;
  pConnCurr = pConnCurrSave;
}

// Format a remaining health percentage using a color based on its range.
void CalcHealthPct(int HitPoints, int HitPointsMax)
{
  DEBUGIT(1)
  if (HitPoints < 1)
  {
    HitPercent = 0;
  }
  else
  {
    HitPercent = (int)(((long long)HitPoints * 100) / HitPointsMax);
  }
  if (HitPercent > 75)
  {
    sprintf(HealthPct, "&C%3d&N", HitPercent);
    return;
  }
  if (HitPercent > 50)
  {
    sprintf(HealthPct, "&Y%3d&N", HitPercent);
    return;
  }
  if (HitPercent > 25)
  {
    sprintf(HealthPct, "&M%3d&N", HitPercent);
    return;
  }
  sprintf(HealthPct, "&R%3d&N", HitPercent);
}

// End combat when the player dies, restore them, and return them to the starting room.
void CombatPlayerDeath()
{
  DEBUGIT(1)
  strcpy(TmpStr1, pMobile->Desc1);
  Up1stChar(TmpStr1);
  sprintf(Buffer, "%s %s you for %d points of damage.\r\nYou have been killed by %s.\r\nYou awaken fully restored.\r\n", TmpStr1, TmpStr, Damage, pMobile->Desc1);
  strcat(pConn->Output, Buffer);
  CombatStop();
  pConn->HitPoints = pConn->pPlayer->Level * PLAYER_HPT_PER_LEVEL;
  pConn->pPlayer->RoomNbr = PLAYER_START_ROOM;
  pConn->PlayerDirty = true;
  DoLook();
}

// Process one player attack followed by one surviving mobile counterattack.
void CombatRound()
{
  DEBUGIT(1)
  strcat(pConn->Output, "\r\n");
  pMobileInstance = pConn->pFightingMobile;
  pMobile = pMobileInstance->pMobile;
  PlayerAttackVerb();
  Damage = (rand() % ((pConn->pPlayer->Level * DAMAGE_PER_LEVEL) + WeaponDamage)) + 1;
  pMobileInstance->HitPoints -= Damage;
  if (pMobileInstance->HitPoints <= 0)
  {
    CombatVictory();
    return;
  }
  MaxHitPoints = (pMobile->Level * MOB_HPT_PER_LEVEL) + pMobile->Hit;
  CalcHealthPct(pMobileInstance->HitPoints, MaxHitPoints);
  sprintf(Buffer, "%s You %s %s for %d points of damage.\r\n", HealthPct, TmpStr, pMobile->Desc1, Damage);
  strcat(pConn->Output, Buffer);
  MobileAttackVerb();
  Damage = (rand() % (pMobile->Level * DAMAGE_PER_LEVEL)) + 1;
  pConn->HitPoints -= Damage;
  if (pConn->HitPoints <= 0)
  {
    CombatPlayerDeath();
    return;
  }
  MaxHitPoints = pConn->pPlayer->Level * PLAYER_HPT_PER_LEVEL;
  CalcHealthPct(pConn->HitPoints, MaxHitPoints);
  strcpy(TmpStr1, pMobile->Desc1);
  Up1stChar(TmpStr1);
  sprintf(Buffer, "%s %s %s you for %d points of damage.\r\n\r\n", HealthPct, TmpStr1, TmpStr, Damage);
  strcat(pConn->Output, Buffer);
  Prompt(pConn);
}

// Release the current player's mobile and restore the mobile to full health.
void CombatStop()
{
  DEBUGIT(1)
  if (pConn->pFightingMobile == NULL)
  {
    return;
  }
  pConn->pFightingMobile->pFightingPlayer = NULL;
  pConn->pFightingMobile->HitPoints = (pConn->pFightingMobile->pMobile->Level * MOB_HPT_PER_LEVEL) + pConn->pFightingMobile->pMobile->Hit;
  pConn->pFightingMobile = NULL;
}

// Finish a defeated mobile and award its experience and loot to the player.
void CombatVictory()
{
  DEBUGIT(1)
  sprintf(Buffer, "You vanquish %s with a %s that did %d points of damage.\r\n", pMobile->Desc1, TmpStr, Damage);
  strcat(pConn->Output, Buffer);
  MobileExpCalc();
  sprintf(Buffer, "You gain %d points of experience!\r\n", ExpAward);
  strcat(pConn->Output, Buffer);
  pMobileInstance->pFightingPlayer = NULL;
  pConn->pFightingMobile = NULL;
  MobileInstanceRemove();
  if (!Equal(pMobile->Loot, "None"))
  {
    strcat(pConn->Output, "You loot:\r\n");
    for (k = 1; k <= Words(pMobile->Loot); k++)
    {
      Word(k, pMobile->Loot, TmpStr1);
      ObjectLookUp(TmpStr1);
      PlayerInvAdd(pObject);
      sprintf(Buffer, "%s\r\n", pObject->Desc1);
      strcat(pConn->Output, Buffer);
    }
    PlayerInvWriteFile();
  }
  strcat(pConn->Output, "\r\n");
  pConn->pPlayer->Experience += ExpAward;
  PlayerLevelUp();
  PlayerWriteFile();
  Prompt(pConn);
}

// Select the current mobile's player-facing attack verb.
void MobileAttackVerb()
{
  DEBUGIT(1)
  if (Equal(pMobile->Attack, "Bite") || Equal(pMobile->Attack, "Bites"))
  {
    strcpy(TmpStr, "bites");
    return;
  }
  if (Equal(pMobile->Attack, "Slash"))
  {
    strcpy(TmpStr, "slashes");
    return;
  }
  if (Equal(pMobile->Attack, "Stab"))
  {
    strcpy(TmpStr, "stabs");
    return;
  }
  strcpy(TmpStr, "hits");
}

// Select the player's attack verb and wielded weapon damage bonus.
void PlayerAttackVerb()
{
  DEBUGIT(1)
  WeaponDamage = 0;
  strcpy(TmpStr, "Wielded");
  PlayerEquSlotLookUp(TmpStr);
  strcpy(TmpStr, "hit");
  if (pPlayerEquList == NULL)
  {
    return;
  }
  WeaponDamage = pPlayerEquList->pObject->Value;
  if (Equal(pPlayerEquList->pObject->Subtype, "Dagger"))
  {
    strcpy(TmpStr, "stab");
    return;
  }
  if (Equal(pPlayerEquList->pObject->Subtype, "Hammer"))
  {
    strcpy(TmpStr, "smash");
    return;
  }
  if (Equal(pPlayerEquList->pObject->Subtype, "Sword"))
  {
    strcpy(TmpStr, "slash");
  }
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// General player communication
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Append the player's current hit points and prompt to online output.
void Prompt(ConnList *pConn)
{
  DEBUGIT(1)
  if (pConn->State == Online)
  {
    sprintf(Buffer, "%dH > ", pConn->HitPoints);
    strcat(pConn->Output, Buffer);
    return;
  }
  strcat(pConn->Output, "> ");
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

// Send a message to every online player in the specified room except pExclude.
void SendToRoom(int RoomNbr, ConnList *pExclude)
{
  DEBUGIT(1)
  pConnSave     = pConn;
  pConnCurrSave = pConnCurr;
  pConnCurr     = pConnHead;
  while (pConnCurr != NULL)
  {
    if (pConnCurr != pExclude && pConnCurr->State == Online && pConnCurr->pPlayer->RoomNbr == RoomNbr)
    {
      pConn = pConnCurr;
      strcat(pConn->Output, "\r\n");
      strcat(pConn->Output, MsgTxt);
      Prompt(pConn);
    }
    pConnCurr = pConnCurr->pConnNext;
  }
  pConn     = pConnSave;
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
      PlayerEquReadFile();
      PlayerInvReadFile();
      SendMotd();
      pConn->State = Online;
      pConn->HitPoints = pConn->pPlayer->Level * PLAYER_HPT_PER_LEVEL;
      pConn->Position = Standing;
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
      PlayerEquReadFile();
      PlayerInvReadFile();
      SendMotd();
      pConn->State = Online;
      pConn->HitPoints = pConn->pPlayer->Level * PLAYER_HPT_PER_LEVEL;
      pConn->Position = Standing;
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
      if (BytesRead <= 0)
      {
        pConn->State = Disconnect;
        pConn->Output[0] = '\0';
        pConnCurr = pConnCurr->pConnNext;
        continue;
      }
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
    SendResult = send(Socket, Buffer, BufferLen, MSG_NOSIGNAL);
    if (SendResult != BufferLen)
    {
      pConn->State = Disconnect;
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
      CombatStop();
      if (pConn->PlayerRcdNbr > 0 && pConn->PlayerDirty)
      {
        PlayerWriteFile();
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
  PlayerOpenFile();
  MobileReadFile();
  ObjectReadFile();
  RoomReadFile();
  ShopReadFile();
  SpawnReadFile();
  SpawnMobiles();
}

// Set up the initial state of a game by resetting various player-related
// variables and setting the game state.
void Initialization()
{ // Do not add DEBUGIT
  GameShutDown       = false;
  CombatTick         = 0;
  HungerThirstTick   = 0;
  MobileMoveTick     = 0;
  MobileRespawnTick  = 0;
  PlayerRecoveryTick = 0;
  NoPlayers          = true;
  NextPlayerAutosave = time(NULL) + PLAYER_AUTOSAVE_SECONDS;
  pConnHead          = NULL;
  pConnTail          = NULL;
  pConnCurr          = NULL;
  srand((unsigned int)time(NULL));
}

// Gracefully shut down the game by closing files and logs.
void ShutItDown()
{
  DEBUGIT(1)
  PlayerAutoSave();
  MobileInstanceFreeList();
  RoomFreeList();
  PlayerCloseFile();
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
  pConn->Afk     = 'N';
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
void PlayerOpenFile()
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
void PlayerCloseFile()
{
  DEBUGIT(1)
  fclose(PlayerFile);
}

// Add an object to the end of the connection's equipment list.
void PlayerEquAdd(Object *pObject, char *Slot)
{
  DEBUGIT(1)
  pPlayerEquListNew = (PlayerEquList *)calloc(1, sizeof(PlayerEquList));
  if (pPlayerEquListNew == NULL)
  {
    sprintf(LogMsg, "ERROR: Memory allocation failed for PlayerEquList node");
    AbortIt();
  }
  pPlayerEquListNew->pObject = pObject;
  pPlayerEquListNew->Slot = strdup(Slot);
  if (pConn->pPlayerEquHead == NULL)
  {
    pConn->pPlayerEquHead = pPlayerEquListNew;
    pConn->pPlayerEquTail = pPlayerEquListNew;
  }
  else
  {
    pConn->pPlayerEquTail->pNextPlayerEqu = pPlayerEquListNew;
    pConn->pPlayerEquTail = pPlayerEquListNew;
  }
}

// Find the first equipped object containing the partial object Id.
void PlayerEquLookUp(char *Id)
{
  DEBUGIT(1)
  pPlayerEquListCurr = pConn->pPlayerEquHead;
  while (pPlayerEquListCurr != NULL)
  {
    if (IdMatch(Id, pPlayerEquListCurr->pObject->Id))
    {
      pPlayerEquList = pPlayerEquListCurr;
      return;
    }
    pPlayerEquListCurr = pPlayerEquListCurr->pNextPlayerEqu;
  }
  pPlayerEquList = NULL;
}

// Read a player's equipment file and build the connection's equipment list.
void PlayerEquReadFile()
{
  DEBUGIT(1)
  sprintf(TmpStr, "%s/%s/%s/%s.txt", YAGS_DIR, WORLD_DIR, PLAYER_EQU_DIR, pConn->pPlayer->Name);
  PlayerEquFile = fopen(TmpStr, "r");
  if (PlayerEquFile == NULL)
  {
    return;
  }
  while (fgets(Buffer, sizeof(Buffer), PlayerEquFile) != NULL)
  {
    TrimRight(Buffer);
    if (Buffer[0] == '\0')
    {
      continue;
    }
    Word(1, Buffer, TmpStr1);
    Word(2, Buffer, TmpStr2);
    if (pConn->pPlayerEquHead == NULL)
    {
      pConn->pPlayerEquHead = (PlayerEquList *)calloc(1, sizeof(PlayerEquList));
      pConn->pPlayerEquTail = pConn->pPlayerEquHead;
    }
    else
    {
      pConn->pPlayerEquTail->pNextPlayerEqu = (PlayerEquList *)calloc(1, sizeof(PlayerEquList));
      pConn->pPlayerEquTail = pConn->pPlayerEquTail->pNextPlayerEqu;
    }
    if (pConn->pPlayerEquTail == NULL)
    {
      sprintf(LogMsg, "ERROR: Memory allocation failed for PlayerEquList node");
      AbortIt();
    }
    ObjectLookUp(TmpStr1);
    pConn->pPlayerEquTail->pObject = pObject;
    pConn->pPlayerEquTail->Slot = strdup(TmpStr2);
  }
  fclose(PlayerEquFile);
}

// Remove an object from the connection's equipment list.
void PlayerEquRemove()
{
  DEBUGIT(1)
  pPlayerEquListCurr = pConn->pPlayerEquHead;
  pPlayerEquListPrev = NULL;
  while (pPlayerEquListCurr != pPlayerEquList)
  {
    pPlayerEquListPrev = pPlayerEquListCurr;
    pPlayerEquListCurr = pPlayerEquListCurr->pNextPlayerEqu;
  }
  if (pPlayerEquListPrev == NULL)
  {
    pConn->pPlayerEquHead = pPlayerEquListCurr->pNextPlayerEqu;
  }
  else
  {
    pPlayerEquListPrev->pNextPlayerEqu = pPlayerEquListCurr->pNextPlayerEqu;
  }
  if (pConn->pPlayerEquTail == pPlayerEquListCurr)
  {
    pConn->pPlayerEquTail = pPlayerEquListPrev;
  }
  free(pPlayerEquListCurr->Slot);
  free(pPlayerEquListCurr);
  pPlayerEquList     = NULL;
  pPlayerEquListCurr = NULL;
}

// Find an occupied equipment slot on the connection's equipment list.
void PlayerEquSlotLookUp(char *Slot)
{
  DEBUGIT(1)
  pPlayerEquListCurr = pConn->pPlayerEquHead;
  while (pPlayerEquListCurr != NULL)
  {
    if (strcasecmp(Slot, pPlayerEquListCurr->Slot) == 0)
    {
      pPlayerEquList = pPlayerEquListCurr;
      return;
    }
    pPlayerEquListCurr = pPlayerEquListCurr->pNextPlayerEqu;
  }
  pPlayerEquList = NULL;
}

// Rewrite the player's equipment file from the connection's equipment list.
void PlayerEquWriteFile()
{
  DEBUGIT(1)
  sprintf(TmpStr, "%s/%s/%s/%s.txt", YAGS_DIR, WORLD_DIR, PLAYER_EQU_DIR, pConn->pPlayer->Name);
  PlayerEquFile = fopen(TmpStr, "w");
  if (PlayerEquFile == NULL)
  {
    sprintf(LogMsg, "ERROR: Open player equipment file failed: %s", strerror(errno));
    AbortIt();
  }
  pPlayerEquListCurr = pConn->pPlayerEquHead;
  while (pPlayerEquListCurr != NULL)
  {
    fprintf(PlayerEquFile, "%s %s", pPlayerEquListCurr->pObject->Id, pPlayerEquListCurr->Slot);
    if (pPlayerEquListCurr->pNextPlayerEqu != NULL)
    {
      fputs("\r\n", PlayerEquFile);
    }
    pPlayerEquListCurr = pPlayerEquListCurr->pNextPlayerEqu;
  }
  ReturnValue1 = fflush(PlayerEquFile);
  if (ReturnValue1 != 0)
  {
    sprintf(LogMsg, "ERROR: fflush player equipment file");
    AbortIt();
  }
  ReturnValue1 = fsync(fileno(PlayerEquFile));
  if (ReturnValue1 != 0)
  {
    sprintf(LogMsg, "ERROR: fsync player equipment file");
    AbortIt();
  }
  fclose(PlayerEquFile);
}

// Add one object to the connection's inventory list.
void PlayerInvAdd(Object *pObject)
{
  DEBUGIT(1)
  PlayerInvLookUp(pObject->Id);
  pPlayerInvListCurr = pPlayerInvList;
  if (pPlayerInvListCurr != NULL)
  {
    pPlayerInvListCurr->Quantity++;
    return;
  }
  pPlayerInvListNew = (PlayerInvList *)calloc(1, sizeof(PlayerInvList));
  if (pPlayerInvListNew == NULL)
  {
    sprintf(LogMsg, "ERROR: Memory allocation failed for PlayerInvList node");
    AbortIt();
  }
  pPlayerInvListNew->pObject = pObject;
  pPlayerInvListNew->Quantity = 1;
  if (pConn->pPlayerInvHead == NULL)
  {
    pConn->pPlayerInvHead = pPlayerInvListNew;
    pConn->pPlayerInvTail = pPlayerInvListNew;
  }
  else
  {
    pConn->pPlayerInvTail->pNextPlayerInv = pPlayerInvListNew;
    pConn->pPlayerInvTail = pPlayerInvListNew;
  }
}

// Find the first inventory object containing the partial object Id.
void PlayerInvLookUp(char *Id)
{
  DEBUGIT(1)
  pPlayerInvListCurr = pConn->pPlayerInvHead;
  while (pPlayerInvListCurr != NULL)
  {
    if (IdMatch(Id, pPlayerInvListCurr->pObject->Id))
    {
      pPlayerInvList = pPlayerInvListCurr;
      return;
    }
    pPlayerInvListCurr = pPlayerInvListCurr->pNextPlayerInv;
  }
  pPlayerInvList = NULL;
}

// Read a player's inventory file and build the connection's inventory list.
void PlayerInvReadFile()
{
  DEBUGIT(1)
  sprintf(TmpStr, "%s/%s/%s/%s.txt", YAGS_DIR, WORLD_DIR, PLAYER_INV_DIR, pConn->pPlayer->Name);
  PlayerInvFile = fopen(TmpStr, "r");
  if (PlayerInvFile == NULL)
  {
    return;
  }
  while (fgets(Buffer, sizeof(Buffer), PlayerInvFile) != NULL)
  {
    TrimRight(Buffer);
    if (Buffer[0] == '\0')
    {
      continue;
    }
    Word(1, Buffer, TmpStr1);
    Word(2, Buffer, TmpStr2);
    if (pConn->pPlayerInvHead == NULL)
    {
      pConn->pPlayerInvHead = (PlayerInvList *)calloc(1, sizeof(PlayerInvList));
      pConn->pPlayerInvTail = pConn->pPlayerInvHead;
    }
    else
    {
      pConn->pPlayerInvTail->pNextPlayerInv = (PlayerInvList *)calloc(1, sizeof(PlayerInvList));
      pConn->pPlayerInvTail = pConn->pPlayerInvTail->pNextPlayerInv;
    }
    if (pConn->pPlayerInvTail == NULL)
    {
      sprintf(LogMsg, "ERROR: Memory allocation failed for PlayerInvList node");
      AbortIt();
    }
    ObjectLookUp(TmpStr1);
    pConn->pPlayerInvTail->pObject = pObject;
    pConn->pPlayerInvTail->Quantity = atoi(TmpStr2);
  }
  fclose(PlayerInvFile);
}

// Remove one object from an inventory list node and delete the node when empty.
void PlayerInvRemoveOne()
{
  DEBUGIT(1)
  if (pPlayerInvList->Quantity > 1)
  {
    pPlayerInvList->Quantity--;
    return;
  }
  pPlayerInvListCurr = pConn->pPlayerInvHead;
  pPlayerInvListPrev = NULL;
  while (pPlayerInvListCurr != pPlayerInvList)
  {
    pPlayerInvListPrev = pPlayerInvListCurr;
    pPlayerInvListCurr = pPlayerInvListCurr->pNextPlayerInv;
  }
  if (pPlayerInvListPrev == NULL)
  {
    pConn->pPlayerInvHead = pPlayerInvListCurr->pNextPlayerInv;
  }
  else
  {
    pPlayerInvListPrev->pNextPlayerInv = pPlayerInvListCurr->pNextPlayerInv;
  }
  if (pConn->pPlayerInvTail == pPlayerInvListCurr)
  {
    pConn->pPlayerInvTail = pPlayerInvListPrev;
  }
  free(pPlayerInvListCurr);
  pPlayerInvList     = NULL;
  pPlayerInvListCurr = NULL;
}

// Rewrite the player's inventory file from the connection's inventory list.
void PlayerInvWriteFile()
{
  DEBUGIT(1)
  sprintf(TmpStr, "%s/%s/%s/%s.txt", YAGS_DIR, WORLD_DIR, PLAYER_INV_DIR, pConn->pPlayer->Name);
  PlayerInvFile = fopen(TmpStr, "w");
  if (PlayerInvFile == NULL)
  {
    sprintf(LogMsg, "ERROR: Open player inventory file failed: %s", strerror(errno));
    AbortIt();
  }
  pPlayerInvListCurr = pConn->pPlayerInvHead;
  while (pPlayerInvListCurr != NULL)
  {
    fprintf(PlayerInvFile, "%s %d", pPlayerInvListCurr->pObject->Id, pPlayerInvListCurr->Quantity);
    if (pPlayerInvListCurr->pNextPlayerInv != NULL)
    {
      fputs("\r\n", PlayerInvFile);
    }
    pPlayerInvListCurr = pPlayerInvListCurr->pNextPlayerInv;
  }
  ReturnValue1 = fflush(PlayerInvFile);
  if (ReturnValue1 != 0)
  {
    sprintf(LogMsg, "ERROR: fflush player inventory file");
    AbortIt();
  }
  ReturnValue1 = fsync(fileno(PlayerInvFile));
  if (ReturnValue1 != 0)
  {
    sprintf(LogMsg, "ERROR: fsync player inventory file");
    AbortIt();
  }
  fclose(PlayerInvFile);
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
  PlayerReadFile();
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
    PlayerReadFile();
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
void PlayerReadFile()
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

// Calculate the total experience required for ExpCalcLevel.
void PlayerExpCalc()
{
  DEBUGIT(1)
  if (ExpCalcLevel <= 1)
  {
    ExpRequired = 0;
    return;
  }
  ExpBase = (long long)BASE_PLAYER_XP * ((((long long)ExpCalcLevel * (ExpCalcLevel + 1)) / 2) - 1);
  ExpAdditional = pow((double)ExpBase, log10((double)ExpCalcLevel + 20.0)) * ((double)ExpCalcLevel / 10000.0);
  ExpRequired = ExpBase + llround(ExpAdditional);
}

// Reduce hunger and thirst for every online player without sending messages.
void PlayerHungerThirst()
{
  DEBUGIT(1)
  pConnSave     = pConn;
  pConnCurrSave = pConnCurr;
  pConnCurr     = pConnHead;
  while (pConnCurr != NULL)
  {
    pConn = pConnCurr;
    if (pConn->State == Online && (pConn->pPlayer->Hunger > 0 || pConn->pPlayer->Thirst > 0))
    {
      pConn->pPlayer->Hunger -= HUNGER_THIRST_RATE;
      if (pConn->pPlayer->Hunger < 0)
      {
        pConn->pPlayer->Hunger = 0;
      }
      pConn->pPlayer->Thirst -= HUNGER_THIRST_RATE;
      if (pConn->pPlayer->Thirst < 0)
      {
        pConn->pPlayer->Thirst = 0;
      }
      pConn->PlayerDirty = true;
    }
    pConnCurr = pConnCurr->pConnNext;
  }
  pConn     = pConnSave;
  pConnCurr = pConnCurrSave;
}

// Silently recover hit points for online players who are not fighting.
void PlayerRecoverHitPoints()
{
  DEBUGIT(1)
  pConnSave     = pConn;
  pConnCurrSave = pConnCurr;
  pConnCurr     = pConnHead;
  while (pConnCurr != NULL)
  {
    pConn = pConnCurr;
    MaxHitPoints = pConn->pPlayer->Level * PLAYER_HPT_PER_LEVEL;
    if (pConn->State == Online && pConn->pFightingMobile == NULL && pConn->HitPoints < MaxHitPoints)
    {
      RecoveryRate = (double)PLAYER_RECOVERY_AMOUNT;
      if (pConn->Position == Sitting)
      {
        RecoveryRate += 0.25;
      }
      else if (pConn->Position == Sleeping)
      {
        RecoveryRate += 0.5;
      }
      RecoveryRate += (((double)pConn->pPlayer->Hunger + pConn->pPlayer->Thirst) / 200.0) * 1.5;
      pConn->HitPointRecovery += RecoveryRate;
      HitPointsRecovered = (int)pConn->HitPointRecovery;
      pConn->HitPointRecovery -= HitPointsRecovered;
      pConn->HitPoints += HitPointsRecovered;
      if (pConn->HitPoints > MaxHitPoints)
      {
        pConn->HitPoints = MaxHitPoints;
        pConn->HitPointRecovery = 0.0;
      }
    }
    else
    {
      pConn->HitPointRecovery = 0.0;
    }
    pConnCurr = pConnCurr->pConnNext;
  }
  pConn     = pConnSave;
  pConnCurr = pConnCurrSave;
}

// Advance the current player through every level earned by their experience.
void PlayerLevelUp()
{
  DEBUGIT(1)
  for (;;)
  {
    ExpCalcLevel = pConn->pPlayer->Level + 1;
    PlayerExpCalc();
    if (pConn->pPlayer->Experience < ExpRequired)
    {
      return;
    }
    pConn->pPlayer->Level++;
    sprintf(Buffer, "You advance to level %d!\r\n\r\n", pConn->pPlayer->Level);
    strcat(pConn->Output, Buffer);
  }
}

// Write player data to the player file.
void PlayerWriteFile()
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
  pConn->pPlayer->Born       = time(NULL);
  pConn->pPlayer->Color      = 'N';
  pConn->pPlayer->Coins      = 0;
  pConn->pPlayer->Experience = 0;
  pConn->pPlayer->Hunger     = 100;
  pConn->pPlayer->Level      = 1;
  pConn->pPlayer->Thirst     = 100;
  pConn->pPlayer->RoomNbr    = PLAYER_START_ROOM;
}

// Determines the next PlayerRcdNbr by reading the player file until EOF.
void GetNextPlayerRcdNbr()
{
  DEBUGIT(1)
  EndFile      = false;
  PlayerRcdNbr = 1;
  PlayerReadFile();
  while (EndFile == false)
  {
    PlayerRcdNbr++;
    PlayerReadFile();
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

// Return true when FullId contains PartialId, ignoring case.
bool IdMatch(char *PartialId, char *FullId)
{
  DEBUGIT(2)
  if (PartialId[0] == '\0')
  {
    return false;
  }
  while (FullId[0] != '\0')
  {
    if (strncasecmp(PartialId, FullId, strlen(PartialId)) == 0)
    {
      return true;
    }
    FullId++;
  }
  return false;
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

// Remove leading and trailing whitespace characters from a given C-style string.
void Trim(char *Str)
{
  DEBUGIT(2)
  TrimLeft(Str);
  TrimRight(Str);
}

// Remove leading whitespace characters from a C-style string.
void TrimLeft(char *Str)
{
  DEBUGIT(2)
  x = 0;
  while (Str[x] != '\0' && isspace((unsigned char)Str[x]))
  {
    x++;
  }
  if (x > 0)
  {
    memmove(Str, Str + x, strlen(Str + x) + 1);
  }
}

// Remove trailing whitespace characters from a C-style string.
void TrimRight(char *Str)
{
  DEBUGIT(2)
  x = strlen(Str);
  while (x > 0 && isspace((unsigned char)Str[x - 1]))
  {
    x--;
  }
  Str[x] = '\0';
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

// Add one runtime mobile instance to the world and its spawn room.
void MobileInstanceAdd()
{
  DEBUGIT(1)
  pMobileInstanceNew = (MobileInstance *)calloc(1, sizeof(MobileInstance));
  if (pMobileInstanceNew == NULL)
  {
    sprintf(LogMsg, "ERROR: Memory allocation failed for MobileInstance");
    AbortIt();
  }
  pMobileInstanceNew->pMobile = pSpawn->pMobile;
  pMobileInstanceNew->pSpawn = pSpawn;
  pMobileInstanceNew->pRoom = pSpawn->pRoom;
  pMobileInstanceNew->HitPoints = (pSpawn->pMobile->Level * MOB_HPT_PER_LEVEL) + pSpawn->pMobile->Hit;
  if (pMobileInstanceHead == NULL)
  {
    pMobileInstanceHead = pMobileInstanceNew;
    pMobileInstanceTail = pMobileInstanceNew;
  }
  else
  {
    pMobileInstanceTail->pNextMobileInstance = pMobileInstanceNew;
    pMobileInstanceTail = pMobileInstanceNew;
  }
  if (pSpawn->pRoom->pMobileInstanceHead == NULL)
  {
    pSpawn->pRoom->pMobileInstanceHead = pMobileInstanceNew;
    pSpawn->pRoom->pMobileInstanceTail = pMobileInstanceNew;
  }
  else
  {
    pSpawn->pRoom->pMobileInstanceTail->pNextRoomMobile = pMobileInstanceNew;
    pSpawn->pRoom->pMobileInstanceTail = pMobileInstanceNew;
  }
  pSpawn->CurrentInWorld++;
}

// Free every runtime mobile instance.
void MobileInstanceFreeList()
{
  DEBUGIT(1)
  pMobileInstanceCurr = pMobileInstanceHead;
  while (pMobileInstanceCurr != NULL)
  {
    pMobileInstanceNext = pMobileInstanceCurr->pNextMobileInstance;
    free(pMobileInstanceCurr);
    pMobileInstanceCurr = pMobileInstanceNext;
  }
  pMobileInstance     = NULL;
  pMobileInstanceCurr = NULL;
  pMobileInstanceHead = NULL;
  pMobileInstanceNext = NULL;
  pMobileInstanceTail = NULL;
}

// Find the first runtime mobile containing the partial mobile Id in the current room.
void MobileInstanceLookUp(char *Id)
{
  DEBUGIT(1)
  pMobileInstanceCurr = pRoom->pMobileInstanceHead;
  while (pMobileInstanceCurr != NULL)
  {
    if (IdMatch(Id, pMobileInstanceCurr->pMobile->Id))
    {
      pMobileInstance = pMobileInstanceCurr;
      return;
    }
    pMobileInstanceCurr = pMobileInstanceCurr->pNextRoomMobile;
  }
  pMobileInstance = NULL;
}

// Move the current runtime mobile to a random eligible connected room.
void MobileInstanceMove()
{
  DEBUGIT(1)
  MobileMoveRoomCount = 0;
  for (k = 0; k < 10; k++)
  {
    Word(k + 1, pMobileInstanceCurr->pRoom->Exits, TmpStr1);
    if (Equal(TmpStr1, "xxxxx"))
    {
      continue;
    }
    RoomLookUp(atoi(TmpStr1));
    pMobileMoveRoom = pRoom;
    if (pMobileMoveRoom == NULL || strstr(pMobileMoveRoom->Flags, "NoNPC") != NULL)
    {
      continue;
    }
    pMobileMoveRooms[MobileMoveRoomCount] = pMobileMoveRoom;
    MobileMoveRoomCount++;
  }
  if (MobileMoveRoomCount == 0)
  {
    return;
  }
  pMobileMoveRoom = pMobileMoveRooms[rand() % MobileMoveRoomCount];
  sprintf(MsgTxt, "%s leaves.\r\n\r\n", pMobileInstanceCurr->pMobile->Desc1);
  SendToRoom(pMobileInstanceCurr->pRoom->RoomNbr, NULL);
  pMobileInstance = pMobileInstanceCurr->pRoom->pMobileInstanceHead;
  pMobileInstancePrev = NULL;
  while (pMobileInstance != pMobileInstanceCurr)
  {
    pMobileInstancePrev = pMobileInstance;
    pMobileInstance = pMobileInstance->pNextRoomMobile;
  }
  if (pMobileInstancePrev == NULL)
  {
    pMobileInstanceCurr->pRoom->pMobileInstanceHead = pMobileInstanceCurr->pNextRoomMobile;
  }
  else
  {
    pMobileInstancePrev->pNextRoomMobile = pMobileInstanceCurr->pNextRoomMobile;
  }
  if (pMobileInstanceCurr->pRoom->pMobileInstanceTail == pMobileInstanceCurr)
  {
    pMobileInstanceCurr->pRoom->pMobileInstanceTail = pMobileInstancePrev;
  }
  pMobileInstanceCurr->pNextRoomMobile = NULL;
  pMobileInstanceCurr->pRoom = pMobileMoveRoom;
  if (pMobileMoveRoom->pMobileInstanceHead == NULL)
  {
    pMobileMoveRoom->pMobileInstanceHead = pMobileInstanceCurr;
    pMobileMoveRoom->pMobileInstanceTail = pMobileInstanceCurr;
  }
  else
  {
    pMobileMoveRoom->pMobileInstanceTail->pNextRoomMobile = pMobileInstanceCurr;
    pMobileMoveRoom->pMobileInstanceTail = pMobileInstanceCurr;
  }
  sprintf(MsgTxt, "%s arrives.\r\n\r\n", pMobileInstanceCurr->pMobile->Desc1);
  SendToRoom(pMobileInstanceCurr->pRoom->RoomNbr, NULL);
}

// Remove the found runtime mobile from its room and the world.
void MobileInstanceRemove()
{
  DEBUGIT(1)
  if (pMobileInstance->pFightingPlayer != NULL)
  {
    pMobileInstance->pFightingPlayer->pFightingMobile = NULL;
    pMobileInstance->pFightingPlayer = NULL;
  }
  pSpawn = pMobileInstance->pSpawn;
  pMobileInstanceCurr = pMobileInstance->pRoom->pMobileInstanceHead;
  pMobileInstancePrev = NULL;
  while (pMobileInstanceCurr != pMobileInstance)
  {
    pMobileInstancePrev = pMobileInstanceCurr;
    pMobileInstanceCurr = pMobileInstanceCurr->pNextRoomMobile;
  }
  if (pMobileInstancePrev == NULL)
  {
    pMobileInstance->pRoom->pMobileInstanceHead = pMobileInstance->pNextRoomMobile;
  }
  else
  {
    pMobileInstancePrev->pNextRoomMobile = pMobileInstance->pNextRoomMobile;
  }
  if (pMobileInstance->pRoom->pMobileInstanceTail == pMobileInstance)
  {
    pMobileInstance->pRoom->pMobileInstanceTail = pMobileInstancePrev;
  }
  pMobileInstanceCurr = pMobileInstanceHead;
  pMobileInstancePrev = NULL;
  while (pMobileInstanceCurr != pMobileInstance)
  {
    pMobileInstancePrev = pMobileInstanceCurr;
    pMobileInstanceCurr = pMobileInstanceCurr->pNextMobileInstance;
  }
  if (pMobileInstancePrev == NULL)
  {
    pMobileInstanceHead = pMobileInstance->pNextMobileInstance;
  }
  else
  {
    pMobileInstancePrev->pNextMobileInstance = pMobileInstance->pNextMobileInstance;
  }
  if (pMobileInstanceTail == pMobileInstance)
  {
    pMobileInstanceTail = pMobileInstancePrev;
  }
  pSpawn->CurrentInWorld--;
  free(pMobileInstance);
  pMobileInstance = NULL;
  pMobileInstanceCurr = NULL;
  pMobileInstancePrev = NULL;
  if (!pSpawn->RespawnPending)
  {
    SpawnScheduleNext();
  }
}

// Give every movable runtime mobile a chance to change rooms.
void MobileMove()
{
  DEBUGIT(1)
  pMobileInstanceCurr = pMobileInstanceHead;
  while (pMobileInstanceCurr != NULL)
  {
    if (pMobileInstanceCurr->pFightingPlayer == NULL && strstr(pMobileInstanceCurr->pMobile->Flags, "NoMove") == NULL && rand() % 100 < MOBILE_MOVE_CHANCE)
    {
      MobileInstanceMove();
    }
    pMobileInstanceCurr = pMobileInstanceCurr->pNextMobileInstance;
  }
}

// Spawn one due replacement per rule during a respawn check.
void MobileRespawn()
{
  DEBUGIT(1)
  pSpawnListCurr = pSpawnListHead;
  while (pSpawnListCurr != NULL)
  {
    pSpawn = pSpawnListCurr->pSpawn;
    if (pSpawn->RespawnPending && pSpawn->CurrentInWorld < pSpawn->MaxInWorld && CurrentTimeSec >= pSpawn->NextSpawnTime)
    {
      pSpawn->RespawnPending = false;
      MobileInstanceAdd();
      if (pSpawn->CurrentInWorld < pSpawn->MaxInWorld)
      {
        SpawnScheduleNext();
      }
    }
    pSpawnListCurr = pSpawnListCurr->pNextSpawn;
  }
}

// Calculate the experience awarded for killing the current mobile.
void MobileExpCalc()
{
  DEBUGIT(1)
  ExpLevelDiff = pConn->pPlayer->Level - pMobile->Level;
  ExpPercent = 100;
  if (ExpLevelDiff > 2)
  {
    ExpPercent = 100 - ((ExpLevelDiff - 2) * 20);
  }
  if (ExpPercent < 0)
  {
    ExpPercent = 0;
  }
  ExpAward = (pMobile->Level * BASE_MOB_XP * ExpPercent) / 100;
  ExpAward += pMobile->Exp;
}

// Search the permanent mobile list for a case-insensitive Id match.
void MobileLookUp(char *Id)
{
  DEBUGIT(1)
  pMobileListCurr = pMobileListHead;
  while (pMobileListCurr != NULL)
  {
    if (strcasecmp(Id, pMobileListCurr->pMobile->Id) == 0)
    {
      pMobile = pMobileListCurr->pMobile;
      return;
    }
    pMobileListCurr = pMobileListCurr->pNextMobile;
  }
  pMobile = NULL;
}

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
    TrimRight(Buffer);
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
    TrimRight(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Desc1 = strdup(TmpStr);
    // Desc2
    fgets(Buffer, sizeof(Buffer), MobileFile);
    TrimRight(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Desc2 = strdup(TmpStr);
    // Desc3
    fgets(Buffer, sizeof(Buffer), MobileFile);
    TrimRight(Buffer);
    TmpStr[0] = '\0';
    while (fgets(Buffer, sizeof(Buffer), MobileFile) != NULL)
    {
      TrimRight(Buffer);
      if (strncmp(Buffer, "Flags:", 6) == 0)
      {
        break;
      }
      StrAppend(TmpStr, Buffer);
      StrAppend(TmpStr, "\n");
    }
    pMobileListTail->pMobile->Desc3 = strdup(TmpStr);
    // Flags
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Flags = strdup(TmpStr);
    // Attack
    fgets(Buffer, sizeof(Buffer), MobileFile);
    TrimRight(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Attack = strdup(TmpStr);
    // Level
    fgets(Buffer, sizeof(Buffer), MobileFile);
    TrimRight(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Level = atoi(TmpStr);
    // Hit
    fgets(Buffer, sizeof(Buffer), MobileFile);
    TrimRight(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Hit = atoi(TmpStr);
    // Exp
    fgets(Buffer, sizeof(Buffer), MobileFile);
    TrimRight(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Exp = atoi(TmpStr);
    // Loot
    fgets(Buffer, sizeof(Buffer), MobileFile);
    TrimRight(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pMobileListTail->pMobile->Loot = strdup(TmpStr);
  }
  fclose(MobileFile);
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Objects
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Search the permanent object list for a case-insensitive Id match.
void ObjectLookUp(char *Id)
{
  DEBUGIT(1)
  pObjectListCurr = pObjectListHead;
  while (pObjectListCurr != NULL)
  {
    if (strcasecmp(Id, pObjectListCurr->pObject->Id) == 0)
    {
      pObject = pObjectListCurr->pObject;
      return;
    }
    pObjectListCurr = pObjectListCurr->pNextObject;
  }
  pObject = NULL;
}

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
    TrimRight(Buffer);
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
    TrimRight(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pObjectListTail->pObject->Desc1 = strdup(TmpStr);
    // Desc2
    fgets(Buffer, sizeof(Buffer), ObjectFile);
    TrimRight(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pObjectListTail->pObject->Desc2 = strdup(TmpStr);
    // Desc3
    fgets(Buffer, sizeof(Buffer), ObjectFile);
    TrimRight(Buffer);
    TmpStr[0] = '\0';
    while (fgets(Buffer, sizeof(Buffer), ObjectFile) != NULL)
    {
      TrimRight(Buffer);
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
    TrimRight(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pObjectListTail->pObject->Cost = atoi(TmpStr);
    // Type
    fgets(Buffer, sizeof(Buffer), ObjectFile);
    TrimRight(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pObjectListTail->pObject->Type = strdup(TmpStr);
    // Subtype
    fgets(Buffer, sizeof(Buffer), ObjectFile);
    TrimRight(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pObjectListTail->pObject->Subtype = strdup(TmpStr);
    // Value
    fgets(Buffer, sizeof(Buffer), ObjectFile);
    TrimRight(Buffer);
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

// Dynamically allocate a Room structure and copy the contents of SingleRoom
// into the global pNewRoom.
void RoomAllocateAndCopy(const Room *SourceRoom)
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
  pNewRoom->pMobileInstanceHead = NULL;
  pNewRoom->pMobileInstanceTail = NULL;
  pNewRoom->pRoomObjectHead = NULL;
  pNewRoom->pRoomObjectTail = NULL;
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
      pRoomObjectListCurr = pRoomListCurr->pRoom->pRoomObjectHead;
      while (pRoomObjectListCurr != NULL)
      {
        pRoomObjectListNext = pRoomObjectListCurr->pNextRoomObject;
        free(pRoomObjectListCurr);
        pRoomObjectListCurr = pRoomObjectListNext;
      }
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

// Search for a room by RoomNbr and store the result in the global pRoom.
void RoomLookUp(int RoomNbr)
{
  DEBUGIT(1)
  pRoomListCurr = pRoomListHead;
  while (pRoomListCurr != NULL)
  {
    if (pRoomListCurr->pRoom != NULL && pRoomListCurr->pRoom->RoomNbr == RoomNbr)
    {
      pRoom = pRoomListCurr->pRoom;
      return;
    }
    pRoomListCurr = pRoomListCurr->pNextRoom;
  }
  pRoom = NULL;
}

// Add one object to the ground in the current room.
void RoomObjectAdd(Object *pObject)
{
  DEBUGIT(1)
  RoomObjectLookUp(pObject->Id);
  pRoomObjectListCurr = pRoomObjectList;
  if (pRoomObjectListCurr != NULL)
  {
    pRoomObjectListCurr->Quantity++;
    return;
  }
  pRoomObjectListNew = (RoomObjectList *)calloc(1, sizeof(RoomObjectList));
  if (pRoomObjectListNew == NULL)
  {
    sprintf(LogMsg, "ERROR: Memory allocation failed for RoomObjectList node");
    AbortIt();
  }
  pRoomObjectListNew->pObject = pObject;
  pRoomObjectListNew->Quantity = 1;
  if (pRoom->pRoomObjectHead == NULL)
  {
    pRoom->pRoomObjectHead = pRoomObjectListNew;
    pRoom->pRoomObjectTail = pRoomObjectListNew;
  }
  else
  {
    pRoom->pRoomObjectTail->pNextRoomObject = pRoomObjectListNew;
    pRoom->pRoomObjectTail = pRoomObjectListNew;
  }
}

// Find the first room object containing the partial object Id.
void RoomObjectLookUp(char *Id)
{
  DEBUGIT(1)
  pRoomObjectListCurr = pRoom->pRoomObjectHead;
  while (pRoomObjectListCurr != NULL)
  {
    if (IdMatch(Id, pRoomObjectListCurr->pObject->Id))
    {
      pRoomObjectList = pRoomObjectListCurr;
      return;
    }
    pRoomObjectListCurr = pRoomObjectListCurr->pNextRoomObject;
  }
  pRoomObjectList = NULL;
}

// Remove one object from the ground and delete the node when empty.
void RoomObjectRemoveOne()
{
  DEBUGIT(1)
  if (pRoomObjectList->Quantity > 1)
  {
    pRoomObjectList->Quantity--;
    return;
  }
  pRoomObjectListCurr = pRoom->pRoomObjectHead;
  pRoomObjectListPrev = NULL;
  while (pRoomObjectListCurr != pRoomObjectList)
  {
    pRoomObjectListPrev = pRoomObjectListCurr;
    pRoomObjectListCurr = pRoomObjectListCurr->pNextRoomObject;
  }
  if (pRoomObjectListPrev == NULL)
  {
    pRoom->pRoomObjectHead = pRoomObjectListCurr->pNextRoomObject;
  }
  else
  {
    pRoomObjectListPrev->pNextRoomObject = pRoomObjectListCurr->pNextRoomObject;
  }
  if (pRoom->pRoomObjectTail == pRoomObjectListCurr)
  {
    pRoom->pRoomObjectTail = pRoomObjectListPrev;
  }
  free(pRoomObjectListCurr);
  pRoomObjectList     = NULL;
  pRoomObjectListCurr = NULL;
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
    TrimRight(Buffer);
    LineNbr++;
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
      TrimRight(Buffer);
      LineNbr++;
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
    TrimRight(Buffer);
    LineNbr++;
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
    TrimRight(Buffer);
    LineNbr++;
    SingleRoom.Exits = strdup(Buffer);
    if (fgets(Buffer, sizeof(Buffer), RoomFile) == NULL)
    {
      sprintf(LogMsg, "ERROR: Failed to skip blank line after exits in %s at line %d", ROOMS_FILE, LineNbr);
      AbortIt();
    }
    LineNbr++;
    RoomAllocateAndCopy(&SingleRoom);
    RoomAddToRoomList();
  }
  fclose(RoomFile);
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Shops
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Search the permanent shop list for a shop in the specified room and store it in pShop.
void ShopLookUp(int RoomNbr)
{
  DEBUGIT(1)
  pShopListCurr = pShopListHead;
  while (pShopListCurr != NULL)
  {
    if (pShopListCurr->pShop->pRoom->RoomNbr == RoomNbr)
    {
      pShop = pShopListCurr->pShop;
      return;
    }
    pShopListCurr = pShopListCurr->pNextShop;
  }
  pShop = NULL;
}

// Find the first shop object containing the partial object Id.
void ShopObjectLookUp(char *Id)
{
  DEBUGIT(1)
  pShopObjectListCurr = pShop->pShopObjectHead;
  while (pShopObjectListCurr != NULL)
  {
    if (IdMatch(Id, pShopObjectListCurr->pObject->Id))
    {
      pShopObjectList = pShopObjectListCurr;
      return;
    }
    pShopObjectListCurr = pShopObjectListCurr->pNextShopObject;
  }
  pShopObjectList = NULL;
}

// Read shop definitions and build the permanent shop and shop object lists.
void ShopReadFile()
{
  DEBUGIT(1)
  sprintf(TmpStr, "%s/%s/%s", YAGS_DIR, WORLD_DIR, SHOPS_FILE);
  ShopFile = fopen(TmpStr, "r");
  if (ShopFile == NULL)
  {
    sprintf(LogMsg, "ERROR: Open %s failed: %s", SHOPS_FILE, strerror(errno));
    AbortIt();
  }
  while (fgets(Buffer, sizeof(Buffer), ShopFile) != NULL)
  {
    TrimRight(Buffer);
    if (Buffer[0] == '\0')
    {
      continue;
    }
    if (pShopListHead == NULL)
    {
      pShopListHead = (ShopList *)calloc(1, sizeof(ShopList));
      pShopListTail = pShopListHead;
    }
    else
    {
      pShopListTail->pNextShop = (ShopList *)calloc(1, sizeof(ShopList));
      pShopListTail = pShopListTail->pNextShop;
    }
    if (pShopListTail == NULL)
    {
      sprintf(LogMsg, "ERROR: Memory allocation failed for ShopList node");
      AbortIt();
    }
    pShopListTail->pShop = (Shop *)calloc(1, sizeof(Shop));
    if (pShopListTail->pShop == NULL)
    {
      sprintf(LogMsg, "ERROR: Memory allocation failed for Shop");
      AbortIt();
    }
    pShop = pShopListTail->pShop;
    RoomLookUp(atoi(Buffer));
    pShop->pRoom = pRoom;
    fgets(Buffer, sizeof(Buffer), ShopFile);
    TrimRight(Buffer);
    pShop->Message = strdup(Buffer);
    while (fgets(Buffer, sizeof(Buffer), ShopFile) != NULL)
    {
      TrimRight(Buffer);
      if (Buffer[0] == '\0')
      {
        break;
      }
      if (pShop->pShopObjectHead == NULL)
      {
        pShop->pShopObjectHead = (ShopObjectList *)calloc(1, sizeof(ShopObjectList));
        pShop->pShopObjectTail = pShop->pShopObjectHead;
      }
      else
      {
        pShop->pShopObjectTail->pNextShopObject = (ShopObjectList *)calloc(1, sizeof(ShopObjectList));
        pShop->pShopObjectTail = pShop->pShopObjectTail->pNextShopObject;
      }
      if (pShop->pShopObjectTail == NULL)
      {
        sprintf(LogMsg, "ERROR: Memory allocation failed for ShopObjectList node");
        AbortIt();
      }
      ObjectLookUp(Buffer);
      pShop->pShopObjectTail->pObject = pObject;
    }
  }
  fclose(ShopFile);
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Spawns
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Calculate when the current spawn rule may create its next replacement.
void SpawnScheduleNext()
{
  DEBUGIT(1)
  CurrentTimeSec = time(NULL);
  pSpawnTime = localtime(&CurrentTimeSec);
  SpawnTime = *pSpawnTime;
  SpawnTime.tm_year += pSpawn->Years;
  SpawnTime.tm_mon += pSpawn->Months;
  SpawnTime.tm_mday += pSpawn->Days + (pSpawn->Weeks * 7);
  SpawnTime.tm_hour += pSpawn->Hours;
  SpawnTime.tm_min += pSpawn->Minutes;
  SpawnTime.tm_sec += pSpawn->Seconds;
  pSpawn->NextSpawnTime = mktime(&SpawnTime);
  pSpawn->RespawnPending = true;
}

// Fill every spawn rule to its maximum runtime mobile population.
void SpawnMobiles()
{
  DEBUGIT(1)
  pSpawnListCurr = pSpawnListHead;
  while (pSpawnListCurr != NULL)
  {
    pSpawn = pSpawnListCurr->pSpawn;
    while (pSpawn->CurrentInWorld < pSpawn->MaxInWorld)
    {
      MobileInstanceAdd();
    }
    pSpawnListCurr = pSpawnListCurr->pNextSpawn;
  }
}

// Read spawn definitions and build the permanent spawn list.
void SpawnReadFile()
{
  DEBUGIT(1)
  sprintf(TmpStr, "%s/%s/%s", YAGS_DIR, WORLD_DIR, SPAWN_FILE);
  SpawnFile = fopen(TmpStr, "r");
  if (SpawnFile == NULL)
  {
    sprintf(LogMsg, "ERROR: Open %s failed: %s", SPAWN_FILE, strerror(errno));
    AbortIt();
  }
  while (fgets(Buffer, sizeof(Buffer), SpawnFile) != NULL)
  {
    TrimRight(Buffer);
    if (Buffer[0] == '\0')
    {
      continue;
    }
    if (pSpawnListHead == NULL)
    {
      pSpawnListHead = (SpawnList *)calloc(1, sizeof(SpawnList));
      pSpawnListTail = pSpawnListHead;
    }
    else
    {
      pSpawnListTail->pNextSpawn = (SpawnList *)calloc(1, sizeof(SpawnList));
      pSpawnListTail = pSpawnListTail->pNextSpawn;
    }
    if (pSpawnListTail == NULL)
    {
      sprintf(LogMsg, "ERROR: Memory allocation failed for SpawnList node");
      AbortIt();
    }
    pSpawnListTail->pSpawn = (Spawn *)calloc(1, sizeof(Spawn));
    if (pSpawnListTail->pSpawn == NULL)
    {
      sprintf(LogMsg, "ERROR: Memory allocation failed for Spawn");
      AbortIt();
    }
    pSpawn = pSpawnListTail->pSpawn;
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    MobileLookUp(TmpStr);
    pSpawn->pMobile = pMobile;
    fgets(Buffer, sizeof(Buffer), SpawnFile);
    TrimRight(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    pSpawn->MaxInWorld = atoi(TmpStr);
    fgets(Buffer, sizeof(Buffer), SpawnFile);
    TrimRight(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    RoomLookUp(atoi(TmpStr));
    pSpawn->pRoom = pRoom;
    fgets(Buffer, sizeof(Buffer), SpawnFile);
    TrimRight(Buffer);
    strcpy(TmpStr, strchr(Buffer, ':') + 1);
    Trim(TmpStr);
    Word(1, TmpStr, TmpStr1);
    pSpawn->Seconds = atoi(TmpStr1);
    Word(2, TmpStr, TmpStr1);
    pSpawn->Minutes = atoi(TmpStr1);
    Word(3, TmpStr, TmpStr1);
    pSpawn->Hours = atoi(TmpStr1);
    Word(4, TmpStr, TmpStr1);
    pSpawn->Days = atoi(TmpStr1);
    Word(5, TmpStr, TmpStr1);
    pSpawn->Weeks = atoi(TmpStr1);
    Word(6, TmpStr, TmpStr1);
    pSpawn->Months = atoi(TmpStr1);
    Word(7, TmpStr, TmpStr1);
    pSpawn->Years = atoi(TmpStr1);
  }
  fclose(SpawnFile);
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
