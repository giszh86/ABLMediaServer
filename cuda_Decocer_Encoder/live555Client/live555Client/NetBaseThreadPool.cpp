/*
���ܣ�
    ���е��̳߳ع��ܣ����Զ���߳�ȥִ��һ��ʵ������ͬ������
    ����̳߳أ����������ݴﵽʱ�����߳�ִ�� 	
����    2021-03-29
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetBaseThreadPool.h"

extern boost::shared_ptr<CNetRevcBase> GetNetRevcBaseClient(NETHANDLE CltHandle);

CNetBaseThreadPool::CNetBaseThreadPool(int nThreadCount)
{
	nThreadProcessCount = 0;
 	nTrueNetThreadPoolCount = nThreadCount;
	if (nThreadCount > MaxNetHandleQueueCount)
		nTrueNetThreadPoolCount = MaxNetHandleQueueCount;

	if (nThreadCount <= 0)
		nTrueNetThreadPoolCount = 64 ;

	pthread_mutex_init(&pThreadPollLock, NULL);

 	bRunFlag = true;
	nGetCurrentThreadOrder = 0;
	for (int i = 0; i < nTrueNetThreadPoolCount; i++)
	{
		bCreateThreadFlag = false;
 		hProcessHandle[i] = pthread_create(&hReadDataThread[i], NULL, OnProcessThread, (void*)this);;
		usleep(20 * OneMicroSecondTime);
	}
	Rtsp_WriteLog(Log_Debug, "CNetBaseThreadPool= %X, ���� ", this);
}

CNetBaseThreadPool::~CNetBaseThreadPool()
{
	StopTask();

	pthread_mutex_destroy(&pThreadPollLock);
	Rtsp_WriteLog(Log_Debug, "CNetBaseThreadPool %X ���� \r\n", this );
}

void  CNetBaseThreadPool::StopTask()
{
	if (bRunFlag)
	{
		bRunFlag = false;
		usleep(100 * OneMicroSecondTime);
		for (int i = 0; i < nTrueNetThreadPoolCount; i++)
		{
			while (!bExitProcessThreadFlag[i])
				usleep(50 * OneMicroSecondTime);
 		}
	}
}

void CNetBaseThreadPool::ProcessFunc()
{
	pthread_mutex_lock(&pThreadPollLock);
 		int nCurrentThreadID = nGetCurrentThreadOrder;
		Rtsp_WriteLog(Log_Debug, "CNetBaseThreadPool = %X nCurrentThreadID = %d ", this, nCurrentThreadID);
 		bExitProcessThreadFlag[nCurrentThreadID] = false;
	    bCreateThreadFlag = true; //�����߳����
		nGetCurrentThreadOrder ++;
	pthread_mutex_unlock(&pThreadPollLock);
	uint64_t nClientID;

	while (bRunFlag)
	{
		if (m_NetHandleQueue[nCurrentThreadID].pop(nClientID))
		{
			boost::shared_ptr<CNetRevcBase> pClient = GetNetRevcBaseClient(nClientID);
			if (pClient != NULL)
			{
				pClient->ProcessNetData();//����ִ��
			}
		}else
			usleep(5 * OneMicroSecondTime);
 	}
	bExitProcessThreadFlag[nCurrentThreadID] = true;
}

void* CNetBaseThreadPool::OnProcessThread(void* lpVoid)
{
	pthread_detach(pthread_self()); //�����̺߳����̷߳��룬�����߳��˳�ʱ���Զ��ͷ����߳��ڴ�
    int nRet2 = 0 ;

	CNetBaseThreadPool* pThread = (CNetBaseThreadPool*)lpVoid;
	pThread->ProcessFunc();
	
	pthread_exit((void*)&nRet2); //�˳��߳�
	return  0;
}

bool CNetBaseThreadPool::InsertIntoTask(uint64_t nClientID)
{
	std::lock_guard<std::mutex> lock(threadLock);

	int               nThreadThread = 0;
	ClientProcessThreadMap::iterator it;

	it = clientThreadMap.find(nClientID);
	if (it != clientThreadMap.end())
	{//�ҵ� 
		nThreadThread = (*it).second;
	}
	else
	{//��δ�����
		nThreadThread = nThreadProcessCount % nTrueNetThreadPoolCount;
 		clientThreadMap.insert(ClientProcessThreadMap::value_type(nClientID, nThreadThread));
		nThreadProcessCount  ++;
	}

	m_NetHandleQueue[nThreadThread].push(nClientID);

	return true;
}
