#ifndef _CudaChanManager_H
#define _CudaChanManager_H

#include <map>
#include <mutex>
using namespace std;

typedef map<int64_t,int64_t> CUDAChan;

class CUDAChanStore
{
public:
  CUDAChan cudaChan ;
  int nCudaGPUOrder;//cuda 序号 0 、 1 、2 、3 、4 

  CUDAChanStore(int nOrder)
  {
	  nCudaGPUOrder = nOrder;
  }

  ~CUDAChanStore()
  {
	  
  }

  int AddCudaChan(int64_t nCudaChan) //获取cuda显卡序号
  {
	  cudaChan.insert(pair<int64_t,int64_t>(nCudaChan,nCudaChan));
	  return true;
  }
  
  bool DeleteCudaChan(int64_t nCudaChan) //删除cuda解码通道 
  {
	  CUDAChan::iterator iterPlayer;

	  iterPlayer = cudaChan.find(nCudaChan);
	  if (iterPlayer != cudaChan.end())
	  { 
		  cudaChan.erase(iterPlayer);

		  return true ;
	  }
	  else  
	  {
		  return false ;
	  }
  }
  
  int GetSize()
  {
	  return cudaChan.size();
  }

};

typedef map<long, CUDAChanStore*, less<long> > CCudaChanManagerMap; //cuda通道号存储

class CCudaChanManager
{
public:
	CCudaChanManager();
	~CCudaChanManager();
	std::mutex              m_mutex;
	//pthread_mutex_t      ManagerLock;
	CCudaChanManagerMap  cudaManagerMap;

	bool  InitCudaManager(int nCudaCount);
	int   GetCudaGPUOrder();
	bool  AddChanToManager(int nCudaOrder,int64_t nCudaChan);
	bool  DeleteChanFromManager(int64_t nCudaChan);
};

#endif
