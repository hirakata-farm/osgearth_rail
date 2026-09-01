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

# include "ghString.hpp"
# include "ghRail.hpp"
# include "ghRailCommand.hpp"

std::vector<std::string> GH_RESERVED_STRING =
{
  GH_STRING_NONE,
  GH_STRING_ROOT,
  GH_STRING_ALL,
  GH_STRING_EXIT,
  GH_STRING_PLAY,
  GH_STRING_PAUSE,
  GH_STRING_ON,
  GH_STRING_OFF,
  GH_STRING_CLAMP,
  GH_STRING_RELATIVE,
  GH_STRING_ABSOLUTE,
  GH_JSON_COMMAND_STRING,
  GH_JSON_NAME_STRING,
  GH_JSON_SIZE_STRING,
  GH_JSON_ARGS_STRING,
  GH_JSON_UTIME_STRING,
  GH_JSON_STATUS_STRING,
  GH_JSON_RESULT_STRING,
  GH_JSON_REVISION_STRING
};

std::string
GH_COMMAND_ALTMODE_STRING[3] = {
  GH_STRING_CLAMP,
  GH_STRING_RELATIVE,
  GH_STRING_ABSOLUTE
};



ghCommandQueue *
ghRailInitCommandQueue(std::string buffer) {


  ghCommandQueue *cmd;
  if ((cmd = (ghCommandQueue *)calloc( (unsigned)1, (unsigned)sizeof( ghCommandQueue ) )) == NULL)
    {
      return( (ghCommandQueue *)NULL ) ;
    }

  cmd->recv = nlohmann::json::parse(buffer);
  cmd->status = GH_QUEUE_STATUS_RECEIVED;
  cmd->resultmsg = GH_STRING_NONE;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_NONE;
  cmd->resultsize = 0;
  cmd->prev = (ghCommandQueue *)NULL;
  return cmd;
}

int
ghRailGetCommandID(ghCommandQueue *cmd) {
  return cmd->recv[GH_JSON_COMMAND_STRING];
}
int
ghRailGetCommandCode(ghCommandQueue *cmd) {
  int code = cmd->recv[GH_JSON_COMMAND_STRING];
  return (code % GH_MOD_2DIGITS );
}
int
ghRailGetCommandType(ghCommandQueue *cmd) {
  int code = cmd->recv[GH_JSON_COMMAND_STRING];
  int com = ( code % GH_MOD_2DIGITS );
  code = code - com;
  return (code % GH_MOD_4DIGITS );
}
int
ghRailGetCommandKey(ghCommandQueue *cmd) {
  int code = cmd->recv[GH_JSON_COMMAND_STRING];
  int com = ( code % GH_MOD_4DIGITS );
  return (code - com);
}

std::string
ghRailGetCommandName(ghCommandQueue *cmd) {
  return cmd->recv[GH_JSON_NAME_STRING];
}

std::string
ghRailGetCommandFirstArgString(ghCommandQueue *cmd) {
  nlohmann::json args = cmd->recv[GH_JSON_ARGS_STRING];
  int size = cmd->recv[GH_JSON_SIZE_STRING];
  std::vector<std::string> v = args.get<std::vector<std::string>>();
  if ( v.size() > 0 ) {
    return v[0];
  } else {
    return GH_STRING_NONE;
  }
}

void
ghRailGetCommandFloatVector(ghCommandQueue *cmd, float *vec) {

  nlohmann::json args = cmd->recv[GH_JSON_ARGS_STRING];
  int size = cmd->recv[GH_JSON_SIZE_STRING];
  std::vector<float> v = args.get<std::vector<float>>();
  if ( v.size() > 0 ) {
    if ( size <= v.size() ) {
      for (int i = 0; i < size; i++) {
	vec[i] = v[i];
      }
    } else {
      vec[0] = v[0];
    }
  } else {
    vec[0] = 0;
  }
  
}

