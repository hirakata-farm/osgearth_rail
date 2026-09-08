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
    print(f" shared memory ON ")
else:
    is_shm_library = False
    print(f" shared memory OFF ")
    default_host = socket.gethostbyname(socket.gethostname())
    
######################################################
#
#   Sub functions
#
#
def about():
    messagebox.showinfo("about", "osgearth_rail_gui 0.8.8")

def cpuload():
    global python_plathome
    if python_plathome == "win64":
        subprocess.Popen(["python.exe","cpusage.py"])  # for Windows
    else:
        subprocess.Popen(["python3","cpusage.py"])
        
def sockcmd_initialize(host,port):
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

        
######################################################
#
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
polling_thread_stop_signal = threading.Event()
#############
custom_auto_tracking = False
#custom_auto_tracking_count = 11
#custom_auto_tracking_times = 12
custom_auto_tracking_count = 170
custom_auto_tracking_times = 180
custom_auto_tracking_windows = { "root" : "NONE" }
#############

def isfloat(s):
    try:
        float(s)
    except ValueError:
        return False
    else:
        return True

def setup_shm():
    global shm_time
    global shm_train
    global shm_train_size_byte
    global shm_camera
    global remote_polling_second

    response = sockcmd("shm set clock time\n");
    if response["status"] != 1:
        return False        

    shm_time = shared_memory.SharedMemory(name=response["result"][0])

    response = sockcmd("shm set train position\n")
    if response["status"] != 1:
        return False
    
    shm_train = shared_memory.SharedMemory(name=response["result"][0])
    shm_train_size_byte = int(response["result"][1])

    response = sockcmd("shm set camera root viewport\n")
    if response["status"] != 1:
        return False

    shm_camera[ "root" ] = shared_memory.SharedMemory(name=response["result"][0])
    remote_polling_second = 1
    

def timetable_dialog(id, tablelist):

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
    response = sockcmd("train icon " + tname + "\n")    
    if response["status"] == 1:
        if 'http' in response["result"][0]:
            imagedata = urlopen(response["result"][0])
            imageobject = Image.open(io.BytesIO(imagedata.read()))
            resized_image = imageobject.resize((icon_width, icon_height), Image.Resampling.LANCZOS)
            #image = ImageTk.PhotoImage(data=imagedata.read())
            image = ImageTk.PhotoImage(resized_image)
            markers[tname] = map_widget.set_marker( lat, lng, text=tname, icon=image, command=click_train_marker )
    else:
        print( response )
    
def click_train_marker(marker):
    response = sockcmd("train timetable " + marker.text + "\n")
    if response["status"] == 1:
        train_id = marker.text
        train_timetable = response["result"]
        timetable_dialog( train_id, train_timetable )

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

def is_same_tracking_id_in_windows(trainid):
    global custom_auto_tracking_windows
    
    for item in custom_auto_tracking_windows:
        if trainid == custom_auto_tracking_windows[item]:
            return True
    return False        
                        
    
def update_socket_train():
    global trainid

    response = sockcmd("train position all\n")
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
                    setup_train_marker(train_lst[i],lat,lng)
                existflag = True
                break
        if existflag == False:
            if item in markers:
                markers[item].delete()
                del markers[item]

def update_socket_viewport():
    global viewport_camera

    map_widget.delete_all_polygon()
    for item in windows:
        response = sockcmd("camera get " + item + " viewport\n")
        if response["status"] == 1 and response["size"] > 5:
            data = response["result"]
            view_lng = data[::2]
            view_lat = data[1::2]
            viewsize = len(view_lng)
            positionlist = []
            for i in range(viewsize):
                lng = float(view_lng[i])
                lat = float(view_lat[i])
                positionlist.append((lat,lng))
            viewport_camera[item] = map_widget.set_polygon(positionlist,fill_color=None,outline_color="yellow",border_width=4)


def update_shm_clock():
    global shm_time
    global timelabel_var

    bytedata = bytes(shm_time.buf[:4])
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

    bytedata = bytes(shm_train.buf[:shm_train_size_byte])
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
        bytedata = bytes(shm_camera[item].buf[:192])
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
        
def update_thread_socket(stop_event):
    global field_isloaded
    global field_isrunning
    global polling_socket_wait_second

    while not stop_event.is_set():
        if field_isloaded:
            if field_isrunning:
                response = sockcmd("clock get time\n")
                if response["status"] == 1:
                    timestr = response["result"]
                    timelabel_var.set(timestr[0]+":"+timestr[1])
                else:
                    return
                update_socket_train()
                update_socket_viewport()
                custom_auto_tracking_camera()
                time.sleep(polling_socket_wait_second)
            else:
                time.sleep(1)
        else:
            break

def update_thread_shm(stop_event):
    global field_isloaded
    global field_isrunning
    global polling_shm_wait_second

    while not stop_event.is_set():
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
    global connect_mode
    global polling_thread
    global polling_thread_stop_signal

    response = sockcmd("field set " + fieldid + "\n");
    if response["status"] != 1 :
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
        
    if connect_mode == "shm":
        setup_shm()
        polling_thread = threading.Thread(target=update_thread_shm, args=(polling_thread_stop_signal,))
        print (" use shared memory")
    else:
        polling_thread = threading.Thread(target=update_thread_socket, args=(polling_thread_stop_signal,))
        print (" use socket only")
    field_isloaded = True
    polling_thread.start()
    return True

