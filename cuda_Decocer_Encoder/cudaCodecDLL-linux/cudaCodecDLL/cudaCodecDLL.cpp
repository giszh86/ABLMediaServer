// cudaCodecDLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "cudaCodecDLL.h"
#include "CudaVideoDecode.h"

bool  ABL_CudeCodecDLL_InitFlag = false;
typedef std::shared_ptr<CCudaVideoDecode> CCudaVideoDecode_ptr;
typedef std::map<uint64_t, CCudaVideoDecode_ptr> CCudaVideoDecodeMap;
uint64_t                                                          ABL_nCudaVideoDecodeNumber = 1;
CCudaVideoDecodeMap                                               xh_ABLCudaVideoDecodeMap;
std::mutex                                                        ABL_CudaVideoDecodeLock;
int                                                               ABL_nCudaGPUCount = 0; //英伟达显卡数量
CCudaChanManager*                                                 pCudaChanManager = NULL;

//创建解码客户端
bool CreateCudaVideoDecodeClient(cudaCodecVideo_enum videoCodec, cudaCodecVideo_enum outYUVType, int nWidth, int nHeight, uint64_t& nCudaChan)
{
	std::lock_guard<std::mutex> lock(ABL_CudaVideoDecodeLock);
    
	if (!ABL_CudeCodecDLL_InitFlag || ABL_nCudaGPUCount <= 0)
		return false;

	nCudaChan = 0;
	CCudaVideoDecode_ptr pXHClient = NULL;
	try
	{
		do
		{
			pXHClient = std::make_shared<CCudaVideoDecode>(videoCodec, outYUVType, nWidth, nHeight, ABL_nCudaVideoDecodeNumber);
		} while (pXHClient == NULL);
 	}
	catch (const std::exception &e)
	{
 		return false ;
	}

	std::pair<std::map<uint64_t, CCudaVideoDecode_ptr>::iterator, bool> ret =
		xh_ABLCudaVideoDecodeMap.insert(std::make_pair(pXHClient->m_CudaChan, pXHClient));
	if (!ret.second)
	{
		pXHClient.reset();
 		return false;
	}
	nCudaChan = ABL_nCudaVideoDecodeNumber;

	ABL_nCudaVideoDecodeNumber ++;
 	return true;
}

//删除解码客户端 
bool  DeleteCudaVideoDecodeClient(uint64_t nCudaChan)
{
	std::lock_guard<std::mutex> lock(ABL_CudaVideoDecodeLock);
 
	CCudaVideoDecodeMap::iterator iterator1;
 
	iterator1 = xh_ABLCudaVideoDecodeMap.find(nCudaChan);
	if (iterator1 != xh_ABLCudaVideoDecodeMap.end())
	{
		xh_ABLCudaVideoDecodeMap.erase(iterator1);

  		return true;
	}
	else
	{
		return false;
	}
}