void
ghRailExecuteCommand( ghCommandQueue *cmd, ghRail *rail ,  double simtime , osgEarth::SkyNode* _sky, std::map<std::string, ghWindow>& _wins)
{
  int com = ghRailGetCommandID(cmd);

  if ( rail->IsLoaded() ) {
    //
    // simulation data loaded
    //
    switch (com) {
    case GH_COMMAND_EXIT:
      ghRailCommandExit(cmd);
      cmd->status = GH_QUEUE_STATUS_PART_EXECUTED;      
      break;
    case (GH_COMMAND_SHOW+GH_TYPE_STATUS):
      ghRailCommandShowStatus(cmd,rail,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED; 
      break;
    case (GH_COMMAND_SHOW+GH_TYPE_VERSION):
      ghRailCommandShowVersion(cmd);
      cmd->status = GH_QUEUE_STATUS_EXECUTED; 
      break;
    case GH_COMMAND_PLAY:
      ghRailCommandPlayPause(cmd,rail,true);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case GH_COMMAND_PAUSE:
      ghRailCommandPlayPause(cmd,rail,false);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CLOCK+GH_TYPE_SET+GH_KEY_TIME):
      ghRailCommandClockSetTime(cmd,rail,_sky);
      cmd->status = GH_QUEUE_STATUS_PART_EXECUTED;
      break;
    case (GH_COMMAND_CLOCK+GH_TYPE_GET+GH_KEY_TIME):
      ghRailCommandClockGetTime(cmd,rail,_sky);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CLOCK+GH_TYPE_SET+GH_KEY_SPEED):
      ghRailCommandClockSetSpeed(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CLOCK+GH_TYPE_GET+GH_KEY_SPEED):
      ghRailCommandClockGetSpeed(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_SET+GH_KEY_TRACKING):
      ghRailCommandCameraSetTracking(cmd,rail,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;	        
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_GET+GH_KEY_TRACKING):
      ghRailCommandCameraGetTracking(cmd,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;	        
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_SET+GH_KEY_POSITION):
      ghRailCommandCameraSetPosition(cmd,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;	        
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_GET+GH_KEY_POSITION):
      ghRailCommandCameraGetPosition(cmd,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;	        
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_SET+GH_KEY_LOOKAT):
      ghRailCommandCameraSetLookat(cmd,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;	        
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_GET+GH_KEY_LOOKAT):
      ghRailCommandCameraGetLookat(cmd,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;	        
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_SET+GH_KEY_UPVEC):
      ghRailCommandCameraSetUpvec(cmd,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;	        
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_GET+GH_KEY_UPVEC):
      ghRailCommandCameraGetUpvec(cmd,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;	        
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_GET+GH_KEY_VIEWPORT):      
      ghRailCommandCameraGetViewport(cmd,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_SET+GH_KEY_SCREEN):
      ghRailCommandCameraSetScreen(cmd,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_GET+GH_KEY_SCREEN):
      ghRailCommandCameraGetScreen(cmd,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_SET+GH_KEY_WINDOW):
      ghRailCommandCameraSetWindow(cmd,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_GET+GH_KEY_WINDOW):
      ghRailCommandCameraGetWindow(cmd,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_ADD):
      ghRailCommandCameraAdd(cmd,rail,_wins);
      cmd->status = GH_QUEUE_STATUS_PART_EXECUTED;
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_REMOVE):
      ghRailCommandCameraRemove(cmd,_wins);
      cmd->status = GH_QUEUE_STATUS_PART_EXECUTED;
      break;
    case (GH_COMMAND_CAMERA+GH_TYPE_GET+GH_KEY_ALL):
      ghRailCommandCameraGetALL(cmd,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_FIELD+GH_TYPE_GET+GH_KEY_ID):
      ghRailCommandFieldGetID(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_FIELD+GH_TYPE_GET+GH_KEY_TRAIN):
      ghRailCommandFieldGetTrain(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_FIELD+GH_TYPE_GET+GH_KEY_LINE):
      ghRailCommandFieldGetLine(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_FIELD+GH_TYPE_GET+GH_KEY_TIMEZONE):
      ghRailCommandFieldGetTimezone(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_FIELD+GH_TYPE_GET+GH_KEY_DESCRIPTION):
      ghRailCommandFieldGetDescription(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_TRAIN+GH_TYPE_LABEL+GH_KEY_ON):
      ghRailCommandTrainLabel(cmd,rail,true);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_TRAIN+GH_TYPE_LABEL+GH_KEY_OFF):
      ghRailCommandTrainLabel(cmd,rail,false);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_TRAIN+GH_TYPE_GET+GH_KEY_POSITION):
      ghRailCommandTrainPosition(cmd,rail,simtime);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_TRAIN+GH_TYPE_GET+GH_KEY_TIMETABLE):
      ghRailCommandTrainTimetable(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_TRAIN+GH_TYPE_GET+GH_KEY_ICON):
      ghRailCommandTrainIcon(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_TRAIN+GH_TYPE_GET+GH_KEY_LINE):
      ghRailCommandTrainLine(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_TRAIN+GH_TYPE_GET+GH_KEY_DISTANCE):
      ghRailCommandTrainDistance(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CONFIG+GH_TYPE_SET+GH_KEY_MAXCLOCKSPEED):
      ghRailCommandConfigSetMaxspeed(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CONFIG+GH_TYPE_GET+GH_KEY_MAXCLOCKSPEED):
      ghRailCommandConfigGetMaxspeed(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CONFIG+GH_TYPE_SET+GH_KEY_ALTMODE):
      ghRailCommandConfigSetAltmode(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CONFIG+GH_TYPE_GET+GH_KEY_ALTMODE):
      ghRailCommandConfigGetAltmode(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CONFIG+GH_TYPE_SET+GH_KEY_DISPLAYDISTANCE):
      ghRailCommandConfigSetDisplaydistance(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CONFIG+GH_TYPE_GET+GH_KEY_DISPLAYDISTANCE):
      ghRailCommandConfigGetDisplaydistance(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CONFIG+GH_TYPE_SET+GH_KEY_MAXCAMERA):
      ghRailCommandConfigSetMaxcamera(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_CONFIG+GH_TYPE_GET+GH_KEY_MAXCAMERA):
      ghRailCommandConfigGetMaxcamera(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_SHM+GH_TYPE_SETCLOCK+GH_KEY_TIME):
      ghRailCommandShmSet(cmd,rail,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_SHM+GH_TYPE_SETTRAIN+GH_KEY_POSITION):
      ghRailCommandShmSet(cmd,rail,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_SHM+GH_TYPE_SETCAMERA+GH_KEY_VIEWPORT):
      ghRailCommandShmSet(cmd,rail,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    case (GH_COMMAND_SHM+GH_TYPE_REMOVE):
      ghRailCommandShmRemove(cmd,rail,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      break;
    default:
      cmd->resultmsg = std::to_string(com) + GH_ERROR_MSG_UNKNOWN_COMMAND;
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    }

  } else {
    //
    // simulation data NOT loaded YET
    //
    switch (com) {
    case GH_COMMAND_EXIT:
      ghRailCommandExit(cmd);
      cmd->status = GH_QUEUE_STATUS_PART_EXECUTED;
      break;
    case (GH_COMMAND_SHOW+GH_TYPE_STATUS):
      ghRailCommandShowStatus(cmd,rail,_wins);
      cmd->status = GH_QUEUE_STATUS_EXECUTED; 
      break;
    case (GH_COMMAND_SHOW+GH_TYPE_VERSION):
      ghRailCommandShowVersion(cmd);
      cmd->status = GH_QUEUE_STATUS_EXECUTED; 
      break;
    case (GH_COMMAND_FIELD+GH_TYPE_SET):
      ghRailCommandFieldSetData(cmd,rail);
      cmd->status = GH_QUEUE_STATUS_PART_EXECUTED;
      break;
    default:
      cmd->resultmsg = std::to_string(com) + GH_ERROR_MSG_DATA_NOT_LOADED;
      cmd->status = GH_QUEUE_STATUS_EXECUTED;
      cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    }
    
  }
  
}

