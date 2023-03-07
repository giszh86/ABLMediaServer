// WriteAVFile.h: interface for the CWriteAVFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WRITEAVFILE_H__311924BE_8255_463B_84F1_956EDC7A48D3__INCLUDED_)
#define AFX_WRITEAVFILE_H__311924BE_8255_463B_84F1_956EDC7A48D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define  MaxCsmFileByteCount    1024*1024*50  //50兆字节
#define  OneWriteDiskMaxLength  1024*1024*2   //每次积累2兆写入一次

class CWriteAVFile  
{
public:
	CWriteAVFile();
	virtual ~CWriteAVFile();

	BOOL    CreateAVFile(char* szFileName, bool bFileExist,DWORD nFileSize);
	BOOL    WriteAVFile(char* szMediaData,int nLength,BOOL bFlastWriteFlag) ; //bWriteFlag TRUE 则立刻写入，FALSE缓存写
    void    CloseAVFile() ;

	CRITICAL_SECTION file_CriticalSection;
	unsigned long     nWriteByteCount ; //写入字节总数
	char    szFileName[255] ;
	BOOL    bOpenFlag ;        //打开文件的标志
	HANDLE  hWriteHandle ;     //文件句柄
	char*   szCacheAVBuffer ;  //音频视频缓冲
	int     nCacheAVLength ;   //写入缓冲的总长度
	char    m_szFileName[256]; //文件名字
	bool    bFileExist;
};

#endif // !defined(AFX_WRITEAVFILE_H__311924BE_8255_463B_84F1_956EDC7A48D3__INCLUDED_)
