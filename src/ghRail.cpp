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

# include <iostream>
# include <map>
# include <curlpp/cURLpp.hpp>
# include <curlpp/Easy.hpp>
# include <curlpp/Options.hpp>

# include <nlohmann/json.hpp>

# include "ghRail.hpp"

using namespace std;
using namespace cURLpp::Options;

bool
ghRail::IsLoaded()
{
  //std::cout << "conf:" << p_configure << std::endl;
  if ( p_configure == "" ) {
    return false;
  } else {
    return true;
  }
  
}

string
ghRail::GetConf()
{
  return p_configure;
}

bool
ghRail::IsPlaying()
{
  return p_running;
}

void
ghRail::SetPlayPause(bool flag)
{
  p_running = flag;
}

void
ghRail::SetSpeed(double sp)
{
  // Speed range
  if ( sp < GH_MIN_CLOCK_SPEED ) {
    // NOP
  } else if ( sp > GH_MAX_CLOCK_SPEED ) {
    // NOP
  } else {
    p_speed = sp;
  } 
}

double
ghRail::GetSpeed()
{
  return p_speed;
}

string
ghRail::GetUnits()
{
  std::string ret = " ";
  for (const auto& [key, value] : p_units) {
    ret += " ";
    ret += key;
  }
  return ret;
}

string
ghRail::GetTimezoneStr()
{
  nlohmann::json timezone = p_field.GetJsonObject("timezone");
  return timezone.get<std::string>();
}

string
ghRail::GetDescription()
{
  nlohmann::json desc = p_field.GetJsonObject("description");
  return desc.get<std::string>();
}

string
ghRail::SetTrainLabel(string trainid, bool flag)
{
  
  std::string ret = "trainid:";

  if ( trainid == "ALL" || trainid == "all" ) {
    for (const auto& [key, value] : p_units) {
      p_units[key].SetModelLabel(flag);
    }
    if ( flag ) {
      ret += " all label ON";
    } else {
      ret += " all label OFF";
    }
  } else {
    ret += trainid;
    ret += " ";
    if ( p_units.count(trainid) < 1 ) {
      // No Units
      ret += "Not found";
    } else {
      p_units[trainid].SetModelLabel(flag);
      if ( flag ) {
	ret += " label ON";
      } else {
	ret += " label OFF";
      }
    }
  }
  return ret;
}

string
ghRail::GetTrainPosition(string trainid, double simtime)
{
  
  std::string ret = " ";
  osg::AnimationPath::ControlPoint point;
  osg::Vec3 position_centric;
  osg::Vec3 position_lnglat;
  osgEarth::Ellipsoid WGS84;
  int coach = 0;

  if ( trainid == "ALL" || trainid == "all" ) {
    for (const auto& [key, value] : p_units) {
      point = p_units[key].GetControlPoint(simtime,coach);
      position_centric = point.getPosition();
      if ( position_centric.x() == 0 && position_centric.y() == 0 ) {
	//ret += "Not running ";
	//ret += std::to_string(simtime);
      } else {
	position_lnglat = WGS84.geocentricToGeodetic(position_centric);
	ret += ",";
	ret += key;
	ret += " ";
	ret += std::to_string(position_lnglat.x());
	ret += " ";
	ret += std::to_string(position_lnglat.y());
	ret += " ";
	ret += std::to_string(position_lnglat.z());
      }
    }
  } else {
    if ( p_units.count(trainid) < 1 ) {
      // No Units
      ret += "Not found";
    } else {
      point = p_units[trainid].GetControlPoint(simtime,coach);
      position_centric = point.getPosition();
      if ( position_centric.x() == 0 && position_centric.y() == 0 ) {
	ret += "Not running ";
	ret += std::to_string(simtime);
      } else {
	position_lnglat = WGS84.geocentricToGeodetic(position_centric);
	ret += std::to_string(position_lnglat.x());
	ret += " ";
	ret += std::to_string(position_lnglat.y());
	ret += " ";
	ret += std::to_string(position_lnglat.z());
      }
    }
  }
  return ret;
}
string
ghRail::GetTrainTimetable(string trainid)
{
  std::string ret = " ";
  if ( p_units.count(trainid) < 1 ) {
    // No Units
    ret += "Not found";
  } else {
    ret += p_units[trainid].GetTimetable();
  }
  return ret;
}

