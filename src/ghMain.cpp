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
#include <osgEarth/ExampleResources>
#include <osgEarth/Sky>
#include <osgViewer/Viewer>
#include <osgViewer/CompositeViewer>
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

#include <signal.h>
#include <thread>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>


#define LC "[imgui] "

using namespace osgEarth;
using namespace osgEarth::Util;

int
ghUsage(const char* name)
{
    OE_NOTICE
        << "\nUsage: " << name << " file.earth" << std::endl
        << MapNodeHelper().usage() << std::endl;
    return 0;
}

void
ghShowPlugins()
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

///////////////////////////////////////////////////////////////////
//
//  Network Variables
//
ghCommandQueue *cmdqueue = (ghCommandQueue *)NULL ;
int client_fd;
int listen_port = GH_DEFAULT_SOCKET_PORT;
//std::mutex ghMutex;
/////////////////////

/*
 *   Child Process quit.
 *    cleanup defunct process
 */
void
ghSignalChild( int sig )
{
  int child_pid = 0;
  do {
    int status;
    child_pid = waitpid(-1,&status,WNOHANG);
  } while(child_pid>0);

} 
/*
 * interrupt handling routine
 */
void
ghSignalQuit( int sig )
{
    signal( SIGINT, SIG_IGN ) ;
    signal( SIGBUS, SIG_IGN ) ;
    signal( SIGSEGV, SIG_IGN ) ;
    signal( SIGQUIT, SIG_IGN ) ;
    signal( SIGHUP, SIG_IGN ) ;
    signal( SIGTERM, SIG_IGN ) ;
}

/*
 * Child process quit routine
 */
void
ghChildQuit( int sig )
{

    fprintf( stderr, "%s's child ID %d exitted\n", GH_APP_NAME , getpid()) ;
    
    exit( 0 ) ;

}
/**                  **/
int
ghSocketInit(int port)
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

/////////////////////////////////////////
//
//  Rendering Variables
//
osgViewer::CompositeViewer *ghView_anchor;  //  Application Root
ghWindow *ghWin_anchor;           // 3D window 
ghRailGUI *ghGui;                 // ImGui 
ghRail ghRail3D;                  // Simulation    Rail Class 
osg::ref_ptr<osg::Node> ghNode3D; // osgearth root node
osgEarth::SkyNode *ghSky;         // manipulate date-time

double ghSimulationTime = 0.0;
double ghElapsedSec = 10.0f;
unsigned int ghScreenWidth = 0;
unsigned int ghScreenHeight = 0;
unsigned int ghScreenNum = 0;
bool ghUpdateState = true;

void
ghSocketThread() {

  char buffer[1024] = {0};
  ghCommandQueue *cmdtmp = (ghCommandQueue *)NULL ;
  bool isThread = true;
  bool prevState = false;
  
  while (isThread)
    {
      // Wait for client receive message
      read(client_fd, buffer, 1024);
      std::string command(buffer);
      if ( command.size() > 3 ) {
	cmdtmp = cmdqueue;

	// Parse Socket Command
	cmdqueue = ghRailParseCommand(command);
	if ( cmdqueue != NULL ) {
	  cmdqueue->prev = cmdtmp;
	  
	  memset(buffer, 0, sizeof(buffer));
	    
	  //if ( cmdqueue->type == GH_COMMAND_CLOSE ||
	  //     cmdqueue->type == GH_COMMAND_EXIT ) {
	  //  break;
	  //}
	}
      } else {
	// Too short buffer
      }

      if ( cmdqueue->isexecute == false ) {

	// Execute Socket Command
	int result = ghRailExecuteCommand(cmdqueue,client_fd,&ghRail3D,ghWin_anchor,ghSky,ghSimulationTime);
	ghUpdateState = false;
	if ( result == GH_POST_EXECUTE_EXIT ) {
	  isThread = false;
	  break;
	} else if ( result == GH_POST_EXECUTE_CLOSE ) {
	  isThread = false;
	  break;
	} else if ( result == GH_POST_EXECUTE_SETCLOCK ) {
	  ghElapsedSec = 0.0f;
	} else if ( result == GH_POST_EXECUTE_TIMEZONE ) {
	  ghGui->setTimeZone( ghRail3D.GetTimeZoneMinutes() );	    
	} else if ( result == GH_POST_EXECUTE_CAMERA_ADD ) {
	  ghWindow *nwin = ghAddNewWindow( ghWin_anchor, cmdqueue->argstr[0],0,0,floor(ghScreenWidth/2),floor(ghScreenHeight/2));
	  if ( nwin != NULL ) {
	    prevState = ghRail3D.IsPlaying();
	    if ( prevState ) ghRail3D.SetPlayPause(false);
	    ghView_anchor->addView( nwin->view );
	    nwin->view->setSceneData( ghNode3D );
	    ghSetWindowTitle(ghView_anchor,cmdqueue->argstr[0]);
	    if ( prevState ) ghRail3D.SetPlayPause(true);
	  }
	} else if ( result == GH_POST_EXECUTE_CAMERA_REMOVE ) {
	  ghWindow *rwin = ghGetWindowByName(ghWin_anchor, cmdqueue->argstr[0]);
	  if ( rwin != NULL ) {
	    prevState = ghRail3D.IsPlaying();
	    if ( prevState ) ghRail3D.SetPlayPause(false);
	    // https://osg-users.openscenegraph.narkive.com/BKtcS0HA/adding-removing-views-from-compositeviewer
	    rwin->view->getCamera()->setNodeMask(0x0);
	    ghView_anchor->removeView( rwin->view );
	    ghRemoveWindow( ghWin_anchor, cmdqueue->argstr[0] );
	    if ( prevState ) ghRail3D.SetPlayPause(true);
	  }
	} else {
	  // NOP
	}
	ghUpdateState = true;
      }
    }

  ghRail3D.RemoveShm(0);
  ghDisposeWindow( ghView_anchor, ghWin_anchor );
  ghChildQuit( SIGQUIT ) ;

  //  End of client socket read loop
}

