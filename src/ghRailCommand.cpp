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
  GH_STRING_WINDOW,
  GH_STRING_SCREEN,
  GH_STRING_NONE,
  GH_STRING_ALL,
  GH_STRING_ADD,
  GH_STRING_FIELD,
  GH_STRING_TRAIN,
  GH_STRING_LINE,
  GH_STRING_DISTANCE,
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
  GH_STRING_MAXWINDOW,
  GH_STRING_SHOW,
  GH_STRING_STATUS,
  GH_STRING_VERSION,
  GH_STRING_SHM,
  GH_STRING_REMOVE,
  GH_STRING_OK
};

std::string
GH_COMMAND_ALTMODE_STRING[3] = {
  GH_STRING_CLAMP,
  GH_STRING_RELATIVE,
  GH_STRING_ABSOLUTE
};

ghCommandQueue *
ghRailInitCommandQueue(std::string buffer) {

  if ( buffer.size() < GH_COMMAND_MIN_SIZE ) {
    return( (ghCommandQueue *)NULL ) ;
  }
  
  ghCommandQueue *cmd;
  if ((cmd = (ghCommandQueue *)calloc( (unsigned)1, (unsigned)sizeof( ghCommandQueue ) )) == NULL)
    {
      return( (ghCommandQueue *)NULL ) ;
    }
  size_t endpos = buffer.find_last_not_of("\r\n");
  if (endpos != std::string::npos) {
    buffer.erase(endpos + 1);
  } else {
    buffer.clear();
    return( (ghCommandQueue *)NULL ) ;
  }
  cmd->type = GH_COMMAND_UNKNOWN;
#ifdef _WINDOWS
  cmd->recv = buffer.substr(0,buffer.size());
#else
  cmd->recv = buffer; //  Bug for Windows 11  Why??
#endif  
  cmd->argstridx = 0;
  cmd->argstr[0] = GH_STRING_NOP;
  cmd->argstr[1] = GH_STRING_NOP;  
  cmd->argnumidx = 0;
  cmd->argnum[0]  = 0.0;
  cmd->argnum[1]  = 0.0;
  cmd->argnum[2]  = 0.0;
  cmd->state         = GH_QUEUE_STATE_INIT;
  cmd->executecode   = GH_EXECUTE_INIT;
  cmd->resultmessage = GH_STRING_NOP;
  cmd->prev = (ghCommandQueue *)NULL;
  return cmd;
}

void
ghRailParseCommand(ghCommandQueue *cmd) {

  std::vector<std::string> command = ghStringSplit(cmd->recv, ' ');
  int command_size = command.size();
  int argint = -1;
  double argdouble = -1;

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
	  } else if ( command[2] == GH_STRING_LINE ) {
	    cmd->type = GH_COMMAND_FIELD_GET_LINE;
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
	//  set arg
	if ( command[3] == GH_STRING_POSITION && command_size == 7 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_POS;
	  cmd->argstr[0] = command[2]; // Camera Name
	  cmd->argnum[0] = std::stod(command[4]);
	  cmd->argnum[1] = std::stod(command[5]);
	  cmd->argnum[2] = std::stod(command[6]);
	  cmd->argnumidx = 3;
	  cmd->argstridx = 1;	  
	} else if ( command[3] == GH_STRING_LOOKAT && command_size == 7 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_LOOK;
	  cmd->argstr[0] = command[2]; // Camera Name
	  cmd->argnum[0] = std::stod(command[4]);
	  cmd->argnum[1] = std::stod(command[5]);
	  cmd->argnum[2] = std::stod(command[6]);
	  cmd->argnumidx = 3;
	  cmd->argstridx = 1;	  
	} else if ( command[3] == GH_STRING_UPVEC && command_size == 7 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_UP;
	  cmd->argstr[0] = command[2]; // Camera Name
	  cmd->argnum[0] = std::stod(command[4]);
	  cmd->argnum[1] = std::stod(command[5]);
	  cmd->argnum[2] = std::stod(command[6]);
	  cmd->argnumidx = 3;
	  cmd->argstridx = 1;	  
	} else if ( command[3] == GH_STRING_TRACKING && command_size == 5 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_TRACK;
	  cmd->argstr[0] = command[2];  // Camera Name
	  cmd->argstr[1] = command[4];  // train ID
	  cmd->argstridx = 2;	  
	} else if ( command[3] == GH_STRING_SCREEN && ( command_size == 6 || command_size == 7 ) )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_SCREEN;
	  cmd->argstr[0] = command[2];  // Camera Name
	  cmd->argstridx = 1;	  
	  cmd->argnum[0] = std::stod(command[4]);  // screen position x 
	  cmd->argnum[1] = std::stod(command[5]);  // screen position y
	  cmd->argnumidx = 2;
	  if ( command_size == 7 ) {
	    cmd->argnum[2] = std::stod(command[6]);  // screen num
	    cmd->argnumidx = 3;
	  }
	} else if ( command[3] == GH_STRING_WINDOW && command_size == 6 )  {
	  cmd->type = GH_COMMAND_CAMERA_SET_WINDOW;
	  cmd->argstr[0] = command[2];  // Camera Name
	  cmd->argstridx = 1;	  
	  cmd->argnum[0] = std::stod(command[4]);  // window width
	  cmd->argnum[1] = std::stod(command[5]);  // window height
	  cmd->argnumidx = 2;
	} else {
	  // NOP
	}
      } else if ( command[1] == GH_STRING_GET ) {
	//  get arg	
	if ( command_size == 2 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET;
	} else if ( command[3] == GH_STRING_POSITION && command_size == 4 )  {
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
	} else if ( command[3] == GH_STRING_SCREEN && command_size == 4 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_SCREEN;
	  cmd->argstr[0] = command[2];  // Camera Name
	  cmd->argstridx = 1;	  	  
	} else if ( command[3] == GH_STRING_WINDOW && command_size == 4 )  {
	  cmd->type = GH_COMMAND_CAMERA_GET_WINDOW;
	  cmd->argstr[0] = command[2];  // Camera Name
	  cmd->argstridx = 1;	  	  
	} else {
	  // NOP
	}
      } else if ( command[1] == GH_STRING_ADD && command_size == 6 ) {
	cmd->type = GH_COMMAND_CAMERA_ADD;
	cmd->argstr[0] = command[2]; // Camera Name
	cmd->argstridx = 1;	  	  	
	cmd->argnum[0] = std::stod(command[3]);
	if ( cmd->argnum[0] < 0.0 ) cmd->argnum[0] = 0.0;
	cmd->argnum[1] = std::stod(command[4]);
	if ( cmd->argnum[1] < 0.0 ) cmd->argnum[1] = 0.0;
	cmd->argnum[2] = std::stod(command[5]);
	if ( cmd->argnum[2] < 0.1 ) cmd->argnum[2] = 0.1;
	if ( cmd->argnum[2] > 1.0 ) cmd->argnum[2] = 1.0;
	cmd->argnumidx = 3;	  	  	
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
      } else if ( command[1] == GH_STRING_LINE  && command_size == 3 ) {
	cmd->type = GH_COMMAND_TRAIN_LINE;
	cmd->argstr[0] = command[2];
	cmd->argstridx = 1;	  	  				
      } else if ( command[1] == GH_STRING_DISTANCE  && command_size == 3 ) {
	cmd->type = GH_COMMAND_TRAIN_DISTANCE;
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
	  } else if ( command[2] == GH_STRING_MAXWINDOW && command_size == 4 ) {
	    cmd->type = GH_COMMAND_CONFIG_SET_MAXWINDOW;
	    cmd->argnum[0] = std::stoi(command[3]);
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
	} else if ( command[2] == GH_STRING_MAXWINDOW  ) {
	  cmd->type = GH_COMMAND_CONFIG_GET_MAXWINDOW;
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

  cmd->state = GH_QUEUE_STATE_PARSED;
  //return cmd;

}


