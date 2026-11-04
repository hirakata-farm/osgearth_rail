#!/usr/bin/python3
#
#  osgearth_rail_gui
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
# apt-get install python3-tk python3-pip python3-pil python3-pil.imagetk
# pip3 install tkintermapview -t packages
# pip3 install geopy -t packages
#
#
#
import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), 'packages'))

import time
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
import json
import subprocess
import importlib.util

connect_mode = "socket"
is_shm_library = False
python_plathome = "win64"
if os.name == "nt":
    python_plathome = "win64"
else:
    python_plathome = "linux"

default_host = "localhost"

######################################################
#
#  Check for Shared Memory Library
#

def is_module_available(module_name: str) -> bool:
    return importlib.util.find_spec(module_name) is not None

#
# check shared_memory module
#
if is_module_available("multiprocessing"):
    from multiprocessing import shared_memory
    is_shm_library = True
    default_host = "localhost"
    print(f" available shared memory ")
else:
    is_shm_library = False
    print(f" NOT allowed shared memory ")
    default_host = socket.gethostbyname(socket.gethostname())
    
######################################################
#
#   Sub functions
#
#
def app_about():
    messagebox.showinfo("about", "osgearth_rail_gui 0.9.9")

def app_cpuload():
    global python_plathome
    if python_plathome == "win64":
        subprocess.Popen(["python.exe","cpusage.py"])  # for Windows
    else:
        subprocess.Popen(["python3","cpusage.py"])
        
def app_quit():
    global field_isloaded
    
    #field_isloaded = False
    if field_isloaded:
        server_exit_close()    

    root_tk.quit
    exit()

