#!/usr/bin/python3
#
#  osgearth_rail command line tool
#
#   Copyright (C) 2025-2026 Yuki Osada
#  This software is released under the BSD License, see LICENSE.
#
#
#  args
# "-t", "--host", default="localhost"
# "-p", "--port", default="57139"
# "-m", "--mode", default="text")
#
#

import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), 'packages'))

import argparse
import socket
import re
import json
from datetime import datetime
##########################
#
#
#   Command List
#
#1. Control Command
#
#exit
#play
#pause
#show (status|version)
#
#
#2. Clock Command
#
#clock [set|get] [time|speed] (number)
#
#clock set time 12:12
#clock set speed 12 ( 0.1 - 12 )
#
#clock get time
#clock get speed
#
#
#3. Camera Command , [camera name] = default [root]
#
#camera (set|get) [camera name] (position|lookat|upvec|tracking|screen|window) (number) (number) (number)
#
#camera set [ camera name ] position 35.2 53.2 32 ( lng[deg], lat[deg], alt[m] )
#camera set [ camera name ] lookat 35.2 53.2 32   ( lng[deg], lat[deg], alt[m] )
#camera set [ camera name ] upvec 35.2 53.2 32   ( lng[deg], lat[deg], alt[m] )
#camera set [ camera name ] tracking 1241 ( train_number or none )
#camera set [ camera name ] screen 100 200 ( screen_position_x, screen_position_y )
#camera set [ camera name ] window 640 480 ( window_width, window_height ) 
#camera add [ camera name ] 0 0 1024 768 ( screen_position_x screen_position_y window_width window_height )
#camera remove [ camera name ]
#
#camera get [ camera name ] position
#camera get [ camera name ] lookat
#camera get [ camera name ] upvec
#camera get [ camera name ] tracking
#camera get [ camera name ] viewport
#camera get [ camera name ] screen
#camera get [ camera name ] window
#camera get all
#
#
#4. Field Command
#
#field (set|get) [configuration specific ID]
#
#field set G012142123
#
#field get id
#field get train
#field get line
#field get timezone
#field get description
#
#
#5. Train Command
#
#train (label|position|timetable|icon|line|distance) [train id]
#
#
#train label [train id] [on|off]
#  available "all" keyword  %train label all on
#train position [train id]
#  available "all" keyword  %train position all
#train timetable [train id]
#train icon [train id]
#train line [train id]
#train distance [train id]
#
#6. Configure Command
#
#config (set|get) (maxclockspeed|altmode|displaydistance|maxcamera) (number)
#
#config set maxclockspeed 30.0 ( default 12.0 )
#config set altmode clamp [ clamp(default), absolute, relative ]
#config set displaydistance 100000 ( default 3000[m] )
#config set maxcamera 8 ( default 4 )
#config get maxclockspeed
#config get altmode
#config get displaydistance
#config get maxcamera
#
#
#7. Shared Memory Command ( windows or linux(SYSV) )
#
#shm (set|remove) (clock|train|camera) (time|position|viewport)
#
#shm set clock time
#shm set train position
#shm set camera [camera name] viewport
#shm remove [ shm key name ]
#
#

##########################
#
# Command Code Define
#
##########################

GH_COMMAND_EXIT   = 10
GH_COMMAND_PLAY   = 12
GH_COMMAND_PAUSE  = 15
GH_COMMAND_SHOW   = 17
GH_COMMAND_CLOCK  = 21
GH_COMMAND_CAMERA = 22
GH_COMMAND_FIELD  = 23
GH_COMMAND_TRAIN  = 24
GH_COMMAND_CONFIG = 25
GH_COMMAND_SHM    = 26


GH_TYPE_SET       = 1100
GH_TYPE_GET       = 1200
GH_TYPE_ADD       = 1300
GH_TYPE_REMOVE    = 1400
GH_TYPE_STATUS    = 2100
GH_TYPE_VERSION   = 2200
GH_TYPE_LABEL     = 2300 
GH_TYPE_SETCLOCK  = 2400
GH_TYPE_SETTRAIN  = 2500
GH_TYPE_SETCAMERA = 2600


