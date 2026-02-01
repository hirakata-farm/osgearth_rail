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
#include <osgEarthImGui/ImGuiApp>
#include <osgEarth/EarthManipulator>
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
#include <OpenThreads/Thread>
#include <OpenThreads/Mutex>

#ifdef _WINDOWS
#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

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
ghCommandQueue *ghQueue = (ghCommandQueue *)NULL ;
int ghClient = 0;
int listen_port = GH_DEFAULT_SOCKET_PORT;

/////////////////////
#ifndef _WINDOWS
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
#endif

/*
 * Child process quit routine
 */
void
ghChildQuit( int sig )
{

    fprintf( stderr, "%s's child ID %d exitted\n", GH_APP_NAME , getpid()) ;
    
    exit( 0 ) ;

}

#ifdef _WINDOWS
/**  Windows Socket   **/
/* https://www.geekpage.jp/programming/winsock/tcp.php */
WSADATA wsaData;
int
ghSocketInit(int port)
{
  int fd;
  int iResult;
  struct sockaddr_in address;

  iResult = WSAStartup(MAKEWORD(2,2),&wsaData);
  if ( iResult != 0 ) {
    perror("WSAStartup failed");
    exit(EXIT_FAILURE);
  }

  fd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  if ( fd == INVALID_SOCKET ) {
    perror("socket failed");
    WSACleanup();
    exit(EXIT_FAILURE);
  } else {
    // NOP
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  // https://prupru-prune.hatenablog.com/entry/2023/12/06/021702
  iResult = ::bind(fd, (struct sockaddr *)&address, sizeof(address) );
  if ( iResult == SOCKET_ERROR ) {  
    perror("winsock2 bind failed");
    printf("Bind faild %d\n", WSAGetLastError());
    closesocket(fd);
    WSACleanup();
    exit(EXIT_FAILURE);
  }

  if (listen(fd, SOMAXCONN) < 0) {
    perror("listen");
    closesocket(fd);
    WSACleanup();
    exit(EXIT_FAILURE);
  }

  return fd;
}
#else
/**  Unix Socket   **/
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
#endif


/////////////////////////////////////////
//
//  Rendering Variables
//
osgViewer::CompositeViewer *ghViewer;  //  Application Root
ghWindow *ghWin_anchor;           // 3D window 
ghRailGUI *ghGui;                 // ImGui 
ghRail *ghRail3D;                  // Simulation    Rail Class 
osg::ref_ptr<osg::Node> ghNode3D; // osgearth root node
osgEarth::SkyNode *ghSky;         // manipulate date-time
//OpenThreads::Mutex ghMutex;

double ghSimulationTime = 0.0;
double ghElapsedSec = 10.0f;
unsigned int ghScreenNum = 0;

void
ghCheckCommand() {

  if ( ghQueue == (ghCommandQueue *)NULL ) return;
  if ( ghQueue->state == GH_QUEUE_STATE_RESULT_SEND ) {
    if ( ghQueue->type == GH_COMMAND_EXIT || ghQueue->type == GH_COMMAND_CLOSE ) {
      ghViewer->setDone(true);
    }
    return;
  }
  // All Command Already Finished

  ghCommandQueue *cmdtmp = ghQueue;
  ghCommandQueue *cmd = (ghCommandQueue *)NULL;

  while (cmdtmp != (ghCommandQueue *)NULL)
    {
      if ( cmdtmp->state == GH_QUEUE_STATE_PARSED ) {
	cmd = cmdtmp;
	break;
      }
      cmdtmp = cmdtmp->prev ;
    }

  if ( cmd == (ghCommandQueue *)NULL ) return;

  ghRailExecuteCommandOSG(cmd,ghRail3D,ghWin_anchor,ghSky);

  if ( cmd->state == GH_QUEUE_STATE_PART_EXECUTED ) {
    if ( cmd->type == GH_COMMAND_CLOCK_SET_TIME ) {
      ghElapsedSec = 0.0f;
      cmd->state = GH_QUEUE_STATE_EXECUTED;
    } else if ( cmd->type == GH_COMMAND_FIELD_SET  ) {
      ghSky->setDateTime( ghRail3D->GetBaseDatetime());
      ghGui->setTimeZone( ghRail3D->GetTimeZoneMinutes() );
      cmd->state = GH_QUEUE_STATE_EXECUTED;
    } else if ( cmd->type == GH_COMMAND_CAMERA_ADD  ) {
      ghWindow *nwin = ghAddWindow( ghWin_anchor,
				    cmd->argstr[0],
				    ghScreenNum,
				    (unsigned int)cmd->argnum[0],
				    (unsigned int)cmd->argnum[1],
				    cmd->argnum[2]);
      if ( nwin != NULL ) {
	ghViewer->addView( nwin->view );
	nwin->view->setSceneData( ghNode3D );
      } else {
	// Error Message
      }
      cmd->state = GH_QUEUE_STATE_EXECUTED;
    } else if ( cmd->type == GH_COMMAND_CAMERA_REMOVE  ) {
      ghWindow *rwin = ghGetWindowByName(ghWin_anchor, cmd->argstr[0]);
      if ( rwin != NULL ) {
	// https://osg-users.openscenegraph.narkive.com/BKtcS0HA/adding-removing-views-from-compositeviewer
	rwin->view->getCamera()->setNodeMask(0x0);
	ghViewer->removeView( rwin->view );
	ghRemoveWindow( ghWin_anchor, cmd->argstr[0] );
      } else {
	// Error Message
      }
      cmd->state = GH_QUEUE_STATE_EXECUTED;
    } else {
      // NOP
    }
  }

}

//
// Create a thread class by inheriting from OpenThreads::Thread
//
class SocketReceiveThread : public OpenThreads::Thread {
public:
  SocketReceiveThread(int fd) : _running(true),_buffer{0},_cmdtmp(nullptr),recv_result(0),_sock(fd) {}

  // The run() method contains the code to be executed in the thread
  virtual void run() {
    while (_running) {
      //std::cout << "Thread  is running." << std::endl;
      // Sleep for 100 milliseconds
      //OpenThreads::Thread::microSleep(100000);
#ifdef _WINDOWS
      recv_result = recv(_sock, _buffer, 1024, 0);
#else      
      read(_sock, _buffer, 1024);
#endif
      std::string command(_buffer);
      if ( command.size() > 3 ) {
	_cmdtmp = ghQueue;
	ghQueue = ghRailInitCommandQueue();
	if ( ghQueue != NULL ) {
	  ghRailParseCommand(ghQueue,command);
	  if ( ghQueue->type == GH_COMMAND_UNKNOWN ) {
	    ghQueue->state = GH_QUEUE_STATE_RECEIVED;
	  } else {
	    ghQueue->state = GH_QUEUE_STATE_PARSED;
	    ghQueue->prev = _cmdtmp;
	  }
	  memset(_buffer, 0, sizeof(_buffer));
	} else {
	  ghQueue = _cmdtmp;
	  // Error message ( cannot allocate command queue )
	}
      } else {
	// Too short buffer
      }
	  
      if ( ghQueue ) {
	if ( ghQueue->state == GH_QUEUE_STATE_PARSED ) {
	  ghRailExecuteCommandData(ghQueue,ghRail3D,ghSimulationTime);
	  if ( ghQueue->type == GH_COMMAND_EXIT || ghQueue->type == GH_COMMAND_CLOSE ) {
	    _running = false;
	  } else {
	    // NOP
	  }
	}

      } else {
	// NOP
	//std::cout<<"CommandQueue NULL="<<command<<std::endl;
      }
    }
    //std::cout << "Thread stopped." << std::endl;
    ghRail3D->SetPlayPause(false);
  }

void stop() { _running = false; }

private:
  bool _running;
  char _buffer[1024];
  ghCommandQueue *_cmdtmp;
  int recv_result;
  int _sock;
};
///////////////////////////////////////////////////////////////
class SocketSendThread : public OpenThreads::Thread {
public:
  SocketSendThread(int fd,int msec) : _running(true),_waittime(msec),_sock(fd) {}

  // The run() method contains the code to be executed in the thread
  virtual void run() {
    while (_running) {
      //std::cout << "Thread  is running." << std::endl;
      // Sleep for XX microseconds
      OpenThreads::Thread::microSleep(_waittime);

      ghCommandQueue *cmdtmp = ghQueue;
      ghCommandQueue *cmd = (ghCommandQueue *)NULL;
      while (cmdtmp != (ghCommandQueue *)NULL)
	{
	  if ( cmdtmp->state == GH_QUEUE_STATE_EXECUTED ) {
	    cmd = cmdtmp;
	  }
	  cmdtmp = cmdtmp->prev ;
	}
      if ( cmd != (ghCommandQueue *)NULL ) {
	if ( cmd->state == GH_QUEUE_STATE_EXECUTED && cmd->result != GH_STRING_NOP ) {
	  const char* retmsg = cmd->result.c_str();
	  send(_sock, retmsg, std::strlen(retmsg), 0);
	  cmd->state = GH_QUEUE_STATE_RESULT_SEND;
	} else {
	  std::cout << ".";
	  // NOP
	  //  Unknown Error
	  //std::cout << "Unknown command cmd=" << cmd << std::endl;
	  //std::cout << cmd->argstr[0] << std::endl;
	  //std::cout << cmd->argstr[1] << std::endl;  
	  //std::cout << cmd->result << std::endl;    
	  //std::cout << "             type=" << cmd->type << std::endl;
	  //std::cout << "            state=" << cmd->state << std::endl;
	}
      }
    }
  }

void stop() { _running = false; }

private:
  bool _running;
  int _waittime;
  int _sock;
};
///////////////////////////////////////////////////////

int
ghMainRail(osg::ArgumentParser args)
{
  
  osgEarth::initialize(args);

  ghRail3D = new ghRail();
  ghGui = new ghRailGUI();
  ghRail3D->Init();
  ghRail3D->SetClockSpeed(1.0);
  ghRail3D->SetPlayPause(false);
      
  ghViewer = new osgViewer::CompositeViewer(args);  //  Application Root
  
  //ghViewer.setThreadingModel(ghViewer.SingleThreaded);
  //ghViewer.setThreadingModel(ghViewer.ThreadPerCamera);
  //ghViewer->setThreadingModel(osgViewer::CompositeViewer::SingleThreaded);  
  ghViewer->setThreadingModel(osgViewer::CompositeViewer::ThreadPerCamera);
  //ghViewer.setThreadingModel(osgViewer::CompositeViewer::DrawThreadPerContext);
  //ghViewer.setThreadingModel(osgViewer::CompositeViewer::ThreadPerContext);

  /*
    SingleThreaded 	
    CullDrawThreadPerContext 	
    ThreadPerContext 	
    DrawThreadPerContext 	
    CullThreadPerCameraDrawThreadPerContext 	
    ThreadPerCamera 	
    AutomaticSelection 
   */
  //ghViewer->setRealizeOperation(new ImGuiAppEngine::RealizeOperation);

  /**   Load the earth file  **/
  ghNode3D = MapNodeHelper().load(args, ghViewer);
    

  if (ghNode3D.valid())
    {

#ifdef _WINDOWS
      // NOP
#else
      /***  Use half of threads **/
      int available_threads = (int)std::thread::hardware_concurrency();
      available_threads = available_threads / 2.0;
      if ( available_threads < 1 ) available_threads = 1;
      jobs::get_pool("oe.rex.loadtile")->set_concurrency(available_threads);
#endif
      
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

      ui->add("Clock", ghGui );

      ui->onStartup = []()
      {
	ImGui::GetIO().FontAllowUserScaling = true;
      };

      /***  Set window view ***/
      ghWin_anchor = ghCreateWindow(GH_STRING_ROOT,&args,ghScreenNum,64,48,0.5);
      ghViewer->addView( ghWin_anchor->view );
      ghWin_anchor->view->getEventHandlers().push_front(ui);
      ghWin_anchor->view->setSceneData( ghNode3D );

      double _elapsed_prev = 0.0f;
      double _elapsed_current = ghViewer->elapsedTime();

      /////////////////////////
      SocketReceiveThread* rsock = new SocketReceiveThread(ghClient);
      SocketSendThread* ssock = new SocketSendThread(ghClient,GH_SOCKET_SEND_WAIT); 
      rsock->start();
      ssock->start();  
      /////////////////////////

      ///////////////////
      //
      //  Rendering Loop
      //
      while (!ghViewer->done())
        {
	  _elapsed_current = ghViewer->elapsedTime() ; // double [sec]
	  if ( ghRail3D->IsPlaying() ) {
	    double elapsed =  ( _elapsed_current - _elapsed_prev ) * ghRail3D->GetClockSpeed(); // duration seconds
	    DateTime dt = ghSky->getDateTime();
	    ghSimulationTime = _calcSimulationTime(dt,ghRail3D->GetBaseDatetime(), ghElapsedSec);

	    // Simulation Update
	    ghRail3D->Update( ghSimulationTime, mapNode , ghWin_anchor);

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

	  _elapsed_prev = _elapsed_current;
	  
	  // Frame Update
	  ghViewer->frame();

	  ghCheckCommand();
	  
        }
      //
      // End of while loop ( Rendering loop )
      ghRail3D->RemoveShm(0);
      ghDisposeWindow( ghViewer, ghWin_anchor );
      ////////////////////////
      rsock->stop();
      rsock->join();
      delete rsock;
      ssock->stop();
      ssock->join();
      delete ssock;
      ////////////////////////

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
    //ghArgs = new osg::ArgumentParser(&argc, argv);
  
#ifdef _WINDOWS
    // Nop signal
#else    
    signal( SIGCHLD, ghSignalChild ) ;
#endif    
    int	server_fd ;
    if ((server_fd = ghSocketInit(listen_port)) < 0)
    {
      fprintf( stderr,"Cannot Initialize Socket, exited\n" ) ;
      exit( -1 ) ;
    }
    
    struct sockaddr_in	client_addr ;
#ifdef _WINDOWS
    int client_len = sizeof( client_addr ) ;
#else    
    socklen_t client_len = sizeof( client_addr ) ;
    int	proc_id ; /* Process Identifier */
#endif
    
    for (;;) 
    {

#ifdef _WINDOWS
      //  Does not fork for Windows
      ghClient = accept( server_fd, (struct sockaddr *)&client_addr, &client_len );
      if (ghClient == INVALID_SOCKET )
	{
	  fprintf( stderr,"Cannot accept new socket\n" ) ;
	  break ;
	}
#else      
      if ((ghClient = accept( server_fd, (struct sockaddr *)&client_addr, &client_len )) < 0)
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

#endif
	  //////////////////
	  ghMainRail(arguments);
	  //////////////////

#ifdef _WINDOWS	  
	  closesocket( ghClient );
	  ghChildQuit( 0 ) ;
#else
	  close( ghClient ) ;
	  ghChildQuit( SIGQUIT ) ;
	  
	} else {
	  
	  /*
	   *  fork Parent > 0
	   */

	  close( ghClient ) ;
	  sleep(1);

	}

#endif
	
    } /* end of  for (;;) */
}

