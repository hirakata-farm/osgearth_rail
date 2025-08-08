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
  cmd->argstr = "NOP";
  cmd->isexecute = false;
  cmd->argnum[0]  = 0.0;
  cmd->argnum[1]  = 0.0;
  cmd->argnum[2]  = 0.0;
  
  if ( command_size > 0 )  {
    if ( command[0] == "exit" ) {
      cmd->type = GH_COMMAND_EXIT;
    } else if ( command[0] == "close" ) {
      cmd->type = GH_COMMAND_CLOSE;
    } else if ( command[0] == "start" ) {
      cmd->type = GH_COMMAND_START;
    } else if ( command[0] == "run" ) {
      cmd->type = GH_COMMAND_START;
    } else if ( command[0] == "stop" ) {
      cmd->type = GH_COMMAND_STOP;
    } else if ( command[0] == "pause" ) {
      cmd->type = GH_COMMAND_STOP;
    } else if ( command[0] == "field" ) {
      if ( command_size > 1 )  {
	if ( command[1] == "set" && command_size == 3 ) {
	  //  get arg
	  cmd->type = GH_COMMAND_FIELD_SET;
	  cmd->argstr = command[2];
	} else if ( command[1] == "get" && command_size == 2 ) {
	  cmd->type = GH_COMMAND_FIELD_GET;
	} else if ( command[1] == "get" && command_size == 3 ) {
	  if ( command[2] == "train" ) {
	    cmd->type = GH_COMMAND_FIELD_GET_TRAIN;
	  } else if ( command[2] == "timezone" ) {
	    cmd->type = GH_COMMAND_FIELD_GET_TIMEZONE;
	  } else if ( command[2] == "description" ) {
	    cmd->type = GH_COMMAND_FIELD_GET_DESC;
	  } else {
	    // NOP
	  }
	} else {
	  // NOP
	}
      } 
    } else if ( command[0] == "show" ) {
      if ( command_size > 1 )  {
	if ( command[1] == "status" ) {
	  cmd->type = GH_COMMAND_SHOW_STATUS;
	} else if ( command[1] == "version" ) {
	  cmd->type = GH_COMMAND_SHOW_VERSION;
	} else {
	  // NOP
	}
      }
    } else if ( command[0] == "clock" ) {
      if ( command[1] == "set" ) {
	//  get arg
	if ( command_size > 3 )  {
	  if ( command[2] == "time" && command_size == 4 ) {
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
	  } else if ( command[2] == "speed" && command_size == 4 ) {
	    argdouble = std::stod(command[3]);
	    if ( argdouble < GH_MIN_CLOCK_SPEED ) {
	      cmd->argnum[0] = GH_MIN_CLOCK_SPEED;
	    } else if ( argdouble > GH_MAX_CLOCK_SPEED ) {
	      cmd->argnum[0] = GH_MAX_CLOCK_SPEED;
	    } else {
	      cmd->argnum[0]  = argdouble;
	    }
	    cmd->type = GH_COMMAND_CLOCK_SET_SPEED;
		      
	  } else {
	    // NOP
	  }
	}
      } else if ( command[1] == "get") {
	if ( command[2] == "time" && command_size == 3 ) {
	  cmd->type = GH_COMMAND_CLOCK_GET_TIME;
	} else if ( command[2] == "speed" && command_size == 3 ) {
	  cmd->type = GH_COMMAND_CLOCK_GET_SPEED;	  
	} else {
	  // NOP
	}
      } else {
	// NOP
      }
    } else if ( command[0] == "camera" ) {
      if ( command[1] == "set" ) {
	//  get arg
	if ( command[2] == "position" && command_size == 6 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_POS;
	  cmd->argnum[0] = std::stod(command[3]);
	  cmd->argnum[1] = std::stod(command[4]);
	  cmd->argnum[2] = std::stod(command[5]);
	} else if ( command[2] == "lookat" && command_size == 6 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_LOOK;
	  cmd->argnum[0] = std::stod(command[3]);
	  cmd->argnum[1] = std::stod(command[4]);
	  cmd->argnum[2] = std::stod(command[5]);
	} else if ( command[2] == "upvec" && command_size == 6 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_UP;
	  cmd->argnum[0] = std::stod(command[3]);
	  cmd->argnum[1] = std::stod(command[4]);
	  cmd->argnum[2] = std::stod(command[5]);
	} else if ( command[2] == "tracking" && command_size == 4 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_TRACK;
	  cmd->argstr = command[3];
	} else {
	  // NOP
	}
      } else if ( command[1] == "get" ) {
	if ( command[2] == "position" && command_size == 3 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_POS;
	} else if ( command[2] == "lookat" && command_size == 3 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_LOOK;
	} else if ( command[2] == "upvec" && command_size == 3 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_UP;
	} else if ( command[2] == "tracking" && command_size == 3 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_TRACK;
	} else if ( command[2] == "viewport" && command_size == 3 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_VIEWPORT;
	} else {
	  // NOP
	}
      } else {
	// NOP
      }
    } else if ( command[0] == "train" ) {
      if ( command[1] == "label" ) {
	if ( command[2] == "on" && command_size == 4 ) {
	    cmd->type = GH_COMMAND_TRAIN_LABEL_ON;
	    cmd->argstr = command[3];
	} else if ( command[2] == "off" && command_size == 4 ) {
	    cmd->type = GH_COMMAND_TRAIN_LABEL_OFF;
	    cmd->argstr = command[3];
	} else {
	  // NOP
	}
      } else if ( command[1] == "position" && command_size == 3  ) {
	cmd->type = GH_COMMAND_TRAIN_POSITION;
	cmd->argstr = command[2];
      } else if ( command[1] == "timetable"  && command_size == 3 ) {
	cmd->type = GH_COMMAND_TRAIN_TIMETABLE;
	cmd->argstr = command[2];
      } else {
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
    result += ghRailCommandField(cmd,rail,_sky,true);
    retval = GH_POST_EXECUTE_TIMEZONE;
    break;
  case GH_COMMAND_FIELD_GET:
    result += ghRailCommandField(cmd,rail,_sky,false);
    break;
  case GH_COMMAND_FIELD_GET_TRAIN:
  case GH_COMMAND_FIELD_GET_TIMEZONE:
  case GH_COMMAND_FIELD_GET_DESC:
    result += ghRailCommandFieldDetail(cmd,rail);
    break;
  case GH_COMMAND_CLOCK_SET_TIME:
    result += ghRailCommandClock(cmd,rail,_sky,true);
    retval = GH_POST_EXECUTE_SETCLOCK;
    break;
  case GH_COMMAND_CLOCK_GET_TIME:
    result += ghRailCommandClock(cmd,rail,_sky,false);
    break;
  case GH_COMMAND_CLOCK_SET_SPEED:
    result += ghRailCommandSpeed(cmd,rail,true);
    break;
  case GH_COMMAND_CLOCK_GET_SPEED:
    result += ghRailCommandSpeed(cmd,rail,false);
    break;
  case GH_COMMAND_TRAIN_LABEL_ON:
    result += ghRailCommandTrainLabel(cmd,rail,true);
    break;
  case GH_COMMAND_TRAIN_LABEL_OFF:
    result += ghRailCommandTrainLabel(cmd,rail,false);
    break;
  case GH_COMMAND_TRAIN_POSITION:
    result += ghRailCommandTrainPosition(cmd,rail,simtime);
    break;
  case GH_COMMAND_TRAIN_TIMETABLE:
    result += ghRailCommandTrainTimetable(cmd,rail);
    break;
  case GH_COMMAND_CAMERA_SET_POS:
    result += ghRailCommandCameraPosition(cmd,rail,_view,true);
    break;
  case GH_COMMAND_CAMERA_GET_POS:
    result += ghRailCommandCameraPosition(cmd,rail,_view,false);
    break;
  case GH_COMMAND_CAMERA_SET_LOOK:
    result += ghRailCommandCameraLookat(cmd,rail,_view,true);
    break;
  case GH_COMMAND_CAMERA_GET_LOOK:
    result += ghRailCommandCameraLookat(cmd,rail,_view,false);
    break;
  case GH_COMMAND_CAMERA_SET_UP:
    result += ghRailCommandCameraUpvec(cmd,rail,_view,true);
    break;
  case GH_COMMAND_CAMERA_GET_UP:
    result += ghRailCommandCameraUpvec(cmd,rail,_view,false);
    break;
  case GH_COMMAND_CAMERA_SET_TRACK:
    result += ghRailCommandCameraTracking(cmd,rail,true);
    break;
  case GH_COMMAND_CAMERA_GET_TRACK:
    result += ghRailCommandCameraTracking(cmd,rail,false);
    break;
  case GH_COMMAND_CAMERA_GET_VIEWPORT:
    result += ghRailCommandCameraViewport(cmd,_view);
    break;
  case GH_COMMAND_SHOW_STATUS:
  case GH_COMMAND_SHOW_VERSION:
    result += ghRailCommandShow(cmd,rail);
    break;
  default:
    // NOP
    result += " Unknown command\n";
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
    return " simulation running\n";
  } else {
    return " NOT loaded simulation data\n";
  }

}

std::string
ghRailCommandStop(ghCommandQueue *cmd, ghRail *rail) {

  if ( rail->IsLoaded() ) {
    rail->SetPlayPause(false);
    return " simulation paused\n";
  } else {
    return " NOT loaded simulation data\n";
  }
 
}

std::string
ghRailCommandField(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky, bool isset) {

  int res = -1;
  std::string ret = " field ";

  if ( isset ) {
    ret += cmd->argstr;
    // SET
    if ( rail->IsLoaded() ) {
      return " simulation data already loaded\n";
    }
    res = rail->Setup(cmd->argstr);
    if (  res == GH_SETUP_RESULT_OK ) {
      ret += " loaded\n";
      _sky->setDateTime( rail->GetBaseDatetime());
    } else {
      ret += " can NOT load\n";
    }
    return ret;
  } else {
    // GET
    if ( ! rail->IsLoaded() ) {
      return " simulation data already loaded\n";
    }
    ret += rail->GetConf();
    ret += "\n";
    return ret;
  }


}

std::string
ghRailCommandFieldDetail(ghCommandQueue *cmd, ghRail *rail) {

  if ( ! rail->IsLoaded() ) {
    return " simulation data already loaded\n";
  }
  std::string ret = " field ";

  switch (cmd->type) {
  case GH_COMMAND_FIELD_GET_TRAIN:
    ret += rail->GetUnits();
    break;
  case GH_COMMAND_FIELD_GET_TIMEZONE:
    ret += rail->GetTimezoneStr();
    break;
  case GH_COMMAND_FIELD_GET_DESC:
    ret += rail->GetDescription();
    break;
  }
  ret += "\n";  
  return ret;

}

std::string
ghRailCommandClock(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky, bool isset) {

  if ( ! rail->IsLoaded() ) {
    return " NOT loaded simulation data\n";
  }

  osgEarth::DateTime dt = _sky->getDateTime();

  int year = dt.year();
  int month = dt.month();
  int days = dt.day();
  double dhours = dt.hours();
  int hours = floor(dhours);
  int mins = (int) ( dhours - hours ) * 60;
  double _timezone_hour = (double)( rail->GetTimeZoneMinutes() / 60.0 ); // 0 ~ 24 

  if ( isset ) {
    // SET
    switch (cmd->type) {
    case GH_COMMAND_CLOCK_SET_TIME:
      hours = (int)cmd->argnum[0];
      mins = (int)cmd->argnum[1];
      break;
    default:
      // NOP
      break;
    }
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

    std::string ret = " clock set ";
    std::string s_hour = std::to_string(hours);
    std::string s_min = std::to_string(mins);
    ret += s_hour;
    ret += ":";
    ret += s_min;
    ret += "\n";
    return ret;

  } else {
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

    std::string ret = " clock ";
    std::string s_hour = std::to_string(hours);
    std::string s_min = std::to_string(mins);
    
    switch (cmd->type) {
    case GH_COMMAND_CLOCK_GET_TIME:
      ret += s_hour;
      ret += ":";
      ret += s_min;
      ret += "\n";
      break;
    default:
      ret += s_hour;
      ret += ":";
      ret += s_min;
      ret += "\n";
      break;
    }
    return ret;
  }
  
}

std::string
ghRailCommandSpeed(ghCommandQueue *cmd, ghRail *rail, bool isset ) {

  if ( ! rail->IsLoaded() ) {
    return " NOT loaded simulation data\n";
  }

  if ( isset ) {
    rail->SetSpeed(cmd->argnum[0]);
    std::string ret = " speed set ";
    double speed = rail->GetSpeed();
    std::string s_sp = std::to_string(speed);
    ret += s_sp;
    ret += "\n";
    return ret;
  } else {
    std::string ret = " speed ";
    double speed = rail->GetSpeed();
    std::string s_sp = std::to_string(speed);
    ret += s_sp;
    ret += "\n";
    return ret;
  }
  
}

std::string
ghRailCommandShow(ghCommandQueue *cmd, ghRail *rail) {

  if (cmd->type == GH_COMMAND_SHOW_STATUS ) {
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
      ret += std::to_string( rail->GetSpeed() );
      ret += "\n";
      return ret;
    } else {
      return " NOT loaded simulation data\n";
    }
  } else if (cmd->type == GH_COMMAND_SHOW_VERSION ) {
    std::string ret = " ";
    ret += GH_WELCOME_MESSAGE;
    ret += " ";
    ret += GH_REVISION;
    ret += "\n";
    //std::cout << ret << std::endl;
    return ret;
  } else {
    // Not Yet
    return " Not Yet\n";
  }
  
}

