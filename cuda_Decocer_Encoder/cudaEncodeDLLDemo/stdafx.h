// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
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
// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�
