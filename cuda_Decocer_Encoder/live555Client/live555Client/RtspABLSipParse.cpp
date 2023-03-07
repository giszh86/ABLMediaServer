#include "stdafx.h"
#include "RtspABLSipParse.h"

CRtspABLSipParse::CRtspABLSipParse()
{
	strcpy(szSplitStr[0], ";");
	strcpy(szSplitStr[1], ",");
}

CRtspABLSipParse::~CRtspABLSipParse()
{
	//�����������У�sipLock ����������Ѿ����ͷŵ�����Ҫ����
	std::lock_guard<std::mutex> lock(sipLock);

	SipFieldStruct * sipKey = NULL;
	for (SipFieldStructMap::iterator iterator1 = sipFieldValueMap.begin(); iterator1 != sipFieldValueMap.end(); ++iterator1)
	{
		if((*iterator1).second != NULL)
		{
			sipKey = (*iterator1).second;

			if(sipKey != NULL)
			{
			  delete sipKey;
			  sipKey = NULL;
			}
		}
 	} 
	sipFieldValueMap.clear(); 
}
	
//��sipͷȫ��װ��map���棬�������ҳ���ϸ��������
bool CRtspABLSipParse::ParseSipString(char* szSipString)
{
	std::lock_guard<std::mutex> lock(sipLock);

	if (strlen(szSipString) <= 4)
		return false;

	string           strSipStringFull = szSipString;
	int              nPosBody;

	for (SipFieldStructMap::iterator iterator1 = sipFieldValueMap.begin(); iterator1 != sipFieldValueMap.end(); ++iterator1)
	{
		if((*iterator1).second != NULL)
		{
			SipFieldStruct * sipKey = (*iterator1).second;
			
			if(sipKey != NULL)
			{
			  delete sipKey;
			  sipKey = NULL;
			}
		}
 	}
	sipFieldValueMap.clear();

	//���ҳ�body����
	memset(szSipBodyContent, 0x00, sizeof(szSipBodyContent));
	nPosBody = strSipStringFull.find("\r\n\r\n", 0);
	if (nPosBody > 0 && strlen(szSipString) - (nPosBody + 4) > 0 && strlen(szSipString) - (nPosBody + 4) < MaxSipBodyContentLength)
	{//��body��������
		memcpy(szSipBodyContent, szSipString + nPosBody + 4, strlen(szSipString) - (nPosBody + 4));
		szSipString[nPosBody + 4] = 0x00;
	}

	string strSipString = szSipString;
	char   szLineString[1024] = { 0 };
	string strLineSting;
	string strFieldValue;

	int nLineCount = 0;
	int nPos1 = 0 ,  nPos2 = 0,nPos3;
	while (true)
	{
	    SipFieldStruct * sipKey = NULL;
	    SipFieldStruct * sipKey2 = NULL;
	    SipFieldStruct * sipKey3 = NULL;
	    SipFieldStruct * sipKey4 = NULL;

		nPos2 = strSipString.find("\r\n", nPos1);
		if (nPos2 > 0 && nPos2 - nPos1 > 0 )
		{
			memset(szLineString, 0x00, sizeof(szLineString));
			memcpy(szLineString, szSipString + nPos1, nPos2 - nPos1);

			do
			{
			  sipKey = new SipFieldStruct;
			  strLineSting = szLineString;
			}while(sipKey == NULL);
			if (nLineCount == 0)
			{
				nPos3 = strLineSting.find(" ", 0);
				if (nPos3 > 0)
				{
					memcpy(sipKey->szKey, szLineString, nPos3);
					memcpy(sipKey->szValue, szLineString + nPos3 + 1, strlen(szLineString) - (nPos3 + 1));
				}
			}
			else
			{
				nPos3 = strLineSting.find(":", 0);
				if (nPos3 > 0)
				{
					memcpy(sipKey->szKey, szLineString, nPos3);
					memcpy(sipKey->szValue, szLineString + nPos3 + 1, strlen(szLineString) - (nPos3 + 1));
#if 1
					//����boost:string trim ����ȥ���ո�
					string strTrimLeft = sipKey->szValue;
					boost::trim(strTrimLeft);
					strcpy(sipKey->szValue, strTrimLeft.c_str());
#endif
				}
			}

			if (strlen(sipKey->szKey) > 0)
			{
				sipFieldValueMap.insert(SipFieldStructMap::value_type(sipKey->szKey, sipKey));

				//�ѷ�����Ϊһ���ؼ��֣�KEY���洢����
				if (nLineCount == 0)
				{
					do
					{
  					  sipKey2 = new SipFieldStruct;
					}while(sipKey2 == NULL);
					strcpy(sipKey2->szKey, "Method");
					strcpy(sipKey2->szValue, sipKey->szKey);
					sipFieldValueMap.insert(SipFieldStructMap::value_type(sipKey2->szKey, sipKey2));
				}

				//��������
				int  nPos4=0, nPos5,nPos6;
				char             subKeyValue[1024] = { 0 };
				string           strSubKeyValue;

				strFieldValue = sipKey->szValue;
				for (int i = 0; i < 2; i++)
				{//ѭ������2�Σ�һ�� ;��һ�� ,

					while (true)
					{
						nPos5 = strFieldValue.find(szSplitStr[i], nPos4);
						if (nPos5 > 0 && nPos5 - nPos4 > 0)
						{
							memset(subKeyValue, 0x00, sizeof(subKeyValue));
							memcpy(subKeyValue, sipKey->szValue + nPos4, nPos5 - nPos4);

							strSubKeyValue = subKeyValue;
							nPos6 = strSubKeyValue.find("=", 0);
							if (nPos6 > 0)
							{
								do
								{
								  sipKey3 = new SipFieldStruct;
								}while(sipKey3 == NULL);
								memcpy(sipKey3->szKey, subKeyValue, nPos6);
								memcpy(sipKey3->szValue, subKeyValue + nPos6 + 1, strlen(subKeyValue) - (nPos6 + 1));

#if 1
								//����boost:string trim ����ȥ���ո�
								string strTrimLeft = sipKey3->szKey;
								boost::trim(strTrimLeft);
								strcpy(sipKey3->szKey, strTrimLeft.c_str());

								//ɾ��˫����
								strTrimLeft = sipKey3->szValue;
								boost::erase_all(strTrimLeft, "\"");
								strcpy(sipKey3->szValue, strTrimLeft.c_str());
#endif
								sipFieldValueMap.insert(SipFieldStructMap::value_type(sipKey3->szKey, sipKey3));
							}
						}
						else
						{
							if (nPos4 > 0 && strstr(sipKey->szValue, szSplitStr[i]) != NULL && strlen(sipKey->szValue) > nPos4)
							{
								memset(subKeyValue, 0x00, sizeof(subKeyValue));
								memcpy(subKeyValue, sipKey->szValue + nPos4, strlen(sipKey->szValue) - nPos4);

								strSubKeyValue = subKeyValue;
								nPos6 = strSubKeyValue.find("=", 0);
								if (nPos6 > 0)
								{
									do
									{
									  sipKey4 = new SipFieldStruct;
									}while(sipKey4 == NULL);
									memcpy(sipKey4->szKey, subKeyValue, nPos6);
									memcpy(sipKey4->szValue, subKeyValue + nPos6 + 1, strlen(subKeyValue) - (nPos6 + 1));

#if 1
									//����boost:string trim ����ȥ���ո�
									string strTrimLeft = sipKey4->szKey;
									boost::trim(strTrimLeft);
									strcpy(sipKey4->szKey, strTrimLeft.c_str());

									//ɾ��˫����
									strTrimLeft = sipKey4->szValue;
									boost::erase_all(strTrimLeft, "\"");
									strcpy(sipKey4->szValue, strTrimLeft.c_str());
#endif
									sipFieldValueMap.insert(SipFieldStructMap::value_type(sipKey4->szKey, sipKey4));
								}
							}

							break;
						}

						nPos4 = nPos5 + 1;
					}

				}//for (int i = 0; i < 2; i++)
			}
			else
			{
				if(sipKey != NULL)
				{
				  delete sipKey;
				  sipKey = NULL;
				}
			}

			nPos1 = nPos2+2;
			nLineCount ++;
		}
		else
			break;
	}

	return true;
}

