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


# include "ghString.hpp"
# include "ghRailUnit.hpp"


void
ghRailUnit::Setup( string id,
		   string marker,
		   ghRailJSON locomotive,
		   string direction,
		   nlohmann::json data,
		   string geom ) 
{
  p_trainid = id;
  p_jsondata = data;
  p_lineid = data["lineid"];
  p_routeid = data["route"];
  p_direction = direction;
  p_islabel = true;
  
  //
  //  Parse geometory CSV to vector ( p_geompath )
  //
  int geompathrows;  // path lines
  std::vector<std::string> geomline;
  vector<string> geomstation;
  
  geomline = _split(geom,"\n");

  std::vector<std::string> tmpline;
  std::vector<std::string> propline;
  geompathrows = 0;
  
  for (const auto& token : geomline) {
    //std::cout << token << std::endl;
    tmpline = _split(token,",");
    if ( tmpline[0].compare(0, 1, "#") == 0 ) {
      // Skip Comment
    } else {
      geompathrows++;
    }
  }
  
  p_geompath.resize(geompathrows, vector<double>(4));
  p_geompathstation.resize(geompathrows);
  p_geompathstationtype.resize(geompathrows);

  int idx = 0;
  int idxlayer = 0;
  double distance = 0.0;
  for (const auto& token : geomline) {
    tmpline = _split(token,",");
    if ( tmpline[0].compare(0, 1, "#") == 0 ) {
      // Comment Property
      if ( idx > 0 && tmpline.size() > 1 ) {
	propline = _split(tmpline[1],":");
	for (const auto& token2 : propline) {
	  //  Check layer Property
	  if ( token2.compare(0, 3, "ly=") == 0 ) {
	    p_geomlayer.resize(p_geomlayer.size()+1, vector<double>(3));
	    p_geomlayer[idxlayer][0] = p_geompath[idx-1][0];        // Latitude
	    p_geomlayer[idxlayer][1] = p_geompath[idx-1][1];        // Longitude
	    p_geomlayer[idxlayer][2] = stod( token2.substr(3) );
	    //std::cout << token2 << " " << std::to_string( p_geomlayer[idxlayer][2] ) << std::endl;
	    idxlayer++;
	  }
	}
      } else {
	// NOP
      }
    } else {
      p_geompath[idx][0] = stod(tmpline[0]);        // Latitude
      p_geompath[idx][1] = stod(tmpline[1]);        // Longitude
      p_geompath[idx][2] = stod(tmpline[2]);        // Altitude 
      p_geompath[idx][3] = (double)0.0;             // distance in meter from 0
      p_geompathstation[idx] = tmpline[3];          // Station Name ( name or x )
      if ( tmpline[4] == "B" ) {
	p_geompathstationtype[idx] = GH_STATION_TYPE_NORMAL;      // Station Type ( L, B, D, V
      } else if ( tmpline[4] == "V" ) {
	p_geompathstationtype[idx] = GH_STATION_TYPE_THROUGH;
      } else {
	p_geompathstationtype[idx] = GH_STATION_TYPE_UNDEFINED;
      }

      if ( idx > 0 ) {
	distance = osgEarth::GeoMath::distance(deg2rad(p_geompath[idx-1][0]),deg2rad(p_geompath[idx-1][1]),
					       deg2rad(p_geompath[idx][0]),deg2rad(p_geompath[idx][1]));
	p_geompath[idx][3] = distance + p_geompath[idx-1][3];
      } else {
	// SKIP
      }
      idx++;
    }
  }

  string unit_marker = data["marker"];
  if ( unit_marker == "default" ) {
    p_marker = marker;
  } else {
    p_marker = unit_marker;
  }

  string unit_locomotive = data["locomotive"];
  if ( unit_locomotive == "default" ) {
    p_locomotive = locomotive;
  } else {
    p_locomotive.SetLocomotiveUri(GEOGLYPH_ROOT_URI, unit_locomotive );
    if(! p_locomotive.GetContent())
    {
      //return GH_SETUP_RESULT_NOT_GET_LOCOMOTIVE;
      return;
    }
  }
  vector<int> locomotivesize = p_locomotive.GetVectorInt("interval");
  vector<string> locomotivemodel = p_locomotive.GetVectorString("model");

  //
  //  Expand geompath vector ( station edge )
  //  head and tail station point 
  //
  //  vector<vector<double>> p_geompath -> vector<vector<double>> p_geometry
  //
  //  Train Unit length  ( < GH_UNIT_GEOM_DISTANCE )
  // center
  //    |     |          |          |          |          |
  //               a1         a2         a3         a4
  //
  //   vector<int> ranges{ a1, a2, a3, a4 .....};
  //
  _initLocomotiveArray(locomotivesize);
  _initLocomotiveModel(locomotivemodel);

  _geompath2geometry(p_locomotives);
  
}

osg::AnimationPath::ControlPoint 
ghRailUnit::GetControlPoint( double seconds, int coach )
{
  osg::AnimationPath::ControlPoint result =
    osg::AnimationPath::ControlPoint( osg::Vec3(0,0,0),
				      osg::Quat(0,0,0,0) );
  double firsttime = p_sim[coach]->getFirstTime();
  double lasttime = p_sim[coach]->getLastTime();
  if ( seconds > firsttime && seconds < lasttime ) {
    bool res = p_sim[coach]->getInterpolatedControlPoint( seconds,
							  (osg::AnimationPath::ControlPoint &)result );
  }

  return result;
}

osg::Vec3d
ghRailUnit::GetControlPointVector(double seconds, int coach )
{
  osg::AnimationPath::ControlPoint result =
    osg::AnimationPath::ControlPoint( osg::Vec3(0,0,0),
				      osg::Quat(0,0,0,0) );
  double firsttime = p_sim[coach]->getFirstTime();
  double lasttime = p_sim[coach]->getLastTime();
  bool res;
  if ( seconds > firsttime && seconds < lasttime ) {
    res = p_simlayer[coach]->getInterpolatedControlPoint( seconds,
							  (osg::AnimationPath::ControlPoint &)result );
  }

  if ( res ) {
    return result.getPosition();
  } else {
    return osg::Vec3(0,0,0);
  }

}


//ghRailUnit::simulatepath( int coaches, string timezonestr )
bool
ghRailUnit::SimulatePath( ghRailTime *railtime )
{
  int coaches = p_locomotives.size();
  p_sim.resize(coaches);
  p_simlayer.resize(coaches);

  for (int i = 0; i < coaches; i++) {
    p_sim[i] = new osg::AnimationPath;
    p_sim[i]->setLoopMode(osg::AnimationPath::NO_LOOPING);
    p_simlayer[i] = new osg::AnimationPath;
    p_simlayer[i]->setLoopMode(osg::AnimationPath::NO_LOOPING);
    _simulateCoach( i , railtime );

  }

  return true;
  
}

