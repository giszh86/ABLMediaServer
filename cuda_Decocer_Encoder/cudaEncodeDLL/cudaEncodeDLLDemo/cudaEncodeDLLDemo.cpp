/*
功能： cuda 英伟达显卡硬件编码调用例子 
 	Author 罗家兄弟
	Date   2022-07-15
	QQ     79941308
	E-mail 79941308@qq.com
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
FILE*          fFile;//输入的YUV文件
int            nSize = (YUV_Width*YUV_Height * 3) / 2;//源尺寸一帧YUV大小
int            nEncodeLength;
unsigned char* szEncodeBuffer = NULL;
FILE*          fFileH264;//编码后，写入264文件句柄
char           szCurrentPath[256] = { 0 };//当前路径

bool GetCurrentPath(char *szCurPath)
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

int main()
{
	char szCudaName[256] = { 0 };
	char szOutH264Name[256] = { 0 };
	bool bInitCudaFlag = false;
	int  nCudaCount = 0;

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
 
	//初始化cuda编码库
	if(cudaEncode_Init)
		bInitCudaFlag = cudaEncode_Init();
	
	//获取cuda显卡个数
	if (cudaEncode_GetDeviceGetCount)
		nCudaCount = cudaEncode_GetDeviceGetCount();

	if (bInitCudaFlag == false || nCudaCount == 0)
	{
		printf("当前电脑没有英伟达显卡或者英伟达显卡官方驱动没有安装好！");
		return -1;
	}

	//获取英伟达第一个显卡名称
	if(cudaEncode_GetDeviceName)
	  cudaEncode_GetDeviceName(0, szCudaName);
	int nRead;

	GetCurrentPath(szCurrentPath);

	if (YUV_Width == 1920)
	{
 		sprintf(szYUVName, "%sYV12_1920x1080.yuv", szCurrentPath);
		sprintf(szOutH264Name, "%sEncodec_1920x1080.264", szCurrentPath);
	}
	else if (YUV_Width == 704)
	{
		sprintf(szYUVName, "%sYV12_704x576.yuv", szCurrentPath);
		sprintf(szOutH264Name, "%sEncodec_704x576.264", szCurrentPath);
	}
	else if (YUV_Width == 640)
	{
		sprintf(szYUVName, "%sYV12_640x480.yuv", szCurrentPath);
		sprintf(szOutH264Name, "%sEncodec_640x480.264", szCurrentPath);
	}
	else if (YUV_Width == 352)
	{
		sprintf(szYUVName, "%sYV12_352x288.yuv", szCurrentPath);
		sprintf(szOutH264Name, "%sEncodec_352x288.264", szCurrentPath);
	}
	else //非法的YUV
		return -1;

	//打开原始的YUV文件
	fFile = fopen(szYUVName, "rb");
	if (fFile == NULL)
		return 0;

	//创建准备写入264编码数据的文件句柄
	fFileH264 = fopen(szOutH264Name, "wb");
	if (fFileH264 == NULL)
		return 0;

	//创建cuda 的 H264 编码器，输入的YUV格式为  cudaEncodeVideo_YV12 ，宽、高分别为  YUV_Width, YUV_Height
	cudaEncode_CreateVideoEncode(cudaEncodeVideo_H264, cudaEncodeVideo_YV12, YUV_Width, YUV_Height, nCudaEncode);

	szYUVBuffer = new unsigned char[(YUV_Width*YUV_Height * 3) / 2];
	szEncodeBuffer = new unsigned char[(YUV_Width*YUV_Height * 3) / 2];
	while (true)
	{
		//每次读取一帧的YUV数据，长度为 nSize =  (YUV_Width*YUV_Height * 3) / 2 
		nRead = fread(szYUVBuffer, 1, nSize, fFile);
		if (nRead <= 0)
			break;
		nEncodeLength = cudaEncode_CudaVideoEncode(nCudaEncode, szYUVBuffer, nSize,(char*)szEncodeBuffer);
		if (nEncodeLength > 0)
		{//如果编码成功 nEncodeLength 大于 0 ，写入编码文件中  fFileH264
			fwrite(szEncodeBuffer, 1, nEncodeLength, fFileH264);
			fflush(fFileH264);
		}
		else
		{
			printf("nEncodeLength %d", nEncodeLength);
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

