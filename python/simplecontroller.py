#!/usr/bin/python3
#
#  simple osgearth_rail controller
#
#   Copyright (C) 2025-2026 Yuki Osada
#  This software is released under the BSD License, see LICENSE.
#
#
#
#
#  Required tkintermapview packages
#    https://github.com/TomSchimansky/TkinterMapView
#
#
# apt-get install python3-tk python3-pip python3-pil python3-pil.imagetk python3-sysv-ipc
# pip3 install tkintermapview -t packages
# pip3 install geopy -t packages
#
#
#
import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), 'packages'))

import socket
import time
from datetime import datetime
import re
import threading
import tkinter
import tkintermapview
from urllib.request import urlopen
from PIL import Image, ImageTk
from tkinter import Menu,ttk,messagebox
from geopy.distance import geodesic

import math
import struct
import random
import io
import subprocess
import importlib.util

######################################################
#
#  Check for Shared Memory Library
#

def is_module_available(module_name: str) -> bool:
    return importlib.util.find_spec(module_name) is not None

shm_mode = False
default_host = "localhost"
if is_module_available("sysv_ipc"):
    import sysv_ipc
    shm_mode = True
    default_host = "localhost"
else:
    shm_mode = False
    default_host = socket.gethostbyname(socket.gethostname())
    
######################################################
#
#  Global Settings
#
main_window_width = 1024
main_window_height = 768
icon_width = 16
icon_height = 16
remote_host = default_host
remote_port = 57139
socket_buffer_size = 16384
show_socket_detail = True
def about():
    messagebox.showinfo("about", "simple osgearth_rail controller 0.5.3")

def cpuload():
    subprocess.Popen(["python3","cpusage.py"])
    #subprocess.Popen(["python.exe","cpusage.py"])  # for Windows
    

######################################################
#
#
field_isloaded = False
field_isrunning = False
shm_time = None
shm_train = None
shm_train_size_byte = 0
shm_camera = { "root" : None }
viewport_camera = {};
tracking_distance = 1500   # [m]
tracking_height = 200      # [m]
#############
polling_thread = None
polling_socket_wait_second = 28
polling_shm_wait_second = 1
#############
custom_auto_tracking = False
custom_auto_tracking_count = 5
custom_auto_tracking_times = 6
#############

class SocketClient():

    def __init__(self,datasize,flag):
        self.host = None
        self.port = None
        self.datasize = datasize
        self.showtimestamp = flag
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
             
remote_socket = SocketClient(socket_buffer_size,show_socket_detail)

##################################################################


def setup_shm():
    global shm_time
    global shm_train
    global shm_train_size_byte
    global shm_camera
    global remote_polling_second
    shmmsg = remote_socket.send("shm set clock time\n")
    if shmmsg[3].isnumeric():
        shm_time = sysv_ipc.SharedMemory(int(shmmsg[3]))
    shmmsg = remote_socket.send("shm set train position\n")
    if shmmsg[3].isnumeric():
        shm_train = sysv_ipc.SharedMemory(int(shmmsg[3]))
        shm_train_size_byte = int(shmmsg[4])
    shmmsg = remote_socket.send("shm set camera root viewport\n")
    if shmmsg[4].isnumeric():
        shm_camera["root"] = sysv_ipc.SharedMemory(int(shmmsg[4]))
    remote_polling_second = 1
    

def timetable_dialog(id, data):

    tablelist = data.split(",")
    dialog = tkinter.Toplevel(root_tk)
    dialog.attributes('-topmost', True)
    dialog.geometry("300x400")
    dialog.title("Timetable  " + id)

    tree = ttk.Treeview(dialog)
    tree["columns"] = ("station", "time")
    tree.column("#0", width=0, stretch=tkinter.NO)
    tree.column("station", anchor=tkinter.CENTER, width=100)
    tree.column("time", anchor=tkinter.CENTER, width=80)

    tree.heading("#0", text="", anchor=tkinter.CENTER)
    tree.heading("station", text="Station", anchor=tkinter.CENTER)
    tree.heading("time", text="Time", anchor=tkinter.CENTER)

    for i in range(0,len(tablelist),2):
        tree.insert('',tkinter.END, values=(tablelist[i+1],tablelist[i]))
    
    scrollbar = ttk.Scrollbar(dialog, orient=tkinter.VERTICAL, command=tree.yview)
    tree.configure(yscrollcommand=scrollbar.set)
    scrollbar.pack(side=tkinter.RIGHT, fill=tkinter.Y)

    def on_close_ok():
        dialog.destroy()

    tree.pack(expand=True, fill="both")

    ok_button = tkinter.Button(dialog, text="close", fg="red", command=on_close_ok)
    ok_button.pack()


