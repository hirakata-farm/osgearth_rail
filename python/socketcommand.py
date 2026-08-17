#!/usr/bin/python3
#
#  simple osgearth_rail socket command module
#
#   Copyright (C) 2025-2026 Yuki Osada
#  This software is released under the BSD License, see LICENSE.
#
#
#
import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), 'packages'))

import socket
import re
from datetime import datetime

#############

class SocketClient():

    def __init__(self):
        self.host = None
        self.port = None
        self.datasize = 1024
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
            return re.split(r"\s+", rcv_data)
    
    def close(self):
        if self.socket != None:        
            self.socket.close()
            self.socket = None

##################################################################
remote_socket = SocketClient()

def SetupSocket(size,detail_flag):
    remote_socket.setup(size,detail_flag)
    return

def ConnectSocket(host,port):
    ret = remote_socket.connect(host, port)
    return ret

def CloseSocket():
    remote_socket.close()
    return


def CommandShmSetClockTime():
    retmsg = remote_socket.send("shm set clock time\n")
    return retmsg
#if shmmsg[1] == "Accept" :
#        return False
#    else:
#        return True
    
def CommandShmSetTrainPosition():
    retmsg = remote_socket.send("shm set train position\n")
    return retmsg

def CommandShmSetCameraViewport(camera):
    cmd = "shm set camera " + camera + " viewport\n"
    retmsg = remote_socket.send(cmd)
    return retmsg

def CommandTrainIcon(train):
    cmd = "train icon " + train + "\n"
    retmsg = remote_socket.send(cmd)
    return retmsg

def CommandTrainTimetable(train):
    cmd = "train timetable " + train + "\n"
    retmsg = remote_socket.send(cmd)
    return retmsg

def CommandTrainPosition(train):
    cmd = "train position " + train + "\n"
    retmsg = remote_socket.send(cmd)
    return retmsg

def CommandCameraGetViewport(camera):
    cmd = "camera get " + camera + " viewport\n"        
    retmsg = remote_socket.send(cmd)
    return retmsg

def CommandCameraGetPosition(camera):
    cmd = "camera get " + camera + " position\n"        
    retmsg = remote_socket.send(cmd)
    return retmsg

def CommandCameraSetPosition(camera,lng,lat,alt):
    cmd = "camera set " + camera + " position " + str(lng) + " " + str(lat) + " " + str(alt) + "\n"
    retmsg = remote_socket.send(cmd);
    return retmsg

def CommandCameraSetTracking(camera,train):
    cmd = "camera set " + camera + " tracking " + train + "\n"
    retmsg = remote_socket.send(cmd);
    return retmsg

def CommandClockGetTime():
    retmsg = remote_socket.send("clock get time\n");
    return retmsg

def CommandClockSetTime(timestring):
    cmd = "clock set time " + timestring + "\n"
    retmsg = remote_socket.send(cmd)
    return retmsg
    
def CommandFieldSet(field):
    cmd = "field set " + field + "\n"
    retmsg = remote_socket.send(cmd);
    return retmsg

def CommandConfigAltmode(mode):
    # mode(string)  = clamp(default), absolute, relative 
    cmd = "config set altmode " + mode + "\n"
    retmsg = remote_socket.send(cmd);
    return retmsg

def CommandFieldGetTrain():    
    retmsg = remote_socket.send("field get train\n")
    return retmsg

def CommandClockSetSpeed(speed):
    cmd = "clock set speed " + str(speed) + "\n"
    retmsg = remote_socket.send(cmd)
    return retmsg

def CommandClockGetSpeed():    
    retmsg = remote_socket.send("clock get speed\n");
    return retmsg

def CommandCameraAdd(name,posx,posy,ratio):
    cmd = "camera add " + name + " " + str(posx) + " " + str(posy) + " " + str(ratio) + "\n"
    retmsg = remote_socket.send(cmd);
    return retmsg
    
def CommandCameraSetWindow(name,width,height):
    cmd = "camera set " + name + " window " + str(width) + " " + str(height) + "\n"
    retmsg = remote_socket.send(cmd);
    return retmsg
    
def CommandExit():
    retmsg = remote_socket.send("exit\n");
    return retmsg

def CommandManual(command):
    cmd = command + "\n"
    retmsg = remote_socket.send( cmd );
    return retmsg


def CommandRunSimulation():
    retmsg = remote_socket.send("run\n");
    return retmsg


def CommandPauseSimulation():            
    retmsg = remote_socket.send("pause\n");
    return retmsg



