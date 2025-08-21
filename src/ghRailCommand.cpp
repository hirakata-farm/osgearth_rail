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

# include "ghRail.hpp"
# include "ghRailCommand.hpp"

///////////////////////////////////////////////////
//
//   Messags
//
#define GH_COMMAND_MESSAGE_NOT_LOADED     "NOT loaded simulation data";
#define GH_COMMAND_MESSAGE_ALREADY_LOADED "simulation data already loaded";
#define GH_COMMAND_MESSAGE_UNKOWN         "Unknown command";

std::string GH_COMMAND_MESSAGE[34] = {
  "application exit ",
  "close window ",
  "simulation running ",
  "simulation paused ",
  "show status ",
  "show version ",
  "field set ",
  "field get ",
  "field train ",
  "field timezone ",
  "field description ",
  "clock set time ",
  "clock get time ",
  "clock set speed ",
  "clock get speed ",
  "camera set position ",
  "camera get position ",
  "camera set lookat ",
  "camera get lookat ",
  "camera set upvec ",
  "camera get upvec ",
  "camera set tracking ",
  "camera get tracking ",
  "camera get viewport ",
  "train label on ",
  "train label off ",  
  "train position ",
  "train timetable ",
  "config set max clock speed ",
  "config get max clock speed ",  
  "config set altmode ",
  "config get altmode ",      
  "config set displaydistance ",
  "config get displaydistance "
};
std::string GH_COMMAND_ALTMODE_STRING[3] = {
  GH_STRING_CLAMP,
  GH_STRING_RELATIVE,
  GH_STRING_ABSOLUTE
};

  
////////////////////////////////////////////////

std::vector<std::string>
ghStringSplit(std::string str, char del) {
    std::vector<std::string> result;
    std::string subStr;

    for (const char c : str) {
        if (c == del) {
	  if ( subStr.length() > 0 ) {
            result.push_back(subStr);
            subStr.clear();
	  }
        }else {
	  if ( iscntrl(c) == 0 ) {
	    subStr += c;
	  } else {
	    // NOP
	  }
        }
    }

    result.push_back(subStr);
    return result;
}