bool CRtspABLSipParse::GetFieldValue(char* szKey, char* szValue)
{
	std::lock_guard<std::mutex> lock(sipLock);

	SipFieldStruct * sipKey = NULL;
	SipFieldStructMap::iterator iterator1 = sipFieldValueMap.find(szKey);

	if (iterator1 != sipFieldValueMap.end())
	{
		sipKey = (*iterator1).second;
		strcpy(szValue, sipKey->szValue);
		 
		return true;
	}
	else
		return false;
}

bool CRtspABLSipParse::AddFieldValue(char* szKey, char* szValue)
{
	std::lock_guard<std::mutex> lock(vectorLock);

	SipFieldStruct  sipKey ;
	strcpy(sipKey.szKey, szKey);
	strcpy(sipKey.szValue, szValue);
	sipFieldValueVector.push_back(sipKey);

	return true;
}

bool CRtspABLSipParse::GetFieldValueString(char* szSipString)
{
	std::lock_guard<std::mutex> lock(vectorLock);

	if (sipFieldValueVector.size() == 0)
		return false;

	int nSize = sipFieldValueVector.size();
	int i;
	for (i = 0; i < nSize; i++)
	{
		sprintf(szSipLineBuffer, "%s: %s\r\n", sipFieldValueVector[i].szKey, sipFieldValueVector[i].szValue);
		if (i == 0)
			strcpy(szSipString, szSipLineBuffer);
		else
			strcat(szSipString, szSipLineBuffer);
	}
	strcat(szSipString, "\r\n");
	sipFieldValueVector.clear();

	return true;
}

int   CRtspABLSipParse::GetSize()
{
	std::lock_guard<std::mutex> lock(vectorLock);
	return sipFieldValueMap.size();
}
