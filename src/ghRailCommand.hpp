/*
*
*  osgearth_rail viewer
*
*     required OpenSceneGraph ( https://github.com/openscenegraph )
*              osgearth       ( https://github.com/gwaldron/osgearth )
*              curlpp         ( https://www.curlpp.org )
*              nlohmann       ( https://github.com/nlohmann/json )
*
*   Copyright (C) 2025 Yuki Osada
*  This software is released under the BSD License, see LICENSE.
*
*
*/

/**************************************

   Command List

exit

start | run
stop | pause


clock [set|get] [time|speed] (number)

clock set time 12:12
clock set speed 12 ( 0.1 - 12 )

clock get time
clock get speed

camera [set|get] [camera name] [position|lookat|upvec] (number) (number) (number)

[camera name] IS default [root]

camera set root position 35.2 53.2 32 ( lng[deg], lat[deg], alt[m] )
camera set root lookat 35.2 53.2 32   ( lng[deg], lat[deg], alt[m] )
camera set root upvec 35.2 53.2 32   ( lng[deg], lat[deg], alt[m] )
camera set root tracking 1241 ( train number or NONE )
camera set root screen 100 200 ( screenX, screenY )
camera set root window 640 480 ( window width, window height )
camera add ( camera name ) 
camera remove ( camera name )

camera get root position
camera get root lookat
camera get root upvec
camera get root tracking
camera get root viewport
camera get root screen
camera get root window

field [set|get] (specific ID)

field [set|get] (text)
field set G012142123

field get
field get train
field get timezone
field get description


train label {on|off} (train id)  
train position (train id) 
train timetable (train id)
train icon (train id) 

config set maxclockspeed 30.0 ( default 12.0 )
config set altmode clamp ( clamp(default), absolute, relative )
config set displaydistance 100000 ( default 3000[m] )
config get maxclockspeed
config get altmode
config get displaydistance

shm set clock time
shm set train position
shm set camera [camera name] viewport
shm remove ( shm key )

show [status|version]

show status
show version




******************************/



# ifndef _GH_RAIL_COMMAND_HPP_
# define _GH_RAIL_COMMAND_HPP_
     
# include <iostream>
# include <map>
# include <vector>
# include <cstdlib>
# include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <osgEarth/DateTime>
#include <osgEarth/Sky>
#include <osgViewer/Viewer>

////////////////////////////////////////////////////////////////
#define GH_COMMAND_UNKNOWN  -1
#define GH_COMMAND_EXIT   0
#define GH_COMMAND_CLOSE  1
#define GH_COMMAND_START  2
#define GH_COMMAND_STOP   3
#define GH_COMMAND_SHOW_STATUS  4
#define GH_COMMAND_SHOW_VERSION 5

#define GH_COMMAND_FIELD_SET  6
#define GH_COMMAND_FIELD_GET  7
#define GH_COMMAND_FIELD_GET_TRAIN  8
#define GH_COMMAND_FIELD_GET_TIMEZONE  9
#define GH_COMMAND_FIELD_GET_DESC  10

#define GH_COMMAND_CLOCK_SET_TIME  11
#define GH_COMMAND_CLOCK_GET_TIME  12
#define GH_COMMAND_CLOCK_SET_SPEED 13
#define GH_COMMAND_CLOCK_GET_SPEED 14

#define GH_COMMAND_CAMERA_SET_POS   15
#define GH_COMMAND_CAMERA_GET_POS   16
#define GH_COMMAND_CAMERA_SET_LOOK  17
#define GH_COMMAND_CAMERA_GET_LOOK  18
#define GH_COMMAND_CAMERA_SET_UP    19
#define GH_COMMAND_CAMERA_GET_UP    20
#define GH_COMMAND_CAMERA_SET_TRACK 21
#define GH_COMMAND_CAMERA_GET_TRACK 22
#define GH_COMMAND_CAMERA_GET_VIEWPORT 23
#define GH_COMMAND_CAMERA_SET_WINDOW 24
#define GH_COMMAND_CAMERA_GET_WINDOW 25
#define GH_COMMAND_CAMERA_SET_SCREEN 26
#define GH_COMMAND_CAMERA_GET_SCREEN 27
#define GH_COMMAND_CAMERA_ADD    28
#define GH_COMMAND_CAMERA_REMOVE 29

#define GH_COMMAND_TRAIN_LABEL_ON  30
#define GH_COMMAND_TRAIN_LABEL_OFF 31
#define GH_COMMAND_TRAIN_POSITION  32
#define GH_COMMAND_TRAIN_TIMETABLE 33
#define GH_COMMAND_TRAIN_ICON      34