ghCommandQueue *
ghRailParseCommand(string str) {

  ghCommandQueue *cmd;
  if ((cmd = (ghCommandQueue *)calloc( (unsigned)1, (unsigned)sizeof( ghCommandQueue ) )) == NULL)
    {
      return( (ghCommandQueue *)NULL ) ;
    }
  std::vector<std::string> command = ghStringSplit(str, ' ');
  int command_size = command.size();
  int argint = -1;
  double argdouble = -1;
  cmd->type = GH_COMMAND_UNKNOWN; 
  cmd->argstr = GH_STRING_NOP;
  cmd->isexecute = false;
  cmd->argnum[0]  = 0.0;
  cmd->argnum[1]  = 0.0;
  cmd->argnum[2]  = 0.0;
  
  if ( command_size > 0 )  {
    if ( command[0] == GH_STRING_EXIT ) {
      cmd->type = GH_COMMAND_EXIT;
    } else if ( command[0] == GH_STRING_CLOSE ) {
      cmd->type = GH_COMMAND_CLOSE;
    } else if ( command[0] == GH_STRING_START ) {
      cmd->type = GH_COMMAND_START;
    } else if ( command[0] == GH_STRING_RUN ) {
      cmd->type = GH_COMMAND_START;
    } else if ( command[0] == GH_STRING_STOP ) {
      cmd->type = GH_COMMAND_STOP;
    } else if ( command[0] == GH_STRING_PAUSE ) {
      cmd->type = GH_COMMAND_STOP;
    } else if ( command[0] == GH_STRING_FIELD ) {
      if ( command_size > 1 )  {
	if ( command[1] == GH_STRING_SET && command_size == 3 ) {
	  //  get arg
	  cmd->type = GH_COMMAND_FIELD_SET;
	  cmd->argstr = command[2];
	} else if ( command[1] == GH_STRING_GET && command_size == 2 ) {
	  cmd->type = GH_COMMAND_FIELD_GET;
	} else if ( command[1] == GH_STRING_GET && command_size == 3 ) {
	  if ( command[2] == GH_STRING_TRAIN ) {
	    cmd->type = GH_COMMAND_FIELD_GET_TRAIN;
	  } else if ( command[2] == GH_STRING_TIMEZONE ) {
	    cmd->type = GH_COMMAND_FIELD_GET_TIMEZONE;
	  } else if ( command[2] == GH_STRING_DESCRIPTION ) {
	    cmd->type = GH_COMMAND_FIELD_GET_DESC;
	  } else {
	    // NOP
	  }
	} else {
	  // NOP
	}
      } 
    } else if ( command[0] == GH_STRING_SHOW ) {
      if ( command_size > 1 )  {
	if ( command[1] == GH_STRING_STATUS ) {
	  cmd->type = GH_COMMAND_SHOW_STATUS;
	} else if ( command[1] == GH_STRING_VERSION ) {
	  cmd->type = GH_COMMAND_SHOW_VERSION;
	} else {
	  // NOP
	}
      }
    } else if ( command[0] == GH_STRING_CLOCK ) {
      if ( command[1] == GH_STRING_SET ) {
	//  get arg
	if ( command_size > 3 )  {
	  if ( command[2] == GH_STRING_TIME && command_size == 4 ) {
	    std::vector<std::string> argtime = ghStringSplit(command[3], ':');
	    argint = std::stoi(argtime[0]);
	    if ( argint > 0 && argint < 25 ) {
	      cmd->argnum[0]  = (double)argint;
	      argint = std::stoi(argtime[1]);
	      if ( argint > -1 && argint < 60 ) {
		cmd->argnum[1]  = (double)argint;
		cmd->type = GH_COMMAND_CLOCK_SET_TIME;
	      }
	    }
	  } else if ( command[2] == GH_STRING_SPEED && command_size == 4 ) {
	    cmd->argnum[0] = std::stod(command[3]);
	    cmd->type = GH_COMMAND_CLOCK_SET_SPEED;
	  } else {
	    // NOP
	  }
	}
      } else if ( command[1] == GH_STRING_GET ) {
	if ( command[2] == GH_STRING_TIME && command_size == 3 ) {
	  cmd->type = GH_COMMAND_CLOCK_GET_TIME;
	} else if ( command[2] == GH_STRING_SPEED && command_size == 3 ) {
	  cmd->type = GH_COMMAND_CLOCK_GET_SPEED;	  
	} else {
	  // NOP
	}
      } else {
	// NOP
      }
    } else if ( command[0] == GH_STRING_CAMERA ) {
      if ( command[1] == GH_STRING_SET ) {
	//  get arg
	if ( command[2] == GH_STRING_POSITION && command_size == 6 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_POS;
	  cmd->argnum[0] = std::stod(command[3]);
	  cmd->argnum[1] = std::stod(command[4]);
	  cmd->argnum[2] = std::stod(command[5]);
	} else if ( command[2] == GH_STRING_LOOKAT && command_size == 6 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_LOOK;
	  cmd->argnum[0] = std::stod(command[3]);
	  cmd->argnum[1] = std::stod(command[4]);
	  cmd->argnum[2] = std::stod(command[5]);
	} else if ( command[2] == GH_STRING_UPVEC && command_size == 6 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_UP;
	  cmd->argnum[0] = std::stod(command[3]);
	  cmd->argnum[1] = std::stod(command[4]);
	  cmd->argnum[2] = std::stod(command[5]);
	} else if ( command[2] == GH_STRING_TRACKING && command_size == 4 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_TRACK;
	  cmd->argstr = command[3];
	} else {
	  // NOP
	}
      } else if ( command[1] == GH_STRING_GET ) {
	if ( command[2] == GH_STRING_POSITION && command_size == 3 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_POS;
	} else if ( command[2] == GH_STRING_LOOKAT && command_size == 3 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_LOOK;
	} else if ( command[2] == GH_STRING_UPVEC && command_size == 3 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_UP;
	} else if ( command[2] == GH_STRING_TRACKING && command_size == 3 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_TRACK;
	} else if ( command[2] == GH_STRING_VIEWPORT && command_size == 3 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_VIEWPORT;
	} else {
	  // NOP
	}
      } else {
	// NOP
      }
    } else if ( command[0] == GH_STRING_TRAIN ) {
      if ( command[1] == GH_STRING_LABEL ) {
	if ( command[2] == GH_STRING_ON && command_size == 4 ) {
	    cmd->type = GH_COMMAND_TRAIN_LABEL_ON;
	    cmd->argstr = command[3];
	} else if ( command[2] == GH_STRING_OFF && command_size == 4 ) {
	    cmd->type = GH_COMMAND_TRAIN_LABEL_OFF;
	    cmd->argstr = command[3];
	} else {
	  // NOP
	}
      } else if ( command[1] == GH_STRING_POSITION && command_size == 3  ) {
	cmd->type = GH_COMMAND_TRAIN_POSITION;
	cmd->argstr = command[2];
      } else if ( command[1] == GH_STRING_TIMETABLE  && command_size == 3 ) {
	cmd->type = GH_COMMAND_TRAIN_TIMETABLE;
	cmd->argstr = command[2];
      } else {
	// NOP
      }
    } else if ( command[0] == GH_STRING_CONFIG ) {
      if ( command[1] == GH_STRING_SET ) {
	  if ( command[2] == GH_STRING_MAXCLOCKSPEED && command_size == 4 ) {
	    cmd->type = GH_COMMAND_CONFIG_SET_MAXCLOCKSPEED;
	    cmd->argnum[0] = std::stod(command[3]);
	  } else if ( command[2] == GH_STRING_ALTMODE && command_size == 4 ) {
	    if ( command[3] == GH_STRING_CLAMP ) {
	      cmd->type = GH_COMMAND_CONFIG_SET_ALTMODE;
	      cmd->argnum[0] = GH_ALTMODE_CLAMP;
	    } else if ( command[3] == GH_STRING_RELATIVE ) {
	      cmd->type = GH_COMMAND_CONFIG_SET_ALTMODE;
	      cmd->argnum[0] = GH_ALTMODE_RELATIVE;
	    } else if ( command[3] == GH_STRING_ABSOLUTE ) {
	      cmd->type = GH_COMMAND_CONFIG_SET_ALTMODE;
	      cmd->argnum[0] = GH_ALTMODE_ABSOLUTE;
	    } else {
	      // NOP
	    }
	  } else if ( command[2] == GH_STRING_DISPLAYDISTANCE && command_size == 4 ) {
	    cmd->type = GH_COMMAND_CONFIG_SET_DISPLAYDISTANCE;
	    cmd->argnum[0] = std::stod(command[3]);
	  } else {
	    // NOP
	  }
      } else if ( command[1] == GH_STRING_GET ) {
	if ( command[2] == GH_STRING_MAXCLOCKSPEED ) {
	  cmd->type = GH_COMMAND_CONFIG_GET_MAXCLOCKSPEED;
	} else if ( command[2] == GH_STRING_ALTMODE  ) {
	  cmd->type = GH_COMMAND_CONFIG_GET_ALTMODE;
	} else if ( command[2] == GH_STRING_DISPLAYDISTANCE  ) {
	  cmd->type = GH_COMMAND_CONFIG_GET_DISPLAYDISTANCE;
	} else {
	  // NOP
	}
      }  else {
	  // NOP
      }
    } else {
      // NOP
    }
  }

  return cmd;

}



