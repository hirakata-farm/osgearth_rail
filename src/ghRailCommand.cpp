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

# include "ghString.hpp"
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
  "field line ",
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
  "camera set window ",
  "camera get window ",
  "camera set screen ",
  "camera get screen ",
  "camera add ",
  "camera remove ",
  "camera get ",
  "train label on ",
  "train label off ",  
  "train position ",
  "train timetable ",
  "train icon ",
  "train line ",
  "train distance ",
  "config set max clock speed ",
  "config get max clock speed ",  
  "config set altmode ",
  "config get altmode ",      
  "config set displaydistance ",
  "config get displaydistance ",
  "config set max window ",
  "config get max window ",  
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

ghCommandQueue *
ghRailInitCommandQueue() {

  ghCommandQueue *cmd;
  if ((cmd = (ghCommandQueue *)calloc( (unsigned)1, (unsigned)sizeof( ghCommandQueue ) )) == NULL)
    {
      return( (ghCommandQueue *)NULL ) ;
    }
  
  cmd->type = GH_COMMAND_UNKNOWN;
  cmd->argstridx = 0;
  cmd->argstr[0] = GH_STRING_NOP;
  cmd->argstr[1] = GH_STRING_NOP;  
  cmd->argnumidx = 0;
  cmd->argnum[0]  = 0.0;
  cmd->argnum[1]  = 0.0;
  cmd->argnum[2]  = 0.0;
  cmd->state = GH_QUEUE_STATE_INIT;
  cmd->result = GH_STRING_NOP;
  cmd->prev = (ghCommandQueue *)NULL;
  return cmd;
}

void
ghRailParseCommand(ghCommandQueue *cmd,string str) {

  std::vector<std::string> command = ghStringSplit(str, ' ');
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

  //return cmd;

}

void
ghRailExecuteCommandOSG( ghCommandQueue *cmd, ghRail *rail , ghWindow* _win, osgEarth::SkyNode* _sky) {

  if ( cmd == NULL ) return;
  if ( rail == NULL ) return;
  if ( cmd->state == GH_QUEUE_STATE_PARSED ) {
    // NOP continue
  } else {
    return;
  }
  char resultmsg[GH_EXECUTE_BUFFER_SIZE];
  int  executecode = GH_EXECUTE_INIT;
  memset(&resultmsg[0], 0, sizeof(char)*GH_EXECUTE_BUFFER_SIZE);

  if ( rail->IsLoaded() ) {
    // simulation data loaded
    switch (cmd->type) {
    case GH_COMMAND_FIELD_SET:
      cmd->state = GH_QUEUE_STATE_PART_EXECUTED;
    case GH_COMMAND_CLOCK_SET_TIME:
      executecode = ghRailCommandClockSetTime(cmd,rail,_sky);
      cmd->state = GH_QUEUE_STATE_PART_EXECUTED;
      break;
    case GH_COMMAND_CLOCK_GET_TIME:
      executecode = ghRailCommandClockGetTime(cmd,rail,_sky,&resultmsg[0]);
      cmd->state = GH_QUEUE_STATE_EXECUTED;      
      break;
    case GH_COMMAND_CAMERA_SET_POS:
      executecode = ghRailCommandCameraSetPosition(cmd,_win);
      cmd->state = GH_QUEUE_STATE_EXECUTED;      
      break;
    case GH_COMMAND_CAMERA_GET_POS:
      executecode = ghRailCommandCameraGetPosition(cmd,_win,&resultmsg[0]);
      cmd->state = GH_QUEUE_STATE_EXECUTED;            
      break;
    case GH_COMMAND_CAMERA_SET_LOOK:
      executecode = ghRailCommandCameraSetLookat(cmd,_win);
      cmd->state = GH_QUEUE_STATE_EXECUTED;            
      break;
    case GH_COMMAND_CAMERA_GET_LOOK:
      executecode = ghRailCommandCameraGetLookat(cmd,_win,&resultmsg[0]);
      cmd->state = GH_QUEUE_STATE_EXECUTED;            
      break;
    case GH_COMMAND_CAMERA_SET_UP:
      executecode = ghRailCommandCameraSetUpvec(cmd,_win);
      cmd->state = GH_QUEUE_STATE_EXECUTED;            
      break;
    case GH_COMMAND_CAMERA_GET_UP:
      executecode = ghRailCommandCameraGetUpvec(cmd,_win,&resultmsg[0]);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                  
      break;
    case GH_COMMAND_CAMERA_SET_TRACK:
      executecode = ghRailCommandCameraSetTracking(cmd,rail,_win);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                  
      break;
    case GH_COMMAND_CAMERA_GET_TRACK:
      executecode = ghRailCommandCameraGetTracking(cmd,_win,&resultmsg[0]);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                  
      break;
    case GH_COMMAND_CAMERA_GET_VIEWPORT:
      executecode = ghRailCommandCameraGetViewport(cmd,_win,&resultmsg[0]);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_CAMERA_SET_SCREEN:
      executecode = ghRailCommandCameraSetScreen(cmd,_win);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                  
      break;
    case GH_COMMAND_CAMERA_GET_SCREEN:
      executecode = ghRailCommandCameraGetScreen(cmd,_win,&resultmsg[0]);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                  
      break;
    case GH_COMMAND_CAMERA_SET_WINDOW:
      executecode = ghRailCommandCameraSetWindow(cmd,_win);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_CAMERA_GET_WINDOW:
      executecode = ghRailCommandCameraGetWindow(cmd,_win,&resultmsg[0]);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_CAMERA_ADD:
      executecode = ghRailCommandCameraAdd(cmd,rail,_win);
      cmd->state = GH_QUEUE_STATE_PART_EXECUTED;
      break;
    case GH_COMMAND_CAMERA_REMOVE:
      executecode = ghRailCommandCameraRemove(cmd,_win);
      cmd->state = GH_QUEUE_STATE_PART_EXECUTED;
      break;
    case GH_COMMAND_CAMERA_GET:
      executecode = ghRailCommandCameraGet(cmd,_win,&resultmsg[0]);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_SHM_CAMERA_VIEW:
      executecode = ghRailCommandShmSet(GH_SHM_TYPE_CAMERA_VIEWPORT,cmd,rail,_win,&resultmsg[0]);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    case GH_COMMAND_SHOW_STATUS:
      executecode = ghRailCommandShowStatus(cmd,rail,_win,&resultmsg[0]);
      cmd->state = GH_QUEUE_STATE_EXECUTED;                        
      break;
    default:
      executecode = GH_EXECUTE_UNKNOWN;
    }
    cmd->result = ghRailReturnMessage(cmd,executecode,&resultmsg[0]);
  }
  
}