//获取解码客户端 
CCudaVideoDecode_ptr GetCudaVideoDecodeClient(uint64_t nCudaChan)
{
	std::lock_guard<std::mutex> lock(ABL_CudaVideoDecodeLock);

	CCudaVideoDecodeMap::iterator iterator1;
	CCudaVideoDecode_ptr   pClient = NULL;

	iterator1 = xh_ABLCudaVideoDecodeMap.find(nCudaChan);
	if (iterator1 != xh_ABLCudaVideoDecodeMap.end())
	{
		pClient = (*iterator1).second;

		return pClient;
	}
	else
	{
		return NULL;
	}
}


 bool cudaCodec_Init()
{
	CUresult nRet = (CUresult)0;
	if (ABL_CudeCodecDLL_InitFlag == false)
	{
		nRet = cuInit(0);
		if (nRet == CUDA_SUCCESS)
		{
			ABL_CudeCodecDLL_InitFlag = true;
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			cuDeviceGetCount(&ABL_nCudaGPUCount) ;

			pCudaChanManager = new CCudaChanManager();
			pCudaChanManager->InitCudaManager(ABL_nCudaGPUCount);

 			return true;
		}
		else
		{
#if  0 
			char szBuff[256] = { 0 };
			sprintf(szBuff, "nRet = %llu", (int)nRet);
			FILE* fFile = fopen("D:\\cuda2.txt", "wb");
			fwrite(szBuff, 1, strlen(szBuff), fFile);
			fclose(fFile);
 #endif
			return false;
		}
	}
	return true;
}

 bool cudaCodec_UnInit()
{
	std::lock_guard<std::mutex> lock(ABL_CudaVideoDecodeLock);

	CCudaVideoDecodeMap::iterator iterator1;
	CCudaVideoDecode_ptr   pClient = NULL;
 	 
	for (iterator1 = xh_ABLCudaVideoDecodeMap.begin(); iterator1 != xh_ABLCudaVideoDecodeMap.end();++iterator1)
	{
		pClient = (*iterator1).second;
		pClient.reset();
 	}
	xh_ABLCudaVideoDecodeMap.clear();

	if (pCudaChanManager != NULL)
	{
		delete pCudaChanManager;
		pCudaChanManager = NULL;
	}

	return true;
}

 int cudaCodec_GetDeviceGetCount()
{
	if (!ABL_CudeCodecDLL_InitFlag)
		return -1;

	int nGpu = 0;
	if (cuDeviceGetCount(&nGpu) == CUDA_SUCCESS)
		return nGpu;
	else
		return -1;
}
 
 bool  cudaCodec_GetDeviceName(int nOrder, char* szName)
{
	if (!ABL_CudeCodecDLL_InitFlag)
		return false ;

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

 int  cudaCodec_GetDeviceUse(int nOrder)
{
	if (!ABL_CudeCodecDLL_InitFlag)
		return -1;

#if 0
	uint64_t nChan = 0;

	CreateCudaVideoDecodeClient(0, cudaCodecVideo_H264, 1920, 1080, nChan);
	CCudaVideoDecode_ptr pClient = GetCudaVideoDecodeClient(nChan) ;

	uint64_t nChan2 = 0;
	CreateCudaVideoDecodeClient(0, cudaCodecVideo_H264, 1920, 1080, nChan2);
	CCudaVideoDecode_ptr pClient2 = GetCudaVideoDecodeClient(nChan2) ;

	DeleteCudaVideoDecodeClient(nChan);
	DeleteCudaVideoDecodeClient(nChan2);

#endif

#if 0
	int64_t nChan = 0;
	CCudaVideoDecode* cudaDecode = new CCudaVideoDecode(0, (cudaCodecVideo_enum)cudaCodecVideo_H264, 1920, 1080,nChan);
	delete cudaDecode;
	cudaDecode = NULL;
#endif

#if 0
	NvDecoder* dec;
	CUdevice cuDevice = 0;
	int  iGpu = 0;
	Rect cropRect = {};
	Dim resizeDim = {};

	cuDeviceGet(&cuDevice, iGpu);

	CUcontext cuContext = NULL;
	cuCtxCreate(&cuContext, 0, cuDevice); //cuCtxDestroy

	dec = new NvDecoder(cuContext, 1920, 1080, false, (cudaVideoCodec) cudaVideoCodec_H264, NULL, false, false, &cropRect, &resizeDim);

	//cuCtxDestroy(cuContext);  //不用单独调用 cuCtxDestroy(cuContext) ，NvDecoder 析构函数里面自动调用 删除 

	delete dec;
	dec = NULL;
#endif 

	return  0;
}

//创建解码句柄 
 bool cudaCodec_CreateVideoDecode(cudaCodecVideo_enum videoCodec, cudaCodecVideo_enum outYUVType, int nWidth, int nHeight, uint64_t& nCudaChan)
{
	//只支持2两种YUV格式输出
	if (!(outYUVType == cudaCodecVideo_YV12 || outYUVType == cudaCodecVideo_NV12))
		return false;

	return CreateCudaVideoDecodeClient(videoCodec, outYUVType, nWidth, nHeight, nCudaChan);
}

//视频解码 
 unsigned char* cudaCodec_CudaVideoDecode(uint64_t nCudaChan, unsigned char* pVideoData, int nVideoLength, int& nDecodeFrameCount, int& nOutDecodeLength)
{
	CCudaVideoDecode_ptr cudaDecode = GetCudaVideoDecodeClient(nCudaChan);
	if (cudaDecode)
	{
		return cudaDecode->CudaVideoDecode(pVideoData, nVideoLength, nDecodeFrameCount, nOutDecodeLength);
	}
	else
	{
		nDecodeFrameCount = 0;
		nOutDecodeLength = 0;
        return NULL ;
	}
}

//删除解码句柄
 bool cudaCodec_DeleteVideoDecode(uint64_t nCudaChan)
{
	return DeleteCudaVideoDecodeClient(nCudaChan);
}

//返回硬解数量 
 int cudaCodec_GetCudaDecodeCount()
{
	std::lock_guard<std::mutex> lock(ABL_CudaVideoDecodeLock);

 	return xh_ABLCudaVideoDecodeMap.size();
}