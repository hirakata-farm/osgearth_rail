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


JSON format

{
"utime" : 1012023121,
"cmd" : 10,
"name" : "",
"args" : [ "arg0", "arg1", ... ],
"size" : 1,
"rev" : 1.0,
}

{
"utime" : 1012023121,
"status" : 0,
"result" : [ "result0", "result1", ... ],
"size" : 2,
"recv" : { ... }
}

---------------------------------------

[key][type][command]


--command--
exit   = 10
play   = 12
pause  = 15
show   = 17
clock  = 21
camera = 22
field  = 23
train  = 24
config = 25
shm    = 26


--type--
set       = 1100
get       = 1200
add       = 1300
remove    = 1400
status    = 2100
version   = 2200
label     = 2300 
setclock  = 2400
settrain  = 2500
setcamera = 2600


--key--
time            = 140000
speed           = 150000
position        = 160000
lookat          = 170000
upvec           = 180000
tracking        = 190000
screen          = 200000
window          = 210000
viewport        = 220000
all             = 230000
id              = 240000
train           = 250000
line            = 260000
timezone        = 270000
description     = 280000
label           = 290000
on              = 300000
off             = 310000
timetable       = 320000
icon            = 330000
distance        = 340000
maxclockspeed   = 410000
altmode         = 420000
displaydistance = 430000
maxcamera       = 440000


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
camera set [ camera name ] tracking 1241 ( train_number or none )
camera set [ camera name ] screen 100 200 ( screen_position_x, screen_position_y )
camera set [ camera name ] window 640 480 ( window_width, window_height )

camera add [ camera name ] 0 0 640 480 ( screen_position_x screen_position_y window_size_x window_size_y  )
camera remove [ camera name ]

camera get [ camera name ] position
camera get [ camera name ] lookat
camera get [ camera name ] upvec
camera get [ camera name ] tracking
camera get [ camera name ] viewport
camera get [ camera name ] screen
camera get [ camera name ] window
camera get all


4. Field Command

field (set|get) [configuration specific ID]

field set G012142123

field get
field get train
field get line
field get timezone
field get description


5. Train Command

train (label|position|timetable|icon|line|distance) [train id]


train label [train id] [on|off] 
train position [train id] 
train timetable [train id]
train icon [train id]
train line [train id]
train distance [train id]

6. Configure Command

config (set|get) (maxclockspeed|altmode|displaydistance|maxcamera) (number)

config set maxclockspeed 30.0 ( default 12.0 )
config set altmode clamp ( clamp(default), absolute, relative )
config set displaydistance 100000 ( default 3000[m] )
config set maxcamera 8 ( default 4 )
config get maxclockspeed
config get altmode
config get displaydistance
config get maxcamera


7. Shared Memory Command ( windows or linux(SYSV) )

shm (set|remove) (clock|train|camera) (time|position|viewport)

shm set clock time
shm set train position
shm set camera [camera name] viewport
shm remove [ shm key name ]



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


////////////////////////////////////////////////////
//
//   Command Reserved Keywords
//
//
#define GH_STRING_COMMAND_PREFIX "Geoglyph: "

#define GH_MOD_2DIGITS     100
#define GH_MOD_4DIGITS   10000


#define GH_COMMAND_EXIT   10
#define GH_COMMAND_PLAY   12
#define GH_COMMAND_PAUSE  15
#define GH_COMMAND_SHOW   17
#define GH_COMMAND_CLOCK  21
#define GH_COMMAND_CAMERA 22
#define GH_COMMAND_FIELD  23
#define GH_COMMAND_TRAIN  24
#define GH_COMMAND_CONFIG 25
#define GH_COMMAND_SHM    26

#define GH_JSON_COMMAND_STRING  "cmd"
#define GH_JSON_NAME_STRING     "name"
#define GH_JSON_SIZE_STRING     "size"
#define GH_JSON_ARGS_STRING     "args"
#define GH_JSON_UTIME_STRING    "utime"
#define GH_JSON_STATUS_STRING   "status"
#define GH_JSON_RESULT_STRING   "result"
#define GH_JSON_REVISION_STRING "rev"


#define GH_TYPE_SET       1100
#define GH_TYPE_GET       1200
#define GH_TYPE_ADD       1300
#define GH_TYPE_REMOVE    1400
#define GH_TYPE_STATUS    2100
#define GH_TYPE_VERSION   2200
#define GH_TYPE_LABEL     2300 
#define GH_TYPE_SETCLOCK  2400
#define GH_TYPE_SETTRAIN  2500
#define GH_TYPE_SETCAMERA 2600


#define GH_KEY_TIME            140000
#define GH_KEY_SPEED           150000
#define GH_KEY_POSITION        160000
#define GH_KEY_LOOKAT          170000
#define GH_KEY_UPVEC           180000
#define GH_KEY_TRACKING        190000
#define GH_KEY_SCREEN          200000
#define GH_KEY_WINDOW          210000
#define GH_KEY_VIEWPORT        220000
#define GH_KEY_ALL             230000
#define GH_KEY_ID              240000
#define GH_KEY_TRAIN           250000
#define GH_KEY_LINE            260000
#define GH_KEY_TIMEZONE        270000
#define GH_KEY_DESCRIPTION     280000
#define GH_KEY_LABEL           290000
#define GH_KEY_ON              300000
#define GH_KEY_OFF             310000
#define GH_KEY_TIMETABLE       320000
#define GH_KEY_ICON            330000
#define GH_KEY_DISTANCE        340000
#define GH_KEY_MAXCLOCKSPEED   410000
#define GH_KEY_ALTMODE         420000
#define GH_KEY_DISPLAYDISTANCE 430000
#define GH_KEY_MAXCAMERA       440000


