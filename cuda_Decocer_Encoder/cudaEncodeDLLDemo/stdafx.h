// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#ifdef _WIN32
#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#else
#include <cstring>
#include <unistd.h>
#include <dlfcn.h>
#include <stdio.h>
#endif

#include <string>

#include "cudaEncodeDLL.h"

using namespace std;
// TODO:  在此处引用程序需要的其他头文件
