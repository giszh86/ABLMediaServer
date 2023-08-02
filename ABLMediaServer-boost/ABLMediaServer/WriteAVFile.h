// WriteAVFile.h: interface for the CWriteAVFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WRITEAVFILE_H__311924BE_8255_463B_84F1_956EDC7A48D3__INCLUDED_)
#define AFX_WRITEAVFILE_H__311924BE_8255_463B_84F1_956EDC7A48D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define  MaxCsmFileByteCount    1024*1024*50  //50���ֽ�
#define  OneWriteDiskMaxLength  1024*1024*2   //ÿ�λ���2��д��һ��

class CWriteAVFile  
{
public:
	CWriteAVFile();
	virtual ~CWriteAVFile();

	bool    CreateAVFile(char* szFileName, bool bFileExist,unsigned long nFileSize);
	bool    WriteAVFile(char* szMediaData,int nLength, bool bFlastWriteFlag) ; //bWriteFlag true ������д�룬false����д
    void    CloseAVFile() ;

	CRITICAL_SECTION file_CriticalSection;
	unsigned long     nWriteByteCount ; //д���ֽ�����
	char    szFileName[255] ;
	bool    bOpenFlag ;        //���ļ��ı�־
	HANDLE  hWriteHandle ;     //�ļ����
	char*   szCacheAVBuffer ;  //��Ƶ��Ƶ����
	int     nCacheAVLength ;   //д�뻺����ܳ���
	char    m_szFileName[256]; //�ļ�����
	bool    bFileExist;
};

#endif // !defined(AFX_WRITEAVFILE_H__311924BE_8255_463B_84F1_956EDC7A48D3__INCLUDED_)