#define GH_QUEUE_STATUS_INIT          0
#define GH_QUEUE_STATUS_RECEIVED      1
#define GH_QUEUE_STATUS_PART_EXECUTED 4
#define GH_QUEUE_STATUS_EXECUTED      8
#define GH_QUEUE_STATUS_RESULT_SEND  16


#define GH_QUEUE_RETURN_CODE_NONE    0
#define GH_QUEUE_RETURN_CODE_SUCCESS 1
#define GH_QUEUE_RETURN_CODE_WARNING 2
#define GH_QUEUE_RETURN_CODE_ERROR   8


typedef struct ghCommandQueue
{
  nlohmann::json recv;
  unsigned int status;
  std::string resultmsg;
  int resultcode;
  int resultsize;
  ghCommandQueue *prev;
} ghCommandQueue ;

ghCommandQueue *ghRailInitCommandQueue(std::string buffer);
int ghRailGetCommandID(ghCommandQueue *cmd);
int ghRailGetCommandCode(ghCommandQueue *cmd);
int ghRailGetCommandType(ghCommandQueue *cmd);
int ghRailGetCommandKey(ghCommandQueue *cmd);

void ghRailGetCommandFloatVector(ghCommandQueue *cmd, float *vec);
std::string ghRailGetCommandName(ghCommandQueue *cmd);
std::string ghRailGetCommandFirstArgString(ghCommandQueue *cmd);

void ghRailExecuteCommand(ghCommandQueue *cmd, ghRail *rail, double simtime,osgEarth::SkyNode *_sky,std::map<std::string, ghWindow>& _wins);

std::string ghRailCreateResultObject(ghCommandQueue *cmd);


void ghRailCommandFieldSetData(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandFieldGetID(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandFieldGetTrain(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandFieldGetLine(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandFieldGetTimezone(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandFieldGetDescription(ghCommandQueue *cmd, ghRail *rail);


void ghRailCommandClockSetTime(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky);
void ghRailCommandClockGetTime(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky);
void ghRailCommandClockSetSpeed(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandClockGetSpeed(ghCommandQueue *cmd, ghRail *rail);


void ghRailCommandTrainLabel(ghCommandQueue *cmd, ghRail *rail,bool flag);
void ghRailCommandTrainPosition(ghCommandQueue *cmd, ghRail *rail, double simtime);
void ghRailCommandTrainTimetable(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandTrainIcon(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandTrainLine(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandTrainDistance(ghCommandQueue *cmd, ghRail *rail);



void ghRailCommandCameraSetTracking(ghCommandQueue *cmd,ghRail *rail,std::map<std::string, ghWindow>& _wins);
void ghRailCommandCameraGetTracking(ghCommandQueue *cmd, std::map<std::string, ghWindow>& _wins );
void ghRailCommandCameraSetPosition(ghCommandQueue *cmd, std::map<std::string, ghWindow>& _wins );
void ghRailCommandCameraGetPosition(ghCommandQueue *cmd, std::map<std::string, ghWindow>& _wins );
void ghRailCommandCameraSetLookat(ghCommandQueue *cmd, std::map<std::string, ghWindow>& _wins );
void ghRailCommandCameraGetLookat(ghCommandQueue *cmd, std::map<std::string, ghWindow>& _wins );
void ghRailCommandCameraSetUpvec(ghCommandQueue *cmd, std::map<std::string, ghWindow>& _wins );
void ghRailCommandCameraGetUpvec(ghCommandQueue *cmd, std::map<std::string, ghWindow>& _wins );

void ghRailCommandCameraGetViewport(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins );
void ghRailCommandCameraSetScreen(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins );
void ghRailCommandCameraGetScreen(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins );
void ghRailCommandCameraSetWindow(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins );
void ghRailCommandCameraGetWindow(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins );
void ghRailCommandCameraAdd(ghCommandQueue *cmd, ghRail *rail, std::map<std::string, ghWindow>& _wins );
void ghRailCommandCameraRemove(ghCommandQueue *cmd, std::map<std::string, ghWindow>& _wins );
void ghRailCommandCameraGetALL(ghCommandQueue *cmd, std::map<std::string, ghWindow>& _wins);


void ghRailCommandShowStatus(ghCommandQueue *cmd, ghRail *rail, std::map<std::string, ghWindow>& _wins);
void ghRailCommandShowVersion(ghCommandQueue *cmd);

void ghRailCommandExit(ghCommandQueue *cmd);
void ghRailCommandPlayPause(ghCommandQueue *cmd, ghRail *rail, bool flag);


void ghRailCommandConfigSetMaxspeed(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandConfigGetMaxspeed(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandConfigSetAltmode(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandConfigGetAltmode(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandConfigSetDisplaydistance(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandConfigGetDisplaydistance(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandConfigSetMaxcamera(ghCommandQueue *cmd, ghRail *rail);
void ghRailCommandConfigGetMaxcamera(ghCommandQueue *cmd, ghRail *rail);

void ghRailCommandShmSet(ghCommandQueue *cmd,ghRail *rail, std::map<std::string, ghWindow>& _wins);
void ghRailCommandShmRemove(ghCommandQueue *cmd,ghRail *rail, std::map<std::string, ghWindow>& _wins);

#endif

