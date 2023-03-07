#ifndef _MyWriteLogFile_H
#define _MyWriteLogFile_H

#define OneLineLogStringMaxLength  1024*1024*2 

BOOL   GetCurrentPath(char *szCurPath) ;
bool   CreateLogDir(char* szPath);
DWORD  GetLogFileByPathName(char* szPath, char* szLogFileName, char* szOutFileName, bool& bFileExist);
BOOL   WriteLog(int nLevel, char* szSQL, ...);

#endif
