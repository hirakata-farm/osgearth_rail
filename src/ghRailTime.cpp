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

# include "ghRailTime.hpp"

using namespace std;

void
ghRailTime::Init() 
{

  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  struct tm *time_struct;
  time_struct = gmtime(&time);

  std::tm timeinfo = {};
  //  timeinfo.tm_year = 2025 - 1900; // 年 - 1900
  //  timeinfo.tm_mon = 0; // 月 (0-11)
  //  timeinfo.tm_mday = 1; // 日
  timeinfo.tm_year = time_struct->tm_year;
  timeinfo.tm_mon = time_struct->tm_mon;
  timeinfo.tm_mday = time_struct->tm_mday;
  timeinfo.tm_hour = 0; // 時
  timeinfo.tm_min = 0; // 分
  timeinfo.tm_sec = 0; // 秒
  
  p_basetimepoint = std::chrono::system_clock::from_time_t(std::mktime(&timeinfo));
  // UTC timepoint 

  int year = timeinfo.tm_year + 1900;
  int month = timeinfo.tm_mon + 1;
  int days = timeinfo.tm_mday;

  p_basedatetime = osgEarth::DateTime(year, month, days, 0.0);

  // UNIX時間を取得 (エポックからの秒数)
  auto epoch = p_basetimepoint.time_since_epoch();
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);
  
}

std::chrono::system_clock::time_point
ghRailTime::GetBaseTimePoint() {
  return p_basetimepoint;
}

osgEarth::DateTime
ghRailTime::GetBaseDatetime() {
  return p_basedatetime;
}

void
ghRailTime::SetTimeZone(std::string timestr) {

  int hour,min;
  
  // 
  // field file data = "timezone":"+09:00"
  // when japan , return -540;
  //
  sscanf( timestr.c_str(),"%d:%d",&hour,&min);

  p_timezoneminutes = -1 * ( hour * 60 ) + min;
  
}

int
ghRailTime::GetTimeZoneMin() {
  return p_timezoneminutes;
}

double
ghRailTime::StrToDurationSeconds(string largetime,string smalltime) 
{

  chrono::system_clock::time_point current = _str2timepoint(largetime);
  chrono::system_clock::time_point prev = _str2timepoint(smalltime);
  std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(current - prev);
  return (double)sec.count();
}
double
ghRailTime::StrToDurationSecondsFromBasetime(string largetime) 
{
  chrono::system_clock::time_point current = _str2timepoint(largetime);
  std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(current - p_basetimepoint);
  return (double)sec.count();
}


//構造体 tm はtime.h の中で宣言され、以下の情報を含みます
//struct tm {
//  int tm_sec;      /* 秒 [0-61] 最大2秒までのうるう秒を考慮 */
//  int tm_min;      /* 分 [0-59] */
//  int tm_hour;     /* 時 [0-23] */
//  int tm_mday;     /* 日 [1-31] */
//  int tm_mon;      /* 月 [0-11] 0から始まることに注意 */
//  int tm_year;     /* 年 [1900からの経過年数] */
//  int tm_wday;     /* 曜日 [0:日 1:月 ... 6:土] */
//  int tm_yday;     /* 年内の通し日数 [0-365] 0から始まることに注意*/
//  int tm_isdst;    /* 夏時間フラグ　[夏時間を採用しているときに正、採用していないときに 0、この情報が得られないときに負] */
//};


chrono::system_clock::time_point
ghRailTime::_str2timepoint(string str)
{
  int dday = 0;
  int dhour = 0;
  int dmin = 0;
  int dsec = 0;
  
  struct tm t;
  sscanf( str.c_str(),
	  "%dT%d:%d:%d",
	  &dday,
	  &dhour,
	  &dmin,
	  &dsec);

  int daysec = dday * 86400 + dhour * 3600 + dmin * 60 + dsec + ( p_timezoneminutes * 60 );

  std::chrono::seconds diffsec( daysec );

  chrono::system_clock::time_point new_time_point = p_basetimepoint + diffsec;

  return new_time_point;
}