string
ghRailUnit::GetModelUri(int coach) {
  return p_models[coach].GetUrl();
}

int
ghRailUnit::GetLocomotiveModelSize() {
  return p_locomotive.GetVectorSize("model");
}

void
ghRailUnit::SetModelLabel(bool flag) {
  int coach = 0;
  if ( flag ) {
    if ( p_islabel ) {
      // NOP
    } else {
      if ( p_models[coach].GetStatus() > GH_MODEL_STATUS_INIT ) {
	p_modellabel = _createLabelNode(p_trainid);
	p_transform[coach]->addChild( p_modellabel );
      } else {
	// NOP model not loaded
      }
    }
  } else {
    if ( p_islabel ) {
      if ( p_models[coach].GetStatus() > GH_MODEL_STATUS_INIT ) {
	p_transform[coach]->removeChild( p_modellabel );
	//delete p_modellabel;
	p_modellabel = NULL;
      } else {
	// NOP model not loaded
      }
    } else {
      // NOP
    }
  }
  p_islabel = flag;
}

string
ghRailUnit::GetTimetable() {
  std::string ret = " ";
  nlohmann::json timetable = p_jsondata["timetable"];
  // timetable[i].get<std::string>()     time string    "0T14:54:00"
  // timetable[i+1].get<std::string>()   station string
  // timetable[i+2].get<std::string>()   station type

  int timetable_size = timetable.size();
  for (int i = 0; i < timetable_size; i=i+3) {
    std::string timestr = timetable[i].get<std::string>();
    ret += timestr.substr(2);
    ret += ",";
    ret += timetable[i+1].get<std::string>();
    ret += ",";
  }

  return ret;
}

int
ghRailUnit::GetModelStatus(int coach) {
  return p_models[coach].GetStatus();
}
void
ghRailUnit::SetModelStatus(int coach,int status) {
  p_models[coach].SetStatus(status);
  
}

//osgEarth::GeoTransform *
void
ghRailUnit::CreateModelNode(int coach) {

  ////////////////////////////////////////////////
#ifdef _WINDOWS
  //std::string gltf(p_models[coach].GetGltf());
  //char* cstr = new char[gltf.size() + 1]; // allocation memory
  //std::strcpy(cstr, gltf.c_str());
  //osg::Node* locomotive = osgDB::readNodeFileChar(cstr);
  osg::Node* locomotive = osgDB::readNodeFileChar( ghString2CharPtr( p_models[coach].GetGltf() ) );
  //
  // include/osgDB/ReadFile
  // src/osgDB/ReadFile.cpp string->char
  // include/osgDB/Registry
  // osgearth/osgEarthDriver/gltf/ReaderWriterGLTF.cpp
  //
#else
  osg::Node* locomotive = osgDB::readNodeFile( p_models[coach].GetGltf() );
#endif  
  p_attitude[coach]->setPosition( osg::Vec3(0.0,0.0,0.0) );
  p_attitude[coach]->setScale( osg::Vec3(1.0,1.0,1.0) );
  p_attitude[coach]->addChild( locomotive );
  p_transform[coach]->addChild( p_attitude[coach] );

  ////////////////////////////////////////////////
  //  Train Label ( p_trainid )
  //
  // https://www.packtpub.com/en-us/learning/how-to-tutorials/openscenegraph-advanced-scene-graph-components
  //
  //
  //
  //p_transform[coach]->addChild( labelnode );
  //
  if ( coach < 1 && p_islabel ) {

    //osgEarth::Style labelStyle;
    //labelStyle.getOrCreate<osgEarth::TextSymbol>()->alignment() = osgEarth::TextSymbol::ALIGN_CENTER_CENTER;
    //labelStyle.getOrCreate<osgEarth::TextSymbol>()->fill().mutable_value().color() = osgEarth::Util::Color(GH_DEFAULT_LABEL_COLOR);
    //labelStyle.getOrCreate<osgEarth::TextSymbol>()->halo() = osgEarth::Util::Color(GH_DEFAULT_LABEL_HALO);
    //labelStyle.getOrCreate<osgEarth::TextSymbol>()->size() = GH_DEFAULT_LABEL_SIZE;
    //labelStyle.getOrCreate<osgEarth::TextSymbol>()->pixelOffset() = osg::Vec2s(0.0, GH_DEFAULT_LABEL_SIZE*3.0);
    //p_modellabel->setText(p_trainid);
    //p_modellabel->setStyle(labelStyle);
    //p_transform[coach]->addChild( p_modellabel );
    p_modellabel = _createLabelNode(p_trainid);
    p_transform[coach]->addChild( p_modellabel );
    
  }

  p_switch[coach]->addChild( p_transform[coach], true );
  
  p_models[coach].SetStatus(GH_MODEL_STATUS_LOADED);
 
  //return p_transform[coach];
}

//
//  https://www.packtpub.com/en-us/learning/how-to-tutorials/openscenegraph-advanced-scene-graph-components
//
//osgText::Text*
//createText( const osg::Vec3& pos,const std::string& content,float size ) {
//
//  osg::ref_ptr<osgText::Text> text = new osgText::Text;
//  text->setFont( g_font.get() );
//  text->setCharacterSize( size );
//  text->setAxisAlignment( osgText::TextBase::XY_PLANE );
//  text->setPosition( pos );
//  text->setText( content );
//  return text.release();
//}


osg::Switch *
ghRailUnit::GetModelSwitch(int coach) {
 return p_switch[coach];
}

osgEarth::GeoTransform *
ghRailUnit::GetModelTransform(int coach) {
 return p_transform[coach];
}

osg::PositionAttitudeTransform *
ghRailUnit::GetModelAttitude(int coach) {
 return p_attitude[coach];
}

