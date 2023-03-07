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

#include "PlayerSDKABLogFile.h"

#define  PlayerSDK_LogMaxByteCount                   1024*1024*25                     //100兆
#define  PlayerSDK_BaseLogName                          "live555Client_"                //基础log名称 
#define  PlayerSDK_LogFilePath                             ".log/live555Client"          //Log 路径
#define  PlayerSDK_RetainMaxFileCount                20                                          //最大保留文件个数
#define  PlayerSDK_DeleteLogFileTimerSecond      700                                      //多长时间执行一次删除日志文件 单位 秒

using namespace std;
typedef    vector<unsigned long> PlayerSDK_FileNameNumber ;
PlayerSDK_FileNameNumber         PlayerSDK_fileNameNumber;
bool                             PlayerSDK_bInitLogFlag = false;
pthread_mutex_t                  PlayerSDK_ABL_LogFileLock;
int                              PlayerSDK_ABL_nFileByteCount = 0;//当前操作文件的大小
unsigned long                    PlayerSDK_ABL_nFileNumber; //当前文件序号
FILE*                            PlayerSDK_ABL_fLogFile = NULL;
char                             PlayerSDK_ABL_wzLog[32000] = { 0 };
char                             PlayerSDK_ABL_LogBuffer[32000] = { 0 };
pthread_t                        PlayerSDK_pThread_deleteLogFile;
char                             PlayerSDK_szLogLevel[3][64] = { "Log_Debug", "Log_Title", "Log_Error" };

int PlayerSDK_showAllFiles(const char * dir_name, bool& bExitingFlag, int& fileSize)
{
	umask(0);
	mkdir("./log",0777);
    umask(0);
	mkdir(PlayerSDK_LogFilePath, 0777);
 
	PlayerSDK_fileNameNumber.clear();
	
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
		if (strstr(filename->d_name, PlayerSDK_BaseLogName) != NULL)
		{
           string strFileName = filename ->d_name ;
		   nPos = strFileName.find("_",0) ;
		   if(nPos >= 0)
		   {
			  memset(szFileNumber,0x00,sizeof(szFileNumber));
			  memcpy(szFileNumber,filename->d_name+nPos+1,strlen(filename->d_name)-nPos-5) ;
			  PlayerSDK_fileNameNumber.push_back(atoi(szFileNumber));
			  //cout<<szFileNumber<< "  " <<endl ;
		   }
		}
	}
	if (PlayerSDK_fileNameNumber.size() == 0)
	{
		bExitingFlag = false ;
		return 1 ;
	}else
	{
		sort(PlayerSDK_fileNameNumber.begin(), PlayerSDK_fileNameNumber.end(), greater<int>());
	  //cout<< "vector Siize = " << fileNameNumber.size() <<  "  " << fileNameNumber[0] << endl ;
 	  //printf("当前文件 XHRTSPClient_%010d \r\n",fileNameNumber[0]) ;
	  char szFileName[64] = {0};
	  sprintf(szFileName, "%s/%s%010d.log", PlayerSDK_LogFilePath, PlayerSDK_BaseLogName, PlayerSDK_fileNameNumber[0]);

	  if (PlayerSDK_fileNameNumber.size() > PlayerSDK_RetainMaxFileCount)
	  {//删除老日志文件
	     char szDelFile[256] = {0};
		 for (int i = PlayerSDK_RetainMaxFileCount; i<PlayerSDK_fileNameNumber.size(); i++)
		 {
			 sprintf(szDelFile, "%s/%s%010d.log", PlayerSDK_LogFilePath, PlayerSDK_BaseLogName, PlayerSDK_fileNameNumber[i]);
			unlink(szDelFile) ;
		 }
	  }
	 
	  struct stat statbuf;
      if(stat(szFileName,&statbuf)==0)
	  {
	     //cout<< "File Siize = " << statbuf.st_size << endl ;
         fileSize = statbuf.st_size ;
		 
		 if (statbuf.st_size >= PlayerSDK_LogMaxByteCount)
		 {//文件大于120兆
	        bExitingFlag = false ;
			return PlayerSDK_fileNameNumber[0] + 1;
		 }else
		 {
	        bExitingFlag = true  ;
			return PlayerSDK_fileNameNumber[0];
		 }
	  }else
	  {
	      bExitingFlag = false ;
		  return PlayerSDK_fileNameNumber[0];
	  }
	}
} 

