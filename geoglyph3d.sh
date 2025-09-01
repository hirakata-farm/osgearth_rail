#!/bin/sh

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/YOURPATH/OSG/dist/lib
export DISPLAY=:0

# for shared memory , defined ghRail.hpp
GFILE=/tmp/geoglyph3d

if [ $# -lt 1 ]; then
  echo "$0 [ start | stop | pid ]"
  exit 1
fi

case "$1" in
  start)
      if [ -e ${GFILE} ]; then
	  rm ${GFILE}
      fi
      build/osgearth_rail earth/osmrail.earth &
      echo $! > ${GFILE}
      ;;
  stop)
      if [ -e ${GFILE} ]; then
	  PID=`cat ${GFILE}`
	  kill -KILL ${PID}
	  rm ${GFILE}
      fi
      ;;
  pid)
      if [ -e ${GFILE} ]; then
	  cat ${GFILE}
      else
          echo "$0 not started"
      fi

      ;;
  *)
      echo "$0 [ start | stop | pid ]"
      exit 1
esac
exit 0

#gdb --args build/osgearth_rail  earth/osmrail.earth
#set follow-fork-mode child