bool
ghRailUnit::_simulateCoach( int coachid , ghRailTime *railtime )
{

  int prev_idx ;           // geom id at station point
  int current_idx ;        // geom id at station point
  int prev_type ;          // station type
  int current_type ;       // station type

  double start_time;
  double prev_time;
  double current_time;
  double diff_time;
  int simidx = 0;
  vector<double> simulationarray;
    
  nlohmann::json timetable = p_jsondata["timetable"];
  int timetable_size = timetable.size();
  osg::Vec3 position = osg::Vec3(1.0,1.0,1.0);
  osg::Quat rotation = osg::Quat(0.0,0.0,1.0,1.0);

  start_time = railtime->StrToDurationSecondsFromBasetime(timetable[0].get<std::string>());
  prev_time = railtime->StrToDurationSecondsFromBasetime(timetable[0].get<std::string>());
  prev_idx = _getStationIndex( timetable[1].get<std::string>(), coachid + 1 ) ;  // start from 1
  if ( prev_idx < 0 ) {
    std::cout << "ERROR " << std::endl;
    std::cout << timetable[1].get<std::string>() << std::endl;
    return false;
  }
  prev_type = timetable[2].get<int>();
  ////////////////////////////////////////////////////////
  for (int i = 3; i < timetable_size; i=i+3) {
    current_time = railtime->StrToDurationSecondsFromBasetime(timetable[i].get<std::string>());
    current_idx = _getStationIndex( timetable[i+1].get<std::string>(), coachid + 1 );
    current_type = timetable[i+2].get<int>();
    diff_time = current_time - start_time;

    if ( current_idx < 0 ) {
      std::cout << "ERROR " << std::endl;
      std::cout << timetable[i+1] << std::endl;
    } else {
      // NOP
    }

    if ( prev_idx == current_idx ) {
      // Dont Move
      position = _calcPositionFromDegrees( p_geometry[current_idx][0],
					   p_geometry[current_idx][1],
					   p_geometry[current_idx][2] );
      rotation = _calcQuatanion( current_idx );

      p_sim[coachid]->insert( diff_time+start_time,
			      osg::AnimationPath::ControlPoint(position,rotation));
      p_simlayer[coachid]->insert( diff_time+start_time,
				   _calcControlPointGeomLayers(p_geometry[current_idx][0],p_geometry[current_idx][1],rotation) );

    } else {
      if ( prev_type == GH_TYPE_DEPATURE && current_type == GH_TYPE_ARRIVAL ) {
	//  station - station
	// ghSimulateStationToStation(
	// previous station point idx
	// current station point idx + 1 // 1 step next;
	// distance ( current station - previous station)
	// time differenct ( current time - previous time )
	// total time ( previous time - start time )
	simulationarray = _simulateStationToStation( prev_idx,
						     current_idx + 1,
						     p_geometry[current_idx][3] - p_geometry[prev_idx][3],
						     current_time - prev_time,
						     prev_time - start_time );

	for (simidx = 0; simidx < simulationarray.size(); simidx++) {
	  	  position = _calcPositionFromDegrees( p_geometry[prev_idx+simidx][0],
						       p_geometry[prev_idx+simidx][1],
						       p_geometry[prev_idx+simidx][2] );
						 
						 
		  rotation = _calcQuatanion( prev_idx+simidx );
		  p_sim[coachid]->insert( simulationarray[simidx]+start_time,
					  osg::AnimationPath::ControlPoint(position,rotation));
		  p_simlayer[coachid]->insert( simulationarray[simidx]+start_time,
					       _calcControlPointGeomLayers(p_geometry[prev_idx+simidx][0],p_geometry[prev_idx+simidx][1],rotation) );
		  
	}
	

      } else if ( prev_type == GH_TYPE_DEPATURE && current_type == GH_TYPE_THROUGH ) {
	//  station - through
	simulationarray = _simulateStationToPassing( prev_idx,
						     current_idx + 1,
						     p_geometry[current_idx][3] - p_geometry[prev_idx][3],
						     current_time - prev_time,
						     prev_time - start_time );
	for (simidx = 0; simidx < simulationarray.size(); simidx++) {
	  	  position = _calcPositionFromDegrees( p_geometry[prev_idx+simidx][0],
						       p_geometry[prev_idx+simidx][1],
						       p_geometry[prev_idx+simidx][2] );
						 
						 
		  rotation = _calcQuatanion( prev_idx+simidx );
		  p_sim[coachid]->insert( simulationarray[simidx]+start_time,
					  osg::AnimationPath::ControlPoint(position,rotation));
		  p_simlayer[coachid]->insert( simulationarray[simidx]+start_time,
					       _calcControlPointGeomLayers(p_geometry[prev_idx+simidx][0],p_geometry[prev_idx+simidx][1],rotation) );

	}
	
      } else if ( prev_type == GH_TYPE_THROUGH && current_type == GH_TYPE_ARRIVAL ) {
	//  through - station
	simulationarray = _simulatePassingToStation( prev_idx,
						     current_idx + 1,
						     p_geometry[current_idx][3] - p_geometry[prev_idx][3],
						     current_time - prev_time,
						     prev_time - start_time );
	for (simidx = 0; simidx < simulationarray.size(); simidx++) {
	  	  position = _calcPositionFromDegrees( p_geometry[prev_idx+simidx][0],
						       p_geometry[prev_idx+simidx][1],
						       p_geometry[prev_idx+simidx][2] );
						 
						 
		  rotation = _calcQuatanion( prev_idx+simidx );
		  p_sim[coachid]->insert( simulationarray[simidx]+start_time,
					  osg::AnimationPath::ControlPoint(position,rotation));
		  p_simlayer[coachid]->insert( simulationarray[simidx]+start_time,
					       _calcControlPointGeomLayers(p_geometry[prev_idx+simidx][0],p_geometry[prev_idx+simidx][1],rotation) );

	}
	
      } else if ( prev_type == GH_TYPE_THROUGH && current_type == GH_TYPE_THROUGH ) {
	//  through - through
	simulationarray = _simulatePassingToPassing( prev_idx,
						     current_idx + 1,
						     p_geometry[current_idx][3] - p_geometry[prev_idx][3],
						     current_time - prev_time,
						     prev_time - start_time );
	
	for (simidx = 0; simidx < simulationarray.size(); simidx++) {
	  	  position = _calcPositionFromDegrees( p_geometry[prev_idx+simidx][0],
						       p_geometry[prev_idx+simidx][1],
						       p_geometry[prev_idx+simidx][2] );
						 
		  rotation = _calcQuatanion( prev_idx+simidx );
		  p_sim[coachid]->insert( simulationarray[simidx]+start_time,
					  osg::AnimationPath::ControlPoint(position,rotation));
		  p_simlayer[coachid]->insert( simulationarray[simidx]+start_time,
					       _calcControlPointGeomLayers(p_geometry[prev_idx+simidx][0],p_geometry[prev_idx+simidx][1],rotation) );

	}

      } else {
	// NOP Wrong status
	std::cout << "ERROR   " << prev_type << "  " << current_type << std::endl;
	std::cout << timetable[i+1].get<std::string>() << std::endl;
      }

    }
    
    prev_idx = current_idx ;
    prev_type = current_type ;
    prev_time = current_time;
    
  }

  return true;
}