GH_KEY_TIME            = 140000
GH_KEY_SPEED           = 150000
GH_KEY_POSITION        = 160000
GH_KEY_LOOKAT          = 170000
GH_KEY_UPVEC           = 180000
GH_KEY_TRACKING        = 190000
GH_KEY_SCREEN          = 200000
GH_KEY_WINDOW          = 210000
GH_KEY_VIEWPORT        = 220000
GH_KEY_ALL             = 230000
GH_KEY_ID              = 240000
GH_KEY_TRAIN           = 250000
GH_KEY_LINE            = 260000
GH_KEY_TIMEZONE        = 270000
GH_KEY_DESCRIPTION     = 280000
GH_KEY_LABEL           = 290000
GH_KEY_ON              = 300000
GH_KEY_OFF             = 310000
GH_KEY_TIMETABLE       = 320000
GH_KEY_ICON            = 330000
GH_KEY_DISTANCE        = 340000
GH_KEY_MAXCLOCKSPEED   = 410000
GH_KEY_ALTMODE         = 420000
GH_KEY_DISPLAYDISTANCE = 430000
GH_KEY_MAXCAMERA       = 440000


GH_ALTMODE_CLAMP    = 0
GH_ALTMODE_RELATIVE = 1
GH_ALTMODE_ABSOLUTE = 2


def isfloat(s):
    try:
        float(s)
    except ValueError:
        return False
    else:
        return True

def isint(s):
    try:
        int(s)
    except ValueError:
        return False
    else:
        return True

###############
class SocketClient():

    def __init__(self):
        self.host = None
        self.port = None
        self.datasize = 16384
        self.showtimestamp = False
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)       
        
    def connect(self,host,port):
        self.host = host
        self.port = port
        try:
            self.socket.connect((host, port))
            print('[{0}] server connect -> address : {1}:{2}'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S'), self.host, self.port) )
            return True
        except socket.error:
            print('[{0}] server connect ERROR -> address : {1}:{2}'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S'), self.host, self.port) )
            return False

    def setup(self,size,isdetail):
        self.datasize = size
        self.showtimestamp = isdetail
        
    def send(self,message):
        if self.socket != None:
            self.socket.send( message.encode('utf-8') )
            if self.showtimestamp:
                print('[{0}] Send:{1}'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S'), message.rstrip()) )
            rcv_data = self.socket.recv(self.datasize)
            rcv_data = rcv_data.decode('utf-8').rstrip()
            if self.showtimestamp:
                print('[{0}] Recv:{1}'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S'), rcv_data) )
            #return re.split(r"\s+", rcv_data)
            return json.loads(rcv_data)
    
    def close(self):
        if self.socket != None:        
            self.socket.close()
            self.socket = None

##################################################################

def SetupSocket(size,detail_flag):
    remote_socket.setup(size,detail_flag)
    return

def ConnectSocket(host,port):
    ret = remote_socket.connect(host, port)
    return ret

def CloseSocket():
    remote_socket.close()
    return


#################################################################

def CommandShmSet(val,code):
    if val == None:
        data = {
            "utime" : int(datetime.now().timestamp()),
            "cmd" : code,
            "size" : 0,
            "rev" : 1
        }
    else:
        data = {
            "utime" : int(datetime.now().timestamp()),
            "cmd" : code,
            "name" : val,
            "size" : 0,
            "rev" : 1
        }
        
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    #print (json_str)
    return remote_socket.send(json_str)

def CommandShmRemove(key):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : 1426,
        "name" : key,
        "size" : 0,
        "rev" : 1
    }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)

def CommandTrainLabelSet(train,code):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : code,
        "name" : train,
        "size" : 0,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)

def CommandTrainGet(train,code):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : code,
        "name" : train,
        "size" : 0,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)

def CommandCameraSetPosition(camera,lng,lat,alt):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : 161122,
        "name" : camera,
        "args" : [ lng, lat, alt ],
        "size" : 3,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)

def CommandCameraSetLookat(camera,lng,lat,alt):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : 171122,
        "name" : camera,
        "args" : [ lng, lat, alt ],
        "size" : 3,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)

def CommandCameraSetUpvec(camera,lng,lat,alt):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : 181122,
        "name" : camera,
        "args" : [ lng, lat, alt ],
        "size" : 3,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)

def CommandCameraSetTracking(camera,train):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : 191122,
        "name" : camera,
        "args" : [ train ],
        "size" : 1,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)