std::string
ghRailCreateResultObject(ghCommandQueue *cmd){

  nlohmann::json json;
  std::time_t now = std::time(nullptr);
  int unix_time = static_cast<int>(now);
  
  json[GH_JSON_UTIME_STRING] = unix_time;
  json[GH_JSON_COMMAND_STRING] = cmd->recv[GH_JSON_COMMAND_STRING];
  json[GH_JSON_STATUS_STRING] = cmd->resultcode;
  
  if ( cmd->resultsize == 1 ) {
    json[GH_JSON_RESULT_STRING] = { cmd->resultmsg };
    json[GH_JSON_SIZE_STRING] = 1;
  } else {
    std::vector<std::string> resultarray = ghStringSplit(cmd->resultmsg, GH_RESULT_MSG_DELIMITER );
    if ( resultarray.size() == cmd->resultsize ) {
      json[GH_JSON_RESULT_STRING] = resultarray;
      json[GH_JSON_SIZE_STRING] = cmd->resultsize;
    } else {
      std::cout << cmd->resultmsg << std::endl;
      std::cout << "Array " << resultarray.size() << " res " << cmd->resultsize << std::endl;
      json[GH_JSON_RESULT_STRING] = { "wrong result data size" };
      json[GH_JSON_SIZE_STRING] = 1;
    }
  }

  return json.dump();
  
}


void
ghRailCommandFieldSetData(ghCommandQueue *cmd, ghRail *rail) {
  int res = -1;
  std::string fieldid = cmd->recv[GH_JSON_NAME_STRING];
  std::string ret = "field set " + fieldid;
  res = rail->Setup(fieldid);
  switch (res) {
  case GH_SETUP_RESULT_OK:
    //
    ret += " loaded";
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    break;
  case GH_SETUP_RESULT_NOT_GET_CONFIG:
    ret += " cannot get config data";
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    //
    break;
  case GH_SETUP_RESULT_NOT_GET_FIELD:
    ret += " cannot get field data";
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    //
    break;
  case GH_SETUP_RESULT_WRONG_LINE:
    ret += " wrong line data";
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    //
    break;
  case GH_SETUP_RESULT_NOT_GET_LOCOMOTIVE:
    ret += " cannot get locomotive data";
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    break;
  default:
    ret += " unknown data";
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
  }

  cmd->resultmsg = ret;
  cmd->resultsize = 1;
}  

void
ghRailCommandShowStatus(ghCommandQueue *cmd, ghRail *rail, std::map<std::string, ghWindow>& _wins) {

  std::string ret = "show status ";
  if ( rail->IsLoaded() ) {
    ret += rail->GetConfigure();
    ret += " ";
    if ( rail->IsPlaying() ) {
      ret += "running ";
    } else {
      ret += "paused ";
    }
    ret += "speed ";
    ret += std::to_string( rail->GetClockSpeed() );

    for (const auto& [key, value] : _wins) {
      ret += " camera ";
      ret += key;
      ret += " tracking ";
      ret += value.tracking;
    }
    
  } else {
    ret += "simulation data not loaded";
  }

  cmd->resultmsg = ret;
  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;

}

void
ghRailCommandShowVersion(ghCommandQueue *cmd) {
  std::string ret = "show version ";
  ret += GH_APP_NAME;
  ret += " ";
  ret += GH_APP_REVISION;

  cmd->resultmsg = ret;
  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;

}


void
ghRailCommandExit(ghCommandQueue *cmd) {
  std::string ret = "exited";

  cmd->resultmsg = ret;
  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;

}

void
ghRailCommandPlayPause(ghCommandQueue *cmd, ghRail *rail, bool flag) {

  std::string ret = "paused";
  if ( flag ) {
    ret = "played";
  }
  
  rail->SetPlayPause(flag);
  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ret;  

}



void
ghRailCommandClockSetTime(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky) {

  osgEarth::DateTime dt = _sky->getDateTime();
  std::string ret = "clock set time ";
  float vec[3];

  ghRailGetCommandFloatVector(cmd, &vec[0]);
  
  int year = dt.year();
  int month = dt.month();
  int days = dt.day();
  double dhours = dt.hours();
  int hours = floor(dhours);
  int mins = (int) ( dhours - hours ) * 60;
  double _timezone_hour = (double)( rail->GetTimeZoneMinutes() / 60.0 ); // 0 ~ 24 

  hours = (int)vec[0];
  mins = (int)vec[1];

  dhours = hours + ( mins / 60.0 );
  dhours = dhours + _timezone_hour;   // append timezone
  if ( dhours < 0 ) {
    dhours = dhours + 24.0;
    days = days - 1;
  } else if ( dhours > 24 ) {
    dhours = dhours - 24.0;
    days = days + 1;
  } else {
    // NOP
  }
  _sky->setDateTime( osgEarth::DateTime( (int)year, (int)month, (int)days, (double)dhours ) );

  ret += std::to_string(hours);
  ret += ":";
  ret += std::to_string(mins);

  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ret;  

}