//
//https://dexall.co.jp/articles/?p=2187
vector<string> ghRailUnit::_split(string str, string delim) {
  
    vector<string> tokens;
    size_t count = 1;
    for (size_t i = 0; i < str.length(); i++) {
        if (str.substr(i, delim.length()) == delim) {
            count++;
            i += delim.length() - 1;
        }
    }
    tokens.reserve(count);
    size_t prev = 0, pos = 0;
    while ((pos = str.find(delim, prev)) != string::npos) {
        if (pos > prev) {
            tokens.push_back(str.substr(prev, pos - prev));
        }
        prev = pos + delim.length();
    }
    if (prev < str.length()) {
        tokens.push_back(str.substr(prev));
    }
    return tokens;
}

  
//		
//   for variable change
//
int
ghRailUnit::_getStationIndex(string str,int num) {
  int samecount = 0;
  //for (int i = p_geometry.size()+1; i > -1 ; i--)  BUG
  for (int i = p_geometry.size()-1; i > -1 ; i--) {    
    if ( p_geometrystation[i] == str  ) {
      samecount++;
    }
    if ( samecount == num ) {
      //*lat = p_geometry[i][0];
      //*lng = p_geometry[i][1];
      //*alt = p_geometry[i][2];
      //*dis = p_geometry[i][3];
      return i;
    }

  }
  return -1;
}


vector<vector<double>>
ghRailUnit::_extendNewPointsForStation(int startidx,int direction,int extend) {

  vector<vector<double>> tmp;
  int i;
  int ilength = p_geompath.size();
  int nextpathidx = 0;
  double out_latRad = 0.0;
  double out_lonRad = 0.0;
  double angle = 0;
  int distance = 0;
  tmp.resize(1,vector<double>(2));
 
  // Search Next Station
  if ( direction > 0 ) {
    for ( i=startidx; i < ilength ; i++ ) {
      if ( p_geompathstation[i].compare(0,1,"x") == 0 ) {
	// SKIP
      } else {
	nextpathidx = i;
	break;
      }
    }
  } else {
    for ( i=startidx; i > 0; i-- ) {
      if ( p_geompathstation[i].compare(0,1,"x") == 0 ) {
	// SKIP
      } else {
	nextpathidx = i;
	break;
      }
    }
  }

  // Calc distance Next Station
  if ( direction > 0 ) {
    distance = p_geompath[nextpathidx][3] - p_geompath[startidx][3];
  } else {
    distance = p_geompath[startidx][3] - p_geompath[nextpathidx][3];
  }

  // Check and Create point when under GH_UNIT_GEOM_DISTANCE
  if ( distance <  extend ) {
    distance = extend - distance;
    if ( startidx == nextpathidx ) {
      if ( direction > 0 ) {
	angle = osgEarth::GeoMath::bearing(deg2rad(p_geompath[startidx][0]),deg2rad(p_geompath[startidx][1]),
					   deg2rad(p_geompath[startidx+2][0]),deg2rad(p_geompath[startidx+2][1]));
      } else {
	angle = osgEarth::GeoMath::bearing(deg2rad(p_geompath[startidx][0]),deg2rad(p_geompath[startidx][1]),
					   deg2rad(p_geompath[startidx-2][0]),deg2rad(p_geompath[startidx-2][1]));
      }
    } else {
      if ( direction > 0 ) {
	angle = osgEarth::GeoMath::bearing(deg2rad(p_geompath[startidx][0]),deg2rad(p_geompath[startidx][1]),
					   deg2rad(p_geompath[startidx+1][0]),deg2rad(p_geompath[startidx+1][1]));
      } else {
	angle = osgEarth::GeoMath::bearing(deg2rad(p_geompath[startidx][0]),deg2rad(p_geompath[startidx][1]),
					   deg2rad(p_geompath[startidx-1][0]),deg2rad(p_geompath[startidx-1][1]));
      }
    }

    angle = angle + GH_PI;   //angle = angle + 180; // reverse angle
    if ( angle > GH_PI ) angle = angle - 2*GH_PI; //  -180 < angle < 180
    osgEarth::GeoMath::destination(deg2rad(p_geompath[startidx][0]), deg2rad(p_geompath[startidx][1]),
				   angle, distance,
				   (double &)out_latRad, (double &)out_lonRad , osg::WGS_84_RADIUS_EQUATOR);
  } else {
    // NOP
  }

  tmp[0][0] = rad2deg(out_latRad);
  tmp[0][1] = rad2deg(out_lonRad);
    
  return tmp;
}

