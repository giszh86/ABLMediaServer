// ConfigFile.h: interface for the CConfigFile class.  
//

#if !defined(AFX_CONFIGFILE_H__22EA1D61_ABBB_11D5_9286_00022A00D63A__INCLUDED_)
#define AFX_CONFIGFILE_H__22EA1D61_ABBB_11D5_9286_00022A00D63A__INCLUDED_

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
	static char szKey[256];

	virtual ~CConfigFile();
	CConfigFile();

protected:
	static char m_ConfigFileName[256];
};

#endif // !defined(AFX_CONFIGFILE_H__22EA1D61_ABBB_11D5_9286_00022A00D63A__INCLUDED_)