int
ghRailExecuteCommand( ghCommandQueue *cmd,
		      int socket,
		      ghRail *rail,
		      osgViewer::Viewer* _view,
		      osgEarth::SkyNode* _sky,
		      double simtime ) {

  int retval = GH_POST_EXECUTE_NONE;
  if ( cmd == NULL ) return retval;
  if ( cmd->isexecute ) return retval; // Already execute

  std::string result = "Geoglyph:";
  const char* exitmsg = "Geoglyph: osgearth_rail exited.\n";
  const char* closemsg = "Geoglyph: osgearth_rail closed.\n";
  
  switch (cmd->type) {
  case GH_COMMAND_EXIT:
    send(socket, exitmsg, std::strlen(exitmsg), 0);
    cmd->isexecute = true;    
    retval = GH_POST_EXECUTE_EXIT;
    return retval;
    break;
  case GH_COMMAND_CLOSE:
    send(socket, closemsg, std::strlen(closemsg), 0);
    cmd->isexecute = true;    
    retval = GH_POST_EXECUTE_CLOSE;
    return retval;
    break;
  case GH_COMMAND_START:
    result += ghRailCommandStart(cmd,rail);
    break;
  case GH_COMMAND_STOP:
    result += ghRailCommandStop(cmd,rail);
    break;
  case GH_COMMAND_FIELD_SET:
    result += ghRailCommandFieldSet(cmd,rail,_sky);
    retval = GH_POST_EXECUTE_TIMEZONE;
    break;
  case GH_COMMAND_FIELD_GET:
    result += ghRailCommandFieldGet(cmd,rail);
    break;
  case GH_COMMAND_FIELD_GET_TRAIN:
    result += ghRailCommandFieldTrain(cmd,rail);
    break;
  case GH_COMMAND_FIELD_GET_TIMEZONE:
    result += ghRailCommandFieldTimezone(cmd,rail);
    break;
  case GH_COMMAND_FIELD_GET_DESC:
    result += ghRailCommandFieldDescription(cmd,rail);
    break;
  case GH_COMMAND_CLOCK_SET_TIME:
    result += ghRailCommandClockSetTime(cmd,rail,_sky);
    retval = GH_POST_EXECUTE_SETCLOCK;
    break;
  case GH_COMMAND_CLOCK_GET_TIME:
    result += ghRailCommandClockGetTime(cmd,rail,_sky);
    break;
  case GH_COMMAND_CLOCK_SET_SPEED:
    result += ghRailCommandClockSetSpeed(cmd,rail);
    break;
  case GH_COMMAND_CLOCK_GET_SPEED:
    result += ghRailCommandClockGetSpeed(cmd,rail);
    break;
  case GH_COMMAND_TRAIN_LABEL_ON:
    result += ghRailCommandTrainLabelOn(cmd,rail);
    break;
  case GH_COMMAND_TRAIN_LABEL_OFF:
    result += ghRailCommandTrainLabelOff(cmd,rail);
    break;
  case GH_COMMAND_TRAIN_POSITION:
    result += ghRailCommandTrainPosition(cmd,rail,simtime);
    break;
  case GH_COMMAND_TRAIN_TIMETABLE:
    result += ghRailCommandTrainTimetable(cmd,rail);
    break;
  case GH_COMMAND_CAMERA_SET_POS:
    result += ghRailCommandCameraSetPosition(cmd,rail,_view);
    break;
  case GH_COMMAND_CAMERA_GET_POS:
    result += ghRailCommandCameraGetPosition(cmd,rail,_view);
    break;
  case GH_COMMAND_CAMERA_SET_LOOK:
    result += ghRailCommandCameraSetLookat(cmd,rail,_view);
    break;
  case GH_COMMAND_CAMERA_GET_LOOK:
    result += ghRailCommandCameraGetLookat(cmd,rail,_view);
    break;
  case GH_COMMAND_CAMERA_SET_UP:
    result += ghRailCommandCameraSetUpvec(cmd,rail,_view);
    break;
  case GH_COMMAND_CAMERA_GET_UP:
    result += ghRailCommandCameraGetUpvec(cmd,rail,_view);
    break;
  case GH_COMMAND_CAMERA_SET_TRACK:
    result += ghRailCommandCameraSetTracking(cmd,rail);
    break;
  case GH_COMMAND_CAMERA_GET_TRACK:
    result += ghRailCommandCameraGetTracking(cmd,rail);
    break;
  case GH_COMMAND_CAMERA_GET_VIEWPORT:
    result += ghRailCommandCameraViewport(cmd,_view);
    break;
  case GH_COMMAND_CONFIG_SET_MAXCLOCKSPEED:
    result += ghRailCommandConfigSetMaxspeed(cmd,rail);
    break;
  case GH_COMMAND_CONFIG_GET_MAXCLOCKSPEED:
    result += ghRailCommandConfigGetMaxspeed(cmd,rail);
    break;
  case GH_COMMAND_CONFIG_SET_ALTMODE:
    result += ghRailCommandConfigSetAltmode(cmd,rail);
    break;
  case GH_COMMAND_CONFIG_GET_ALTMODE:
    result += ghRailCommandConfigGetAltmode(cmd,rail);
    break;
  case GH_COMMAND_CONFIG_SET_DISPLAYDISTANCE:
    result += ghRailCommandConfigSetDisplaydistance(cmd,rail);
    break;
  case GH_COMMAND_CONFIG_GET_DISPLAYDISTANCE:
    result += ghRailCommandConfigGetDisplaydistance(cmd,rail);
    break;
  case GH_COMMAND_SHOW_STATUS:
    result += ghRailCommandShowStatus(cmd,rail);
    break;
  case GH_COMMAND_SHOW_VERSION:
    result += ghRailCommandShowVersion();
    break;
  default:
    // NOP
    result += GH_COMMAND_MESSAGE_UNKOWN;
  }

  const char* retmsg = result.c_str();
  send(socket, retmsg, std::strlen(retmsg), 0);
  cmd->isexecute = true;    

  return retval;
}


