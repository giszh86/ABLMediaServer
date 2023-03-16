/*
���ܣ� cuda Ӣΰ���Կ�Ӳ������������� 
 	Author �޼��ֵ�
	Date   2022-07-15
	QQ     79941308
	E-mail 79941308@qq.com
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
FILE*          fFile;//�����YUV�ļ�
int            nSize = (YUV_Width*YUV_Height * 3) / 2;//Դ�ߴ�һ֡YUV��С
int            nEncodeLength;
unsigned char* szEncodeBuffer = NULL;
FILE*          fFileH264;//�����д��264�ļ����
char           szCurrentPath[256] = { 0 };//��ǰ·��

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

//cuda ���� 
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
 
	//��ʼ��cuda�����
	if(cudaEncode_Init)
		bInitCudaFlag = cudaEncode_Init();
	
	//��ȡcuda�Կ�����
	if (cudaEncode_GetDeviceGetCount)
		nCudaCount = cudaEncode_GetDeviceGetCount();

	if (bInitCudaFlag == false || nCudaCount == 0)
	{
		printf("��ǰ����û��Ӣΰ���Կ�����Ӣΰ���Կ��ٷ�����û�а�װ�ã�");
		return -1;
	}

	//��ȡӢΰ���һ���Կ�����
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
	else //�Ƿ���YUV
		return -1;

	//��ԭʼ��YUV�ļ�
	fFile = fopen(szYUVName, "rb");
	if (fFile == NULL)
		return 0;

	//����׼��д��264�������ݵ��ļ����
	fFileH264 = fopen(szOutH264Name, "wb");
	if (fFileH264 == NULL)
		return 0;

	//����cuda �� H264 �������������YUV��ʽΪ  cudaEncodeVideo_YV12 �����߷ֱ�Ϊ  YUV_Width, YUV_Height
	cudaEncode_CreateVideoEncode(cudaEncodeVideo_H264, cudaEncodeVideo_YV12, YUV_Width, YUV_Height, nCudaEncode);

	szYUVBuffer = new unsigned char[(YUV_Width*YUV_Height * 3) / 2];
	szEncodeBuffer = new unsigned char[(YUV_Width*YUV_Height * 3) / 2];
	while (true)
	{
		//ÿ�ζ�ȡһ֡��YUV���ݣ�����Ϊ nSize =  (YUV_Width*YUV_Height * 3) / 2 
		nRead = fread(szYUVBuffer, 1, nSize, fFile);
		if (nRead <= 0)
			break;
		nEncodeLength = cudaEncode_CudaVideoEncode(nCudaEncode, szYUVBuffer, nSize,(char*)szEncodeBuffer);
		if (nEncodeLength > 0)
		{//�������ɹ� nEncodeLength ���� 0 ��д������ļ���  fFileH264
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

	//���������ɾ�������� nCudaEncode
	cudaEncode_DeleteVideoEncode(nCudaEncode);

	//�ͷ�cuda�����
	cudaEncode_UnInit();

    return 0;
}

