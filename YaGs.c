//***************************
//* Yet Another Game Server *
//***************************

#define _DEFAULT_SOURCE           // Required for usleep() in my development environment

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//$$ Includes
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include <pwd.h>                    // getpwuid()
#include <stdbool.h>                // bool data type
#include <stdio.h>                  // sprintf(), fopen(), fprintf(), fclose()
#include <string.h>                 // strlen()
#include <time.h>                   // time()
#include <unistd.h>                 // getuid()

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//$$ Global variables
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Booleans
bool                GameShutDown;       // Set this to true to stop the game
bool                NoPlayers;          // True when we have no players

// Numbers
long int            CurrentTimeSec;     // Current time in seconds
long unsigned int   x;                  // a short lived number

//Pointers
char               *CurrentTime;        // Current timestamp
FILE               *LogFile;            // Log File
struct passwd      *pw;                 // Password struct (used to get $HOME)
char               *HomeDir;            // Value of $HOME

// Strings
char                LogMsg[100];        // Log Message
char                LogFileName[25];    // Log File Name

// Messages
char               *GameSleepMsg = "No Connections: Going to sleep";      // Game sleeping message
char               *GameStartMsg = "YaGs v1.0.2 Starting";                // Game starting message
char               *GameStopMsg  = "YaGs has shutdown";                   // Game stop message
char               *GameWakeMsg  = "Waking up";                           // Game wake up message

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//$$ Macros
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#define DEBUGIT(x)      if (DEBUGIT_LVL >= x) {sprintf(LogMsg,"*** %s ***",__FUNCTION__);LogIt(LogMsg);}
#define DEBUGIT_LVL     1               // Range of 0 to 5 with 0 = No debug messages and 5 = Maximum debug messages
#define YAGS_HOME_DIR   "YaGs"          // YaGs home directory
#define LOG_DIR         "Logs"          // Log directory
#define LOG_FILE        "Log.txt"       // Log file
#define SLEEP_TIME      0400000         // Sleep for a short period of time
#define USE_USLEEP      'N'             // Use usleep() Y or N

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//$$ Functions
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

void CheckForNewPlayers();
void CloseLog();
void GetPlayerInput();
void GetTime();
void HeartBeat();
void LogIt(char *LogMsg);
void OpenLog();
void SendPlayerOutput();
void ShutItDown();
void Sleep();
void StartItUp();

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
}

// Get player input
void GetPlayerInput()
{
  DEBUGIT(1)
}

// Send player output
void SendPlayerOutput()
{
  DEBUGIT(1)
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
  LogIt(GameStartMsg);
}

// Write to log
void LogIt(char *LogMsg)
{
  GetTime();
  fprintf(LogFile, "%s - %s\r\n", CurrentTime, LogMsg);
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
