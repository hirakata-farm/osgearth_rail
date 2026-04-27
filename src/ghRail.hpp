/*
*
*  osgearth_rail viewer
*
*     required OpenSceneGraph ( https://github.com/openscenegraph )
*              osgearth       ( https://github.com/gwaldron/osgearth )
*              curlpp         ( https://www.curlpp.org )
*              nlohmann       ( https://github.com/nlohmann/json )
*
*   Copyright (C) 2025,2026 Yuki Osada
*  This software is released under the BSD License, see LICENSE.
*
*
*/


# ifndef _GH_RAIL_HPP_
# define _GH_RAIL_HPP_
     
# include <iostream>
# include <map>
# include <cstdlib>
# include <cstring>
# include <chrono>

#ifdef _WINDOWS
//#include <windows.h>
#else
//# include <sys/types.h>
//# include <sys/ipc.h>
//# include <sys/shm.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#include <osgDB/ReadFile>
#include <osgEarth/MapNode>
#include <osgViewer/Viewer>
#include <osgViewer/CompositeViewer>
#include <osgEarth/EarthManipulator>
#include <osgEarth/TerrainEngineNode>

// view configurations.
//#include <osgViewer/config/SingleWindow>
//#include <osgViewer/config/SingleScreen>

# include <nlohmann/json.hpp>

#include "ghRailData.hpp"
#include "ghRailUnit.hpp"
#include "ghRailTime.hpp"

#define GH_APP_REVISION "0.6.5"
#define GH_APP_NAME "osgearth_rail"
#define GH_WELCOME_MESSAGE "Welcome osgearth_rail viewer"

#define GH_DEFAULT_SOCKET_PORT 57139
#define GH_MAX_SOCKET 3
#define GH_SOCKET_SEND_WAIT 1000000 // 1 sec [microseconds]

#define GH_ELAPSED_THRESHOLD 1.0
#define GH_SUN_AMBIENT 0.03

/////////////////////////////////////////////////////////


#define GH_SETUP_RESULT_OK 0
#define GH_SETUP_RESULT_NOT_GET_CONFIG 8
#define GH_SETUP_RESULT_NOT_GET_FIELD 10
#define GH_SETUP_RESULT_WRONG_LINE 12
#define GH_SETUP_RESULT_NOT_GET_LOCOMOTIVE 14

#define GH_COMMAND_CAMERA_UNTRACKING "none"

#define GH_DEFAULT_MAX_CLOCK_SPEED 12.0
#define GH_DEFAULT_MIN_CLOCK_SPEED 0.1

#define GH_DEFAULT_DISPLAY_DISTANCE 5000.0 // [m]
#define GH_MIN_DISPLAY_DISTANCE      100.0 // [m]

#define GH_DEFAULT_MAX_WINDOW 8
#define GH_DEFAULT_MIN_WINDOW 1

#define GH_ALTMODE_CLAMP 0
#define GH_ALTMODE_RELATIVE 1
#define GH_ALTMODE_ABSOLUTE 2
#define GH_DEFAULT_ALTMODE 0
#define GH_ALTMODE_LAYER_UNIT 9.0


#define GH_TRACKING_SAMPLING_POINTS 3
#define GH_TRACKING_ADJUST_PARAM 0.0004  // smaller the value, it becomes slower. ( > 0 )

#define GH_THRESHOLD_TIME_BACK_SEC -1.0 // [sec]
// 60fps = 0.016666 [sec / frame]
// 30fps = 0.033333 [sec / frame]
// 15fps = 0.066666 [sec / frame]
//  5fps = 0.2      [sec / frame]        

//////////////////////////////////////////////////////////
//#define GH_SHM_PATH  "/tmp/geoglyph3d"
#define GH_SHM_PATH  "/geoglyph3d"
#define GH_SHM_DIGITS  1000000
#define GH_SHM_TYPE_NONE           -1 // int 4 byte :  [sec]
#define GH_SHM_TYPE_CLOCK_TIME      0 // int 4 byte :  [sec]
#define GH_SHM_TYPE_TRAIN_POSITION  1 // (char[32],double[2])x(numoftrains)  48xn byte : [id,lat,lng]
#define GH_SHM_TYPE_CAMERA_VIEWPORT 2 // (double[2])x12  192 byte : [lat,lng]
// sizeof double 8
// sizeof int    4
// sizeof char   1

typedef struct ghSharedMemory
{
  int	key ;
  std::string keyname;
  int	type ;
  int	size;
#ifdef _WINDOWS
  void *shmhandle;
#else
  int	shmid ;
#endif
  char	*addr ;
} ghSharedMemory ;

