// cudaEncodeDLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "cudaEncodeDLL.h"

#include "CudaVideoEncode.h"

bool  ABL_CudeCodecDLL_InitFlag = false;
typedef std::shared_ptr<CCudaVideoEncode>                       CCudaVideoEncode_ptr;
typedef std::map<uint64_t, CCudaVideoEncode_ptr>      CCudaVideoEncodeMap;
uint64_t                                                          ABL_nCudaVideoEncodeNumber = 1;
CCudaVideoEncodeMap                                               xh_ABLCudaVideoEncodeMap;
std::mutex                                                        ABL_CudaVideoEncodeLock;
int                                                               ABL_nCudaGPUCount = 0; //英伟达显卡数量
CCudaChanManager*                                                 pCudaChanManager = NULL;

//创建编码客户端
bool CreateCudaVideoEncodeClient(cudaEncodeVideo_enum videoCodec, cudaEncodeVideo_enum yuvType, int nWidth, int nHeight, uint64_t& nCudaChan)
{
	std::lock_guard<std::mutex> lock(ABL_CudaVideoEncodeLock);

	if (!ABL_CudeCodecDLL_InitFlag || ABL_nCudaGPUCount <= 0 || !(yuvType == cudaEncodeVideo_YUV420 || yuvType == cudaEncodeVideo_YV12 || yuvType == cudaEncodeVideo_NV12))
		return false;

	nCudaChan = 0;
	CCudaVideoEncode_ptr pXHClient = NULL;
	try
	{
		pXHClient = std::make_shared<CCudaVideoEncode>(videoCodec, yuvType, nWidth, nHeight, ABL_nCudaVideoEncodeNumber);
	}
	catch (const std::exception &e)
	{
		pXHClient.reset();
		return false;
	}

	std::pair<std::map<uint64_t, CCudaVideoEncode_ptr>::iterator, bool> ret =
		xh_ABLCudaVideoEncodeMap.insert(std::make_pair(pXHClient->m_CudaChan, pXHClient));
	if (!ret.second)
	{
		pXHClient.reset();
		return false;
	}
	nCudaChan = ABL_nCudaVideoEncodeNumber;

	ABL_nCudaVideoEncodeNumber++;
	return true;
}

//删除编码客户端 
bool  DeleteCudaVideoEncodeClient(uint64_t nCudaChan)
{
	std::lock_guard<std::mutex> lock(ABL_CudaVideoEncodeLock);

	CCudaVideoEncodeMap::iterator iterator1;

	iterator1 = xh_ABLCudaVideoEncodeMap.find(nCudaChan);
	if (iterator1 != xh_ABLCudaVideoEncodeMap.end())
	{
		xh_ABLCudaVideoEncodeMap.erase(iterator1);
		return true;
	}
	else
	{
		return false;
	}
}

//获取解码客户端 
CCudaVideoEncode_ptr GetCudaVideoEncodeClient(uint64_t nCudaChan)
{
	std::lock_guard<std::mutex> lock(ABL_CudaVideoEncodeLock);

	CCudaVideoEncodeMap::iterator iterator1;
	CCudaVideoEncode_ptr   pClient = NULL;

	iterator1 = xh_ABLCudaVideoEncodeMap.find(nCudaChan);
	if (iterator1 != xh_ABLCudaVideoEncodeMap.end())
	{
		pClient = (*iterator1).second;

		return pClient;
	}
	else
	{
		return NULL;
	}
}

CUDAENCODEDLL_API int cudaEncode_GetDeviceGetCount()
{
	if (!ABL_CudeCodecDLL_InitFlag)
		return -1;

	int nGpu = 0;
	if (cuDeviceGetCount(&nGpu) == CUDA_SUCCESS)
		return nGpu;
	else
		return -1;
}

CUDAENCODEDLL_API bool cudaEncode_Init()
{
	CUresult nRet = (CUresult)0;
	if (ABL_CudeCodecDLL_InitFlag == false)
	{
		nRet = cuInit(0);
		if (nRet == CUDA_SUCCESS)
		{
			ABL_CudeCodecDLL_InitFlag = true;
			ABL_nCudaGPUCount = cudaEncode_GetDeviceGetCount();

			pCudaChanManager = new CCudaChanManager();
			pCudaChanManager->InitCudaManager(ABL_nCudaGPUCount);
			return true;
		}
		else
		{
			return false;
		}
	}
	return true;
}

CUDAENCODEDLL_API bool cudaEncode_UnInit()
{
	//清除硬编资源 
	std::lock_guard<std::mutex> lock(ABL_CudaVideoEncodeLock);
	CCudaVideoEncodeMap::iterator iterator1;

	for (iterator1 = xh_ABLCudaVideoEncodeMap.begin(); iterator1 != xh_ABLCudaVideoEncodeMap.end(); ++iterator1)
	{
		CCudaVideoEncode_ptr  pClient = (*iterator1).second;
		pClient.reset();
	}
	xh_ABLCudaVideoEncodeMap.clear();
 
	if (pCudaChanManager != NULL)
	{
		delete pCudaChanManager;
		pCudaChanManager = NULL;
	}
	return true;
}

CUDAENCODEDLL_API bool  cudaEncode_GetDeviceName(int nOrder, char* szName)
{
	if (!ABL_CudeCodecDLL_InitFlag)
		return false;

	CUdevice cuDevice = 0;
	if (cuDeviceGet(&cuDevice, nOrder) != CUDA_SUCCESS)
		return false;

	char szDeviceName[256] = { 0 };
	if (cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice) != CUDA_SUCCESS)
		return false;

	if (strlen(szDeviceName) > 0)
	{
		strcpy(szName, szDeviceName);
		return true;
	}
	else
		return false;
}

//创建解码句柄 
CUDAENCODEDLL_API bool cudaEncode_CreateVideoEncode(cudaEncodeVideo_enum videoCodec, cudaEncodeVideo_enum yuvType, int nWidth, int nHeight, uint64_t& nCudaChan)
{
	return CreateCudaVideoEncodeClient(videoCodec, yuvType, nWidth, nHeight, nCudaChan);
}

//视频解码 
CUDAENCODEDLL_API int cudaEncode_CudaVideoEncode(uint64_t nCudaChan, unsigned char* pYUVData, int nYUVLength,char* pOutEncodeData)
{
	CCudaVideoEncode_ptr cudaEncode = GetCudaVideoEncodeClient(nCudaChan);
	if (cudaEncode)
	{
		return  cudaEncode->cudaEncodeVideo(pYUVData, nYUVLength, pOutEncodeData);
 	}
	else
	{
		pOutEncodeData = NULL;
		return 0 ;
 	}
}

CUDAENCODEDLL_API bool cudaEncode_DeleteVideoEncode(uint64_t nCudaChan)
{
	return DeleteCudaVideoEncodeClient(nCudaChan);
}