void
ghRailUnit::_geompath2geometry( vector<int> ranges )
{
  //
  //let d_geometry = [];
  ////  [ latitude , longitude, altitude, station name , station type, distance from start point , station position ]
  //
  //   vector<vector<double>> p_geometry;   // formatted geometry
  //
  //

  int size = p_geompath.size();
  vector<vector<double>> extend ;
  vector<vector<double>> tgeometry;   // template geometry
  std::map<std::string, int> stationname;
  int tgeomidx = 0;
  
  int idx;
  double distance = 0.0;

  extend = _extendNewPointsForStation(0,1,GH_UNIT_GEOM_DISTANCE);
  
  if ( extend[0][0] == 0.0 && extend[0][1] == 0.0 ) {
    // NOP
  } else {
    tgeometry.resize(1);
    tgeometry[0].resize(4);
    tgeometry[0][0] = extend[0][0];   // latitude
    tgeometry[0][1] = extend[0][1];   // longitude 
    tgeometry[0][2] = p_geompath[0][2]; // altitude
    tgeometry[0][3] = distance ; // distance in meter from 0
    tgeomidx++;
   }

  double angle01 ;
  double angle12 ;
  double angle ;

  for (idx = 0; idx < size; idx++) {
    tgeometry.resize(tgeometry.size()+1);
    tgeometry[tgeomidx].resize(4);
    tgeometry[tgeomidx][0] = p_geompath[idx][0];   // latitude
    tgeometry[tgeomidx][1] = p_geompath[idx][1];   // longitude 
    tgeometry[tgeomidx][2] = p_geompath[idx][2];   // altitude
    tgeometry[tgeomidx][3] = p_geompath[idx][3];   // distance in meter from 0
    
    if ( tgeomidx > 0 ) {
    	distance = osgEarth::GeoMath::distance(deg2rad(tgeometry[tgeomidx-1][0]),deg2rad(tgeometry[tgeomidx-1][1]),
    					       deg2rad(tgeometry[tgeomidx][0]),deg2rad(tgeometry[tgeomidx][1]));
    	tgeometry[tgeomidx][3] = distance + tgeometry[tgeomidx-1][3];
    }
    if ( p_geompathstation[idx].compare(0,1,"x") == 0 ) {
      // SKIP
    } else {
      stationname[ p_geompathstation[idx] ] = tgeomidx;
    }
    tgeomidx++;
    
    
    if ( idx < size - 4 && idx > 4 ) {

      angle01 = osgEarth::GeoMath::bearing(deg2rad(p_geompath[idx-2][0]),deg2rad(p_geompath[idx-2][1]),
					   deg2rad(p_geompath[idx][0]),deg2rad(p_geompath[idx][1]));
      angle12 = osgEarth::GeoMath::bearing(deg2rad(p_geompath[idx][0]),deg2rad(p_geompath[idx][1]),
					   deg2rad(p_geompath[idx+2][0]),deg2rad(p_geompath[idx+2][1]));
      
      angle = rad2deg(_calcObtuseAngle(angle01,angle12));

      if ( angle < 90 || angle > 270.0 ) {
	// NOP
      } else {
	extend = _extendNewPointsForStation(idx,-1,GH_UNIT_GEOM_DISTANCE);
	if ( extend[0][0] == 0.0 && extend[0][1] == 0.0 ) {
	  // NOP
	} else {
	  tgeometry.resize(tgeometry.size()+1);
	  tgeometry[tgeomidx].resize(4);
	  tgeometry[tgeomidx][0] = extend[0][0];   // latitude
	  tgeometry[tgeomidx][1] = extend[0][1];   // longitude 
	  tgeometry[tgeomidx][2] = p_geompath[idx][2];   // altitude
	  tgeometry[tgeomidx][3] = p_geompath[idx][3];   // distance in meter from 0
	  if ( tgeomidx > 0 ) {
	    distance = osgEarth::GeoMath::distance(deg2rad(tgeometry[tgeomidx-1][0]),deg2rad(tgeometry[tgeomidx-1][1]),
						   deg2rad(tgeometry[tgeomidx][0]),deg2rad(tgeometry[tgeomidx][1]));
	    tgeometry[tgeomidx][3] = distance + tgeometry[tgeomidx-1][3];
	  }
      	  tgeomidx++;
      	}
      }
      
    }
  }
      
  extend = _extendNewPointsForStation(size-1,-1,GH_UNIT_GEOM_DISTANCE);
  if ( extend[0][0] == 0.0 && extend[0][1] == 0.0 ) {
    // NOP
  } else {
    tgeometry.resize(tgeometry.size()+1);
    tgeometry[tgeomidx].resize(4);
    tgeometry[tgeomidx][0] = extend[0][0];   // latitude
    tgeometry[tgeomidx][1] = extend[0][1];   // longitude 
    tgeometry[tgeomidx][2] = p_geompath[size-1][2];   // altitude
    tgeometry[tgeomidx][3] = p_geompath[size-1][3];   // distance in meter from 0
    if ( tgeomidx > 0 ) {
      distance = osgEarth::GeoMath::distance(deg2rad(tgeometry[tgeomidx-1][0]),deg2rad(tgeometry[tgeomidx-1][1]),
  					     deg2rad(tgeometry[tgeomidx][0]),deg2rad(tgeometry[tgeomidx][1]));
      tgeometry[tgeomidx][3] = distance + tgeometry[tgeomidx-1][3];
    }
  }

  ///////////////////////////////////////////////////////////////
  //
  //    Arrangement geometry data from timetable
  //
  //////////////////////////////////////////////////////////////

  nlohmann::json timetable = p_jsondata["timetable"];
  vector<double> rangepoints ;
  
  //  "timetable":["0T16:56:00","Brussels Midi",4,"0T17:34:00","Lille Europe",2,"0T17:35:00","Lille Europe",4,"0T18:34:00","Ashford Intl",2,"0T18:35:00","Ashford Intl",4,"0T18:55:00","Ebbsfleet Intl",2,"0T18:56:00","Ebbsfleet Intl",4,"0T19:15:00","London St. Pancras Intl",2]
  //size = timetable.size();  // timetable size

  int timetableid = 0;
  int p_geometry_count = 0;

  //
  //   First Station
  //
  rangepoints = _createStationRangePoints( (vector<vector<double>> &) tgeometry,                  //  Base Geometry array vector 
					   stationname[ timetable[timetableid+1].get<std::string>() ],           //  Station id in Geometry array vector 
					   ranges );                                                 //   Create points distance from station point
  //
  //
  // result.push_back(n);    inserted geometry id
  // result.push_back(n+1);   inserted latitude (deg)
  // result.push_back(n+2);   inserted longitude (deg)
  //  ..
  //  ..
  //  ..
  //

  int tail_id = (int)rangepoints[0];
  int head_id = (int)rangepoints[ rangepoints.size() -3 ]+1; // plus one is important
  int idx2 = 0;
  for (idx = tail_id; idx < head_id; idx++) {
    _appendGeometryData( tgeometry[idx][0],
			 tgeometry[idx][1],
			 tgeometry[idx][2],
			 "x",
			 GH_TYPE_THROUGH,GH_STATION_TYPE_UNDEFINED,GH_STATION_TYPE_UNDEFINED);

    for (idx2 = 0; idx2 < rangepoints.size(); idx2=idx2+3) {
      if ( idx == (int)rangepoints[idx2] ) {
	_appendGeometryData( rangepoints[idx2+1],
			     rangepoints[idx2+2],
			     tgeometry[idx][2],
			     timetable[timetableid+1].get<std::string>(),
			     timetable[timetableid+2].get<int>() ,GH_STATION_TYPE_NORMAL_HEAD , GH_STATION_TYPE_THROUGH_HEAD);
      }
    }
  }
  
  int prevheadidx = head_id;

  //
  //  Each Station
  //
  for (timetableid = 3; timetableid < timetable.size(); timetableid=timetableid+3) {
    if ( timetable[timetableid-2].get<std::string>() == timetable[timetableid+1].get<std::string>() ) {
      // NOP
      // Same station 'Stop operation'
      
    } else {

      rangepoints = _createStationRangePoints( (vector<vector<double>> &) tgeometry,
					       stationname[ timetable[timetableid+1].get<std::string>() ],
					       ranges );

      ////////////////////////////   station to station points
      for ( idx = prevheadidx ; idx < (int)rangepoints[0]; idx++ ) {
	_appendGeometryData( tgeometry[idx][0],
			     tgeometry[idx][1],
			     tgeometry[idx][2],
			     "x",
			     GH_TYPE_THROUGH,GH_STATION_TYPE_UNDEFINED,GH_STATION_TYPE_UNDEFINED);
      }
      ////////////////////////////

      tail_id = (int)rangepoints[0];
      head_id = (int)rangepoints[ rangepoints.size() -3 ]+1; // plus one is important
      idx2 = 0;
      for (idx = tail_id; idx < head_id; idx++) {
	_appendGeometryData( tgeometry[idx][0],
			     tgeometry[idx][1],
			     tgeometry[idx][2],
			     "x",
			     GH_TYPE_THROUGH,GH_STATION_TYPE_UNDEFINED,GH_STATION_TYPE_UNDEFINED);
	for (idx2 = 0; idx2 < rangepoints.size(); idx2=idx2+3) {
	  if ( idx == (int)rangepoints[idx2] ) {
	    _appendGeometryData( rangepoints[idx2+1],
				 rangepoints[idx2+2],
				 tgeometry[idx][2],
				 timetable[timetableid+1].get<std::string>(),
				 timetable[timetableid+2].get<int>() ,GH_STATION_TYPE_NORMAL_HEAD , GH_STATION_TYPE_THROUGH_HEAD);
	  }
	}
      }
      prevheadidx = head_id;
    }
    
  }


}


