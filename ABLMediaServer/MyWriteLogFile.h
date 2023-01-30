#ifndef _MyWriteLogFile_H
#define _MyWriteLogFile_H

#define OneLineLogStringMaxLength  1024*1024*2 

bool   GetCurrentPath(char *szCurPath) ;
bool   CreateLogDir(char* szPath);
unsigned long  GetLogFileByPathName(char* szPath, char* szLogFileName, char* szOutFileName, bool& bFileExist);
bool   WriteLog(int nLevel, char* szSQL, ...);

#endif
