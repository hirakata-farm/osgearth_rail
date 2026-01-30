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
# ifndef _GH_RAIL_TIME_HPP_
# define _GH_RAIL_TIME_HPP_
     
# include <iostream>
# include <cstdlib>
# include <cstring>
# include <time.h>
# include <chrono>
#include <osgEarth/Sky>

class ghRailTime
    {
    public:
      void Init();
      std::chrono::system_clock::time_point GetBaseTimePoint();
      osgEarth::DateTime GetBaseDatetime();
      double StrToDurationSeconds(std::string largetime,std::string smalltime);
      double StrToDurationSecondsFromBasetime(std::string largetime);
      void SetTimeZone(std::string timestr);
      int  GetTimeZoneMin();
    private:
      std::chrono::system_clock::time_point p_basetimepoint;
      osgEarth::DateTime p_basedatetime;
      int p_timezoneminutes;
      std::chrono::system_clock::time_point _str2timepoint(std::string str);
      //time_t _str2time_t_err(std::string str,time_t base);
      //time_t _str2time_t(std::string str);
    };


#endif