double
ghRailUnit::_calcObtuseAngle( double angle01,
			      double angle12
			      ) {

  double ret = -1;

  //
  // -180.0 <  angle  < 180.0
  //  - PI/2 < angle < PI/2
  //
  
  if ( angle01 > 0 ) {
    if ( angle12 > 0 ) {
      ret = abs( angle01 - angle12 );
    } else {
      ret = angle01 - angle12;
    }
  } else {
    // angle01 < 0
    if ( angle12 > 0 ) {
      ret = angle12 - angle01;
    } else {
      // angle12 < 0
      ret = abs( angle01 - angle12 );
    }
  }

  return ret;

}


vector<double>
ghRailUnit::_createStationRangePoints( vector<vector<double>>& geom,
				       int stationidx,
				       vector<int> unit_distance ) {


  vector<double> result;
  //int search_count = 50; // search point max;
  //  vector<int> unit_distance;
  //
  //  Extend the range on both sides ( negative and positive )
  //
  //  int value = 0;
  //  for (auto ite = ranges.begin(); ite < ranges.end(); ite++) {
  //    value = *ite;
  //    unit_distance.push_back( value );
  //    unit_distance.push_back( -1 * value );
  //  }
  sort(unit_distance.begin(), unit_distance.end());  // Sort  -2,-1,0.1,2
  size_t unit_distance_size = unit_distance.size();
  int i = 0;
  vector<double> tpoint;
  for ( i=0; i < unit_distance_size ; i++ ) {
    //std::cout << "-R- " << i << " -- " << unit_distance[i]  << "\n";
    tpoint = _createDistancePoint( geom,
				   stationidx,
				   unit_distance[i] );
    result.push_back(tpoint[0]);
    result.push_back(tpoint[1]);
    result.push_back(tpoint[2]);
  }
  return result;
}

vector<double>
ghRailUnit::_createDistancePoint( vector<vector<double>> &geom,
				  int startidx,
				  int distance  )
{


  vector<double> res(3) ;
  int search_count = 50; // search point max;
  int searchmax = startidx;
  int point_id = -1;
  double point_distance = -1;
  double angle = 0;
  double out_latRad = 0.0;
  double out_lonRad = 0.0;
  int i = 0;

  if ( distance < 0 )
    {
      searchmax = startidx - search_count;
      if ( searchmax < 0 ) searchmax = -1;
      for ( i=startidx-1; i > searchmax; i-- ) {
	point_distance = geom[startidx][3] - geom[i][3] ;
	if ( point_distance > abs(distance) ) {
	  point_id = i;
	  point_distance = abs(point_distance-abs(distance));
	  break;
	}
      }
      //////////////////
      if ( point_id < 0 ) {
	std::cout << "Warning Station Distance Point. " << startidx << "  " << distance << "\n";
	point_distance = abs(point_distance-abs(distance));
	angle = osgEarth::GeoMath::bearing(deg2rad(geom[startidx+1][0]),deg2rad(geom[startidx+1][1]),
				     deg2rad(geom[startidx][0]),deg2rad(geom[startidx][1]));
	osgEarth::GeoMath::destination(deg2rad(geom[startidx][0]), deg2rad(geom[startidx][1]),
				       angle, point_distance,
				       (double &)out_latRad, (double &)out_lonRad , osg::WGS_84_RADIUS_EQUATOR);
	res[0] = (double)startidx;
	res[1] = rad2deg(out_latRad);
	res[2] = rad2deg(out_lonRad);
	return res;
      }
      //////////////////      
    }
  else
    {
      searchmax = startidx + search_count;
      if ( searchmax > geom.size() ) searchmax = geom.size();
      for ( i=startidx+1; i < searchmax; i++ ) {
	point_distance = geom[i][3] - geom[startidx][3];
	if ( point_distance > abs(distance) ) {
	  point_id = i-1;
	  point_distance = abs(abs(distance) - ( geom[i-1][3] - geom[startidx][3] ));
	  break;
	}
      }

      
    }

  angle = osgEarth::GeoMath::bearing(deg2rad(geom[point_id][0]),deg2rad(geom[point_id][1]),
				     deg2rad(geom[point_id+1][0]),deg2rad(geom[point_id+1][1]));
  osgEarth::GeoMath::destination(deg2rad(geom[point_id][0]), deg2rad(geom[point_id][1]),
				 angle, point_distance,
				 (double &)out_latRad, (double &)out_lonRad , osg::WGS_84_RADIUS_EQUATOR);

  res[0] = (double)point_id;
  res[1] = rad2deg(out_latRad);
  res[2] = rad2deg(out_lonRad);

  return res;
}


//////////////////////////////////////////////

void
ghRailUnit::_appendGeometryData( double lat,
				 double lon,
				 double alt,
				 string name,
				 int type, int type1, int type2)
{
  double distance = 0.0;

  if ( p_geometry_count < 1 ) {
    p_geometry.resize(1);
  } else {
    p_geometry.resize(p_geometry.size()+1);
  }
  p_geometrystation.resize(p_geometrystation.size()+1);
  p_geometrystationtype.resize(p_geometrystationtype.size()+1);
  
  p_geometry[p_geometry_count].resize(4);
  p_geometry[p_geometry_count][0] = lat;
  p_geometry[p_geometry_count][1] = lon;
  p_geometry[p_geometry_count][2] = alt;
  p_geometry[p_geometry_count][3] = distance;
  p_geometrystation[p_geometry_count] = name;
  if ( type == GH_TYPE_DEPATURE || type == GH_TYPE_ARRIVAL ) {
    p_geometrystationtype[p_geometry_count] = type1;
  } else {
    p_geometrystationtype[p_geometry_count] = type2;
  }

  if ( p_geometry_count < 1 )
    {
      // NOP
    }
  else
    {
      distance = osgEarth::GeoMath::distance(deg2rad(p_geometry[p_geometry_count][0]),deg2rad(p_geometry[p_geometry_count][1]),
    					     deg2rad(p_geometry[p_geometry_count-1][0]),deg2rad(p_geometry[p_geometry_count-1][1]));
      p_geometry[p_geometry_count][3] = distance + p_geometry[p_geometry_count-1][3];
    }
  
  p_geometry_count++;

}


