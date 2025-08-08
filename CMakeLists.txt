#
#
# https://docs.osgearth.org/en/latest/install.html
#
#
#  depend on https://github.com/nlohmann/json
#
#
# cmake -B build -S .
# cmake --build build
#
#

cmake_minimum_required(VERSION 3.2)

set(osgEarth_DIR /YOURPATH/OSG/dist )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} cmake)

project(osgearth_rail)

find_package( osgEarth REQUIRED )
find_package( curlpp REQUIRED )

link_directories("/YOURPATH/OSG/dist/lib")

add_executable(osgearth_rail src/ghRailTime.cpp src/ghRailUnit.cpp src/ghRailData.cpp src/ghRail.cpp src/ghRailCommand.cpp src/ghMain.cpp)

target_include_directories(osgearth_rail PUBLIC "/YOURPATH/OSG/dist/include")

target_link_libraries(osgearth_rail
	osgEarthImGui
	osgEarth
	osgViewer
	osgGA
	osgDB
	osgText
	osgUtil
	osg
	curl
	curlpp )


