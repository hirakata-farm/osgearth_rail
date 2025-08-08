/*
*
*  osgearth_rail viewer
*
*     required OpenSceneGraph ( https://github.com/openscenegraph )
*              osgearth       ( https://github.com/gwaldron/osgearth )
*              curlpp         ( https://www.curlpp.org )
*              nlohmann       ( https://github.com/nlohmann/json )
*
*   Copyright (C) 2025 Yuki Osada
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

#include <osgDB/ReadFile>
#include <osgEarth/MapNode>
#include <osgViewer/Viewer>
#include <osgEarth/TerrainEngineNode>

# include <nlohmann/json.hpp>

using namespace std;

#include "ghRailData.hpp"
#include "ghRailUnit.hpp"
#include "ghRailTime.hpp"

#define GH_REVISION "0.1"
#define GH_WELCOME_MESSAGE "Welcome osgearth_rail viewer"

#define GH_DEFAULT_SOCKET_PORT 57139
#define GH_MAX_SOCKET 3

/////////////////////////////////////////////////////////


#define GH_SETUP_RESULT_OK 0
#define GH_SETUP_RESULT_NOT_GET_CONFIG 8
#define GH_SETUP_RESULT_NOT_GET_FIELD 10
#define GH_SETUP_RESULT_WRONG_LINE 12
#define GH_SETUP_RESULT_NOT_GET_LOCOMOTIVE 14

#define GH_MAX_CLOCK_SPEED 12
#define GH_MIN_CLOCK_SPEED 0.1

#define GH_COMMAND_CAMERA_UNTRACKING "NONE"

#define GH_DISPLAY_DISTANCE 5000 // [m]


//
//
//  ghRail Management Class
//
//
class ghRail
    {
    public:
      int Setup(string conf);
      string GetConf();
      double GetSpeed();
      string GetUnits();
      string GetTimezoneStr();
      string GetDescription();
      string SetTrainLabel(string trainid, bool flag);
      string GetTrainPosition(string trainid, double simtime);
      string GetTrainTimetable(string trainid);
      bool IsLoaded();
      bool IsPlaying();
      void SetPlayPause(bool flag);
      string GetTrackingTrain();
      bool SetTrackingTrain(string trainid);
      void SetSpeed(double sp);
      void Update(double simulationTime, osgEarth::MapNode* _map, osgViewer::Viewer* _view );
      int GetTimeZoneMinutes();
      osgEarth::DateTime GetBaseDatetime();
    private:
      //vector<int> p_locomotives{-84,-60,-36,-12,12,36,60,84};   // 8 coaches Even obsolete
      //vector<int> p_locomotives{-96,-72,-48,-24,0,24,48,72,96};  // 9 coaches Odd obsolete
      string p_configure;
      string p_tracking;
      osg::Vec3d p_prev_position_tracking;
      bool p_running;
      double p_speed;
      ghRailTime p_time;
      ghRailJSON p_config;
      ghRailJSON p_field;
      ghRailJSON p_default_locomotive;
      std::map<std::string, ghRailJSON> p_line;
      std::map<std::string, std::string> p_route;
      std::map<std::string, nlohmann::json> p_station;
      int p_numlines;
      std::map<std::string, ghRailUnit> p_units;
    };

#endif