std::string
ghRailCommandTrainLabel(ghCommandQueue *cmd, ghRail *rail, bool isset) {

  if ( ! rail->IsLoaded() ) {
    return " NOT loaded simulation data\n";
  }

  std::string ret = " train label ";
  if ( isset ) {
    ret += "on ";
  } else {
    ret += "off ";
  }
  ret += cmd->argstr;
  ret += rail->SetTrainLabel(cmd->argstr,isset);
  ret += "\n";
  return ret;

}

std::string
ghRailCommandTrainPosition(ghCommandQueue *cmd, ghRail *rail, double simtime) {

  if ( ! rail->IsLoaded() ) {
    return " NOT loaded simulation data\n";
  }

  std::string ret = " train position ";
  ret += cmd->argstr;
  ret += " ";
  ret += rail->GetTrainPosition(cmd->argstr,simtime);
  ret += "\n";
  return ret;

}

std::string
ghRailCommandTrainTimetable(ghCommandQueue *cmd, ghRail *rail) {

  if ( ! rail->IsLoaded() ) {
    return "NOT loaded simulation data\n";
  }

  std::string ret = " train timetable ";
  ret += cmd->argstr;
  ret += " ";
  ret += rail->GetTrainTimetable(cmd->argstr);
  ret += "\n";
  return ret;

}