#define GH_COMMAND_CONFIG_SET_MAXCLOCKSPEED    35
#define GH_COMMAND_CONFIG_GET_MAXCLOCKSPEED    36
#define GH_COMMAND_CONFIG_SET_ALTMODE          37
#define GH_COMMAND_CONFIG_GET_ALTMODE          38
#define GH_COMMAND_CONFIG_SET_DISPLAYDISTANCE  39
#define GH_COMMAND_CONFIG_GET_DISPLAYDISTANCE  40

#define GH_COMMAND_SHM_CLOCK_TIME  41
#define GH_COMMAND_SHM_TRAIN_POS   42
#define GH_COMMAND_SHM_CAMERA_VIEW 43
#define GH_COMMAND_SHM_REMOVE      44

#define GH_NUMBER_OF_COMMANDS      45 //  count for above commands

////////////////////////////////////////////////////
//
//   Command Reserved Keywords
//
//

#define GH_STRING_SET "set"
#define GH_STRING_GET "get"
#define GH_STRING_NOP "nop"

#define GH_STRING_EXIT   "exit"
#define GH_STRING_CLOSE  "close"
#define GH_STRING_START  "start"
#define GH_STRING_RUN    "run"
#define GH_STRING_STOP   "stop"
#define GH_STRING_PAUSE  "pause"

#define GH_STRING_CLOCK  "clock"
#define GH_STRING_TIME   "time"
#define GH_STRING_SPEED  "speed"

#define GH_STRING_CAMERA    "camera"
#define GH_STRING_POSITION  "position"
#define GH_STRING_LOOKAT    "lookat"
#define GH_STRING_UPVEC     "upvec"
#define GH_STRING_TRACKING  "tracking"
#define GH_STRING_VIEWPORT  "viewport"
#define GH_STRING_ROOT      "root"
#define GH_STRING_SCREEN    "screen"
#define GH_STRING_WINDOW    "window"
#define GH_STRING_NONE      "none"
#define GH_STRING_ALL       "all"
#define GH_STRING_ADD       "add"

#define GH_STRING_FIELD       "field"
#define GH_STRING_TRAIN       "train"
#define GH_STRING_TIMEZONE    "timezone"
#define GH_STRING_DESCRIPTION "description"

#define GH_STRING_LABEL      "label"
#define GH_STRING_ON         "on"
#define GH_STRING_OFF        "off"
#define GH_STRING_TIMETABLE  "timetable"
#define GH_STRING_ICON       "icon"

#define GH_STRING_CONFIG          "config"
#define GH_STRING_MAXCLOCKSPEED   "maxclockspeed"
#define GH_STRING_ALTMODE         "altmode"
#define GH_STRING_CLAMP           "clamp"
#define GH_STRING_RELATIVE        "relative"
#define GH_STRING_ABSOLUTE        "absolute"
#define GH_STRING_DISPLAYDISTANCE "displaydistance"

#define GH_STRING_SHOW    "show"
#define GH_STRING_STATUS  "status"
#define GH_STRING_VERSION "version"

#define GH_STRING_SHM    "shm"
#define GH_STRING_REMOVE "remove"

#define GH_STRING_OK "OK"

//////////////////////////////////////////////////
#define GH_EXECUTE_BUFFER_SIZE 1024
#define GH_EXECUTE_SUCCESS 0
#define GH_EXECUTE_UNKNOWN 1
#define GH_EXECUTE_NOT_LOADED 4
#define GH_EXECUTE_CANNOT_LOAD 11
#define GH_EXECUTE_SIZE_ERROR  13
#define GH_EXECUTE_NOT_FOUND   15
#define GH_EXECUTE_ALREADY_EXIST 17
#define GH_EXECUTE_RESERVED  19
#define GH_EXECUTE_CANNOT_GET  21
#define GH_EXECUTE_CANNOT_ALLOCATE  23


#define GH_POST_EXECUTE_NONE 0
#define GH_POST_EXECUTE_EXIT 2
#define GH_POST_EXECUTE_CLOSE 4
#define GH_POST_EXECUTE_TIMEZONE 8
#define GH_POST_EXECUTE_SETCLOCK 9
#define GH_POST_EXECUTE_CAMERA_ADD 12
#define GH_POST_EXECUTE_CAMERA_REMOVE 14

////////////////////////////////////////////////

using namespace std;