void
ghRailExecuteCommandData( ghCommandQueue *cmd, ghRail *rail, double simtime ) {

  if ( cmd == NULL ) return;
  if ( rail == NULL ) return;
  if ( cmd->state == GH_QUEUE_STATE_PARSED ) {
    // NOP continue
  } else {
    return;
  }

  char resultmsg[GH_EXECUTE_BUFFER_SIZE];
  int  executecode = GH_EXECUTE_INIT;
  memset(&resultmsg[0], 0, sizeof(char)*GH_EXECUTE_BUFFER_SIZE);

  if ( rail->IsLoaded() ) {
    // simulation data loaded
    switch (cmd->type) {
    case GH_COMMAND_EXIT:
      executecode = GH_EXECUTE_SUCCESS;
      break;
    case GH_COMMAND_CLOSE:
      executecode = GH_EXECUTE_SUCCESS;
      break;
    case GH_COMMAND_START:
      rail->SetPlayPause(true);
      executecode = GH_EXECUTE_SUCCESS;
      break;
    case GH_COMMAND_STOP:
      rail->SetPlayPause(false);
      executecode = GH_EXECUTE_SUCCESS;
      break;
    case GH_COMMAND_FIELD_GET:
      executecode = ghRailCommandFieldGet(cmd,rail,&resultmsg[0]);
      break;
    case GH_COMMAND_FIELD_GET_TRAIN:
      executecode = ghRailCommandFieldTrain(cmd,rail,&resultmsg[0]);
      break;
    case GH_COMMAND_FIELD_GET_LINE:
      executecode = ghRailCommandFieldLine(cmd,rail,&resultmsg[0]);
      break;
    case GH_COMMAND_FIELD_GET_TIMEZONE:
      executecode = ghRailCommandFieldTimezone(cmd,rail,&resultmsg[0]);
      break;
    case GH_COMMAND_FIELD_GET_DESC:
      executecode = ghRailCommandFieldDescription(cmd,rail,&resultmsg[0]);
      break;
    case GH_COMMAND_CLOCK_SET_TIME:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CLOCK_GET_TIME:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CLOCK_SET_SPEED:
      executecode = ghRailCommandClockSetSpeed(cmd,rail);
      break;
    case GH_COMMAND_CLOCK_GET_SPEED:
      executecode = ghRailCommandClockGetSpeed(cmd,rail,&resultmsg[0]);
      break;
    case GH_COMMAND_TRAIN_LABEL_ON:
      executecode = ghRailCommandTrainLabel(cmd,rail,true);
      break;
    case GH_COMMAND_TRAIN_LABEL_OFF:
      executecode = ghRailCommandTrainLabel(cmd,rail,false);
      break;
    case GH_COMMAND_TRAIN_POSITION:
      executecode = ghRailCommandTrainPosition(cmd,rail,simtime,&resultmsg[0]);
      break;
    case GH_COMMAND_TRAIN_TIMETABLE:
      executecode = ghRailCommandTrainTimetable(cmd,rail,&resultmsg[0]);
      break;
    case GH_COMMAND_TRAIN_ICON:
      executecode = ghRailCommandTrainIcon(cmd,rail,&resultmsg[0]);
      break;
    case GH_COMMAND_TRAIN_LINE:
      executecode = ghRailCommandTrainLine(cmd,rail,&resultmsg[0]);
      break;
    case GH_COMMAND_TRAIN_DISTANCE:
      executecode = ghRailCommandTrainDistance(cmd,rail,&resultmsg[0]);
      break;
    case GH_COMMAND_CAMERA_SET_POS:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CAMERA_GET_POS:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CAMERA_SET_LOOK:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CAMERA_GET_LOOK:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CAMERA_SET_UP:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CAMERA_GET_UP:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CAMERA_SET_TRACK:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CAMERA_GET_TRACK:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CAMERA_GET_VIEWPORT:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CAMERA_SET_SCREEN:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CAMERA_GET_SCREEN:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CAMERA_SET_WINDOW:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CAMERA_GET_WINDOW:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CAMERA_ADD:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CAMERA_REMOVE:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CAMERA_GET:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_CONFIG_SET_MAXCLOCKSPEED:
      executecode = ghRailCommandConfigSetMaxspeed(cmd,rail);
      break;
    case GH_COMMAND_CONFIG_GET_MAXCLOCKSPEED:
      executecode = ghRailCommandConfigGetMaxspeed(cmd,rail,&resultmsg[0]);
      break;
    case GH_COMMAND_CONFIG_SET_ALTMODE:
      executecode = ghRailCommandConfigSetAltmode(cmd,rail);
      break;
    case GH_COMMAND_CONFIG_GET_ALTMODE:
      executecode = ghRailCommandConfigGetAltmode(cmd,rail,&resultmsg[0]);
      break;
    case GH_COMMAND_CONFIG_SET_DISPLAYDISTANCE:
      executecode = ghRailCommandConfigSetDisplaydistance(cmd,rail);
      break;
    case GH_COMMAND_CONFIG_GET_DISPLAYDISTANCE:
      executecode = ghRailCommandConfigGetDisplaydistance(cmd,rail,&resultmsg[0]);
      break;
    case GH_COMMAND_CONFIG_SET_MAXWINDOW:
      executecode = ghRailCommandConfigSetMaxwindow(cmd,rail);
      break;
    case GH_COMMAND_CONFIG_GET_MAXWINDOW:
      executecode = ghRailCommandConfigGetMaxwindow(cmd,rail,&resultmsg[0]);
      break;
    case GH_COMMAND_SHM_CLOCK_TIME:
#ifdef _WINDOWS
      executecode = GH_EXECUTE_UNKNOWN;
#else
      executecode = ghRailCommandShmSet(GH_SHM_TYPE_CLOCK_TIME,cmd,rail,NULL,&resultmsg[0]);
#endif      
      break;
    case GH_COMMAND_SHM_TRAIN_POS:
#ifdef _WINDOWS
      executecode = GH_EXECUTE_UNKNOWN;
#else
      executecode = ghRailCommandShmSet(GH_SHM_TYPE_TRAIN_POSITION,cmd,rail,NULL,&resultmsg[0]);
#endif      
      break;
    case GH_COMMAND_SHM_CAMERA_VIEW:
#ifdef _WINDOWS
      executecode = GH_EXECUTE_UNKNOWN;
#else
      executecode = GH_EXECUTE_DELAY;
      //executecode = ghRailCommandShmSet(GH_SHM_TYPE_CAMERA_VIEWPORT,cmd,rail,_win,&resultmsg[0]);
#endif      
      break;
    case GH_COMMAND_SHM_REMOVE:
#ifdef _WINDOWS
      executecode = GH_EXECUTE_UNKNOWN;
#else
      executecode = ghRailCommandShmRemove(cmd,rail);
#endif      
      break;
    case GH_COMMAND_SHOW_STATUS:
      executecode = GH_EXECUTE_DELAY;
      break;
    case GH_COMMAND_SHOW_VERSION:
      executecode = ghRailCommandShowVersion(&resultmsg[0]);
      break;
    default:
      executecode = GH_EXECUTE_UNKNOWN;
    }
  } else {
    // Not loaded
    switch (cmd->type) {
    case GH_COMMAND_EXIT:
      executecode = GH_EXECUTE_SUCCESS;      
      break;
    case GH_COMMAND_CLOSE:
      executecode = GH_EXECUTE_SUCCESS;      
      break;
    case GH_COMMAND_FIELD_SET:
      executecode = ghRailCommandFieldSetData(cmd,rail);
      break;
    case GH_COMMAND_SHOW_VERSION:
      executecode = ghRailCommandShowVersion(&resultmsg[0]);
      break;
    default:
      executecode = GH_EXECUTE_NOT_LOADED;
    }
  }

  if ( executecode == GH_EXECUTE_DELAY ) {
    cmd->state = GH_QUEUE_STATE_PARSED;
  } else {
    cmd->state = GH_QUEUE_STATE_EXECUTED;
    cmd->result = ghRailReturnMessage(cmd,executecode,&resultmsg[0]);
  }
  
}


