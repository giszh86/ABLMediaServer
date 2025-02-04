/*
  功能:
     实现调用cuda 进行硬件视频编码  
*/

#include "stdafx.h"
#include "CudaVideoEncode.h"

extern CCudaChanManager*     pCudaChanManager ;
unsigned char spsFrame[5]={0x00,0x00,0x00,0x01,0x67};
unsigned char idrFrame[5]={0x00,0x00,0x00,0x01,0x65};

CCudaVideoEncode::CCudaVideoEncode(cudaEncodeVideo_enum videoCodec, cudaEncodeVideo_enum yuvType, int nWidth, int nHeight, uint64_t nCudaChan)
{
	bInitFlag = false;
	bCopySpsPpsSuccessFlag = false ;
  	nSpsPpsBufferLength = 0;
	
 	m_videoCodec = videoCodec;
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_CudaChan = nCudaChan;
	int        nGpu = pCudaChanManager->GetCudaGPUOrder();
 
 	cuDevice = 0;
	if (cuDeviceGet(&cuDevice, nGpu) != CUDA_SUCCESS)
		return;

	m_yuvType = yuvType;
	cuContext = NULL;
	if (cuCtxCreate(&cuContext, 0, cuDevice) != CUDA_SUCCESS)
		return;

	if (cudaEncodeVideo_YUV420 == yuvType)
		nvBufferFormat = NV_ENC_BUFFER_FORMAT_IYUV;
	else if (cudaEncodeVideo_YV12 == yuvType)
		nvBufferFormat = NV_ENC_BUFFER_FORMAT_YV12;
	else if (cudaEncodeVideo_NV12 == yuvType)
		nvBufferFormat = NV_ENC_BUFFER_FORMAT_NV12;

	enc = new NvEncoderCuda(cuContext, nWidth, nHeight, nvBufferFormat);
	if (enc == nullptr)
		return;

	initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
	encodeConfig = { NV_ENC_CONFIG_VER };

	initializeParams.encodeConfig = &encodeConfig;

	if (videoCodec == cudaEncodeVideo_H264)
		enc->CreateDefaultEncoderParams(&initializeParams, NV_ENC_CODEC_H264_GUID, NV_ENC_PRESET_P7_GUID, NV_ENC_TUNING_INFO_HIGH_QUALITY); //NV_ENC_CODEC_HEVC_GUID
	else if (videoCodec == cudaEncodeVideo_HEVC)
		enc->CreateDefaultEncoderParams(&initializeParams, NV_ENC_CODEC_HEVC_GUID, NV_ENC_PRESET_P4_GUID, NV_ENC_TUNING_INFO_HIGH_QUALITY);

		//encodeConfig.encodeCodecConfig.h264Config.idrPeriod = 25;//IDR 帧的周期为 25 帧；
		//encodeConfig.gopLength = 25;// GOP 长度为 25 帧；
		//encodeConfig.frameIntervalP = 1;// 编码输出帧类型为 I、P、P、；
		//encodeConfig.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;// 码率控制模式为恒定比特率（CBR）；
		//encodeConfig.rcParams.averageBitRate = static_cast<unsigned int>(5.0f * initializeParams.encodeWidth * initializeParams.encodeHeight);// 平均比特率为图像宽度和高度的乘积的 5 倍；
		//encodeConfig.rcParams.vbvBufferSize = (encodeConfig.rcParams.averageBitRate * initializeParams.frameRateDen / initializeParams.frameRateNum) * 5;// VBV 缓存大小为平均比特率乘以帧率的倒数再乘以 5；
		//encodeConfig.rcParams.maxBitRate = encodeConfig.rcParams.averageBitRate;// 最大比特率等于平均比特率；
		//encodeConfig.rcParams.vbvInitialDelay = encodeConfig.rcParams.vbvBufferSize;// VBV 初始延迟等于 VBV 缓存大小。

	encodeConfig.gopLength = NVENC_INFINITE_GOPLENGTH;
	encodeConfig.frameIntervalP = 1;
	encodeConfig.profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;
	encodeConfig.encodeCodecConfig.h264Config.idrPeriod = 25;//IDR 帧的周期为 25 帧；
	encodeConfig.rcParams.rateControlMode = NV_ENC_PARAMS_RC_VBR;
	//encodeConfig.rcParams.multiPass = NV_ENC_TWO_PASS_FULL_RESOLUTION;
	encodeConfig.rcParams.averageBitRate = static_cast<unsigned int>(5.0f * initializeParams.encodeWidth * initializeParams.encodeHeight);
	encodeConfig.rcParams.vbvBufferSize =(encodeConfig.rcParams.averageBitRate * initializeParams.frameRateDen /initializeParams.frameRateNum) *3;
	encodeConfig.rcParams.maxBitRate = encodeConfig.rcParams.averageBitRate;
	encodeConfig.rcParams.vbvInitialDelay = encodeConfig.rcParams.vbvBufferSize;

	//NV_ENC_PARAMS_RC_CONSTQP：常量量化参数模式，即使用固定的量化参数对视频进行编码。
	//	NV_ENC_PARAMS_RC_VBR：可变比特率模式，即根据视频内容动态地调整比特率以控制视频质量和大小。
	//	NV_ENC_PARAMS_RC_CBR：恒定比特率模式，即在保证一定质量下控制视频大小，可以更好地控制视频码率，适用于对码率有比较严格要求的场景。
	//	NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ：低延迟 CBR 模式，高质量。
	//	NV_ENC_PARAMS_RC_CBR_HQ：CBR 模式，高质量。
	//	NV_ENC_PARAMS_RC_VBR_HQ：VBR 模式，高质量。

	//encodeCLIOptions.SetInitParams(&initializeParams, nvBufferFormat);
	enc->CreateEncoder(&initializeParams);

	//把cuda编码通道号加入管理 
	pCudaChanManager->AddChanToManager(nGpu,m_CudaChan);

#ifdef WriteMp4File_Flag 
	char szFileName[256] = { 0 };
	sprintf(szFileName, "D:\\CudaEncode_%X-%d.264", this, m_CudaChan);
	fWriteMp4 = fopen(szFileName, "wb");
	bIFrameAttachFlag = false ;

	//sprintf(szFileName, "D:\\CudaSize_%X-%d.txt", this, m_CudaChan);
	//fPacketSizeFile = fopen(szFileName, "wb");
#endif

	bInitFlag = true;
}