std::string
ghRailCommandCameraPosition(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view,bool isset) {
  if ( ! rail->IsLoaded() ) {
    return " NOT loaded simulation data\n";
  }

  osgEarth::Ellipsoid WGS84;
  osg::Vec3d eye, up, center;
  osg::Camera *cam = _view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );

  //osgEarth::GeoPoint eyeGeo;
  //eyeGeo.fromWorld( _map->getMapSRS(), eye );
  
  if ( isset ) {
    osg::Vec3d eye2 = osg::Vec3d((double)cmd->argnum[0],(double)cmd->argnum[1], (double)cmd->argnum[2]);
    osg::Matrixd mat ;
    mat.makeLookAt( WGS84.geodeticToGeocentric(eye2), center, up );
    _view->getCameraManipulator()->setByInverseMatrix(mat);

    std::string ret = " camera position set ";
    ret += std::to_string(eye2.x());
    ret += " ";
    ret += std::to_string(eye2.y());
    ret += " ";
    ret += std::to_string(eye2.z());
    ret += "\n";
    return ret;

  } else {

    osg::Vec3 position = WGS84.geocentricToGeodetic(eye);
    //osgEarth::GeoPoint geopoint = osgEarth::GeoPoint(_map->getMapSRS()->getGeographicSRS(), position.x(), position.y(), position.z(), osgEarth::ALTMODE_RELATIVE );
    std::string ret = " camera position ";
    ret += std::to_string(position.x());
    ret += " ";
    ret += std::to_string(position.y());
    ret += " ";
    ret += std::to_string(position.z());
    ret += "\n";
    return ret;

  }
}

