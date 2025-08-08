#!/bin/sh

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/YOURPATH/OSG/dist/lib

export DISPLAY=:0

build/osgearth_rail earth/osmrail.earth

#gdb --args build/osgearth_rail  earth/osmrail.earth
##set follow-fork-mode child