////////////////////////////////////////////////////////////


void
ghRailExecuteCommand( ghCommandQueue *cmd, ghRail *rail ,  double simtime , osgEarth::SkyNode* _sky, std::map<std::string, ghWindow>& _wins)
{

  if ( cmd == NULL ) return;
  if ( rail == NULL ) return;
  if ( cmd->state == GH_QUEUE_STATE_PARSED ) {
    // NOP continue
  } else {
    return;
  }

  if ( rail->IsLoaded() ) {
    //
    // simulation data already loaded
    //
    switch (cmd->type) {
    case GH_COMMAND_EXIT:
      cmd->executecode = GH_EXECUTE_SUCCESS;
      cmd->resultmessage = "exit";
      cmd->state = GH_QUEUE_STATE_EXECUTED;                              
      break;
    case GH_COMMAND_CLOSE:
      cmd->executecode = GH_EXECUTE_SUCCESS;
      cmd->resultmessage = "close";
      cmd->state = GH_QUEUE_STATE_EXECUTED;                              
      break;
    case GH_COMMAND_START:
      rail->SetPlayPause(true);
      cmd->executecode = GH_EXECUTE_SUCCESS;
      cmd->resultmessage = "start";
      cmd->state = GH_QUEUE_STATE_EXECUTED;                              
      break;
    case GH_COMMAND_STOP:
      rail->SetPlayPause(false);
      cmd->executecode = GH_EXECUTE_SUCCESS;
      cmd->resultmessage = "stop";
      cmd->state = GH_QUEUE_STATE_EXECUTED;                              
      break;
    case GH_COMMAND_SHOW_STATUS:
      cmd->executecode = ghRailCommandShowStatus(cmd,rail,_wins);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_SHOW_VERSION:
      cmd->executecode = ghRailCommandShowVersion(cmd);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                              
      break;
    case GH_COMMAND_FIELD_SET:
      cmd->executecode = GH_EXECUTE_ALREADY_LOADED;
      cmd->state = GH_QUEUE_STATE_EXECUTED;
    case GH_COMMAND_FIELD_GET:
      cmd->executecode = ghRailCommandFieldGet(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;
      break;
    case GH_COMMAND_FIELD_GET_TRAIN:
      cmd->executecode = ghRailCommandFieldTrain(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;
      break;
    case GH_COMMAND_FIELD_GET_LINE:
      cmd->executecode = ghRailCommandFieldLine(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;
      break;
    case GH_COMMAND_FIELD_GET_TIMEZONE:
      cmd->executecode = ghRailCommandFieldTimezone(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;      
      break;
    case GH_COMMAND_FIELD_GET_DESC:
      cmd->executecode = ghRailCommandFieldDescription(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;      
      break;
    case GH_COMMAND_CLOCK_SET_TIME:
      cmd->executecode = ghRailCommandClockSetTime(cmd,rail,_sky);
      if ( cmd->executecode == GH_EXECUTE_SUCCESS ) {
	cmd->state = GH_QUEUE_STATE_PART_EXECUTED;
      } else {
	cmd->state = GH_QUEUE_STATE_EXECUTED;
      }
      break;
    case GH_COMMAND_CLOCK_GET_TIME:
      cmd->executecode = ghRailCommandClockGetTime(cmd,rail,_sky);
      cmd->state = GH_QUEUE_STATE_EXECUTED;      
      break;
    case GH_COMMAND_CLOCK_SET_SPEED:
      cmd->executecode = ghRailCommandClockSetSpeed(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;      
      break;
    case GH_COMMAND_CLOCK_GET_SPEED:
      cmd->executecode = ghRailCommandClockGetSpeed(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;      
      break;
    case GH_COMMAND_CAMERA_SET_POS:
      if ( _wins.count(cmd->argstr[0]) > 0 ) {
	cmd->executecode = ghRailCommandCameraSetPosition(cmd,_wins[cmd->argstr[0]]);
      } else {
	cmd->executecode = GH_EXECUTE_NOT_FOUND;
      }
      cmd->state = GH_QUEUE_STATE_EXECUTED;
      break;
    case GH_COMMAND_CAMERA_GET_POS:
      if ( _wins.count(cmd->argstr[0]) > 0 ) {
	cmd->executecode = ghRailCommandCameraGetPosition(cmd,_wins[cmd->argstr[0]]);
      } else {
	cmd->executecode = GH_EXECUTE_NOT_FOUND;
      }
      cmd->state = GH_QUEUE_STATE_EXECUTED;            
      break;
    case GH_COMMAND_CAMERA_SET_LOOK:
      if ( _wins.count(cmd->argstr[0]) > 0 ) {
	cmd->executecode = ghRailCommandCameraSetLookat(cmd,_wins[cmd->argstr[0]]);
      } else {
	cmd->executecode = GH_EXECUTE_NOT_FOUND;
      }
      cmd->state = GH_QUEUE_STATE_EXECUTED;            
      break;
    case GH_COMMAND_CAMERA_GET_LOOK:
      if ( _wins.count(cmd->argstr[0]) > 0 ) {
	cmd->executecode = ghRailCommandCameraGetLookat(cmd,_wins[cmd->argstr[0]]);
      } else {
	cmd->executecode = GH_EXECUTE_NOT_FOUND;
      }
      cmd->state = GH_QUEUE_STATE_EXECUTED;            
      break;
    case GH_COMMAND_CAMERA_SET_UP:
      if ( _wins.count(cmd->argstr[0]) > 0 ) {
	cmd->executecode = ghRailCommandCameraSetUpvec(cmd,_wins[cmd->argstr[0]]);
      } else {
	cmd->executecode = GH_EXECUTE_NOT_FOUND;
      }
      cmd->state = GH_QUEUE_STATE_EXECUTED;            
      break;
    case GH_COMMAND_CAMERA_GET_UP:
      if ( _wins.count(cmd->argstr[0]) > 0 ) {
	cmd->executecode = ghRailCommandCameraGetUpvec(cmd,_wins[cmd->argstr[0]]);
      } else {
	cmd->executecode = GH_EXECUTE_NOT_FOUND;
      }
      cmd->state = GH_QUEUE_STATE_EXECUTED;                  
      break;
    case GH_COMMAND_CAMERA_SET_TRACK:
      if ( _wins.count(cmd->argstr[0]) > 0 ) {
	cmd->executecode = ghRailCommandCameraSetTracking(cmd,rail,_wins[cmd->argstr[0]]);
	if ( cmd->executecode == GH_EXECUTE_SUCCESS ) {
	  _wins[cmd->argstr[0]].tracking = cmd->argstr[1];
	}
	//std::cout<<"set track "<<_wins[cmd->argstr[0]].tracking<<std::endl;
      } else {
	cmd->executecode = GH_EXECUTE_NOT_FOUND;
      }
      cmd->state = GH_QUEUE_STATE_EXECUTED;                  
      break;
    case GH_COMMAND_CAMERA_GET_TRACK:
      if ( _wins.count(cmd->argstr[0]) > 0 ) {
	cmd->executecode = ghRailCommandCameraGetTracking(cmd,_wins[cmd->argstr[0]]);
      } else {
	cmd->executecode = GH_EXECUTE_NOT_FOUND;
      }
      cmd->state = GH_QUEUE_STATE_EXECUTED;                  
      break;
    case GH_COMMAND_CAMERA_GET_VIEWPORT:
      if ( _wins.count(cmd->argstr[0]) > 0 ) {
	cmd->executecode = ghRailCommandCameraGetViewport(cmd,_wins[cmd->argstr[0]]);
      } else {
	cmd->executecode = GH_EXECUTE_NOT_FOUND;
      }
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_CAMERA_SET_WINDOW:
      if ( _wins.count(cmd->argstr[0]) > 0 ) {
	cmd->executecode = ghRailCommandCameraSetWindow(cmd,_wins[cmd->argstr[0]]);
      } else {
	cmd->executecode = GH_EXECUTE_NOT_FOUND;
      }
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_CAMERA_GET_WINDOW:
      if ( _wins.count(cmd->argstr[0]) > 0 ) {
	cmd->executecode = ghRailCommandCameraGetWindow(cmd,_wins[cmd->argstr[0]]);
      } else {
	cmd->executecode = GH_EXECUTE_NOT_FOUND;
      }
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_CAMERA_SET_SCREEN:
      if ( _wins.count(cmd->argstr[0]) > 0 ) {
	cmd->executecode = ghRailCommandCameraSetScreen(cmd,_wins[cmd->argstr[0]]);
      } else {
	cmd->executecode = GH_EXECUTE_NOT_FOUND;
      }
      cmd->state = GH_QUEUE_STATE_EXECUTED;                  
      break;
    case GH_COMMAND_CAMERA_GET_SCREEN:
      if ( _wins.count(cmd->argstr[0]) > 0 ) {
	cmd->executecode = ghRailCommandCameraGetScreen(cmd,_wins[cmd->argstr[0]]);
      } else {
	cmd->executecode = GH_EXECUTE_NOT_FOUND;
      }
      cmd->state = GH_QUEUE_STATE_EXECUTED;                  
      break;
    case GH_COMMAND_CAMERA_ADD:
      if ( _wins.count(cmd->argstr[0]) < 1 ) {
	cmd->executecode = ghRailCommandCameraAdd(cmd,rail,_wins.size());
      } else {
	cmd->executecode = GH_EXECUTE_ALREADY_EXIST;
      }
      if ( cmd->executecode == GH_EXECUTE_SUCCESS ) {
	cmd->state = GH_QUEUE_STATE_PART_EXECUTED;
      } else {
	cmd->state = GH_QUEUE_STATE_EXECUTED;
      }
      break;
    case GH_COMMAND_CAMERA_REMOVE:
      if ( _wins.count(cmd->argstr[0]) > 0 ) {
	cmd->executecode = ghRailCommandCameraRemove(cmd,_wins[cmd->argstr[0]]);
      } else {
	cmd->executecode = GH_EXECUTE_NOT_FOUND;
      }
      if ( cmd->executecode == GH_EXECUTE_SUCCESS ) {
	cmd->state = GH_QUEUE_STATE_PART_EXECUTED;
      } else {
	cmd->state = GH_QUEUE_STATE_EXECUTED;
      }
      break;
    case GH_COMMAND_CAMERA_GET:
      cmd->executecode = ghRailCommandCameraGet(cmd,_wins);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_TRAIN_LABEL_ON:
      cmd->executecode = ghRailCommandTrainLabel(cmd,rail,true);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_TRAIN_LABEL_OFF:
      cmd->executecode = ghRailCommandTrainLabel(cmd,rail,false);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_TRAIN_POSITION:
      cmd->executecode = ghRailCommandTrainPosition(cmd,rail,simtime);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                              
      break;
    case GH_COMMAND_TRAIN_TIMETABLE:
      cmd->executecode = ghRailCommandTrainTimetable(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_TRAIN_ICON:
      cmd->executecode = ghRailCommandTrainIcon(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_TRAIN_LINE:
      cmd->executecode = ghRailCommandTrainLine(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_TRAIN_DISTANCE:
      cmd->executecode = ghRailCommandTrainDistance(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_CONFIG_SET_MAXCLOCKSPEED:
      cmd->executecode = ghRailCommandConfigSetMaxspeed(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                              
      break;
    case GH_COMMAND_CONFIG_GET_MAXCLOCKSPEED:
      cmd->executecode = ghRailCommandConfigGetMaxspeed(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_CONFIG_SET_ALTMODE:
      cmd->executecode = ghRailCommandConfigSetAltmode(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_CONFIG_GET_ALTMODE:
      cmd->executecode = ghRailCommandConfigGetAltmode(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_CONFIG_SET_DISPLAYDISTANCE:
      cmd->executecode = ghRailCommandConfigSetDisplaydistance(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_CONFIG_GET_DISPLAYDISTANCE:
      cmd->executecode = ghRailCommandConfigGetDisplaydistance(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                              
      break;
    case GH_COMMAND_CONFIG_SET_MAXWINDOW:
      cmd->executecode = ghRailCommandConfigSetMaxwindow(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                              
      break;
    case GH_COMMAND_CONFIG_GET_MAXWINDOW:
      cmd->executecode = ghRailCommandConfigGetMaxwindow(cmd,rail);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_SHM_CLOCK_TIME:
#ifdef _WINDOWS
      cmd->executecode = GH_EXECUTE_UNKNOWN;
#else
      cmd->executecode = ghRailCommandShmSet(GH_SHM_TYPE_CLOCK_TIME,cmd,rail,_wins);
#endif
      cmd->state = GH_QUEUE_STATE_EXECUTED;                              
      break;
    case GH_COMMAND_SHM_TRAIN_POS:
#ifdef _WINDOWS
      cmd->executecode = GH_EXECUTE_UNKNOWN;
#else
      cmd->executecode = ghRailCommandShmSet(GH_SHM_TYPE_TRAIN_POSITION,cmd,rail,_wins);
#endif
      cmd->state = GH_QUEUE_STATE_EXECUTED;                              
      break;
    case GH_COMMAND_SHM_CAMERA_VIEW:
#ifdef _WINDOWS
      cmd->executecode = GH_EXECUTE_UNKNOWN;
#else
      cmd->executecode = ghRailCommandShmSet(GH_SHM_TYPE_CAMERA_VIEWPORT,cmd,rail,_wins);
#endif
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_SHM_REMOVE:
#ifdef _WINDOWS
      cmd->executecode = GH_EXECUTE_UNKNOWN;
#else
      cmd->executecode = ghRailCommandShmRemove(cmd,rail);
#endif
      cmd->state = GH_QUEUE_STATE_EXECUTED;                                    
      break;
    default:
      cmd->executecode = GH_EXECUTE_UNKNOWN;
      cmd->state = GH_QUEUE_STATE_EXECUTED;                                    
    }
    //cmd->result = ghRailCreateResultMessage(cmd,executecode,&resultmsg[0]);
  } else {
    //
    // simulation data NOT loaded  yet.
    //
    switch (cmd->type) {
    case GH_COMMAND_EXIT:
      cmd->executecode = GH_EXECUTE_SUCCESS;
      cmd->state = GH_QUEUE_STATE_EXECUTED;                              
      break;
    case GH_COMMAND_CLOSE:
      cmd->executecode = GH_EXECUTE_SUCCESS;
      cmd->state = GH_QUEUE_STATE_EXECUTED;                              
      break;
    case GH_COMMAND_SHOW_STATUS:
      cmd->executecode = ghRailCommandShowStatus(cmd,rail,_wins);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_SHOW_VERSION:
      cmd->executecode = ghRailCommandShowVersion(cmd);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                              
      break;
    case GH_COMMAND_FIELD_SET:
      cmd->executecode = ghRailCommandFieldSetData(cmd,rail);
      if ( cmd->executecode == GH_EXECUTE_SUCCESS ) {
	cmd->state = GH_QUEUE_STATE_PART_EXECUTED;
      } else {
	cmd->state = GH_QUEUE_STATE_EXECUTED;
      }
      break;
    default:
      cmd->executecode = GH_EXECUTE_NOT_LOADED;
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
    }
  }
  
}


void
ghRailCreateResultMessage(ghCommandQueue *cmd){

  std::string result = GH_STRING_COMMAND_PREFIX;
  
  //result += GH_COMMAND_MESSAGE[cmd->type];
  //int messagelength = std::strlen(message);

  //for (int i = 0; i < cmd->argstridx; i++) {
  //result += " ";
  //result += cmd->argstr[i];
  //}
  if ( cmd->executecode == GH_EXECUTE_SUCCESS ) {
    // NOP
    result += "Accept ";
    result += cmd->resultmessage;
  } else if ( cmd->executecode == GH_EXECUTE_UNKNOWN ) {
    result += "Error unknown command ";
    result += cmd->recv;
  } else if ( cmd->executecode == GH_EXECUTE_NOT_LOADED ) {
    result += "Error simulation data not loaded ";
    result += cmd->recv;
  } else if ( cmd->executecode == GH_EXECUTE_CANNOT_LOAD ) {
    result += "Error cannot load ";
    result += cmd->recv;
  } else if ( cmd->executecode == GH_EXECUTE_SIZE_ERROR ) {
    result += "Error exceed buffer size ";
    result += cmd->recv;
  } else if ( cmd->executecode == GH_EXECUTE_NOT_FOUND ) {
    result += "Error data not found ";
    result += cmd->recv;
  } else if ( cmd->executecode == GH_EXECUTE_ALREADY_EXIST ) {
    result += "Error data already exist ";
    result += cmd->recv;
  } else if ( cmd->executecode == GH_EXECUTE_RESERVED ) {
    result += "Error set reserved keywod ";
    result += cmd->recv;
  } else if ( cmd->executecode == GH_EXECUTE_CANNOT_GET ) {
    result += "Error cannot get data ";
    result += cmd->recv;
  } else if ( cmd->executecode == GH_EXECUTE_CANNOT_ALLOCATE ) {
    result += "Error cannot allocate data ";
    result += cmd->recv;
  } else if ( cmd->executecode == GH_EXECUTE_UNDER_TRACKING ) {
    result += "Warning camera under tracking ";
    result += cmd->recv;
  } else {
    result += "Wrong data ";
    result += cmd->recv;
  }

  result += "\n";
  
  cmd->resultmessage = result;
  //return result;
}


int
ghRailCommandFieldSetData(ghCommandQueue *cmd, ghRail *rail) {
  int res = -1;
  std::string ret = "field set ";
  res = rail->Setup(cmd->argstr[0]);
  if (  res == GH_SETUP_RESULT_OK ) {
    ret += cmd->argstr[0];
    ret += " loaded";
    cmd->resultmessage = ret;
    if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
      cmd->resultmessage = ret;
      return GH_EXECUTE_SUCCESS;
    } else {
      return GH_EXECUTE_SIZE_ERROR;
    }
  } else {
    std::cout << "Error field code " << res << std::endl;
    return GH_EXECUTE_CANNOT_LOAD;
  }
}

//int
//ghRailCommandFieldSet(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky) {
//  int res = -1;
//  res = rail->Setup(cmd->argstr[0]);
//  if (  res == GH_SETUP_RESULT_OK ) {
//    _sky->setDateTime( rail->GetBaseDatetime());
//    return GH_EXECUTE_SUCCESS;
//  } else {
//    std::cout << "Error field code " << res << std::endl;
//    return GH_EXECUTE_CANNOT_LOAD;
//  }
//}

int
ghRailCommandFieldGet(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "field get ";
  ret += rail->GetConfigure();
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandFieldTrain(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "field get train ";
  ret += rail->GetUnits();
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}
int
ghRailCommandFieldLine(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "field get line ";
  ret += rail->GetLines();
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}
int
ghRailCommandFieldTimezone(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "field get timezone ";
  ret += rail->GetTimezoneStr();
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}
int
ghRailCommandFieldDescription(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "field get description ";  
  ret += rail->GetDescription();
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandClockSetTime(ghCommandQueue *cmd, ghRail *rail, osgEarth::SkyNode *_sky) {

  osgEarth::DateTime dt = _sky->getDateTime();
  std::string ret = "clock set time ";
  
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

  ret += std::to_string((int)cmd->argnum[0]);
  ret += ":";
  ret += std::to_string((int)cmd->argnum[1]);
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
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

  std::string ret = "clock get time ";
  ret += std::to_string(hours);
  ret += ":";
  ret += std::to_string(mins);
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
  
}

int
ghRailCommandClockSetSpeed(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "clock set speed ";
  ret += std::to_string(cmd->argnum[0]);
  rail->SetClockSpeed(cmd->argnum[0]);
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandClockGetSpeed(ghCommandQueue *cmd, ghRail *rail) {
  double speed = rail->GetClockSpeed();
  std::string ret = "clock get speed ";
  ret += std::to_string(speed);
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandTrainLabel(ghCommandQueue *cmd, ghRail *rail,bool flag) {
  bool res ;
  std::string ret = "train label ";
  ret += cmd->argstr[0];
  if ( cmd->argstr[0] == GH_STRING_ALL ) {
    res = rail->SetTrainLabel("",flag);
  } else {
    res = rail->SetTrainLabel(cmd->argstr[0],flag);
  }
  if ( flag ) {
    ret += " on";
  } else {
    ret += " off";
  }
  if ( res ) {
    if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
      cmd->resultmessage = ret;
      return GH_EXECUTE_SUCCESS;
    } else {
      return GH_EXECUTE_SIZE_ERROR;
    }
  } else {
    return GH_EXECUTE_NOT_FOUND;
  }
}

int
ghRailCommandTrainPosition(ghCommandQueue *cmd, ghRail *rail, double simtime) {
  
  std::string ret = "train position ";
  if ( cmd->argstr[0] == GH_STRING_ALL ) {
    ret += "all ";
    ret += rail->GetTrainPosition("",simtime);
  } else {
    if ( rail->IsTrainID(cmd->argstr[0]) ) {
      ret += cmd->argstr[0];
      ret += " ";
      ret += rail->GetTrainPosition(cmd->argstr[0],simtime);
    } else {
      return GH_EXECUTE_NOT_FOUND;
    }
  }
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandTrainTimetable(ghCommandQueue *cmd, ghRail *rail) {

  std::string ret = "train timetable ";
  if ( rail->IsTrainID(cmd->argstr[0]) ) {
    ret += cmd->argstr[0];
    ret += " ";
    ret += rail->GetTrainTimetable(cmd->argstr[0]);
  } else {
    return GH_EXECUTE_NOT_FOUND;
  }
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandTrainIcon(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "train icon ";
  if ( rail->IsTrainID(cmd->argstr[0]) ) {
    ret += cmd->argstr[0];
    ret += " ";
    ret += rail->GetTrainIcon(cmd->argstr[0]);
  } else {
    return GH_EXECUTE_NOT_FOUND;
  }
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandTrainLine(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "train line ";
  if ( rail->IsTrainID(cmd->argstr[0]) ) {
    ret += cmd->argstr[0];
    ret += " ";
    ret += rail->GetTrainLine(cmd->argstr[0]);
  } else {
    return GH_EXECUTE_NOT_FOUND;
  }
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandTrainDistance(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "train distance ";
  if ( rail->IsTrainID(cmd->argstr[0]) ) {
    ret += cmd->argstr[0];
    ret += " ";
    ret += rail->GetTrainDistance(cmd->argstr[0]);
  } else {
    return GH_EXECUTE_NOT_FOUND;
  }
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}


int
ghRailCommandCameraSetPosition(ghCommandQueue *cmd,ghWindow _win) {
  osgEarth::Ellipsoid WGS84;
  std::string ret = "camera set ";
  //  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  //  if ( tmp == (ghWindow *)NULL ) {
  //    return GH_EXECUTE_NOT_FOUND;
  //}
  if ( _win.tracking == GH_STRING_NONE ) {
    // NOP process continue
    // camera set position , when tracking is none ONLY
  } else {
    return GH_EXECUTE_UNDER_TRACKING;
  }
  
  osg::Vec3d eye, up, center;
  osg::Camera *cam = _win.view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );
  osg::Vec3d eye2 = osg::Vec3d((double)cmd->argnum[0],(double)cmd->argnum[1], (double)cmd->argnum[2]);
  osg::Matrixd mat ;
  mat.makeLookAt( WGS84.geodeticToGeocentric(eye2), center, up );
  _win.view->getCameraManipulator()->setByInverseMatrix(mat);
  
  ret += cmd->argstr[0];
  ret += " position ";
  ret += std::to_string(cmd->argnum[0]);
  ret += " ";
  ret += std::to_string(cmd->argnum[1]);
  ret += " ";
  ret += std::to_string(cmd->argnum[2]);

  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandCameraGetPosition(ghCommandQueue *cmd,ghWindow _win) {
  osgEarth::Ellipsoid WGS84;
  std::string ret = "camera get ";
  //  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  //  if ( tmp == (ghWindow *)NULL ) {
  //    return GH_EXECUTE_NOT_FOUND;
  //  }
  osg::Vec3d eye, up, center;
  osg::Camera *cam = _win.view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );
  ret += cmd->argstr[0];
  ret += " position ";
  osg::Vec3 position = WGS84.geocentricToGeodetic(eye);
  ret += std::to_string(position.x());
  ret += " ";
  ret += std::to_string(position.y());
  ret += " ";
  ret += std::to_string(position.z());

  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandCameraSetLookat(ghCommandQueue *cmd, ghWindow _win) {

  osgEarth::Ellipsoid WGS84;
  std::string ret = "camera set ";
  //  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  //  if ( tmp == (ghWindow *)NULL ) {
  //    return GH_EXECUTE_NOT_FOUND;
  //  }
  osg::Vec3d eye, up, center;
  osg::Camera *cam = _win.view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );

  osg::Vec3d center2 = osg::Vec3d((double)cmd->argnum[0],(double)cmd->argnum[1], (double)cmd->argnum[2]);
  osg::Matrixd mat ;
  mat.makeLookAt( eye, WGS84.geodeticToGeocentric(center2), up );
  _win.view->getCameraManipulator()->setByInverseMatrix(mat);

  ret += cmd->argstr[0];
  ret += " lookat ";
  ret += std::to_string(cmd->argnum[0]);
  ret += " ";
  ret += std::to_string(cmd->argnum[1]);
  ret += " ";
  ret += std::to_string(cmd->argnum[2]);

  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}
int
ghRailCommandCameraGetLookat(ghCommandQueue *cmd, ghWindow _win) {

  osgEarth::Ellipsoid WGS84;
  std::string ret = "camera get ";
  //  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  //  if ( tmp == (ghWindow *)NULL ) {
  //    return GH_EXECUTE_NOT_FOUND;
  //  }
  osg::Vec3d eye, up, center;
  osg::Camera *cam = _win.view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );
  ret += cmd->argstr[0];
  ret += " lookat ";

  osg::Vec3 center2 = WGS84.geocentricToGeodetic(center);
  ret += std::to_string(center2.x());
  ret += " ";
  ret += std::to_string(center2.y());
  ret += " ";
  ret += std::to_string(center2.z());
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandCameraSetUpvec(ghCommandQueue *cmd, ghWindow _win) {

  osgEarth::Ellipsoid WGS84;
  std::string ret = "camera set ";
  //  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  //  if ( tmp == (ghWindow *)NULL ) {
  //    return GH_EXECUTE_NOT_FOUND;
  //  }
  osg::Vec3d eye, up, center;
  osg::Camera *cam = _win.view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );

  osg::Vec3d up2 = osg::Vec3d((double)cmd->argnum[0],(double)cmd->argnum[1], (double)cmd->argnum[2]);
  osg::Matrixd mat ;
  mat.makeLookAt( eye, center, WGS84.geodeticToGeocentric(up2) );
  _win.view->getCameraManipulator()->setByInverseMatrix(mat);

  ret += cmd->argstr[0];
  ret += " upvec ";
  ret += std::to_string(cmd->argnum[0]);
  ret += " ";
  ret += std::to_string(cmd->argnum[1]);
  ret += " ";
  ret += std::to_string(cmd->argnum[2]);

  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}
int
ghRailCommandCameraGetUpvec(ghCommandQueue *cmd, ghWindow _win) {
  osgEarth::Ellipsoid WGS84;
  std::string ret = "camera get ";
  //  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  //  if ( tmp == (ghWindow *)NULL ) {
  //    return GH_EXECUTE_NOT_FOUND;
  //  }
  osg::Vec3d eye, up, center;
  osg::Camera *cam = _win.view->getCamera();
  cam->getViewMatrixAsLookAt( eye, center, up );
  ret += cmd->argstr[0];
  ret += " upvec ";

  osg::Vec3 up2 = WGS84.geocentricToGeodetic(up);
  ret += std::to_string(up2.x());
  ret += " ";
  ret += std::to_string(up2.y());
  ret += " ";
  ret += std::to_string(up2.z());
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandCameraSetTracking(ghCommandQueue *cmd,ghRail *rail,ghWindow _win) {
  std::string ret = "camera set ";
  //  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  //char *trackname = NULL;
  //  if ( tmp == NULL ) {
  //      return GH_EXECUTE_NOT_FOUND;
  //  }
  if ( cmd->argstr[1] == GH_STRING_NONE ) {
    ret += cmd->argstr[0];
    ret += " tracking ";
    ret += GH_STRING_NONE; 
    //if ( tmp->tracking != (char *)NULL ) {
    //  free(tmp->tracking);
    //}
    //trackname = ghString2CharPtr( GH_STRING_NONE );
    _win.tracking = GH_STRING_NONE;
  } else if ( rail->IsTrainID(cmd->argstr[1]) ) {
    ret += cmd->argstr[0];
    ret += " tracking ";
    ret += cmd->argstr[1];
    //if ( tmp->tracking != (char *)NULL ) {
    //  free(tmp->tracking);
    //}
    //trackname = ghString2CharPtr( cmd->argstr[1] );
    _win.tracking = cmd->argstr[1];
  } else {
    return GH_EXECUTE_NOT_FOUND;
  }
  //if ( trackname != (char *)NULL) {
  //    free(trackname);
  //  }
  
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
  
}

int
ghRailCommandCameraGetTracking(ghCommandQueue *cmd,ghWindow _win) {

  std::string ret = "camera get ";
  //  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  //  if ( tmp == (ghWindow *)NULL ) {
  //      return GH_EXECUTE_NOT_FOUND;
  //  }

  ret += cmd->argstr[0];
  ret += " tracking ";
  ret += _win.tracking;
  
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandCameraGetViewport(ghCommandQueue *cmd, ghWindow _win) {

  std::string ret = "camera get ";
  //  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  //  if ( tmp == (ghWindow *)NULL ) {
  //      return GH_EXECUTE_NOT_FOUND;
  //  }
  std::vector<osg::Vec3d> points = _calcCameraViewpoints(_win.view);
  ret += cmd->argstr[0];
  ret += " viewport ";
  // points max 12
  for (int i = 0; i < points.size(); i++) {
    ret += ",";
    ret += std::to_string(points[i].x());
    ret += " ";
    ret += std::to_string(points[i].y());
    //ret += " ";
    //ret += std::to_string(points[i].z());
  }
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
    
}

int
ghRailCommandCameraSetScreen(ghCommandQueue *cmd,ghWindow _win) {

  std::string ret = "camera set ";
  //  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  //  if ( tmp == (ghWindow *)NULL ) {
  //      return GH_EXECUTE_NOT_FOUND;
  //  }
  ret += cmd->argstr[0];
  ret += " screen ";
  ret += std::to_string(cmd->argnum[0]);
  ret += " ";
  ret += std::to_string(cmd->argnum[1]);

  ghSetConfigWindow(_win,cmd->argstr[0],cmd->argnum[0],cmd->argnum[1],-1,-1);

  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }

 
}

int
ghRailCommandCameraGetScreen(ghCommandQueue *cmd,ghWindow _win) {
  int current[4];
  std::string ret = "camera get ";
  //  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  //  if ( tmp == (ghWindow *)NULL ) {
  //      return GH_EXECUTE_NOT_FOUND;
  //  }
  ret += cmd->argstr[0];
  ret += " screen ";
  ghGetConfigWindow(_win,cmd->argstr[0],&current[0]);
  ret += std::to_string(current[0]);
  ret += " ";
  ret += std::to_string(current[1]);
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }

}

int
ghRailCommandCameraSetWindow(ghCommandQueue *cmd,ghWindow _win) {

  std::string ret = "camera set ";
  //  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  //  if ( tmp == (ghWindow *)NULL ) {
  //      return GH_EXECUTE_NOT_FOUND;
  //  }
  ret += cmd->argstr[0];
  ret += " window ";
  ret += std::to_string(cmd->argnum[0]);
  ret += " ";
  ret += std::to_string(cmd->argnum[1]);

  ghSetConfigWindow(_win,cmd->argstr[0],-1,-1,cmd->argnum[0],cmd->argnum[1]);

  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }

  
}

int
ghRailCommandCameraGetWindow(ghCommandQueue *cmd,ghWindow _win) {
  int current[4];
  std::string ret = "camera get ";
  //  ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
  //  if ( tmp == (ghWindow *)NULL ) {
  //      return GH_EXECUTE_NOT_FOUND;
  //  }
  ret += cmd->argstr[0];
  ret += " window ";
  ghGetConfigWindow(_win,cmd->argstr[0],&current[0]);
  ret += std::to_string(current[2]);
  ret += " ";
  ret += std::to_string(current[3]);
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }

}


int
ghRailCommandCameraAdd(ghCommandQueue *cmd, ghRail *rail, int cameracount) {

  std::string ret = "camera add ";
  std::string cname = cmd->argstr[0];
  ret += cname;
  ret += " ";
  //  ghWindow *tmp = _win;
  for (int i = 0; i < GH_RESERVED_STRING.size(); i++) {
    if ( GH_RESERVED_STRING[i] == cname ) {
      return GH_EXECUTE_RESERVED;
    }
  }
  //  while (tmp != (ghWindow *)NULL) {
  //    if ( tmp->name == cname ) {
  //      return GH_EXECUTE_ALREADY_EXIST;
  //    }
  //    tmp = tmp->next ;
  //  }
  int maxwin = rail->GetMaxWindow();
  if ( cameracount > maxwin ) {
    return GH_EXECUTE_SIZE_ERROR;
  }

  ret += std::to_string(cmd->argnum[0]);
  ret += " ";
  ret += std::to_string(cmd->argnum[1]);
  ret += " ";
  ret += std::to_string(cmd->argnum[2]);
  
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
  
}

int
ghRailCommandCameraRemove(ghCommandQueue *cmd, ghWindow _win) {
  std::string ret = "camera remove ";
  std::string cname = cmd->argstr[0];
  //  ghWindow *tmp = _win;
  for (int i = 0; i < GH_RESERVED_STRING.size(); i++) {
    if ( GH_RESERVED_STRING[i] == cname ) {
      return GH_EXECUTE_RESERVED;
    }
  }
  //  while (tmp != (ghWindow *)NULL) {
  //    if ( tmp->name == cname ) {
  //      ret += cname;
  //      if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
  //	cmd->resultmessage = ret;
  //	return GH_EXECUTE_SUCCESS;
  //      } else {
  //	return GH_EXECUTE_SIZE_ERROR;
  //      }
  //}
  //tmp = tmp->next ;
  //}
  //return GH_EXECUTE_NOT_FOUND;

  ret += cname;
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandCameraGet(ghCommandQueue *cmd,  std::map<std::string, ghWindow>& _wins) {
  //ghWindow *tmp = _win;
  std::string ret = "camera get ";
    
  //  while (tmp != (ghWindow *)NULL) {
  //    ret += tmp->name;
  //    ret += " ";
  //    tmp = tmp->next ;
  //  }
  for (const auto& [key, value] : _wins) {
    //std::cout << key << ": " << value << std::endl;
    ret += key;
    ret += " ";
  }
  
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }

}

int
ghRailCommandConfigSetMaxspeed(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "config set maxclockspeed ";

  rail->SetClockMaxSpeed(cmd->argnum[0]);

  ret += std::to_string(cmd->argnum[0]);
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}
int
ghRailCommandConfigGetMaxspeed(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "config get maxclockspeed ";
  
  double speed = rail->GetClockMaxSpeed();
  ret += std::to_string(speed);
  
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandConfigSetAltmode(ghCommandQueue *cmd, ghRail *rail) {
  int mode = (int)cmd->argnum[0];
  std::string ret = "config set altmode ";
  rail->SetAltmode(mode);
  ret += std::to_string(cmd->argnum[0]);
  cmd->resultmessage = ret;

  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandConfigGetAltmode(ghCommandQueue *cmd, ghRail *rail) {

  std::string ret = "config get altmode ";
  ret += GH_COMMAND_ALTMODE_STRING[rail->GetAltmode()];
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandConfigSetDisplaydistance(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "config set displaydistance ";
  rail->SetDisplayDistance(cmd->argnum[0]);
  ret += std::to_string(cmd->argnum[0]);
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }

}
int
ghRailCommandConfigGetDisplaydistance(ghCommandQueue *cmd, ghRail *rail) {

  std::string ret = "config get displaydistance ";
  double dis = rail->GetDisplayDistance();
  ret += std::to_string(dis);
  
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandConfigSetMaxwindow(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "config set maxwindow ";
  rail->SetMaxWindow((int)cmd->argnum[0]);
  ret += std::to_string(cmd->argnum[0]);
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }

}
int
ghRailCommandConfigGetMaxwindow(ghCommandQueue *cmd, ghRail *rail) {
  std::string ret = "config get maxwindow ";
  int maxwin = rail->GetMaxWindow();
  ret += std::to_string(maxwin);
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}


int
ghRailCommandShmSet(int shmtype,ghCommandQueue *cmd,ghRail *rail, std::map<std::string, ghWindow>& _wins) {

  //  int offset = 0;
  //  if ( _win != (ghWindow *)NULL ) {
  //    offset = 24 + ghCountWindow(_win);
  //  }

  int offset = 24 + _wins.size();
  
#ifdef _WINDOWS
  int shmkey = 86822723;
#else  
  int shmkey = ftok(GH_SHM_PATH,shmtype+offset);
#endif
  std::string ret = "shm set ";
  if (shmkey < 0) {
    return GH_EXECUTE_CANNOT_GET;
  } else {
    std::string skey = std::to_string(shmkey);
    int ssize = -1;
    if ( shmtype == GH_SHM_TYPE_CLOCK_TIME ) {
      ssize = rail->InitShmClock(shmkey);
      ret += "clock time ";
    } else if ( shmtype == GH_SHM_TYPE_TRAIN_POSITION ) {
      ssize = rail->InitShmTrain(shmkey);
      ret += "train potision ";
    } else if ( shmtype == GH_SHM_TYPE_CAMERA_VIEWPORT ) {
      ssize = ghInitShmWindow(shmkey,_wins[cmd->argstr[0]]);
      ret += "camera ";
      ret += cmd->argstr[0];
      ret += " viewport ";
    } else {
      return GH_EXECUTE_NOT_FOUND;
    }
    if ( ssize < 0 ) {
      return GH_EXECUTE_CANNOT_ALLOCATE;
    } else {
      ret += skey + " ";
      ret += std::to_string(ssize);
    }
  }
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    //strcpy(result, ret.c_str());
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}

int
ghRailCommandShmRemove(ghCommandQueue *cmd,ghRail *rail) {
  std::string ret = "shm remove ";
  int shmkey = (int)cmd->argnum[0];
  ret += std::to_string(cmd->argnum[0]);

  int ssize = rail->RemoveShm(shmkey);

  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }

}

int
ghRailCommandShowStatus(ghCommandQueue *cmd, ghRail *rail, std::map<std::string, ghWindow>& _wins) {

  //ghWindow *wtmp = _win;
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
    
    //    while (wtmp != (ghWindow *)NULL) {
    //      ret += " camera ";
    //      ret += wtmp->name;
    //      ret += " tracking ";
    //      ret += wtmp->tracking;
    //      wtmp = wtmp->next ;
    //    }

    
  } else {
    ret += "simulation data not loaded";
  }

  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
    
}

int
ghRailCommandShowVersion(ghCommandQueue *cmd) {
  std::string ret = "show version ";
  ret += GH_APP_NAME;
  ret += " ";
  ret += GH_APP_REVISION;
  if ( ret.size() < GH_SOCKET_BUFFER_SIZE ) {
    cmd->resultmessage = ret;
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }
}