osg::Vec3d
ghRailUnit::_calcPositionFromDegrees( double lat,
				      double lng,
				      double alt )
{
  osgEarth::Ellipsoid WGS84;
  osg::Vec3d ret = WGS84.geodeticToGeocentric(osg::Vec3d(lng, lat, alt));
  return ret;

}


osg::Quat
ghRailUnit::_calcQuatanion( int currentid )
{
  int nextid = currentid + 1;
  int maxid = p_geometry.size() -1 ;
  if ( nextid > maxid ) {
    nextid = currentid;
    currentid = currentid - 1;
  } else {
    // NOP
  }
  
  //double lat = p_geometry[currentid][0];  S->N
  //double lng = p_geometry[currentid][1];  W->E
  //double alt = p_geometry[currentid][2];  D->U
  double bearing_next = osgEarth::GeoMath::rhumbBearing(
					      deg2rad(p_geometry[currentid][0]),deg2rad(p_geometry[currentid][1]),
					      deg2rad(p_geometry[nextid][0]),deg2rad(p_geometry[nextid][1]) );
  
  osg::Quat quatanion_pan;
  osg::Vec3d axis(0.0f,0.0f,-1.0f);  // Important ! . This axis depends on Gltf model local coordinate(local axis)
  quatanion_pan.makeRotate( bearing_next+GH_PI ,axis );

  // atan2 [rad] ( -PI ~ PI );
  double tangent =  atan2(
			  ( p_geometry[nextid][2] - p_geometry[currentid][2] ),
			  osgEarth::GeoMath::distance(
						      deg2rad(p_geometry[currentid][0]),deg2rad(p_geometry[currentid][1]),
						      deg2rad(p_geometry[nextid][0]),deg2rad(p_geometry[nextid][1])  )
			  );

  if ( tangent > GH_QUATANION_TILT_MAX ) {
    tangent = GH_QUATANION_TILT_MAX;
  } else if ( tangent < GH_QUATANION_TILT_MIN ) {
    tangent = GH_QUATANION_TILT_MIN;
  } else {
    // NOP
  }

  osg::Quat quatanion_tilt;
  osg::Vec3d axis2(1.0f,0.0f,0.0f);  // Important ! . This axis depends on Gltf model local coordinate(local axis)
  quatanion_tilt.makeRotate( tangent ,axis2 );
  
  return quatanion_pan * quatanion_tilt;

}

vector<double>
ghRailUnit::_simulateStationToStation( int startidx,
				       int stopidx,
				       double distance,
				       double sec,
				       double startsec )
{

  vector<double> res ;

  double x = 0;
  double y = 0;
  double vel = distance / ( ( 1 - GH_ACCL_RATIO_D ) * sec );
  double xbound[2];
  double ybound[2];

  xbound[0] = ( GH_ACCL_RATIO_D * distance ) / ( 2 * ( 1 - GH_ACCL_RATIO_D ) );
  xbound[1] = ( ( 2 - 3 * GH_ACCL_RATIO_D ) * distance ) / ( 2 * ( 1 - GH_ACCL_RATIO_D ) );
  ybound[0] = GH_ACCL_RATIO_D * sec;
  ybound[1] = ( 1 - GH_ACCL_RATIO_D ) * sec;
  
  int j = 0;
  
  for ( j=startidx;j<stopidx; j++ ) {
    //lat1 = p_geometry[j][0];
    //lng1 = p_geometry[j][1];
    //alt1 = p_geometry[j][2];    

    x = p_geometry[j][3] - p_geometry[startidx][3];
        
    if ( x >= 0 && x < xbound[0] ) {
      // Accle
      y = __simulateTwoPoints( x, GH_ACCL_RATIO_D * sec , xbound[0] , 0);
    } else if ( x >= xbound[0] && x <= xbound[1] ) {
      // Vel
      y = __simulateTwoPoints( x - xbound[0], ( 1 - 2 * GH_ACCL_RATIO_D ) * sec , xbound[1] - xbound[0] , vel) + ybound[0];
    } else if ( x > xbound[1] && x <= distance ) {
      //  0.1 margin for error
      // De-Accel
      y = __simulateTwoPoints( x - xbound[1], GH_ACCL_RATIO_D * sec , xbound[0] , vel)  + ybound[1];
    } else {
      // Wrong parameter   
    }
    //printf("%.10lf ", y );
    res.push_back(startsec + y);
  }

  return res;
  
}

vector<double>
ghRailUnit::_simulateStationToPassing( int startidx,
				       int stopidx,
				       double distance,
				       double sec,
				       double startsec )
{

  vector<double> res ;
  double x = 0;
  double y = 0;
  double vel = 2 * distance / ( ( 2 - GH_ACCL_RATIO_A ) * sec );
  double xbound[2];
  double ybound[2];
  
  xbound[0] = ( GH_ACCL_RATIO_A * distance ) / ( 2 - GH_ACCL_RATIO_A );
  xbound[1] = 0.0;
  ybound[0] = GH_ACCL_RATIO_A * sec;
  ybound[1] = 0.0;

  int j = 0;
  
  for ( j=startidx;j<stopidx; j++ ) {
    x = p_geometry[j][3] - p_geometry[startidx][3];

    if ( x >= 0 && x < xbound[0] ) {
      // Accle
      y = __simulateTwoPoints( x, GH_ACCL_RATIO_A * sec , xbound[0] , 0);
    } else if ( x >= xbound[0] && x <= distance + 0.1 ) {
      // Vel
      y = __simulateTwoPoints( x - xbound[0], ( 1 - GH_ACCL_RATIO_A ) * sec , distance - xbound[0] , vel) + ybound[0]; 
    } else {
      // Wrong parameter   
    }
    res.push_back(startsec + y);
  }
  return res;
  
}

vector<double>
ghRailUnit::_simulatePassingToStation( int startidx,
				       int stopidx,
				       double distance,
				       double sec,
				       double startsec )
{

  vector<double> res ;
  double x = 0;
  double y = 0;
  double vel = 2 * distance / ( ( 2 - GH_ACCL_RATIO_D ) * sec );
  double xbound[2];
  double ybound[2];

  xbound[0] = 2 * ( 1 - GH_ACCL_RATIO_A ) * distance  / ( 2 - GH_ACCL_RATIO_A );
  xbound[1] = 0.0;
  ybound[0] = ( 1 - GH_ACCL_RATIO_A ) * sec;
  ybound[1] = 0.0;
  
  int j = 0;
  
  for ( j=startidx;j<stopidx; j++ ) {
    x = p_geometry[j][3] - p_geometry[startidx][3];
        
        
    if ( x >= 0 && x < xbound[0] ) {
      // Vel 
      y = __simulateTwoPoints( x, ( 1 - GH_ACCL_RATIO_A ) * sec , xbound[0] , vel);
    } else if ( x >= xbound[0] && x <= distance + 0.1 ) {
      // De-Accel
      y = __simulateTwoPoints( x - xbound[0], GH_ACCL_RATIO_A * sec , distance - xbound[0] , vel) + ybound[0]; 
    } else {
      // Wrong parameter   
    }
    res.push_back(startsec + y);
  }
  return res;
  
}

