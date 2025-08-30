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

camera [set|get] [position|lookat|upvec] (number) (number) (number)

camera set position 35.2 53.2 32 ( lng[deg], lat[deg], alt[m] )
camera set lookat 35.2 53.2 32   ( lng[deg], lat[deg], alt[m] )
camera set upvec 35.2 53.2 32   ( lng[deg], lat[deg], alt[m] )
camera set tracking 1241 ( train number or NONE )

camera get position
camera get lookat
camera get upvec
camera get tracking
camera get viewport

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
shm set camera viewport
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

#define GH_COMMAND_TRAIN_LABEL_ON  24
#define GH_COMMAND_TRAIN_LABEL_OFF 25
#define GH_COMMAND_TRAIN_POSITION  26
#define GH_COMMAND_TRAIN_TIMETABLE 27
#define GH_COMMAND_TRAIN_ICON      28

#define GH_COMMAND_CONFIG_SET_MAXCLOCKSPEED    29
#define GH_COMMAND_CONFIG_GET_MAXCLOCKSPEED    30
#define GH_COMMAND_CONFIG_SET_ALTMODE          31
#define GH_COMMAND_CONFIG_GET_ALTMODE          32
#define GH_COMMAND_CONFIG_SET_DISPLAYDISTANCE  33
#define GH_COMMAND_CONFIG_GET_DISPLAYDISTANCE  34

#define GH_COMMAND_SHM_CLOCK_TIME  35
#define GH_COMMAND_SHM_TRAIN_POS   36
#define GH_COMMAND_SHM_CAMERA_VIEW 37
#define GH_COMMAND_SHM_REMOVE      38

#define GH_NUMBER_OF_COMMANDS                  39  //  count for above commands

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
#define GH_STRING_NONE      "none"
#define GH_STRING_ALL       "all"

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


//////////////////////////////////////////////////
#define GH_POST_EXECUTE_NONE 0
#define GH_POST_EXECUTE_EXIT 2
#define GH_POST_EXECUTE_CLOSE 4
#define GH_POST_EXECUTE_TIMEZONE 8
#define GH_POST_EXECUTE_SETCLOCK 9


////////////////////////////////////////////////

using namespace std;

typedef struct ghCommandQueue
{
  int	count ;
  int	type ;
  string argstr ;
  double argnum[3];
  bool isexecute;
  //string msg;
  ghCommandQueue *prev;
} ghCommandQueue ;


std::vector<std::string> ghStringSplit(std::string str, char del);
ghCommandQueue *ghRailParseCommand(string str);
int ghRailExecuteCommand(ghCommandQueue *cmd,
			 int socket,
			 ghRail *rail,
			 osgViewer::Viewer* _view,
			 osgEarth::SkyNode *_sky,
			 double simtime);

std::string ghRailCommandStart(ghCommandQueue *cmd, ghRail *rail);
std::string ghRailCommandStop(ghCommandQueue *cmd, ghRail *rail);

std::string ghRailCommandFieldSet(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky);
std::string ghRailCommandFieldGet(ghCommandQueue *cmd, ghRail *rail);

std::string ghRailCommandFieldTrain(ghCommandQueue *cmd, ghRail *rail);
std::string ghRailCommandFieldTimezone(ghCommandQueue *cmd, ghRail *rail);
std::string ghRailCommandFieldDescription(ghCommandQueue *cmd, ghRail *rail);

std::string ghRailCommandClockSetTime(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky);
std::string ghRailCommandClockGetTime(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky);
std::string ghRailCommandClockSetSpeed(ghCommandQueue *cmd, ghRail *rail);
std::string ghRailCommandClockGetSpeed(ghCommandQueue *cmd, ghRail *rail);

std::string ghRailCommandTrainLabelOn(ghCommandQueue *cmd, ghRail *rail);
std::string ghRailCommandTrainLabelOff(ghCommandQueue *cmd, ghRail *rail);
std::string ghRailCommandTrainPosition(ghCommandQueue *cmd, ghRail *rail, double simtime);
std::string ghRailCommandTrainTimetable(ghCommandQueue *cmd, ghRail *rail);
std::string ghRailCommandTrainIcon(ghCommandQueue *cmd, ghRail *rail);

std::string ghRailCommandCameraSetPosition(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view);
std::string ghRailCommandCameraGetPosition(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view);
std::string ghRailCommandCameraSetLookat(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view);
std::string ghRailCommandCameraGetLookat(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view);
std::string ghRailCommandCameraSetUpvec(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view);
std::string ghRailCommandCameraGetUpvec(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view);
std::string ghRailCommandCameraSetTracking(ghCommandQueue *cmd, ghRail *rail);
std::string ghRailCommandCameraGetTracking(ghCommandQueue *cmd, ghRail *rail);
std::string ghRailCommandCameraViewport(ghCommandQueue *cmd, osgViewer::Viewer* _view);

std::string ghRailCommandConfigSetMaxspeed(ghCommandQueue *cmd, ghRail *rail);
std::string ghRailCommandConfigGetMaxspeed(ghCommandQueue *cmd, ghRail *rail);
std::string ghRailCommandConfigSetAltmode(ghCommandQueue *cmd, ghRail *rail);
std::string ghRailCommandConfigGetAltmode(ghCommandQueue *cmd, ghRail *rail);
std::string ghRailCommandConfigSetDisplaydistance(ghCommandQueue *cmd, ghRail *rail);
std::string ghRailCommandConfigGetDisplaydistance(ghCommandQueue *cmd, ghRail *rail);

std::string ghRailCommandShmSet(int shmtype,ghCommandQueue *cmd,ghRail *rail);
std::string ghRailCommandShmRemove(ghCommandQueue *cmd,ghRail *rail);

std::string ghRailCommandShowStatus(ghCommandQueue *cmd, ghRail *rail);
std::string ghRailCommandShowVersion();


#endif

