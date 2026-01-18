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


# ifndef _GH_STRING_HPP_
# define _GH_STRING_HPP_

#include <iostream>
#include <algorithm>
#include <vector>
#include <cstring>

char *ghString2CharPtr( const std::string& str );
std::string ghStringTrim(const std::string& str);
std::vector<std::string> ghStringSplit(const std::string& str, char delimiter);

#endif