std::string
ghRailCommandStart(ghCommandQueue *cmd, ghRail *rail) {

  if ( rail->IsLoaded() ) {
    rail->SetPlayPause(true);
    return GH_COMMAND_MESSAGE[GH_COMMAND_START];
  } else {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }

}

std::string
ghRailCommandStop(ghCommandQueue *cmd, ghRail *rail) {

  if ( rail->IsLoaded() ) {
    rail->SetPlayPause(false);
    return GH_COMMAND_MESSAGE[GH_COMMAND_STOP];
  } else {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }
 
}

std::string
ghRailCommandFieldSet(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky) {
  int res = -1;
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_FIELD_SET];
  ret += cmd->argstr;
  if ( rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_ALREADY_LOADED;
  }
  res = rail->Setup(cmd->argstr);
  if (  res == GH_SETUP_RESULT_OK ) {
    ret += " loaded\n";
    _sky->setDateTime( rail->GetBaseDatetime());
  } else {
    ret += " can NOT load\n";
  }
  return ret;
}
std::string
ghRailCommandFieldGet(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_FIELD_GET];
  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }
  ret += rail->GetConfigure();
  ret += "\n";
  return ret;
}

std::string
ghRailCommandFieldTrain(ghCommandQueue *cmd, ghRail *rail) {

  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_FIELD_GET_TRAIN];
  ret += rail->GetUnits();
  ret += "\n";  
  return ret;

}
std::string
ghRailCommandFieldTimezone(ghCommandQueue *cmd, ghRail *rail) {

  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_FIELD_GET_TIMEZONE];
  ret += rail->GetTimezoneStr();
  ret += "\n";  
  return ret;
}
std::string
ghRailCommandFieldDescription(ghCommandQueue *cmd, ghRail *rail) {

  if ( ! rail->IsLoaded() ) {
   return GH_COMMAND_MESSAGE_NOT_LOADED;
  }
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_FIELD_GET_DESC];
  ret += rail->GetDescription();
  ret += "\n";  
  return ret;
}