int
ghRailExecuteCommandObsolute( ghCommandQueue *cmd,
		      ghRail *rail,
		      ghWindow* _win,
		      osgEarth::SkyNode* _sky,
		      double simtime ) {

  int retval = GH_POST_EXECUTE_NONE;
  if ( cmd == NULL ) return retval;
  //if ( cmd->isexecute ) return retval; // Already execute

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
    case GH_COMMAND_FIELD_GET_LINE:
      resultcode = ghRailCommandFieldLine(cmd,rail,&resultstring[0]);
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
    case GH_COMMAND_TRAIN_LINE:
      resultcode = ghRailCommandTrainLine(cmd,rail,&resultstring[0]);
      break;
    case GH_COMMAND_TRAIN_DISTANCE:
      resultcode = ghRailCommandTrainDistance(cmd,rail,&resultstring[0]);
      break;
    case GH_COMMAND_CAMERA_SET_POS:
      resultcode = ghRailCommandCameraSetPosition(cmd,_win);
      break;
    case GH_COMMAND_CAMERA_GET_POS:
      resultcode = ghRailCommandCameraGetPosition(cmd,_win,&resultstring[0]);
      break;
    case GH_COMMAND_CAMERA_SET_LOOK:
      resultcode = ghRailCommandCameraSetLookat(cmd,_win);
      break;
    case GH_COMMAND_CAMERA_GET_LOOK:
      resultcode = ghRailCommandCameraGetLookat(cmd,_win,&resultstring[0]);
      break;
    case GH_COMMAND_CAMERA_SET_UP:
      resultcode = ghRailCommandCameraSetUpvec(cmd,_win);
      break;
    case GH_COMMAND_CAMERA_GET_UP:
      resultcode = ghRailCommandCameraGetUpvec(cmd,_win,&resultstring[0]);
      break;
    case GH_COMMAND_CAMERA_SET_TRACK:
      resultcode = ghRailCommandCameraSetTracking(cmd,rail,_win);
      break;
    case GH_COMMAND_CAMERA_GET_TRACK:
      resultcode = ghRailCommandCameraGetTracking(cmd,_win,&resultstring[0]);
      break;
    case GH_COMMAND_CAMERA_GET_VIEWPORT:
      resultcode = ghRailCommandCameraGetViewport(cmd,_win,&resultstring[0]);
      break;
    case GH_COMMAND_CAMERA_SET_SCREEN:
      resultcode = ghRailCommandCameraSetScreen(cmd,_win);
      break;
    case GH_COMMAND_CAMERA_GET_SCREEN:
      resultcode = ghRailCommandCameraGetScreen(cmd,_win,&resultstring[0]);
      break;
    case GH_COMMAND_CAMERA_SET_WINDOW:
      resultcode = ghRailCommandCameraSetWindow(cmd,_win);
      break;
    case GH_COMMAND_CAMERA_GET_WINDOW:
      resultcode = ghRailCommandCameraGetWindow(cmd,_win,&resultstring[0]);
      break;
    case GH_COMMAND_CAMERA_ADD:
      resultcode = ghRailCommandCameraAdd(cmd,rail,_win);
      if ( resultcode == GH_EXECUTE_SUCCESS ) retval = GH_POST_EXECUTE_CAMERA_ADD;
      break;
    case GH_COMMAND_CAMERA_REMOVE:
      resultcode = ghRailCommandCameraRemove(cmd,_win);
      if ( resultcode == GH_EXECUTE_SUCCESS ) retval = GH_POST_EXECUTE_CAMERA_REMOVE;
      break;
    case GH_COMMAND_CAMERA_GET:
      resultcode = ghRailCommandCameraGet(cmd,_win,&resultstring[0]);
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
    case GH_COMMAND_CONFIG_SET_MAXWINDOW:
      resultcode = ghRailCommandConfigSetMaxwindow(cmd,rail);
      break;
    case GH_COMMAND_CONFIG_GET_MAXWINDOW:
      resultcode = ghRailCommandConfigGetMaxwindow(cmd,rail,&resultstring[0]);
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

  cmd->result = ghRailReturnMessage(cmd,resultcode,&resultstring[0]);
  //std::string result = ghRailReturnMessage(cmd,resultcode,&resultstring[0]);
  //const char* retmsg = result.c_str();
  //send(socket, retmsg, std::strlen(retmsg), 0);
  //cmd->isexecute = true;    

  return retval;
}

