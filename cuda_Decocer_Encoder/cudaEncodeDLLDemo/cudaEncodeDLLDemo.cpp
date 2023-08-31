/*
功能： cuda 英伟达显卡硬件编码调用例子 
*/

#include "stdafx.h"

//通过注释其中3行来设定YUV的宽、高
#define  CurrentYUVSize_1920X1080  
//#define  CurrentYUVSize_704X576  
//#define  CurrentYUVSize_640X480  
//#define  CurrentYUVSize_352X288  

#ifdef CurrentYUVSize_1920X1080 
	#define YUV_Width  1920 
	#define YUV_Height 1080 
#endif 
#ifdef CurrentYUVSize_704X576
#define YUV_Width  704 
#define YUV_Height 576 
#endif 
#ifdef CurrentYUVSize_640X480
#define YUV_Width  640 
#define YUV_Height 480 
#endif 
#ifdef CurrentYUVSize_352X288
#define YUV_Width  352 
#define YUV_Height 288 
#endif 

char szYUVName[256] = { 0 };
unsigned char* szYUVBuffer = NULL ;

uint64_t       nCudaEncode;//cuda硬件编码句柄

int            nSize = (YUV_Width*YUV_Height * 3) / 2;//源尺寸一帧YUV大小
int            nEncodeLength;
unsigned char* szEncodeBuffer = NULL;
FILE*          fFileH264;//编码后，写入264文件句柄
char           szCurrentPath[256] = { 0 };//当前路径

#ifdef _WIN32
bool GetCurrentPath(char* szCurPath)
{
	char    szPath[255] = { 0 };
	string  strTemp;
	int     nPos;

	GetModuleFileNameA(NULL, szPath, sizeof(szPath));
	strTemp = szPath;

	nPos = strTemp.rfind("\\", strlen(szPath));
	if (nPos >= 0)
	{
		memcpy(szCurPath, szPath, nPos + 1);
		return true;
	}
	else
		return false;
}
//cuda 编码 
HINSTANCE             hInstance;
ABL_cudaEncode_Init  cudaEncode_Init = NULL;
ABL_cudaEncode_GetDeviceGetCount cudaEncode_GetDeviceGetCount = NULL;
ABL_cudaEncode_GetDeviceName cudaEncode_GetDeviceName = NULL;
ABL_cudaEncode_CreateVideoEncode cudaEncode_CreateVideoEncode = NULL;
ABL_cudaEncode_DeleteVideoEncode cudaEncode_DeleteVideoEncode = NULL;
ABL_cudaEncode_CudaVideoEncode cudaEncode_CudaVideoEncode = NULL;
ABL_cudaEncode_UnInit cudaEncode_UnInit = NULL;


HINSTANCE             hCudaDecodeInstance;
ABL_cudaCodec_Init cudaCodec_Init = NULL;
ABL_cudaCodec_GetDeviceGetCount  cudaCodec_GetDeviceGetCount = NULL;
ABL_cudaCodec_GetDeviceName cudaCodec_GetDeviceName = NULL;
ABL_cudaCodec_GetDeviceUse cudaCodec_GetDeviceUse = NULL;
ABL_cudaCodec_CreateVideoDecode cudaCodec_CreateVideoDecode = NULL;
ABL_cudaCodec_CudaVideoDecode cudaCodec_CudaVideoDecode = NULL;
ABL_cudaCodec_DeleteVideoDecode cudaCodec_DeleteVideoDecode = NULL;
ABL_cudaCodec_GetCudaDecodeCount cudaCodec_GetCudaDecodeCount = NULL;
ABL_cudaCodec_UnInit cudaCodec_UnInit = NULL;

#else

bool GetCurrentPath(char* szCurPath)
{
	strcpy(szCurPath, get_current_dir_name());	
   return true;
}