def setup_train_marker(tname,lat,lng):
    message = "train icon " + tname + "\n"
    msgstr = remote_socket.send(message)
    if 'http' in msgstr[3]:
        imagedata = urlopen(msgstr[3])
        imageobject = Image.open(io.BytesIO(imagedata.read()))
        resized_image = imageobject.resize((icon_width, icon_height), Image.Resampling.LANCZOS)
        #image = ImageTk.PhotoImage(data=imagedata.read())
        image = ImageTk.PhotoImage(resized_image)
        markers[tname] = map_widget.set_marker( lat, lng, text=tname, icon=image, command=click_train_marker )
    else:
        print( msgstr )
    
def click_train_marker(marker):
    message = "train timetable " + marker.text + "\n"
    response = remote_socket.send(message)
    train_id = response[2]
    train_timetable = " ".join(response[3:])
    timetable_dialog( train_id, train_timetable.rstrip(',') )

def get_random_train_id():
    global trainid
    length = len(trainid)
    while True:
        idx = int(random.uniform(0, length-1))
        if trainid[idx] in markers:
            return trainid[idx]

def is_same_tracking_id_in_windows(trainid):
    for item in windows:
        if trainid == windows[item]:
            return True
    return False        
                        
    
def update_socket_train(data):
    global trainid
    
    markerlist = data.split(",")
    for item in trainid:
        existflag = False
        for updatemarker in markerlist:
            markerdata = updatemarker.split(" ")
            trainname = markerdata[0]
            if trainname == item:
                lng = float(markerdata[1])
                lat = float(markerdata[2])
                if trainname in markers:
                    markers[trainname].set_position(lat,lng)
                else:
                    if len(trainname) > 1:
                        setup_train_marker(trainname,lat,lng)
                existflag = True
                break
        if existflag == False:
            if item in markers:
                markers[item].delete()
                del markers[item]

def update_socket_viewport(data):
    map_widget.delete_all_polygon()
    positionlist = []
    pointlist = data.split(",")
    for point in pointlist:
        pointdata = point.split(" ")
        if len(pointdata) > 1:
            lng = float(pointdata[0])
            lat = float(pointdata[1])
            positionlist.append((lat,lng))
    if len(positionlist) > 3:
        viewport = map_widget.set_polygon(positionlist,fill_color=None,border_width=2)

def update_shm_clock():
    global shm_time
    global timelabel_var

    bytedata = shm_time.read()
    datasec = int.from_bytes(bytedata, byteorder='little', signed=True)
    dhour = math.floor(datasec / 3600) ;
    dmin = math.floor( ( datasec - dhour * 3600 ) / 60) ;
    dsec = datasec % 60 ;
    shour = str(dhour)
    smin = str(dmin)
    ssec = str(dsec)
    if dhour < 10:
        shour = "0" + shour
    if dmin < 10:
        smin = "0" + smin
    if dsec < 10:
        ssec = "0" + ssec
    timelabel_var.set(shour + ":" + smin + ":" + ssec)
    
def update_shm_train():
    global shm_train
    global shm_train_size_byte
    #global tracking_id
    bytedata = shm_train.read()
    offset = 0
    for icnt in range(0,shm_train_size_byte,48):
        if (offset+48) < shm_train_size_byte+1:
            byteslice = bytedata[offset:offset+48]
            traindata = struct.unpack('<32s2d', byteslice)
            trainstr = traindata[0].decode('ascii')
            nullpos = trainstr.find('\0')
            trainname = trainstr[0:nullpos]
            lng = float(traindata[1])
            lat = float(traindata[2])
            if lat < 0 and lng < 0:
                if trainname in markers:
                    markers[trainname].delete()
                    del markers[trainname]
            else:
                if trainname in markers:
                    markers[trainname].set_position(lat,lng)
                else:
                    if len(trainname) > 1:
                        setup_train_marker(trainname,lat,lng)
            offset = offset+48
        else:
            print(offset)
            byteslice = bytedata[offset:]
            traindata = struct.unpack('<32s2d', byteslice)
            print(traindata)

def update_shm_viewport():
    global shm_camera
    global viewport_camera

    map_widget.delete_all_polygon()
    for item in shm_camera:
        bytedata = shm_camera[item].read()
        double_number = struct.unpack('<24d', bytedata)
        positionlist = []
        for i in range(0,24,2):
            if double_number[i+1] != 0.0 or double_number[i] != 0.0:
                positionlist.append((double_number[i+1],double_number[i]))
        if len(positionlist) > 3:            
            viewport_camera[item] = map_widget.set_polygon(positionlist,fill_color=None,outline_color="yellow",border_width=4)
                    

def update_map_center(lat,lng):
    center = map_widget.get_position()
    # center[2] = lat lng
    mapredraw = False
    if abs(lat - center[0]) > 0.001:
        mapredraw = True
    if abs(lng - center[1]) > 0.001:
        mapredraw = True
    if mapredraw:
        map_widget.set_position(lat,lng)        
        
def update_thread_socket():
    global field_isloaded
    global field_isrunning
    global polling_socket_wait_second
    
    while True:
        if field_isloaded:
            if field_isrunning:
                timestr = remote_socket.send("clock get time\n");
                if len(timestr) > 3 and timestr[0] == "Geoglyph:clock":
                    timelabel_var.set(timestr[3])
                response = remote_socket.send("train position all\n");
                update_socket_train(" ".join(response[3:]))
                response = remote_socket.send("camera get root viewport\n");
                update_socket_viewport(" ".join(response[4:]))
                custom_auto_tracking_camera()
                time.sleep(polling_socket_wait_second)
            else:
                time.sleep(1)
        else:
            break

def update_thread_shm():
    global field_isloaded
    global field_isrunning
    global polling_shm_wait_second

    while True:
        if field_isloaded:
            if field_isrunning:
                update_shm_clock()
                update_shm_train()
                update_shm_viewport()
                custom_auto_tracking_camera()
                time.sleep(polling_shm_wait_second)
            else:
                time.sleep(1)
        else:
            break


#####################################################################        
def server_initialize(fieldid):
    global trainid
    global field_isloaded
    global remote_host
    global shm_mode
    global polling_thread

    message = "field set " + fieldid + "\n"
    fieldresult = remote_socket.send(message);
    if len(fieldresult) > 3 or len(fieldresult) < 2:
        return False
    menu0.entryconfig("3D view open", state=tkinter.DISABLED)
    menu0.entryconfig("3D view close", state=tkinter.NORMAL)
    menu1.entryconfig("clock", state=tkinter.NORMAL)
    menu1.entryconfig("speed", state=tkinter.NORMAL)
    menu1.entryconfig("camera", state=tkinter.NORMAL)
    menu1.entryconfig("manual input", state=tkinter.NORMAL)
    menu_button_2.config(state=tkinter.NORMAL)
    menu_button_3.config(state=tkinter.DISABLED)
    time.sleep(3) # wait time for Setup 3D Viewer        
    response = remote_socket.send("clock set time 12:00\n")
    time.sleep(3) # wait time for Setup 3D Viewer        
    response = remote_socket.send("config set altmode relative\n")
    time.sleep(3) # wait time for Setup 3D Viewer                
    trainid = remote_socket.send("field get train\n")
    del trainid[0]  # Geoglyph header string
    del trainid[0]  # train string
    if remote_host == "localhost":
        shm_mode = True
        setup_shm()
        polling_thread = threading.Thread(target=update_thread_shm)
        print (" shared memory mode")
    else:
        shm_mode = False
        polling_thread = threading.Thread(target=update_thread_socket)
        print (" socket mode")
    field_isloaded = True
    polling_thread.start()
    return True

def server_multiview_script():
    global custom_auto_tracking
    response = remote_socket.send("clock set time 9:10\n")
    time.sleep(2)
    response = remote_socket.send("clock set speed 0.1\n")
    time.sleep(2)
    message = "camera add camera1 1280 720 0.3\n"
    response = remote_socket.send(message);
    windows["camera1"] = "NONE"
    time.sleep(2)
    message = "camera set camera1 window 1280 710\n"
    response = remote_socket.send(message);
    time.sleep(2)
    message = "camera add camera2 2560 720 0.3\n"
    response = remote_socket.send(message);
    windows["camera2"] = "NONE"
    time.sleep(2)
    message = "camera set camera2 window 1280 710\n"
    response = remote_socket.send(message);
    time.sleep(2)
    message = "camera add camera3 0 1440 0.3\n"
    response = remote_socket.send(message);
    windows["camera3"] = "NONE"
    time.sleep(2)
    message = "camera set camera3 window 2560 710\n"
    response = remote_socket.send(message);
    time.sleep(2)
    message = "camera add camera4 2560 1440 0.3\n"
    response = remote_socket.send(message);
    windows["camera4"] = "NONE"
    time.sleep(2)
    message = "camera set camera4 window 1280 710\n"
    response = remote_socket.send(message);
    time.sleep(2)
    run_command()
    custom_auto_tracking = True
    menu1.entryconfig("4K Multiview Script", state=tkinter.DISABLED)    


    
