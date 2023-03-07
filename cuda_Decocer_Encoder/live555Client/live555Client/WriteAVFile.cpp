// WriteAVFile.cpp: implementation of the CWriteAVFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WriteAVFile.h"
#include "MyWriteLogFile.h"

extern char                           ABL_szCurrentPath[512]  ;
extern char                           ABL_szLogPath[256] ;
extern char                           ABL_BaseLogFileName[256];
extern DWORD  GetLogFileByPathName(char* szPath, char* szLogFileName, char* szOutFileName, bool& bFileExist);

CWriteAVFile::CWriteAVFile()
{
	 bOpenFlag = FALSE ;        //���ļ��ı�־
	 hWriteHandle = NULL  ;     //�ļ����
	 szCacheAVBuffer = new char[OneWriteDiskMaxLength];
	 InitializeCriticalSection(&file_CriticalSection);
	 memset(m_szFileName, 0x00, sizeof(m_szFileName));
}

CWriteAVFile::~CWriteAVFile()
{
      CloseAVFile() ;
	  DeleteCriticalSection(&file_CriticalSection);
	  delete[] szCacheAVBuffer;
}

//�����ļ�
BOOL CWriteAVFile::CreateAVFile(char* szFileName, bool bFileExist, DWORD nFileSize)
{
	DWORD dwWrite ;

    if(bOpenFlag)
	  return FALSE ;

	if (bFileExist == false)
	{//�����ļ�
		hWriteHandle = CreateFile(szFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hWriteHandle == INVALID_HANDLE_VALUE)
			return FALSE;
	}
	else
	{//ԭ�ļ��Ѿ�����
		hWriteHandle = CreateFile(szFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hWriteHandle == INVALID_HANDLE_VALUE)
			return FALSE;
		::SetFilePointer(hWriteHandle, NULL, NULL, FILE_END);
	}

	//ͳ���ֽ����������ܳ���1G
	nWriteByteCount = nFileSize;

    nCacheAVLength = 0 ;
	bOpenFlag = TRUE ;
	if (strlen(m_szFileName) == 0)
	 strcpy(m_szFileName, szFileName);

	return TRUE ;
}

//д��ý������
BOOL  CWriteAVFile::WriteAVFile(char* szMediaData, int nLength, BOOL bFlastWriteFlag)
{
	DWORD dwWrite ;
	DWORD dwFileSize = 0;
     if(!bOpenFlag || nLength <= 0 || szMediaData == NULL)
         return FALSE ;

	 EnterCriticalSection(&file_CriticalSection);

	 if (bFlastWriteFlag)
	 {//����д��
		 WriteFile(hWriteHandle, szMediaData, nLength, &dwWrite, NULL);
		 nWriteByteCount += nLength;
	 }
	 else
	 {//�Ȼ�����д��
		 if (OneWriteDiskMaxLength - nCacheAVLength < nLength)
		 {//����ʣ��Ŀռ䲻�������֡
			 if (nCacheAVLength > 0)
				 WriteFile(hWriteHandle, szCacheAVBuffer, nCacheAVLength, &dwWrite, NULL);

			 //�ܼ��ֽ��������ܳ���1G
			 nWriteByteCount += nCacheAVLength;

			 nCacheAVLength = 0;
		 }

		 memcpy(szCacheAVBuffer + nCacheAVLength, szMediaData, nLength);
		 nCacheAVLength += nLength;
	 }
	 LeaveCriticalSection(&file_CriticalSection);

	 //�������1G�ֽڣ������йر�
	 if (nWriteByteCount >= MaxCsmFileByteCount)
	 {
		 CloseAVFile();

		 //��ȡ��־����
		 dwFileSize = GetLogFileByPathName(ABL_szLogPath, ABL_BaseLogFileName, m_szFileName, bFileExist);

		 CreateAVFile(m_szFileName, bFileExist, dwFileSize);//�����������ļ�
	 }

	 return TRUE ;
}

//�ر��ļ�
void  CWriteAVFile::CloseAVFile() 
{
	DWORD dwWrite ;

	if(bOpenFlag)
	{
       EnterCriticalSection(&file_CriticalSection);
		bOpenFlag = FALSE ;
	
	    if(nCacheAVLength > 0)
  		  WriteFile(hWriteHandle,szCacheAVBuffer,nCacheAVLength,&dwWrite,NULL) ;
	   
		nCacheAVLength = 0;
		CloseHandle(hWriteHandle) ;
 	   LeaveCriticalSection(&file_CriticalSection);

	}
}