//cuda 编码 
ABL_cudaEncode_Init  cudaEncode_Init = NULL;
ABL_cudaEncode_GetDeviceGetCount cudaEncode_GetDeviceGetCount = NULL;
ABL_cudaEncode_GetDeviceName cudaEncode_GetDeviceName = NULL;
ABL_cudaEncode_CreateVideoEncode cudaEncode_CreateVideoEncode = NULL;
ABL_cudaEncode_DeleteVideoEncode cudaEncode_DeleteVideoEncode = NULL;
ABL_cudaEncode_CudaVideoEncode cudaEncode_CudaVideoEncode = NULL;
ABL_cudaEncode_UnInit cudaEncode_UnInit = NULL;


ABL_cudaCodec_Init cudaCodec_Init = NULL;
ABL_cudaCodec_GetDeviceGetCount  cudaCodec_GetDeviceGetCount = NULL;
ABL_cudaCodec_GetDeviceName cudaCodec_GetDeviceName = NULL;
ABL_cudaCodec_GetDeviceUse cudaCodec_GetDeviceUse = NULL;
ABL_cudaCodec_CreateVideoDecode cudaCodec_CreateVideoDecode = NULL;
ABL_cudaCodec_CudaVideoDecode cudaCodec_CudaVideoDecode = NULL;
ABL_cudaCodec_DeleteVideoDecode cudaCodec_DeleteVideoDecode = NULL;
ABL_cudaCodec_GetCudaDecodeCount cudaCodec_GetCudaDecodeCount = NULL;
ABL_cudaCodec_UnInit cudaCodec_UnInit = NULL;

#endif




