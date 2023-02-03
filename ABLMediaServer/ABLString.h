#pragma once

namespace ABL {

	std::string& trim(std::string& s);

	// É¾³ý×Ö·û´®ÖÐÖ¸¶¨µÄ×Ö·û´®
	int	erase_all(std::string& strBuf, std::string  strDel);

	std::string to_lower(std::string strBuf);

	int	replace_all(std::string& strBuf, std::string  strSrc, std::string  strDes);
	
	bool is_digits(const std::string& str);
}