void
ghRailCommandClockGetTime(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky) {

  osgEarth::DateTime dt = _sky->getDateTime();

  int year = dt.year();
  int month = dt.month();
  int days = dt.day();
  double dhours = dt.hours();
  int hours = floor(dhours);
  int mins = (int) ( dhours - hours ) * 60;
  double _timezone_hour = (double)( rail->GetTimeZoneMinutes() / 60.0 ); // 0 ~ 24 

  // GET
  dhours = dhours - _timezone_hour;   // append timezone

  if ( dhours < 0 ) {
    dhours = dhours + 24.0;
  } else if ( dhours > 23 ) {
    dhours = dhours - 24.0;
  } else {
    // NOP
  }
  hours = floor(dhours);
  mins = (int) ( ( dhours - hours ) * 60 ) ;

  std::string ret = std::to_string(hours);
  ret += GH_RESULT_MSG_DELIMITER;
  ret += std::to_string(mins);

  cmd->resultsize = 2;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ret;  
  
  
}

void
ghRailCommandClockSetSpeed(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "clock set speed ";

  float vec[3];
  ghRailGetCommandFloatVector(cmd, &vec[0]);
  rail->SetClockSpeed((double)vec[0]);
  
  ret += std::to_string(vec[0]);

  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ret;  

}

void
ghRailCommandClockGetSpeed(ghCommandQueue *cmd, ghRail *rail) {
  double speed = rail->GetClockSpeed();

  std::string ret = std::to_string(speed);

  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ret;  
  
  
}


void
ghRailCommandCameraSetTracking(ghCommandQueue *cmd,ghRail *rail, std::map<std::string, ghWindow>& _wins) {
  std::string ret = "camera set ";
  std::string camname = ghRailGetCommandName(cmd);
  std::string trainid = ghRailGetCommandFirstArgString(cmd);

  if ( _wins.count(camname) > 0 ) {

    if ( trainid == GH_STRING_NONE ) {
      ret += camname;
      ret += " tracking ";
      ret += GH_STRING_NONE; 
      _wins[camname].tracking = GH_STRING_NONE;
    } else if ( rail->IsTrainID(trainid) ) {
      ret += camname;
      ret += " tracking ";
      ret += trainid;
      _wins[camname].tracking = trainid;
    } else {
      ret += "train ";
      ret += trainid;
      ret += " ";
      ret += GH_ERROR_MSG_NOT_FOUND;
      cmd->resultsize = 1;
      cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
      cmd->resultmsg = ret;  
      return;
    }
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;  
    return;
  } else {
    ret += "camera ";
    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
    return;
  }
  
}

void
ghRailCommandCameraGetTracking(ghCommandQueue *cmd, std::map<std::string, ghWindow>& _wins) {

  std::string ret = "camera get ";
  std::string camname = ghRailGetCommandName(cmd);

  if ( _wins.count(camname) > 0 ) {
    ret = _wins[camname].tracking;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;  
  } else {
    ret += "camera ";
    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
  }

}


osg::Matrixd
_calcCameraMatrix(ghCommandQueue *cmd, osg::Camera *cam) {

  osgEarth::Ellipsoid WGS84;
  osg::Vec3d eye, up, center;
  //osg::Camera *cam = ghWindows[cmd->argstr[0]].view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );
  osg::Matrixd mat ;
  int cmdkey = ghRailGetCommandKey(cmd);
  float vec3[3];

  if ( cmdkey == GH_KEY_POSITION ) {
    ghRailGetCommandFloatVector(cmd,&vec3[0]);
    //osg::Vec3d eye2 = osg::Vec3d((double)cmd->argnum[0],(double)cmd->argnum[1], (double)cmd->argnum[2]);
    osg::Vec3d eye2 = osg::Vec3d((double)vec3[0],(double)vec3[1],(double)vec3[2]);
    mat.makeLookAt( WGS84.geodeticToGeocentric(eye2), center, up );
  } else if ( cmdkey == GH_KEY_LOOKAT ) {
    ghRailGetCommandFloatVector(cmd,&vec3[0]);
    osg::Vec3d center2 = osg::Vec3d((double)vec3[0],(double)vec3[1],(double)vec3[2]);
    mat.makeLookAt( eye, WGS84.geodeticToGeocentric(center2), up );
  } else if ( cmdkey == GH_KEY_UPVEC ) {
    ghRailGetCommandFloatVector(cmd,&vec3[0]);
    osg::Vec3d up2 = osg::Vec3d((double)vec3[0],(double)vec3[1],(double)vec3[2]);
    mat.makeLookAt( eye, center, WGS84.geodeticToGeocentric(up2) );
  } else {
    mat.makeLookAt( eye, center, up );
  }
    
  return mat;
}


void
ghRailCommandCameraSetPosition(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins) {

  std::string ret = "camera set ";
  std::string camname = ghRailGetCommandName(cmd);

  if ( _wins.count(camname) > 0 ) {

    if ( _wins[camname].tracking == GH_STRING_NONE ) {
      // NOP process continue
      // camera set position , when tracking is none ONLY

    } else {
      ret += camname;
      ret += " position ";
      ret += GH_ERROR_MSG_STRING_TRACKING;
      cmd->resultsize = 1;
      cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
      cmd->resultmsg = ret;  
      return;
    }

    osgEarth::Ellipsoid WGS84;
    osg::Vec3d eye, up, center;
    _wins[camname].view->getCamera()->getViewMatrixAsLookAt( eye, center, up );
    osg::Matrixd mat ;
    float vec[3];

    ghRailGetCommandFloatVector(cmd,&vec[0]);
    osg::Vec3d eye2 = osg::Vec3d((double)vec[0],(double)vec[1],(double)vec[2]);
    mat.makeLookAt( WGS84.geodeticToGeocentric(eye2), center, up );
    _wins[camname].view->getCameraManipulator()->setByInverseMatrix(mat);

    ret += camname;
    ret += " position ";
    ret += std::to_string(vec[0]);
    ret += " ";
    ret += std::to_string(vec[1]);
    ret += " ";
    ret += std::to_string(vec[2]);
    
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;  
    return;

  } else {
    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
    return;
  }
  
}

