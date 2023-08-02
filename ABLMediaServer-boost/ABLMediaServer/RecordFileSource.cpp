/*
���ܣ�
    1��װ���ϵ���ʷ¼���ļ�����
    2����¼���ļ����ֽ�����������
	3��������¼���ļ��ļ���׷�ӵ�list��β��
	4��ɾ�����ڵ�¼���ļ�
	5������ app\ stream ��ʱ��� ���ҳ���������������¼���ļ����� 
	6������ app\ stream \ һ��¼������ ���жϸ��ļ��Ƿ����  
	 
����    2022-01-13
����    �޼��ֵ� 
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "RecordFileSource.h"
extern MediaServerPort                       ABL_MediaServerPort;
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                            pMessageNoticeFifo;          //��Ϣ֪ͨFIFO

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

//�޸Ĺ���¼���ļ�
bool  CRecordFileSource::UpdateExpireRecordFile(char* szNewFileName)
{
	std::lock_guard<std::mutex> lock(RecordFileLock);
	uint64_t nGetFile;
	uint64_t nSecond = 0; 
	char    szDateTime[128] = { 0 };
	bool    bUpdateFlag = false;

	if (fileList.size() <= 0 )
	{
		WriteLog(Log_Debug, "UpdateExpireRecordFile %s ��δ��¼���ļ� ,������Ϊ %s ", m_szShareURL, szNewFileName);
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
			//����޸�ʧ�ܣ������Ժ��ٴ��޸�
			if (rename(szDeleteFile,szNewFileName) != 0 )
			{
				fileList.push_back(nGetFile); 
				WriteLog(Log_Debug, "UpdateExpireRecordFile %s �޸��ļ� %llu.mp4 ʧ�ܣ������Ժ����޸� ", m_szShareURL, nGetFile);
				break;
			}
			else
			{
			  bUpdateFlag = true;

			  //���һ������һ��mp4�ļ�֪ͨ 
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
		WriteLog(Log_Debug, "UpdateExpireRecordFile %s û��¼���ļ����� ,������Ϊ %s ", m_szShareURL, szNewFileName);
 
	return bUpdateFlag ;
}

//��ѯ¼���ļ��Ƿ���� 
bool  CRecordFileSource::queryRecordFile(char* szRecordFileName)
{
	std::lock_guard<std::mutex> lock(RecordFileLock);

	bool bRet = false;
	//�ļ����ֳ�������
	if (strlen(szRecordFileName) != 14) 
		return false;

	//ȥ����չ�� .flv , .mp4 , .m3u8 
	if (strstr(szRecordFileName, ".flv") != NULL || strstr(szRecordFileName, ".mp4") != NULL)
		szRecordFileName[strlen(szRecordFileName) - 4] = 0x00;
	if (strstr(szRecordFileName, ".m3u8") != NULL )
		szRecordFileName[strlen(szRecordFileName) - 5] = 0x00;

	//�ж��Ƿ�Ϊ����
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

	//�����Ҳ���
	if (ABL_MediaServerPort.hook_enable == 1 && bRet == false &&  ABL_MediaServerPort.nClientNotFound > 0)
	{
		MessageNoticeStruct msgNotice;
		msgNotice.nClient = ABL_MediaServerPort.nClientNotFound;
		sprintf(msgNotice.szMsg, "{\"app\":\"%s\",\"stream\":\"%s___ReplayFMP4RecordFile__%s\",\"mediaServerId\":\"%s\"}", m_app, m_stream, szRecordFileName, ABL_MediaServerPort.mediaServerID);
		pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
	}

	return bRet;
}
