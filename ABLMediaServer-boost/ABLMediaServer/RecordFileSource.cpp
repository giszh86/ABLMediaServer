/*
功能：
    1、装入老的历史录像文件名字
    2、对录像文件名字进行升序排列
	3、增加新录像文件文件，追加到list的尾部
	4、删除过期的录像文件
	5、根据 app\ stream 、时间段 查找出符合条件的所有录像文件名字 
	6、根据 app\ stream \ 一个录像名字 ，判断该文件是否存在  
	 
日期    2022-01-13
作者    罗家兄弟 
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "RecordFileSource.h"
extern MediaServerPort                       ABL_MediaServerPort;
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                            pMessageNoticeFifo;          //消息通知FIFO

CRecordFileSource::CRecordFileSource(char* app, char* stream)
{
	memset(m_app,0x00,sizeof(m_app));
	memset(m_stream, 0x00, sizeof(m_stream));
	memset(m_szShareURL, 0x00, sizeof(m_szShareURL));
 
	strcpy(m_app, app);
	strcpy(m_stream, stream);
	sprintf(m_szShareURL, "/%s/%s", app, stream);
}

CRecordFileSource::~CRecordFileSource()
{
	malloc_trim(0);
}

bool CRecordFileSource::AddRecordFile(char* szFileName)
{
	std::lock_guard<std::mutex> lock(RecordFileLock);

	memset(szBuffer, 0x00, sizeof(szBuffer));
	memcpy(szBuffer, szFileName, strlen(szFileName) - 4);
	uint64_t nSecond = GetCurrentSecond() - GetCurrentSecondByTime(szBuffer);

	fileList.push_back(atoll(szBuffer));
 
	return true;
}

void CRecordFileSource::Sort()
{
	std::lock_guard<std::mutex> lock(RecordFileLock);
 	fileList.sort();
}

//修改过期录像文件
bool  CRecordFileSource::UpdateExpireRecordFile(char* szNewFileName)
{
	std::lock_guard<std::mutex> lock(RecordFileLock);
	uint64_t nGetFile;
	uint64_t nSecond = 0; 
	char    szDateTime[128] = { 0 };
	bool    bUpdateFlag = false;

	if (fileList.size() <= 0 )
	{
		WriteLog(Log_Debug, "UpdateExpireRecordFile %s 尚未有录像文件 ,新名字为 %s ", m_szShareURL, szNewFileName);
		return false ; 
	}

	while (fileList.size() > 0 )
	{
		nGetFile = fileList.front();
		sprintf(szDateTime, "%llu", nGetFile);
		nSecond = GetCurrentSecond() - GetCurrentSecondByTime(szDateTime);
		if (nSecond > (ABL_MediaServerPort.fileKeepMaxTime * 3600))
		{
			fileList.pop_front();
#ifdef OS_System_Windows
			sprintf(szDeleteFile, "%s%s\\%s\\%s.mp4", ABL_MediaServerPort.recordPath, m_app, m_stream, szDateTime);
#else 
			sprintf(szDeleteFile, "%s%s/%s/%s.mp4", ABL_MediaServerPort.recordPath, m_app, m_stream, szDateTime);
#endif
			//如果修改失败，回收以后再次修改
			if (rename(szDeleteFile,szNewFileName) != 0 )
			{
				fileList.push_back(nGetFile); 
				WriteLog(Log_Debug, "UpdateExpireRecordFile %s 修改文件 %llu.mp4 失败，回收以后再修改 ", m_szShareURL, nGetFile);
				break;
			}
			else
			{
			  bUpdateFlag = true;

			  //完成一个覆盖一个mp4文件通知 
			  if (ABL_MediaServerPort.hook_enable == 1 && ABL_MediaServerPort.nClientDeleteRecordMp4 > 0)
			  {
				  MessageNoticeStruct msgNotice;
				  msgNotice.nClient = ABL_MediaServerPort.nClientDeleteRecordMp4;
				  sprintf(msgNotice.szMsg, "{\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"fileName\":\"%s.mp4\"}", m_app, m_stream, ABL_MediaServerPort.mediaServerID, szDateTime);
				  pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
			  }
			  break;
			}
   		}
		else
			break;
	}

	if(!bUpdateFlag)
		WriteLog(Log_Debug, "UpdateExpireRecordFile %s 没有录像文件到期 ,新名字为 %s ", m_szShareURL, szNewFileName);
 
	return bUpdateFlag ;
}

//查询录像文件是否存在 
bool  CRecordFileSource::queryRecordFile(char* szRecordFileName)
{
	std::lock_guard<std::mutex> lock(RecordFileLock);

	bool bRet = false;
	//文件名字长度有误
	if (strlen(szRecordFileName) != 14) 
		return false;

	//去掉扩展名 .flv , .mp4 , .m3u8 
	if (strstr(szRecordFileName, ".flv") != NULL || strstr(szRecordFileName, ".mp4") != NULL)
		szRecordFileName[strlen(szRecordFileName) - 4] = 0x00;
	if (strstr(szRecordFileName, ".m3u8") != NULL )
		szRecordFileName[strlen(szRecordFileName) - 5] = 0x00;

	//判断是否为数字
	if (!boost::all(szRecordFileName, boost::is_digit()))
		return false;
 
	list<uint64_t>::iterator it2;
	for (it2 = fileList.begin(); it2 != fileList.end(); it2++)
	{
		if (*it2 == atoll(szRecordFileName))
		{
			bRet = true;
			break;
		}
	}

	//码流找不到
	if (ABL_MediaServerPort.hook_enable == 1 && bRet == false &&  ABL_MediaServerPort.nClientNotFound > 0)
	{
		MessageNoticeStruct msgNotice;
		msgNotice.nClient = ABL_MediaServerPort.nClientNotFound;
		sprintf(msgNotice.szMsg, "{\"app\":\"%s\",\"stream\":\"%s___ReplayFMP4RecordFile__%s\",\"mediaServerId\":\"%s\"}", m_app, m_stream, szRecordFileName, ABL_MediaServerPort.mediaServerID);
		pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
	}

	return bRet;
}
