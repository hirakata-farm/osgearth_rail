# osgearth_rail
=================

# Overview

Simple 3D train simulation using osgearth libraries.

# Requirements. install and setup before osgearth_rail build.

  * Open Scene Graph ( https://github.com/openscenegraph )
  * osgearth         ( https://github.com/gwaldron/osgearth )
  * curlpp           ( https://www.curlpp.org )
  * nlohmann         ( https://github.com/nlohmann/json )

# How to build.
  1. Edit CMakeLists.txt for your environment.
  2. configure command %cmake -B build -S .
  3. build command     %cmake --build build

# Getting start.

  1. Edit startup script geoglyph3d.sh for your environment.
  2. Run 3D viewer     %./geoglyph3d.sh

  ## for text command interface using socket library.

see demo video
https://www.youtube.com/watch?v=oatOFEbMzUw


  ## for Graphical user interface using python and tkinter.
Require installing some python packages, see python/simplecontoller.py file.

see demo video
https://www.youtube.com/watch?v=YVDdq4JZ19Y

# simulation data

https://earth.geoglyph.info/

 ## Demo

 ![test image](https://github.com/user-attachments/assets/48d0127e-eba1-42ae-b7e0-64b15cfe6e9a)
 
multi view demo https://www.youtube.com/watch?v=opiigSCtFOg

Thalys simulation from Paris to Brussels using Open Street Map imagery layer. https://www.youtube.com/watch?v=qBoS3elHFFE

Thalys simulation using ArcGIS imagery layer. https://www.youtube.com/watch?v=xtAWG-OCOR4




# License
 
"osgearth_rail" is under [MIT license](https://en.wikipedia.org/wiki/MIT_License).
 
Enjoy virtual trip.
 
Thank you