void
ghRailCommandCameraGetPosition(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins) {
  osgEarth::Ellipsoid WGS84;
  std::string ret = "camera get ";
  osg::Vec3d eye, up, center;
  std::string camname = ghRailGetCommandName(cmd);

  if ( _wins.count(camname) > 0 ) {

    osg::Camera *cam = _wins[camname].view->getCamera();
    cam->getViewMatrixAsLookAt( eye, center, up );
    osg::Vec3 position = WGS84.geocentricToGeodetic(eye);
    ret = std::to_string(position.x());
    ret += GH_RESULT_MSG_DELIMITER;
    ret += std::to_string(position.y());
    ret += GH_RESULT_MSG_DELIMITER;
    ret += std::to_string(position.z());

    cmd->resultsize = 3;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;

  } else {

    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  

  }

}


void
ghRailCommandCameraSetLookat(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins) {

  std::string ret = "camera set ";
  std::string camname = ghRailGetCommandName(cmd);

  if ( _wins.count(camname) > 0 ) {

    if ( _wins[camname].tracking == GH_STRING_NONE ) {
      // NOP process continue
      // camera set position , when tracking is none ONLY

    } else {
      ret += camname;
      ret += " lookat ";
      ret += GH_ERROR_MSG_STRING_TRACKING;
      cmd->resultsize = 1;
      cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
      cmd->resultmsg = ret;  
      return;
    }

    osgEarth::Ellipsoid WGS84;
    osg::Vec3d eye, up, center;
    _wins[camname].view->getCamera()->getViewMatrixAsLookAt( eye, center, up );
    osg::Matrixd mat ;
    float vec[3];

    ghRailGetCommandFloatVector(cmd,&vec[0]);
    osg::Vec3d center2 = osg::Vec3d((double)vec[0],(double)vec[1],(double)vec[2]);
    mat.makeLookAt( eye, WGS84.geodeticToGeocentric(center2), up );
    _wins[camname].view->getCameraManipulator()->setByInverseMatrix(mat);

    ret += camname;
    ret += " lookat ";
    ret += std::to_string(vec[0]);
    ret += " ";
    ret += std::to_string(vec[1]);
    ret += " ";
    ret += std::to_string(vec[2]);
    
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;  
    return;

  } else {
    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
    return;
  }
  
}

void
ghRailCommandCameraGetLookat(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins) {
  osgEarth::Ellipsoid WGS84;
  std::string ret = "camera get ";
  osg::Vec3d eye, up, center;
  std::string camname = ghRailGetCommandName(cmd);

  if ( _wins.count(camname) > 0 ) {

    osg::Camera *cam = _wins[camname].view->getCamera();
    cam->getViewMatrixAsLookAt( eye, center, up );
    osg::Vec3 center2 = WGS84.geocentricToGeodetic(center);
    ret = std::to_string(center2.x());
    ret += GH_RESULT_MSG_DELIMITER;
    ret += std::to_string(center2.y());
    ret += GH_RESULT_MSG_DELIMITER;
    ret += std::to_string(center2.z());

    cmd->resultsize = 3;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;

  } else {

    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  

  }

}


void
ghRailCommandCameraSetUpvec(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins) {

  std::string ret = "camera set ";
  std::string camname = ghRailGetCommandName(cmd);

  if ( _wins.count(camname) > 0 ) {

    if ( _wins[camname].tracking == GH_STRING_NONE ) {
      // NOP process continue
      // camera set position , when tracking is none ONLY

    } else {
      ret += camname;
      ret += " upvec ";
      ret += GH_ERROR_MSG_STRING_TRACKING;
      cmd->resultsize = 1;
      cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
      cmd->resultmsg = ret;  
      return;
    }

    osgEarth::Ellipsoid WGS84;
    osg::Vec3d eye, up, center;
    _wins[camname].view->getCamera()->getViewMatrixAsLookAt( eye, center, up );
    osg::Matrixd mat ;
    float vec[3];

    ghRailGetCommandFloatVector(cmd,&vec[0]);
    osg::Vec3d up2 = osg::Vec3d((double)vec[0],(double)vec[1],(double)vec[2]);
    mat.makeLookAt( eye, center, up2 );
    _wins[camname].view->getCameraManipulator()->setByInverseMatrix(mat);

    ret += camname;
    ret += " upvec ";
    ret += std::to_string(vec[0]);
    ret += " ";
    ret += std::to_string(vec[1]);
    ret += " ";
    ret += std::to_string(vec[2]);
    
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;  
    return;

  } else {
    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
    return;
  }
  
}

void
ghRailCommandCameraGetUpvec(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins) {
  osgEarth::Ellipsoid WGS84;
  std::string ret = "camera get ";
  osg::Vec3d eye, up, center;
  std::string camname = ghRailGetCommandName(cmd);

  if ( _wins.count(camname) > 0 ) {

    osg::Camera *cam = _wins[camname].view->getCamera();
    cam->getViewMatrixAsLookAt( eye, center, up );
    osg::Vec3 up2 = WGS84.geocentricToGeodetic(up);
    ret = std::to_string(up2.x());
    ret += GH_RESULT_MSG_DELIMITER;
    ret += std::to_string(up2.y());
    ret += GH_RESULT_MSG_DELIMITER;
    ret += std::to_string(up2.z());

    cmd->resultsize = 3;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;

  } else {

    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  

  }

}


