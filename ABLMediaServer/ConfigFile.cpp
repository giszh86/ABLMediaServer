/*
  功能：  读写配置文件，扩展名为 .ini 格式如下：
	[CDlgLoginSystem_EN]   
	  staticName        =  User
	  staticPwd         =  Password
	  checkBtnRemberPwd =  Rember Passord  
	  checkBtnAutoLogin =  Auto Login
	  szMsgInputPwd     =  Please input password !
	  szMsgPwdInvalid   =  password is invalid !
*/

#include "stdafx.h"
#include "ConfigFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


char CConfigFile::m_ConfigFileName[256] = { 0 };
char CConfigFile::lpBuff[2048] = { 0 };
char CConfigFile::szKey[64] = { 0 };

CConfigFile::CConfigFile()
{
	strcpy(szKey,"@#$%^*&") ;
}

CConfigFile::~CConfigFile()
{

}

bool CConfigFile::FindFile(char* sCongfigName)
{
   FILE* m_File = NULL;
  
   m_File = fopen(sCongfigName, "r");
   if (m_File == NULL)
   {
	   return false;
   }
 
   strcpy(m_ConfigFileName, sCongfigName);
   fclose(m_File);
   return true;
}

char* CConfigFile::ReadConfigString(LPCTSTR lpSection, LPCTSTR lpKey, LPCTSTR lpDefault)
{
   memset(lpBuff,0x00,sizeof(lpBuff)) ;

   ::GetPrivateProfileString(lpSection,lpKey,"@#$%^*&",lpBuff,sizeof(lpBuff),m_ConfigFileName) ;
   if(memcmp(lpBuff,szKey,7) == 0)
   {
	  return (char*)lpDefault  ;
   }
   else
   {
	   string strConfig = lpBuff ;
#ifdef USE_BOOST
	   boost::trim(strConfig);
#else
	   ABL::trim(strConfig);
#endif
	
	   memset(lpBuff, 0x00, sizeof(lpBuff));
	   strcpy(lpBuff, strConfig.c_str());
	   return  (char*)lpBuff ;
    }
}

bool CConfigFile::WriteConfigString(LPCTSTR lpSection, LPCTSTR lpKey, LPCTSTR lpWriteBuff)
{
   return  ::WritePrivateProfileString(lpSection,lpKey,lpWriteBuff,m_ConfigFileName) ;
}