def CommandCameraSetScreen(camera,posx,posy):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : 201122,
        "name" : camera,
        "args" : [ posx, posy ],
        "size" : 2,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)

def CommandCameraSetWindow(camera,width,height):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : 211122,
        "name" : camera,
        "args" : [ width, height ],
        "size" : 2,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)    

def CommandCameraGet(name,code):    
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : int(code),
        "name" : name,
        "size" : 0,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)

def CommandClockSetTime(hour,mins):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : 141121,
        "args" : [ hour, mins ],
        "size" : 2,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)    
    
def CommandFieldSet(field):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : 1123,
        "name" : field,
        "size" : 0,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)    

def CommandConfigSet(val,code):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : code,
        "args" : [ val ],
        "size" : 1,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)    

def CommandClockSetSpeed(speed):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : 151121,
        "args" : [ speed ],
        "size" : 1,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)    

def CommandCameraAdd(name,posx,posy,width,height):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : 1322,
        "name" : name,
        "args" : [ posx,posy,width,height ],
        "size" : 4,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)

def CommandCameraRemove(name):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : 1422,
        "name" : name,
        "size" : 0,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)
    
def CommandCodeOnly(code):
    data = {
        "utime" : int(datetime.now().timestamp()),
        "cmd" : code,
        "size" : 0,
        "rev" : 1
        }
    json_str = json.dumps(data,ensure_ascii=False, indent=1)
    return remote_socket.send(json_str)

###########################################

def CmdClockSet(key,val):
    if key == "time":
        parts = val.split(":")
        size = len(parts)
        if size == 2:
            if isint(parts[0]) and isint(parts[1]):
                h = int(parts[0])
                m = int(parts[1])
                if h < 24 and h > -1 and m < 60 and m > -1:
                    #tstr = str(h) + ":" + str(m)
                    return CommandClockSetTime(h,m)
                else:
                    return "clock set time: wrong time data"
            else:
                return "clock set time: wrong arguments"
        else:
            return "clock set time: wrong arguments"
            
    elif key == "speed":
        if isfloat(val):
            v = float(val)
            if v > 0.001  and v < 12.1:
                return CommandClockSetSpeed(v)
            else:
                return "clock set speed: wrong speed data"
        else:
            return "clock set speed: wrong speed data"
    else:
        return "clock set speed: wrong arguments"

def CmdClockGet(key):
    if key == "time":
        return CommandCodeOnly(141221)
    elif key == "speed":
        return CommandCodeOnly(151221)
    else:
        return "clock get : wrong arguments"

def CmdFieldGet(key):
    match key:
        case "id":
            return CommandCodeOnly(241223)
        case "train":
            return CommandCodeOnly(251223)
        case "line":
            return CommandCodeOnly(261223)
        case "timezone":
            return CommandCodeOnly(271223)
        case "description":
            return CommandCodeOnly(281223)
        case _:
            return "field get : wrong arguments"

def CmdTrainLabel(train,key):
    if key == "on":
        return CommandTrainLabelSet(train,302324)
    elif key == "off":
        return CommandTrainLabelSet(train,312324)
    else:
        return "train label : wrong arguments"

def CmdCameraSet(name,val0,val1,val2,val3):
    match val0:
        case "position":
            if isfloat(val1) and isfloat(val2) and isfloat(val3):
                return CommandCameraSetPosition(name,float(val1),float(val2),float(val3))
            else:
                return "camera set : wrong arguments"
        case "lookat":
            if isfloat(val1) and isfloat(val2) and isfloat(val3):
                return CommandCameraSetLookat(name,float(val1),float(val2),float(val3))
            else:
                return "camera set : wrong arguments"
        case "upvec":
            if isfloat(val1) and isfloat(val2) and isfloat(val3):
                return CommandCameraSetUpvec(name,float(val1),float(val2),float(val3))
            else:
                return "camera set : wrong arguments"
        case "tracking":
            return CommandCameraSetTracking(name,val1)
        case "screen":
            if isint(val1) and isint(val2):
                return CommandCameraSetScreen(name,int(val1),int(val2))
            else:
                return "camera set : wrong arguments"
        case "window":
            if isint(val1) and isint(val2):
                return CommandCameraSetWindow(name,int(val1),int(val2))
            else:
                return "camera set : wrong arguments"
        case _:
            return "camera set : wrong arguments"
    

