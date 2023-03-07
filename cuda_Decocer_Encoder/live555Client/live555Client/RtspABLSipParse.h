#ifndef _RtspABLSipParse_H
#define _RtspABLSipParse_H

#define  MaxSipBodyContentLength   128*1024 //最大的body数据长度

#include <mutex>

struct SipFieldStruct
{
	char szKey[1024];
	char szValue[1024];
	SipFieldStruct()
	{
		memset(szKey, 0x00, sizeof(szKey));
		memset(szValue, 0x00, sizeof(szValue));
	}
};

struct SipBodyHead
{
	char CmdType[1024];
	char SN[1024];
	char DeviceID[1024];
	char Status[1024];
	SipBodyHead()
	{
		memset(CmdType, 0x00, sizeof(CmdType));
		memset(SN, 0x00, sizeof(SN));
		memset(DeviceID, 0x00, sizeof(DeviceID));
		memset(Status, 0x00, sizeof(Status));
	}
};


typedef  map<string, SipFieldStruct*, less<string> > SipFieldStructMap;
typedef  vector<SipFieldStruct > SipFieldStructVector;

class CRtspABLSipParse
{
	public:
	CRtspABLSipParse();
	~CRtspABLSipParse();

	int          GetSize();
	SipBodyHead  sipBodyHead;
	
	bool ParseSipString(char* szSipString);
	bool GetFieldValue(char* szKey, char* szValue);

	bool AddFieldValue(char* szKey, char* szValue);
	bool GetFieldValueString(char* szSipString);

	std::mutex           sipLock;
	std::mutex           vectorLock;
	SipFieldStructMap    sipFieldValueMap;

	SipFieldStructVector sipFieldValueVector;
	char                 szSipLineBuffer[2048];
	char                 szSplitStr[2][1024];

	char                 szSipBodyContent[MaxSipBodyContentLength];//SipBody数据
};

#endif