#####################################################################


    
def server_connect_dialog():
    global remost_host

    dialog = tkinter.Toplevel(root_tk)
    dialog.geometry("400x400")
    dialog.title("3D view data Load")

    lbl = tkinter.Label(dialog,text='osgearth_rail IP address')
    lbl.place(x=30, y=10)

    host_txt = tkinter.Entry(dialog,width=20)
    host_txt.place(x=200, y=10)
    host_txt.insert(0,remote_host)
    
    selected_option = tkinter.StringVar(value="G175448050EUROTHALYS")

    option01_radio = ttk.Radiobutton(dialog, text="Eurostar and Thalys (UK,France,Belguim,Netherland)", value="G175448050EUROTHALYS", variable=selected_option)
    option02_radio = ttk.Radiobutton(dialog, text="TGV (France)", value="G175351030TGV", variable=selected_option)
    option03_radio = ttk.Radiobutton(dialog, text="ICE (Germany)", value="G175396040ICE", variable=selected_option)
    option04_radio = ttk.Radiobutton(dialog, text="ACELA (USA)", value="G174087320ACELA", variable=selected_option)

    option05_radio = ttk.Radiobutton(dialog, text="England (UK)", value="G177028100ENGLAND", variable=selected_option)
    option06_radio = ttk.Radiobutton(dialog, text="Switzerland (CH)", value="G177182101SWISS", variable=selected_option)
    option07_radio = ttk.Radiobutton(dialog, text="Ireland (IRE)", value="G175656102IRELAND", variable=selected_option)
    option08_radio = ttk.Radiobutton(dialog, text="Netherland (NS)", value="G175930105DUTCH", variable=selected_option)
    option09_radio = ttk.Radiobutton(dialog, text="Portugal (PT)", value="G177090107PORTUGUESE", variable=selected_option)
    option10_radio = ttk.Radiobutton(dialog, text="Spain (ES)", value="G177182108SPAIN", variable=selected_option)

    option11_radio = ttk.Radiobutton(dialog, text="Custom", value="CUSTOM", variable=selected_option)

    option01_radio.place(x=30,y=40)
    option02_radio.place(x=30,y=60)
    option03_radio.place(x=30,y=80)
    option04_radio.place(x=30,y=100)
    option05_radio.place(x=30,y=120)
    option06_radio.place(x=30,y=140)
    option07_radio.place(x=30,y=160)
    option08_radio.place(x=30,y=180)
    option09_radio.place(x=30,y=200)
    option10_radio.place(x=30,y=220)
    
    option11_radio.place(x=30,y=240)        
    tc_txt = tkinter.Entry(dialog,width=20)
    tc_txt.place(x=120, y=240)
    tc_txt.insert(0,"require ID")

    def on_server_ok():
        global remote_host
        remote_host = host_txt.get()
        custom_code = tc_txt.get()
        fieldid = selected_option.get()

        if fieldid == "CUSTOM":
            if ' ' in custom_code:
                messagebox.showinfo("Warning", "Invalid custom code")
                return
            if custom_code.isspace():
                messagebox.showinfo("Warning", "Invalid custom code")
                return 
            fieldid = custom_code
        
        response = remote_socket.connect(remote_host, remote_port)
        if response == False:
            dialog.destroy()
            messagebox.showinfo("Warning", "3D viewer Error")
            return
        time.sleep(1) # wait time for Setup 3D Viewer
        status = server_initialize( fieldid )
        if status == True:
            messagebox.showinfo("Information", "3D viewer setting up OK")
        else:
            messagebox.showinfo("Warning", "3D viewer NOT setup")
        dialog.destroy()

    def on_server_no():
        dialog.destroy()
        
    ok_button = tkinter.Button(dialog, text="Data Load", fg="blue", command=on_server_ok)
    ok_button.place(x=30,y=300)
    no_button = tkinter.Button(dialog, text="Close", fg="red", command=on_server_no)
    no_button.place(x=200,y=300)


    dialog.grab_set() # modal dialog
    root_tk.wait_window(dialog) # wait for close dialog

