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

char *
ghString2CharPtr( const std::string& str ) {
  char* cstr = new char[str.size() + 1]; // allocation memory
  std::strcpy(cstr, str.c_str());
  return cstr;
}

std::string
ghStringTrim(const std::string& str) {
  auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
  auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
  return (start < end ? std::string(start, end) : "");
}

std::vector<std::string>
ghStringSplit(const std::string& str, char delimiter) {
  std::vector<std::string> result;
  size_t start = 0;
  size_t end = str.find_first_of(delimiter);

  while (start < str.size()) {
    result.push_back(ghStringTrim(str.substr(start, end - start)));
    start = end + 1;
    end = str.find_first_of(delimiter, start);
    if (end == std::string::npos) {
      end = str.size();
    }
  }
  return result;
}