std::string
ghRailCommandCameraLookat(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view,bool isset) {
  if ( ! rail->IsLoaded() ) {
    return " NOT loaded simulation data\n";
  }

  osgEarth::Ellipsoid WGS84;
  osg::Vec3d eye, up, center;
  osg::Camera *cam = _view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );

  if ( isset ) {
    osg::Vec3d center2 = osg::Vec3d((double)cmd->argnum[0],(double)cmd->argnum[1], (double)cmd->argnum[2]);
    osg::Matrixd mat ;
    mat.makeLookAt( eye, WGS84.geodeticToGeocentric(center2), up );
    _view->getCameraManipulator()->setByInverseMatrix(mat);

    std::string ret = " camera lookat set ";
    ret += std::to_string(center2.x());
    ret += " ";
    ret += std::to_string(center2.y());
    ret += " ";
    ret += std::to_string(center2.z());
    ret += "\n";
    return ret;

  } else {

    osg::Vec3 center2 = WGS84.geocentricToGeodetic(center);
    std::string ret = " camera lookat ";
    ret += std::to_string(center2.x());
    ret += " ";
    ret += std::to_string(center2.y());
    ret += " ";
    ret += std::to_string(center2.z());
    ret += "\n";
    return ret;

  }
}

std::string
ghRailCommandCameraUpvec(ghCommandQueue *cmd, ghRail *rail, osgViewer::Viewer* _view,bool isset) {
  if ( ! rail->IsLoaded() ) {
    return " NOT loaded simulation data\n";
  }

  osgEarth::Ellipsoid WGS84;
  osg::Vec3d eye, up, center;
  osg::Camera *cam = _view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );

  if ( isset ) {
    osg::Vec3d up2 = osg::Vec3d((double)cmd->argnum[0],(double)cmd->argnum[1], (double)cmd->argnum[2]);
    osg::Matrixd mat ;
    mat.makeLookAt( eye, center, WGS84.geodeticToGeocentric(up2) );
    _view->getCameraManipulator()->setByInverseMatrix(mat);

    std::string ret = " camera upvector set ";
    ret += std::to_string(up2.x());
    ret += " ";
    ret += std::to_string(up2.y());
    ret += " ";
    ret += std::to_string(up2.z());
    ret += "\n";
    return ret;

  } else {

    osg::Vec3 up2 = WGS84.geocentricToGeodetic(up);
    std::string ret = " camera upvector ";
    ret += std::to_string(up2.x());
    ret += " ";
    ret += std::to_string(up2.y());
    ret += " ";
    ret += std::to_string(up2.z());
    ret += "\n";
    return ret;

  }
}

std::string
ghRailCommandCameraTracking(ghCommandQueue *cmd, ghRail *rail, bool isset) {
  if ( ! rail->IsLoaded() ) {
    return " NOT loaded simulation data\n";
  }

  if ( isset ) {
    if ( rail->SetTrackingTrain(cmd->argstr) ) {
      std::string ret = " camera tracking set ";
      ret += cmd->argstr;
      ret += "\n";
      return ret;
    } else {
      return " camera tracking Undefined train\n";
    }
  } else {
    std::string ret = " camera tracking ";
    ret += rail->GetTrackingTrain();
    ret += "\n";
    return ret;
  }
  
  
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

  std::string ret = " camera viewport ";
  for (int i = 0; i < points.size(); i++) {
    ret += ",";
    ret += std::to_string(points[i].x());
    ret += " ";
    ret += std::to_string(points[i].y());
    ret += " ";
    ret += std::to_string(points[i].z());
  }
  ret += "\n";
  return ret;
    
}
