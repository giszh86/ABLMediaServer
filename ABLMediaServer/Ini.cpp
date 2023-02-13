#include "stdafx.h"
#include "Ini.h"
/******************************************************************************
* 功  能：构造函数
* 参  数：无
* 返回值：无
* 备  注：
******************************************************************************/
CIni::CIni()
{
	memset(m_szKey, 0, sizeof(m_szKey));
	memset(szIniFileName, 0x00, sizeof(szIniFileName));
	memset(szFindKey, 0x00, sizeof(szFindKey));
	memset(szReturn, 0x00, sizeof(szReturn));
	m_fp = NULL;
	m_fpWrite = NULL;
	pFileBuffer = new char[IniFileMaxBuffer];
	memset(pFileBuffer, 0x00, IniFileMaxBuffer);
	nFileBufferLength = 0;
	strcpy(szReturn, "\r\n");
}

/******************************************************************************
* 功  能：析构函数
* 参  数：无
* 返回值：无
* 备  注：
******************************************************************************/

CIni::~CIni()
{
	m_Map.clear();
	if (pFileBuffer)
	{
		delete[] pFileBuffer;
		pFileBuffer = NULL;
	}
}

/******************************************************************************
* 功  能：打开文件函数
* 参  数：无
* 返回值：
* 备  注：
******************************************************************************/
INI_RES CIni::OpenFile(const char* pathName, const char* type)
{
	string szLine, szMainKey, szLastMainKey, szSubKey;
	char strLine[CONFIGLEN] = { 0 };
	KEYMAP mLastMap;
	int  nIndexPos = -1;
	int  nLeftPos = -1;
	int  nRightPos = -1;
	m_fp = fopen(pathName, type);

	if (m_fp == NULL)
	{
		printf("open inifile %s error!\n", pathName);
		return INI_OPENFILE_ERROR;
	}
	strcpy(szIniFileName, pathName);
	m_Map.clear();

	while (fgets(strLine, CONFIGLEN, m_fp))
	{
		szLine.assign(strLine);
		//删除字符串中的非必要字符
		nLeftPos = szLine.find("\n");
		if (string::npos != nLeftPos)
		{
			szLine.erase(nLeftPos, 1);
		}
		nLeftPos = szLine.find("\r");
		if (string::npos != nLeftPos)
		{
			szLine.erase(nLeftPos, 1);
		}

		//注释的行
		if (strLine[0] == '#' || strLine[0] == ';')
			continue;

		//判断是否是主键
		nLeftPos = szLine.find("[");
		nRightPos = szLine.find("]");
		if (nLeftPos != string::npos && nRightPos != string::npos)
		{
			szLine.erase(nLeftPos, 1);
			nRightPos--;
			szLine.erase(nRightPos, 1);
			m_Map[szLastMainKey] = mLastMap;
			mLastMap.clear();
			szLastMainKey = szLine;
		}
		else
		{
 			//是否是子键
			if (nIndexPos = szLine.find("="), string::npos != nIndexPos)
			{
				string szSubKey, szSubValue;
				szSubKey = szLine.substr(0, nIndexPos);
				szSubValue = szLine.substr(nIndexPos + 1, szLine.length() - nIndexPos - 1);
				mLastMap[szSubKey] = szSubValue;
			}
			else
			{
				//TODO:不符合ini键值模板的内容 如注释等
			}
		}

	}
	//插入最后一次主键
	m_Map[szLastMainKey] = mLastMap;

	OpenWriteFile(szIniFileName, "rb");

	return INI_SUCCESS;
}

/******************************************************************************
* 功  能：关闭文件函数
* 参  数：无
* 返回值：
* 备  注：
******************************************************************************/
INI_RES CIni::CloseFile()
{
	if (m_fp != NULL)
	{
		fclose(m_fp);
		m_fp = NULL;
	}

	return INI_SUCCESS;
}

/******************************************************************************
* 功  能：获取[SECTION]下的某一个键值的字符串
* 参  数：
*  char* mAttr  输入参数    主键
*  char* cAttr  输入参数 子键
*  char* value  输出参数 子键键值
* 返回值：
* 备  注：
******************************************************************************/
INI_RES CIni::GetKey(const char* mAttr, const char* cAttr, char* pValue)
{

	KEYMAP mKey = m_Map[mAttr];

	string sTemp = mKey[cAttr];


#ifdef USE_BOOST

	//去掉空格 
	boost::trim(sTemp);
#else
	
	//去掉空格 
	ABL::trim(sTemp);
#endif
	strcpy(pValue, sTemp.c_str());

	return INI_SUCCESS;
}

