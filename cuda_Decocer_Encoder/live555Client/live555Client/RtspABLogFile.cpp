#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#include <time.h>
#include <stdarg.h>
#include <syslog.h>

#include <pthread.h>

#include "RtspABLogFile.h"

#define  Rtsp_LogMaxByteCount                   1024*1024*25                     //100兆
#define  Rtsp_BaseLogName                          "live555Client_"                //基础log名称 
#define  Rtsp_LogFilePath                             "./log/live555Client"          //Log 路径
#define  Rtsp_RetainMaxFileCount                10                                          //最大保留文件个数
#define  Rtsp_DeleteLogFileTimerSecond      700                                      //多长时间执行一次删除日志文件 单位 秒

using namespace std;
typedef    vector<unsigned long> Rtsp_FileNameNumber ;
Rtsp_FileNameNumber              Rtsp_fileNameNumber;
bool                             Rtsp_bInitLogFlag = false;
pthread_mutex_t                  Rtsp_ABL_LogFileLock;
int                              Rtsp_ABL_nFileByteCount = 0;//当前操作文件的大小
unsigned long                    Rtsp_ABL_nFileNumber; //当前文件序号
FILE*                            Rtsp_ABL_fLogFile = NULL;
char                             Rtsp_ABL_wzLog[32000] = { 0 };
char                             Rtsp_ABL_LogBuffer[32000] = { 0 };
pthread_t                        Rtsp_pThread_deleteLogFile;
char                             Rtsp_szLogLevel[3][64] = { "Log_Debug", "Log_Title", "Log_Error" };

int Rtsp_showAllFiles(const char * dir_name, bool& bExitingFlag, int& fileSize)
{
	mkdir("./log",777);
	mkdir(Rtsp_LogFilePath, 777);

	Rtsp_fileNameNumber.clear();
	
	fileSize = 0 ;
	// check the parameter !
	if( NULL == dir_name )
	{
		cout<<" dir_name is null ! "<<endl;
		return -1;
	}
 
	// check if dir_name is a valid dir
	struct stat s;
	lstat( dir_name , &s );
	if( ! S_ISDIR( s.st_mode ) )
	{
		cout<<"dir_name is not a valid directory !"<<endl;
		return -1;
	}
	
	struct dirent * filename;    // return value for readdir()
 	DIR * dir;                   // return value for opendir()
	dir = opendir( dir_name );
	if( NULL == dir )
	{
		cout<<"Can not open dir "<<dir_name<<endl;
		return -1;
	}
	cout<<"Successfully opened the dir !"<<endl;
	
    int nPos ;
	char  szFileNumber[64] ;
	/* read all the files in the dir ~ */
	while( ( filename = readdir(dir) ) != NULL )
	{
		// get rid of "." and ".."
		if( strcmp( filename->d_name , "." ) == 0 || 
			strcmp( filename->d_name , "..") == 0    )
			continue;
		if (strstr(filename->d_name, Rtsp_BaseLogName) != NULL)
		{
           string strFileName = filename ->d_name ;
		   nPos = strFileName.find("_",0) ;
		   if(nPos >= 0)
		   {
			  memset(szFileNumber,0x00,sizeof(szFileNumber));
			  memcpy(szFileNumber,filename->d_name+nPos+1,strlen(filename->d_name)-nPos-5) ;
			  Rtsp_fileNameNumber.push_back(atoi(szFileNumber));
			  //cout<<szFileNumber<< "  " <<endl ;
		   }
		}
	}
	if (Rtsp_fileNameNumber.size() == 0)
	{
		bExitingFlag = false ;
		return 1 ;
	}else
	{
		sort(Rtsp_fileNameNumber.begin(), Rtsp_fileNameNumber.end(), greater<int>());
	  //cout<< "vector Siize = " << fileNameNumber.size() <<  "  " << fileNameNumber[0] << endl ;
 	  //printf("当前文件 XHRTSPClient_%010d \r\n",fileNameNumber[0]) ;
	  char szFileName[64] = {0};
	  sprintf(szFileName, "%s/%s%010d.log", Rtsp_LogFilePath, Rtsp_BaseLogName, Rtsp_fileNameNumber[0]);

	  if (Rtsp_fileNameNumber.size() > Rtsp_RetainMaxFileCount)
	  {//删除老日志文件
	     char szDelFile[256] = {0};
		 for (int i = Rtsp_RetainMaxFileCount; i<Rtsp_fileNameNumber.size(); i++)
		 {
			 sprintf(szDelFile, "%s/%s%010d.log", Rtsp_LogFilePath, Rtsp_BaseLogName, Rtsp_fileNameNumber[i]);
			unlink(szDelFile) ;
		 }
	  }
	 
	  struct stat statbuf;
      if(stat(szFileName,&statbuf)==0)
	  {
	     //cout<< "File Siize = " << statbuf.st_size << endl ;
         fileSize = statbuf.st_size ;
		 
		 if (statbuf.st_size >= Rtsp_LogMaxByteCount)
		 {//文件大于120兆
	        bExitingFlag = false ;
			return Rtsp_fileNameNumber[0] + 1;
		 }else
		 {
	        bExitingFlag = true  ;
			return Rtsp_fileNameNumber[0];
		 }
	  }else
	  {
	      bExitingFlag = false ;
		  return Rtsp_fileNameNumber[0];
	  }
	}
} 

