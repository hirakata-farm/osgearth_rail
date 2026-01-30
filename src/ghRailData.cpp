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

# include <iostream>
# include <fstream>

# include <curlpp/cURLpp.hpp>
# include <curlpp/Easy.hpp>
# include <curlpp/Options.hpp>

# include <nlohmann/json.hpp>

# include "ghRailData.hpp"

#ifdef _WINDOWS
#include <windows.h>
#include <algorithm>
#endif

using namespace std;
using namespace cURLpp::Options;

void
ghRailJSON::SetConfigUri(string host, string url)
{
  p_host = host;
  p_url = host + GEOGLYPH_CONF_PATH + url + GEOGLYPH_CONF_EXT;
}

void
ghRailJSON::SetFieldUri(string host, string url)
{
  p_host = host;
  p_url = host + GEOGLYPH_RSC_FIELD_PATH + url;
}

void
ghRailJSON::SetLineUri(string host, string line)
{
  p_host = host;
  p_url = host + GEOGLYPH_RSC_PATH + line;
}

void
ghRailJSON::SetLocomotiveUri(string host, string locomotive)
{
  p_host = host;
  p_url = host + GEOGLYPH_3DMODEL_PATH + locomotive;
}

bool
ghRailJSON::GetContent()
{
  try {
    cURLpp::Cleanup cleaner;
    cURLpp::Easy    req;

    WriterMemoryClass mWriterChunk;

    // Set the writer callback to enable cURL 
    // to write result in a memory area
    using namespace std::placeholders;
    curlpp::types::WriteFunctionFunctor functor = std::bind(&WriterMemoryClass::WriteMemoryCallback, &mWriterChunk, _1, _2, _3);
    curlpp::options::WriteFunction *test = new curlpp::options::WriteFunction(functor);
    req.setOpt(test);

    req.setOpt(new Url(p_url));

    req.perform();

    //mWriterChunk.print();//  for DEBUG
    p_json = mWriterChunk.getJSON();
    if ( p_json.dump() == "{}" ) {
      p_json = NULL;
      return false;
    }
    
  } catch (cURLpp::LogicError & e) {
    return false;
  } catch (cURLpp::RuntimeError & e) {
    return false;
  }
  return true;
}

nlohmann::json
ghRailJSON::GetJson()
{
  return p_json;
}

nlohmann::json
ghRailJSON::GetJsonObject( string str0 )
{
  return p_json[str0];
}

nlohmann::json
ghRailJSON::GetJsonObject( string str0, string str1 )
{
  return p_json[str0][str1];
}

std::string
ghRailJSON::GetJsonString( string str0 )
{
  return p_json[str0].get<std::string>();
}

std::string
ghRailJSON::GetJsonString( string str0, string str1 )
{
  return p_json[str0][str1].get<std::string>();
}

int
ghRailJSON::GetVectorSize( string str )
{
  size_t count = 0;
  nlohmann::json json = GetJsonObject( str );
  for (nlohmann::json::iterator itss = json.begin(); itss != json.end(); ++itss) {
    count++;
  }
  return count;
}

vector<int>
ghRailJSON::GetVectorInt( string str )
{

  vector<int> p_array;
  nlohmann::json json = GetJsonObject( str );
  p_array.reserve( GetVectorSize(str) );
  for (nlohmann::json::iterator itss = json.begin(); itss != json.end(); ++itss) {
    p_array.push_back( (int)itss.value() );
  }
  return p_array;
}

vector<string>
ghRailJSON::GetVectorString( string str )
{
  vector<string> p_array;
  nlohmann::json json = GetJsonObject( str );
  p_array.reserve( GetVectorSize(str) );
  for (nlohmann::json::iterator itss = json.begin(); itss != json.end(); ++itss) {
    p_array.push_back( itss.value().get<std::string>() );
  }
  return p_array;
}

std::string
ghRailJSON::GetUrl()
{
  return p_url;
}


/////////////////////////

void
ghRailCSV::SetCsvUrl(string host, string base, string file)
{
  p_host = host;
  p_url = host + GEOGLYPH_RSC_PATH + base + file;
}

bool
ghRailCSV::GetContent()
{
  try {
    cURLpp::Cleanup cleaner;
    cURLpp::Easy    req;

    WriterMemoryClass mWriterChunk;

    // Set the writer callback to enable cURL 
    // to write result in a memory area
    using namespace std::placeholders;
    curlpp::types::WriteFunctionFunctor functor = std::bind(&WriterMemoryClass::WriteMemoryCallback, &mWriterChunk, _1, _2, _3);
    curlpp::options::WriteFunction *test = new curlpp::options::WriteFunction(functor);
    req.setOpt(test);

    req.setOpt(new Url(p_url));

    req.perform();

    //mWriterChunk.print();//  for DEBUG
    p_csv = mWriterChunk.getString();
	  
  } catch (cURLpp::LogicError & e) {
    return false;
  } catch (cURLpp::RuntimeError & e) {
    return false;
  }
  return true;
}

string
ghRailCSV::GetCsv()
{
  return p_csv;
}


////////////////////////////////////////



void
ghRailModel::Setup(string host, string locomotive)
{
  p_host = host;
  p_url = host + GEOGLYPH_3DMODEL_PATH + locomotive;
  p_model = locomotive;
  p_status = 0;
}

std::string
ghRailModel::GetUrl()
{
  return p_url;
}

std::string
ghRailModel::GetModel()
{
  return p_model;
}

std::string
ghRailModel::GetGltf()
{
  std::string tmp = p_model;
  unsigned int pos = tmp.find("glb");
  //
  //
  //
  //
  std::string modelfile = tmp.replace(pos, 3, "gltf");
  //std::cout << modelfile << std::endl;
  //locomotive/Amtrak17_car1.gltf

#ifdef _WINDOWS  
  std::string modelf = modelfile;
  std::string::size_type pos2 = 0;
  while ((pos2 = modelf.find("/", pos2)) != std::string::npos) {
    modelf.replace(pos2, 1, "\\");
    pos2 += 2;
  }
  modelfile = modelf;
  //std::cout << modelf << std::endl;
#endif

  //
  // check for exists
  //
  FILE* fp = fopen(modelfile.c_str(), "r");
  if (fp == NULL) {
    // No Files
    modelfile = GEOGLYPH_MODEL_FILE_TEMPLATE;
  } else {
    fclose(fp);
  }
  //
  //
  //
  
  return modelfile;
}

void
ghRailModel::SetStatus(int status)
{
  p_status = status;
}

int
ghRailModel::GetStatus()
{
  return p_status;
}