void* PlayerSDK_DeleteLogFileThread(void* lpVoid)
{
	int nSleepCount = 0;
	bool bExiting ;
	char szFileName[256] ={0};
    int  nFileByteCount ;
	
	while (PlayerSDK_bInitLogFlag)
	{
		if (nSleepCount >= PlayerSDK_DeleteLogFileTimerSecond)
		{
			nSleepCount = 0;
			PlayerSDK_showAllFiles(PlayerSDK_LogFilePath, bExiting, nFileByteCount);
		}
		
		nSleepCount ++ ;
		sleep(5) ;
	}
}

bool  PlayerSDK_InitLogFile()
{
	if (PlayerSDK_bInitLogFlag == false)
	{
		pthread_mutex_init(&PlayerSDK_ABL_LogFileLock, NULL);
		
		pthread_mutex_lock(&PlayerSDK_ABL_LogFileLock);
		bool bExiting ;
		char szFileName[256] ={0};
		PlayerSDK_ABL_nFileNumber = PlayerSDK_showAllFiles(PlayerSDK_LogFilePath, bExiting, PlayerSDK_ABL_nFileByteCount);
		sprintf(szFileName, "%s/%s%010d.log", PlayerSDK_LogFilePath, PlayerSDK_BaseLogName, PlayerSDK_ABL_nFileNumber);
		if(bExiting)
		{//文件已经存在
			PlayerSDK_ABL_fLogFile = fopen(szFileName, "a");
		}else
		{
			PlayerSDK_ABL_fLogFile = fopen(szFileName, "w");
		}
		
		PlayerSDK_bInitLogFlag = true;
		pthread_create(&PlayerSDK_pThread_deleteLogFile, NULL, PlayerSDK_DeleteLogFileThread, (void*)NULL);
		
		pthread_mutex_unlock(&PlayerSDK_ABL_LogFileLock);
		
		return true ;
	}else
		return false ;
}

bool PlayerSDK_WriteLog(LogLevel nLogLevel, const char* ms, ...)
{
	if (PlayerSDK_bInitLogFlag == true && nLogLevel >= 0 && nLogLevel <= 2)
	{
		pthread_mutex_lock(&PlayerSDK_ABL_LogFileLock);
		
		//先检查原来的大小
		if (PlayerSDK_ABL_nFileByteCount >= PlayerSDK_LogMaxByteCount && PlayerSDK_ABL_fLogFile != NULL)
		{
			fclose(PlayerSDK_ABL_fLogFile);
			
			char szFileName[256] ={0};
			PlayerSDK_ABL_nFileNumber++; //文件序号递增一个
			sprintf(szFileName, "%s/%s%010d.log", PlayerSDK_LogFilePath, PlayerSDK_BaseLogName, PlayerSDK_ABL_nFileNumber);
			PlayerSDK_ABL_fLogFile = fopen(szFileName, "w");
			PlayerSDK_ABL_nFileByteCount = 0; //大小重新开始计算
		}
		
		va_list args;
		va_start(args, ms);
		vsprintf(PlayerSDK_ABL_wzLog, ms, args);
		va_end(args);
	 
		time_t now;
		time(&now);
		struct tm *local;
		local = localtime(&now);
		
		/*printf("%04d-%02d-%02d %02d:%02d:%02d %s %s\n", local->tm_year+1900, local->tm_mon,
				local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec,szLogLevel[nLogLevel],
				ABL_wzLog);
		*/		
		sprintf(PlayerSDK_ABL_LogBuffer, "%04d-%02d-%02d %02d:%02d:%02d %s %s\n", local->tm_year + 1900, local->tm_mon + 1,
			local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec, PlayerSDK_szLogLevel[nLogLevel],
			PlayerSDK_ABL_wzLog);
					
		if (PlayerSDK_ABL_fLogFile != NULL)
		 {
			 fwrite(PlayerSDK_ABL_LogBuffer, 1, strlen(PlayerSDK_ABL_LogBuffer), PlayerSDK_ABL_fLogFile);
			 fflush(PlayerSDK_ABL_fLogFile);
		   
			 PlayerSDK_ABL_nFileByteCount += strlen(PlayerSDK_ABL_LogBuffer);
		 }
		pthread_mutex_unlock(&PlayerSDK_ABL_LogFileLock);
		return true ;
	}else
		return false ;	
 }

bool  PlayerSDK_ExitLogFile()
{
	if (PlayerSDK_bInitLogFlag == true)
	{
		pthread_mutex_lock(&PlayerSDK_ABL_LogFileLock);
		
		if (PlayerSDK_ABL_fLogFile != NULL)
		{
			fclose(PlayerSDK_ABL_fLogFile);
			PlayerSDK_ABL_fLogFile = NULL;
		}
		
		PlayerSDK_bInitLogFlag = false;
		pthread_mutex_unlock(&PlayerSDK_ABL_LogFileLock);
		return true ;
	}else
		return false ;
}
 