typedef struct
{
  char train[32];
  double position[2];
} ghShmTrainPosition;


struct ghWindow
{
  osgViewer::View* view;
  std::string tracking;
  ghSharedMemory shm;
};

osgViewer::View* ghCreateView( std::string name , int screenNum , unsigned int x,unsigned int y ,double sizeratio );
void ghSetConfigWindow(ghWindow _win,std::string title,int x,int y,int width,int height);
void ghGetConfigWindow(ghWindow _win,std::string title,int *ret);
int ghInitShmWindow(int shmkey,ghSharedMemory *shm,std::string winname);
std::string ghGetShmWindowKeyname(ghSharedMemory *shm);

//
//
//  ghRail Management Class
//
//
class ghRail
    {
    public:
      void Init();
      int Setup(std::string fieldname);
      //void Update(double simulationTime, osgEarth::MapNode* _map, ghWindow* _win);
      void Update(double simulationTime, osgEarth::MapNode* _map, const std::map<std::string, ghWindow>& _wins);

      std::string GetConfigure();

      double GetClockSpeed();
      void SetClockSpeed(double sp);
      void SetClockMaxSpeed(double sp);
      double GetClockMaxSpeed();
      int GetAltmode();
      void SetAltmode(int mode);
      double GetDisplayDistance();
      void SetDisplayDistance(double distance);
      void SetMaxWindow(int wins);
      int  GetMaxWindow();

      std::string GetUnits();
      std::string GetLines();
      std::string GetTimezoneStr();
      std::string GetDescription();
      bool SetTrainLabel(std::string trainid, bool flag);
      std::string GetTrainPosition(std::string trainid, double simtime);
      std::string GetTrainTimetable(std::string trainid);
      std::string GetTrainIcon(std::string trainid);
      std::string GetTrainLine(std::string trainid);
      std::string GetTrainDistance(std::string trainid);      
      bool IsLoaded();
      bool IsPlaying();
      void SetPlayPause(bool flag);
      int InitShmClock(int shmkey);
      std::string GetShmClockKeyname();
      int InitShmTrain(int shmkey);
      std::string GetShmTrainKeyname();
      //int RemoveShm(int shmkey);
      int RemoveShm(std::string shmkeyname);      
      
      bool IsTrainID(std::string trainid);

      int GetTimeZoneMinutes();
      osgEarth::DateTime GetBaseDatetime();

    private:
      //vector<int> p_locomotives{-84,-60,-36,-12,12,36,60,84};   // 8 coaches Even obsolete
      //vector<int> p_locomotives{-96,-72,-48,-24,0,24,48,72,96};  // 9 coaches Odd obsolete
      std::string p_configure;
      double p_previous_simulationTime;
      std::map<std::string, osg::Vec3d> p_position_tracking;
      std::map<std::string, osg::Vec3d> p_prev_position_tracking;
      bool p_running;
      double p_clockspeed;

      double p_max_clockspeed;
      double p_min_clockspeed;
      int p_altmode;
      double p_displaydistance;
      double p_max_window;
      double p_min_window;

      ghRailTime p_time;
      ghRailJSON p_config;
      ghRailJSON p_field;
      ghRailJSON p_default_locomotive;
      std::string p_default_icon;
      std::map<std::string, ghRailJSON> p_line;
      std::map<std::string, std::string> p_route;
      std::map<std::string, nlohmann::json> p_station;
      int p_numlines;
      std::map<std::string, ghRailUnit> p_units;
      ghSharedMemory p_shm_clock;
      ghSharedMemory p_shm_train;
      
      osgEarth::GeoPoint _calcGeoPoint(const osgEarth::SpatialReference* srs, osg::Vec3d position , std::string key, double simtime, int coach);
      osg::Matrixd _calcTrackingCameraMatrix( osg::Vec3d eye,osg::Vec3d center,osg::Vec3d up,std::string winkey,osgEarth::MapNode* _map);

      void _updateShmClockTime(double stime);
      void _updateShmTrainPosition(int cnt,std::string strtrain,osg::Vec3d position);
      void _updateShmCameraViewport(osgViewer::View* _view,ghSharedMemory shm);
      double _getMinimumDistanceFromCamera(const std::map<std::string, ghWindow>& _wins,osg::Vec3d position);
    };


std::vector<osg::Vec3d> _calcCameraViewpoints(osgViewer::View* _view);

#endif

