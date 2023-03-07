// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	int ret;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		ret = XHNetSDK_Init(2, 8);
		StartLogFile("live555Client", "live555Client_00*.log", 5);

		//ABL_BreakRtspClientFifo.InitFifo(1024 * 1024 * 2);
		WriteLog(Log_Debug, "\r\n-------------------------live555Client ¿Í»§¶ËÆô¶¯ ---------------------------");

		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

