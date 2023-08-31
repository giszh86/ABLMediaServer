/*
���ܣ� cuda Ӣΰ���Կ�Ӳ������������� 
*/

#include "stdafx.h"

//ͨ��ע������3�����趨YUV�Ŀ���
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

uint64_t       nCudaEncode;//cudaӲ��������

int            nSize = (YUV_Width*YUV_Height * 3) / 2;//Դ�ߴ�һ֡YUV��С
int            nEncodeLength;
unsigned char* szEncodeBuffer = NULL;
FILE*          fFileH264;//�����д��264�ļ����
char           szCurrentPath[256] = { 0 };//��ǰ·��

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
//cuda ���� 
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

//cuda ���� 
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
//	else //�Ƿ���YUV
//		return -1;
//	printf("open file [%s]  \r\n", szYUVName);
//	//��ԭʼ��YUV�ļ�
//	FILE* fFile;//�����YUV�ļ�
//	fFile = fopen(szYUVName, "rb");
//	if (fFile == NULL)
//	{
//		printf("open fail errno = % d reason = % s \n", errno);
//		printf("open file error =[%s] \r\n", szYUVName);
//		return 0;
//	}
//
//	printf("open file [%s]  \r\n", szOutH264Name);
//	//����׼��д��264�������ݵ��ļ����
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
//		//ÿ�ζ�ȡһ֡��YUV���ݣ�����Ϊ nSize =  (YUV_Width*YUV_Height * 3) / 2 
//		int nRead = fread(szYUVBuffer, 1, nSize, fFile);
//		if (nRead <= 0)
//		{
//			printf("open file nRead =[%d] \r\n", nRead);
//			break;
//		}
//		// �������Ѿ���ȡ��YUV���ݵ� szYUVBuffer ��
//		unsigned char* yuvData = szYUVBuffer; // ָ��YUV���ݵ�ָ��
//		int yuvWidth = YUV_Width;
//		int yuvHeight = YUV_Height;
//		nvFrame *frame=new nvFrame;
//
//		// ���� nvFrame ���ֶ�
//		frame->flags = 0; // ������Ҫ����
//		frame->payload_size[0] = yuvWidth * yuvHeight; // Y�����Ĵ�С
//		frame->payload_size[1] = yuvWidth * yuvHeight / 4; // U�����Ĵ�С
//		frame->payload_size[2] = yuvWidth * yuvHeight / 4; // V�����Ĵ�С
//		frame->payload[0] = yuvData; // Y��������ָ��
//		frame->payload[1] = yuvData + frame->payload_size[0]; // U��������ָ��
//		frame->payload[2] = yuvData + frame->payload_size[0] + frame->payload_size[1]; // V��������ָ��
//		frame->linesize[0] = yuvWidth; // Y����ÿ�еĴ�С
//		frame->linesize[1] = yuvWidth / 2; // U����ÿ�еĴ�С
//		frame->linesize[2] = yuvWidth / 2; // V����ÿ�еĴ�С
//		frame->type = nvPixFormat::NV_PIX_YUV420;
//		frame->width = yuvWidth;
//		frame->height = yuvHeight;
//		frame->timestamp = time(nullptr); // ����ʱ���
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
//			//�������ɹ� nEncodeLength ���� 0 ��д������ļ���  fFileH264
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
//	//���������ɾ�������� nCudaEncode
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
	//���ö�̬���صķ�ʽ����Ϊ��Щ������Ӣΰ���Կ�����Щ����û��Ӣΰ���Կ� 
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
	//��ʼ��cuda�����
	if(cudaEncode_Init)
		bInitCudaFlag = cudaEncode_Init();
	
	//��ȡcuda�Կ�����
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

	//��ȡӢΰ���һ���Կ�����
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
	else //�Ƿ���YUV
		return -1;
	printf("open file [%s]  \r\n", szYUVName);
	//��ԭʼ��YUV�ļ�
	FILE* fFile;//�����YUV�ļ�
	fFile = fopen(szYUVName, "rb");
	if (fFile == NULL)
	{
		printf("open fail errno = % d reason = % s \n", errno);
		printf("open file error =[%s] \r\n", szYUVName);
		return 0;
	}
	
	printf("open file [%s]  \r\n", szOutH264Name);
	//����׼��д��264�������ݵ��ļ����
	fFileH264 = fopen(szOutH264Name, "wb");
	if (fFileH264 == NULL)
	{
		printf("open file error =[%s] \r\n", szOutH264Name);
		return 0;
	}
	
	//����cuda �� H264 �������������YUV��ʽΪ  cudaEncodeVideo_YV12 �����߷ֱ�Ϊ  YUV_Width, YUV_Height
	bool bres1= cudaEncode_CreateVideoEncode(cudaEncodeVideo_H264, cudaEncodeVideo_YV12, YUV_Width, YUV_Height, nCudaEncode);

	printf("cudaEncode_CreateVideoEncode =[%d] \r\n", bres1);

	szYUVBuffer = new unsigned char[(YUV_Width*YUV_Height * 3) / 2];
	szEncodeBuffer = new unsigned char[(YUV_Width*YUV_Height * 3) / 2];
	while (true)
	{
		//ÿ�ζ�ȡһ֡��YUV���ݣ�����Ϊ nSize =  (YUV_Width*YUV_Height * 3) / 2 
		nRead = fread(szYUVBuffer, 1, nSize, fFile);
		if (nRead <= 0)
		{
			printf("open file nRead =[%d] \r\n", nRead);
			break;
		}
		
		nEncodeLength = cudaEncode_CudaVideoEncode(nCudaEncode, szYUVBuffer, nSize,(char*)szEncodeBuffer);
		if (nEncodeLength > 0)
		{//�������ɹ� nEncodeLength ���� 0 ��д������ļ���  fFileH264
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

	//���������ɾ�������� nCudaEncode
	cudaEncode_DeleteVideoEncode(nCudaEncode);

	//�ͷ�cuda�����
	cudaEncode_UnInit();

    return 0;
}