/******************************************************************************
* 功  能：获取整形的键值
* 参  数：
*       cAttr                     主键
*      cAttr                     子键
* 返回值：正常则返回对应的数值 未读取成功则返回0(键值本身为0不冲突)
* 备  注：
******************************************************************************/
int CIni::GetInt(const char* mAttr, const char* cAttr)
{
	int nRes = 0;

	memset(m_szKey, 0, sizeof(m_szKey));

	if (INI_SUCCESS == GetKey(mAttr, cAttr, m_szKey))
	{
		nRes = atoi(m_szKey);
	}
	return nRes;
}

/******************************************************************************
* 功  能：获取键值的字符串
* 参  数：
*       cAttr                     主键
*      cAttr                     子键
* 返回值：正常则返回读取到的子键字符串 未读取成功则返回"NULL"
* 备  注：
******************************************************************************/
char *CIni::GetStr(const char* mAttr, const char* cAttr)
{
	memset(m_szKey, 0, sizeof(m_szKey));

	if (INI_SUCCESS != GetKey(mAttr, cAttr, m_szKey))
	{
		strcpy(m_szKey, "NULL");
	}

	return m_szKey;
}

//读取源文件的内存放置到内存中
INI_RES CIni::OpenWriteFile(const char* pathName, const char* type)
{
	m_fpWrite = fopen(pathName, type);

	if (m_fpWrite == NULL)
	{
		printf("open inifile %s error!\n", pathName);
		return INI_OPENFILE_ERROR;
	}

	nIniFileSize = IniFileMaxBuffer ;
#ifdef OS_System_Windows
	struct _stat64 fileBuf;
	int error = _stat64(pathName, &fileBuf);
	if (error == 0)
		nIniFileSize = fileBuf.st_size;
#else 
	struct stat fileBuf;
	int error = stat(pathName, &fileBuf);
	if (error == 0)
		nIniFileSize = fileBuf.st_size;
#endif

	if (nIniFileSize > IniFileMaxBuffer)
	{
		fclose(m_fpWrite);
		m_fpWrite = NULL;
		return INI_OPENFILE_ERROR;
    }

	fread(pFileBuffer, 1, nIniFileSize, m_fpWrite);
	fclose(m_fpWrite);
	m_fpWrite = NULL ;

	return INI_SUCCESS;
}

//更新key 的值
INI_RES CIni::WriteKeyString(char* szSection, char* szKey, char* szValue)
{
	std::lock_guard<std::mutex> lock(writeFileMutex);
	if (nIniFileSize <= 0)
		return INI_OPENFILE_ERROR;

	string  strIniBuffer = pFileBuffer;
	nKeyPos = -1;
	nReturnPos = -1;

	sprintf(szFindKey, "%s=", szKey);
	nKeyPos = strIniBuffer.find(szFindKey, 0);
	if (nKeyPos < 0)
		return INI_NO_ATTR;
	nKeyPos += strlen(szFindKey);
	nReturnPos = strIniBuffer.find(szReturn, nKeyPos);
	if (nReturnPos < 0)
		return INI_NO_ATTR;

	//原来长度
	oldLenth = nReturnPos - nKeyPos;
	newLength = strlen(szValue);
	if (newLength == oldLenth)
	{//总长度不变
		memcpy(pFileBuffer + nKeyPos, szValue, newLength);
	}
	else if (newLength > oldLenth)
	{//总长度变长
		nAddLenth = newLength - oldLenth;
		nMoveLenth = nIniFileSize - nKeyPos;
		memmove(pFileBuffer + (nKeyPos + nAddLenth), pFileBuffer + nKeyPos, nMoveLenth);

		memcpy(pFileBuffer + nKeyPos, szValue, newLength);
		nIniFileSize += (newLength - oldLenth);
	}
	else if (newLength < oldLenth)
	{//总长度变短
		nAddLenth = oldLenth - newLength;
		nMoveLenth = nIniFileSize - nKeyPos - nAddLenth;
		memmove(pFileBuffer + nKeyPos , pFileBuffer + (nKeyPos + nAddLenth), nMoveLenth);

		if(newLength > 0)
		  memcpy(pFileBuffer + nKeyPos, szValue, newLength);
		nIniFileSize -= (oldLenth - newLength);
	}

	m_fpWrite = fopen(szIniFileName, "wb");
	if (m_fpWrite)
	{
		fwrite(pFileBuffer, 1, nIniFileSize, m_fpWrite);
		fflush(m_fpWrite);
		fclose(m_fpWrite);
		m_fpWrite = NULL;
	    return INI_SUCCESS;
	}
	else
		return INI_ERROR;
}

INI_RES CIni::WriteKeyInt(char* szSection, char* szKey, int nValue)
{
	char szBuffer[128];
	sprintf(szBuffer, "%d", nValue);
	return WriteKeyString(szSection, szKey, szBuffer);
}