def server_multiview_script():
    global custom_auto_tracking
    global custom_auto_tracking_windows

    response = sockcmd("clock set time 9:10\n")
    time.sleep(2)
    response = sockcmd("clock set speed 0.0625\n") # 1/16
    time.sleep(2)
    windows["camera1"] = "NONE"
    response = sockcmd("camera add camera1 1280 720 1280 710\n")
    time.sleep(2)
    windows["camera2"] = "NONE"
    response = sockcmd("camera add camera2 2560 720 1280 710\n")
    time.sleep(2)
    windows["camera3"] = "NONE"
    response = sockcmd("camera add camera3 0 1440 2560 710\n")
    time.sleep(2)
    windows["camera4"] = "NONE"
    response = sockcmd("camera add camera4 2560 1440 1280 710\n")
    time.sleep(2)
    run_command()
    custom_auto_tracking = True
    custom_auto_tracking_windows = { "camera1" : "NONE" , "camera2" : "NONE", "camera3" : "NONE", "camera4" : "NONE" }
    menu1.entryconfig("4K Multiview Script", state=tkinter.DISABLED)    

    
#####################################################################


    
def server_connect_dialog():
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

    option11_radio = ttk.Radiobutton(dialog, text="Custom", value="CUSTOM", variable=selected_option)

    option01_radio.place(x=30,y=60)
    option02_radio.place(x=30,y=80)
    option03_radio.place(x=30,y=100)
    option04_radio.place(x=30,y=120)
    option05_radio.place(x=30,y=140)
    option06_radio.place(x=30,y=160)
    option07_radio.place(x=30,y=180)
    option08_radio.place(x=30,y=200)
    option09_radio.place(x=30,y=220)
    option10_radio.place(x=30,y=240)
    
    option11_radio.place(x=30,y=260)        
    tc_txt = tkinter.Entry(dialog,width=20)
    tc_txt.place(x=120, y=260)
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

        response = sockcmd_initialize(remote_host, remote_port)
        if response[1] != "OK":
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
    ok_button.place(x=30,y=320)
    no_button = tkinter.Button(dialog, text="Close", fg="red", command=on_server_no)
    no_button.place(x=200,y=320)


    dialog.grab_set() # modal dialog
    root_tk.wait_window(dialog) # wait for close dialog

def server_exit_close():
    global polling_thread
    global polling_thread_stop_signal
    global field_isloaded
    global connect_mode
    global shm_time
    global shm_train
    global shm_camera
    global polling_socket_wait_second
    
    field_isloaded = False

    if polling_thread == None:
        #print(threading.active_count())
        print("exited")
    else:
        polling_thread_stop_signal.set()
        polling_thread.join(timeout=polling_socket_wait_second)

    if connect_mode == "shm":
        shm_time.close()
        shm_train.close()
        shm_time.unlink()
        shm_train.unlink()
        for item in shm_camera:
            shm_camera[item].close()
            shm_camera[item].unlink()

            
    #sockcmd.CommandExit()
    sockcmd("exit\n")
    #time.sleep(1)
    #sockcmd.CloseSocket()
    #messagebox.showinfo("3D view", " closing process..")
    menu0.entryconfig("3D view open", state=tkinter.NORMAL)
    menu0.entryconfig("3D view close", state=tkinter.DISABLED)
    menu1.entryconfig("clock", state=tkinter.DISABLED)
    menu1.entryconfig("speed", state=tkinter.DISABLED)
    menu1.entryconfig("camera", state=tkinter.DISABLED)
    menu_button_2.config(state=tkinter.DISABLED)
    menu_button_3.config(state=tkinter.DISABLED)

    
def server_exit_dialog():
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

def clock_dialog():
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

def speed_dialog():
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

def manual_input_dialog():
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

def camera_add_dialog():
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


def run_command():
    global field_isrunning
    field_isrunning = True
    response = sockcmd("play\n")
    menu_button_2.config(state=tkinter.DISABLED)
    menu_button_3.config(state=tkinter.NORMAL)

def pause_command():
    global field_isrunning
    field_isrunning = False
    response = sockcmd("pause\n")
    menu_button_2.config(state=tkinter.NORMAL)
    menu_button_3.config(state=tkinter.DISABLED)
    
def app_quit():
    global field_isloaded
    
    #field_isloaded = False
    if field_isloaded:
        server_exit_close()    

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
    global custom_auto_tracking_windows
    global custom_auto_tracking_count
    global custom_auto_tracking_times
    global trainid
    global tracking_distance
    global tracking_height

    if not field_isloaded:
        return
    if not field_isrunning:
        return
    if not custom_auto_tracking:
        return
    
    if custom_auto_tracking_count > custom_auto_tracking_times:
        for item in custom_auto_tracking_windows:
            loopcount = 0
            loopmax = len(trainid) * 20
            while True:
                newid = get_random_train_id()
                if not is_same_tracking_id_in_windows(newid):
                    #response = sockcmd.CommandCameraSetTracking(item,"none")
                    response = sockcmd("camera set " + item + " tracking none\n")
                    if response["status"] == 1:
                        set_camera_tracking_position(item,newid,tracking_distance,tracking_height)
                        windows[item] = newid
                    break
                if loopcount > loopmax:
                    break
                loopcount += 1
            time.sleep(3)
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
print(f"plathome: {python_plathome} {os.name}")
print(f"Resolution: {screen_width}x{screen_height}")
#root_tk.geometry(f"{1000}x{700}")
root_tk.title("osgearth_rail_gui")

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
