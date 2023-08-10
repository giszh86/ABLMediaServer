#include "stdafx.h"
#include "CudaChanManager.h"

CCudaChanManager::CCudaChanManager()
{

}

CCudaChanManager::~CCudaChanManager()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	 CUDAChanStore* cudaStore; 
	 for (auto iterator1  : cudaManagerMap)
	 {
		 cudaStore = iterator1.second;
		 delete   cudaStore;
		 cudaStore = NULL;
	 }
	
	  cudaManagerMap.clear();
}

//初始化显卡管理 
bool  CCudaChanManager::InitCudaManager(int nCudaCount)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (nCudaCount <= 0 || cudaManagerMap.size() > 0 )
	{
		return false;
	}
	for (int i = 0; i < nCudaCount; i++)
	{
		CUDAChanStore* cudaStore = new CUDAChanStore(i);
		cudaManagerMap.insert(CCudaChanManagerMap::value_type(i, cudaStore));
	}

	return true;
}

//获取待使用显卡序号
int  CCudaChanManager::GetCudaGPUOrder()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	CUDAChanStore* cudaStore;
	int            nChanCount = 40960000 ;
	int            nCurrentChan = 0 ;

	//只有一个显卡 
	if (cudaManagerMap.size() <= 1)
	{
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
	 

	return nCurrentChan ;
}

//把解码序号加入管理 
bool  CCudaChanManager::AddChanToManager(int nCudaOrder, int64_t nCudaChan)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	CCudaChanManagerMap::iterator iterPlayer;
	CUDAChanStore* tmpPlayer;

	iterPlayer = cudaManagerMap.find(nCudaOrder);
	if (iterPlayer != cudaManagerMap.end())
	{//找到该链接
		tmpPlayer = (*iterPlayer).second;
		tmpPlayer->AddCudaChan(nCudaChan);
		return true;
	}
	else //没有找到该连接
	{
		return false ;
	}
}

//从管理中删除解码序号 
bool  CCudaChanManager::DeleteChanFromManager(int64_t nCudaChan)
{
std::lock_guard<std::mutex> lock(m_mutex);
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

  return bDeleteFlag;
}