void
ghRailCommandCameraGetViewport(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins) {

  std::string ret = "camera get ";
  std::string camname = ghRailGetCommandName(cmd);

  if ( _wins.count(camname) > 0 ) {
  
    std::vector<osg::Vec3d> points = ghGetCameraViewpoints(_wins[camname].view);
    ret = " ";
    // points max 12 (ghRail.cpp)
    for (int i = 0; i < points.size(); i++) {
      ret += std::to_string(points[i].x());
      ret += GH_RESULT_MSG_DELIMITER;
      ret += std::to_string(points[i].y());
      ret += GH_RESULT_MSG_DELIMITER;
      //ret += " ";
      //ret += std::to_string(points[i].z());
    }

    cmd->resultsize = points.size()*2;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ghStringTrim(ret);

  } else {

    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  

  }
    
}


void
ghRailCommandCameraSetScreen(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins) {

  std::string ret = "camera set ";
  std::string camname = ghRailGetCommandName(cmd);

  if ( _wins.count(camname) > 0 ) {

    float vec[3];
    ghRailGetCommandFloatVector(cmd,&vec[0]);

    ret += camname;
    ret += " screen ";
    ret += std::to_string(vec[0]);
    ret += " ";
    ret += std::to_string(vec[1]);

    ghSetConfigWindow(_wins[camname],camname,(int)vec[0],(int)vec[1],-1,-1);

    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;

  } else {

    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  

  }
 
}

void
ghRailCommandCameraGetScreen(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins) {
  int current[4];
  std::string ret = "camera get ";

  std::string camname = ghRailGetCommandName(cmd);

  if ( _wins.count(camname) > 0 ) {
  
    ghGetConfigWindow(_wins[camname],camname,&current[0]);
    
    ret = std::to_string(current[0]);
    ret += GH_RESULT_MSG_DELIMITER;
    ret += std::to_string(current[1]);
  
    cmd->resultsize = 2;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;

  } else {

    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  

  }

}

void
ghRailCommandCameraSetWindow(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins) {

  std::string ret = "camera set ";

  std::string camname = ghRailGetCommandName(cmd);

  if ( _wins.count(camname) > 0 ) {

    float vec[3];
    ghRailGetCommandFloatVector(cmd,&vec[0]);

    ret += camname;
    ret += " window ";
    ret += std::to_string((int)vec[0]);
    ret += " ";
    ret += std::to_string((int)vec[1]);

    ghSetConfigWindow(_wins[camname],camname,-1,-1,(int)vec[0],(int)vec[1]);

    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;

  } else {

    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  

  }

  
}

void
ghRailCommandCameraGetWindow(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins) {
  int current[4];
  std::string ret = "camera get ";

  std::string camname = ghRailGetCommandName(cmd);
  
  if ( _wins.count(camname) > 0 ) {
  
    ghGetConfigWindow(_wins[camname],camname,&current[0]);
    
    ret = std::to_string(current[2]);
    ret += GH_RESULT_MSG_DELIMITER;
    ret += std::to_string(current[3]);
  
    cmd->resultsize = 2;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;

  } else {

    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  

  }

}

void
ghRailCommandCameraAdd(ghCommandQueue *cmd, ghRail *rail,  std::map<std::string, ghWindow>& _wins) {

  std::string ret = "camera add ";
  std::string camname = ghRailGetCommandName(cmd);

  if ( _wins.count(camname) > 0 ) {
    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_ALREADY_EXIST;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
    return;
  }

  for (int i = 0; i < GH_RESERVED_STRING.size(); i++) {
    if ( GH_RESERVED_STRING[i] == camname ) {
      ret += camname;
      ret += " ";
      ret += GH_ERROR_MSG_STRING_RESERVED;
      cmd->resultsize = 1;
      cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
      cmd->resultmsg = ret;  
      return;
    }
  }
  
  int maxcam = rail->GetMaxCamera();
  if ( _wins.size() > maxcam ) {
    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_SIZE_ERROR;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
    return;
  }
    
  float vec[4];
  ghRailGetCommandFloatVector(cmd,&vec[0]);

  ret += camname;
  ret += " ";
  ret += std::to_string((int)vec[0]);
  ret += " ";
  ret += std::to_string((int)vec[1]);
  ret += " ";
  ret += std::to_string((int)vec[2]);
  ret += " ";
  ret += std::to_string((int)vec[3]);

  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ret;  

}



void
ghRailCommandCameraRemove(ghCommandQueue *cmd, std::map<std::string, ghWindow>& _wins) {
  std::string ret = "camera remove ";
  std::string camname = ghRailGetCommandName(cmd);

  //  for (int i = 0; i < GH_RESERVED_STRING.size(); i++) {
  //    if ( GH_RESERVED_STRING[i] == cname ) {
  //      return GH_EXECUTE_RESERVED;
  //    }
  //  }

  if ( _wins.count(camname) > 0 ) {

    ret += camname;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;  
    return;

  } else {

    ret += camname;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
    return;

  }
  

}


void
ghRailCommandCameraGetALL(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins) {
  std::string ret = " ";

  int count = 0;
  for (const auto& [key, value] : _wins) {
    //std::cout << key << ": " << value << std::endl;
    ret += key;
    ret += GH_RESULT_MSG_DELIMITER;
    count++;
  }

  cmd->resultsize = count;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ghStringTrim(ret);

}


void
ghRailCommandFieldGetID(ghCommandQueue *cmd, ghRail *rail) {
  if ( rail->IsLoaded() ) {
    std::string ret = rail->GetConfigure();
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;
  } else {
    std::string ret = "field get id ";
    ret += GH_ERROR_MSG_DATA_NOT_LOADED;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
  }
}

void
ghRailCommandFieldGetTrain(ghCommandQueue *cmd, ghRail *rail) {
  if ( rail->IsLoaded() ) {
    std::string ret = rail->GetUnits();
    cmd->resultsize = rail->GetUnitsSize();
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ghStringTrim(ret);
  } else {
    std::string ret = "field get train ";
    ret += GH_ERROR_MSG_DATA_NOT_LOADED;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
  }
}

