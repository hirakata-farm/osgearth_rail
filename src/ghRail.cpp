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
ghRail::GetConfigure()
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
ghRail::SetClockSpeed(double sp)
{
  // Speed range
  if ( sp < p_min_clockspeed ) {
    p_clockspeed = p_min_clockspeed;
  } else if ( sp > p_max_clockspeed ) {
    p_clockspeed = p_max_clockspeed;
  } else {
    p_clockspeed = sp;
  } 
}
double
ghRail::GetClockSpeed()
{
  return p_clockspeed;
}


void
ghRail::SetClockMaxSpeed(double sp)
{
  if ( sp < GH_DEFAULT_MAX_CLOCK_SPEED ) p_max_clockspeed = GH_DEFAULT_MAX_CLOCK_SPEED;
  p_max_clockspeed = sp;
}

double
ghRail::GetClockMaxSpeed()
{
  return p_max_clockspeed;
}

void
ghRail::SetAltmode(int mode)
{
  p_altmode = mode;
}

int
ghRail::GetAltmode()
{
  return p_altmode;
}

void
ghRail::SetDisplayDistance(double distance)
{
  if ( distance < GH_MIN_DISPLAY_DISTANCE ) distance = GH_MIN_DISPLAY_DISTANCE;
  p_displaydistance = distance;
}