def CmdCameraGet(name,val0):

    if name == "all":
        return CommandCameraGet(name,231222)
    else:
        match val0:
            case "position":
                return CommandCameraGet(name,161222)
            case "lookat":
                return CommandCameraGet(name,171222)
            case "upvec":
                return CommandCameraGet(name,181222)
            case "tracking":
                return CommandCameraGet(name,191222)
            case "screen":
                return CommandCameraGet(name,201222)
            case "window":
                return CommandCameraGet(name,211222)
            case "viewport":
                return CommandCameraGet(name,221222)
            case _:
                return "camera get : wrong arguments"

def CmdConfigSet(key,val):
    match key:
        case "maxclockspeed":
            if isfloat(val):
                return CommandConfigSet(val,411125)
            else:
                return "config set maxclockspeed : wrong arguments"
            
        case "altmode":
            match val:
                case "clamp":
                    return CommandConfigSet(GH_ALTMODE_CLAMP,421125)
                case "absolute":
                    return CommandConfigSet(GH_ALTMODE_ABSOLUTE,421125)
                case "relative":
                    return CommandConfigSet(GH_ALTMODE_RELATIVE,421125)
                case _:
                    return "config set altmode : wrong arguments"
                    
        case "displaydistance":
            if isint(val):
                return CommandConfigSet(val,431125)
            else:
                return "config set displaydistance : wrong arguments"
        case "maxcamera":
            if isint(val):
                return CommandConfigSet(val,441125)
            else:
                return "config set maxcamera : wrong arguments"
        case _:
            return "config set : wrong arguments"

def CmdConfigGet(key):
    match key:
        case "maxclockspeed":
            return CommandCodeOnly(411225)
        case "altmode":
            return CommandCodeOnly(421225)
        case "displaydistance":
            return CommandCodeOnly(431225)
        case "maxcamera":
            return CommandCodeOnly(441225)
        case _:
            return "config get : wrong arguments"

def CmdShmSet(key,val0,val1):
    match key:
        case "clock":
            if val0 == "time":
                return CommandShmSet(None,142426)
            else:
                return "shm set clock : wrong arguments"
            
        case "train":
            if val0 == "position":
                return CommandShmSet(None,162526)
            else:
                return "shm set train : wrong arguments"
        case "camera":
            if val1 == "viewport":
                return CommandShmSet(val0,222626)
            else:
                return "shm set camera : wrong arguments"
        case _:
            return "shm set : wrong arguments"

###################################
#

def getResultMessage(data):
    global return_mode
    if data == None:
        return "none"
    if return_mode == "json":
        return data
    else:
        msg = ""
        for item in data['result']:
            if isfloat(item):
                msg += str(item)
            else:
                msg += item
            msg += " "
        return msg

#
###################################

return_mode = "text"
psr = argparse.ArgumentParser()
psr.add_argument("-t", "--host", default="localhost")
psr.add_argument("-p", "--port", default="57139")
psr.add_argument("-m", "--mode", default="text")
pargs = psr.parse_args()

remote_socket = SocketClient()
remote_socket.setup(16384,False)
#remote_socket.setup(16384,True)

if remote_socket.connect(pargs.host, int(pargs.port)):
    print("connection OK", pargs.host, pargs.port)
else:
    print("connection ERROR exited ", pargs.host, pargs.port)
    sys.exit()

if pargs.mode == "json":
    return_mode = "json"
print("return mode ", return_mode)
    
