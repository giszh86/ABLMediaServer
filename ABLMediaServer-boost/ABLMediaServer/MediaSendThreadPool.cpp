/*
���ܣ�
    ���е��̳߳ع��ܣ��첽��ʽ֪ͨ�̳߳أ��ᴥ��ִ�л����Ա�� SendVideo()��SendAuido()������
    �Ӷ������Ƶ����Ƶ�ķ��͹��� 
����    2024-03-25
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "MediaSendThreadPool.h"

extern boost::shared_ptr<CNetRevcBase> GetNetRevcBaseClient(NETHANDLE CltHandle);

CMediaSendThreadPool::CMediaSendThreadPool(int nThreadCount)
{
	nThreadProcessCount = 0;
 	nTrueNetThreadPoolCount = nThreadCount;
	if (nThreadCount > MaxNetHandleQueueCount)
		nTrueNetThreadPoolCount = MaxNetHandleQueueCount;

	if (nThreadCount <= 0)
		nTrueNetThreadPoolCount = 64 ;

	unsigned long dwThread;
	bRunFlag = true;
	for (int i = 0; i < nTrueNetThreadPoolCount; i++)
	{
		nGetCurrentThreadOrder = i;
		bCreateThreadFlag = false;
#ifdef OS_System_Windows
		hProcessHandle[i] = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OnProcessThread, (LPVOID)this, 0, &dwThread);
#else
		pthread_create(&hProcessHandle[i], NULL, OnProcessThread, (void*)this);
#endif
		while (bCreateThreadFlag == false)
			Sleep(5);
	}
	WriteLog(Log_Debug, "CMediaSendThreadPool ���� = %X, ", this);
}

CMediaSendThreadPool::~CMediaSendThreadPool()
{
	bRunFlag = false;
	int i;
	std::lock_guard<std::mutex> lock(threadLock);
	for (i = 0; i < nTrueNetThreadPoolCount; i++)
	{//֪ͨ�����߳�
 		cv[i].notify_one();
	}

	for ( i = 0; i < nTrueNetThreadPoolCount; i++)
	{
		while (!bExitProcessThreadFlag[i])
	  		Sleep(50);
#ifdef  OS_System_Windows
	    CloseHandle(hProcessHandle[i]);
#endif 
	}
	WriteLog(Log_Debug, "CMediaSendThreadPool ���� = %X  \r\n", this );
}

void CMediaSendThreadPool::ProcessFunc()
{
	int nCurrentThreadID = nGetCurrentThreadOrder;
 	bExitProcessThreadFlag[nCurrentThreadID] = false;
	uint64_t nClientID;
	int      i,nVideoFrameCount, nAudioFrameCount;

	i = nVideoFrameCount = nAudioFrameCount = 0;
	bCreateThreadFlag = true; //�����߳����
	while (bRunFlag)
	{
		if (m_NetHandleQueue[nCurrentThreadID].pop(nClientID) && bRunFlag )
		{
			boost::shared_ptr<CNetRevcBase> pClient = GetNetRevcBaseClient(nClientID);
			if (pClient != NULL)
			{
				if (pClient->bRunFlag)
				{
					nVideoFrameCount = pClient->m_videoFifo.GetSize();
					nAudioFrameCount = pClient->m_audioFifo.GetSize();

					for (i = 0; i < nVideoFrameCount; i++)
						pClient->SendVideo();

					for (i = 0; i < nAudioFrameCount; i++)
						pClient->SendAudio();
				}
			}
		}
		else
		{
			if (bRunFlag)
			{
 			  std::unique_lock<std::mutex> lck(mtx[nCurrentThreadID]);
			  cv[nCurrentThreadID].wait(lck);
			}
			else
				break;
		}
 	}
	bExitProcessThreadFlag[nCurrentThreadID] = true;
}

void* CMediaSendThreadPool::OnProcessThread(void* lpVoid)
{
	int nRet = 0 ;
#ifndef OS_System_Windows
	pthread_detach(pthread_self()); //�����̺߳����̷߳��룬�����߳��˳�ʱ���Զ��ͷ����߳��ڴ�
#endif

	CMediaSendThreadPool* pThread = (CMediaSendThreadPool*)lpVoid;
	pThread->ProcessFunc();

#ifndef OS_System_Windows
	pthread_exit((void*)&nRet); //�˳��߳�
#endif
	return  0;
}

bool CMediaSendThreadPool::InsertIntoTask(uint64_t nClientID)
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
	cv[nThreadThread].notify_one();

	return true;
}
