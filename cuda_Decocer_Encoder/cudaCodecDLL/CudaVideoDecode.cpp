/*
  功能:
     实现调用cuda 进行硬件解码视频 
*/

#include "stdafx.h"
#include "CudaVideoDecode.h"

extern CCudaChanManager*   pCudaChanManager ;

CCudaVideoDecode::CCudaVideoDecode(cudaCodecVideo_enum videoCodec, cudaCodecVideo_enum outYUVType, int nWidth, int nHeight, uint64_t nCudaChan)
{
	dec = NULL ;
	converter8 = NULL;
	converter16 = NULL ;

	m_videoCodec = videoCodec;
	m_nWidth     = nWidth ;
	m_nHeight    = nHeight;
	m_CudaChan   = nCudaChan;
	m_outYUVType = outYUVType;//YUV输出格式 
	int          nGpu = 0; 
	cuDevice     =  0;
	cuContext    = NULL;
 
	nGpu = pCudaChanManager->GetCudaGPUOrder();

	memset((char*)&cropRect, 0x00, sizeof(cropRect));
	memset((char*)&resizeDim, 0x00, sizeof(resizeDim));

	if (cuDeviceGet(&cuDevice, nGpu) != CUDA_SUCCESS)
		return;

 	if (cuCtxCreate(&cuContext, 0, cuDevice) != CUDA_SUCCESS)
		return;

	converter8 = new   YuvConverter<uint8_t> (nWidth, nHeight);
	converter16 = new  YuvConverter<uint16_t> (nWidth, nHeight);
	ppFrame = NULL;

#ifdef WritePicthFlag
   fWritePitch = fopen("./pitch.txt","wb");
   nWriteCount = 0;
#endif

#ifdef WriteYUVFile_Flag
	nWriteFrameCount = 0;
	fWriteYUV = fopen("/home/ABLMediaServer/bin/out-2022-08-11.yuv", "wb");
#endif

#ifdef LibYUVScaleYUVFlag
	 nScaleWidth = 720 ;
	 nScaleHeight = 480 ;

	 pScaleYUVData = new unsigned char[(nScaleWidth * nScaleHeight * 3) / 2];
	 nWriteFrameScaleCount = 0 ;
	 fWriteYUVScale = fopen("D:\\Scale-2021-02-03.yuv", "wb");
#endif

	if (pCudaChanManager)
	  pCudaChanManager->AddChanToManager(nGpu, m_CudaChan);

	dec = new NvDecoder(cuContext, false, (cudaVideoCodec)videoCodec, false, false, &cropRect, &resizeDim);
}

CCudaVideoDecode::~CCudaVideoDecode()
{
	std::lock_guard<std::mutex> lock(cudaVideoDecodeLock);
	
	if (dec)
	{
		delete dec;
		dec = NULL;
	}

	if (converter8 != NULL)
	{
		delete converter8;
		converter8 = NULL;
	}
	if (converter16 != NULL)
	{
		delete converter16;
		converter16 = NULL;
	}

	if(cuContext)
	  cuCtxDestroy(cuContext);

	if(pCudaChanManager)
		pCudaChanManager->DeleteChanFromManager(m_CudaChan) ;

#ifdef WriteYUVFile_Flag
	if (fWriteYUV)
	{
		fclose(fWriteYUV);
		fWriteYUV = NULL;
	}
#endif
#ifdef LibYUVScaleYUVFlag
	if (fWriteYUVScale)
	{
		fclose(fWriteYUVScale);
		fWriteYUVScale = NULL;
	}
	if (pScaleYUVData)
	{
		delete[] pScaleYUVData;
		pScaleYUVData = NULL;
	}
#endif

#ifdef WritePicthFlag
   if(fWritePitch)
	 fclose(fWritePitch);
#endif

#ifndef _WIN32
   malloc_trim(0);
#endif // _WIN32
}

void CCudaVideoDecode::ConvertSemiplanarToPlanar(uint8_t *pHostFrame, int nWidth, int nHeight, int nBitDepth) 
{
	if (nBitDepth == 8) {
		// nv12->iyuv
 		converter8->UVInterleavedToPlanar(pHostFrame);
	}
	else {
		// p016->yuv420p16
 		converter16->UVInterleavedToPlanar((uint16_t *)pHostFrame);
	}
}