//#include <thread>
//#include "../jetson-ffmpeg/include/nvmpi.h"
//
//int test()
//{
//	char szOutH264Name[256] = { 0 };
//
//	GetCurrentPath(szCurrentPath);
//
//	if (YUV_Width == 1920)
//	{
//		sprintf(szYUVName, "%s/YV12_1920x1080.yuv", szCurrentPath);
//		sprintf(szOutH264Name, "%s/Encodec_1920x1080.264", szCurrentPath);
//	}
//	else if (YUV_Width == 704)
//	{
//		sprintf(szYUVName, "%s/YV12_704x576.yuv", szCurrentPath);
//		sprintf(szOutH264Name, "%s/Encodec_704x576.264", szCurrentPath);
//	}
//	else if (YUV_Width == 640)
//	{
//		sprintf(szYUVName, "%s/YV12_640x480.yuv", szCurrentPath);
//		sprintf(szOutH264Name, "%s/Encodec_640x480.264", szCurrentPath);
//	}
//	else if (YUV_Width == 352)
//	{
//		sprintf(szYUVName, "%s/YV12_352x288.yuv", szCurrentPath);
//		sprintf(szOutH264Name, "%s/Encodec_352x288.264", szCurrentPath);
//	}
//	else //非法的YUV
//		return -1;
//	printf("open file [%s]  \r\n", szYUVName);
//	//打开原始的YUV文件
//	FILE* fFile;//输入的YUV文件
//	fFile = fopen(szYUVName, "rb");
//	if (fFile == NULL)
//	{
//		printf("open fail errno = % d reason = % s \n", errno);
//		printf("open file error =[%s] \r\n", szYUVName);
//		return 0;
//	}
//
//	printf("open file [%s]  \r\n", szOutH264Name);
//	//创建准备写入264编码数据的文件句柄
//	fFileH264 = fopen(szOutH264Name, "wb");
//	if (fFileH264 == NULL)
//	{
//		printf("open file error =[%s] \r\n", szOutH264Name);
//		return 0;
//	}
//
//	nvEncParam param = { 0 };
//	param.width = YUV_Width;
//	param.height = YUV_Height;	
//	param.mode_vbr = 0;
//	param.idr_interval = 60;
//	param.iframe_interval = 30;
//	param.fps_n = 1;
//	param.fps_d = 25;
//	param.profile = 77;
//	param.level = 10;
//	param.hw_preset_type = 1;
//
//	nvmpictx* ctx = nvmpi_create_encoder(NV_VIDEO_CodingH264, &param);
//	if (ctx == nullptr)
//	{
//		printf("cudaEncode_CreateVideoEncode  fail \r\n");
//	}
//	else
//	{
//		printf("cudaEncode_CreateVideoEncode  \r\n" );
//	}
//	szYUVBuffer = new unsigned char[(YUV_Width * YUV_Height * 3) / 2];
//	szEncodeBuffer = new unsigned char[(YUV_Width * YUV_Height * 3) / 2];
//	while (true)
//	{
//		//每次读取一帧的YUV数据，长度为 nSize =  (YUV_Width*YUV_Height * 3) / 2 
//		int nRead = fread(szYUVBuffer, 1, nSize, fFile);
//		if (nRead <= 0)
//		{
//			printf("open file nRead =[%d] \r\n", nRead);
//			break;
//		}
//		// 假设您已经读取了YUV数据到 szYUVBuffer 中
//		unsigned char* yuvData = szYUVBuffer; // 指向YUV数据的指针
//		int yuvWidth = YUV_Width;
//		int yuvHeight = YUV_Height;
//		nvFrame *frame=new nvFrame;
//
//		// 设置 nvFrame 的字段
//		frame->flags = 0; // 根据需要设置
//		frame->payload_size[0] = yuvWidth * yuvHeight; // Y分量的大小
//		frame->payload_size[1] = yuvWidth * yuvHeight / 4; // U分量的大小
//		frame->payload_size[2] = yuvWidth * yuvHeight / 4; // V分量的大小
//		frame->payload[0] = yuvData; // Y分量数据指针
//		frame->payload[1] = yuvData + frame->payload_size[0]; // U分量数据指针
//		frame->payload[2] = yuvData + frame->payload_size[0] + frame->payload_size[1]; // V分量数据指针
//		frame->linesize[0] = yuvWidth; // Y分量每行的大小
//		frame->linesize[1] = yuvWidth / 2; // U分量每行的大小
//		frame->linesize[2] = yuvWidth / 2; // V分量每行的大小
//		frame->type = nvPixFormat::NV_PIX_YUV420;
//		frame->width = yuvWidth;
//		frame->height = yuvHeight;
//		frame->timestamp = time(nullptr); // 设置时间戳
//		int num1= nvmpi_encoder_put_frame(ctx, frame);
//		if (num1>0)
//		{
//			printf("nvmpi_encoder_put_frame =[%d] \r\n", num1);
//		}
//		else
//		{
//			printf("nvmpi_encoder_put_frame =[%d] \r\n", num1);
//		}
//		nvPacket *packet;
//		int num2 = nvmpi_encoder_get_packet(ctx, &packet);
//		if (num2 > 0)
//		{
//			printf("nvmpi_encoder_get_packet =[%d] \r\n", num2);
//		}
//		else
//		{
//			printf("nvmpi_encoder_get_packet =[%d] \r\n", num2);
//		}
//
//		if (packet&&packet->payload_size>0)
//		{
//			//如果编码成功 nEncodeLength 大于 0 ，写入编码文件中  fFileH264
//			fwrite(packet->payload, 1, packet->payload_size, fFileH264);
//			fflush(fFileH264);
//			printf("nEncodeLength %d  \r\n", packet->payload_size);
//		}
//		else
//		{
//			printf("nEncodeLength %d  \r\n", packet->payload_size);
//		}
//	}
//
//	if (szYUVBuffer)
//		delete[] szYUVBuffer;
//	if (szEncodeBuffer)
//		delete szEncodeBuffer;
//
//	fclose(fFile);
//	fclose(fFileH264);
//
//	//编码结束后，删除编码句柄 nCudaEncode
//	nvmpi_encoder_close(ctx);
//	return 0;
//
//}
int main()
{
	//test();

	char szCudaName[256] = { 0 };
	char szOutH264Name[256] = { 0 };
	bool bInitCudaFlag = false;
	int  nCudaCount = 0;
#ifdef _WIN32
	//采用动态加载的方式，因为有些电脑有英伟达显卡，有些电脑没有英伟达显卡 
	hInstance = ::LoadLibrary(TEXT("cudaEncodeDLL.dll"));
	if (hInstance != NULL)
	{
		cudaEncode_Init = (ABL_cudaEncode_Init) ::GetProcAddress(hInstance, "cudaEncode_Init");
		cudaEncode_GetDeviceGetCount = (ABL_cudaEncode_GetDeviceGetCount) ::GetProcAddress(hInstance, "cudaEncode_GetDeviceGetCount");
		cudaEncode_GetDeviceName = (ABL_cudaEncode_GetDeviceName) ::GetProcAddress(hInstance, "cudaEncode_GetDeviceName");
		cudaEncode_CreateVideoEncode = (ABL_cudaEncode_CreateVideoEncode) ::GetProcAddress(hInstance, "cudaEncode_CreateVideoEncode");
		cudaEncode_DeleteVideoEncode = (ABL_cudaEncode_DeleteVideoEncode) ::GetProcAddress(hInstance, "cudaEncode_DeleteVideoEncode");
		cudaEncode_CudaVideoEncode = (ABL_cudaEncode_CudaVideoEncode) ::GetProcAddress(hInstance, "cudaEncode_CudaVideoEncode");
		cudaEncode_UnInit = (ABL_cudaEncode_UnInit) ::GetProcAddress(hInstance, "cudaEncode_UnInit");
	}
	hCudaDecodeInstance = ::LoadLibrary(LR"(cudaCodecDLL.dll)");
	if (hCudaDecodeInstance != NULL)
	{
		cudaCodec_Init = (ABL_cudaCodec_Init)::GetProcAddress(hCudaDecodeInstance, "cudaCodec_Init");
		cudaCodec_GetDeviceGetCount = (ABL_cudaCodec_GetDeviceGetCount) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_GetDeviceGetCount");
		cudaCodec_GetDeviceName = (ABL_cudaCodec_GetDeviceName) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_GetDeviceName");
		cudaCodec_GetDeviceUse = (ABL_cudaCodec_GetDeviceUse) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_GetDeviceUse");
		cudaCodec_CreateVideoDecode = (ABL_cudaCodec_CreateVideoDecode) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_CreateVideoDecode");
		cudaCodec_CudaVideoDecode = (ABL_cudaCodec_CudaVideoDecode) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_CudaVideoDecode");

		cudaCodec_DeleteVideoDecode = (ABL_cudaCodec_DeleteVideoDecode) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_DeleteVideoDecode");
		cudaCodec_GetCudaDecodeCount = (ABL_cudaCodec_GetCudaDecodeCount) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_GetCudaDecodeCount");
		cudaCodec_UnInit = (ABL_cudaCodec_UnInit) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_UnInit");

	}


#else
	void* pCudaDecodeHandle = NULL;
	pCudaDecodeHandle = dlopen("libcudaCodecDLL.so", RTLD_LAZY);
	if (pCudaDecodeHandle != NULL)
	{
		printf( " dlopen libcudaCodecDLL.so success , NVIDIA graphics card installed  ");

		cudaCodec_Init = (ABL_cudaCodec_Init)dlsym(pCudaDecodeHandle, "cudaCodec_Init");
		if (cudaCodec_Init != NULL)
			printf(" dlsym cudaCodec_Init success ");
		cudaCodec_GetDeviceGetCount = (ABL_cudaCodec_GetDeviceGetCount)dlsym(pCudaDecodeHandle, "cudaCodec_GetDeviceGetCount");
		cudaCodec_GetDeviceName = (ABL_cudaCodec_GetDeviceName)dlsym(pCudaDecodeHandle, "cudaCodec_GetDeviceName");
		cudaCodec_GetDeviceUse = (ABL_cudaCodec_GetDeviceUse)dlsym(pCudaDecodeHandle, "cudaCodec_GetDeviceUse");
		cudaCodec_CreateVideoDecode = (ABL_cudaCodec_CreateVideoDecode)dlsym(pCudaDecodeHandle, "cudaCodec_CreateVideoDecode");
		cudaCodec_CudaVideoDecode = (ABL_cudaCodec_CudaVideoDecode)dlsym(pCudaDecodeHandle, "cudaCodec_CudaVideoDecode");
		cudaCodec_DeleteVideoDecode = (ABL_cudaCodec_DeleteVideoDecode)dlsym(pCudaDecodeHandle, "cudaCodec_DeleteVideoDecode");
		cudaCodec_GetCudaDecodeCount = (ABL_cudaCodec_GetCudaDecodeCount)dlsym(pCudaDecodeHandle, "cudaCodec_GetCudaDecodeCount");
		cudaCodec_UnInit = (ABL_cudaCodec_UnInit)dlsym(pCudaDecodeHandle, "cudaCodec_UnInit");
}

	else
		printf( " dlopen libcudaCodecDLL.so failed , NVIDIA graphics card is not installed  ");


	void* pCudaEncodeHandle = NULL;
	pCudaEncodeHandle = dlopen("libcudaEncodeDLL.so", RTLD_LAZY);
	if (pCudaEncodeHandle != NULL)
	{
		printf(" dlopen libcudaEncodeDLL.so success , NVIDIA graphics card installed! \r\n ");
		cudaEncode_Init = (ABL_cudaEncode_Init)dlsym(pCudaEncodeHandle, "cudaEncode_Init");
		cudaEncode_GetDeviceGetCount = (ABL_cudaEncode_GetDeviceGetCount)dlsym(pCudaEncodeHandle, "cudaEncode_GetDeviceGetCount");
		cudaEncode_GetDeviceName = (ABL_cudaEncode_GetDeviceName)dlsym(pCudaEncodeHandle, "cudaEncode_GetDeviceName");
		cudaEncode_CreateVideoEncode = (ABL_cudaEncode_CreateVideoEncode)dlsym(pCudaEncodeHandle, "cudaEncode_CreateVideoEncode");
		cudaEncode_DeleteVideoEncode = (ABL_cudaEncode_DeleteVideoEncode)dlsym(pCudaEncodeHandle, "cudaEncode_DeleteVideoEncode");
		cudaEncode_CudaVideoEncode = (ABL_cudaEncode_CudaVideoEncode)dlsym(pCudaEncodeHandle, "cudaEncode_CudaVideoEncode");
		cudaEncode_UnInit = (ABL_cudaEncode_UnInit)dlsym(pCudaEncodeHandle, "cudaEncode_UnInit");
	}
#endif

	/*bool bCudaFlag=false;
	int ABL_nCudaCount=0;
	if (cudaCodec_Init)
		 bCudaFlag = cudaCodec_Init();
	if (cudaCodec_GetDeviceGetCount)
		 ABL_nCudaCount = cudaCodec_GetDeviceGetCount();

	if (bCudaFlag == false || ABL_nCudaCount == 0)
	{
		printf("error NVIDIA graphics card installed ! bCudaFlag=[%d]  ABL_nCudaCount=[%d]\r\n ", bCudaFlag, ABL_nCudaCount);
		return -1;
	}
	else
	{

		printf(" nCudaCount =[%d] ! \r\n", ABL_nCudaCount);
	}
	char szDeviceName[512];
	cudaCodec_GetDeviceName(0, szDeviceName);
	printf(" szDeviceName =[%s] ! \r\n", szDeviceName);
	uint64_t nCudaDecodeChan;
	bool bres= cudaCodec_CreateVideoDecode(cudaCodecVideo_H264, cudaCodecVideo_YV12, 1920, 1080, nCudaDecodeChan);

	printf(" nCudaDecodeChan =[%d]  bres=[%d]! \r\n", nCudaDecodeChan, bres);*/
	//初始化cuda编码库
	if(cudaEncode_Init)
		bInitCudaFlag = cudaEncode_Init();
	
	//获取cuda显卡个数
	if (cudaEncode_GetDeviceGetCount)
		nCudaCount = cudaEncode_GetDeviceGetCount();

	if (bInitCudaFlag == false || nCudaCount == 0)
	{
		printf("error NVIDIA graphics card installed ! nCudaCount=[%d]  bInitCudaFlag=[%d]\r\n ", nCudaCount , bInitCudaFlag);
		return -1;
	}
	else
	{

		printf(" nCudaCount =[%d] ! \r\n", nCudaCount);
	}

	//获取英伟达第一个显卡名称
	if(cudaEncode_GetDeviceName)
	  cudaEncode_GetDeviceName(0, szCudaName);
	int nRead;

	GetCurrentPath(szCurrentPath);

	if (YUV_Width == 1920)
	{
 		sprintf(szYUVName, "%s/YV12_1920x1080.yuv", szCurrentPath);
		sprintf(szOutH264Name, "%s/Encodec_1920x1080.264", szCurrentPath);
	}
	else if (YUV_Width == 704)
	{
		sprintf(szYUVName, "%s/YV12_704x576.yuv", szCurrentPath);
		sprintf(szOutH264Name, "%s/Encodec_704x576.264", szCurrentPath);
	}
	else if (YUV_Width == 640)
	{
		sprintf(szYUVName, "%s/YV12_640x480.yuv", szCurrentPath);
		sprintf(szOutH264Name, "%s/Encodec_640x480.264", szCurrentPath);
	}
	else if (YUV_Width == 352)
	{
		sprintf(szYUVName, "%s/YV12_352x288.yuv", szCurrentPath);
		sprintf(szOutH264Name, "%s/Encodec_352x288.264", szCurrentPath);
	}
	else //非法的YUV
		return -1;
	printf("open file [%s]  \r\n", szYUVName);
	//打开原始的YUV文件
	FILE* fFile;//输入的YUV文件
	fFile = fopen(szYUVName, "rb");
	if (fFile == NULL)
	{
		printf("open fail errno = % d reason = % s \n", errno);
		printf("open file error =[%s] \r\n", szYUVName);
		return 0;
	}
	
	printf("open file [%s]  \r\n", szOutH264Name);
	//创建准备写入264编码数据的文件句柄
	fFileH264 = fopen(szOutH264Name, "wb");
	if (fFileH264 == NULL)
	{
		printf("open file error =[%s] \r\n", szOutH264Name);
		return 0;
	}
	
	//创建cuda 的 H264 编码器，输入的YUV格式为  cudaEncodeVideo_YV12 ，宽、高分别为  YUV_Width, YUV_Height
	bool bres1= cudaEncode_CreateVideoEncode(cudaEncodeVideo_H264, cudaEncodeVideo_YV12, YUV_Width, YUV_Height, nCudaEncode);

	printf("cudaEncode_CreateVideoEncode =[%d] \r\n", bres1);

	szYUVBuffer = new unsigned char[(YUV_Width*YUV_Height * 3) / 2];
	szEncodeBuffer = new unsigned char[(YUV_Width*YUV_Height * 3) / 2];
	while (true)
	{
		//每次读取一帧的YUV数据，长度为 nSize =  (YUV_Width*YUV_Height * 3) / 2 
		nRead = fread(szYUVBuffer, 1, nSize, fFile);
		if (nRead <= 0)
		{
			printf("open file nRead =[%d] \r\n", nRead);
			break;
		}
		
		nEncodeLength = cudaEncode_CudaVideoEncode(nCudaEncode, szYUVBuffer, nSize,(char*)szEncodeBuffer);
		if (nEncodeLength > 0)
		{//如果编码成功 nEncodeLength 大于 0 ，写入编码文件中  fFileH264
			fwrite(szEncodeBuffer, 1, nEncodeLength, fFileH264);
			fflush(fFileH264);
			printf("nEncodeLength %d  \r\n", nEncodeLength);
		}
		else
		{
			printf("nEncodeLength %d  \r\n", nEncodeLength);
		}
	}

	if (szYUVBuffer)
		delete [] szYUVBuffer;
	if (szEncodeBuffer)
		delete szEncodeBuffer;

	fclose(fFile);
	fclose(fFileH264);

	//编码结束后，删除编码句柄 nCudaEncode
	cudaEncode_DeleteVideoEncode(nCudaEncode);

	//释放cuda编码库
	cudaEncode_UnInit();

    return 0;
}

