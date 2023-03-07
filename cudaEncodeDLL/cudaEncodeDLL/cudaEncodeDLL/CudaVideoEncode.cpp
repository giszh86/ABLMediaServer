/*
  功能:
     实现调用cuda 进行硬件视频编码  
*/

#include "stdafx.h"
#include "CudaVideoEncode.h"

extern CCudaChanManager*     pCudaChanManager ;

CCudaVideoEncode::CCudaVideoEncode(cudaEncodeVideo_enum videoCodec, cudaEncodeVideo_enum yuvType, int nWidth, int nHeight, uint64_t nCudaChan)
{
	bInitFlag = false;

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
	if (enc == false)
		return;

	initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
	encodeConfig = { NV_ENC_CONFIG_VER };

	initializeParams.encodeConfig = &encodeConfig;
	if (videoCodec == cudaEncodeVideo_H264)
	   enc->CreateDefaultEncoderParams(&initializeParams, NV_ENC_CODEC_H264_GUID, NV_ENC_PRESET_DEFAULT_GUID); //NV_ENC_CODEC_HEVC_GUID
	else if (videoCodec == cudaEncodeVideo_HEVC)
	   enc->CreateDefaultEncoderParams(&initializeParams, NV_ENC_CODEC_HEVC_GUID, NV_ENC_PRESET_DEFAULT_GUID); 

	encodeCLIOptions.SetInitParams(&initializeParams, nvBufferFormat);
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
		return 0;

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
		   nEncodeLength = packet.size();
		   memcpy(pEncodecData, reinterpret_cast<char*>(packet.data()), nEncodeLength);
		   return nEncodeLength;
		}
	}
	else
	{
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
