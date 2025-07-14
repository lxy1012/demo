#include "AESUtil.hpp"
#include "ConfUtil.h"

vector<ConfFileInfoDef> ConfUtil::m_vcFileConf;

void ConfUtil::DeleteStringEnter(string &str)
{
	int iIndex = str.find('\r');
	if (iIndex == -1)
	{
		iIndex = str.find('\r\n');
		if (iIndex == -1)
		{
			iIndex = str.find('\n');
		}
	}
	if (iIndex != -1)
	{
		str.erase(iIndex);
	}
}

void ConfUtil::DeleteCharArrayWhiteSpace(char *src)
{
	if (strlen(src) == 0)
		return;

	char cChar = 32;
	int iBeginIndex = 0;
	for (int i = 0; i < strlen(src); i++)
	{
		if (src[i] != cChar)
		{
			iBeginIndex = i;
			break;
		}
	}
	int iEndIndex = 0;
	for (int i = strlen(src) - 1; i >= 0; i--)
	{
		if (src[i] != cChar)
		{
			iEndIndex = i;
			break;
		}
	}
	if (iBeginIndex == 0 && iEndIndex == 0 && src[0] == cChar)
	{
		strcpy(src, "");
	}
	else
	{
		string strResult(src);
		string strResult2 = strResult.substr(iBeginIndex, iEndIndex - iBeginIndex + 1);
		strcpy(src, strResult2.c_str());
	}
}

void ConfUtil::DeleteStringWhiteSpace(string &str)
{
	char *pTmp = new char[str.length() + 1];
	strcpy(pTmp, str.c_str());
	DeleteCharArrayWhiteSpace(pTmp);
	str = pTmp;
	delete[] pTmp;
}

vector<string> ConfUtil::getLineContent(string fileValue)
{
	vector<string> strLineContent;

	while (!fileValue.empty())
	{
		int iIndex = fileValue.find('\n');
		if (iIndex != -1)
		{
			string line = fileValue.substr(0, iIndex);

			fileValue = fileValue.substr(iIndex + 1, fileValue.size());
			strLineContent.push_back(line);
		}
		else
		{
			string line = fileValue.substr(0, fileValue.size());
			strLineContent.push_back(line);
			break;
		}
	}

	return strLineContent;
}

void ConfUtil::AnalyzeFileContent(string fileName, string fileValue)
{
	ConfFileInfoDef *pConfFile = GetConfFileInfo(fileName);
	if (pConfFile != NULL)
	{
		return;
	}

	ConfFileInfoDef confFile;

	confFile.strFile = fileName;

	std::vector<std::string> strLineContent = getLineContent(fileValue);
	while (!strLineContent.empty())
	{
		std::string singleLine = strLineContent.at(0);
		DeleteStringEnter(singleLine);
		if (singleLine.length() != 0)
		{
			if (strncmp("#", (char*)singleLine.c_str(), 1) == 0)
				continue;

			if (singleLine.find("[") == 0 && singleLine.find("]") > 0)
			{
				ConfSecInfoDef confSection;
				confSection.strSection = singleLine.substr(singleLine.find("[") + 1, singleLine.find("]") - 1);
				confFile.vcConfSection.push_back(confSection);
			}
			else
			{
				if (confFile.vcConfSection.size() == 0)
				{
					return;
				}
				ConfNameInfoDef confKeyName;
				int iBeginPos = singleLine.find("=");
				confKeyName.strName = singleLine.substr(0, iBeginPos);				
				confKeyName.strValue = singleLine.substr(iBeginPos + 1);
				DeleteStringWhiteSpace(confKeyName.strName);
				DeleteStringWhiteSpace(confKeyName.strValue);
				confFile.vcConfSection[confFile.vcConfSection.size() - 1].vcConfNameInfo.push_back(confKeyName);
			}
		}

		strLineContent.erase(strLineContent.begin());
	}

	m_vcFileConf.push_back(confFile);
}

int ConfUtil::GetValueInt(int *value, char *name, char *filename, char *section, char *defval)
{
	return GetConfValue(value, name, filename, section, defval);
}

int ConfUtil::GetValueShort(short int *value, char *name, char *filename, char *section, char *defval)
{
	int iValue = 0;
	int rt = GetConfValue(&iValue, name, filename, section, defval);

	*value = (short)iValue;
	return rt;
}

int ConfUtil::GetValueFloat(float *value, char *name, char *filename, char *section, char *defval)
{
	return GetConfValue(value, name, filename, section, defval);
}

int ConfUtil::GetValueStr(char *value, char *name, char *filename, char *section, char *defval)
{
	return GetConfValue(value, name, filename, section, defval);
}

ConfFileInfoDef* ConfUtil::GetConfFileInfo(string fileName)
{
	for (int i = 0; i < m_vcFileConf.size(); i++)
	{
		if (m_vcFileConf.at(i).strFile.compare(fileName) == 0)
		{
			return &m_vcFileConf.at(i);
		}
	}

	return NULL;
}

int ConfUtil::GetConfValue(int *iValue, char *szName, char *szFile, char *szSection /*= NULL*/, char *strDefault /*= NULL*/)
{
	char tmp[128];
	memset(tmp, 0, sizeof(tmp));
	int rt = GetConfValue(tmp, szName, szFile, szSection, strDefault);
	if (strlen(tmp) > 0)
	{
		*iValue = atoi(tmp);
	}
	else
	{
		*iValue = 0;
	}
	return rt;
}

int ConfUtil::GetConfValue(float *fValue, char *szName, char *szFile, char *szSection /*= NULL*/, char *strDefault /*= NULL*/)
{
	char tmp[128];
	memset(tmp, 0, sizeof(tmp));
	int rt = GetConfValue(tmp, szName, szFile, szSection, strDefault);
	if (strlen(tmp) > 0)
	{
		*fValue = atof(tmp);
	}
	else
	{
		*fValue = 0.0f;
	}
	return rt;
}


int ConfUtil::GetConfValue(char *strInfo, char *szName, char *szFile, char *szSection /*= NULL*/, char *strDefault /*= NULL*/)
{
	if (strDefault)
	{
		strcpy(strInfo, strDefault);
	}
	else
	{
		strcpy(strInfo, "");
	}
	
	ConfFileInfoDef *confFile = GetConfFileInfo(szFile);
	if (confFile == NULL)
	{
		return 0;
	}

	string strResult = "";
	for (int i = 0; i < confFile->vcConfSection.size(); i++)
	{
		if (szSection && confFile->vcConfSection[i].strSection.compare(szSection) != 0)
			continue;

		ConfSecInfoDef confSection = confFile->vcConfSection[i];

		for (int j = 0; j< confSection.vcConfNameInfo.size(); j++)
		{
			if (strcmp(confSection.vcConfNameInfo[j].strName.c_str(), szName) == 0)
			{
				strResult = confSection.vcConfNameInfo[j].strValue;
				break;
			}
		}		
	}

	if (strResult.length() == 0)
	{
		return 0;
	}

	strcpy(strInfo, strResult.c_str());

	return 1;
}