def server_exit_dialog():
    global field_isloaded
    dialog = tkinter.Toplevel(root_tk)
    dialog.geometry("400x160")
    dialog.title("3D view close")

    def on_exit_ok():
        global field_isloaded
        field_isloaded = False
        response = remote_socket.send("exit\n");
        remote_socket.close()
        dialog.destroy()
        #messagebox.showinfo("3D view", " closing process..")
        menu0.entryconfig("3D view open", state=tkinter.NORMAL)
        menu0.entryconfig("3D view close", state=tkinter.DISABLED)
        menu1.entryconfig("clock", state=tkinter.DISABLED)
        menu1.entryconfig("speed", state=tkinter.DISABLED)
        menu1.entryconfig("camera", state=tkinter.DISABLED)
        menu_button_2.config(state=tkinter.DISABLED)
        menu_button_3.config(state=tkinter.DISABLED)

    def on_exit_no():
        dialog.destroy()

    msg_label = ttk.Label(dialog, text="Comfirmation 3D view close")
    msg_label.place(x=100,y=40)

    ok_button = tkinter.Button(dialog, text="3D view Close", fg="blue", command=on_exit_ok)
    no_button = tkinter.Button(dialog, text="NO", fg="red", command=on_exit_no)    
    ok_button.place(x=50,y=80)
    no_button.place(x=250,y=80)

    dialog.grab_set()
    root_tk.wait_window(dialog)

def clock_dialog():
    dialog = tkinter.Toplevel(root_tk)
    dialog.attributes('-topmost', True)
    dialog.geometry("300x100")
    dialog.title("Clock")

    timestr = remote_socket.send("clock get time\n");
    if len(timestr) > 3 and timestr[0] == "Geoglyph:clock":
        timestr2 = timestr[3].split(":")
    else:
        dialog.destroy()
        return
    
    def set_selected_time():
        hour = hour_spinbox.get()
        minute = minute_spinbox.get()
        message = "clock set time " + hour + ":" + minute + "\n"
        response = remote_socket.send(message);
    def close_selected_time():
        dialog.destroy()

    # Hour Spinbox
    hour_spinbox = ttk.Spinbox(dialog, from_=0, to=23, wrap=True, width=8)
    hour_spinbox.set(timestr2[0]) # Initial value
    hour_spinbox.grid(row=0, column=0, padx=5, pady=5)
    hour_label = ttk.Label(dialog, text="hour")
    hour_label.grid(row=0, column=1, padx=5, pady=5)

    # Minute Spinbox
    minute_spinbox = ttk.Spinbox(dialog, from_=0, to=59, wrap=True, width=5)
    minute_spinbox.set(timestr2[1]) # Initial value    
    minute_spinbox.grid(row=0, column=2, padx=5, pady=5)
    minute_label = ttk.Label(dialog, text="min")
    minute_label.grid(row=0, column=3, padx=5, pady=5)

    # Get Time Button
    ok_button = tkinter.Button(dialog, text="Set Time", fg="blue", command=set_selected_time)
    ok_button.grid(row=1, column=0, columnspan=2, pady=10)
    close_button = tkinter.Button(dialog, text="Close", fg="red", command=close_selected_time)
    close_button.grid(row=1, column=2, columnspan=2, pady=10)
    #dialog.grab_set()
    #root_tk.wait_window(dialog)

def speed_dialog():
    dialog = tkinter.Toplevel(root_tk)
    dialog.attributes('-topmost', True)
    dialog.geometry("300x180")
    dialog.title("Speed")

    speedstr = remote_socket.send("clock get speed\n");
    if len(speedstr) > 3 and speedstr[0] == "Geoglyph:clock":
        speed = float(speedstr[3])
    else:
        dialog.destroy()
        return
    
    def set_selected_speed():
        message = "clock set speed "
        labeltxt = "x "
        if selected_value.get() == "fast":
            message += str(scaleF.get()) + "\n"
            labeltxt += str(scaleF.get())
        else:
            message += str(scaleS.get()) + "\n"
            labeltxt += str(scaleS.get())
        response = remote_socket.send(message);            
        speedlabel_var.set(labeltxt)
    def close_selected_speed():
        dialog.destroy()

    selected_value = tkinter.StringVar()
    if speed < 1.0:
        selected_value.set("slow")
    else:
        selected_value.set("fast")
    radioF = tkinter.Radiobutton(dialog, text="Fast", variable=selected_value, value="fast")
    radioS = tkinter.Radiobutton(dialog, text="Slow", variable=selected_value, value="slow")    
    radioF.grid(row=0, column=0, padx=5, pady=5)
    radioS.grid(row=1, column=0, padx=5, pady=5)    

    scale_varF = tkinter.DoubleVar()
    scale_varS = tkinter.DoubleVar()
    if speed > 0.99:
        scale_varF.set(speed)
        scale_varS.set(0.1)
    else:
        scale_varF.set(1.0)
        scale_varS.set(speed)

    scaleF = tkinter.Scale(dialog, from_=1.0, to_=12.0, length=200, resolution=1, orient=tkinter.HORIZONTAL, variable=scale_varF , showvalue=True)
    scaleF.grid(row=0, column=1, columnspan=2, padx=5, pady=5)
    scaleS = tkinter.Scale(dialog, from_=0.1, to_=1.0, length=200, resolution=0.1, orient=tkinter.HORIZONTAL, variable=scale_varS , showvalue=True)
    scaleS.grid(row=1, column=1, columnspan=2, padx=5, pady=5)

    ok_button = tkinter.Button(dialog, text="Set speed", fg="blue", command=set_selected_speed)
    ok_button.grid(row=2, column=0, columnspan=2, pady=10)
    close_button = tkinter.Button(dialog, text="Close",  fg="red", command=close_selected_speed)
    close_button.grid(row=2, column=2, columnspan=2, pady=10)
    #dialog.grab_set()
    #root_tk.wait_window(dialog)

def manual_input_dialog():
    dialog = tkinter.Toplevel(root_tk)
    dialog.geometry("500x120")
    dialog.title("Command manual input")

    cmdlbl = tkinter.Label(dialog,text='Command')
    cmdlbl.place(x=30, y=10)

    cmdlbl = tkinter.Label(dialog,text='Result')
    cmdlbl.place(x=30, y=40)

    cmdtxt = tkinter.Entry(dialog,width=40)
    cmdtxt.place(x=100, y=10)
    
    cmdret = tkinter.Entry(dialog,width=40)
    cmdret.place(x=100, y=40)
    
    def on_button_ok():
        cmdret.delete(0, tkinter.END)
        response = remote_socket.send( cmdtxt.get() );
        cmdret.insert(0, " ".join(response) )
        cmdtxt.delete(0, tkinter.END)

    def on_button_no():
        dialog.destroy()
    
    ok_button = tkinter.Button(dialog, text="  OK  ",  fg="blue", command=on_button_ok)
    no_button = tkinter.Button(dialog, text="Close",  fg="red", command=on_button_no)    
    ok_button.place(x=50,y=70)
    no_button.place(x=200,y=70)

def camera_add_dialog():
    global shm_mode
    dialog = tkinter.Toplevel(root_tk)
    dialog.geometry("400x260")
    dialog.title("New Camera add")

    lbl = tkinter.Label(dialog,text='Name')
    lbl.place(x=30, y=10)

    txt = tkinter.Entry(dialog,width=20)
    txt.place(x=200, y=10)
    defaultname = "CAMERA" + str(len(windows));
    txt.insert(0,defaultname)

    # size ratio slider
    s_scale_var = tkinter.DoubleVar()
    s_scale = tkinter.Scale(dialog, from_=0.1, to_=1.0, length=200, resolution=0.1, orient=tkinter.HORIZONTAL, variable=s_scale_var , showvalue=True)
    s_scale.place(x=150, y=40)
    s_label = ttk.Label(dialog, text="size ratio")
    s_label.place(x=50, y=40)

    # screenX Spinbox
    x_spinbox = ttk.Spinbox(dialog, from_=0, to=1920, increment=10, wrap=True, width=8)
    x_spinbox.set(0) # Initial value
    x_spinbox.place(x=150, y=100)
    x_label = ttk.Label(dialog, text="screen x")
    x_label.place(x=50, y=100)

    # screenY Spinbox
    y_spinbox = ttk.Spinbox(dialog, from_=0, to=1080, increment=10, wrap=True, width=8)
    y_spinbox.set(0) # Initial value    
    y_spinbox.place(x=150, y=130)
    y_label = ttk.Label(dialog, text="screen y")
    y_label.place(x=50, y=130)

    def on_button_ok():
        cname = txt.get()
        s = s_scale.get()
        x = x_spinbox.get()
        y = y_spinbox.get()
        message = "camera add " + cname + " " + str(x) + " " + str(y) + " " + str(s) + "\n"
        response = remote_socket.send(message);
        dialog.destroy()
        if response[3] != "cannot" and response[3] != "already":
            if shm_mode:
                time.sleep(5) # wait time for Setup 3D Viewer
                message = "shm set camera " + cname + " viewport\n"
                shmmsg = remote_socket.send(message)
                shm_camera[cname] = sysv_ipc.SharedMemory(int(shmmsg[4]))
            windows[cname] = "NONE"

    def on_button_no():
        dialog.destroy()
    
    ok_button = tkinter.Button(dialog, text="Add camera",  fg="blue", command=on_button_ok)
    no_button = tkinter.Button(dialog, text="  Close   ",  fg="red", command=on_button_no)    
    ok_button.place(x=50,y=180)
    no_button.place(x=250,y=180)

