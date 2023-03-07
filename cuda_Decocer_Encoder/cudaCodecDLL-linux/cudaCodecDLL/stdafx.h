// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#ifndef _Stdafx_H
#define _Stdafx_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <float.h>
#include <dirent.h>
#include <sys/stat.h>
#include <malloc.h>

#include<sys/types.h> 
#include<sys/socket.h>
#include<sys/time.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h> 
#include <ifaddrs.h>
#include <netdb.h>

#include <pthread.h>
#include <signal.h>
#include <string>
#include <list>
#include <map>
#include <mutex>
#include <vector>
#include <math.h>
#include <iconv.h>
#include <malloc.h>

#include <iostream>
#include <algorithm>
#include <thread>

#include <mutex>
#include <string>
#include <map>
#include <list>

#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;

#include <cuda.h>
#include "NvDecoder/NvDecoder.h"
#include "../Utils/NvCodecUtils.h"

#include "CudaChanManager.h"

//#define    WriteYUVFile_Flag 1
//#define    LibYUVScaleYUVFlag  1 //ÊÇ·ñËõÐ¡YUV

#ifdef LibYUVScaleYUVFlag
  #include "libyuv.h"
  #pragma comment(lib, "yuv.lib")
#endif 


static __inline int Abs(int v) {
	return v >= 0 ? v : -v;
}

#endif

