#include "stdafx.h"
#include "CudaChanManager.h"

CCudaChanManager::CCudaChanManager()
{
	InitializeCriticalSection(&ManagerLock);
}

CCudaChanManager::~CCudaChanManager()
{
	EnterCriticalSection(&ManagerLock);
	 CUDAChanStore* cudaStore;
 
	  for (CCudaChanManagerMap::iterator iterator1 = cudaManagerMap.begin(); iterator1 != cudaManagerMap.end(); ++iterator1)
	  {
		cudaStore = (*iterator1).second;
 
		delete   cudaStore;
		cudaStore = NULL;
	  }
	  cudaManagerMap.clear();
	LeaveCriticalSection(&ManagerLock);

	DeleteCriticalSection(&ManagerLock);
}

//初始化显卡管理 
bool  CCudaChanManager::InitCudaManager(int nCudaCount)
{
	EnterCriticalSection(&ManagerLock);
	if (nCudaCount <= 0 || cudaManagerMap.size() > 0 )
	{
		LeaveCriticalSection(&ManagerLock);
		return false;
	}

	for (int i = 0; i < nCudaCount; i++)
	{
		CUDAChanStore* cudaStore = new CUDAChanStore(i);
		cudaManagerMap.insert(CCudaChanManagerMap::value_type(i, cudaStore));
	}

	LeaveCriticalSection(&ManagerLock);
	return true;
}

//获取待使用显卡序号
int  CCudaChanManager::GetCudaGPUOrder()
{
	EnterCriticalSection(&ManagerLock);
	CUDAChanStore* cudaStore;
	int            nChanCount = 40960000 ;
	int            nCurrentChan = 0 ;

	//只有一个显卡 
	if (cudaManagerMap.size() <= 1)
	{
		LeaveCriticalSection(&ManagerLock);
		return nCurrentChan ;
	}

	for (CCudaChanManagerMap::iterator iterator1 = cudaManagerMap.begin(); iterator1 != cudaManagerMap.end(); ++iterator1)
	{
		cudaStore = (*iterator1).second;

		if (cudaStore->GetSize() < nChanCount)
		{
			nCurrentChan = cudaStore->nCudaGPUOrder;
			nChanCount = cudaStore->GetSize();
		}
	}
	 
	LeaveCriticalSection(&ManagerLock);
	return nCurrentChan ;
}

//把解码序号加入管理 
bool  CCudaChanManager::AddChanToManager(int nCudaOrder, int64_t nCudaChan)
{
	EnterCriticalSection(&ManagerLock);

	CCudaChanManagerMap::iterator iterPlayer;
	CUDAChanStore* tmpPlayer;

	iterPlayer = cudaManagerMap.find(nCudaOrder);
	if (iterPlayer != cudaManagerMap.end())
	{//找到该链接
		tmpPlayer = (*iterPlayer).second;
		tmpPlayer->AddCudaChan(nCudaChan);

		LeaveCriticalSection(&ManagerLock);
		return true;
	}
	else //没有找到该连接
	{
		LeaveCriticalSection(&ManagerLock);
		return false ;
	}
}

//从管理中删除解码序号 
bool  CCudaChanManager::DeleteChanFromManager(int64_t nCudaChan)
{
  EnterCriticalSection(&ManagerLock);
	CUDAChanStore* cudaStore;
	bool           bDeleteFlag = false;

	for (CCudaChanManagerMap::iterator iterator1 = cudaManagerMap.begin(); iterator1 != cudaManagerMap.end(); ++iterator1)
	{
		cudaStore = (*iterator1).second;

		if (cudaStore->DeleteCudaChan(nCudaChan))
		{
			bDeleteFlag = true;
			break; //删除成功 
		}
 	}
 
  LeaveCriticalSection(&ManagerLock);
  return bDeleteFlag;
}