CCudaVideoEncode::~CCudaVideoEncode()
{
	std::lock_guard<std::mutex> lock(cudaVideoEncodeLock);


	if (enc)
	{
		delete enc;
		enc = NULL;
	}

	if (cuContext)
		cuCtxDestroy(cuContext);

	//移除cuda通道 
	pCudaChanManager->DeleteChanFromManager(m_CudaChan);

#ifdef WriteMp4File_Flag 
 	fclose(fWriteMp4);
	//fclose(fPacketSizeFile);
#endif
 //  malloc_trim(0);
}

bool  CCudaVideoEncode::CheckVideoIsIFrame(int nVideoFrameType, unsigned char* szPVideoData, int nPVideoLength)
{
	int            nPos = 0;
	bool           bVideoIsIFrameFlag = false;
	unsigned char  nFrameType = 0x00;
	unsigned char  szVideoFrameHead[4] = {0x00,0x00,0x00,0x01};

	for (int i = 0; i < nPVideoLength; i++)
	{
		if (memcmp(szPVideoData + i, szVideoFrameHead, 4) == 0)
		{//找到帧片段
			if (nVideoFrameType == (int)cudaEncodeVideo_H264)
			{
				nFrameType = (szPVideoData[i + 4] & 0x1F);
				if (nFrameType == 7 || nFrameType == 8 || nFrameType == 5)
				{//SPS   PPS   IDR 
					bVideoIsIFrameFlag = true;
					break;
				}
			}
			else if (nVideoFrameType == (int)cudaEncodeVideo_HEVC)
			{
				nFrameType = (szPVideoData[i + 4] & 0x7E) >> 1;
				if ((nFrameType >= 16 && nFrameType <= 21) || (nFrameType >= 32 && nFrameType <= 34))
				{//SPS   PPS   IDR 
					bVideoIsIFrameFlag = true;
					break;
				}
			}
			else
				break;
		}
	}

	return bVideoIsIFrameFlag;
}

