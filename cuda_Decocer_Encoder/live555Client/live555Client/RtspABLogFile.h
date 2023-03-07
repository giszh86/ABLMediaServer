#ifndef _ABLogFile_H
#define _ABLogFile_H

//Log 信息级别
enum  LogLevel
{
	Log_Debug = 0,   //用于调试
	Log_Title = 1,   //用于提示
	Log_Error = 2    //标识为错误
};

int   Rtsp_showAllFiles( const char * dir_name,bool& bExitingFlag,int& fileSize);
void* Rtsp_DeleteLogFileThread(void* lpVoid);
bool  Rtsp_InitLogFile();
bool  Rtsp_WriteLog(LogLevel nLogLevel, const char* ms, ...);
bool  Rtsp_ExitLogFile();

#endif

