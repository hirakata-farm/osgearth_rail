#!/usr/bin/python3
#
# https://qiita.com/damyarou/items/a7261dbd2d8cd2bc4837
# https://qiita.com/damyarou/items/f4394ad9b88c9b188ac6
#
# pip3 install psutil -t packages
# pip3 install numpy -t packages
#
import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), 'packages'))

import tkinter as tk
from tkinter import ttk
import numpy as np
import psutil

update_periods = 1000

def update():
    global root,lb2,val,mlb
    r=psutil.cpu_percent(interval=1, percpu=True)
    for k in range(0,len(r)):
        lb2[k]['text']='{0:.1f}'.format(r[k])
        val[k]['value']=r[k]
    mlb[1]['text']='{0:6.1f}%'.format(sum(r)/len(r))
    mlb[3]['text']='{0:6.1f}%'.format(psutil.virtual_memory().percent)
    root.after(update_periods, update)

def main():
    global root,lb2,val,mlb

    # basin parameter
    nc=psutil.cpu_count() # number pf cores
    cpu=[]
    for i in range(0,nc):
        s='P'  # performance core
        if 0<=i<=1: s='E' # efficiency core
        cpu.append('{0}-{1}'.format(i+1,s))

    # GUI
    root =tk.Tk()
    root.resizable(False,False)
    root.attributes('-topmost',True)
    root.title('CPU & Mem')
    style=ttk.Style()
    style.theme_use('alt')
    fontn='Calibri 10'
    fontb='Calibri 12 bold'
    style.configure('.',font=(fontn))
    style.configure('.',background='#393939',foreground='#ffffff')
    style.configure('Horizontal.TProgressbar', background='#999999')
    style.configure('Horizontal.TProgressbar', troughcolor='#0f0f0f')
    style.configure("Horizontal.TProgressbar", bordercolor='#393939')

    frame0=ttk.Frame(root)
    lb1=np.empty(nc,dtype=object)
    lb2=np.empty(nc,dtype=object)
    val=np.empty(nc,dtype=object)
    for k in range(0,nc):
        lb1[k]=ttk.Label(frame0,text=cpu[k],borderwidth=0,relief=tk.SOLID,anchor=tk.CENTER,width=4)
        lb2[k]=ttk.Label(frame0,text='',borderwidth=0,relief=tk.SOLID,anchor=tk.CENTER,width=6)
        val[k]=ttk.Progressbar(frame0,orient='horizontal',length=100,maximum=100,mode='determinate')
        lb1[k].grid(row=k,column=0)
        lb2[k].grid(row=k,column=1)
        val[k].grid(row=k,column=2)
    frame0.pack()

    frame1=ttk.Frame(root)
    nn=4
    mlb=np.empty(nn,dtype=object)
    txt=['CPU','','MEM','']
    for k in range(0,len(mlb)):
        mlb[k]=ttk.Label(frame1,text=txt[k],borderwidth=1,relief=tk.SOLID,anchor=tk.CENTER,width=10,font=(fontb))
        i,j=k//2,k%2
        mlb[k].grid(row=i,column=j)
    frame1.pack()

    update()

    root.mainloop()


if __name__ == '__main__': main()