void
ghRailCommandFieldGetLine(ghCommandQueue *cmd, ghRail *rail) {
  if ( rail->IsLoaded() ) {
    std::string ret = rail->GetLines();
    cmd->resultsize = rail->GetLinesSize();
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ghStringTrim(ret);
  } else {
    std::string ret = "field get line ";
    ret += GH_ERROR_MSG_DATA_NOT_LOADED;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
  }
}

void
ghRailCommandFieldGetTimezone(ghCommandQueue *cmd, ghRail *rail) {
  if ( rail->IsLoaded() ) {
    std::string ret = rail->GetTimezoneStr();
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;
  } else {
    std::string ret = "field get timezone ";
    ret += GH_ERROR_MSG_DATA_NOT_LOADED;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
  }
}
void
ghRailCommandFieldGetDescription(ghCommandQueue *cmd, ghRail *rail) {
  if ( rail->IsLoaded() ) {
    std::string ret = rail->GetDescription();
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;
  } else {
    std::string ret = "field get description ";
    ret += GH_ERROR_MSG_DATA_NOT_LOADED;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
  }
}


void
ghRailCommandTrainLabel(ghCommandQueue *cmd, ghRail *rail,bool flag) {
  bool result ;
  std::string ret = "train label ";
  std::string trainid = ghRailGetCommandName(cmd);

  if ( trainid == GH_STRING_ALL ) {
    result = rail->SetTrainLabel("",flag);
  } else {
    result = rail->SetTrainLabel(trainid,flag);
  }

  if ( result ) {
    ret = trainid;
    ret += GH_RESULT_MSG_DELIMITER;
    if ( flag ) {
      ret += "on";
    } else {
      ret += "off";
    }
    cmd->resultsize = 2;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;
  } else {
    ret += trainid;
    if ( flag ) {
      ret += " on";
    } else {
      ret += " off";
    }
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
  }

}

void
ghRailCommandTrainPosition(ghCommandQueue *cmd, ghRail *rail, double simtime) {

  std::string ret = "";
  std::string trainid = ghRailGetCommandName(cmd);

  if ( trainid == GH_STRING_ALL ) {
    ret = rail->GetTrainPosition("",simtime);
    cmd->resultsize = rail->GetRunningTrainCount(simtime) * 4;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ghStringTrim(ret);
  } else {
    if ( rail->IsTrainID(trainid) ) {
      ret = rail->GetTrainPosition(trainid,simtime);
      if ( ret == GH_ERROR_MSG_NOT_RUNNING ) {
	cmd->resultsize = 1;
	ret = "train position ";
	ret += trainid;
	ret += " ";
	ret += GH_ERROR_MSG_NOT_RUNNING;
	cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
	cmd->resultmsg = ret;  
      } else if ( ret == GH_ERROR_MSG_NOT_FOUND ) {
	cmd->resultsize = 1;
	ret = "train position ";
	ret += trainid;
	ret += " ";
	ret += GH_ERROR_MSG_NOT_FOUND;
	cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
	cmd->resultmsg = ret;  
      } else {
	cmd->resultsize = 3;
	cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
	cmd->resultmsg = ghStringTrim(ret);
      }
    } else {
      cmd->resultsize = 1;
      ret = "train position ";
      ret += trainid;
      ret += " ";
      ret += GH_ERROR_MSG_NOT_FOUND;
      cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
      cmd->resultmsg = ret;  
    }
  }
}

void
ghRailCommandTrainTimetable(ghCommandQueue *cmd, ghRail *rail) {

  std::string ret = "train timetable ";
  std::string trainid = ghRailGetCommandName(cmd);

  if ( rail->IsTrainID(trainid) ) {
    ret = rail->GetTrainTimetable(trainid);
    cmd->resultsize = rail->GetTrainTimetableRows(trainid) * 2;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ghStringTrim(ret);
  } else {
    cmd->resultsize = 1;
    ret += trainid;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
  }
}

void
ghRailCommandTrainIcon(ghCommandQueue *cmd, ghRail *rail) {

  std::string ret = "train icon ";
  std::string trainid = ghRailGetCommandName(cmd);

  if ( rail->IsTrainID(trainid) ) {
    ret = rail->GetTrainIcon(trainid);
    cmd->resultsize = 1;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;
  } else {
    cmd->resultsize = 1;
    ret += trainid;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
  }
}



void
ghRailCommandTrainLine(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "train line ";
  std::string trainid = ghRailGetCommandName(cmd);

  if ( rail->IsTrainID(trainid) ) {
    ret = rail->GetTrainLine(trainid);
    cmd->resultsize = 3;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;
  } else {
    cmd->resultsize = 1;
    ret += trainid;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
  }

}



void
ghRailCommandTrainDistance(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "train distance ";
  std::string trainid = ghRailGetCommandName(cmd);

  if ( rail->IsTrainID(trainid) ) {
    ret = rail->GetTrainDistance(trainid);
    int rows = rail->GetTrainTimetableRows(trainid);
    cmd->resultsize = ( rows + 2 );
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ghStringTrim(ret);
  } else {
    cmd->resultsize = 1;
    ret += trainid;
    ret += " ";
    ret += GH_ERROR_MSG_NOT_FOUND;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
  }
}

void
ghRailCommandConfigSetMaxspeed(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "config set maxclockspeed ";
  float vec[2];
  ghRailGetCommandFloatVector(cmd, &vec[0]);
  rail->SetClockMaxSpeed((double)vec[0]);
  ret += std::to_string(vec[0]);
  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ret;
}