double
ghRail::GetDisplayDistance()
{
  return p_displaydistance;
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

  if ( trainid.empty() ) {
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

  if ( trainid.empty() ) {
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
ghRail::GetTrainIcon(string trainid)
{
  std::string ret = " ";
  if ( p_units.count(trainid) < 1 ) {
    // No Units
    ret += "Not found";
  } else {
    ret += p_units[trainid].GetMarkerUri();
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
ghRail::Setup(string configname)
{

  //
  //  Configure Data
  //
  //ghRailJSON p_config;
  p_config.SetConfigUri(GEOGLYPH_ROOT_URI,configname);
  if(! p_config.GetContent())
    {
      return GH_SETUP_RESULT_NOT_GET_CONFIG;
    }
  std::string fieldname = p_config.GetJsonString("field","base");

  //
  //   Field Data
  //
  p_field.SetFieldUri(GEOGLYPH_ROOT_URI,fieldname);
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
  //  Default icon
  //
  nlohmann::json ticon = p_field.GetJsonObject("marker");
  p_default_icon = ticon.get<std::string>();

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
    p_units[trainid2].Setup( trainid2,
			     p_default_icon,
			     p_default_locomotive,
			     tunit,
			     p_route[routename2] );
    p_units[trainid2].SimulatePath( &p_time );
    
  }

  //
  //  Params
  //
  p_configure = configname;
  p_running = false;
  p_clockspeed = 1.0;
  p_tracking = GH_COMMAND_CAMERA_UNTRACKING;
  p_prev_position_tracking = osg::Vec3d(0.0,0.0,0.0); 
  p_previous_simulationTime = 0.0;
  p_max_clockspeed = GH_DEFAULT_MAX_CLOCK_SPEED;
  p_min_clockspeed = GH_DEFAULT_MIN_CLOCK_SPEED;
  p_altmode = GH_DEFAULT_ALTMODE;
  p_displaydistance = GH_DEFAULT_DISPLAY_DISTANCE;
  for (int i = 0; i < GH_SHM_COUNT; i++) {
    p_shm[i].key = -1;
  }

  
  return GH_SETUP_RESULT_OK;
}

////////////////////////////////////////
//

void
ghRail::Update( double simulationTime, osgEarth::MapNode* _map ,  osgViewer::Viewer* _view )
{

  osg::AnimationPath::ControlPoint point;
  osg::Vec3d position_centric;
  osg::Vec3d position_tracking = osg::Vec3d(0.0,0.0,0.0);

  double timediff = simulationTime - p_previous_simulationTime; // [sec]
  if ( GH_THRESHOLD_TIME_BACK_SEC < timediff && timediff < 0 ) {
    // Time is going back just a little
    //std::cout << "warning simulation time " << to_string(timediff) << std::endl;
    return;
  }
  
  osg::Vec3d eye, up, center;
  _view->getCamera()->getViewMatrixAsLookAt( eye, center, up );
  double distance = 0.0;

  _updateShmClockTime(simulationTime);
  
  //
  //
  //  Train Position (quatanion) Update
  //
  //
  int traincnt = 0;
  for (const auto& [key, value] : p_units) {
    int unitsize = p_units[key].GetLocomotiveModelSize();
    
    for (int i = 0; i < unitsize; i++) {

      point = p_units[key].GetControlPoint(simulationTime,i);
      position_centric = point.getPosition();
      //if ( i == 0 && ! key.empty() ) {
      if ( i == 0 && ! key.empty() ) {
	_updateShmTrainPosition(traincnt,key,position_centric);
	//std::cout << "train count " << traincnt << std::endl;
	traincnt++;
      }
  
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
	    trans->setPosition( _calcGeoPoint( _map->getMapSRS()->getGeographicSRS() , position_centric , key, simulationTime , i ) );
	    osg::PositionAttitudeTransform *attitude = p_units[key].GetModelAttitude(i);
	    attitude->setAttitude( point.getRotation() );
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
	  if ( distance > p_displaydistance ) {
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
	      trans->setPosition( _calcGeoPoint( _map->getMapSRS()->getGeographicSRS() , position_centric , key, simulationTime , i ) );
	      osg::PositionAttitudeTransform *attitude = p_units[key].GetModelAttitude(i);
	      attitude->setAttitude( point.getRotation() );
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

  _updateShmCameraViewport(_view);

  
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

int 
ghRail::InitShm(int shmkey,int shmtype) {

  p_shm[shmtype].key = shmkey;
  p_shm[shmtype].type = shmtype;
  
  if ( shmtype == GH_SHM_TYPE_CLOCK_TIME ) {
    // sizeof int = 4
    p_shm[shmtype].size = sizeof(int);
  } else if ( shmtype == GH_SHM_TYPE_TRAIN_POSITION ) {
    int num = 0;
    for (const auto& [key, value] : p_units) {
      if ( ! key.empty() ) {
	num ++;
      }
    }
    p_shm[shmtype].size = sizeof(GH_SHM_TrainPosition)*num;
  } else if ( shmtype == GH_SHM_TYPE_CAMERA_VIEWPORT ) {
    // sizeof double 8 * 24 = 192
    p_shm[shmtype].size = sizeof(double)*2*12;
  } else {
    p_shm[shmtype].key = -1;
    return -1;
  }
  /////////////////
  
  if ((p_shm[shmtype].shmid = shmget(p_shm[shmtype].key, p_shm[shmtype].size, IPC_CREAT | 0666)) < 0) {
    //printf("Error getting shared memory id");
    p_shm[shmtype].key = -1;
    return -1;
  }
  // Attached shared memory
  if ((p_shm[shmtype].addr = (char *)shmat(p_shm[shmtype].shmid, (void *)0, 0)) == (char *) -1) {
    //printf("Error attaching shared memory id");
    p_shm[shmtype].key = -1;
    return -1;
  }

  return p_shm[shmtype].size;
  
}


int 
ghRail::RemoveShm(int shmkey) {

  for (int i = 0; i < GH_SHM_COUNT; i++) {
    if ( p_shm[i].key = shmkey || shmkey == 0 ) {
      // Detach shmkey
      shmdt( p_shm[i].addr );
      shmctl(  p_shm[i].shmid, IPC_RMID, NULL);
      p_shm[i].key = -1;
    } else {
      // NOP
    }
  }

  return -1;
}


osgEarth::GeoPoint
ghRail::_calcGeoPoint( const osgEarth::SpatialReference* srs, osg::Vec3d position , std::string key, double simtime, int coach) {
  osgEarth::Ellipsoid WGS84;
  osg::Vec3d position_lnglat;
  osgEarth::GeoPoint geopoint;
  double layerunit = GH_ALTMODE_LAYER_UNIT;

  position_lnglat = WGS84.geocentricToGeodetic(position);

  if ( p_altmode == GH_ALTMODE_RELATIVE ) {
    osg::Vec3d pos = p_units[key].GetControlPointLayer(simtime,coach);
    if ( pos.z() > 0 ) pos.z() = 0.0;
    geopoint = osgEarth::GeoPoint(srs, position_lnglat.x(), position_lnglat.y(), pos.z()*layerunit , osgEarth::ALTMODE_RELATIVE );
  } else if ( p_altmode == GH_ALTMODE_ABSOLUTE ) {
    geopoint = osgEarth::GeoPoint(srs, position_lnglat.x(), position_lnglat.y(), position_lnglat.z(), osgEarth::ALTMODE_ABSOLUTE );
  } else {
    // GH_ALTMODE_CLAMP or Unknown
    geopoint = osgEarth::GeoPoint(srs, position_lnglat.x(), position_lnglat.y(), 0, osgEarth::ALTMODE_RELATIVE );
  }

  return geopoint;
}

void
ghRail::_updateShmClockTime(double stime) {

  if ( p_shm[GH_SHM_TYPE_CLOCK_TIME].key < 0 ) return;

  double timezone_sec = (double)GetTimeZoneMinutes() * 60.0;
  
  int data;
  data = floor(stime - timezone_sec);
  //data[0] = (int)stime / 3600;
  //data[1] = (int)( stime - data[0] * 3600 ) / 60;
  //data[2] = (int)stime % 60;

  memcpy( p_shm[GH_SHM_TYPE_CLOCK_TIME].addr , &data, sizeof(int) ) ;
}

void
ghRail::_updateShmTrainPosition(int cnt,std::string strtrain,osg::Vec3d position) {

  if ( p_shm[GH_SHM_TYPE_TRAIN_POSITION].key < 0 ) return;

  osgEarth::Ellipsoid WGS84;
  osg::Vec3d position_lnglat = WGS84.geocentricToGeodetic(position);
  GH_SHM_TrainPosition data;

  memset(&data.train[0], 0, sizeof(char)*32);
  int len = strtrain.length();
  if ( len > 31 ) len = 32;
  strtrain.copy(data.train, len);
  data.train[len] = '\0';

  if ( position.x() == 0 && position.y() == 0 ) {
    data.position[0] = -1.0;
    data.position[1] = -1.0;
  } else {
    data.position[0] = position_lnglat.x();
    data.position[1] = position_lnglat.y();
  }

  int offset = sizeof(GH_SHM_TrainPosition) * cnt;

  memcpy( p_shm[GH_SHM_TYPE_TRAIN_POSITION].addr + offset , &data, sizeof(GH_SHM_TrainPosition) ) ;  

}

void
ghRail::_updateShmCameraViewport(osgViewer::Viewer* _view) {

  if ( p_shm[GH_SHM_TYPE_CAMERA_VIEWPORT].key < 0 ) return;

  std::vector<osg::Vec3d> points = _calcCameraViewpoints(_view);
  // points max 12
  double data[2];
  double *ptr = (double *)p_shm[GH_SHM_TYPE_CAMERA_VIEWPORT].addr;
  memset(ptr, 0, sizeof(double)*2*12);
  for (int i = 0; i < points.size(); i++) {
    data[0] = points[i].x();
    data[1] = points[i].y();
    memcpy( ptr , &data[0], sizeof(double)*2 ) ;
    ptr=ptr+2;
  }

}


std::vector<osg::Vec3d>
_calcCameraViewpoints(osgViewer::Viewer* _view) {

  osg::Matrixd proj = _view->getCamera()->getProjectionMatrix();
  osg::Matrixd mv = _view->getCamera()->getViewMatrix();
  osg::Matrixd invmv = osg::Matrixd::inverse( mv );

  double nearPlane = proj(3,2) / (proj(2,2)-1.0);
  double farPlane = proj(3,2) / (1.0+proj(2,2));

  // Get the sides of the near plane.
  double nLeft = nearPlane * (proj(2,0)-1.0) / proj(0,0);
  double nRight = nearPlane * (1.0+proj(2,0)) / proj(0,0);
  double nTop = nearPlane * (1.0+proj(2,1)) / proj(1,1);
  double nBottom = nearPlane * (proj(2,1)-1.0) / proj(1,1);

  // Get the sides of the far plane.
  double fLeft = farPlane * (proj(2,0)-1.0) / proj(0,0);
  double fRight = farPlane * (1.0+proj(2,0)) / proj(0,0);
  double fTop = farPlane * (1.0+proj(2,1)) / proj(1,1);
  double fBottom = farPlane * (proj(2,1)-1.0) / proj(1,1);

  double dist = farPlane - nearPlane;

  //int samples = 24;
  int samples = 4;
  std::vector<osg::Vec3d> verts;
  verts.reserve(samples * 4 - 4);
  for (int i = 0; i < samples - 1; ++i) {
    double j = (double)i / (double)(samples - 1);
    verts.push_back(osg::Vec3d(nLeft + (nRight - nLeft) * j, nBottom, nearPlane));
    verts.push_back(osg::Vec3d(fLeft + (fRight - fLeft) * j, fBottom, farPlane));
  }
  for (int i = 0; i < samples - 1; ++i) {
    double j = (double)i / (double)(samples - 1);
    verts.push_back(osg::Vec3d(nRight, nBottom + (nTop - nBottom) * j, nearPlane));
    verts.push_back(osg::Vec3d(fRight, fBottom + (fTop - fBottom) * j, farPlane));
  }
  for (int i = 0; i < samples - 1; ++i) {
    double j = (double)i / (double)(samples - 1);
    verts.push_back(osg::Vec3d(nRight - (nRight - nLeft) * j, nTop, nearPlane));
    verts.push_back(osg::Vec3d(fRight - (fRight - fLeft) * j, fTop, farPlane));
  }
  for (int i = 0; i < samples - 1; ++i) {
    double j = (double)i / (double)(samples - 1);
    verts.push_back(osg::Vec3d(nLeft, nTop - (nTop - nBottom) * j, nearPlane));
    verts.push_back(osg::Vec3d(fLeft, fTop - (fTop - fBottom) * j, farPlane));
  }

  const auto* wgs84 = osgEarth::SpatialReference::create("epsg:4326");
  auto& ellip = wgs84->getEllipsoid();

  std::vector<osg::Vec3d> points;
  points.reserve(verts.size() / 2);

  auto ecef = wgs84->getGeocentricSRS();

  for (int i = 0; i < verts.size() / 2; ++i)
  {
    osg::Vec3d p1 = verts[i * 2] * invmv;
    osg::Vec3d p2 = verts[i * 2 + 1] * invmv;
    osg::Vec3d out;

    if (ellip.intersectGeocentricLine(p1, p2, out))
    {
      ecef->transform(out, wgs84, out);
      points.push_back(out);
    }
  }

  // points max 12
  return points;
}