std::string
ghRailCommandClockSetTime(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky) {

  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }

  osgEarth::DateTime dt = _sky->getDateTime();

  int year = dt.year();
  int month = dt.month();
  int days = dt.day();
  double dhours = dt.hours();
  int hours = floor(dhours);
  int mins = (int) ( dhours - hours ) * 60;
  double _timezone_hour = (double)( rail->GetTimeZoneMinutes() / 60.0 ); // 0 ~ 24 

  hours = (int)cmd->argnum[0];
  mins = (int)cmd->argnum[1];

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

  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CLOCK_SET_TIME];
  std::string s_hour = std::to_string(hours);
  std::string s_min = std::to_string(mins);
  ret += s_hour;
  ret += ":";
  ret += s_min;
  ret += "\n";
  return ret;
  
}

std::string
ghRailCommandClockGetTime(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky) {

  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }

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

  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CLOCK_GET_TIME];
  std::string s_hour = std::to_string(hours);
  std::string s_min = std::to_string(mins);
    
  ret += s_hour;
  ret += ":";
  ret += s_min;
  ret += "\n";
  return ret;
  
}

std::string
ghRailCommandClockSetSpeed(ghCommandQueue *cmd, ghRail *rail) {
  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }
  rail->SetClockSpeed(cmd->argnum[0]);
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CLOCK_SET_SPEED];
  double speed = rail->GetClockSpeed();
  std::string s_sp = std::to_string(speed);
  ret += s_sp;
  ret += "\n";
  return ret;
}
std::string
ghRailCommandClockGetSpeed(ghCommandQueue *cmd, ghRail *rail) {
  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CLOCK_GET_SPEED];
  double speed = rail->GetClockSpeed();
  std::string s_sp = std::to_string(speed);
  ret += s_sp;
  ret += "\n";
  return ret;
}


