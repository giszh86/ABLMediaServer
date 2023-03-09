// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "cudaCodecDLL.h"

extern "C" cudaCodecDLL_EXPORTSIMPL bool cudaCodec_UnInit();

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		cudaCodec_UnInit();
		break;
	}
	return TRUE;
}