def calculate_bearing(pointA, pointB):
    lat1 = math.radians(pointA[0])
    lat2 = math.radians(pointB[0])
    diffLong = math.radians(pointB[1] - pointA[1])
    x = math.sin(diffLong) * math.cos(lat2)
    y = math.cos(lat1) * math.sin(lat2) - (math.sin(lat1) * math.cos(lat2) * math.cos(diffLong))
    initial_bearing = math.atan2(x, y)
    initial_bearing = math.degrees(initial_bearing)
    compass_bearing = (initial_bearing + 360) % 360
    return compass_bearing

def set_camera_tracking_position(cameraid,trainid,distance,height):
    global field_isrunning

    message = "camera get " + cameraid + " position\n"
    cameraret = remote_socket.send(message);
    if len(cameraret) == 7:
        camera_point = ( float(cameraret[5]), float(cameraret[4]) )
        camera_alt = float(cameraret[6])
    else:
        return
    message = "train position " + trainid + "\n"
    trainret = remote_socket.send(message);
    if len(trainret) == 6:
        train_point = ( float(trainret[4]), float(trainret[3]) )
        train_alt = float(trainret[5])
    else:
        return

    tmp_flag = field_isrunning
    if tmp_flag:
        pause_command()
    bearing = calculate_bearing(train_point,camera_point)
    distance_km = distance / 1000
    camera_alt = train_alt + height
    destination = geodesic(kilometers=distance_km).destination((train_point[0], train_point[1]), bearing )
    message = "camera set " + cameraid + " position " + str(destination.longitude) + " " + str(destination.latitude) + " " + str(camera_alt) + "\n"
    result = remote_socket.send(message); 
    if tmp_flag:
        run_command()
    
def camera_tracking_dialog():
    global tracking_ver
    global tracking_distance
    global tracking_height
    
    dialog = tkinter.Toplevel(root_tk)
    dialog.attributes('-topmost', True)
    dialog.geometry("400x100")
    dialog.title("Camera Tracking")

    def set_camera_tracking():
        c_id = ccombo.get()
        t_id = tcombo.get()
        message = "camera set " + c_id + " tracking " + t_id + "\n"
        response = remote_socket.send(message)
        set_camera_tracking_position(c_id,t_id,tracking_distance,tracking_height)
        windows[c_id] = t_id
        dialog.destroy()        
    def close_camera_tracking():
        dialog.destroy()        

    cname_label = ttk.Label(dialog, text="camera ID:")
    cname_label.grid(row=0, column=0, padx=5, pady=5)

    tname_label = ttk.Label(dialog, text="train ID:")
    tname_label.grid(row=1, column=0, padx=5, pady=5)

    c_options = []
    for item in windows:
        c_options.append(item)
    camera_ver = tkinter.StringVar()
    ccombo = ttk.Combobox ( dialog , values = c_options , textvariable = camera_ver , height = 5)
    ccombo.grid(row=0, column=1, columnspan=2, pady=5)

    t_options = []
    for item in markers:
        if item != "sample":
            t_options.append( item )
        else:
            t_options.append("NONE")
    tracking_ver = tkinter.StringVar()
    tcombo = ttk.Combobox ( dialog , values = t_options , textvariable = tracking_ver , height = 5)
    tcombo.grid(row=1, column=1, columnspan=2, pady=5)

    ok_button = tkinter.Button(dialog, text="Camera Tracking ", fg="blue", command=set_camera_tracking)
    ok_button.grid(row=3, column=1, pady=10)
    close_button = tkinter.Button(dialog, text="Close ", fg="red", command=close_camera_tracking)
    close_button.grid(row=3, column=3, pady=10)
    #dialog.grab_set()
    #root_tk.wait_window(dialog)


def run_command():
    global field_isrunning
    field_isrunning = True
    response = remote_socket.send("run\n");
    menu_button_2.config(state=tkinter.DISABLED)
    menu_button_3.config(state=tkinter.NORMAL)

def pause_command():
    global field_isrunningo
    field_isrunning = False
    response = remote_socket.send("pause\n");
    menu_button_2.config(state=tkinter.NORMAL)
    menu_button_3.config(state=tkinter.DISABLED)
    
def app_quit():
    global field_isloaded
    field_isloaded = False
    if polling_thread == None:
        #print(threading.active_count())
        print("exited")
    else:
        polling_thread.join()
    root_tk.quit
    exit()

