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
#include <osgEarth/DateTime>
#include <osgEarth/Sky>
#include <osgViewer/Viewer>


#define GH_COMMAND_UNKNOWN  -1
#define GH_COMMAND_EXIT   0
#define GH_COMMAND_CLOSE  1
#define GH_COMMAND_START  2
#define GH_COMMAND_STOP   4

#define GH_COMMAND_FIELD_SET  8
#define GH_COMMAND_FIELD_GET  9
#define GH_COMMAND_FIELD_GET_TRAIN  10
#define GH_COMMAND_FIELD_GET_TIMEZONE  11
#define GH_COMMAND_FIELD_GET_DESC  12

#define GH_COMMAND_SHOW_STATUS  16
#define GH_COMMAND_SHOW_VERSION 17

#define GH_COMMAND_CLOCK_SET_TIME  32
#define GH_COMMAND_CLOCK_SET_SPEED 33
#define GH_COMMAND_CLOCK_GET_TIME  42
#define GH_COMMAND_CLOCK_GET_SPEED 43

#define GH_COMMAND_CAMERA_SET_POS   64
#define GH_COMMAND_CAMERA_SET_LOOK  65
#define GH_COMMAND_CAMERA_SET_UP    66
#define GH_COMMAND_CAMERA_SET_TRACK 67
#define GH_COMMAND_CAMERA_GET_POS   74
#define GH_COMMAND_CAMERA_GET_LOOK  75
#define GH_COMMAND_CAMERA_GET_UP    76
#define GH_COMMAND_CAMERA_GET_TRACK 77
#define GH_COMMAND_CAMERA_GET_VIEWPORT 78

#define GH_COMMAND_TRAIN_LABEL_ON  128
#define GH_COMMAND_TRAIN_LABEL_OFF 129
#define GH_COMMAND_TRAIN_POSITION  138
#define GH_COMMAND_TRAIN_TIMETABLE 148

#define GH_POST_EXECUTE_NONE 0
#define GH_POST_EXECUTE_EXIT 2
#define GH_POST_EXECUTE_CLOSE 4
#define GH_POST_EXECUTE_TIMEZONE 8
#define GH_POST_EXECUTE_SETCLOCK 9

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

std::string ghRailCommandField(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky,bool isset);
std::string ghRailCommandFieldDetail(ghCommandQueue *cmd, ghRail *rail);

std::string ghRailCommandClock(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky,bool isset);
std::string ghRailCommandSpeed(ghCommandQueue *cmd, ghRail *rail, bool isset);
std::string ghRailCommandShow(ghCommandQueue *cmd, ghRail *rail);

std::string ghRailCommandTrainLabel(ghCommandQueue *cmd, ghRail *rail, bool isset);
std::string ghRailCommandTrainPosition(ghCommandQueue *cmd, ghRail *rail, double simtime);
std::string ghRailCommandTrainTimetable(ghCommandQueue *cmd, ghRail *rail);

std::string ghRailCommandCameraPosition(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view,bool isset);
std::string ghRailCommandCameraLookat(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view,bool isset);
std::string ghRailCommandCameraUpvec(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view,bool isset);
std::string ghRailCommandCameraTracking(ghCommandQueue *cmd, ghRail *rail,bool isset);
std::string ghRailCommandCameraViewport(ghCommandQueue *cmd, osgViewer::Viewer* _view);


#endif