def main():
  while True:
    try:
      # show prompt , get input data
      cwd = os.getcwd()
      if return_mode == "json":
          cmd_input = input()
      else:
          cmd_input = input(f"osgearth_rail:{cwd}> ")

      # delete white space
      cmd_input = cmd_input.strip()
      if not cmd_input:
        continue

      # loop exit 
      #if cmd_input in ("exit", "quit"):
      #  break
      if cmd_input in ("quit"):
        break

      # split command input
      parts = cmd_input.split()
      command = parts[0]
      args = parts[1:]

      if len(args) < 1:
          match command:
              case "exit":
                  print( getResultMessage( CommandCodeOnly(10) ) )
                  sys.exit()
              case "play":
                  print( getResultMessage( CommandCodeOnly(12) ) )
              case "pause":
                  print( getResultMessage( CommandCodeOnly(15) ) )
              case _:
                  print("command : wrong arguments")
      else:
          match command:
              case "clock":
                  if args[0] == "set":
                      print( getResultMessage( CmdClockSet(args[1],args[2]) ) )
                  elif args[0] == "get":
                      print( getResultMessage( CmdClockGet(args[1]) ) )
                  else:
                      print("clock : wrong arguments")
              case "camera":
                  match args[0]:
                      case "set":
                          match len(args):
                              case 6:
                                  print( getResultMessage( CmdCameraSet(args[1],args[2],args[3],args[4],args[5]) ) )
                              case 5:
                                  print( getResultMessage( CmdCameraSet(args[1],args[2],args[3],args[4],0.0) ) )
                              case 4:
                                  print( getResultMessage( CmdCameraSet(args[1],args[2],args[3],0.0,0.0) ) )
                              case _:
                                  print("camera set : wrong arguments")                              
                      case "get":
                          if len(args) == 3:
                              print( getResultMessage( CmdCameraGet(args[1],args[2]) ) )
                          elif len(args) == 2:
                              print( getResultMessage( CmdCameraGet(args[1],0.0) ) )
                          else:
                              print("camera get : wrong arguments")
                      case "add":
                          if len(args) == 6:
                              print( getResultMessage( CommandCameraAdd(args[1],float(args[2]),float(args[3]),float(args[4]),float(args[5])) ) )
                          else:
                              print("camera add: wrong arguments")
                      case "remove":
                          print( getResultMessage( CommandCameraRemove(args[1]) ) )
                      case _:
                          print("camera : wrong arguments")

              case "field":
                  if args[0] == "set":
                      print( getResultMessage( CommandFieldSet(args[1]) ) )
                  elif args[0] == "get":
                      print( getResultMessage( CmdFieldGet(args[1]) ) )
                  else:
                      print("field : wrong arguments")
              case "train":
                  match args[0]:
                      case "label":
                          if len(args) == 3:
                              print( getResultMessage( CmdTrainLabel(args[1],args[2]) ) )
                          else:
                              print("field label : wrong arguments")                          
                      case "position":
                          # 160000 + 1200 + 24
                          print( getResultMessage( CommandTrainGet(args[1],161224) ) )
                      case "timetable":
                          print( getResultMessage( CommandTrainGet(args[1],321224) ) )
                      case "icon":
                          print( getResultMessage( CommandTrainGet(args[1],331224) ) )
                      case "line":
                          print( getResultMessage( CommandTrainGet(args[1],261224) ) )
                      case "distance":
                          print( getResultMessage( CommandTrainGet(args[1],341224) ) )
                      case _:
                          print("field : wrong arguments")
              case "config":
                  if args[0] == "set":
                      if len(args) == 3:
                          print( getResultMessage( CmdConfigSet(args[1],args[2]) ) )
                      else:
                          print("config set : wrong arguments")
                  elif args[0] == "get":
                      print( getResultMessage( CmdConfigGet(args[1]) ) )
                  else:
                      print("config : wrong arguments")
              case "shm":
                  if args[0] == "set":
                      if len(args) == 4:
                          print( getResultMessage( CmdShmSet(args[1],args[2],args[3]) ) )
                      elif len(args) == 3:
                          print( getResultMessage( CmdShmSet(args[1],args[2],None) ) )
                      else:
                          print("shm set : wrong arguments")                          
                  elif args[0] == "remove":
                      print( getResultMessage( CommandShmRemove(args[1]) ) )
                  else:
                      print("field : wrong arguments")
              case "show":
                  if args[0] == "status":
                      print( getResultMessage( CommandCodeOnly(2117) ) )
                  elif args[0] == "version":
                      print( getResultMessage( CommandCodeOnly(2217) ) )
                  else:
                      print("show : wrong arguments")
              case _:
                  print("Unknown command ")

    except KeyboardInterrupt:
      print()  # ignore Ctrl+C
    except FileNotFoundError:
      print(f"Unkown command: {cmd_input}")
    except Exception as e:
      print(f"Error: {e}")


if __name__ == "__main__":
  main()


  