typedef struct ghCommandQueue
{
  int	count ;
  int	type ;
  int   argstridx;
  string argstr[2] ;
  int   argnumidx;
  double argnum[3];
  bool isexecute;
  ghCommandQueue *prev;
} ghCommandQueue ;

std::vector<std::string> ghStringSplit(std::string str, char del);
ghCommandQueue *ghRailParseCommand(string str);
int ghRailExecuteCommand(ghCommandQueue *cmd,
			 int socket,
			 ghRail *rail,
			 ghWindow* _win,
			 osgEarth::SkyNode *_sky,
			 double simtime);
std::string ghRailReturnMessage(ghCommandQueue *cmd,int code, char *message);

int ghRailCommandFieldSet(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky);
int ghRailCommandFieldGet(ghCommandQueue *cmd, ghRail *rail, char *result);

int ghRailCommandFieldTrain(ghCommandQueue *cmd, ghRail *rail, char *result);
int ghRailCommandFieldTimezone(ghCommandQueue *cmd, ghRail *rail, char *result);
int ghRailCommandFieldDescription(ghCommandQueue *cmd, ghRail *rail, char *result);

int ghRailCommandClockSetTime(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky);
int ghRailCommandClockGetTime(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky, char *result);
int ghRailCommandClockSetSpeed(ghCommandQueue *cmd, ghRail *rail);
int ghRailCommandClockGetSpeed(ghCommandQueue *cmd, ghRail *rail, char *result);

int ghRailCommandTrainLabel(ghCommandQueue *cmd, ghRail *rail,bool flag);
int ghRailCommandTrainPosition(ghCommandQueue *cmd, ghRail *rail, double simtime, char *result);
int ghRailCommandTrainTimetable(ghCommandQueue *cmd, ghRail *rail, char *result);
int ghRailCommandTrainIcon(ghCommandQueue *cmd, ghRail *rail, char *result);

int ghRailCommandCameraSetPosition(ghCommandQueue *cmd,ghWindow* _win);
int ghRailCommandCameraGetPosition(ghCommandQueue *cmd,ghWindow* _win, char *result);
int ghRailCommandCameraSetLookat(ghCommandQueue *cmd, ghWindow* _win);
int ghRailCommandCameraGetLookat(ghCommandQueue *cmd, ghWindow* _win, char *result);
int ghRailCommandCameraSetUpvec(ghCommandQueue *cmd, ghWindow* _win);
int ghRailCommandCameraGetUpvec(ghCommandQueue *cmd, ghWindow* _win, char *result);
int ghRailCommandCameraSetTracking(ghCommandQueue *cmd,ghRail *rail,ghWindow* _win);
int ghRailCommandCameraGetTracking(ghCommandQueue *cmd,ghWindow* _win, char *result);
int ghRailCommandCameraViewport(ghCommandQueue *cmd, ghWindow* _win, char *result);
int ghRailCommandCameraSetScreen(ghCommandQueue *cmd,ghWindow* _win);
int ghRailCommandCameraGetScreen(ghCommandQueue *cmd,ghWindow* _win, char *result);
int ghRailCommandCameraSetWindow(ghCommandQueue *cmd,ghWindow* _win);
int ghRailCommandCameraGetWindow(ghCommandQueue *cmd,ghWindow* _win, char *result);
int ghRailCommandCameraAdd(ghCommandQueue *cmd, ghWindow* _win);
int ghRailCommandCameraRemove(ghCommandQueue *cmd, ghWindow* _win);

int ghRailCommandConfigSetMaxspeed(ghCommandQueue *cmd, ghRail *rail);
int ghRailCommandConfigGetMaxspeed(ghCommandQueue *cmd, ghRail *rail, char *result);
int ghRailCommandConfigSetAltmode(ghCommandQueue *cmd, ghRail *rail);
int ghRailCommandConfigGetAltmode(ghCommandQueue *cmd, ghRail *rail, char *result);
int ghRailCommandConfigSetDisplaydistance(ghCommandQueue *cmd, ghRail *rail);
int ghRailCommandConfigGetDisplaydistance(ghCommandQueue *cmd, ghRail *rail, char *result);

int ghRailCommandShmSet(int shmtype,ghCommandQueue *cmd,ghRail *rail,ghWindow* _win, char *result);
int ghRailCommandShmRemove(ghCommandQueue *cmd,ghRail *rail);

int ghRailCommandShowStatus(ghCommandQueue *cmd, ghRail *rail, ghWindow* _win, char *result);
int ghRailCommandShowVersion(char *result);


#endif

