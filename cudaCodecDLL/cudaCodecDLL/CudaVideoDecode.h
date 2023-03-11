#ifndef _CUDADecode_H
#define _CUDADecode_H

#include "cudaCodecDLL.h"
//#define WritePicthFlag   1

class CCudaVideoDecode
{
public:
	CCudaVideoDecode(cudaCodecVideo_enum videoCodec, cudaCodecVideo_enum outYUVType, int nWidth, int nHeight, uint64_t nCudaChan);
    ~CCudaVideoDecode();

	cudaCodecVideo_enum m_outYUVType;
#ifdef WritePicthFlag
   FILE* fWritePitch;
   int   nWriteCount ;
#endif

#ifdef WriteYUVFile_Flag
	FILE*    fWriteYUV;
	int      nWriteFrameCount;
#endif

#ifdef LibYUVScaleYUVFlag
	FILE*    fWriteYUVScale;
	int      nWriteFrameScaleCount;
	unsigned char* pScaleYUVData;
	int      nScaleWidth;
	int      nScaleHeight;
	int nRet;

	int src_width_uv  ;
	int src_height_uv  ;
 	int src_stride_y  ;
	int src_stride_uv  ;
 	int dst_width_uv  ;
	int dst_height_uv  ;
 	int dst_stride_y  ;
	int dst_stride_uv  ;
	int64_t nStartTime;
	int64_t nEndTime;
	char    szTimeBuffer[256];
 #endif

	YuvConverter<uint8_t>*  converter8;
	YuvConverter<uint16_t>* converter16;
	unsigned char* CudaVideoDecode(unsigned char* pVideo, int nLength, int& nFrameReturnCount, int& nOutDecodeLength);
	void           ConvertSemiplanarToPlanar(uint8_t *pHostFrame, int nWidth, int nHeight, int nBitDepth);
public:
	uint64_t             m_CudaChan;
	uint8_t*             ppFrame;
private:
	std::mutex          cudaVideoDecodeLock;

	cudaCodecVideo_enum m_videoCodec;
	int                 m_nWidth;
	int                 m_nHeight;

	NvDecoder* dec;
	CUdevice   cuDevice ;
	int        iGpu  ;
	Rect       cropRect  ;
	Dim        resizeDim ;
	CUcontext  cuContext;
	bool       bDecodeOutSemiPlanar;
	int        nFrameReturned;
};

#endif