
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
	int erase_all(std::string& strBuf, const std::string& strDel)
	{
		if (strDel.empty())
		{
			return 0;  // ÎÞÐèÉ¾³ý£¬·µ»ØÉ¾³ý´ÎÊýÎª0
		}

		std::size_t pos = 0;
		int nCount = 0;

		while ((pos = strBuf.find(strDel, pos)) != std::string::npos)
		{
			strBuf.erase(pos, strDel.length());
			++nCount;
		}

		return nCount;
	}

	//std::string to_lower(std::string strBuf)
	//{
	//	if (strBuf.empty())
	//	{
	//		return "";
	//	}
	//	return strlwr((char*)strBuf.c_str());
	////	_strlwr_s((char*)strBuf.c_str(), strBuf.length() + 1);
	////	return strBuf;

	//}

	int	replace_all(std::string& strBuf, std::string  strSrc, std::string  strDes)
	{
		if (strSrc.empty())
		{
			return 0;  // ÎÞÐèÌæ»»£¬·µ»ØÌæ»»´ÎÊýÎª0
		}

		std::size_t pos = 0;
		int nCount = 0;

		while ((pos = strBuf.find(strSrc, pos)) != std::string::npos)
		{
			strBuf.replace(pos, strSrc.length(), strDes);
			pos += strDes.length();
			++nCount;
		}

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

	void to_lower(char* str)
	{
		std::size_t length = std::strlen(str);
		for (std::size_t i = 0; i < length; ++i) {
			str[i] = std::tolower(static_cast<unsigned char>(str[i]));
		}
	}
	void to_lower(std::string& str)
	{
		for (char& c : str) {
			c = std::tolower(static_cast<unsigned char>(c));
		}
	}
	void to_upper(char* str)
	{
		std::size_t length = std::strlen(str);
		for (std::size_t i = 0; i < length; ++i) {
			str[i] = std::toupper(static_cast<unsigned char>(str[i]));
		}
	}
	void to_upper(std::string& str)
	{
		for (char& c : str) {
			c = std::toupper(static_cast<unsigned char>(c));
		}
	}
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