
#include "ABLString.h"
#include <thread>
#include <algorithm>
#include<ctype.h>


namespace ABL {

	char* strlwr(char* str)
	{
		if (str == NULL)
			return NULL;

		char* p = str;
		while (*p != '\0')
		{
			if (*p >= 'A' && *p <= 'Z')
				*p = (*p) + 0x20;
			p++;
		}
		return str;
	}

	inline char* strupr_(char* str)
	{
		char* origin = str;
		while (*str != '\0')
			*str++ = toupper(*str);
		return origin;
	}
	std::string& trim(std::string& s)
	{
		if (s.empty())
		{
			return s;
		}

		s.erase(0, s.find_first_not_of(" "));
		s.erase(s.find_last_not_of(" ") + 1);
		return s;
	}

	// É¾³ý×Ö·û´®ÖÐÖ¸¶¨µÄ×Ö·û´®
	int	erase_all(std::string& strBuf, std::string  strDel)
	{
		size_t				sBufSize = strBuf.size();
		char* pStart = (char*)strBuf.c_str();
		char* pEnd = pStart + sBufSize;
		std::string 			strReturn;

		if (strBuf.empty())
		{
			return strBuf.size();
		}
	
		for (;;)
		{
			char* pFind = strstr(pStart, (char*)strDel.c_str());
			if (NULL == pFind)
			{
				strReturn.append(pStart);
				break;
			}
			strReturn.append(pStart, pFind);
			pStart = pFind + strDel.size();
			if (pStart >= pEnd)
			{
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		strBuf = strReturn;
		return strBuf.size();


	}

	std::string to_lower(std::string strBuf)
	{
		if (strBuf.empty())
		{
			return "";
		}
		return strlwr((char*)strBuf.c_str());
	//	_strlwr_s((char*)strBuf.c_str(), strBuf.length() + 1);
	//	return strBuf;

	}

	int	replace_all(std::string& strBuf, std::string  strSrc, std::string  strDes)
	{
		size_t				sBufSize = strBuf.size();
		char* pStart = (char*)strBuf.c_str();
		char* pEnd = pStart + sBufSize;
		std::string 			strReturn;
		int					nCount = 0;

		if (strBuf.empty())
		{
			return strBuf.size();
		}

		for (;;)
		{
			char* pFind = strstr(pStart, (char*)strSrc.c_str());

			if (NULL == pFind)
			{
				strReturn.append(pStart);
				break;
			}

			nCount++;
			strReturn.append(pStart, pFind);
			strReturn.append(strDes);
			pStart = pFind + strSrc.size();

			if (pStart >= pEnd)
			{
				break;
			}
		}

		strBuf = strReturn;

		return nCount;
	}

	bool is_digits(const std::string& str)
	{
		return std::all_of(str.begin(), str.end(), ::isdigit); // C++11
	}

	/*
 *	Function:		StrToLwr
 *	Explanation:	×Ö·û´®×ªÐ¡Ð´
 *	Input:			strBuf		×Ö·û´®
 *	Return:			Ð¡Ð´×Ö·û´®
 */
	std::string  StrToLwr(std::string  strBuf)
	{
		if (strBuf.empty())
		{
			return "";
		}
		return strlwr((char*)strBuf.c_str());
	//	_strlwr_s((char*)strBuf.c_str(), strBuf.length() + 1);
		return strBuf;
	}
	/*
*	Function:		StrToLwr
*	Explanation:	×Ö·û´®×ª´óÐ´
*	Input:			strBuf		×Ö·û´®
*	Return:			´óÐ´×Ö·û´®
*/
	std::string  StrToUpr(std::string  strBuf)
	{
		if (strBuf.empty())
		{
			return "";
		}
		return strupr_((char*)strBuf.c_str());
	//	_strupr_s((char*)strBuf.c_str(), strBuf.length() + 1);
		//return strBuf;

	}

}