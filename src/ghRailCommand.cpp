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

std::string
GH_COMMAND_MESSAGE[GH_NUMBER_OF_COMMANDS] = {
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
  "camera add ",
  "camera remove ",
  "train label on ",
  "train label off ",  
  "train position ",
  "train timetable ",
  "train icon ",
  "config set max clock speed ",
  "config get max clock speed ",  
  "config set altmode ",
  "config get altmode ",      
  "config set displaydistance ",
  "config get displaydistance ",
  "shm clock time ",
  "shm train position ",
  "shm camera viewport ",
  "shm remove "    
};

std::string
GH_COMMAND_ALTMODE_STRING[3] = {
  GH_STRING_CLAMP,
  GH_STRING_RELATIVE,
  GH_STRING_ABSOLUTE
};

std::vector<std::string> GH_RESERVED_STRING =
{
  GH_STRING_SET,
  GH_STRING_GET,
  GH_STRING_NOP,
  GH_STRING_EXIT,
  GH_STRING_CLOSE,
  GH_STRING_START,
  GH_STRING_RUN,
  GH_STRING_STOP,
  GH_STRING_PAUSE,
  GH_STRING_CLOCK,
  GH_STRING_TIME,
  GH_STRING_SPEED,
  GH_STRING_CAMERA,
  GH_STRING_POSITION,
  GH_STRING_LOOKAT,
  GH_STRING_UPVEC,
  GH_STRING_TRACKING,
  GH_STRING_VIEWPORT,
  GH_STRING_ROOT,
  GH_STRING_NONE,
  GH_STRING_ALL,
  GH_STRING_ADD,
  GH_STRING_FIELD,
  GH_STRING_TRAIN,
  GH_STRING_TIMEZONE,
  GH_STRING_DESCRIPTION,
  GH_STRING_LABEL,
  GH_STRING_ON,
  GH_STRING_OFF,
  GH_STRING_TIMETABLE,
  GH_STRING_ICON,
  GH_STRING_CONFIG,
  GH_STRING_MAXCLOCKSPEED,
  GH_STRING_ALTMODE,
  GH_STRING_CLAMP,
  GH_STRING_RELATIVE,
  GH_STRING_ABSOLUTE,
  GH_STRING_DISPLAYDISTANCE,
  GH_STRING_SHOW,
  GH_STRING_STATUS,
  GH_STRING_VERSION,
  GH_STRING_SHM,
  GH_STRING_REMOVE,
  GH_STRING_OK
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
  cmd->argstridx = 0;
  cmd->argstr[0] = GH_STRING_NOP;
  cmd->argstr[1] = GH_STRING_NOP;  
  cmd->argnumidx = 0;
  cmd->argnum[0]  = 0.0;
  cmd->argnum[1]  = 0.0;
  cmd->argnum[2]  = 0.0;
  cmd->isexecute = false;

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
	  cmd->argstr[0] = command[2];
	  cmd->argstridx = 1;
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
		cmd->argnumidx = 2;
	      }
	    }
	  } else if ( command[2] == GH_STRING_SPEED && command_size == 4 ) {
	    cmd->argnum[0] = std::stod(command[3]);
	    cmd->type = GH_COMMAND_CLOCK_SET_SPEED;
	    cmd->argnumidx = 1;
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
	if ( command[3] == GH_STRING_POSITION && command_size == 7 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_POS;
	  cmd->argstr[0] = command[2]; // Camera Name
	  cmd->argnum[0] = std::stod(command[3]);
	  cmd->argnum[1] = std::stod(command[4]);
	  cmd->argnum[2] = std::stod(command[5]);
	  cmd->argnumidx = 3;
	  cmd->argstridx = 1;	  
	} else if ( command[3] == GH_STRING_LOOKAT && command_size == 7 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_LOOK;
	  cmd->argstr[0] = command[2]; // Camera Name
	  cmd->argnum[0] = std::stod(command[3]);
	  cmd->argnum[1] = std::stod(command[4]);
	  cmd->argnum[2] = std::stod(command[5]);
	  cmd->argnumidx = 3;
	  cmd->argstridx = 1;	  
	} else if ( command[3] == GH_STRING_UPVEC && command_size == 7 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_UP;
	  cmd->argstr[0] = command[2]; // Camera Name
	  cmd->argnum[0] = std::stod(command[3]);
	  cmd->argnum[1] = std::stod(command[4]);
	  cmd->argnum[2] = std::stod(command[5]);
	  cmd->argnumidx = 3;
	  cmd->argstridx = 1;	  
	} else if ( command[3] == GH_STRING_TRACKING && command_size == 5 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_TRACK;
	  cmd->argstr[0] = command[2];  // Camera Name
	  cmd->argstr[1] = command[4];  // train ID
	  cmd->argstridx = 2;	  
	} else {
	  // NOP
	}
      } else if ( command[1] == GH_STRING_GET ) {
	if ( command[3] == GH_STRING_POSITION && command_size == 4 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_POS;
	  cmd->argstr[0] = command[2];  // Camera Name
	  cmd->argstridx = 1;	  
	} else if ( command[3] == GH_STRING_LOOKAT && command_size == 4 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_LOOK;
	  cmd->argstr[0] = command[2];  // Camera Name
	  cmd->argstridx = 1;	  	  
	} else if ( command[3] == GH_STRING_UPVEC && command_size == 4 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_UP;
	  cmd->argstr[0] = command[2];  // Camera Name
	  cmd->argstridx = 1;	  
	} else if ( command[3] == GH_STRING_TRACKING && command_size == 4 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_TRACK;
	  cmd->argstr[0] = command[2];  // Camera Name
	  cmd->argstridx = 1;	  	  
	} else if ( command[3] == GH_STRING_VIEWPORT && command_size == 4 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_VIEWPORT;
	  cmd->argstr[0] = command[2];  // Camera Name
	  cmd->argstridx = 1;	  	  
	} else {
	  // NOP
	}
      } else if ( command[1] == GH_STRING_ADD && command_size == 3 ) {
	cmd->type = GH_COMMAND_CAMERA_ADD;
	cmd->argstr[0] = command[2]; // Camera Name
	cmd->argstridx = 1;	  	  	
      } else if ( command[1] == GH_STRING_REMOVE && command_size == 3 ) {
	cmd->type = GH_COMMAND_CAMERA_REMOVE;
	cmd->argstr[0] = command[2]; // Camera Name
	cmd->argstridx = 1;	  	  		
      } else {
	// NOP
      }
    } else if ( command[0] == GH_STRING_TRAIN ) {
      if ( command[1] == GH_STRING_LABEL ) {
	if ( command[2] == GH_STRING_ON && command_size == 4 ) {
	    cmd->type = GH_COMMAND_TRAIN_LABEL_ON;
	    cmd->argstr[0] = command[3];
	    cmd->argstridx = 1;	  	  		
	} else if ( command[2] == GH_STRING_OFF && command_size == 4 ) {
	    cmd->type = GH_COMMAND_TRAIN_LABEL_OFF;
	    cmd->argstr[0] = command[3];
	    cmd->argstridx = 1;	  	  			    
	} else {
	  // NOP
	}
      } else if ( command[1] == GH_STRING_POSITION && command_size == 3  ) {
	cmd->type = GH_COMMAND_TRAIN_POSITION;
	cmd->argstr[0] = command[2];
	cmd->argstridx = 1;	  	  		
      } else if ( command[1] == GH_STRING_TIMETABLE  && command_size == 3 ) {
	cmd->type = GH_COMMAND_TRAIN_TIMETABLE;
	cmd->argstr[0] = command[2];
	cmd->argstridx = 1;	  	  			
      } else if ( command[1] == GH_STRING_ICON  && command_size == 3 ) {
	cmd->type = GH_COMMAND_TRAIN_ICON;
	cmd->argstr[0] = command[2];
	cmd->argstridx = 1;	  	  				
      } else {
	// NOP
      }
    } else if ( command[0] == GH_STRING_CONFIG ) {
      if ( command[1] == GH_STRING_SET ) {
	  if ( command[2] == GH_STRING_MAXCLOCKSPEED && command_size == 4 ) {
	    cmd->type = GH_COMMAND_CONFIG_SET_MAXCLOCKSPEED;
	    cmd->argnum[0] = std::stod(command[3]);
	    cmd->argnumidx = 1;	  	  			
	  } else if ( command[2] == GH_STRING_ALTMODE && command_size == 4 ) {
	    if ( command[3] == GH_STRING_CLAMP ) {
	      cmd->type = GH_COMMAND_CONFIG_SET_ALTMODE;
	      cmd->argnum[0] = GH_ALTMODE_CLAMP;
	      cmd->argnumidx = 1;	  	  			
	    } else if ( command[3] == GH_STRING_RELATIVE ) {
	      cmd->type = GH_COMMAND_CONFIG_SET_ALTMODE;
	      cmd->argnum[0] = GH_ALTMODE_RELATIVE;
	      cmd->argnumidx = 1;	  	  				      
	    } else if ( command[3] == GH_STRING_ABSOLUTE ) {
	      cmd->type = GH_COMMAND_CONFIG_SET_ALTMODE;
	      cmd->argnum[0] = GH_ALTMODE_ABSOLUTE;
	      cmd->argnumidx = 1;	  	  				      
	    } else {
	      // NOP
	    }
	  } else if ( command[2] == GH_STRING_DISPLAYDISTANCE && command_size == 4 ) {
	    cmd->type = GH_COMMAND_CONFIG_SET_DISPLAYDISTANCE;
	    cmd->argnum[0] = std::stod(command[3]);
	    cmd->argnumidx = 1;	  	  				    
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
    } else if ( command[0] == GH_STRING_SHM ) {
      if ( command[1] == GH_STRING_SET ) {
	if ( command[2] == GH_STRING_CLOCK && command[3] == GH_STRING_TIME && command_size == 4 ) {
	  cmd->type = GH_COMMAND_SHM_CLOCK_TIME;
	} else if ( command[2] == GH_STRING_TRAIN && command[3] == GH_STRING_POSITION && command_size == 4 ) {
	  cmd->type = GH_COMMAND_SHM_TRAIN_POS;
	} else if ( command[2] == GH_STRING_CAMERA && command[4] == GH_STRING_VIEWPORT && command_size == 5 ) {
	  cmd->type = GH_COMMAND_SHM_CAMERA_VIEW;
	  cmd->argstr[0] = command[3];
	  cmd->argstridx = 1;	  	  			
	} else {
	  // NOP
	}
      }	else if ( command[1] == GH_STRING_REMOVE && command_size == 3 ) {
	cmd->type = GH_COMMAND_SHM_REMOVE;
	cmd->argnum[0] = std::stod(command[2]);
	cmd->argnumidx = 1;	  	  			
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
		      ghWindow* _win,
		      osgEarth::SkyNode* _sky,
		      double simtime ) {

  int retval = GH_POST_EXECUTE_NONE;
  if ( cmd == NULL ) return retval;
  if ( cmd->isexecute ) return retval; // Already execute

  char resultstring[GH_EXECUTE_BUFFER_SIZE];
  int  resultcode;

  memset(&resultstring[0], 0, sizeof(char)*GH_EXECUTE_BUFFER_SIZE);

  if ( rail->IsLoaded() ) {
    // simulation data loaded
    switch (cmd->type) {
    case GH_COMMAND_EXIT:
      retval = GH_POST_EXECUTE_EXIT;
      resultcode = GH_EXECUTE_SUCCESS;      
      break;
    case GH_COMMAND_CLOSE:
      retval = GH_POST_EXECUTE_CLOSE;
      resultcode = GH_EXECUTE_SUCCESS;      
      break;
    case GH_COMMAND_START:
      rail->SetPlayPause(true);
      resultcode = GH_EXECUTE_SUCCESS;
      break;
    case GH_COMMAND_STOP:
      rail->SetPlayPause(false);
      resultcode = GH_EXECUTE_SUCCESS;      
      break;
    case GH_COMMAND_FIELD_GET:
      resultcode = ghRailCommandFieldGet(cmd,rail,&resultstring[0]);
      break;
    case GH_COMMAND_FIELD_GET_TRAIN:
      resultcode = ghRailCommandFieldTrain(cmd,rail,&resultstring[0]);
      break;
    case GH_COMMAND_FIELD_GET_TIMEZONE:
      resultcode = ghRailCommandFieldTimezone(cmd,rail,&resultstring[0]);
      break;
    case GH_COMMAND_FIELD_GET_DESC:
      resultcode = ghRailCommandFieldDescription(cmd,rail,&resultstring[0]);
      break;
    case GH_COMMAND_CLOCK_SET_TIME:
      resultcode = ghRailCommandClockSetTime(cmd,rail,_sky);
      if ( resultcode == GH_EXECUTE_SUCCESS ) retval = GH_POST_EXECUTE_SETCLOCK;
      break;
    case GH_COMMAND_CLOCK_GET_TIME:
      resultcode = ghRailCommandClockGetTime(cmd,rail,_sky,&resultstring[0]);
      break;
    case GH_COMMAND_CLOCK_SET_SPEED:
      resultcode = ghRailCommandClockSetSpeed(cmd,rail);
      break;
    case GH_COMMAND_CLOCK_GET_SPEED:
      resultcode = ghRailCommandClockGetSpeed(cmd,rail,&resultstring[0]);
      break;
    case GH_COMMAND_TRAIN_LABEL_ON:
      resultcode = ghRailCommandTrainLabel(cmd,rail,true);
      break;
    case GH_COMMAND_TRAIN_LABEL_OFF:
      resultcode = ghRailCommandTrainLabel(cmd,rail,false);
      break;
    case GH_COMMAND_TRAIN_POSITION:
      resultcode = ghRailCommandTrainPosition(cmd,rail,simtime,&resultstring[0]);
      break;
    case GH_COMMAND_TRAIN_TIMETABLE:
      resultcode = ghRailCommandTrainTimetable(cmd,rail,&resultstring[0]);
      break;
    case GH_COMMAND_TRAIN_ICON:
      resultcode = ghRailCommandTrainIcon(cmd,rail,&resultstring[0]);
      break;
    case GH_COMMAND_CAMERA_SET_POS:
      resultcode = ghRailCommandCameraSetPosition(cmd,rail,_win);
      break;
    case GH_COMMAND_CAMERA_GET_POS:
      resultcode = ghRailCommandCameraGetPosition(cmd,rail,_win,&resultstring[0]);
      break;
    case GH_COMMAND_CAMERA_SET_LOOK:
      resultcode = ghRailCommandCameraSetLookat(cmd,rail,_win);
      break;
    case GH_COMMAND_CAMERA_GET_LOOK:
      resultcode = ghRailCommandCameraGetLookat(cmd,rail,_win,&resultstring[0]);
      break;
    case GH_COMMAND_CAMERA_SET_UP:
      resultcode = ghRailCommandCameraSetUpvec(cmd,rail,_win);
      break;
    case GH_COMMAND_CAMERA_GET_UP:
      resultcode = ghRailCommandCameraGetUpvec(cmd,rail,_win,&resultstring[0]);
      break;
    case GH_COMMAND_CAMERA_SET_TRACK:
      resultcode = ghRailCommandCameraSetTracking(cmd,rail,_win);
      break;
    case GH_COMMAND_CAMERA_GET_TRACK:
      resultcode = ghRailCommandCameraGetTracking(cmd,rail,_win,&resultstring[0]);
      break;
    case GH_COMMAND_CAMERA_GET_VIEWPORT:
      resultcode = ghRailCommandCameraViewport(cmd,rail,_win,&resultstring[0]);
      break;
    case GH_COMMAND_CAMERA_ADD:
      resultcode = ghRailCommandCameraAdd(cmd,rail,_win);
      if ( resultcode == GH_EXECUTE_SUCCESS ) retval = GH_POST_EXECUTE_CAMERA_ADD;
      break;
    case GH_COMMAND_CAMERA_REMOVE:
      resultcode = ghRailCommandCameraRemove(cmd,rail,_win);
      if ( resultcode == GH_EXECUTE_SUCCESS ) retval = GH_POST_EXECUTE_CAMERA_REMOVE;
      break;
    case GH_COMMAND_CONFIG_SET_MAXCLOCKSPEED:
      resultcode = ghRailCommandConfigSetMaxspeed(cmd,rail);
      break;
    case GH_COMMAND_CONFIG_GET_MAXCLOCKSPEED:
      resultcode = ghRailCommandConfigGetMaxspeed(cmd,rail,&resultstring[0]);
      break;
    case GH_COMMAND_CONFIG_SET_ALTMODE:
      resultcode = ghRailCommandConfigSetAltmode(cmd,rail);
      break;
    case GH_COMMAND_CONFIG_GET_ALTMODE:
      resultcode = ghRailCommandConfigGetAltmode(cmd,rail,&resultstring[0]);
      break;
    case GH_COMMAND_CONFIG_SET_DISPLAYDISTANCE:
      resultcode = ghRailCommandConfigSetDisplaydistance(cmd,rail);
      break;
    case GH_COMMAND_CONFIG_GET_DISPLAYDISTANCE:
      resultcode = ghRailCommandConfigGetDisplaydistance(cmd,rail,&resultstring[0]);
      break;
    case GH_COMMAND_SHM_CLOCK_TIME:
      resultcode = ghRailCommandShmSet(GH_SHM_TYPE_CLOCK_TIME,cmd,rail,NULL,&resultstring[0]);
      break;
    case GH_COMMAND_SHM_TRAIN_POS:
      resultcode = ghRailCommandShmSet(GH_SHM_TYPE_TRAIN_POSITION,cmd,rail,NULL,&resultstring[0]);
      break;
    case GH_COMMAND_SHM_CAMERA_VIEW:
      resultcode = ghRailCommandShmSet(GH_SHM_TYPE_CAMERA_VIEWPORT,cmd,rail,_win,&resultstring[0]);
      break;
    case GH_COMMAND_SHM_REMOVE:
      resultcode = ghRailCommandShmRemove(cmd,rail);
      break;
    case GH_COMMAND_SHOW_STATUS:
      resultcode = ghRailCommandShowStatus(cmd,rail,_win,&resultstring[0]);
      break;
    case GH_COMMAND_SHOW_VERSION:
      resultcode = ghRailCommandShowVersion(&resultstring[0]);
      break;
    default:
      resultcode = GH_EXECUTE_UNKNOWN;
    }
  } else {
    // Not loaded
    switch (cmd->type) {
    case GH_COMMAND_EXIT:
      retval = GH_POST_EXECUTE_EXIT;
      resultcode = GH_EXECUTE_SUCCESS;      
      break;
    case GH_COMMAND_CLOSE:
      retval = GH_POST_EXECUTE_CLOSE;
      resultcode = GH_EXECUTE_SUCCESS;      
      break;
    case GH_COMMAND_FIELD_SET:
      resultcode = ghRailCommandFieldSet(cmd,rail,_sky);
      if ( resultcode == GH_EXECUTE_SUCCESS ) retval = GH_POST_EXECUTE_TIMEZONE;
      break;
    case GH_COMMAND_SHOW_STATUS:
      resultcode = ghRailCommandShowStatus(cmd,rail,_win,&resultstring[0]);
      break;
    case GH_COMMAND_SHOW_VERSION:
      resultcode = ghRailCommandShowVersion(&resultstring[0]);
      break;
    default:
      resultcode = GH_EXECUTE_NOT_LOADED;
    }
  }

  std::string result = ghRailReturnMessage(cmd,resultcode,&resultstring[0]);
  const char* retmsg = result.c_str();
  send(socket, retmsg, std::strlen(retmsg), 0);
  cmd->isexecute = true;    

  return retval;
}

std::string
ghRailReturnMessage(ghCommandQueue *cmd,int code, char *message){

  std::string result = "Geoglyph:";
  result += GH_COMMAND_MESSAGE[cmd->type];
  int messagelength = std::strlen(message);
  //std::cout << messagelength << std::endl;

  for (int i = 0; i < cmd->argstridx; i++) {
    result += " ";
    result += cmd->argstr[i];
  }
  if ( code == GH_EXECUTE_SUCCESS ) {
    // NOP
  } else if ( code == GH_EXECUTE_UNKNOWN ) {
    result += " Unknown";
  } else if ( code == GH_EXECUTE_NOT_LOADED ) {
    result += " not loaded";
  } else if ( code == GH_EXECUTE_CANNOT_LOAD ) {
    result += " cannot load";
  } else if ( code == GH_EXECUTE_SIZE_ERROR ) {
    result += " size error";
  } else if ( code == GH_EXECUTE_NOT_FOUND ) {
    result += " not found";
  } else if ( code == GH_EXECUTE_ALREADY_EXIST ) {
    result += " already exist";
  } else if ( code == GH_EXECUTE_RESERVED ) {
    result += " cannot use reserved keywod";
  } else if ( code == GH_EXECUTE_CANNOT_GET ) {
    result += " cannot get data";
  } else if ( code == GH_EXECUTE_CANNOT_ALLOCATE ) {
    result += " cannot allocate data";
  } else {
    // NOP
  }
  for (int i = 0; i < cmd->argnumidx; i++) {
    result += " ";
    result += std::to_string(cmd->argnum[i]);
  }

  if ( messagelength > 0 ) {
    result += " ";
    std::string messagestr(message); 
    result += messagestr.c_str();
  }
  
  return result;
}


int
ghRailCommandFieldSet(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky) {
  int res = -1;
  res = rail->Setup(cmd->argstr[0]);
  if (  res == GH_SETUP_RESULT_OK ) {
    _sky->setDateTime( rail->GetBaseDatetime());
    return GH_EXECUTE_SUCCESS;
  } else {
    std::cout << "Error field code " << res << std::endl;
    return GH_EXECUTE_CANNOT_LOAD;
  }
}
int
ghRailCommandFieldGet(ghCommandQueue *cmd, ghRail *rail, char *result) {

  std::string ret(rail->GetConfigure());
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandFieldTrain(ghCommandQueue *cmd, ghRail *rail, char *result) {
  std::string ret(rail->GetUnits());
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}
int
ghRailCommandFieldTimezone(ghCommandQueue *cmd, ghRail *rail, char *result) {
  std::string ret(rail->GetTimezoneStr());
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}
int
ghRailCommandFieldDescription(ghCommandQueue *cmd, ghRail *rail, char *result) {
  std::string ret(rail->GetDescription());
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandClockSetTime(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky) {

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

  return GH_EXECUTE_SUCCESS;
}

int
ghRailCommandClockGetTime(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky, char *result) {

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
  std::string s_min = std::to_string(mins);
  ret += ":";
  ret += s_min;

  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
  
}

int
ghRailCommandClockSetSpeed(ghCommandQueue *cmd, ghRail *rail) {
  rail->SetClockSpeed(cmd->argnum[0]);
  return GH_EXECUTE_SUCCESS;
}

int
ghRailCommandClockGetSpeed(ghCommandQueue *cmd, ghRail *rail, char *result) {
  double speed = rail->GetClockSpeed();
  std::string ret = std::to_string(speed);
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandTrainLabel(ghCommandQueue *cmd, ghRail *rail,bool flag) {
  bool ret ;
  if ( cmd->argstr[0] == GH_STRING_ALL ) {
    ret = rail->SetTrainLabel("",flag);
  } else {
    ret = rail->SetTrainLabel(cmd->argstr[0],flag);
  }
  if ( ret ) {
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_NOT_FOUND;
  }
}

int
ghRailCommandTrainPosition(ghCommandQueue *cmd, ghRail *rail, double simtime,char *result) {
  
  std::string ret = " ";
  if ( cmd->argstr[0] == GH_STRING_ALL ) {
    ret += rail->GetTrainPosition("",simtime);
  } else {
    if ( rail->IsTrainID(cmd->argstr[0]) ) {
      ret += rail->GetTrainPosition(cmd->argstr[0],simtime);
    } else {
      return GH_EXECUTE_NOT_FOUND;
    }
  }
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandTrainTimetable(ghCommandQueue *cmd, ghRail *rail,char *result) {

  std::string ret = " ";
  if ( rail->IsTrainID(cmd->argstr[0]) ) {
    ret += rail->GetTrainTimetable(cmd->argstr[0]);
  } else {
    return GH_EXECUTE_NOT_FOUND;
  }
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandTrainIcon(ghCommandQueue *cmd, ghRail *rail,char *result) {
  std::string ret = " ";
  if ( rail->IsTrainID(cmd->argstr[0]) ) {
    ret += rail->GetTrainIcon(cmd->argstr[0]);
  } else {
    return GH_EXECUTE_NOT_FOUND;
  }
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}


int
ghRailCommandCameraSetPosition(ghCommandQueue *cmd, ghRail *rail,ghWindow* _win) {

  osgEarth::Ellipsoid WGS84;
  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  if ( tmp == (ghWindow *)NULL ) {
    return GH_EXECUTE_NOT_FOUND;
  }
  osg::Vec3d eye, up, center;
  osg::Camera *cam = tmp->view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );
  osg::Vec3d eye2 = osg::Vec3d((double)cmd->argnum[0],(double)cmd->argnum[1], (double)cmd->argnum[2]);
  osg::Matrixd mat ;
  mat.makeLookAt( WGS84.geodeticToGeocentric(eye2), center, up );
  tmp->view->getCameraManipulator()->setByInverseMatrix(mat);
  return GH_EXECUTE_SUCCESS;
}

int
ghRailCommandCameraGetPosition(ghCommandQueue *cmd, ghRail *rail,ghWindow* _win,char *result) {
  osgEarth::Ellipsoid WGS84;
  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  if ( tmp == (ghWindow *)NULL ) {
    return GH_EXECUTE_NOT_FOUND;
  }
  osg::Vec3d eye, up, center;
  osg::Camera *cam = tmp->view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );
  
  osg::Vec3 position = WGS84.geocentricToGeodetic(eye);
  std::string ret = std::to_string(position.x());
  ret += " ";
  ret += std::to_string(position.y());
  ret += " ";
  ret += std::to_string(position.z());
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandCameraSetLookat(ghCommandQueue *cmd, ghRail *rail, ghWindow* _win) {

  osgEarth::Ellipsoid WGS84;
  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  if ( tmp == (ghWindow *)NULL ) {
    return GH_EXECUTE_NOT_FOUND;
  }
  osg::Vec3d eye, up, center;
  osg::Camera *cam = tmp->view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );

  osg::Vec3d center2 = osg::Vec3d((double)cmd->argnum[0],(double)cmd->argnum[1], (double)cmd->argnum[2]);
  osg::Matrixd mat ;
  mat.makeLookAt( eye, WGS84.geodeticToGeocentric(center2), up );
  tmp->view->getCameraManipulator()->setByInverseMatrix(mat);
  return GH_EXECUTE_SUCCESS;
}
int
ghRailCommandCameraGetLookat(ghCommandQueue *cmd, ghRail *rail, ghWindow* _win,char *result) {

  osgEarth::Ellipsoid WGS84;
  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  if ( tmp == (ghWindow *)NULL ) {
    return GH_EXECUTE_NOT_FOUND;
  }
  osg::Vec3d eye, up, center;
  osg::Camera *cam = tmp->view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );

  osg::Vec3 center2 = WGS84.geocentricToGeodetic(center);
  std::string ret = std::to_string(center2.x());
  ret += " ";
  ret += std::to_string(center2.y());
  ret += " ";
  ret += std::to_string(center2.z());
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandCameraSetUpvec(ghCommandQueue *cmd, ghRail *rail, ghWindow* _win) {

  osgEarth::Ellipsoid WGS84;
  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  if ( tmp == (ghWindow *)NULL ) {
    return GH_EXECUTE_NOT_FOUND;
  }
  osg::Vec3d eye, up, center;
  osg::Camera *cam = tmp->view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );

  osg::Vec3d up2 = osg::Vec3d((double)cmd->argnum[0],(double)cmd->argnum[1], (double)cmd->argnum[2]);
  osg::Matrixd mat ;
  mat.makeLookAt( eye, center, WGS84.geodeticToGeocentric(up2) );
  tmp->view->getCameraManipulator()->setByInverseMatrix(mat);
  return GH_EXECUTE_SUCCESS;
}
int
ghRailCommandCameraGetUpvec(ghCommandQueue *cmd, ghRail *rail, ghWindow* _win,char *result) {
  osgEarth::Ellipsoid WGS84;
  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  if ( tmp == (ghWindow *)NULL ) {
    return GH_EXECUTE_NOT_FOUND;
  }
  osg::Vec3d eye, up, center;
  osg::Camera *cam = tmp->view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );

  osg::Vec3 up2 = WGS84.geocentricToGeodetic(up);
  std::string ret = std::to_string(up2.x());
  ret += " ";
  ret += std::to_string(up2.y());
  ret += " ";
  ret += std::to_string(up2.z());
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandCameraSetTracking(ghCommandQueue *cmd, ghRail *rail,ghWindow* _win) {
  if ( rail->IsTrainID(cmd->argstr[1]) ) {
    ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
    if ( tmp != NULL ) {
      tmp->tracking = cmd->argstr[1];
      return GH_EXECUTE_SUCCESS;
    } else {
      return GH_EXECUTE_NOT_FOUND;
    }
  } else {
    return GH_EXECUTE_NOT_FOUND;
  }
}

int
ghRailCommandCameraGetTracking(ghCommandQueue *cmd, ghRail *rail,ghWindow* _win,char *result) {
  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  if ( tmp == (ghWindow *)NULL ) {
      return GH_EXECUTE_NOT_FOUND;
  }
  std::string ret(tmp->tracking);
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandCameraViewport(ghCommandQueue *cmd, ghRail *rail, ghWindow* _win,char *result) {

  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  if ( tmp == (ghWindow *)NULL ) {
      return GH_EXECUTE_NOT_FOUND;
  }
  std::vector<osg::Vec3d> points = _calcCameraViewpoints(tmp->view);
  std::string ret = " ";
  // points max 12
  for (int i = 0; i < points.size(); i++) {
    ret += ",";
    ret += std::to_string(points[i].x());
    ret += " ";
    ret += std::to_string(points[i].y());
    //ret += " ";
    //ret += std::to_string(points[i].z());
  }
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
    
}

int
ghRailCommandCameraAdd(ghCommandQueue *cmd, ghRail *rail, ghWindow* _win) {
  string cname = cmd->argstr[0];
  ghWindow *tmp = _win;
  for (int i = 0; i < GH_RESERVED_STRING.size(); i++) {
    if ( GH_RESERVED_STRING[i] == cname ) {
      return GH_EXECUTE_RESERVED;
    }
  }
  while (tmp != (ghWindow *)NULL) {
    if ( tmp->name == cname ) {
      return GH_EXECUTE_ALREADY_EXIST;
    }
    tmp = tmp->next ;
  }
  return GH_EXECUTE_SUCCESS;
}

int
ghRailCommandCameraRemove(ghCommandQueue *cmd, ghRail *rail, ghWindow* _win) {
  string cname = cmd->argstr[0];
  ghWindow *tmp = _win;
  while (tmp != (ghWindow *)NULL) {
    if ( tmp->name == cname ) {
      return GH_EXECUTE_SUCCESS;
    }
    tmp = tmp->next ;
  }
  return GH_EXECUTE_NOT_FOUND;
}


int
ghRailCommandConfigSetMaxspeed(ghCommandQueue *cmd, ghRail *rail) {
  rail->SetClockMaxSpeed(cmd->argnum[0]);
  return GH_EXECUTE_SUCCESS;
}
int
ghRailCommandConfigGetMaxspeed(ghCommandQueue *cmd, ghRail *rail,char *result) {
  double speed = rail->GetClockMaxSpeed();
  std::string ret = std::to_string(speed);
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandConfigSetAltmode(ghCommandQueue *cmd, ghRail *rail) {
  int mode = (int)cmd->argnum[0];
  rail->SetAltmode(mode);
  return GH_EXECUTE_SUCCESS;
}

int
ghRailCommandConfigGetAltmode(ghCommandQueue *cmd, ghRail *rail,char *result) {
  std::string ret = GH_COMMAND_ALTMODE_STRING[rail->GetAltmode()];
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandConfigSetDisplaydistance(ghCommandQueue *cmd, ghRail *rail) {
  rail->SetDisplayDistance(cmd->argnum[0]);
  return GH_EXECUTE_SUCCESS;
}
int
ghRailCommandConfigGetDisplaydistance(ghCommandQueue *cmd, ghRail *rail,char *result) {
  double dis = rail->GetDisplayDistance();
  std::string ret = std::to_string(dis);
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandShmSet(int shmtype,ghCommandQueue *cmd,ghRail *rail,ghWindow* _win,char *result) {

  int offset = 0;
  if ( _win != (ghWindow *)NULL ) {
    offset = 24 + ghCountWindow(_win);
  }
  int shmkey = ftok(GH_SHM_PATH,shmtype+offset);
  std::string ret = " ";
  if (shmkey < 0) {
    return GH_EXECUTE_CANNOT_GET;
  } else {
    std::string skey = std::to_string(shmkey);
    ret += skey + " ";
    int ssize = -1;
    if ( shmtype == GH_SHM_TYPE_CLOCK_TIME ) {
      ssize = rail->InitShmClock(shmkey);
    } else if ( shmtype == GH_SHM_TYPE_TRAIN_POSITION ) {
      ssize = rail->InitShmTrain(shmkey);
    } else if ( shmtype == GH_SHM_TYPE_CAMERA_VIEWPORT ) {
      ssize = ghInitShmWindow(shmkey,_win,cmd->argstr[0]);
    } else {
      return GH_EXECUTE_NOT_FOUND;
    }
    if ( ssize < 0 ) {
      return GH_EXECUTE_CANNOT_ALLOCATE;
    } else {
      ret += std::to_string(ssize);
    }
  }
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandShmRemove(ghCommandQueue *cmd,ghRail *rail) {
  int shmkey = (int)cmd->argnum[0];
  int ssize = rail->RemoveShm(shmkey);
  return GH_EXECUTE_SUCCESS;
}

int
ghRailCommandShowStatus(ghCommandQueue *cmd, ghRail *rail, ghWindow* _win,char *result) {

  ghWindow *wtmp = _win;
  std::string ret = " ";
  if ( rail->IsLoaded() ) {
    if ( rail->IsPlaying() ) {
      ret += "running ";
    } else {
      ret += "paused ";
    }
    while (wtmp != (ghWindow *)NULL) {
      ret += "camera ";
      ret += wtmp->name;
      ret += " tracking ";
      ret += wtmp->tracking;
      wtmp = wtmp->next ;
    }
    ret += " speed ";
    ret += std::to_string( rail->GetClockSpeed() );
  } else {
    ret += " not loaded yet.";
  }

  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
  
  
}

int
ghRailCommandShowVersion(char *result) {
  std::string ret = " ";
  ret += GH_APP_NAME;
  ret += " ";
  ret += GH_APP_REVISION;
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}