std::string
ghRailCommandCameraSetPosition(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view) {
  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }

  osgEarth::Ellipsoid WGS84;
  osg::Vec3d eye, up, center;
  osg::Camera *cam = _view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );

  //osgEarth::GeoPoint eyeGeo;
  //eyeGeo.fromWorld( _map->getMapSRS(), eye );
  
  osg::Vec3d eye2 = osg::Vec3d((double)cmd->argnum[0],(double)cmd->argnum[1], (double)cmd->argnum[2]);
  osg::Matrixd mat ;
  mat.makeLookAt( WGS84.geodeticToGeocentric(eye2), center, up );
  _view->getCameraManipulator()->setByInverseMatrix(mat);

  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CAMERA_SET_POS];
  ret += std::to_string(eye2.x());
  ret += " ";
  ret += std::to_string(eye2.y());
  ret += " ";
  ret += std::to_string(eye2.z());
  ret += "\n";
  return ret;

}
std::string
ghRailCommandCameraGetPosition(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view) {
  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }

  osgEarth::Ellipsoid WGS84;
  osg::Vec3d eye, up, center;
  osg::Camera *cam = _view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );

  //osgEarth::GeoPoint eyeGeo;
  //eyeGeo.fromWorld( _map->getMapSRS(), eye );
  
  osg::Vec3 position = WGS84.geocentricToGeodetic(eye);
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CAMERA_GET_POS];
  ret += std::to_string(position.x());
  ret += " ";
  ret += std::to_string(position.y());
  ret += " ";
  ret += std::to_string(position.z());
  ret += "\n";
  return ret;
}

std::string
ghRailCommandCameraSetLookat(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view) {
  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }

  osgEarth::Ellipsoid WGS84;
  osg::Vec3d eye, up, center;
  osg::Camera *cam = _view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );

  osg::Vec3d center2 = osg::Vec3d((double)cmd->argnum[0],(double)cmd->argnum[1], (double)cmd->argnum[2]);
  osg::Matrixd mat ;
  mat.makeLookAt( eye, WGS84.geodeticToGeocentric(center2), up );
  _view->getCameraManipulator()->setByInverseMatrix(mat);

  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CAMERA_SET_LOOK];
  ret += std::to_string(center2.x());
  ret += " ";
  ret += std::to_string(center2.y());
  ret += " ";
  ret += std::to_string(center2.z());
  ret += "\n";
  return ret;
}
std::string
ghRailCommandCameraGetLookat(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view) {
  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }

  osgEarth::Ellipsoid WGS84;
  osg::Vec3d eye, up, center;
  osg::Camera *cam = _view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );

  osg::Vec3 center2 = WGS84.geocentricToGeodetic(center);
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CAMERA_GET_LOOK];
  ret += std::to_string(center2.x());
  ret += " ";
  ret += std::to_string(center2.y());
  ret += " ";
  ret += std::to_string(center2.z());
  ret += "\n";
  return ret;
}

std::string
ghRailCommandCameraSetUpvec(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view) {
  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }

  osgEarth::Ellipsoid WGS84;
  osg::Vec3d eye, up, center;
  osg::Camera *cam = _view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );

  osg::Vec3d up2 = osg::Vec3d((double)cmd->argnum[0],(double)cmd->argnum[1], (double)cmd->argnum[2]);
  osg::Matrixd mat ;
  mat.makeLookAt( eye, center, WGS84.geodeticToGeocentric(up2) );
  _view->getCameraManipulator()->setByInverseMatrix(mat);

  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CAMERA_SET_UP];
  ret += std::to_string(up2.x());
  ret += " ";
  ret += std::to_string(up2.y());
  ret += " ";
  ret += std::to_string(up2.z());
  ret += "\n";
  return ret;

}
std::string
ghRailCommandCameraGetUpvec(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view) {
  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }

  osgEarth::Ellipsoid WGS84;
  osg::Vec3d eye, up, center;
  osg::Camera *cam = _view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );

  osg::Vec3 up2 = WGS84.geocentricToGeodetic(up);
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CAMERA_GET_UP];
  ret += std::to_string(up2.x());
  ret += " ";
  ret += std::to_string(up2.y());
  ret += " ";
  ret += std::to_string(up2.z());
  ret += "\n";
  return ret;
}

std::string
ghRailCommandCameraSetTracking(ghCommandQueue *cmd, ghRail *rail) {
  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }

  if ( rail->SetTrackingTrain(cmd->argstr) ) {
    std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CAMERA_SET_TRACK];
    ret += cmd->argstr;
    ret += "\n";
    return ret;
  } else {
      return " camera tracking Undefined train\n";
  }
}

