#!/usr/bin/python3
#
#  simple osgearth_rail controller
#
#   Copyright (C) 2025 Yuki Osada
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
from PIL import ImageTk
from tkinter import Menu,ttk,messagebox

import math
import struct
import sysv_ipc

#################################################################
remote_polling_second = 30
remote_host = "localhost"
remote_port = 57139
socket_buffer_size = 4096
field_isloaded = False
shm_time = None
shm_train = None
shm_train_size_byte = 0
shm_camera = { "root" : None }
viewport_camera = {};
#tracking_id = None

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
             
remote_socket = SocketClient(socket_buffer_size,False)

##################################################################


def about():
    message = "simple osgearth_rail controller 0.3"
    messagebox.showinfo("about",message)

def setup_shm():
    global shm_time
    global shm_train
    global shm_train_size_byte
    global shm_camera
    global remote_polling_second
    shmmsg = remote_socket.send("shm set clock time\n")
    shm_time = sysv_ipc.SharedMemory(int(shmmsg[3]))
    shmmsg = remote_socket.send("shm set train position\n")
    shm_train = sysv_ipc.SharedMemory(int(shmmsg[3]))
    shm_train_size_byte = int(shmmsg[4])
    shmmsg = remote_socket.send("shm set camera root viewport\n")
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

    ok_button = ttk.Button(dialog, text="close", command=on_close_ok)
    ok_button.pack()
    
def click_train_marker(marker):
    message = "train timetable " + marker.text + "\n"
    response = remote_socket.send(message)
    train_id = response[2]
    train_timetable = " ".join(response[3:])
    timetable_dialog( train_id, train_timetable.rstrip(',') )
        
def update_socket_marker(data):
    #global tracking_id
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
                        message = "train icon " + trainname + "\n"
                        msgstr = remote_socket.send(message)
                        if msgstr[2] != "not":
                            imagedata = urlopen(msgstr[3])
                            image = ImageTk.PhotoImage(data=imagedata.read())
                            markers[trainname] = map_widget.set_marker( lat, lng, text=trainname, icon=image, command=click_train_marker )
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
                    #if tracking_id == trainname:
                    #    update_map_center(lat,lng)
                else:
                    if len(trainname) > 1:
                        message = "train icon " + trainname + "\n"
                        msgstr = remote_socket.send(message)
                        if msgstr[2] != "not":
                            imagedata = urlopen(msgstr[3])
                            image = ImageTk.PhotoImage(data=imagedata.read())
                            markers[trainname] = map_widget.set_marker( lat, lng, text=trainname, icon=image, command=click_train_marker )
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
        
def timerproc_socket():
    global field_isloaded
    global polling_timer

    if field_isloaded:
        timestr = remote_socket.send("clock get time\n");
        if len(timestr) > 3 and timestr[0] == "Geoglyph:clock":
            timelabel_var.set(timestr[3])
        response = remote_socket.send("train position all\n");
        update_socket_marker(" ".join(response[3:]))
        response = remote_socket.send("camera get root viewport\n");
        update_socket_viewport(" ".join(response[4:]))
        polling_timer=threading.Timer(remote_polling_second,timerproc_socket)
        polling_timer.start()


def timerproc_shm():
    global field_isloaded
    global polling_timer
    if field_isloaded:
        update_shm_clock()
        update_shm_train()
        update_shm_viewport()

        polling_timer=threading.Timer(remote_polling_second,timerproc_shm)
        polling_timer.start()

        
