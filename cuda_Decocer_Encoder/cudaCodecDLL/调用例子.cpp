一、声明导出函数指针变量 
	HINSTANCE            hCudaDecodeInstance;
	ABL_cudaDecode_Init  cudaEncode_Init = NULL;
	ABL_cudaDecode_GetDeviceGetCount cudaEncode_GetDeviceGetCount = NULL;
	ABL_cudaDecode_GetDeviceName cudaEncode_GetDeviceName = NULL;
	ABL_cudaDecode_GetDeviceUse cudaDecode_GetDeviceUse = NULL;
	ABL_CreateVideoDecode cudaCreateVideoDecode = NULL;
	ABL_CudaVideoDecode cudaVideoDecode = NULL;
	ABL_DeleteVideoDecode cudaDeleteVideoDecode = NULL;
	ABL_GetCudaDecodeCount getCudaDecodeCount = NULL ;
	ABL_VideoDecodeUnInit cudaVideoDecodeUnInit = NULL;
	
	bool  ABL_bCudaFlag = false  ;//标识电脑是否有cuda解码功能 
    int   ABL_nCudaCount = 0;     //记录电脑有几块英伟达显卡
	
二、从解码库获取到函数指针 
	hCudaDecodeInstance = ::LoadLibrary("cudaCodecDLL.dll");
	if (hCudaDecodeInstance != NULL)
	{
		cudaEncode_Init = (ABL_cudaDecode_Init)::GetProcAddress(hCudaDecodeInstance, "cudaCodec_Init");
		cudaEncode_GetDeviceGetCount = (ABL_cudaDecode_GetDeviceGetCount) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_GetDeviceGetCount");
		cudaEncode_GetDeviceName = (ABL_cudaDecode_GetDeviceName) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_GetDeviceName");
		cudaDecode_GetDeviceUse = (ABL_cudaDecode_GetDeviceUse) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_GetDeviceUse");
		cudaCreateVideoDecode = (ABL_CreateVideoDecode) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_CreateVideoDecode");
		cudaVideoDecode = (ABL_CudaVideoDecode) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_CudaVideoDecode");

		cudaDeleteVideoDecode = (ABL_DeleteVideoDecode) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_DeleteVideoDecode");
		getCudaDecodeCount = (ABL_GetCudaDecodeCount) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_GetCudaDecodeCount");
		cudaVideoDecodeUnInit = (ABL_VideoDecodeUnInit) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_UnInit");

	}
	if (cudaEncode_Init)
		ABL_bCudaFlag = cudaEncode_Init();
	if (cudaEncode_GetDeviceGetCount)
		ABL_nCudaCount = cudaEncode_GetDeviceGetCount();

三、 创建cuda解码句柄 （每个通道 nCudaDecodeChan 只创建一次 ）

		if (strcmp(szVideoCodec, "H264") == 0)
			cudaCreateVideoDecode(cudaCodecVideo_H264, cudaCodecVideo_YV12, m_mediaCodecInfo.nWidth, m_mediaCodecInfo.nHeight, nCudaDecodeChan);
		else if (strcmp(szVideoCodec, "H265") == 0)
			cudaCreateVideoDecode(cudaCodecVideo_HEVC, cudaCodecVideo_YV12, m_mediaCodecInfo.nWidth, m_mediaCodecInfo.nHeight, nCudaDecodeChan);

四、 进行cuda硬解码 
	  pCudaDecodeYUVFrame = cudaVideoDecode(nCudaDecodeChan, szVideo, nLength, nCudaDecodeFrameCount, nCudeDecodeOutLength);
	  if (nCudeDecodeOutLength > 0)
	  {
		//这里只是把解码出来的YUV数据进行缩放，然后再编码 ，别的应用比如显示YUV数据、对YUV数据进行保存、YUV数据进行画线、画框、切割YUV数据都可以 
	    for (int i = 0; i < nCudaDecodeFrameCount; i++)
	    {
		  avFrameSWS.AVFrameSWSYUV(pCudaDecodeYUVFrame[i], nCudeDecodeOutLength);  //其中  pCudaDecodeYUVFrame[i] 就是解码出来的YUV数据 ，nCudeDecodeOutLength 为长度
		  return videoEncode.EncodecYUV(avFrameSWS.szDestData, avFrameSWS.numBytes2, pOutEncodeBuffer, &nOutLength);   
	    }
	 }  
	 
五、关闭某一个路的cuda解码句柄 nCudaDecodeChan（确保不再解码时才调用，并不是解码一帧关闭一次 ）

    cudaDeleteVideoDecode(nCudaDecodeChan);



	 
	 
			
	 
	