/*
功能：
    自研的线程池功能，异步方式通知线程池，会触发执行基类成员的 SendVideo()，SendAuido()函数，
    从而完成视频、音频的发送功能 
日期    2024-03-25
作者    罗家兄弟
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
	WriteLog(Log_Debug, "CMediaSendThreadPool 构造 = %X, ", this);
}

CMediaSendThreadPool::~CMediaSendThreadPool()
{
	bRunFlag = false;
	int i;
	std::lock_guard<std::mutex> lock(threadLock);
	for (i = 0; i < nTrueNetThreadPoolCount; i++)
	{//通知所有线程
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
	WriteLog(Log_Debug, "CMediaSendThreadPool 析构 = %X  \r\n", this );
}

void CMediaSendThreadPool::ProcessFunc()
{
	int nCurrentThreadID = nGetCurrentThreadOrder;
 	bExitProcessThreadFlag[nCurrentThreadID] = false;
	uint64_t nClientID;
	int      i,nVideoFrameCount, nAudioFrameCount;

	i = nVideoFrameCount = nAudioFrameCount = 0;
	bCreateThreadFlag = true; //创建线程完毕
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
	pthread_detach(pthread_self()); //让子线程和主线程分离，当子线程退出时，自动释放子线程内存
#endif

	CMediaSendThreadPool* pThread = (CMediaSendThreadPool*)lpVoid;
	pThread->ProcessFunc();

#ifndef OS_System_Windows
	pthread_exit((void*)&nRet); //退出线程
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
	{//找到 
		nThreadThread = (*it).second;
	}
	else
	{//尚未加入过
		nThreadThread = nThreadProcessCount % nTrueNetThreadPoolCount;
 		clientThreadMap.insert(ClientProcessThreadMap::value_type(nClientID, nThreadThread));
		nThreadProcessCount  ++;
	}

	m_NetHandleQueue[nThreadThread].push(nClientID);
	cv[nThreadThread].notify_one();

	return true;
}
