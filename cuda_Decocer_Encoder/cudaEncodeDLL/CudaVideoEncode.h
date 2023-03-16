#ifndef _CUDAEncode_H
#define _CUDAEncode_H

#include "cudaEncodeDLL.h"

#include <cuda.h>
#include "NvEncoder/NvEncoderCuda.h"
//#include "../Utils/Logger.h"
//#include "../Utils/NvEncoderCLIOptions.h"

//#define    WriteMp4File_Flag    1

class CCudaVideoEncode
{
public:
	CCudaVideoEncode(cudaEncodeVideo_enum videoCodec, cudaEncodeVideo_enum yuvType, int nWidth, int nHeight, uint64_t nCudaChan);
    ~CCudaVideoEncode();

	int nSize ;
	int nEncodeLength ;
    bool                 bCopySpsPpsSuccessFlag ;
	unsigned char        pSpsPpsBuffer[4096] ;
	int                  nSpsPpsBufferLength ;
 
    bool                 CopySpsPpsBuffer(unsigned char* pH264Data,int nLength);
#ifdef WriteMp4File_Flag 
	//FILE*                fPacketSizeFile;
	FILE*                fWriteMp4;
	bool                 bIFrameAttachFlag;
#endif

	bool                 CheckVideoIsIFrame(int nVideoFrameType, unsigned char* szPVideoData, int nPVideoLength);
	int                  cudaEncodeVideo(unsigned char* pYUVData, int nLength, char* pEncodecData);

	NV_ENC_BUFFER_FORMAT nvBufferFormat;
	int                  m_yuvType;
	bool                 bInitFlag;
	int64_t              m_CudaChan;
	int                  m_nWidth;
	int                  m_nHeight;
	std::mutex           cudaVideoEncodeLock;

	cudaEncodeVideo_enum m_videoCodec;
	CUdevice            cuDevice;
	int                 iGpu  ;
 	CUcontext           cuContext;

	//NvEncoderInitParam  encodeCLIOptions;
	NvEncoderCuda*      enc;

	NV_ENC_INITIALIZE_PARAMS initializeParams ;
	NV_ENC_CONFIG            encodeConfig ;
	std::vector<std::vector<uint8_t>> vPacket;

#ifdef WriteYUVFile_Flag
	FILE*    fWriteMP4;
	int      nWriteFrameCount;
#endif
 
};

#endif