// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once



#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#include <iostream>
#include <algorithm>
#include <thread>

#include <mutex>
#include <string>
#include <map>
#include <list>


#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;
#else

using namespace std;


#endif



#include "cuda.h"
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

// TODO: reference additional headers your program requires here