def init_osgearth_rail_command(host,port):
    global python_plathome
    global cmdp
    if python_plathome == "win64":
        cmdp = subprocess.Popen(
            ["python.exe","osgearth_rail_command.py","-t",host,"-p",str(port),"-m","json"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1 )
    else:
        cmdp = subprocess.Popen(
            ["./osgearth_rail_command.py","-t",host,"-p",str(port),"-m","json"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1 )
        
    output0 = cmdp.stdout.readline()
    output1 = cmdp.stdout.readline()
    output2 = cmdp.stdout.readline()
    return output1.split(" ")

def sockcmd(command):
    global cmdp
    cmdp.stdin.write(command)
    cmdp.stdin.flush()
    output = cmdp.stdout.readline().replace("'", '"')
    #print(output)
    return json.loads(output)

        
def isfloat(s):
    try:
        float(s)
    except ValueError:
        return False
    else:
        return True

######################################################
#
#
#  Global Settings
#
main_window_width = 1024
main_window_height = 768
map_widget_tracking = False
icon_width = 16
icon_height = 16
remote_host = default_host
remote_port = 57139
#socket_buffer_size = 16384
socket_buffer_size = 131072
show_socket_detail = True
#
field_isloaded = False
field_isrunning = False
tracking_distance = 1500   # [m]
tracking_height = 200      # [m]
#############
pollingupdate = None
#############

def setup_field_3dserver(fieldid):
    global root_tk
    global cmdp
    global trainid
    global field_isloaded
    global remote_host
    global connect_mode
    global pollingupdate

    response = sockcmd("field set " + fieldid + "\n");
    if response["status"] != 1 :
        print(response)
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

    response = sockcmd("clock set time 12:00\n")
    time.sleep(3) # wait time for Setup 3D Viewer
    response = sockcmd("config set altmode relative\n")
    time.sleep(3) # wait time for Setup 3D Viewer
    response = sockcmd("field get train\n")
    trainid = response["result"]
    field_isloaded = True
    pollingupdate = PollingUpdate(root_tk,connect_mode,cmdp)
    return True    
    
def get_random_train_id():
    global trainid
    length = len(trainid)
    loop_count = 0
    loop_max = length * 10
    while True:
        idx = int(random.uniform(0, length-1))
        if trainid[idx] in markers:
            return trainid[idx]
        if loop_count > loop_max:
            return trainid[1]
            break
        loop_count += 1

#def is_same_tracking_id_in_windows(trainid):
#    global custom_auto_tracking_windows
#    
#    for item in custom_auto_tracking_windows:
#        if trainid == custom_auto_tracking_windows[item]:
#            return True
#    return False        
                        

#####################################################################
#
#
#  GUI Dialogs
#
#
    
def dialog_train_timetable(id, tablelist):

    dialog = tkinter.Toplevel(root_tk)
    dialog.attributes('-topmost', True)
    dialog.geometry("300x400")
    #dialog.title("Timetable  " + id)
    dialog.title("Timetable")

    tree = ttk.Treeview(dialog)
    tree["columns"] = ("station", "time")
    tree.column("#0", width=0, stretch=tkinter.NO)
    tree.column("station", anchor=tkinter.CENTER, width=100)
    tree.column("time", anchor=tkinter.CENTER, width=80)

    tree.heading("#0", text="", anchor=tkinter.CENTER)
    tree.heading("station", text="Station", anchor=tkinter.CENTER)
    #tree.heading("time", text="Time", anchor=tkinter.CENTER)
    tree.heading("time", text=id, anchor=tkinter.CENTER)

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

    
def click_train_marker(marker):
    response = sockcmd("train timetable " + marker.text + "\n")
    if response["status"] == 1:
        train_id = marker.text
        train_timetable = response["result"]
        dialog_train_timetable( train_id, train_timetable )


def dialog_3dview_open():
    global remost_host
    global connect_mode

    dialog = tkinter.Toplevel(root_tk)
    dialog.geometry("400x400")
    dialog.title("3D view data Load")

    lbl = tkinter.Label(dialog,text='osgearth_rail IP address')
    lbl.place(x=30, y=10)

    host_txt = tkinter.Entry(dialog,width=20)
    host_txt.place(x=200, y=10)
    host_txt.insert(0,remote_host)

    selected_mode = tkinter.StringVar(value=connect_mode)
    mode_radio_socket = ttk.Radiobutton(dialog, text="connect socket only", value="socket", variable=selected_mode)
    mode_radio_shm = ttk.Radiobutton(dialog, text="use shared memory", value="shm", variable=selected_mode)    
    mode_radio_socket.place(x=30,y=30)
    mode_radio_shm.place(x=200,y=30)        

    
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

    option11_radio = ttk.Radiobutton(dialog, text="Geoglyph ID", value="CUSTOM", variable=selected_option)

    option01_radio.place(x=30,y=70)
    option02_radio.place(x=30,y=90)
    option03_radio.place(x=30,y=110)
    option04_radio.place(x=30,y=130)
    option05_radio.place(x=30,y=150)
    option06_radio.place(x=30,y=170)
    option07_radio.place(x=30,y=190)
    option08_radio.place(x=30,y=210)
    option09_radio.place(x=30,y=230)
    option10_radio.place(x=30,y=250)
    
    option11_radio.place(x=30,y=270)        
    tc_txt = tkinter.Entry(dialog,width=20)
    tc_txt.place(x=140, y=270)
    tc_txt.insert(0,"require ID")

    def on_server_ok():
        global remote_host
        global connect_mode
                
        remote_host = host_txt.get()
        custom_code = tc_txt.get()
        fieldid = selected_option.get()
        connect_mode = selected_mode.get()

        if fieldid == "CUSTOM":
            if ' ' in custom_code:
                messagebox.showinfo("Warning", "Invalid custom code")
                return
            if custom_code.isspace():
                messagebox.showinfo("Warning", "Invalid custom code")
                return 
            fieldid = custom_code

        response = init_osgearth_rail_command(remote_host, remote_port)
        if response[1] != "OK":
            dialog.destroy()
            messagebox.showinfo("Warning", "3D viewer Error")
            return
        time.sleep(1) # wait time for Setup 3D Viewer
        status = setup_field_3dserver( fieldid )
        if status == True:
            messagebox.showinfo("Information", "3D viewer setting up OK")
        else:
            messagebox.showinfo("Warning", "3D viewer NOT setup")
        dialog.destroy()

    def on_server_no():
        dialog.destroy()
        
    ok_button = tkinter.Button(dialog, text="Data Load", fg="blue", command=on_server_ok)
    ok_button.place(x=30,y=320)
    no_button = tkinter.Button(dialog, text="Close", fg="red", command=on_server_no)
    no_button.place(x=200,y=320)
    dialog.grab_set() # modal dialog
    root_tk.wait_window(dialog) # wait for close dialog

def server_exit_close():
    global field_isloaded
    global connect_mode
    global pollingupdate
    
    field_isloaded = False
    pollingupdate.close()
    sockcmd("exit\n")
    menu0.entryconfig("3D view open", state=tkinter.NORMAL)
    menu0.entryconfig("3D view close", state=tkinter.DISABLED)
    menu1.entryconfig("clock", state=tkinter.DISABLED)
    menu1.entryconfig("speed", state=tkinter.DISABLED)
    menu1.entryconfig("camera", state=tkinter.DISABLED)
    menu_button_2.config(state=tkinter.DISABLED)
    menu_button_3.config(state=tkinter.DISABLED)

def dialog_3dview_close():
    global field_isloaded
    dialog = tkinter.Toplevel(root_tk)
    dialog.geometry("400x160")
    dialog.title("3D view close")

    def on_exit_ok():
        server_exit_close()
        dialog.destroy()

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

def dialog_clock():
    dialog = tkinter.Toplevel(root_tk)
    dialog.attributes('-topmost', True)
    dialog.geometry("300x100")
    dialog.title("Clock")

    response = sockcmd("clock get time\n")
    if response["status"] != 1:
        dialog.destroy()
        return
    timestr = response["result"]
    
    def set_selected_time():
        hour = hour_spinbox.get()
        minute = minute_spinbox.get()
        response = sockcmd("clock set time " + str(hour) + ":" + str(minute) + "\n")

    def close_selected_time():
        dialog.destroy()

    # Hour Spinbox
    hour_spinbox = ttk.Spinbox(dialog, from_=0, to=23, wrap=True, width=8)
    hour_spinbox.set(timestr[0]) # Initial value
    hour_spinbox.grid(row=0, column=0, padx=5, pady=5)
    hour_label = ttk.Label(dialog, text="hour")
    hour_label.grid(row=0, column=1, padx=5, pady=5)

    # Minute Spinbox
    minute_spinbox = ttk.Spinbox(dialog, from_=0, to=59, wrap=True, width=5)
    minute_spinbox.set(timestr[1]) # Initial value    
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

def dialog_speed():
    dialog = tkinter.Toplevel(root_tk)
    dialog.attributes('-topmost', True)
    dialog.geometry("300x180")
    dialog.title("Speed")

    response = sockcmd("clock get speed\n")
    if response["status"] == 1:
        speed = float(response["result"][0])
    else:
        dialog.destroy()
        return
    def set_selected_speed():
        speed = 0
        labeltxt = "x "
        if selected_value.get() == "fast":
            speed = scaleF.get()
            labeltxt += str(scaleF.get())
        else:
            speed = scaleS.get()
            labeltxt += str(scaleS.get())
        response = sockcmd("clock set speed " + str(speed) + "\n")
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

def dialog_manual_input():
    dialog = tkinter.Toplevel(root_tk)
    dialog.geometry("620x120")
    dialog.title("Command manual input")

    cmdlbl = tkinter.Label(dialog,text='Command')
    cmdlbl.place(x=30, y=10)

    cmdlbl = tkinter.Label(dialog,text='Result')
    cmdlbl.place(x=30, y=40)

    cmdtxt = tkinter.Entry(dialog,width=60)
    cmdtxt.place(x=100, y=10)
    
    cmdret = tkinter.Entry(dialog,width=60)
    cmdret.place(x=100, y=40)
    
    def on_button_ok():
        cmdret.delete(0, tkinter.END)
        response = sockcmd( cmdtxt.get() + "\n");
        msg = ""
        for item in response["result"]:
            if isfloat(item):
                msg += str(item)
            else:
                msg += item
            msg += " "
        cmdret.insert(0, msg )
        #cmdtxt.delete(0, tkinter.END)

    def on_button_no():
        dialog.destroy()
    
    ok_button = tkinter.Button(dialog, text="  OK  ",  fg="blue", command=on_button_ok)
    no_button = tkinter.Button(dialog, text="Close",  fg="red", command=on_button_no)    
    ok_button.place(x=50,y=70)
    no_button.place(x=200,y=70)

def dialog_camera_add():
    global connect_mode
    dialog = tkinter.Toplevel(root_tk)
    dialog.geometry("400x260")
    dialog.title("New Camera add")

    lbl = tkinter.Label(dialog,text='Name')
    lbl.place(x=30, y=10)

    txt = tkinter.Entry(dialog,width=20)
    txt.place(x=200, y=10)
    defaultname = "CAMERA" + str(len(windows));
    txt.insert(0,defaultname)

    # width Spinbox
    w_spinbox = ttk.Spinbox(dialog, from_=640, to=1920, increment=2, wrap=True, width=8)
    w_spinbox.set(1024) # Initial value
    w_spinbox.place(x=150, y=40)
    w_label = ttk.Label(dialog, text="width")
    w_label.place(x=50, y=40)

    # height Spinbox
    h_spinbox = ttk.Spinbox(dialog, from_=480, to=1920, increment=2, wrap=True, width=8)
    h_spinbox.set(768) # Initial value
    h_spinbox.place(x=150, y=70)
    h_label = ttk.Label(dialog, text="height")
    h_label.place(x=50, y=70)

    # screenX Spinbox
    x_spinbox = ttk.Spinbox(dialog, from_=0, to=1920, increment=10, wrap=True, width=8)
    x_spinbox.set(32) # Initial value
    x_spinbox.place(x=150, y=100)
    x_label = ttk.Label(dialog, text="screen x")
    x_label.place(x=50, y=100)

    # screenY Spinbox
    y_spinbox = ttk.Spinbox(dialog, from_=0, to=1080, increment=10, wrap=True, width=8)
    y_spinbox.set(32) # Initial value    
    y_spinbox.place(x=150, y=130)
    y_label = ttk.Label(dialog, text="screen y")
    y_label.place(x=50, y=130)

    def on_button_ok():
        cname = txt.get()
        w = w_spinbox.get()
        h = h_spinbox.get()
        x = x_spinbox.get()
        y = y_spinbox.get()
        response = sockcmd("camera add " + cname + " " + str(x) + " " + str(y) + " " + str(w) + " " + str(h) + "\n")
        dialog.destroy()
        if response["status"] == 1:
            if connect_mode == "shm":
                time.sleep(3) # wait time for Setup 3D Viewer
                response = sockcmd("shm set camera " + cname + " viewport\n")
                if response["status"] == 1:
                    shm_camera[ cname ] = shared_memory.SharedMemory(name=response["result"][0])
                    windows[cname] = "NONE"
                else:
                    print(response["result"][0])

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

    response = sockcmd("camera get " + cameraid + " position\n")
    if response["status"] == 1 and response["size"] == 3:
        camera_point = ( float(response["result"][1]), float(response["result"][0]) )
        camera_alt = float(response["result"][2])
    else:
        return
    response = sockcmd("train position " + trainid + "\n")
    if response["status"] == 1:
        train_point = ( float(response["result"][1]), float(response["result"][0]) )
        train_alt = float(response["result"][2])
    else:
        return
    bearing = calculate_bearing(train_point,camera_point)
    distance_km = distance / 1000
    camera_alt = train_alt + height
    destination = geodesic(kilometers=distance_km).destination((train_point[0], train_point[1]), bearing )
    response = sockcmd("camera set " + cameraid + " position " + str(destination.longitude) + " " + str(destination.latitude) + " " + str(camera_alt) + "\n")
    response = sockcmd("camera set " + cameraid + " tracking " + trainid + "\n")
    
    
def dialog_camera_tracking():
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

        response = sockcmd("camera set " + c_id + " tracking none\n")
        if response["status"] == 1:
            if t_id == "none":
                return
            else:
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
            t_options.append("none")
    tracking_ver = tkinter.StringVar()
    tcombo = ttk.Combobox ( dialog , values = t_options , textvariable = tracking_ver , height = 5)
    tcombo.grid(row=1, column=1, columnspan=2, pady=5)

    ok_button = tkinter.Button(dialog, text="Camera Tracking ", fg="blue", command=set_camera_tracking)
    ok_button.grid(row=3, column=1, pady=10)
    close_button = tkinter.Button(dialog, text="Close ", fg="red", command=close_camera_tracking)
    close_button.grid(row=3, column=3, pady=10)
    #dialog.grab_set()
    #root_tk.wait_window(dialog)


def command_button_run():
    global field_isrunning
    field_isrunning = True
    response = sockcmd("play\n")
    menu_button_2.config(state=tkinter.DISABLED)
    menu_button_3.config(state=tkinter.NORMAL)

def command_button_pause():
    global field_isrunning
    field_isrunning = False
    response = sockcmd("pause\n")
    menu_button_2.config(state=tkinter.NORMAL)
    menu_button_3.config(state=tkinter.DISABLED)
    


def dialog_mapproperty():
    global map_widget_tracking

    dialog = tkinter.Toplevel(root_tk)
    dialog.geometry("300x200")
    dialog.title("Map property")

    map_prop_ontop = tkinter.BooleanVar(value=False)
    if map_widget_tracking:
        map_prop_tracking_center = tkinter.BooleanVar(value=True)
    else:
        map_prop_tracking_center = tkinter.BooleanVar(value=False)

    def close_prop():
        global map_widget_tracking
        if map_prop_ontop.get() :
            root_tk.attributes('-topmost',True)
        else:
            root_tk.attributes('-topmost',False)
        if map_prop_tracking_center.get() :
            map_widget_tracking = True
        else:
            map_widget_tracking = False
        dialog.destroy()

    cb1 = tkinter.Checkbutton(dialog, text="Map always on top", variable=map_prop_ontop)
    cb2 = tkinter.Checkbutton(dialog, text="Map center of tracking camera", variable=map_prop_tracking_center)
    
    cb1.pack(anchor="w", padx=20, pady=5)
    cb2.pack(anchor="w", padx=20, pady=5)

    close_button = tkinter.Button(dialog, text="Close", fg="red", command=close_prop)
    close_button.pack(pady=10)

    
        
################################################################################
#
#   Update Class
#
################################################################################    
    
#default_polling_socket_wait_second = 28
#default_polling_shm_wait_second = 2
class PollingUpdate:
    global field_isloaded
    global field_isrunning
    global timelabel_var
    global markers
    global windows
    global trainid
    global map_widget
    global map_widget_tracking

    def __init__(self, root, mode,cmdp):
        self.root = root
        self.mode = mode
        self.counter = 0        
        self.cmdp = cmdp
        self.shm_time = None
        self.shm_train = None
        self.shm_train_size_byte = 0
        self.shm_camera = { "root" : None }
        self.viewport_camera = {}
        if mode == "shm":
            self.polling = 2
            self.setup_shm()
        else:
            self.polling = 28
        self.update_polling()

    def close(self):
        if self.mode == "shm":
            self.shm_time.close()
            self.shm_train.close()
            self.shm_time.unlink()
            self.shm_train.unlink()
            for item in self.shm_camera:
                self.shm_camera[item].close()
                self.shm_camera[item].unlink()

    def command(self,command):
        self.cmdp.stdin.write(command)
        self.cmdp.stdin.flush()
        output = self.cmdp.stdout.readline().replace("'", '"')
        return json.loads(output)

    def setup_shm(self):
        response = self.command("shm set clock time\n");
        if response["status"] != 1:
            return False        
        self.shm_time = shared_memory.SharedMemory(name=response["result"][0])
        response = self.command("shm set train position\n")
        if response["status"] != 1:
            return False
        self.shm_train = shared_memory.SharedMemory(name=response["result"][0])
        self.shm_train_size_byte = int(response["result"][1])
        response = self.command("shm set camera root viewport\n")
        if response["status"] != 1:
            return False
        self.shm_camera[ "root" ] = shared_memory.SharedMemory(name=response["result"][0])

    def setup_train_marker(self,tname,lat,lng):
        response = self.command("train icon " + tname + "\n")    
        if response["status"] == 1:
            if 'http' in response["result"][0]:
                imagedata = urlopen(response["result"][0])
                imageobject = Image.open(io.BytesIO(imagedata.read()))
                resized_image = imageobject.resize((icon_width, icon_height), Image.Resampling.LANCZOS)
                image = ImageTk.PhotoImage(resized_image)
                markers[tname] = map_widget.set_marker( lat, lng, text=tname, icon=image, command=click_train_marker )
                #markers[tname] = map_widget.set_marker( lat, lng, text=tname, icon=image )                
        else:
            print( response )
        
    def update_clock_shm(self):
        bytedata = bytes(self.shm_time.buf[:4])
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

    def update_train_shm(self):
        bytedata = bytes(self.shm_train.buf[:self.shm_train_size_byte])
        offset = 0
        for icnt in range(0,self.shm_train_size_byte,48):
            if (offset+48) < self.shm_train_size_byte+1:
                byteslice = bytedata[offset:offset+48]
                traindata = struct.unpack('<32s2d', byteslice)
                trainstr = traindata[0].decode('ascii')
                nullpos = trainstr.find('\0')
                trainname = trainstr[0:nullpos]
                if isfloat(traindata[1]):
                    lng = float(traindata[1])
                else:
                    lng = -100
                if isfloat(traindata[2]):
                    lat = float(traindata[2])
                else:
                    lat = -100
                if lat < -90 or lng < -90:
                    # Not running train at this time
                    if trainname in markers:
                        markers[trainname].delete()
                        del markers[trainname]
                else:
                    if trainname in markers:
                        markers[trainname].set_position(lat,lng)
                    else:
                        if len(trainname) > 1:
                            self.setup_train_marker(trainname,lat,lng)
                    offset = offset+48
            else:
                print(f"Something wrong data : {offset} {self.shm_train_size_byte}")


    def update_viewport_shm(self):
        map_widget.delete_all_polygon()
        for item in self.shm_camera:
            bytedata = bytes(self.shm_camera[item].buf[:192])
            double_number = struct.unpack('<24d', bytedata)
            positionlist = []
            center_0 = 0.0
            center_1 = 0.0           
            for i in range(0,24,2):
                if double_number[i+1] != 0.0 or double_number[i] != 0.0:
                    positionlist.append((double_number[i+1],double_number[i]))
                    center_0 = center_0 + double_number[i+1]
                    center_1 = center_1 + double_number[i]
            if len(positionlist) > 3:            
                self.viewport_camera[item] = map_widget.set_polygon(positionlist,fill_color=None,outline_color="yellow",border_width=4)
                if map_widget_tracking:
                    map_widget.set_position(center_0/len(positionlist),center_1/len(positionlist))  

    def update_train_socket(self):
        response = self.command("train position all\n")
        if response["status"] != 1:
            return
        data = response["result"]
        train_lst = data[::4]
        train_lng = data[1::4]
        train_lat = data[2::4]
        trainsize = len(train_lst)
        for item in trainid:
            existflag = False
            for i in range(trainsize):
                if train_lst[i] == item:
                    lng = float(train_lng[i])
                    lat = float(train_lat[i])
                    if train_lst[i] in markers:
                        markers[train_lst[i]].set_position(lat,lng)
                    else:
                        self.setup_train_marker(train_lst[i],lat,lng)
                    existflag = True
                    break
            if existflag == False:
                if item in markers:
                    markers[item].delete()
                    del markers[item]

    def update_viewport_socket(self):
        map_widget.delete_all_polygon()
        for item in windows:
            response = self.command("camera get " + item + " viewport\n")
            if response["status"] == 1 and response["size"] > 5:
                data = response["result"]
                view_lng = data[::2]
                view_lat = data[1::2]
                viewsize = len(view_lng)
                positionlist = []
                center_0 = 0.0
                center_1 = 0.0            
                for i in range(viewsize):
                    lng = float(view_lng[i])
                    lat = float(view_lat[i])
                    center_0 = center_0 + lat
                    center_1 = center_1 + lng
                    positionlist.append((lat,lng))
                self.viewport_camera[item] = map_widget.set_polygon(positionlist,fill_color=None,outline_color="yellow",border_width=4)
                if map_widget_tracking:
                    map_widget.set_position(center_0/len(positionlist),center_1/len(positionlist))

    def update_shm(self):
        if field_isloaded:
            if field_isrunning:
                self.update_clock_shm()
                self.update_train_shm()
                self.update_viewport_shm()
    
    def update_socket(self):
        if field_isloaded:
            if field_isrunning:
                response = self.command("clock get time\n")
                if response["status"] == 1:
                    timestr = response["result"]
                    timelabel_var.set(timestr[0]+":"+timestr[1])
                    self.update_train_socket()
                    self.update_viewport_socket()

                    
    def update_polling(self):
        self.counter += 1
        if self.counter == self.polling:
            if self.mode == "shm":
                self.update_shm()
            else:
                self.update_socket()
            self.counter = 0
                
        self.root.after(1000, self.update_polling)

        
################################################################################
#
#    Widget
#
################################################################################    
    
# create tkinter window
root_tk = tkinter.Tk()
screen_width = root_tk.winfo_screenwidth()
screen_height = root_tk.winfo_screenheight()
print(f"plathome: {python_plathome} {os.name}")
print(f"Resolution: {screen_width}x{screen_height}")
#root_tk.geometry(f"{1000}x{700}")
root_tk.title("osgearth_rail_gui")
#root_tk.attributes('-topmost',True)

    
# Custom Menubar
menu_frame = tkinter.Frame(root_tk, bg="lightgray", height=30)
menu_frame.pack(side="top", fill="x")

# add menu item
menu_button_0 = tkinter.Menubutton(menu_frame, text="File", bg="lightgray")
menu_button_0.pack(side="left", padx=10)

# setting menu dropdown
menu0 = tkinter.Menu(menu_button_0, tearoff=0)
menu0.add_command(label="3D view open", command=dialog_3dview_open)
menu0.add_command(label="3D view close", command=dialog_3dview_close)
menu0.add_separator()
menu0.add_command(label="Map property", command=dialog_mapproperty)
menu0.add_separator()
menu0.add_command(label="CPU Load", command=app_cpuload)
menu0.add_command(label="About", command=app_about)
menu0.add_command(label="Exit", command=app_quit)
menu_button_0.config(menu=menu0)
menu0.entryconfig("3D view close", state=tkinter.DISABLED)

# add menu item
menu_button_1 = tkinter.Menubutton(menu_frame, text="Command", bg="lightgray")
menu_button_1.pack(side="left", padx=10)

# setting menu dropdown
menu1 = tkinter.Menu(menu_button_1, tearoff=0)
camera_submenu = tkinter.Menu(menu1, tearoff=0)
menu1.add_command(label="clock", command=dialog_clock)
menu1.add_command(label="speed", command=dialog_speed)
menu1.add_cascade(label="camera", menu=camera_submenu)
menu1.add_command(label="manual input", command=dialog_manual_input)
#menu1.add_command(label="4K Multiview Script", command=server_multiview_script)

camera_submenu.add_command(label="tracking", command=dialog_camera_tracking)
camera_submenu.add_command(label="New(add)", command=dialog_camera_add)

menu_button_1.config(menu=menu1)
menu1.entryconfig("clock", state=tkinter.DISABLED)
menu1.entryconfig("speed", state=tkinter.DISABLED)
menu1.entryconfig("camera", state=tkinter.DISABLED)
menu1.entryconfig("manual input", state=tkinter.DISABLED)

# add menu item
menu_button_2 = tkinter.Button(menu_frame, text="Run", bg="lightgray", fg="blue", command=command_button_run)
menu_button_2.pack(side="left", padx=20)
menu_button_2.config(state=tkinter.DISABLED)

menu_button_3 = tkinter.Button(menu_frame, text="Pause", bg="lightgray", fg="red", command=command_button_pause)
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
#map_widget.set_tile_server(
#    "https://tile.openstreetmap.fr/osmfr/{z}/{x}/{y}.png",
#    max_zoom=19
#)
#map_widget.set_tile_server(
#    "https://stamen-tiles.a.ssl.fastly.net/toner/{z}/{x}/{y}.png",
#    max_zoom=19
#)
map_widget.set_tile_server(
    "https://tile.opentopomap.org/{z}/{x}/{y}.png",
    max_zoom=19
)
#map_widget.set_tile_server(
#    "https://cartodb-basemaps-a.global.ssl.fastly.net/light_all/{z}/{x}/{y}.png",
#    max_zoom=19
#)


map_widget.pack(fill="both", expand=True)
map_widget.set_zoom(2)

markers = { "sample" : map_widget.set_marker( 0, 0, text="sample") }
windows = { "root" : "NONE" }

root_tk.mainloop()