//视频编码 
int  CCudaVideoEncode::cudaEncodeVideo(unsigned char* pYUVData, int nLength,char* pEncodecData)
{
	std::lock_guard<std::mutex> lock(cudaVideoEncodeLock);
	if (!bInitFlag)
		return 0 ;

	//H264编码 -----------------------------------------------------------
	const NvEncInputFrame* encoderInputFrame = enc->GetNextInputFrame();
	if (encoderInputFrame == NULL)
	{
		printf("encoderInputFrame  ==  null \r\n");
		return 0;

	}
	

	NvEncoderCuda::CopyToDeviceFrame(cuContext, pYUVData, 0, (CUdeviceptr)encoderInputFrame->inputPtr,
		(int)encoderInputFrame->pitch,
		enc->GetEncodeWidth(),
		enc->GetEncodeHeight(),
		CU_MEMORYTYPE_HOST,
		encoderInputFrame->bufferFormat,
		encoderInputFrame->chromaOffsets,
		encoderInputFrame->numChromaPlanes);

	enc->EncodeFrame(vPacket);
	//-----------------------------------------------------------

	nSize = 0;
	nEncodeLength = 0;
	for (std::vector<uint8_t> &packet : vPacket)
	{
		nSize ++;
	}

	if(nSize > 0 )
	{
 		for (std::vector<uint8_t> &packet : vPacket)
		{
		  //拷贝spsp,pps 
		  CopySpsPpsBuffer((unsigned char*) packet.data(),packet.size()) ;
		  
		  if(memcmp((unsigned char*) packet.data(),idrFrame,5) == 0)
		  {
 		    nEncodeLength = packet.size() + nSpsPpsBufferLength;
			if(nSpsPpsBufferLength > 0)
			{
			  memcpy(pEncodecData,pSpsPpsBuffer,nSpsPpsBufferLength);
		      memcpy(pEncodecData+nSpsPpsBufferLength, reinterpret_cast<char*>(packet.data()), packet.size());
			}
		    return nEncodeLength;
		  }else
		  {
 		    nEncodeLength = packet.size();
		    memcpy(pEncodecData, reinterpret_cast<char*>(packet.data()), nEncodeLength);
		    return nEncodeLength;
		  }
		}
	}
	else
	{
		printf("CCudaVideoEncode::cudaEncodeVideo  nSize  == 0 \r\n");
		return 0 ;
	}

#ifdef WriteMp4File_Flag
	int nSize = 0;
 	for (std::vector<uint8_t> &packet : vPacket)
	{
 		if (!bIFrameAttachFlag)
		{//检测I帧是否到达
			if (CheckVideoIsIFrame(m_videoCodec, packet.data(), packet.size()))
				bIFrameAttachFlag = true;
		}

		if (bIFrameAttachFlag)
		{//I帧到达才开始写
			fwrite(reinterpret_cast<char*>(packet.data()), 1, packet.size(), fWriteMp4);
		}

		nSize ++;
	}

	//fprintf(fPacketSizeFile, "size() = %d\r\n", nSize);
	//fflush(fPacketSizeFile);
#endif
}

bool  CCudaVideoEncode::CopySpsPpsBuffer(unsigned char* pH264Data,int nLength)
{
	if(bCopySpsPpsSuccessFlag)
		return false ;
	
	if(pH264Data == NULL || nLength <= 4)
		return false;
			
	if(memcmp(pH264Data,spsFrame,5) != 0)
		return false ;
	
	int nIdrPos = 0;
	for(int i=0;i<nLength;i++)
	{
		if(memcmp(pH264Data+i,idrFrame,5) == 0)
		{
			nIdrPos = i ;
			break ;
		}
	}
	
	if(nIdrPos > 0 && nIdrPos <= 4096 )
	{
		bCopySpsPpsSuccessFlag = true ;
		memcpy(pSpsPpsBuffer,pH264Data,nIdrPos);
		nSpsPpsBufferLength = nIdrPos ;
	}
	 else 
		 return false  ;
}