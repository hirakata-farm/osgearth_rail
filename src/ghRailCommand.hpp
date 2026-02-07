/*
*
*  osgearth_rail viewer
*
*     required OpenSceneGraph ( https://github.com/openscenegraph )
*              osgearth       ( https://github.com/gwaldron/osgearth )
*              curlpp         ( https://www.curlpp.org )
*              nlohmann       ( https://github.com/nlohmann/json )
*
*   Copyright (C) 2025,2026 Yuki Osada
*  This software is released under the BSD License, see LICENSE.
*
*
*/

/**************************************

   Command List

1. Control Command

exit
start
run
stop
pause
show (status|version)

2. Clock Command

clock [set|get] [time|speed] (number)

clock set time 12:12
clock set speed 12 ( 0.1 - 12 )

clock get time
clock get speed


3. Camera Command , [camera name] = default [root]

camera (set|get) [camera name] (position|lookat|upvec|tracking|screen|window) (number) (number) (number)

camera set [ camera name ] position 35.2 53.2 32 ( lng[deg], lat[deg], alt[m] )
camera set [ camera name ] lookat 35.2 53.2 32   ( lng[deg], lat[deg], alt[m] )
camera set [ camera name ] upvec 35.2 53.2 32   ( lng[deg], lat[deg], alt[m] )
camera set [ camera name ] tracking 1241 ( train_number or NONE )
camera set [ camera name ] screen 100 200 ( screen_position_x, screen_position_y )
camera set [ camera name ] window 640 480 ( window_width, window_height ) 
camera add [ camera name ] 0 0 0.2 ( screen_position_x screen_position_y window_size_ratio_from_ScreenSize )
camera remove [ camera name ]

camera get [ camera name ] position
camera get [ camera name ] lookat
camera get [ camera name ] upvec
camera get [ camera name ] tracking
camera get [ camera name ] viewport
camera get [ camera name ] screen
camera get [ camera name ] window
camera get


4. Field Command

field (set|get) [configuration specific ID]

field set G012142123

field get
field get train
field get line
field get timezone
field get description


5. Train Command

field (label|position|timetable|icon|line|distance) [train id]


train label [on|off] [train id]  
train position [train id] 
train timetable [train id]
train icon [train id]
train line [train id]
train distance [train id]

6. Configure Command

config (set|get) (maxclockspeed|altmode|displaydistance|maxwindow) (number)

config set maxclockspeed 30.0 ( default 12.0 )
config set altmode clamp ( clamp(default), absolute, relative )
config set displaydistance 100000 ( default 3000[m] )
config set maxwindow 8 ( default 4 )
config get maxclockspeed
config get altmode
config get displaydistance
config get maxwindow


7. Shared Memory Command ( linux Only yet )

shm (set|remove) (clock|train|camera) (time|position|viewport)

shm set clock time
shm set train position
shm set camera [camera name] viewport
shm remove [ shm key ]



******************************/



# ifndef _GH_RAIL_COMMAND_HPP_
# define _GH_RAIL_COMMAND_HPP_
     
# include <iostream>
# include <map>
# include <vector>
# include <cstdlib>
# include <cstring>
#ifdef _WINDOWS
#include <winsock2.h>
#include <iostream>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#endif

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
#define GH_COMMAND_FIELD_GET_TRAIN     8
#define GH_COMMAND_FIELD_GET_LINE      9
#define GH_COMMAND_FIELD_GET_TIMEZONE 10
#define GH_COMMAND_FIELD_GET_DESC     11

#define GH_COMMAND_CLOCK_SET_TIME  12
#define GH_COMMAND_CLOCK_GET_TIME  13
#define GH_COMMAND_CLOCK_SET_SPEED 14
#define GH_COMMAND_CLOCK_GET_SPEED 15

#define GH_COMMAND_CAMERA_SET_POS      16
#define GH_COMMAND_CAMERA_GET_POS      17
#define GH_COMMAND_CAMERA_SET_LOOK     18
#define GH_COMMAND_CAMERA_GET_LOOK     19
#define GH_COMMAND_CAMERA_SET_UP       20
#define GH_COMMAND_CAMERA_GET_UP       21
#define GH_COMMAND_CAMERA_SET_TRACK    22
#define GH_COMMAND_CAMERA_GET_TRACK    23
#define GH_COMMAND_CAMERA_GET_VIEWPORT 24
#define GH_COMMAND_CAMERA_SET_WINDOW   25
#define GH_COMMAND_CAMERA_GET_WINDOW   26
#define GH_COMMAND_CAMERA_SET_SCREEN   27
#define GH_COMMAND_CAMERA_GET_SCREEN   28
#define GH_COMMAND_CAMERA_ADD          29
#define GH_COMMAND_CAMERA_REMOVE       30
#define GH_COMMAND_CAMERA_GET          31

