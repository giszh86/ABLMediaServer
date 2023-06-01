// ConfigFile.h: interface for the CConfigFile class.  
//
#ifdef OS_System_Windows
#pragma  once 


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CConfigFile  
{
public:
	static bool WriteConfigString(LPCTSTR lpSection, LPCTSTR lpKey, LPCTSTR  lpWriteBuff);
	static char* ReadConfigString(LPCTSTR lpSection, LPCTSTR lpKey, LPCTSTR lpDefault);
	static bool FindFile(char* sCongfigName);

	static char lpBuff[2048];
	static char szKey[64];

	virtual ~CConfigFile();
	CConfigFile();

protected:
	static char m_ConfigFileName[256];
};

#endif