#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <float.h>

#include<sys/types.h> 
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h> 

#include <pthread.h>
#include <string>
#include <list>
#include <map>
#include <math.h>
#include <iconv.h>

#include "live555Client.h"
#include "cudaCodecDLL.h"
#include "cudaEncodeDLL.h"

using namespace std;
#define  MaxRtspClientCount  1 

int       nRecvPacketCount = 0 ;
char      szRtspURL[MaxRtspClientCount][256] ;
uint64_t  nRtspChan[MaxRtspClientCount] ;
char*     pOutH264Data = new char[1024*1024*2];

uint64_t  nCudaChan = 0,nEncodeCudaChan = 0 ;
int       nDecodeFrameCount,nOutDecodeLength;

#define  WriteH264File   1

#ifdef WriteH264File
  FILE*    fWrite264 = NULL ;
  uint64_t nWriteFrameCount = 0;    
#endif

void live555RTSP_AudioVideo(uint64_t nRtspChan, int RtspDataType, char* codeName, unsigned char * pAVData, int nAVDataLength, int64_t timeValue, void* pCustomerData)
{
	nRecvPacketCount ++ ;
	if(nRecvPacketCount % 50 == 0)
	{
	  printf("nRtspChan= %d , RtspDataType = %d ,  codeName = %s , nAVDataLength = %d \r\n",nRtspChan, RtspDataType, codeName, nAVDataLength);
	  if(nRecvPacketCount >= 10000)
		  nRecvPacketCount = 0;
	}
	if(RtspDataType == XHRtspDataType_Video)
	{
		if(nCudaChan == 0)
		{
		  if(strcmp(codeName,"H264") == 0)
		  {
	        bool bCreateCudaFlag = cudaCodec_CreateVideoDecode(cudaCodecVideo_H264,cudaCodecVideo_NV12, 720, 480, nCudaChan);	
	        printf("bCreateCudaFlag = %d , nCudaChan = %d \r\n",nCudaChan,nCudaChan) ;	
		  }else if(strcmp(codeName,"H265") == 0)
		  {
	        bool bCreateCudaFlag = cudaCodec_CreateVideoDecode(cudaCodecVideo_HEVC,cudaCodecVideo_NV12, 720, 480, nCudaChan);	
	        printf("bCreateCudaFlag = %d , nCudaChan = %d \r\n",nCudaChan,nCudaChan) ;	
		  }
 		}
		
		//创建cuda编码器 
		if(nEncodeCudaChan == 0)
		{
		  cudaEncode_CreateVideoEncode((cudaEncodeVideo_enum)cudaEncodeVideo_H264,(cudaEncodeVideo_enum)cudaCodecVideo_NV12, 1920, 1080, nEncodeCudaChan);
		}
		
		if(nCudaChan != 0)
		{
			unsigned char* yuvData = NULL ;
	        yuvData = cudaCodec_CudaVideoDecode(nCudaChan,pAVData,nAVDataLength,nDecodeFrameCount,nOutDecodeLength);
		  
		   if(nEncodeCudaChan != 0 )
		   {
			  if(yuvData != NULL &&  nOutDecodeLength > 0)
			  {
				  int nH264Len = cudaEncode_CudaVideoEncode(nEncodeCudaChan, yuvData, nOutDecodeLength, pOutH264Data);
				  if(nH264Len > 0)
				  {
#ifdef WriteH264File
                    if(fWrite264 != NULL && nWriteFrameCount < ( (25 * 60) * 3 ) )
					{
                      fwrite(pOutH264Data,1,nH264Len,fWrite264) ;
					  fflush(fWrite264);
					}
					nWriteFrameCount ++ ;
#endif					  
				  }
			  }
		   }
		}
	}
}

int main(void)
{ 
	printf("hello world \r\n") ;
	
	bool bInitCudaFlag = cudaCodec_Init(); 
	bool bInitCudaFlag2 = cudaEncode_Init(); 
 	printf("bInitCudaFlag = %d , bInitCudaFlag2 = %d \r\n",bInitCudaFlag,bInitCudaFlag2) ;
 	
	int nCudaCount = cudaCodec_GetDeviceGetCount();
 	int nCudaCount2 = cudaEncode_GetDeviceGetCount();
	printf("nCudaCount = %d , nCudaCount2 = %d \r\n",nCudaCount,nCudaCount2) ;
	
	char cudaName1[256]={0},cudaName2[256];
	cudaCodec_GetDeviceName(0,cudaName1);
	cudaCodec_GetDeviceName(1,cudaName2);
	printf("cudaName1 = %s , cudaName2 = %s \r\n",cudaName1,cudaName2) ;
		
	bool bRet = live555Client_Init(NULL)  ;
    printf("live555Client_Init = %d \r\n",bRet);
 			
	strcpy(szRtspURL[0],"rtsp://admin:Jsxzh_123@172.20.200.155:554/cam/realmonitor?channel=1&subtype=0"); 
    //摄像机取流地址：rtsp://admin:Jsxzh_123@172.20.200.155:554/cam/realmonitor?channel=1&subtype=0
    //从NVR过来的地址：rtsp://admin:admin123@172.20.200.248:554/cam/realmonitor?channel=2&subtype=0
	//strcpy(szRtspURL[0],"rtsp://admin:admin123@172.20.200.248:554/cam/realmonitor?channel=2&subtype=0"); 
  
    #ifdef WriteH264File
	   char szFileName[256]={0};
	   sprintf(szFileName,"out_%d.264",rand());
       fWrite264 = fopen(szFileName,"wb");
    #endif
  
	for(int i=0;i<MaxRtspClientCount;i++)
	{
	  live555Client_ConnectCallBack(szRtspURL[i],XHRtspURLType_Liveing,true,NULL,live555RTSP_AudioVideo,nRtspChan[i]);
	  usleep(1000*15);
	}
	
	int nCount = 0; 
	unsigned char nGet ;
	while(true)
	{
	   nGet = getchar() ;
	   if(nGet == 'x' || nGet == 'X')
		   break ;
	}
	
	for(int i=0;i<MaxRtspClientCount;i++)
	{
  	  live555Client_Disconnect(nRtspChan[i]);
	  usleep(1000*10);
	}
 
    #ifdef WriteH264File
        fclose(fWrite264) ;
    #endif
	
	return 0 ;
}