#define GH_COMMAND_TRAIN_LABEL_ON  32
#define GH_COMMAND_TRAIN_LABEL_OFF 33
#define GH_COMMAND_TRAIN_POSITION  34
#define GH_COMMAND_TRAIN_TIMETABLE 35
#define GH_COMMAND_TRAIN_ICON      36
#define GH_COMMAND_TRAIN_LINE      37
#define GH_COMMAND_TRAIN_DISTANCE  38

#define GH_COMMAND_CONFIG_SET_MAXCLOCKSPEED    39
#define GH_COMMAND_CONFIG_GET_MAXCLOCKSPEED    40
#define GH_COMMAND_CONFIG_SET_ALTMODE          41
#define GH_COMMAND_CONFIG_GET_ALTMODE          42
#define GH_COMMAND_CONFIG_SET_DISPLAYDISTANCE  43
#define GH_COMMAND_CONFIG_GET_DISPLAYDISTANCE  44
#define GH_COMMAND_CONFIG_SET_MAXWINDOW        45
#define GH_COMMAND_CONFIG_GET_MAXWINDOW        46

#define GH_COMMAND_SHM_CLOCK_TIME  47
#define GH_COMMAND_SHM_TRAIN_POS   48
#define GH_COMMAND_SHM_CAMERA_VIEW 49
#define GH_COMMAND_SHM_REMOVE      50

#define GH_NUMBER_OF_COMMANDS      51 //  count for above commands

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
#define GH_STRING_LINE        "line"
#define GH_STRING_DISTANCE    "distance"
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
#define GH_STRING_MAXWINDOW       "maxwindow"

#define GH_STRING_SHOW    "show"
#define GH_STRING_STATUS  "status"
#define GH_STRING_VERSION "version"

#define GH_STRING_SHM    "shm"
#define GH_STRING_REMOVE "remove"

#define GH_STRING_OK "OK"

//////////////////////////////////////////////////
//#define GH_EXECUTE_BUFFER_SIZE 1024
#define GH_EXECUTE_BUFFER_SIZE 8192
#define GH_EXECUTE_INIT -1
#define GH_EXECUTE_SUCCESS 0
#define GH_EXECUTE_UNKNOWN 1
#define GH_EXECUTE_DELAY   2
#define GH_EXECUTE_NOT_LOADED 4
#define GH_EXECUTE_CANNOT_LOAD 11
#define GH_EXECUTE_SIZE_ERROR  13
#define GH_EXECUTE_NOT_FOUND   15
#define GH_EXECUTE_ALREADY_EXIST 17
#define GH_EXECUTE_RESERVED  19
#define GH_EXECUTE_CANNOT_GET  21
#define GH_EXECUTE_CANNOT_ALLOCATE  23

//////////////////////////////////////// Obsolete
#define GH_POST_EXECUTE_NONE 0
#define GH_POST_EXECUTE_DONE 1
#define GH_POST_EXECUTE_EXIT 2
#define GH_POST_EXECUTE_CLOSE 4
#define GH_POST_EXECUTE_TIMEZONE 8
#define GH_POST_EXECUTE_SETCLOCK 9
#define GH_POST_EXECUTE_CAMERA_ADD 12
#define GH_POST_EXECUTE_CAMERA_REMOVE 14
////////////////////////////////////////


#define GH_QUEUE_STATE_INIT 0
#define GH_QUEUE_STATE_RECEIVED 1
#define GH_QUEUE_STATE_PARSED 2
#define GH_QUEUE_STATE_PART_EXECUTED 3
#define GH_QUEUE_STATE_EXECUTED 4
#define GH_QUEUE_STATE_RESULT_SEND 9

////////////////////////////////////////////////

using namespace std;