string
ghRail::GetTrackingTrain() {
  return p_tracking;
}

bool
ghRail::SetTrackingTrain(string trainid) {

  if ( p_units.count(trainid) < 1 ) {
    // No Units
    if ( trainid == GH_COMMAND_CAMERA_UNTRACKING ) {
      p_tracking = GH_COMMAND_CAMERA_UNTRACKING;
    } else {
      return false;
    }
  } else {
    p_tracking = trainid;
  }
  return true;

}




int
ghRail::Setup(string conf)
{

  //
  //  Configure Data
  //
  //ghRailJSON p_config;
  p_config.SetConfigUri(GEOGLYPH_ROOT_URI,conf);
  if(! p_config.GetContent())
    {
      return GH_SETUP_RESULT_NOT_GET_CONFIG;
    }
  std::string field = p_config.GetJsonString("field","base");

  //
  //   Field Data
  //
  p_field.SetFieldUri(GEOGLYPH_ROOT_URI,field);
  if( ! p_field.GetContent() )
    {
      return GH_SETUP_RESULT_NOT_GET_FIELD;
    } else {
      // NOP
    }

  
  //
  //https://qiita.com/yohm/items/0f389ba5c5de4e2df9cf
  //
  //  Line data
  //
  nlohmann::json tlines = p_field.GetJsonObject("lines");

  p_numlines = 0;
  for (nlohmann::json::iterator it = tlines.begin(); it != tlines.end(); ++it) {
    std::string lineid(it.key()); //  string = lineid 
	
    p_line[ lineid ].SetLineUri(GEOGLYPH_ROOT_URI,it.value());
    bool res = p_line[ lineid ].GetContent();
    if ( res ) {
	// NOP
    } else {
      return GH_SETUP_RESULT_WRONG_LINE;
    }
    
    std::string baseuri = p_line[ lineid ].GetJsonString("baseuri");

    std::string directionname;
    nlohmann::json tway = p_line[ lineid ].GetJsonObject("way");
    for (nlohmann::json::iterator it2 = tway.begin(); it2 != tway.end(); ++it2) {
      directionname = it2.value()["direction"].get<std::string>();
      directionname = lineid + "_" + directionname;  // directionname = lineid_directionname
      p_station[directionname] = "";                 // p_station[directionname] 
	
      //
      //  Geometry
      //
      nlohmann::json tgeom = it2.value()["geometry"];
      ghRailCSV csvdata[16];
      int j = 0;
      for (nlohmann::json::iterator it3 = tgeom.begin(); it3 != tgeom.end(); ++it3) {
	csvdata[j].SetCsvUrl(GEOGLYPH_ROOT_URI,baseuri,it3.value());
	csvdata[j].GetContent();
	j++;	
      }
      
      //
      //  Route
      //
      nlohmann::json troute = it2.value()["route"];
      for (nlohmann::json::iterator it4 = troute.begin(); it4 != troute.end(); ++it4) {
	std::string routename(it4.key()); //  string = routename
	routename = lineid + "_" + routename;  //  routename = lineid_routename
	
	nlohmann::json tpath = it4.value();
	p_route[routename] = "";               // p_route[routename] 
	for (nlohmann::json::iterator it5 = tpath.begin(); it5 != tpath.end(); ++it5) {
	  int idx = it5.value();
	  p_route[routename] += csvdata[idx].GetCsv();
	}
      }

      p_station[directionname] = it2.value()["stations"];

    }
    p_numlines++;
  }
  //  End of lines
  //
  
  //
  //  Default Locomotive
  //
  nlohmann::json tlocomotive = p_field.GetJsonObject("locomotive");
  p_default_locomotive.SetLocomotiveUri(GEOGLYPH_ROOT_URI, tlocomotive.get<std::string>() );
  if(! p_default_locomotive.GetContent())
    {
      return GH_SETUP_RESULT_NOT_GET_LOCOMOTIVE;
    }
  //vector<int> locomotivesize = p_default_locomotive.GetVectorInt("interval");
  //vector<string> locomotivemodel = p_default_locomotive.GetVectorString("model");

  //
  //  Timezone data
  //
  p_time.Init();
  nlohmann::json timezone = p_field.GetJsonObject("timezone");
  p_time.SetTimeZone( timezone.get<std::string>() );

  //
  //  Unit data
  //
  nlohmann::json tmpunits = p_field.GetJsonObject("units");

  for (nlohmann::json::iterator it7 = tmpunits.begin(); it7 != tmpunits.end(); ++it7) {
    nlohmann::json tunit = *it7;
    std::string lineid2 = tunit["lineid"].get<std::string>();
    std::string route2 = tunit["route"].get<std::string>();
    std::string trainid2 = tunit["trainid"].get<std::string>();
    std::string routename2 = lineid2 + "_" + route2;

    //    p_units[trainid2].Setup( trainid2,
    //			     locomotivesize,
    //			     locomotivemodel,
    //			     tunit,
    //			     p_route[routename2] );
    //    p_units[trainid2].SimulatePath( locomotivemodel.size() , &p_time );
    p_units[trainid2].Setup( trainid2,p_default_locomotive, tunit, p_route[routename2] );
    p_units[trainid2].SimulatePath( &p_time );
    
  }
  p_configure = conf;
  p_running = false;
  p_tracking = GH_COMMAND_CAMERA_UNTRACKING;
  p_prev_position_tracking = osg::Vec3d(0.0,0.0,0.0); 
  p_previous_simulationTime = 0.0;
      
  return GH_SETUP_RESULT_OK;
}