def server_connect_dialog():
    dialog = tkinter.Toplevel(root_tk)
    dialog.geometry("400x300")
    dialog.title("Connect setting")

    lbl = tkinter.Label(dialog,text='osgearth_rail IP address')
    lbl.place(x=30, y=10)

    txt = tkinter.Entry(dialog,width=20)
    txt.place(x=200, y=10)
    txt.insert(0,"localhost")
    
    selected_option = tkinter.StringVar(value="G175448050EUROTHALYS")

    option1_radio = ttk.Radiobutton(dialog, text="Eurostar and Thalys (UK,France,Belguim,Netherland)", value="G175448050EUROTHALYS", variable=selected_option)
    option2_radio = ttk.Radiobutton(dialog, text="TGV (France)", value="G175351030TGV", variable=selected_option)
    option3_radio = ttk.Radiobutton(dialog, text="ICE (Germany)", value="G175396040ICE", variable=selected_option)
    option4_radio = ttk.Radiobutton(dialog, text="ACELA (USA)", value="G174087320ACELA", variable=selected_option)
    #option5_radio = ttk.Radiobutton(dialog, text="London north area (UK)", value="G175517100LONDON", variable=selected_option)
    option5_radio = ttk.Radiobutton(dialog, text="London GATWICK", value="G175749103GATWICK", variable=selected_option)    
    option6_radio = ttk.Radiobutton(dialog, text="Zurich (CH)", value="G175581101ZURICH", variable=selected_option)
    option7_radio = ttk.Radiobutton(dialog, text="Ireland (IRE)", value="G175656102IRELAND", variable=selected_option)    

    option1_radio.place(x=30,y=40)
    option2_radio.place(x=30,y=60)
    option3_radio.place(x=30,y=80)
    option4_radio.place(x=30,y=100)
    option5_radio.place(x=30,y=120)
    option6_radio.place(x=30,y=140)
    option7_radio.place(x=30,y=160)

    def on_server_ok():
        global trainid
        global field_isloaded
        global remote_host
        remote_host = txt.get();
        response = remote_socket.connect(remote_host, remote_port)
        if response == False:
            dialog.destroy()
            messagebox.showinfo("Warning", "3D viewer Error")
            return

        time.sleep(5) # wait time for Setup 3D Viewer
        message = "field set " + selected_option.get() + "\n"
        fieldresult = remote_socket.send(message);
        if len(fieldresult) > 3:
            dialog.destroy()
            messagebox.showinfo("Warning", "Field load error")
            return
        messagebox.showinfo("receive",message)
        menu0.entryconfig("3D view open", state=tkinter.DISABLED)
        menu0.entryconfig("3D view close", state=tkinter.NORMAL)
        menu1.entryconfig("clock", state=tkinter.NORMAL)
        menu1.entryconfig("speed", state=tkinter.NORMAL)
        menu1.entryconfig("camera", state=tkinter.NORMAL)
        menu_button_2.config(state=tkinter.NORMAL)
        menu_button_3.config(state=tkinter.DISABLED)
        response = remote_socket.send("clock set time 12:00\n")
        response = remote_socket.send("config set altmode relative\n")
        trainid = remote_socket.send("field get train\n")
        del trainid[0]  # Geoglyph header string
        del trainid[0]  # train string
        #print (str(len(trainid)) + " trains")
        if remote_host == "localhost":
            setup_shm()
            print (" shared memory mode")
        else:
            print (" socket mode")
            
        field_isloaded = True
        dialog.destroy()

    ok_button = ttk.Button(dialog, text="OK", command=on_server_ok)
    ok_button.place(x=30,y=250)

    dialog.grab_set() # modal dialog
    root_tk.wait_window(dialog) # wait for close dialog

def server_exit_dialog():
    global field_isloaded
    global polling_timer
    dialog = tkinter.Toplevel(root_tk)
    dialog.geometry("400x200")
    dialog.title("3D view close")

    def on_exit_ok():
        global field_isloaded
        global polling_timer
        response = remote_socket.send("exit\n");
        dialog.destroy()
        #messagebox.showinfo("3D view", " closing process..")
        menu0.entryconfig("3D view open", state=tkinter.NORMAL)
        menu0.entryconfig("3D view close", state=tkinter.DISABLED)
        menu1.entryconfig("clock", state=tkinter.DISABLED)
        menu1.entryconfig("speed", state=tkinter.DISABLED)
        menu1.entryconfig("camera", state=tkinter.DISABLED)
        menu_button_2.config(state=tkinter.DISABLED)
        menu_button_3.config(state=tkinter.DISABLED)
        remote_socket.close()
        field_isloaded = False
        if polling_timer.is_alive():
            polling_timer.cancel()
        
    def on_exit_no():
        #messagebox.showinfo("3D view", " cancel close ")
        dialog.destroy()

    msg_label = ttk.Label(dialog, text="Comfirmation 3D view close")
    msg_label.place(x=100,y=40)

    ok_button = ttk.Button(dialog, text="OK", command=on_exit_ok)
    no_button = ttk.Button(dialog, text="NO", command=on_exit_no)    
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
        #messagebox.showinfo("receive",response)
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
    ok_button = ttk.Button(dialog, text="Set Time", command=set_selected_time)
    ok_button.grid(row=1, column=0, columnspan=2, pady=10)
    close_button = ttk.Button(dialog, text="Close", command=close_selected_time)
    close_button.grid(row=1, column=2, columnspan=2, pady=10)
    #dialog.grab_set()
    #root_tk.wait_window(dialog)

def speed_dialog():
    dialog = tkinter.Toplevel(root_tk)
    dialog.attributes('-topmost', True)
    dialog.geometry("300x200")
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
        #messagebox.showinfo("receive",response)
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

    ok_button = ttk.Button(dialog, text="Set speed", command=set_selected_speed)
    ok_button.grid(row=2, column=0, columnspan=2, pady=10)
    close_button = ttk.Button(dialog, text="Close", command=close_selected_speed)
    close_button.grid(row=2, column=2, columnspan=2, pady=10)
    #dialog.grab_set()
    #root_tk.wait_window(dialog)

def camera_add_dialog():
    dialog = tkinter.Toplevel(root_tk)
    dialog.geometry("400x300")
    dialog.title("New Camera add")

    lbl = tkinter.Label(dialog,text='Name')
    lbl.place(x=30, y=10)

    txt = tkinter.Entry(dialog,width=20)
    txt.place(x=200, y=10)
    defaultname = "CAMERA" + str(len(windows));
    txt.insert(0,defaultname)

    # width Spinbox
    w_spinbox = ttk.Spinbox(dialog, from_=320, to=1020, increment=10, wrap=True, width=8)
    w_spinbox.set(640) # Initial value
    w_spinbox.place(x=150, y=40)
    w_label = ttk.Label(dialog, text="width")
    w_label.place(x=50, y=40)

    # height Spinbox
    h_spinbox = ttk.Spinbox(dialog, from_=240, to=940, increment=10, wrap=True, width=8)
    h_spinbox.set(480) # Initial value    
    h_spinbox.place(x=150, y=70)
    h_label = ttk.Label(dialog, text="height")
    h_label.place(x=50, y=70)

    def on_button_ok():
        cname = txt.get()
        w = w_spinbox.get()
        h = h_spinbox.get()
        #message = "camera add " + cname + " " + w + " " + h + "\n"
        message = "camera add " + cname + "\n"
        response = remote_socket.send(message);
        message = "camera set " + cname + " window " + w + " " + h + "\n"
        response = remote_socket.send(message);
        message = "shm set camera " + cname + " viewport\n"
        shmmsg = remote_socket.send(message)
        shm_camera[cname] = sysv_ipc.SharedMemory(int(shmmsg[4]))
        windows[cname] = "NONE";
        dialog.destroy()

    def on_button_no():
        dialog.destroy()
    
    ok_button = tkinter.Button(dialog, text="  OK  ", command=on_button_ok)
    no_button = tkinter.Button(dialog, text="Close", command=on_button_no)    
    ok_button.place(x=50,y=180)
    no_button.place(x=250,y=180)

    
def camera_tracking_dialog():
    global tracking_ver
    dialog = tkinter.Toplevel(root_tk)
    dialog.attributes('-topmost', True)
    dialog.geometry("400x100")
    dialog.title("Camera Tracking")

    def set_camera_tracking():
        c_id = ccombo.get()
        t_id = tcombo.get()
        message = "camera set " + c_id + " tracking " + t_id + "\n"
        response = remote_socket.send(message);
        windows[c_id] = t_id;
        #messagebox.showinfo("receive",response)
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

    ok_button = ttk.Button(dialog, text="Camera Tracking ", command=set_camera_tracking)
    ok_button.grid(row=3, column=1, pady=10)
    close_button = ttk.Button(dialog, text="Close ", command=close_camera_tracking)
    close_button.grid(row=3, column=3, pady=10)
    #dialog.grab_set()
    #root_tk.wait_window(dialog)


def run_command():
    global remote_host
    response = remote_socket.send("run\n");
    menu_button_2.config(state=tkinter.DISABLED)
    menu_button_3.config(state=tkinter.NORMAL)
    #messagebox.showinfo("receive",response)
    if remote_host == "localhost":
        t=threading.Thread(target=timerproc_shm)
    else:
        t=threading.Thread(target=timerproc_socket)
    t.start()

def pause_command():
    global polling_timer
    response = remote_socket.send("pause\n");
    menu_button_2.config(state=tkinter.NORMAL)
    menu_button_3.config(state=tkinter.DISABLED)
    #messagebox.showinfo("receive",response)
    if polling_timer.is_alive():
        polling_timer.cancel()

################################################################################    
# create tkinter window
root_tk = tkinter.Tk()
root_tk.geometry(f"{1000}x{700}")
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
menu0.add_command(label="About", command=about)
menu0.add_command(label="Exit", command=root_tk.quit)
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

camera_menu.add_command(label="tracking", command=camera_tracking_dialog)
camera_menu.add_command(label="New(add)", command=camera_add_dialog)

menu_button_1.config(menu=menu1)
menu1.entryconfig("clock", state=tkinter.DISABLED)
menu1.entryconfig("speed", state=tkinter.DISABLED)
menu1.entryconfig("camera", state=tkinter.DISABLED)

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
map_widget = tkintermapview.TkinterMapView(root_tk, width=1000, height=700, corner_radius=0)
map_widget.pack(fill="both", expand=True)

map_widget.set_zoom(2)

markers = { "sample" : map_widget.set_marker( 0, 0, text="sample") }
windows = { "root" : "NONE" }

root_tk.mainloop()