typedef struct ghCommandQueue
{
  int	type ;
  int   argstridx;
  std::string argstr[2] ;
  int   argnumidx;
  double argnum[3];
  //bool isexecute;
  unsigned int state;
  std::string result;
  ghCommandQueue *prev;
} ghCommandQueue ;

ghCommandQueue *ghRailInitCommandQueue();

void ghRailParseCommand(ghCommandQueue *cmd,string str);
void ghRailExecuteCommandData(ghCommandQueue *cmd, ghRail *rail, double simtime);
void ghRailExecuteCommandOSG(ghCommandQueue *cmd,  ghRail *rail, ghWindow* _win, osgEarth::SkyNode *_sky);

std::string ghRailReturnMessage(ghCommandQueue *cmd,int code, char *message);

int ghRailCommandFieldSetData(ghCommandQueue *cmd, ghRail *rail);
int ghRailCommandFieldSet(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky);
int ghRailCommandFieldGet(ghCommandQueue *cmd, ghRail *rail, char *result);

int ghRailCommandFieldTrain(ghCommandQueue *cmd, ghRail *rail, char *result);
int ghRailCommandFieldLine(ghCommandQueue *cmd, ghRail *rail, char *result);
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
int ghRailCommandTrainLine(ghCommandQueue *cmd, ghRail *rail, char *result);
int ghRailCommandTrainDistance(ghCommandQueue *cmd, ghRail *rail, char *result);

int ghRailCommandCameraSetPosition(ghCommandQueue *cmd,ghWindow* _win);
int ghRailCommandCameraGetPosition(ghCommandQueue *cmd,ghWindow* _win, char *result);
int ghRailCommandCameraSetLookat(ghCommandQueue *cmd, ghWindow* _win);
int ghRailCommandCameraGetLookat(ghCommandQueue *cmd, ghWindow* _win, char *result);
int ghRailCommandCameraSetUpvec(ghCommandQueue *cmd, ghWindow* _win);
int ghRailCommandCameraGetUpvec(ghCommandQueue *cmd, ghWindow* _win, char *result);
int ghRailCommandCameraSetTracking(ghCommandQueue *cmd,ghRail *rail,ghWindow* _win);
int ghRailCommandCameraGetTracking(ghCommandQueue *cmd,ghWindow* _win, char *result);
int ghRailCommandCameraGetViewport(ghCommandQueue *cmd, ghWindow* _win, char *result);
int ghRailCommandCameraSetScreen(ghCommandQueue *cmd,ghWindow* _win);
int ghRailCommandCameraGetScreen(ghCommandQueue *cmd,ghWindow* _win, char *result);
int ghRailCommandCameraSetWindow(ghCommandQueue *cmd,ghWindow* _win);
int ghRailCommandCameraGetWindow(ghCommandQueue *cmd,ghWindow* _win, char *result);
int ghRailCommandCameraAdd(ghCommandQueue *cmd, ghRail *rail, ghWindow* _win);
int ghRailCommandCameraRemove(ghCommandQueue *cmd, ghWindow* _win);
int ghRailCommandCameraGet(ghCommandQueue *cmd, ghWindow* _win, char *result);

int ghRailCommandConfigSetMaxspeed(ghCommandQueue *cmd, ghRail *rail);
int ghRailCommandConfigGetMaxspeed(ghCommandQueue *cmd, ghRail *rail, char *result);
int ghRailCommandConfigSetAltmode(ghCommandQueue *cmd, ghRail *rail);
int ghRailCommandConfigGetAltmode(ghCommandQueue *cmd, ghRail *rail, char *result);
int ghRailCommandConfigSetDisplaydistance(ghCommandQueue *cmd, ghRail *rail);
int ghRailCommandConfigGetDisplaydistance(ghCommandQueue *cmd, ghRail *rail, char *result);
int ghRailCommandConfigSetMaxwindow(ghCommandQueue *cmd, ghRail *rail);
int ghRailCommandConfigGetMaxwindow(ghCommandQueue *cmd, ghRail *rail, char *result);

int ghRailCommandShmSet(int shmtype,ghCommandQueue *cmd,ghRail *rail,ghWindow* _win, char *result);
int ghRailCommandShmRemove(ghCommandQueue *cmd,ghRail *rail);

int ghRailCommandShowStatus(ghCommandQueue *cmd, ghRail *rail, ghWindow* _win, char *result);
int ghRailCommandShowVersion(char *result);


#endif