std::string
ghRailReturnMessage(ghCommandQueue *cmd,int code, char *message){

  std::string result = "Geoglyph:";
  result += GH_COMMAND_MESSAGE[cmd->type];
  int messagelength = std::strlen(message);

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
ghRailCommandFieldSetData(ghCommandQueue *cmd, ghRail *rail) {
  int res = -1;
  res = rail->Setup(cmd->argstr[0]);
  if (  res == GH_SETUP_RESULT_OK ) {
    return GH_EXECUTE_DELAY;
  } else {
    std::cout << "Error field code " << res << std::endl;
    return GH_EXECUTE_CANNOT_LOAD;
  }
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
ghRailCommandFieldLine(ghCommandQueue *cmd, ghRail *rail, char *result) {
  std::string ret(rail->GetLines());
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
ghRailCommandTrainLine(ghCommandQueue *cmd, ghRail *rail,char *result) {
  std::string ret = " ";
  if ( rail->IsTrainID(cmd->argstr[0]) ) {
    ret += rail->GetTrainLine(cmd->argstr[0]);
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
ghRailCommandTrainDistance(ghCommandQueue *cmd, ghRail *rail,char *result) {
  std::string ret = " ";
  if ( rail->IsTrainID(cmd->argstr[0]) ) {
    ret += rail->GetTrainDistance(cmd->argstr[0]);
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
ghRailCommandCameraSetPosition(ghCommandQueue *cmd,ghWindow* _win) {

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
ghRailCommandCameraGetPosition(ghCommandQueue *cmd,ghWindow* _win,char *result) {
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
ghRailCommandCameraSetLookat(ghCommandQueue *cmd, ghWindow* _win) {

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
ghRailCommandCameraGetLookat(ghCommandQueue *cmd, ghWindow* _win,char *result) {

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
ghRailCommandCameraSetUpvec(ghCommandQueue *cmd, ghWindow* _win) {

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
ghRailCommandCameraGetUpvec(ghCommandQueue *cmd, ghWindow* _win,char *result) {
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
ghRailCommandCameraSetTracking(ghCommandQueue *cmd,ghRail *rail,ghWindow* _win) {
  if ( rail->IsTrainID(cmd->argstr[1]) ) {
    ghWindow *tmp = ghGetWindowByName(_win,cmd->argstr[0]);
    if ( tmp != NULL ) {
      tmp->tracking = ghString2CharPtr( cmd->argstr[1] );
      return GH_EXECUTE_SUCCESS;
    } else {
      return GH_EXECUTE_NOT_FOUND;
    }
  } else {
    return GH_EXECUTE_NOT_FOUND;
  }
}

int
ghRailCommandCameraGetTracking(ghCommandQueue *cmd,ghWindow* _win,char *result) {
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
ghRailCommandCameraGetViewport(ghCommandQueue *cmd, ghWindow* _win,char *result) {

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
ghRailCommandCameraSetScreen(ghCommandQueue *cmd,ghWindow* _win) {

  ghSetConfigWindow(_win,cmd->argstr[0],cmd->argnum[0],cmd->argnum[1],-1,-1);
  return GH_EXECUTE_SUCCESS;
 
}

int
ghRailCommandCameraGetScreen(ghCommandQueue *cmd,ghWindow* _win, char *result) {
  int current[4];
  ghGetConfigWindow(_win,cmd->argstr[0],&current[0]);
  std::string ret = std::to_string(current[0]);
  ret += " ";
  ret += std::to_string(current[1]);
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }

}

int
ghRailCommandCameraSetWindow(ghCommandQueue *cmd,ghWindow* _win) {

  ghSetConfigWindow(_win,cmd->argstr[0],-1,-1,cmd->argnum[0],cmd->argnum[1]);
  return GH_EXECUTE_SUCCESS;
  
}

int
ghRailCommandCameraGetWindow(ghCommandQueue *cmd,ghWindow* _win, char *result) {
  int current[4];
  ghGetConfigWindow(_win,cmd->argstr[0],&current[0]);
  std::string ret = std::to_string(current[2]);
  ret += " ";
  ret += std::to_string(current[3]);
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }

}


int
ghRailCommandCameraAdd(ghCommandQueue *cmd, ghRail *rail,  ghWindow* _win) {
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
  int maxwin = rail->GetMaxWindow();
  if ( ghCountWindow(_win) == maxwin ) {
    return GH_EXECUTE_SIZE_ERROR;
  }
  return GH_EXECUTE_SUCCESS;
}

int
ghRailCommandCameraRemove(ghCommandQueue *cmd, ghWindow* _win) {
  string cname = cmd->argstr[0];
  ghWindow *tmp = _win;
  for (int i = 0; i < GH_RESERVED_STRING.size(); i++) {
    if ( GH_RESERVED_STRING[i] == cname ) {
      return GH_EXECUTE_RESERVED;
    }
  }
  while (tmp != (ghWindow *)NULL) {
    if ( tmp->name == cname ) {
      return GH_EXECUTE_SUCCESS;
    }
    tmp = tmp->next ;
  }
  return GH_EXECUTE_NOT_FOUND;
}

int
ghRailCommandCameraGet(ghCommandQueue *cmd,ghWindow* _win, char *result) {
  ghWindow *tmp = _win;
  std::string ret = " ";
    
  while (tmp != (ghWindow *)NULL) {
    ret += tmp->name;
    ret += " ";
    tmp = tmp->next ;
  }
  if ( ret.size() < GH_EXECUTE_BUFFER_SIZE ) {
    strcpy(result, ret.c_str());
    return GH_EXECUTE_SUCCESS;
  } else {
    return GH_EXECUTE_SIZE_ERROR;
  }

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
ghRailCommandConfigSetMaxwindow(ghCommandQueue *cmd, ghRail *rail) {
  rail->SetMaxWindow((int)cmd->argnum[0]);
  return GH_EXECUTE_SUCCESS;
}
int
ghRailCommandConfigGetMaxwindow(ghCommandQueue *cmd, ghRail *rail,char *result) {
  int maxwin = rail->GetMaxWindow();
  std::string ret = std::to_string(maxwin);
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
#ifdef _WINDOWS
  int shmkey = 86822723;
#else  
  int shmkey = ftok(GH_SHM_PATH,shmtype+offset);
#endif
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



