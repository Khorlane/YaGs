//***************************
//* Yet Another Game Server *
//***************************

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//$$ Includes
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include <pwd.h>                    // getpwuid()
#include <stdio.h>                  // sprintf(), fopen(), fprintf(), fclose()
#include <string.h>                 // strlen()
#include <time.h>                   // time()
#include <unistd.h>                 // getuid()

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//$$ Global variables
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

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
char               *GameStartMsg = "YaGs v1.0.1 Starting";                // Game starting message
char               *GameStopMsg  = "YaGs has shutdown";                   // Game stop message

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//$$ Macros
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#define DEBUGIT(x)      if (DEBUGIT_LVL >= x) {sprintf(LogMsg,"*** %s ***",__FUNCTION__);LogIt(LogMsg);}
#define DEBUGIT_LVL     1               // Range of 0 to 5 with 0 = No debug messages and 5 = Maximum debug messages
#define YAGS_HOME_DIR   "YaGs"          // YaGs home directory
#define LOG_DIR         "Logs"          // Log directory
#define LOG_FILE        "Log.txt"       // Log file

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//$$ Functions
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

void CloseLog();
void GetTime();
void HeartBeat();
void LogIt(char *LogMsg);
void OpenLog();
void ShutItDown();
void StartItUp();

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//$$ Main
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

int main(int argc, char **argv)
{
  StartItUp();
  HeartBeat();
  ShutItDown();
}

// Start the game up
void StartItUp()
{
  pw = getpwuid(getuid());
  HomeDir = pw->pw_dir;
  OpenLog();
}

// Heart beat
void HeartBeat()
{
  DEBUGIT(1)
  x = 1;
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