////////////////////////////////////////
//

void
ghRail::Update( double simulationTime, osgEarth::MapNode* _map ,  osgViewer::Viewer* _view )
{

  osg::AnimationPath::ControlPoint point;
  osg::Vec3d position_centric;
  osg::Vec3d position_lnglat;
  osg::Vec3d position_tracking = osg::Vec3d(0.0,0.0,0.0);
  osgEarth::Ellipsoid WGS84;
  osgEarth::GeoPoint geopoint;

  double timediff = simulationTime - p_previous_simulationTime; // [sec]
  if ( -0.2 < timediff && timediff < 0 ) {
    // 60fps = 0.016666 [sec / frame]
    // 30fps = 0.033333 [sec / frame]
    // 15fps = 0.066666 [sec / frame]
    //  5fps = 0.2      [sec / frame]        
    // NOP
    // Time is going back just a little
    //std::cout << "warning simulation time " << to_string(timediff) << std::endl;
    return;
  }
  
  osg::Vec3d eye, up, center;
  _view->getCamera()->getViewMatrixAsLookAt( eye, center, up );
  double distance = 0.0;

  //
  //
  //  Train Position (quatanion) Update
  //
  //
  for (const auto& [key, value] : p_units) {
    int unitsize = p_units[key].GetLocomotiveModelSize();
    
    for (int i = 0; i < unitsize; i++) {

      point = p_units[key].GetControlPoint(simulationTime,i);
      position_centric = point.getPosition();

      if ( position_centric.x() == 0 && position_centric.y() == 0 ) {
	//  Need not display train
	if ( p_units[key].GetModelStatus(i) == GH_MODEL_STATUS_MAPPED ) {
	  osg::Switch *modelswitch = p_units[key].GetModelSwitch(i);
	  osgEarth::GeoTransform *trans = p_units[key].GetModelTransform(i);
	  modelswitch->setChildValue(trans,false);
	  p_units[key].SetModelStatus(i,GH_MODEL_STATUS_REMOVED );
	} else {
	  // NOP
	}
      } else {
	distance = osg::Vec3d(eye - position_centric).length();

	if ( i == 0 ) {
	  //
	  // coach   i == 0
	  //
	  if ( p_units[key].GetModelStatus(i) < GH_MODEL_STATUS_LOADED ) {
	    p_units[key].CreateModelNode(i);
	    osg::Switch *modelswitch = p_units[key].GetModelSwitch(i);
	    osgEarth::GeoTransform *trans = p_units[key].GetModelTransform(i);	    
	    trans->setTerrain(_map->getTerrain());
	    _map->addChild(modelswitch);
	    p_units[key].SetModelStatus(i,GH_MODEL_STATUS_MAPPED );
	  } else {
	    osgEarth::GeoTransform *trans = p_units[key].GetModelTransform(i);	    
	    position_lnglat = WGS84.geocentricToGeodetic(position_centric);
	    if ( distance > GH_DISPLAY_DISTANCE ) {
	      geopoint = osgEarth::GeoPoint(_map->getMapSRS()->getGeographicSRS(), position_lnglat.x(), position_lnglat.y(), position_lnglat.z(), osgEarth::ALTMODE_ABSOLUTE );
	      trans->setPosition(geopoint);
	    } else {
	      geopoint = osgEarth::GeoPoint(_map->getMapSRS()->getGeographicSRS(), position_lnglat.x(), position_lnglat.y(), 0, osgEarth::ALTMODE_RELATIVE );
	      osg::PositionAttitudeTransform *attitude = p_units[key].GetModelAttitude(i);
	      attitude->setAttitude( point.getRotation() );
	      trans->setPosition(geopoint);
	    }
	    if (  p_units[key].GetModelStatus(i) == GH_MODEL_STATUS_REMOVED ) {
	      osg::Switch *modelswitch = p_units[key].GetModelSwitch(i);
	      modelswitch->setChildValue(trans,true);
	      p_units[key].SetModelStatus(i,GH_MODEL_STATUS_MAPPED );
	    }
	  }

	  if ( key == p_tracking ) {
	    position_tracking = position_centric;
	  }

	} else {
	  //
	  // coach   i != 0
	  //
	  if ( distance > GH_DISPLAY_DISTANCE ) {
	    if ( p_units[key].GetModelStatus(i) < GH_MODEL_STATUS_LOADED ) {
	      // NOP
	    } else {
	      osg::Switch *modelswitch = p_units[key].GetModelSwitch(i);
	      osgEarth::GeoTransform *trans = p_units[key].GetModelTransform(i);
	      modelswitch->setChildValue(trans,false);
	      p_units[key].SetModelStatus(i,GH_MODEL_STATUS_REMOVED );	      
	    }
	  } else {
	    if ( p_units[key].GetModelStatus(i) < GH_MODEL_STATUS_LOADED ) {
	      p_units[key].CreateModelNode(i);
	      osg::Switch *modelswitch = p_units[key].GetModelSwitch(i);
	      osgEarth::GeoTransform *trans = p_units[key].GetModelTransform(i);
	      trans->setTerrain(_map->getTerrain());
	      _map->addChild(modelswitch);
	      p_units[key].SetModelStatus(i,GH_MODEL_STATUS_MAPPED );
	    } else {
	      osgEarth::GeoTransform *trans = p_units[key].GetModelTransform(i);
	      position_lnglat = WGS84.geocentricToGeodetic(position_centric);
	      geopoint = osgEarth::GeoPoint(_map->getMapSRS()->getGeographicSRS(), position_lnglat.x(), position_lnglat.y(), 0, osgEarth::ALTMODE_RELATIVE );
	      osg::PositionAttitudeTransform *attitude = p_units[key].GetModelAttitude(i);
	      attitude->setAttitude( point.getRotation() );
	      trans->setPosition(geopoint);
	      if (  p_units[key].GetModelStatus(i) == GH_MODEL_STATUS_REMOVED ) {
		osg::Switch *modelswitch = p_units[key].GetModelSwitch(i);
		modelswitch->setChildValue(trans,true);
		p_units[key].SetModelStatus(i,GH_MODEL_STATUS_MAPPED );
	      }
	    }
	  }
	}
      }
    }

  }


  //
  //
  // Camera tracking update
  //
  //
  if ( position_tracking.x() == 0 && position_tracking.y() == 0 ) {
    // NOP
    p_prev_position_tracking = osg::Vec3d(0.0,0.0,0.0); 
  } else {
    // Camera Position Update
    if ( p_prev_position_tracking.x() == 0 && p_prev_position_tracking.y() == 0 ) {
      // NOP first frame
    } else {

      osg::Vec3d rightvec = osg::Vec3d(p_prev_position_tracking ^ eye);
      osg::Vec3d dirvec = osg::Vec3d(p_prev_position_tracking - eye);
      osg::Vec3d upvec = osg::Vec3d(rightvec ^ dirvec);
      osg::Matrixd mat ;
      mat.makeLookAt( eye + (position_tracking - p_prev_position_tracking) , position_tracking , upvec );
      _view->getCameraManipulator()->setByInverseMatrix(mat);
    }
    p_prev_position_tracking = position_tracking;
  }

  p_previous_simulationTime = simulationTime;
}

int
ghRail::GetTimeZoneMinutes()
{
  return p_time.GetTimeZoneMin();
}

osgEarth::DateTime
ghRail::GetBaseDatetime() {
  return p_time.GetBaseDatetime();
}

