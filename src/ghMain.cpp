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
#include <osgEarthImGui/ImGuiApp>
#include <osgEarth/EarthManipulator>
#include <osgEarth/ExampleResources>
#include <osgEarth/Sky>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgEarth/MapNode>
#include <osgDB/PluginQuery>
#include <osg/Group>
#include <osgEarth/PlaceNode>

#include <osgEarthImGui/LayersGUI>
#include <osgEarthImGui/NetworkMonitorGUI>
#include <osgEarthImGui/SceneGraphGUI>
#include <osgEarthImGui/SystemGUI>
#include <osgEarthImGui/EnvironmentGUI>
#include <osgEarthImGui/TerrainGUI>
#include <osgEarthImGui/RenderingGUI>
#include <osgEarthImGui/AnnotationsGUI>
#include <osgEarthImGui/PickerGUI>

#include "ghRailGUI"
#include "ghRail.hpp"
#include "ghRailCommand.hpp"

#include <thread>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>

#define GEOGLYPH_ELAPSED_THRESHOLD 1.0
#define GEOGLYPH_SUN_AMBIENT 0.03

#define LC "[imgui] "

using namespace osgEarth;
using namespace osgEarth::Util;


int
usage(const char* name)
{
    OE_NOTICE
        << "\nUsage: " << name << " file.earth" << std::endl
        << MapNodeHelper().usage() << std::endl;
    return 0;
}

void
showPlugins()
{

  osgDB::FileNameList plugins = osgDB::listAllAvailablePlugins();
  for(osgDB::FileNameList::iterator itr = plugins.begin(); itr != plugins.end(); ++itr)
    {
      osgDB::outputPluginDetails(std::cout,*itr);
      //std::cout << (*itr) << std::endl;
    }
  
}

double
_calcSimulationTime(DateTime dt,DateTime base, double elapsesec) {
  time_t basetime = base.asTimeStamp();
  time_t simtime = dt.asTimeStamp();
  double sec = simtime - basetime + elapsesec;
  return sec;
}