vector<double>
ghRailUnit::_simulatePassingToPassing( int startidx,
				       int stopidx,
				       double distance,
				       double sec,
				       double startsec )
{

  vector<double> res ;
  double x = 0;
  double y = 0;
  double vel = distance / sec;
  int j = 0;
  
  for ( j=startidx;j<stopidx; j++ ) {
    x = p_geometry[j][3] - p_geometry[startidx][3];

    y = __simulateTwoPoints( x, sec , distance , vel);

    res.push_back(startsec + y);
  }
  return res;
}



double
ghRailUnit::__simulateTwoPoints(double x,double t, double d,double v) {
    //   x = distance from startpoint
    //   t = time interval between start point  and end point
    //   d = distance between start point  and end point
    //   v = velocity at start point

    if ( t < 1.0 ) {
      std::cout << "Wrong Data " << x << "  " << t  << "  " << d << "  " << v << "\n";
      return 0.0;
    }
    double alpha = 2 * ( d - v * t ) / ( t * t );
    double ret = 0;
    double param = 0;
    if ( abs(alpha) < 0.001 && v != 0 ) {
        // nearly constat velocity   
        ret = x / v ;
    } else {
        param = v * v + 2 * alpha * x;
        if ( param < 0 ) param = 0;
        ret = ( -1 * v + sqrt( param ) ) / alpha;
        if ( ret < 0 ) ret = 0;
    }
    return ret;
}

void
ghRailUnit::_initLocomotiveArray(vector<int>  lsize) {

  int locomotivesumlength = 0;
  for (const int& snumber : lsize ) {
    locomotivesumlength += snumber;
  }
  p_locomotives.reserve( (int)lsize.size() + 1);
  locomotivesumlength = (int) ceil( locomotivesumlength/2.0) ;
  p_locomotives.push_back( locomotivesumlength );  // head
    
  for (const int& snumber : lsize ) {
    locomotivesumlength = (int) locomotivesumlength - snumber ;
    p_locomotives.push_back( locomotivesumlength );
  }

}
  
void
ghRailUnit::_initLocomotiveModel(vector<string>  locomotive) {

  p_models.reserve( (int)locomotive.size() );
  p_transform.reserve( (int)locomotive.size() );
  p_switch.reserve( (int)locomotive.size() );
  p_attitude.reserve( (int)locomotive.size() );
  
  for (const string& uri : locomotive ) {
    ghRailModel loco;
    loco.Setup(GEOGLYPH_ROOT_URI,uri);

    p_models.push_back( loco );
    p_switch.push_back( new osg::Switch ); // initialize
    p_transform.push_back( new osgEarth::GeoTransform ); // initialize
    p_attitude.push_back( new osg::PositionAttitudeTransform ); // initialize
  }

}
  

  
osgEarth::LabelNode *
ghRailUnit::_createLabelNode(string text) {

  osgEarth::Style labelStyle;
  osgEarth::LabelNode *label = new osgEarth::LabelNode();
  
  labelStyle.getOrCreate<osgEarth::TextSymbol>()->alignment() = osgEarth::TextSymbol::ALIGN_CENTER_CENTER;
  labelStyle.getOrCreate<osgEarth::TextSymbol>()->fill().mutable_value().color() = osgEarth::Util::Color(GH_DEFAULT_LABEL_COLOR);
  labelStyle.getOrCreate<osgEarth::TextSymbol>()->halo() = osgEarth::Util::Color(GH_DEFAULT_LABEL_HALO);
  labelStyle.getOrCreate<osgEarth::TextSymbol>()->size() = GH_DEFAULT_LABEL_SIZE;
  labelStyle.getOrCreate<osgEarth::TextSymbol>()->pixelOffset() = osg::Vec2s(0.0, GH_DEFAULT_LABEL_SIZE*3.0);
  label->setStyle(labelStyle);
  label->setText(text);
  return label;
  
}

osg::AnimationPath::ControlPoint 
ghRailUnit::_calcControlPointGeomLayers(double lat, double lng, osg::Quat quat)
{
  int glength = p_geomlayer.size();
  int idx = 0;
  int altidx = -1;
  double distance = 0.0;
  double distance_check_range = GH_LAYER_DISTANCE_THRESHOLD;
  
  for (idx = 0; idx < glength; idx++) {
    distance = osgEarth::GeoMath::distance(deg2rad(lat),deg2rad(lng),
					   deg2rad(p_geomlayer[idx][0]),deg2rad(p_geomlayer[idx][1]));
    if ( distance < distance_check_range ) {
      altidx = idx;
      break;
    }
  }
  osg::Vec3d ret;
  if ( altidx > -1 ) {
    ret = osg::Vec3d(deg2rad(lng), deg2rad(lat), p_geomlayer[altidx][2]);
  } else {
    ret = osg::Vec3d(deg2rad(lng), deg2rad(lat), 0.0);
  }
  //std::cout << std::to_string( lat ) << " " << std::to_string( lng ) << " " << std::to_string( p_geomlayer[altidx][2] ) << " " << std::to_string( distance ) << std::endl;

  osg::AnimationPath::ControlPoint result = osg::AnimationPath::ControlPoint( ret, quat ) ;
  
  return result;
}

std::string
ghRailUnit::GetMarkerUri() {
  string uri = GEOGLYPH_ROOT_URI;
  string path = GEOGLYPH_RSC_ICON_PATH;
  return uri + path + p_marker;
}

std::string
ghRailUnit::GetLineInfo() {
  return p_lineid + " " + p_direction + " " + p_routeid;
}

std::string
ghRailUnit::GetDistanceInfo() {

  std::string ret = " ";
  nlohmann::json timetable = p_jsondata["timetable"];
  // timetable[i].get<std::string>()     time string    "0T14:54:00"
  // timetable[i+1].get<std::string>()   station string
  // timetable[i+2].get<std::string>()   station type

  int timetable_size = timetable.size();
  std::string prevname = " ";
  double distance = 0.0;
  double firstdistance = -1.0;
  for (int i = 0; i < timetable_size; i=i+3) {
    std::string name = timetable[i+1].get<std::string>();
    if ( name != prevname ) {
      ret += name;
      ret += ",";
      int idx = _getStationIndex( name,1 ) ;  // start from 1
      distance = p_geometry[idx][3];
      if ( firstdistance < 0 ) firstdistance = distance;
      ret += std::to_string( distance - firstdistance );
      ret += ",";

      prevname = name;
    }
  }


  return ret;
}