std::string
ghRailCommandCameraGetTracking(ghCommandQueue *cmd, ghRail *rail) {
  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CAMERA_GET_TRACK];
  ret += rail->GetTrackingTrain();
  ret += "\n";
  return ret;
}


std::string
ghRailCommandCameraViewport(ghCommandQueue *cmd, osgViewer::Viewer* _view) {

  // Get the corners of all points on the view frustum.  Mostly modified from osgthirdpersonview
  osg::Matrixd proj = _view->getCamera()->getProjectionMatrix();
  osg::Matrixd mv = _view->getCamera()->getViewMatrix();
  osg::Matrixd invmv = osg::Matrixd::inverse( mv );

  double nearPlane = proj(3,2) / (proj(2,2)-1.0);
  double farPlane = proj(3,2) / (1.0+proj(2,2));

  // Get the sides of the near plane.
  double nLeft = nearPlane * (proj(2,0)-1.0) / proj(0,0);
  double nRight = nearPlane * (1.0+proj(2,0)) / proj(0,0);
  double nTop = nearPlane * (1.0+proj(2,1)) / proj(1,1);
  double nBottom = nearPlane * (proj(2,1)-1.0) / proj(1,1);

  // Get the sides of the far plane.
  double fLeft = farPlane * (proj(2,0)-1.0) / proj(0,0);
  double fRight = farPlane * (1.0+proj(2,0)) / proj(0,0);
  double fTop = farPlane * (1.0+proj(2,1)) / proj(1,1);
  double fBottom = farPlane * (proj(2,1)-1.0) / proj(1,1);

  double dist = farPlane - nearPlane;

  int samples = 24;
  std::vector<osg::Vec3d> verts;
  verts.reserve(samples * 4 - 4);
  for (int i = 0; i < samples - 1; ++i) {
    double j = (double)i / (double)(samples - 1);
    verts.push_back(osg::Vec3d(nLeft + (nRight - nLeft) * j, nBottom, nearPlane));
    verts.push_back(osg::Vec3d(fLeft + (fRight - fLeft) * j, fBottom, farPlane));
  }
  for (int i = 0; i < samples - 1; ++i) {
    double j = (double)i / (double)(samples - 1);
    verts.push_back(osg::Vec3d(nRight, nBottom + (nTop - nBottom) * j, nearPlane));
    verts.push_back(osg::Vec3d(fRight, fBottom + (fTop - fBottom) * j, farPlane));
  }
  for (int i = 0; i < samples - 1; ++i) {
    double j = (double)i / (double)(samples - 1);
    verts.push_back(osg::Vec3d(nRight - (nRight - nLeft) * j, nTop, nearPlane));
    verts.push_back(osg::Vec3d(fRight - (fRight - fLeft) * j, fTop, farPlane));
  }
  for (int i = 0; i < samples - 1; ++i) {
    double j = (double)i / (double)(samples - 1);
    verts.push_back(osg::Vec3d(nLeft, nTop - (nTop - nBottom) * j, nearPlane));
    verts.push_back(osg::Vec3d(fLeft, fTop - (fTop - fBottom) * j, farPlane));
  }

  const auto* wgs84 = osgEarth::SpatialReference::create("epsg:4326");
  auto& ellip = wgs84->getEllipsoid();

  std::vector<osg::Vec3d> points;
  points.reserve(verts.size() / 2);

  auto ecef = wgs84->getGeocentricSRS();

  for (int i = 0; i < verts.size() / 2; ++i)
  {
    osg::Vec3d p1 = verts[i * 2] * invmv;
    osg::Vec3d p2 = verts[i * 2 + 1] * invmv;
    osg::Vec3d out;

    if (ellip.intersectGeocentricLine(p1, p2, out))
    {
      ecef->transform(out, wgs84, out);
      points.push_back(out);
    }
  }

  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CAMERA_GET_VIEWPORT];
  for (int i = 0; i < points.size(); i++) {
    ret += ",";
    ret += std::to_string(points[i].x());
    ret += " ";
    ret += std::to_string(points[i].y());
    //ret += " ";
    //ret += std::to_string(points[i].z());
  }
  ret += "\n";
  return ret;
    
}