DateTime
_calcElapsedTime(DateTime dt,double elapsesec) {
  int year = dt.year();
  int month = dt.month();
  int days = dt.day();
  double hours = dt.hours(); // double  0 ~ 24.0

  hours = hours + ( elapsesec / 3600.0 ); // sec -> hours

  if ( hours > 24.0 ) {
    days = days + 1;
    hours = hours - 24.0;
  }
  if ( hours < 0.0 ) {
    days = days - 1;
    hours = hours + 24.0;
  }
  return DateTime(year, month, days, hours);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ghCommandQueue *cmdqueue = (ghCommandQueue *)NULL ;
int client_fd;
int listen_port = GH_DEFAULT_SOCKET_PORT;
std::mutex ghMutex;
/////////////////////

void
socketthread() {

  char buffer[1024] = {0};
  ghCommandQueue *cmdtmp = (ghCommandQueue *)NULL ;

  while (true)
    {
      // Wait for client receive message
      read(client_fd, buffer, 1024);
      std::string command(buffer);
      cmdtmp = cmdqueue;
      cmdqueue = ghRailParseCommand(command);
      if ( cmdqueue != NULL ) {
	cmdqueue->prev = cmdtmp;
	  
	memset(buffer, 0, sizeof(buffer));
	    
	if ( cmdqueue->type == GH_COMMAND_CLOSE ||
	     cmdqueue->type == GH_COMMAND_EXIT ) {
	  break;
	}
      }
    }
  //  End of client socket read loop
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *   Child Process quit.
 *    cleanup defunct process
 */
void
signalChild( int sig )
{
  int child_pid = 0;
  do {
    int status;
    child_pid = waitpid(-1,&status,WNOHANG);
  } while(child_pid>0);

} 

int
socketInit(int port)
{
  int fd;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(fd, GH_MAX_SOCKET) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  return fd;
}

int
mainloop(osg::ArgumentParser args,unsigned int width, unsigned int height)
{

  // Set up the viewer and input handler:
  osgViewer::Viewer ghViewer(args);
  osgEarth::EarthManipulator *ghManipulator = new osgEarth::EarthManipulator(args);
  osgEarth::SkyNode *ghSky; // manipulate date-time
  double ghSimulationTime = 0.0;

  ghViewer.setUpViewInWindow( 0, 0, width, height, 0 );
  ghViewer.setThreadingModel(ghViewer.ThreadPerContext);
  ghViewer.setCameraManipulator( ghManipulator );
  ghViewer.setRealizeOperation(new ImGuiAppEngine::RealizeOperation);

  osgViewer::Viewer::Windows windows;
  ghViewer.getWindows(windows);
  windows[0]->setWindowName(GH_WELCOME_MESSAGE);

  //
  // Load the earth file.
  //
  osg::ref_ptr<osg::Node> ghNode3D = MapNodeHelper().load(args, &ghViewer);
    
  if (ghNode3D.valid())
    {
      // Call this to add the GUI. 
      auto ui = new ImGuiAppEngine(args);

      ui->add("File", new QuitGUI());

      ui->add("Tools", new NetworkMonitorGUI());
      ui->add("Tools", new AnnotationsGUI());
      ui->add("Tools", new PickerGUI());
      ui->add("Tools", new RenderingGUI());
      ui->add("Tools", new SceneGraphGUI());
      ui->add("Tools", new SystemGUI());
      ui->add("Tools", new TerrainGUI());

      osgEarth::MapNode* mapNode = osgEarth::MapNode::findMapNode(ghNode3D);
      //std::cout << mapNode->getMapSRS()->getGeographicSRS() << std::endl;
      
      /***   Sky and date time **/
      ghSky = SkyNode::create();
      ghSky->setDateTime(DateTime());
      ghSky->setSimulationTimeTracksDateTime(true);
      ghSky->setAtmosphereVisible(true);
      ghSky->getSunLight()->setAmbient(osg::Vec4(GEOGLYPH_SUN_AMBIENT, GEOGLYPH_SUN_AMBIENT, GEOGLYPH_SUN_AMBIENT, 1.0f));
      ghSky->setLighting(true);
      ghSky->setSunVisible(true);
      ghSky->setMoonVisible(false);
      ghSky->setStarsVisible(true);
      auto parent = mapNode->getParent(0);
      ghSky->addChild(mapNode);
      parent->addChild(ghSky);
      parent->removeChild(mapNode);
      /***   Sky and date time **/

      ui->onStartup = []()
      {
	ImGui::GetIO().FontAllowUserScaling = true;
      };
      ghViewer.getEventHandlers().push_front(ui);
      ghViewer.setSceneData(ghNode3D);

      ghRailGUI *ghGui = new ghRailGUI();
      ui->add("Clock", ghGui );
      ghRail ghrail; //    Rail Class 
      ghrail.SetSpeed(1.0);
      ghrail.SetPlayPause(false);
      std::thread ghSock(socketthread);
      /////////////////////////////////////////////  Socket LOOP
    
      double _elapsed_prev = 0.0f;    // Important
      double _elapsed_current = ghViewer.elapsedTime(); // Important
      double _elapsed_sec = 10.0f;

      while (!ghViewer.done())
        {
	  _elapsed_current = ghViewer.elapsedTime() ; // double [sec]
	  if ( ghrail.IsPlaying() ) {
	    double elapsed =  ( _elapsed_current - _elapsed_prev ) * ghrail.GetSpeed(); // duration seconds
	    DateTime dt = ghSky->getDateTime();
	    ghSimulationTime = _calcSimulationTime(dt,ghrail.GetBaseDatetime(), _elapsed_sec);

	    // Simulation Update
	    ghrail.Update( ghSimulationTime, mapNode , &ghViewer);

	    if ( _elapsed_sec > GEOGLYPH_ELAPSED_THRESHOLD ) {
	      // Change Date Time Dislpay
	      ghSky->setDateTime(_calcElapsedTime(dt,_elapsed_sec));
	      _elapsed_sec = 0.0f;
	    } else {
	      _elapsed_sec += elapsed;
	    }
	  } else {
	    // Not Playing
	  }
	  _elapsed_prev = _elapsed_current;

	  // Frame Update
	  ghViewer.frame();

	  // Execute Socket Command
	  int result = ghRailExecuteCommand(cmdqueue,client_fd,&ghrail,&ghViewer,ghSky,ghSimulationTime);
	  switch (result) {
	  case GH_POST_EXECUTE_EXIT:
	    close(client_fd);
	    exit(0);
	    break;
	  case GH_POST_EXECUTE_CLOSE:
	    //close(client_fd);
	    return -1;
	    break;
	  case GH_POST_EXECUTE_SETCLOCK:
	    _elapsed_sec = 0.0f;
	    break;
	  case GH_POST_EXECUTE_TIMEZONE:
	    ghGui->setTimeZone( ghrail.GetTimeZoneMinutes() );
	    break;
	  }

        } // End of while loop ( rendering loop )
	//
	//    
	ghSock.join();
    }
    else
    {
      //return usage(argv[0]);
      return -1;
    }
  
}

 
int
main(int argc, char** argv)
{
    osg::ArgumentParser arguments(&argc, argv);
    if (arguments.read("--help"))
        return usage(argv[0]);

    fprintf( stderr,"--------------------------------------------\n" ) ;
    fprintf( stderr,"    %s\n", GH_WELCOME_MESSAGE ) ;
    fprintf( stderr,"    Revision %s\n", GH_REVISION ) ;
    fprintf( stderr,"--------------------------------------------\n" ) ;

    osgEarth::initialize(arguments);

    //
    //Setup our main view that will show the loaded earth file.
    //
    unsigned int MapWidth = 0;
    unsigned int MapHeight = 0;
    osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
    if ( wsi )
      wsi->getScreenResolution( osg::GraphicsContext::ScreenIdentifier(0), MapWidth, MapHeight );

    // Limit 4K
    if ( MapWidth > 3840 ) MapWidth = 3840;
    if ( MapHeight > 2160 ) MapHeight = 2160;
    

    signal( SIGCHLD, signalChild ) ;
    int	server_fd ;
    if ((server_fd = socketInit(listen_port)) < 0)
    {
      exit( -1 ) ;
    }
    
    struct sockaddr_in	client_addr ;
    socklen_t client_len = sizeof( client_addr ) ;
    int	proc_id ;		/*	Process Identifier	*/

    for (;;) 
    {
	if ((client_fd = accept( server_fd, (struct sockaddr *)&client_addr, &client_len )) < 0)
	{
	  //sprintf( "daemonloop: cannot accept new socket" ) ;
	  break ;
	}

	proc_id = fork();
	if (proc_id < 0)
	{
	  //sprintf( "process : cannot fork child process" ) ;
	  exit(1);
	}

	if (proc_id == 0)
	{

	  /*
	   *  fork Child
	   */
	  close( server_fd ) ;

	  //sprintf( line, "%s's child started\n", rend_progName ) ;
	  //////////////////
	  mainloop(arguments,MapWidth,MapHeight);
	  //////////////////
	  close( client_fd ) ;
	  exit(1);
	  
	} else {
	  
	  /*
	   *  fork Parent > 0
	   */

	  close( client_fd ) ;
	  sleep(1);

	}
    } /* end of  for (;;) */
}

