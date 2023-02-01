/*
���ܣ�
    ���е��̳߳ع��ܣ����Զ���߳�ȥִ��һ��ʵ������ͬ������
    ����̳߳أ����ﵽ֡����ʱ�ᴥ���̳߳� ������Ƶ����  	
����    2021-03-29
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "MediaSendThreadPool.h"

extern std::shared_ptr<CNetRevcBase> GetNetRevcBaseClient(NETHANDLE CltHandle);
extern bool                            DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                      pDisconnectBaseNetFifo; //������ѵ����� 

CMediaSendThreadPool::CMediaSendThreadPool(int nMaxThreadCount)
{
	nTrueMaxNetThreadPoolCount = nMaxThreadCount;
	if (nMaxThreadCount > MaxMediaSendThreadCount)
		nTrueMaxNetThreadPoolCount = MaxMediaSendThreadCount;

	if (nMaxThreadCount <= 0)
		nTrueMaxNetThreadPoolCount = 128;

	bRunFlag = true;
	nCreateThreadProcessCount = 0;//�Ѿ��������߳�����
	for (int i = 0; i < MaxMediaSendThreadCount; i++)
	{
		bExitProcessThreadFlag[i] = true;
		hProcessHandle[i] = NULL;
		threadContainClient[i].nThreadOrder = 0;
		threadContainClient[i].nTrueClientsCount = 0;
		threadContainClient[i].nMaxClientArraySize = 0;
		for (int j = 0; j < MaxMediaSendThreadCount; j++)
			threadContainClient[i].nClients[j] = 0;
	}
	WriteLog(Log_Debug, "CMediaSendThreadPool = %X ���� ", this);
}

CMediaSendThreadPool::~CMediaSendThreadPool()
{
	bRunFlag = false;
	for (int i = 0; i < nTrueMaxNetThreadPoolCount; i++)
	{
		while (!bExitProcessThreadFlag[i])
	  		Sleep(50);
		if (hProcessHandle[i] != NULL)
		{
#ifdef      OS_System_Windows
			CloseHandle(hProcessHandle[i]);
#endif
		}
	}
	WriteLog(Log_Debug, "CMediaSendThreadPool = %X ���� \r\n", this);
	malloc_trim(0);
}

//���ҿͻ����Ƿ���ڷ����߳���
bool CMediaSendThreadPool::findClientAtArray(uint64_t nClient)
{
	int i = 0;
	int j = 0;
	for ( i = 0; i < nCreateThreadProcessCount; i++)
	{
		for ( j= 0;j < threadContainClient[i].nMaxClientArraySize ;j++)
		{
			if (threadContainClient[i].nClients[j] == nClient)
			{
 				WriteLog(Log_Debug, "�ѿͻ��� nClient = %llu �Ѿ�����ý�巢���̳߳��У�����Ҫ�ټ��� ,nThreadID = %d ", nClient, i);
				return true;
			}
		}
	}
	return false;
}

//�ѿͻ��˼��뷢���̳߳� 
bool  CMediaSendThreadPool::AddClientToThreadPool(uint64_t nClient)
{
	std::lock_guard<std::mutex> lock(threadLock);
	
	//���ҿͻ����Ƿ���ڷ����߳���
	if (findClientAtArray(nClient))
		return false;

	//�Ȳ��ҿ��е��̣߳�����п����߳�����ʹ�ã�����Ҫ�����߳� 
	for (int i = 0; i < nCreateThreadProcessCount; i++)
	{
		if (threadContainClient[i].nTrueClientsCount == 0)
		{//�ҵ����е��߳� 
			threadContainClient[i].nClients[0] = nClient;
			threadContainClient[i].nTrueClientsCount = 1;
			threadContainClient[i].nMaxClientArraySize = 1;
			WriteLog(Log_Debug, "�ѿͻ��� nClient = %llu �ɹ����뵽ý�巢���̳߳� ,nThreadID = %d ", nClient,i );
			return true;
		}
	}

	if (nCreateThreadProcessCount <= nTrueMaxNetThreadPoolCount)
	{//��ǰ�߳�����û�дﵽ ���������߳��������͵�������һ���̣߳����з���
 		//����Ҳ���
		bCreateThreadFlag = false;
		unsigned long dwThread;
#ifdef OS_System_Windows
		hProcessHandle[nCreateThreadProcessCount] = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OnProcessThread, (LPVOID)this, 0, &dwThread);
#else 
		pthread_create(&hProcessHandle[nCreateThreadProcessCount], NULL, OnProcessThread, (void*)this);
#endif
		while (bCreateThreadFlag == false)
			Sleep(5);

		threadContainClient[nCreateThreadProcessCount].nThreadOrder = nCreateThreadProcessCount; //��ǰ�߳����
		threadContainClient[nCreateThreadProcessCount].nTrueClientsCount = 1;//��ǰ�߳��������Ŀͻ������� 
		threadContainClient[nCreateThreadProcessCount].nMaxClientArraySize = 1;
		threadContainClient[nCreateThreadProcessCount].nClients[0] = nClient;//�洢���̸߳����͵Ŀͻ���ID  

		WriteLog(Log_Debug, "�ѿͻ��� nClient = %llu �ɹ����뵽ý�巢���̳߳� ,nThreadID = %d ", nClient, nCreateThreadProcessCount);

		nCreateThreadProcessCount ++;

		return true;
	}
	else
	{//��ǰ�������߳��Ѿ��ﵽ ���㴴��������߳����������ٴ����̣߳������̹߳���ķ�ʽ������Ƶ 
		//���ҳ��ͻ����������ٵ��߳� 
		int  nMinThreadContainClient = MaxMediaSendThreadCount;
		int  nMinThreadContainClientThread = 0 ;//���¿ͻ����������ٵ��߳�ID 
 
		for (int i = 0; i < nCreateThreadProcessCount; i++)
		{
			if (threadContainClient[i].nTrueClientsCount < nMinThreadContainClient)
			{//�ҵ����е��߳� 
				nMinThreadContainClient = threadContainClient[i].nTrueClientsCount;
				nMinThreadContainClientThread = i;
  			}
		}
 
		//�Ѿ��ҵ��ͻ����������ٵ��߳�ID
		if (nMinThreadContainClientThread >= 0 && nMinThreadContainClientThread < nCreateThreadProcessCount)
		{
			bool FillArrayPositionFlag = false; //�Ƿ��Ѿ��ɹ�ռλ
			int  nMaxArraySize = 1;//��������±� 

			for (int i = 0; i < nCreateThreadProcessCount; i++)
			{
				if (!FillArrayPositionFlag)
				{
				   if (threadContainClient[nMinThreadContainClientThread].nClients[i] == 0)
				   {
					 threadContainClient[nMinThreadContainClientThread].nClients[i] = nClient; //��ClientID �����ڿ�ȱ��λ���� 
					 threadContainClient[nMinThreadContainClientThread].nTrueClientsCount += 1; //�ͻ���������1  

					 FillArrayPositionFlag = true; //ռλ�ɹ� 
					 WriteLog(Log_Debug, "�ѿͻ��� nClient = %llu �ɹ����뵽ý�巢���̳߳� ,nThreadID = %d ", nClient, i);
 				    }
			     }

				if (threadContainClient[nMinThreadContainClientThread].nClients[i] > 0)
				{
					nMaxArraySize = i + 1;
				}
  			 }

			threadContainClient[nMinThreadContainClientThread].nMaxClientArraySize = nMaxArraySize;//����nClient �洢λ�õ���������±� 
			WriteLog(Log_Debug, "�ѿͻ��� nClient = %llu �ɹ����뵽ý�巢���̳߳� ,nThreadID = %d , nMaxClientArraySize = %d ", nClient, nMinThreadContainClientThread,nMaxArraySize);
		}

 	   return true;
 	}
}

//�ѿͻ��˴��̳߳��Ƴ� 
bool  CMediaSendThreadPool::DeleteClientToThreadPool(uint64_t nClient)
{
	std::lock_guard<std::mutex> lock(threadLock);

	for (int i = 0; i < MaxMediaSendThreadCount; i++)
	{
		for (int j = 0; j < MaxMediaSendThreadCount; j++)
		{
			if (threadContainClient[i].nClients[j] == nClient)
			{
				threadContainClient[i].nClients[j] = 0;
				threadContainClient[i].nTrueClientsCount -= 1;

				if (threadContainClient[i].nTrueClientsCount < 0) //��������ֹ���� 
					threadContainClient[i].nTrueClientsCount = 0;

				WriteLog(Log_Debug, "�ѿͻ��� nClient = %llu ��ý�巢���̳߳��Ƴ� ", nClient);
				return true;
			}
		}
	}
	return false;
}

void CMediaSendThreadPool::MySleep(int nSleepTime)
{
	if (nSleepTime <= 0)
		Sleep(5);
	else
		Sleep(nSleepTime);
}

void CMediaSendThreadPool::ProcessFunc()
{
	int nCurrentThreadID = nCreateThreadProcessCount;
 	bExitProcessThreadFlag[nCurrentThreadID] = false;
	bCreateThreadFlag = true; //�����߳����
	int i;
	int nVideoFrameCount, nAudioFrameCount;

	while (bRunFlag)
	{
		if (threadContainClient[nCurrentThreadID].nTrueClientsCount > 0)
		{
			for (i = 0; i < threadContainClient[nCurrentThreadID].nMaxClientArraySize; i++)
			{
				if (threadContainClient[nCurrentThreadID].nClients[i] > 0)
				{
		           std::shared_ptr<CNetRevcBase> pClient= GetNetRevcBaseClient(threadContainClient[nCurrentThreadID].nClients[i]);
				   if (pClient != NULL )
				   {
					   if (pClient->netBaseNetType == NetBaseNetType_HttpHLSClientRecv)
					   {//����HLS��Ƶ����Ƶ��������ý��Դ���Ժ󻹿���֧�ֱ��û����Ƶ����Ƶ���͵���
						   pClient->SendVideo();
						   pClient->SendAudio();
						   Sleep(5);
 					   }
					   else
					   {
 						   nVideoFrameCount = pClient->m_videoFifo.GetSize();
						   nAudioFrameCount = pClient->m_audioFifo.GetSize();
						   if (nVideoFrameCount == 0 && nAudioFrameCount == 0)
						   {
							   if (threadContainClient[nCurrentThreadID].nTrueClientsCount == 1)
							   {//������߳�ֻ��1�����ӣ����Եȴ�5����
								   Sleep(10);
								   continue;
							   }
							   else //������̳߳���2�����ӣ�����ѭ�����ܵȴ�ʱ�� ����Ϊ����Ҫ������һ�����ӵ����� 
							   {
								   Sleep(10); //��ֹ��ѭ��
								   continue;
							   }
						   }

						   for (i = 0; i < nVideoFrameCount; i++)
							   pClient->SendVideo();

						   for (i = 0; i < nAudioFrameCount; i++)
							   pClient->SendAudio();
					   }
				   }
				   else
				   {//�Ҳ����ͻ��� 
					   WriteLog(Log_Debug, "�Ҳ����ͻ��� ,�Ѹÿͻ��� nClient = %llu ��ý�巢���̳߳��Ƴ� ", threadContainClient[nCurrentThreadID].nClients[i]);

					   threadContainClient[nCurrentThreadID].nClients[i] = 0; //�Ѹ�ͨ������Ϊ 0 ��
					   threadContainClient[nCurrentThreadID].nTrueClientsCount -= 1;//����������1 

					   if (threadContainClient[nCurrentThreadID].nTrueClientsCount < 0)
						   threadContainClient[nCurrentThreadID].nTrueClientsCount = 0 ;
				   }
				}
 		    }
 		}else 
 		  Sleep(5);
 	}
	bExitProcessThreadFlag[nCurrentThreadID] = true;
}
 
void* CMediaSendThreadPool::OnProcessThread(void* lpVoid)
{
	int nRet = 0;
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