int
ghMainLoop(osg::ArgumentParser args)
{

  osgViewer::CompositeViewer ghViewer(args);  //  Application Root
  ghView_anchor = &ghViewer;
  
  ghViewer.setThreadingModel(osgViewer::CompositeViewer::ThreadPerCamera);
  /*
    SingleThreaded 	
    CullDrawThreadPerContext 	
    ThreadPerContext 	
    DrawThreadPerContext 	
    CullThreadPerCameraDrawThreadPerContext 	
    ThreadPerCamera 	
    AutomaticSelection 
   */
  ghViewer.setRealizeOperation(new ImGuiAppEngine::RealizeOperation);

  ghWin_anchor = ghCreateNewWindow(GH_STRING_ROOT,0,0,ghScreenWidth,ghScreenHeight);
  ghViewer.addView( ghWin_anchor->view );
  /***  Set window name ***/
  ghSetWindowTitle(&ghViewer,GH_WELCOME_MESSAGE);

  /**   Load the earth file  **/
  ghNode3D = MapNodeHelper().load(args, &ghViewer);
    
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

      /***  Use half of threads **/
      int available_threads = (int)std::thread::hardware_concurrency();
      available_threads = available_threads / 2.0;
      if ( available_threads < 1 ) available_threads = 1;
      jobs::get_pool("oe.rex.loadtile")->set_concurrency(available_threads);

      /***  map node  **/
      osgEarth::MapNode* mapNode = osgEarth::MapNode::findMapNode(ghNode3D);
      //std::cout << mapNode->getMapSRS()->getGeographicSRS() << std::endl;

      /***  Sky and date time **/
      ghSky = SkyNode::create();
      ghSky->setDateTime(DateTime());
      ghSky->setSimulationTimeTracksDateTime(true);
      ghSky->setAtmosphereVisible(true);
      ghSky->getSunLight()->setAmbient(osg::Vec4(GH_SUN_AMBIENT, GH_SUN_AMBIENT, GH_SUN_AMBIENT, 1.0f));
      ghSky->setLighting(true);
      ghSky->setSunVisible(true);
      ghSky->setMoonVisible(false);
      ghSky->setStarsVisible(true);
      auto parent = mapNode->getParent(0);
      ghSky->addChild(mapNode);
      parent->addChild(ghSky);
      parent->removeChild(mapNode);
      /***  Sky and date time **/

      ui->onStartup = []()
      {
	ImGui::GetIO().FontAllowUserScaling = true;
      };
      ghWin_anchor->view->getEventHandlers().push_front(ui);
      ghWin_anchor->view->setSceneData( ghNode3D );

      ghGui = new ghRailGUI();
      ui->add("Clock", ghGui );
      ghRail3D.SetClockSpeed(1.0);
      ghRail3D.SetPlayPause(false);
      std::thread ghSock(ghSocketThread);
      /////////////////////////////////////////////  Socket LOOP
    
      double _elapsed_prev = 0.0f;    // Important
      double _elapsed_current = ghViewer.elapsedTime(); // Important

      while (!ghViewer.done())
        {
	  _elapsed_current = ghViewer.elapsedTime() ; // double [sec]
	  if ( ghRail3D.IsPlaying() ) {
	    double elapsed =  ( _elapsed_current - _elapsed_prev ) * ghRail3D.GetClockSpeed(); // duration seconds
	    DateTime dt = ghSky->getDateTime();
	    ghSimulationTime = _calcSimulationTime(dt,ghRail3D.GetBaseDatetime(), ghElapsedSec);

	    // Simulation Update
	    ghRail3D.Update( ghSimulationTime, mapNode , ghWin_anchor);

	    if ( ghElapsedSec > GH_ELAPSED_THRESHOLD ) {
	      // Change Date Time Dislpay
	      ghSky->setDateTime(_calcElapsedTime(dt,ghElapsedSec));
	      ghElapsedSec = 0.0f;
	    } else {
	      ghElapsedSec += elapsed;
	    }

	  } else {
	    
	    // Not Playing
	  }

	  if ( ghUpdateState ) {
	    _elapsed_prev = _elapsed_current;
	  
	    // Frame Update
	    ghViewer.frame();
	  }

        } // End of while loop ( rendering loop )
	//
	//
	ghSock.join();
	return 1;
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
        return ghUsage(argv[0]);
    fprintf( stderr,"\n--------------------------------------------\n" ) ;
    fprintf( stderr,"    %s\n", GH_WELCOME_MESSAGE ) ;
    fprintf( stderr,"    %s rev %s\n", GH_APP_NAME, GH_APP_REVISION ) ;
    fprintf( stderr,"--------------------------------------------\n" ) ;

    osgEarth::initialize(arguments);

    //
    //Setup our main view that will show the loaded earth file.
    //
    osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
    if ( wsi )
      wsi->getScreenResolution( osg::GraphicsContext::ScreenIdentifier(ghScreenNum), ghScreenWidth, ghScreenHeight );
    //
    ////////////////////////////////////////

    signal( SIGCHLD, ghSignalChild ) ;
    int	server_fd ;
    if ((server_fd = ghSocketInit(listen_port)) < 0)
    {
      fprintf( stderr,"Cannot Initialize Socket, exited\n" ) ;
      exit( -1 ) ;
    }
    
    struct sockaddr_in	client_addr ;
    socklen_t client_len = sizeof( client_addr ) ;
    int	proc_id ;		/*	Process Identifier	*/

    for (;;) 
    {
	if ((client_fd = accept( server_fd, (struct sockaddr *)&client_addr, &client_len )) < 0)
	{
	  fprintf( stderr,"Cannot accept new socket\n" ) ;
	  break ;
	}

	proc_id = fork();
	if (proc_id < 0)
	{
	  fprintf( stderr,"Cannot fork child process, exited\n" ) ;
	  exit(1);
	}

	if (proc_id == 0)
	{

	  /* fork Child */
	  if (signal( SIGINT, SIG_IGN ) != SIG_IGN)
	    signal( SIGINT, ghSignalQuit ) ;
	  if (signal( SIGBUS, SIG_IGN ) != SIG_IGN)
	    signal( SIGBUS, ghSignalQuit ) ;
	  if (signal( SIGSEGV, SIG_IGN ) != SIG_IGN)
	    signal( SIGSEGV, ghSignalQuit ) ;
	  if (signal( SIGQUIT, SIG_IGN ) != SIG_IGN)
	    signal( SIGQUIT, ghSignalQuit ) ;
	  if (signal( SIGHUP, SIG_IGN ) != SIG_IGN)
	    signal( SIGHUP, ghSignalQuit ) ;
	  if (signal( SIGTERM, SIG_IGN ) != SIG_IGN)
	    signal( SIGTERM, ghSignalQuit ) ;
	  if (signal( SIGPIPE, SIG_IGN ) != SIG_IGN)
	    signal( SIGPIPE, ghSignalQuit ) ;
	  close( server_fd ) ;

	  //////////////////
	  ghMainLoop(arguments);
	  //////////////////
	  close( client_fd ) ;

	  ghChildQuit( SIGQUIT ) ;
	  
	} else {
	  
	  /*
	   *  fork Parent > 0
	   */

	  close( client_fd ) ;
	  sleep(1);

	}
    } /* end of  for (;;) */
}