std::string
ghRailCommandTrainLabelOn(ghCommandQueue *cmd, ghRail *rail) {

  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }

  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_TRAIN_LABEL_ON];
  ret += cmd->argstr;
  if ( cmd->argstr == GH_STRING_ALL ) {
    ret += rail->SetTrainLabel("",true);
  } else {
    ret += rail->SetTrainLabel(cmd->argstr,true);
  }
  ret += "\n";
  return ret;

}

std::string
ghRailCommandTrainLabelOff(ghCommandQueue *cmd, ghRail *rail) {
  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_TRAIN_LABEL_OFF];
  ret += "off ";
  ret += cmd->argstr;
  if ( cmd->argstr == GH_STRING_ALL ) {
    ret += rail->SetTrainLabel("",false);
  } else {
    ret += rail->SetTrainLabel(cmd->argstr,false);
  }
  ret += "\n";
  return ret;

}

std::string
ghRailCommandTrainPosition(ghCommandQueue *cmd, ghRail *rail, double simtime) {

  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }

  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_TRAIN_POSITION];
  ret += cmd->argstr;
  ret += " ";
  if ( cmd->argstr == GH_STRING_ALL ) {
    ret += rail->GetTrainPosition("",simtime);
  } else {
    ret += rail->GetTrainPosition(cmd->argstr,simtime);
  }
  ret += "\n";
  return ret;

}

std::string
ghRailCommandTrainTimetable(ghCommandQueue *cmd, ghRail *rail) {

  if ( ! rail->IsLoaded() ) {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }

  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_TRAIN_TIMETABLE];
  ret += cmd->argstr;
  ret += " ";
  ret += rail->GetTrainTimetable(cmd->argstr);
  ret += "\n";
  return ret;

}





std::string
ghRailCommandConfigSetMaxspeed(ghCommandQueue *cmd, ghRail *rail) {
  rail->SetClockMaxSpeed(cmd->argnum[0]);
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CONFIG_SET_MAXCLOCKSPEED];
  std::string s_sp = std::to_string(cmd->argnum[0]);
  ret += s_sp;
  ret += "\n";
  return ret;
}
std::string
ghRailCommandConfigGetMaxspeed(ghCommandQueue *cmd, ghRail *rail) {

  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CONFIG_GET_MAXCLOCKSPEED];
  double speed = rail->GetClockMaxSpeed();
  std::string s_sp = std::to_string(speed);
  ret += s_sp;
  ret += "\n";
  return ret;
}

std::string
ghRailCommandConfigSetAltmode(ghCommandQueue *cmd, ghRail *rail) {

  int mode = (int)cmd->argnum[0];
  rail->SetAltmode(mode);
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CONFIG_SET_ALTMODE];
  ret += GH_COMMAND_ALTMODE_STRING[mode];
  return ret;
}

std::string
ghRailCommandConfigGetAltmode(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CONFIG_GET_ALTMODE];
  ret += GH_COMMAND_ALTMODE_STRING[rail->GetAltmode()];
  return ret;
}

std::string
ghRailCommandConfigSetDisplaydistance(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CONFIG_SET_DISPLAYDISTANCE];
  rail->SetDisplayDistance(cmd->argnum[0]);
  std::string dis = std::to_string(cmd->argnum[0]);
  ret += dis;
  return ret;
}
std::string
ghRailCommandConfigGetDisplaydistance(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = GH_COMMAND_MESSAGE[GH_COMMAND_CONFIG_GET_DISPLAYDISTANCE];
  double dis = rail->GetDisplayDistance();
  std::string s_dis = std::to_string(dis);
  ret += s_dis;
  ret += "\n";
  return ret;

}


std::string
ghRailCommandShowStatus(ghCommandQueue *cmd, ghRail *rail) {

  if ( rail->IsLoaded() ) {
    std::string ret = " ";
    if ( rail->IsPlaying() ) {
      ret += "running ";
    } else {
      ret += "paused ";
    }
    ret += "tracking ";
    ret += rail->GetTrackingTrain();
    ret += " speed ";
    ret += std::to_string( rail->GetClockSpeed() );
    ret += "\n";
    return ret;
  } else {
    return GH_COMMAND_MESSAGE_NOT_LOADED;
  }
  
}


std::string
ghRailCommandShowVersion() {
  std::string ret = " ";
  ret += GH_WELCOME_MESSAGE;
  ret += " ";
  ret += GH_REVISION;
  ret += "\n";
  //std::cout << ret << std::endl;
  return ret;
}