################################################################################
#
#
#  Custom command
#

    
def custom_auto_tracking_camera():
    global field_isloaded
    global field_isrunning
    global custom_auto_tracking
    global custom_auto_tracking_count
    global custom_auto_tracking_times
    global trainid
    global tracking_distance
    global tracking_height

    if field_isloaded:
        if field_isrunning:
            if custom_auto_tracking:
                if custom_auto_tracking_count > custom_auto_tracking_times:
                    for item in windows:
                        loopcount = 0
                        while True:
                            newid = get_random_train_id()
                            if is_same_tracking_id_in_windows(newid):
                                time.sleep(1)
                            else:
                                message = "camera set " + item + " tracking " + newid + "\n"
                                response = remote_socket.send(message);
                                set_camera_tracking_position(item,newid,tracking_distance,tracking_height)
                                windows[item] = newid
                                break
                            if loopcount > len(trainid):
                                break
                            loopcount += 1
                        time.sleep(2)
                    custom_auto_tracking_count = 0
                else:
                    custom_auto_tracking_count += 1

            
    
    
################################################################################
#
#    Widget
#
################################################################################    
    
# create tkinter window
root_tk = tkinter.Tk()
screen_width = root_tk.winfo_screenwidth()
screen_height = root_tk.winfo_screenheight()
print(f"Resolution: {screen_width}x{screen_height}")
#root_tk.geometry(f"{1000}x{700}")
root_tk.title("simple osgearth_rail controller")

# Custom Menubar
menu_frame = tkinter.Frame(root_tk, bg="lightgray", height=30)
menu_frame.pack(side="top", fill="x")

# add menu item
menu_button_0 = tkinter.Menubutton(menu_frame, text="File", bg="lightgray")
menu_button_0.pack(side="left", padx=10)

# setting menu dropdown
menu0 = tkinter.Menu(menu_button_0, tearoff=0)
menu0.add_command(label="3D view open", command=server_connect_dialog)
menu0.add_command(label="3D view close", command=server_exit_dialog)
menu0.add_separator()
menu0.add_command(label="CPU Load", command=cpuload)
menu0.add_command(label="About", command=about)
menu0.add_command(label="Exit", command=app_quit)
menu_button_0.config(menu=menu0)
menu0.entryconfig("3D view close", state=tkinter.DISABLED)

# add menu item
menu_button_1 = tkinter.Menubutton(menu_frame, text="Command", bg="lightgray")
menu_button_1.pack(side="left", padx=10)

# setting menu dropdown
menu1 = tkinter.Menu(menu_button_1, tearoff=0)
camera_menu = tkinter.Menu(menu1, tearoff=0)
menu1.add_command(label="clock", command=clock_dialog)
menu1.add_command(label="speed", command=speed_dialog)
menu1.add_cascade(label="camera", menu=camera_menu)
menu1.add_command(label="manual input", command=manual_input_dialog)
menu1.add_command(label="4K Multiview Script", command=server_multiview_script)

camera_menu.add_command(label="tracking", command=camera_tracking_dialog)
camera_menu.add_command(label="New(add)", command=camera_add_dialog)

menu_button_1.config(menu=menu1)
menu1.entryconfig("clock", state=tkinter.DISABLED)
menu1.entryconfig("speed", state=tkinter.DISABLED)
menu1.entryconfig("camera", state=tkinter.DISABLED)
menu1.entryconfig("manual input", state=tkinter.DISABLED)

# add menu item
menu_button_2 = tkinter.Button(menu_frame, text="Run", bg="lightgray", fg="blue", command=run_command)
menu_button_2.pack(side="left", padx=20)
menu_button_2.config(state=tkinter.DISABLED)

menu_button_3 = tkinter.Button(menu_frame, text="Pause", bg="lightgray", fg="red", command=pause_command)
menu_button_3.pack(side="left", padx=20)
menu_button_3.config(state=tkinter.DISABLED)

# add menu item
timelabel_var = tkinter.StringVar()
timelabel_var.set("00:00")
timelabel = tkinter.Label(menu_frame,textvariable=timelabel_var, bg="lightgray")
timelabel.pack(side="left", padx=10)

# add menu item
speedlabel_var = tkinter.StringVar()
speedlabel_var.set("x 1.0")
speedlabel = tkinter.Label(menu_frame,textvariable=speedlabel_var, bg="lightgray")
speedlabel.pack(side="left", padx=10)

#
# create map widget
#
map_widget = tkintermapview.TkinterMapView(root_tk, width=main_window_width, height=main_window_height, corner_radius=0)
map_widget.pack(fill="both", expand=True)

map_widget.set_zoom(2)

markers = { "sample" : map_widget.set_marker( 0, 0, text="sample") }
windows = { "root" : "NONE" }

root_tk.mainloop()
