// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#endif

#include <iostream>
#include <algorithm>
#include <thread>
#include <stdint.h>
#include <memory>
#include <mutex>
#include <string>
#include <map>
#include <list>
#include "CudaChanManager.h"
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;
#else

#endif


// TODO: reference additional headers your program requires here