void* Rtsp_DeleteLogFileThread(void* lpVoid)
{
	int nSleepCount = 0;
	bool bExiting ;
	char szFileName[256] ={0};
    int  nFileByteCount ;
	
	while (Rtsp_bInitLogFlag)
	{
		if (nSleepCount >= Rtsp_DeleteLogFileTimerSecond)
		{
			nSleepCount = 0;
			Rtsp_showAllFiles(Rtsp_LogFilePath, bExiting, nFileByteCount);
		}
		
		nSleepCount ++ ;
		sleep(5) ;
	}
}

bool  Rtsp_InitLogFile()
{
	if (Rtsp_bInitLogFlag == false)
	{
		pthread_mutex_init(&Rtsp_ABL_LogFileLock, NULL);
		
		pthread_mutex_lock(&Rtsp_ABL_LogFileLock);
		bool bExiting ;
		char szFileName[256] ={0};
		Rtsp_ABL_nFileNumber = Rtsp_showAllFiles(Rtsp_LogFilePath, bExiting, Rtsp_ABL_nFileByteCount);
		sprintf(szFileName, "%s/%s%010d.log", Rtsp_LogFilePath, Rtsp_BaseLogName, Rtsp_ABL_nFileNumber);
		if(bExiting)
		{//文件已经存在
			Rtsp_ABL_fLogFile = fopen(szFileName, "a");
		}else
		{
			Rtsp_ABL_fLogFile = fopen(szFileName, "w");
		}
		
		Rtsp_bInitLogFlag = true;
		pthread_create(&Rtsp_pThread_deleteLogFile, NULL, Rtsp_DeleteLogFileThread, (void*)NULL);
		
		pthread_mutex_unlock(&Rtsp_ABL_LogFileLock);
		
		return true ;
	}else
		return false ;
}

bool Rtsp_WriteLog(LogLevel nLogLevel, const char* ms, ...)
{
	if (Rtsp_bInitLogFlag == true && nLogLevel >= 0 && nLogLevel <= 2)
	{
		pthread_mutex_lock(&Rtsp_ABL_LogFileLock);
		
		//先检查原来的大小
		if (Rtsp_ABL_nFileByteCount >= Rtsp_LogMaxByteCount && Rtsp_ABL_fLogFile != NULL)
		{
			fclose(Rtsp_ABL_fLogFile);
			
			char szFileName[256] ={0};
			Rtsp_ABL_nFileNumber++; //文件序号递增一个
			sprintf(szFileName, "%s/%s%010d.log", Rtsp_LogFilePath, Rtsp_BaseLogName, Rtsp_ABL_nFileNumber);
			Rtsp_ABL_fLogFile = fopen(szFileName, "w");
			Rtsp_ABL_nFileByteCount = 0; //大小重新开始计算
		}
		
		va_list args;
		va_start(args, ms);
		vsprintf(Rtsp_ABL_wzLog, ms, args);
		va_end(args);
	 
		time_t now;
		time(&now);
		struct tm *local;
		local = localtime(&now);
		
		/*printf("%04d-%02d-%02d %02d:%02d:%02d %s %s\n", local->tm_year+1900, local->tm_mon,
				local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec,szLogLevel[nLogLevel],
				ABL_wzLog);
		*/		
		sprintf(Rtsp_ABL_LogBuffer, "%04d-%02d-%02d %02d:%02d:%02d %s %s\n", local->tm_year + 1900, local->tm_mon + 1,
			local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec, Rtsp_szLogLevel[nLogLevel],
			Rtsp_ABL_wzLog);
					
		if (Rtsp_ABL_fLogFile != NULL)
		 {
			 fwrite(Rtsp_ABL_LogBuffer, 1, strlen(Rtsp_ABL_LogBuffer), Rtsp_ABL_fLogFile);
			 fflush(Rtsp_ABL_fLogFile);
		   
			 Rtsp_ABL_nFileByteCount += strlen(Rtsp_ABL_LogBuffer);
		 }
		pthread_mutex_unlock(&Rtsp_ABL_LogFileLock);
		return true ;
	}else
		return false ;	
 }

bool  Rtsp_ExitLogFile()
{
	if (Rtsp_bInitLogFlag == true)
	{
		pthread_mutex_lock(&Rtsp_ABL_LogFileLock);
		
		if (Rtsp_ABL_fLogFile != NULL)
		{
			fclose(Rtsp_ABL_fLogFile);
			Rtsp_ABL_fLogFile = NULL;
		}
		
		Rtsp_bInitLogFlag = false;
		pthread_mutex_unlock(&Rtsp_ABL_LogFileLock);
		return true ;
	}else
		return false ;
}
 
