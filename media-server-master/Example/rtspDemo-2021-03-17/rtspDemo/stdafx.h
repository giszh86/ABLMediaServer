// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <string.h>

#include <Windows.h>

#include "rapidjson\document.h"
#include "rapidjson\stringbuffer.h"
#include "rapidjson\writer.h"

using namespace std ;

//#define  WriteRtspDataFlag    1 //定义写RTSP数据

#ifdef WriteRtspDataFlag

//媒体数据类型 
enum MediaDataType
{
	MedisDataType_H264  = 1,  //H264
	MedisDataType_H265  = 2,  //H265

	MedisDataType_G711A = 10,  //G711A \PCMA 
	MedisDataType_G711U = 11,  //G711U \ PCMU
	MedisDataType_AAC   = 12  //AAC
};

struct  MediaDataHead
{
	unsigned char  mediaDataType;
	int            mediaDataLength;
};

#endif 