void
ghRailCommandConfigGetMaxspeed(ghCommandQueue *cmd, ghRail *rail) {
  //std::string ret = "config get maxclockspeed ";
  double speed = rail->GetClockMaxSpeed();
  std::string ret = std::to_string(speed);
  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ret;
}
void
ghRailCommandConfigSetAltmode(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "config set altmode ";
  float vec[2];
  ghRailGetCommandFloatVector(cmd, &vec[0]);
  rail->SetAltmode((int)vec[0]);
  //ret += std::to_string(vec[0]);
  ret += GH_COMMAND_ALTMODE_STRING[(int)vec[0]];
  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ret;
}
void
ghRailCommandConfigGetAltmode(ghCommandQueue *cmd, ghRail *rail) {
  //std::string ret = "config get altmode ";
  std::string ret = GH_COMMAND_ALTMODE_STRING[rail->GetAltmode()];
  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ret;
}
void
ghRailCommandConfigSetDisplaydistance(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "config set displaydistance ";
  float vec[2];
  ghRailGetCommandFloatVector(cmd, &vec[0]);
  rail->SetDisplayDistance((double)vec[0]);
  ret += std::to_string(vec[0]);
  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ret;
}
void
ghRailCommandConfigGetDisplaydistance(ghCommandQueue *cmd, ghRail *rail) {
  //std::string ret = "config get displaydistance ";
  double dis = rail->GetDisplayDistance();
  std::string ret = std::to_string(dis);
  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ret;
}

void
ghRailCommandConfigSetMaxcamera(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "config set maxcamera ";
  float vec[2];
  ghRailGetCommandFloatVector(cmd, &vec[0]);
  rail->SetMaxCamera((int)vec[0]);
  ret += std::to_string(vec[0]);
  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ret;
}
void
ghRailCommandConfigGetMaxcamera(ghCommandQueue *cmd, ghRail *rail) {
  //std::string ret = "config get maxcamera ";
  int maxcam = rail->GetMaxCamera();
  std::string ret = std::to_string(maxcam);
  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ret;
}

void
ghRailCommandShmSet(ghCommandQueue *cmd,ghRail *rail, std::map<std::string, ghWindow>& _wins) {

  //  int offset = 0;
  //  if ( _win != (ghWindow *)NULL ) {
  //    offset = 24 + ghCountWindow(_win);
  //  }

  //int offset = 24 + _wins.size();
  //#ifdef _WINDOWS
  //  int shmkey = 86822723;
  //#else  
  //  int shmkey = ftok(GH_SHM_PATH,shmtype+offset);
  //#endif
  
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  int shmkey = ( std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() ) % GH_SHM_DIGITS ;
  std::string keyname = GH_STRING_NONE;
  std::string ret = "shm set ";
  int ssize = -1;
  
  if (shmkey < 0) {

    cmd->resultsize = 1;
    ret += GH_ERROR_MSG_CANNOT_GET;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
    cmd->resultmsg = ret;  
    return;
    
  } else {
    //std::string skey = std::to_string(shmkey);
    int shmtype = ghRailGetCommandType(cmd);
    if ( shmtype == GH_TYPE_SETCLOCK ) {
      ssize = rail->InitShmClock(shmkey);
      keyname = rail->GetShmClockKeyname();
      ret += "clock time ";
    } else if ( shmtype == GH_TYPE_SETTRAIN ) {
      ssize = rail->InitShmTrain(shmkey);
      keyname = rail->GetShmTrainKeyname();
      ret += "train potision ";
    } else if ( shmtype == GH_TYPE_SETCAMERA ) {
      std::string camname = ghRailGetCommandName(cmd);
      if ( _wins.count(camname) > 0 ) {
	ssize = ghInitShmWindow(shmkey,&_wins[camname].shm,camname);
	ret += "camera ";
	ret += camname;
	ret += " viewport ";
	keyname = ghGetShmWindowKeyname(&_wins[camname].shm);
      } else {
	ret += camname;
	ret += " ";
	ret += GH_ERROR_MSG_NOT_FOUND;
	cmd->resultsize = 1;
	cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
	cmd->resultmsg = ret;  
	return;
      }
    } else {
      ret += GH_ERROR_MSG_NOT_FOUND;
      cmd->resultsize = 1;
      cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
      cmd->resultmsg = ret;  
      return;
    }

  }

  if ( ssize < 0 ) {
      cmd->resultsize = 1;
      ret += GH_ERROR_MSG_CANNOT_ALLOCATE;
      cmd->resultcode = GH_QUEUE_RETURN_CODE_ERROR;
      cmd->resultmsg = ret;  
      return;
  } else {
    //#ifdef _WINDOWS
    //      ret += keyname + " ";
    //#else
    //      ret += skey + " ";
    //#endif
    ret = keyname;
    ret += GH_RESULT_MSG_DELIMITER;
    ret += std::to_string(ssize);
    
    cmd->resultsize = 2;
    cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
    cmd->resultmsg = ret;
    return;
  }

}

void
ghRailCommandShmRemove(ghCommandQueue *cmd,ghRail *rail, std::map<std::string, ghWindow>& _wins) {
  std::string ret = "shm remove ";
  //int shmkey = (int)cmd->argnum[0];
  //ret += std::to_string(cmd->argnum[0]);
  std::string keyname = ghRailGetCommandName(cmd);
  ret += keyname;

  int ssize = rail->RemoveShm(keyname);
  cmd->resultsize = 1;
  cmd->resultcode = GH_QUEUE_RETURN_CODE_SUCCESS;
  cmd->resultmsg = ret;

  //  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
  //    cmd->resultmessage = ret;
  //    return GH_EXECUTE_SUCCESS;
  //  } else {
  //    return GH_EXECUTE_SIZE_ERROR;
  //  }

}

