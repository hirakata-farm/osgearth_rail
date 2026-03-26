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

# ifndef _GH_RAIL_DATA_HPP_
# define _GH_RAIL_DATA_HPP_
     
# include <iostream>
# include <cstdlib>
# include <cstring>

# include <nlohmann/json.hpp>

#define GEOGLYPH_ROOT_URI "https://earth.geoglyph.info/rail/"
#define GEOGLYPH_RSC_FIELD_PATH "RSC/fields/"
#define GEOGLYPH_RSC_PATH "RSC/"
#define GEOGLYPH_RSC_ICON_PATH "RSC/marker/"
#define GEOGLYPH_3DMODEL_PATH "RSC/models/"
#define GEOGLYPH_CONF_PATH "conf/"
#define GEOGLYPH_CONF_EXT ".json"

#define MAX_FILE_LENGTH 200000

#ifdef _WINDOWS
#define GEOGLYPH_MODEL_FILE_TEMPLATE "locomotive\\DSBIC3_car2.gltf"
#else
#define GEOGLYPH_MODEL_FILE_TEMPLATE "locomotive/DSBIC3_car2.gltf"
#endif


//#define GEOGLYPH_MIN_SIZE 8000

//
// ghRailJSON  Class 
//
class ghRailJSON
    {
    public:
      void SetConfigUri(std::string host, std::string conf);
      void SetFieldUri(std::string host, std::string field);
      void SetLineUri(std::string host, std::string line);
      void SetLocomotiveUri(std::string host, std::string locomotive);
      bool GetContent();
      nlohmann::json GetJson();
      nlohmann::json GetJsonObject( std::string str0 );
      nlohmann::json GetJsonObject( std::string str0 , std::string str1 );
      std::string GetJsonString( std::string str0 );
      std::string GetJsonString( std::string str0 , std::string str1 );
      std::vector<int> GetVectorInt( std::string str );
      std::vector<std::string> GetVectorString( std::string str );
      int GetVectorSize( std::string str );
      std::string GetUrl();
    private:
      std::string p_url;
      std::string p_host;
      nlohmann::json p_json;
    };

class ghRailCSV
    {
    public:
      void SetCsvUrl(std::string host, std::string base, std::string file);
      bool GetContent();
      std::string GetCsv();
      std::string GetUrl();
    private:
      std::string p_url;
      std::string p_host;
      std::string p_csv;
    };

class ghRailModel
    {
    public:
      void Setup(std::string host, std::string locomotive);
      std::string GetUrl();
      std::string GetModel();
      std::string GetGltf();
      void SetStatus(int status);
      int GetStatus();
    private:
      std::string p_url;
      std::string p_host;
      std::string p_model;
      int p_status;
    };

//
//
// JSON Call Back Class
//
//
class WriterMemoryClass
{
private:
  char* m_pBuffer;
  size_t m_Size;
public:
	// Helper Class for reading result from remote host
	WriterMemoryClass()
	{
		this->m_pBuffer = NULL;
		this->m_pBuffer = (char*) malloc(MAX_FILE_LENGTH * sizeof(char));
		this->m_Size = 0;
	};

	~WriterMemoryClass()
	{
		if (this->m_pBuffer)
			free(this->m_pBuffer);
	};

	void* Realloc(void* ptr, size_t size)
	{
		if(ptr)
			return realloc(ptr, size);
		else
			return malloc(size);
	};

	// Callback must be declared static, otherwise it won't link...
	size_t WriteMemoryCallback(char* ptr, size_t size, size_t nmemb)
	{
		// Calculate the real size of the incoming buffer
		size_t realsize = size * nmemb;

		// (Re)Allocate memory for the buffer
		m_pBuffer = (char*) Realloc(m_pBuffer, m_Size + realsize + 1); // +1 terminator

		// Test if Buffer is initialized correctly & copy memory
		if (m_pBuffer == NULL) {
			realsize = 0;
		}

		memcpy(&(m_pBuffer[m_Size]), ptr, realsize);
		m_Size += realsize;

		m_pBuffer[m_Size] = '\0'; // Terminator
		
		// return the real size of the buffer...
		return realsize;
	};

	size_t WriteMemoryBinaryCallback(char* ptr, size_t size, size_t nmemb)
	{
		// Calculate the real size of the incoming buffer
		size_t realsize = size * nmemb;

		// (Re)Allocate memory for the buffer
		m_pBuffer = (char*) Realloc(m_pBuffer, m_Size + realsize );

		// Test if Buffer is initialized correctly & copy memory
		if (m_pBuffer == NULL) {
			realsize = 0;
		}

		memcpy(&(m_pBuffer[m_Size]), ptr, realsize);
		m_Size += realsize;

		// return the real size of the buffer...
		return realsize;
	};


	void print() 
	{
	  std::cout << "Size: " << m_Size << std::endl;
	  std::cout << "Content: " << std::endl << m_pBuffer << std::endl;
	}

	char *getbuffer() 
	{
	  return m_pBuffer;
	}

	size_t getbuffersize() 
	{
	  return m_Size;
	}

        nlohmann::json getJSON() 
	{

	  std::string strjson(m_pBuffer);
	  //std::cout << "Str: " << strjson << std::endl;
	  unsigned int pos = strjson.find("File Not Found");
	  unsigned int length = strjson.length();
	  if ( pos < length ) {
	    std::string errjson("{}");
	    return nlohmann::json::parse(errjson);
	  } else {
	    return nlohmann::json::parse(strjson);
	  }
	}

        std::string getString() 
	{
	  std::string strtxt(m_pBuffer);
	  return strtxt;
	}

};

#endif
