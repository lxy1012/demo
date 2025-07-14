#ifndef _CONFUTIL_H_
#define _CONFUTIL_H_

#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

using namespace  std;

typedef struct ConfNameInfo
{
	string strName;
	string strValue;
}ConfNameInfoDef;

typedef struct ConfSecInfo
{
	string strSection;
	vector<ConfNameInfoDef> vcConfNameInfo;
}ConfSecInfoDef;

typedef struct ConfFileInfo
{
	string strFile;
	vector<ConfSecInfoDef> vcConfSection;
}ConfFileInfoDef;

class ConfUtil
{
public:
	ConfUtil() {};
	~ConfUtil() {};	

	static void AnalyzeFileContent(string fileName, string fileValue);

	static int GetValueInt(int * value, char * name, char *filename, char *section = NULL, char *defval = NULL);
	static int GetValueShort(short int *value, char *name, char *filename, char *section = NULL, char *defval = NULL);
	static int GetValueFloat(float* value, char * name, char *filename, char *section = NULL, char *defval = NULL);
	static int GetValueStr(char* value, char * name, char *filename, char *section = NULL, char *defval = NULL);

private:
	static void DeleteStringEnter(string &str);
	static void DeleteCharArrayWhiteSpace(char *src);
	static void DeleteStringWhiteSpace(string &str);
	static vector<string> getLineContent(string strDst);	
	static ConfFileInfoDef* GetConfFileInfo(string fileName);

	static int GetConfValue(int *iValue, char *szName, char *szFile, char *szSection /*= NULL*/, char *strDefault /*= NULL*/);
	static int GetConfValue(float *fValue, char *szName, char *szFile, char *szSection /*= NULL*/, char *strDefault /*= NULL*/);
	static int GetConfValue(char *strInfo, char *szName, char *szFile, char *szSection /*= NULL*/, char *strDefault /*= NULL*/);

	static vector<ConfFileInfoDef> m_vcFileConf;
};

#endif