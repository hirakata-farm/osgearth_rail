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

# ifndef _GH_RAIL_UNIT_HPP_
# define _GH_RAIL_UNIT_HPP_
     
# include <iostream>
# include <cstdlib>
# include <cstring>
# include <vector>
# include <chrono>

# include <osg/AnimationPath>
# include <osg/PositionAttitudeTransform>
# include <osgEarth/GeoMath>
# include <osgEarth/GeoTransform>
# include <osgEarth/LabelNode>

#include <osgDB/ReadFile>
#include <osg/Node>
#include <osg/Switch>
#include <osgDB/Registry>
#include <osgDB/Input>
#include <osgDB/Options>

# include <nlohmann/json.hpp>

#include "ghRailData.hpp"
#include "ghRailTime.hpp"

////////////////////////////////////////////////////////
#define GH_TYPE_ARRIVAL 2
#define GH_TYPE_DEPATURE 4
#define GH_TYPE_THROUGH 7
#define GH_ACCL_RATIO_D 0.11
#define GH_ACCL_RATIO_A 0.21
#define GH_STOP_TIME 30 // [sec]

#define GH_UNIT_GEOM_DISTANCE 210    // unit [m] for extend train length target
#define GH_UNIT_TARGET_DISTANCE 200  // unit [m] for extend train length target

#define GH_STATION_TYPE_UNDEFINED 0
#define GH_STATION_TYPE_NORMAL 4
#define GH_STATION_TYPE_NORMAL_HEAD 8
#define GH_STATION_TYPE_NORMAL_TAIL 16

#define GH_STATION_TYPE_THROUGH 32
#define GH_STATION_TYPE_THROUGH_HEAD 64
#define GH_STATION_TYPE_THROUGH_TAIL 128

#define GH_PI 3.14159265358979323846
#define deg2rad(ghdegree) (ghdegree*GH_PI/180)
#define rad2deg(ghradian) (ghradian*180/GH_PI)


#define GH_WGS84_RADIUS_SQUARD_X (6378137.0 * 6378137.0)
#define GH_WGS84_RADIUS_SQUARD_Y (6378137.0 * 6378137.0)
#define GH_WGS84_RADIUS_SQUARD_Z (6356752.3142451793 * 6356752.3142451793)
#define GH_WGS84_RATIO (1.0/298.257223563)

//  1.2 [Degree] = 0.020943951023932 [rad]
//  1.5 [Degree] = 0.026179938779915 [rad]
//  1.8 [Degree] = 0.031415926535898 [rad]
//  2.0 [Degree] = 0.034906585039886 [rad]
#define GH_QUATANION_TILT_MAX 0.026179938779915
#define GH_QUATANION_TILT_MIN -0.026179938779915


#define GH_DEFAULT_LABEL_SIZE 32
#define GH_DEFAULT_LABEL_COLOR "#ffff00"
#define GH_DEFAULT_LABEL_HALO "#4f4f4f"

#define GH_MODEL_STATUS_UNKNOWN 0
#define GH_MODEL_STATUS_INIT 1
#define GH_MODEL_STATUS_LOADED 2
#define GH_MODEL_STATUS_MAPPED 4
#define GH_MODEL_STATUS_REMOVED 8


#define GH_LAYER_DISTANCE_THRESHOLD 1.8

////////////////////////////////////////////////////////


////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
//
// ghRailUnit  Class 
//
class ghRailUnit
    {
    public:
      //void Setup(string id, vector<int> locomotivesize,vector<string> locomotivemodel,nlohmann::json data,string geom);
      void Setup(std::string id, std::string marker, ghRailJSON locomotive, std::string direction , nlohmann::json data,std::string geom);      
      //bool SimulatePath(int coaches, ghRailTime *railtime );
      bool SimulatePath( ghRailTime *railtime );
      osg::AnimationPath::ControlPoint GetControlPoint(double seconds,int coach);
      osg::Vec3d GetControlPointVector(double seconds,int coach);
      std::string GetModelUri(int coach);
      void SetModelLabel(bool flag);
      std::string GetTimetable();
      void CreateModelNode(int coach);
      int GetLocomotiveModelSize();
      osg::Switch *GetModelSwitch(int coach);
      osgEarth::GeoTransform *GetModelTransform(int coach);
      osg::PositionAttitudeTransform *GetModelAttitude(int coach);      
      int GetModelStatus(int coach);
      void SetModelStatus(int coach,int status);
      std::string GetMarkerUri();
      std::string GetLineInfo();
      std::string GetDistanceInfo();
    private:
      std::string p_trainid;
      std::string p_lineid;
      std::string p_routeid;
      std::string p_direction;
      bool p_islabel;
      ghRailJSON p_locomotive;
      std::string p_marker;
      std::vector<int> p_locomotives;
      osgEarth::LabelNode *p_modellabel;
      nlohmann::json p_jsondata;
      std::vector<ghRailModel> p_models;
      std::vector<osg::Switch *> p_switch;
      std::vector<osgEarth::GeoTransform *> p_transform;
      std::vector<osg::PositionAttitudeTransform *> p_attitude;

      std::vector<std::vector<double>> p_geompath;   // row geometry from CSV
      std::vector<std::vector<double>> p_geomlayer;   // row geometry property layer from CSV      
      std::vector<std::string> p_geompathstation;
      std::vector<int> p_geompathstationtype;
      
      int p_geometry_count ;
      std::vector<std::vector<double>> p_geometry;   // formatted geometry
      std::vector<std::string> p_geometrystation;
      std::vector<int> p_geometrystationtype;

      std::vector<osg::AnimationPath*> p_sim; // each coachs
      std::vector<osg::AnimationPath*> p_simlayer; // each coachs for OSM layer property

      void _geompath2geometry(std::vector<int> ranges);
      void _appendGeometryData(double lat,double lon,double alt,std::string name,int type,int type1, int type2);
      std::vector<std::vector<double>> _extendNewPointsForStation(int startidx,int direction,int extend);
      std::vector<std::string> _split(std::string str, std::string delim);
      double _calcObtuseAngle(double angle01, double angle12);
      std::vector<double> _createStationRangePoints(std::vector<std::vector<double>> &geom,int startidx, std::vector<int> ranges);
      std::vector<double> _createDistancePoint(std::vector<std::vector<double>> &geom,int startidx, int distance);

      int _getStationIndex(std::string str,int num);

      bool _simulateCoach(int coachid, ghRailTime *railtime );
      
      osg::Vec3d _calcPositionFromDegrees(double lat, double lng, double alt);
      osg::Vec3d _calcCartesian3FromDegrees(double lat, double lng, double alt);
      osg::Quat _calcQuatanion( int currentid );

      std::vector<double> _simulateStationToStation(int startidx,int stopidx,double distance, double sec, double startsec);
      std::vector<double> _simulateStationToPassing(int startidx,int stopidx,double distance, double sec, double startsec);
      std::vector<double> _simulatePassingToStation(int startidx,int stopidx,double distance, double sec, double startsec);
      std::vector<double> _simulatePassingToPassing(int startidx,int stopidx,double distance, double sec, double startsec);            

      double __simulateTwoPoints(double x,double t, double d,double v);
      void _initLocomotiveArray( std::vector<int>  lsize );
      void _initLocomotiveModel(std::vector<std::string>  locomotive );
      osgEarth::LabelNode *_createLabelNode(std::string text);

      osg::AnimationPath::ControlPoint _calcControlPointGeomLayers(double lat, double lng, osg::Quat quat);
      
};



#endif