unsigned char* CCudaVideoDecode::CudaVideoDecode(unsigned char* pVideo, int nLength,int& nFrameReturnCount, int& nOutDecodeLength)
{
	std::lock_guard<std::mutex> lock(cudaVideoDecodeLock);

	nOutDecodeLength = 0;
	nFrameReturnCount = 0;
	if (dec == NULL || nLength <= 0 )
		return NULL;

 	nFrameReturned = dec->Decode(pVideo, nLength);
	if (nFrameReturned > 0 )
	{
 		bDecodeOutSemiPlanar = (dec->GetOutputFormat() == cudaVideoSurfaceFormat_NV12) || (dec->GetOutputFormat() == cudaVideoSurfaceFormat_P016);
		
 		for (int i = 0; i < nFrameReturned; i++)
		{
			ppFrame = dec->GetFrame();
			
			if (m_outYUVType == cudaCodecVideo_YV12 && bDecodeOutSemiPlanar)
			{//当输出 cudaCodecVideo_YV12 才转换，默认NV12 时不转换 
				ConvertSemiplanarToPlanar(ppFrame, dec->GetWidth(), dec->GetHeight(), dec->GetBitDepth());
			}

#ifdef LibYUVScaleYUVFlag
			src_width_uv = (Abs(dec->GetWidth()) + 1) >> 1;
			src_height_uv = (Abs(dec->GetHeight()) + 1) >> 1;

			src_stride_y = Abs(dec->GetWidth());
			src_stride_uv = src_width_uv * 2;

			dst_width_uv = (Abs(nScaleWidth) + 1) >> 1;
			dst_height_uv = (Abs(nScaleHeight) + 1) >> 1;

			dst_stride_y = Abs(nScaleWidth);
			dst_stride_uv = dst_width_uv * 2;
 
			nStartTime = ::GetTickCount64();
			nRet = libyuv::NV12Scale((uint8_t*)ppFrame[i], Abs(dec->GetWidth()), (uint8_t*)ppFrame[i] + (dec->GetWidth()*dec->GetHeight()), src_stride_uv, dec->GetWidth(), dec->GetHeight(), pScaleYUVData, nScaleWidth, pScaleYUVData + (nScaleWidth*nScaleHeight), dst_stride_uv, nScaleWidth, nScaleHeight, (libyuv::FilterMode)0);
			nEndTime = ::GetTickCount64();
			if (nWriteFrameScaleCount <= 100 && nRet == 0 && fWriteYUVScale && ppFrame[i] != NULL && dec->GetFrameSize() > 0)
			{
				//sprintf(szTimeBuffer, "Scale, Width %d ,Height %d , Count Time %llu \r\n", nScaleWidth, nScaleHeight, nEndTime - nStartTime);
				//fwrite(szTimeBuffer, 1, strlen(szTimeBuffer), fWriteYUVScale);
				//fflush(fWriteYUVScale);

				fwrite(pScaleYUVData, 1, (nScaleWidth*nScaleHeight*3)/2, fWriteYUVScale);
 				fflush(fWriteYUVScale);
			}
			nWriteFrameScaleCount ++;
#endif

#ifdef WriteYUVFile_Flag
			if (nWriteFrameCount <= 30 && fWriteYUV && ppFrame != NULL && dec->GetFrameSize() > 0)
			{
				fwrite(ppFrame, 1, dec->GetFrameSize(), fWriteYUV);
				//fprintf(fWriteYUV,"nFrameReturned = %d \r\n ", nFrameReturned);
				fflush(fWriteYUV);
			}
			nWriteFrameCount++;
#endif
		}
		
#ifdef WritePicthFlag
    if(nWriteCount <= 100)
	{
     fprintf(fWritePitch,"pitch=%d\r\n",dec->GetDeviceFramePitch());
 	 fflush(fWritePitch);
	}
	nWriteCount ++ ;
#endif

		nFrameReturnCount = nFrameReturned; 
		nOutDecodeLength = dec->GetFrameSize();
		return ppFrame;
	}
	else
	{
		nFrameReturnCount = 0;
		nOutDecodeLength = 0 ; 
		return NULL;
	}
}