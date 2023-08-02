#ifndef  _INI_H
#define _INI_H

#include <map>
#include <string>
using namespace std;
 
#define CONFIGLEN           512 
#define IniFileMaxBuffer    1024*512
 
enum INI_RES
{
    INI_SUCCESS,            //成功
    INI_ERROR,              //普通错误
    INI_OPENFILE_ERROR,     //打开文件失败
    INI_NO_ATTR            //无对应的键值
};
 
//              子键索引    子键值 
typedef map<std::string,std::string> KEYMAP;
//              主键索引 主键值  
typedef map<std::string,KEYMAP> MAINKEYMAP;
// config 文件的基本操作类
 
class CIni 
{
public:
    // 构造函数
    CIni();
 
    char * pFileBuffer;
	int             nFileBufferLength;
    // 析够函数
    virtual ~CIni();
public:
    //获取整形的键值
    int  GetInt(const char* mAttr, const char* cAttr );
    //获取键值的字符串
    char *GetStr(const char* mAttr, const char* cAttr );
    // 打开config 文件
    INI_RES OpenFile(const char* pathName, const char* type);
    // 关闭config 文件
    INI_RES CloseFile();

	//支持写 ini 文件 
	INI_RES OpenWriteFile(const char* pathName, const char* type);
	INI_RES WriteKeyString(char* szSection,char* szKey,char* szValue);
	INI_RES WriteKeyInt(char* szSection, char* szKey, int nValue);

protected:
    // 读取config文件
    INI_RES GetKey(const char* mAttr, const char* cAttr, char* value);
protected:
    // 被打开的文件局柄
    FILE* m_fp;
    char  m_szKey[ CONFIGLEN ];
    MAINKEYMAP m_Map;

	std::mutex    writeFileMutex;
	FILE* m_fpWrite;
	int   nIniFileSize;
	char  szIniFileName[256];
	char  szFindKey[256] ;
	char  szReturn[128];
	int    nKeyPos , nReturnPos ,oldLenth , newLength,nMoveLenth,nAddLenth;

};

#endif

