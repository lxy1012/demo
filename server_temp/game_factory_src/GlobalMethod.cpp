#include "GlobalMethod.h"
#ifdef _WIN32
#include "iconv.h"
#include "common.h"
#else
#include <iconv.h>
#include <sys/time.h>
#include <iostream>
#include <string.h>
#endif
using namespace std;
//代码转换:从一种编码转为另一种编码
GlobalMethod::GlobalMethod()
{

}

GlobalMethod::~GlobalMethod()
{
}


int GlobalMethod::code_convert(char *from_charset,char *to_charset,char *inbuf,size_t inlen,char *outbuf,size_t outlen)
{
	iconv_t cd;
	int rc;
	char **pin = &inbuf;
	char **pout = &outbuf;
	
	cd = iconv_open(to_charset,from_charset);
	if (cd==0) return -1;
	memset(outbuf,0,outlen);
#ifdef _WIN32
	if (iconv(cd, (const char**)pin, &inlen, pout, &outlen) == -1)
#else
	if (iconv(cd, pin, &inlen, pout, &outlen) == -1)
#endif
	{
		iconv_close(cd);
		return -1;
	}
	iconv_close(cd);
	return 0;
}

int GlobalMethod::GetDayTimeFlag(const time_t& theTime)
{
	struct tm* timenow;
	timenow = localtime(&theTime);
	int year = 1900 + timenow->tm_year;
	int month = 1 + timenow->tm_mon;
	int day = timenow->tm_mday;
	return year * 10000 + month * 100 + day;
}


int GlobalMethod::GetDayTimeFlag(char* szTime)
{
	int iYear = 0;
	int iMonth = 0;
	int iDay = 0;
	int iHour = 0;
	int iMinute = 0;
	int iSec = 0;
	//MM/DD/YYYY hh24:mi:ss
	int iRt = sscanf(szTime, "%d/%d/%d %d:%d:%d", &iMonth, &iDay, &iYear, &iHour, &iMinute, &iSec);
	if (iRt != 6)
	{
		return -1;
	}
	return  iYear * 10000 + iMonth * 100 + iDay;
}

void GlobalMethod::GetTodayTimeString(char* szTime)
{
	time_t theTime = time(NULL);
	struct tm* timenow;
	timenow = localtime(&theTime);
	int year = 1900 + timenow->tm_year;
	int month = 1 + timenow->tm_mon;
	int day = timenow->tm_mday;
	sprintf(szTime, "%d-%02d-%02d", year, month, day);
}

int GlobalMethod::GetTodayLeftSeconds()
{
	time_t theTime = time(NULL);
	struct tm* tm_t;
	tm_t = localtime(&theTime);
	tm_t->tm_hour = 23;
	tm_t->tm_min = 59;
	tm_t->tm_sec = 59;
	int iEndTm = mktime(tm_t);
	int iLeftTime = iEndTm - time(NULL);
	return iLeftTime;
}

time_t GlobalMethod::GetTodayStartTimestamp()
{
	time_t theTime = time(NULL);
	struct tm* tm_t;
	tm_t = localtime(&theTime);
	tm_t->tm_hour = 0;
	tm_t->tm_min = 0;
	tm_t->tm_sec = 0;
	time_t iEndTm = mktime(tm_t);
	return iEndTm;
}

time_t GlobalMethod::GetWeekStartTimestamp()
{
	time_t theTime = time(NULL);
	struct tm* tm_t;
	tm_t = localtime(&theTime);
	int iWeekDay = tm_t->tm_wday;
	tm_t->tm_hour = 0;
	tm_t->tm_min = 0;
	tm_t->tm_sec = 0;
	time_t iEndTm = mktime(tm_t);
	if (iWeekDay == 0)
	{
		iWeekDay = 7;
	}
	int iSubTm = (iWeekDay - 1) * 24 * 3600;
	iEndTm -= iSubTm;
	return iEndTm;
}

time_t GlobalMethod::GetNowHourStartTimeStamp()
{
	time_t theTime = time(NULL);
	struct tm* tm_t;
	tm_t = localtime(&theTime);
	tm_t->tm_min = 0;
	tm_t->tm_sec = 0;
	time_t iEndTm = mktime(tm_t);
	return iEndTm;
}

time_t  GlobalMethod::GetPointDayStartTimestamp(char* szTimeIn)
{
	struct tm tm_t;
	int iRt = sscanf(szTimeIn, "%d/%d/%d", &(tm_t.tm_year), &(tm_t.tm_mon), &(tm_t.tm_mday));
	if (iRt != 3)
	{
		return -1;
	}
	tm_t.tm_mon -= 1;
	tm_t.tm_year -= 1900;
	tm_t.tm_hour = 0;
	tm_t.tm_min = 0;
	tm_t.tm_sec = 0;
	time_t iTm = mktime(&tm_t);
	return iTm;
}

time_t GlobalMethod::GetPointDayStartTimestamp(int iYear, int iMonth, int iDay)
{
	struct tm tm_t;
	tm_t.tm_mon = iMonth - 1;
	tm_t.tm_year = iYear - 1900;
	tm_t.tm_mday = iDay;
	tm_t.tm_hour = 0;
	tm_t.tm_min = 0;
	tm_t.tm_sec = 0;
	time_t iTm = mktime(&tm_t);
	return iTm;
}

time_t GlobalMethod::GetPointDayStartTimestamp(int iDateFlag)
{
	char szTime[16];
	sprintf(szTime, "%d", iDateFlag);
	char szTemp[8];
	memset(szTemp, 0, sizeof(szTemp));
	memcpy(szTemp, szTime, 4);
	int iYear = atoi(szTemp);
	memset(szTemp, 0, sizeof(szTemp));
	if (szTime[4] == '0')
	{
		memcpy(szTemp, szTime + 5, 1);
	}
	else
	{
		memcpy(szTemp, szTime + 4, 2);
	}
	int iMonth = atoi(szTemp);
	memset(szTemp, 0, sizeof(szTemp));
	if (szTime[6] == '0')
	{
		memcpy(szTemp, szTime + 7, 1);
	}
	else
	{
		memcpy(szTemp, szTime + 6, 2);
	}
	int iDay = atoi(szTemp);
	return GetPointDayStartTimestamp(iYear, iMonth, iDay);
}

time_t  GlobalMethod::GetPointDayEndTimestamp(char* szTimeIn)
{
	struct tm tm_t;
	int iRt = sscanf(szTimeIn, "%d/%d/%d", &(tm_t.tm_year), &(tm_t.tm_mon), &(tm_t.tm_mday));
	if (iRt != 3)
	{
		return -1;
	}
	tm_t.tm_mon -= 1;
	tm_t.tm_year -= 1900;
	tm_t.tm_hour = 23;
	tm_t.tm_min = 59;
	tm_t.tm_sec = 59;
	time_t iTm = mktime(&tm_t);
	return iTm;
}


time_t GlobalMethod::GetTodayPointTimestamp(char* szHourMinSec)
{
	time_t theTime = time(NULL);
	struct tm* tm_t;
	tm_t = localtime(&theTime);
	int iRt = sscanf(szHourMinSec, "%d:%d:%d", &(tm_t->tm_hour), &(tm_t->tm_min), &(tm_t->tm_sec));
	if (iRt != 3)
	{
		return -1;
	}
	time_t iTm = mktime(tm_t);
	return iTm;
}

void GlobalMethod::ConvertSpeDayFlag(int iSpeDayFlag, int& iYear, int& iMonth, int& iDay)
{
	int iTempYear = iSpeDayFlag / 100000;
	int iTempYear2 = (iSpeDayFlag - iTempYear * 100000) / 10000;
	int iTempMonth = (iSpeDayFlag - iTempYear * 100000 - iTempYear2 * 10000) / 1000;
	int iTempMonth2 = (iSpeDayFlag - iTempYear * 100000 - iTempYear2 * 10000 - iTempMonth * 1000) / 100;
	int iTempDay = (iSpeDayFlag - iTempYear * 100000 - iTempYear2 * 10000 - iTempMonth * 1000 - iTempMonth2 * 100) / 10;
	int iTempDay2 = iSpeDayFlag - iTempYear * 100000 - iTempYear2 * 10000 - iTempMonth * 1000 - iTempMonth2 * 100 - iTempDay * 10;
	iYear = 2000 + iTempYear * 10 + iTempYear2;
	iMonth = iTempMonth * 10 + iTempMonth2;
	iDay = iTempDay * 10 + iTempDay2;
	//_log(_DEBUG, "DB-LAG", "ConvertSpeDayFlag: iSpeDayFlag[%d],year[%d-%d],month[%d-%d],day[%d-%d],[%d,%d,%d]", iSpeDayFlag, iTempYear, iTempYear2, iTempMonth, iTempMonth2, iTempDay, iTempDay2, iYear, iMonth, iDay);
}


time_t GlobalMethod::GetTimeFlag(char* szTime)
{
	struct tm tm_t;
	int iRt = sscanf(szTime, "%d/%d/%d %d:%d:%d", &(tm_t.tm_year), &(tm_t.tm_mon), &(tm_t.tm_mday), &(tm_t.tm_hour), &(tm_t.tm_min), &(tm_t.tm_sec));
	if (iRt != 6)
	{
		return -1;
	}
	tm_t.tm_mon -= 1;
	tm_t.tm_year -= 1900;
	time_t iTm = mktime(&tm_t);
	return iTm;
}

void GlobalMethod::ParaseParamValue(char* pValue, int iValue[], const char* pFlag)
{
	char* cToken = strtok(pValue, pFlag);
	int iIndex = 0;
	while (cToken != NULL)
	{
		iValue[iIndex] = atoi(cToken);
		cToken = strtok(NULL, pFlag);
		iIndex++;
	}
}

void GlobalMethod::ParaseParamValue(char* pValue, int iValue[], int iMaxValueCnt, const char* pFlag)
{
	char* cToken = strtok(pValue, pFlag);
	int iIndex = 0;
	while (cToken != NULL)
	{
		iValue[iIndex] = atoi(cToken);
		cToken = strtok(NULL, pFlag);
		iIndex++;
		if (iIndex >= iMaxValueCnt)
		{
			break;
		}
	}
}

void GlobalMethod::CutIntString(vector<int>& vcStrOut, string strIn, string strCut)
{
	vcStrOut.clear();

	while (strIn.length() > 0 && strIn[strIn.length() - 1] == ' ')
	{
		strIn.erase(strIn.end() - 1);
	}

	size_t iBegin = 0;
	while (true)
	{
		size_t iPos = strIn.find(strCut, iBegin);
		if (iPos == string::npos)
		{
			string strTemp = strIn.substr(iBegin);
			if (strTemp.length() > 0)  //做下判断,避免末位加了分隔符
				vcStrOut.push_back(atoi(strTemp.c_str()));
			break;
		}
		string strTemp2 = strIn.substr(iBegin, iPos - iBegin);
		vcStrOut.push_back(atoi(strTemp2.c_str()));
		iBegin = iPos + strCut.length();
	}
}


void GlobalMethod::CutString(vector<string>& vcStrOut, string strIn, string strCut)
{
	vcStrOut.clear();

	while (strIn.length() > 0 && strIn[strIn.length() - 1] == ' ')
	{
		strIn.erase(strIn.end() - 1);
	}

	size_t iBegin = 0;
	while (true)
	{
		size_t iPos = strIn.find(strCut, iBegin);
		if (iPos == string::npos)
		{
			string strTemp = strIn.substr(iBegin);
			if (strTemp.length() > 0)  //做下判断,避免末位加了分隔符
				vcStrOut.push_back(strTemp);
			break;
		}
		vcStrOut.push_back(strIn.substr(iBegin, iPos - iBegin));
		iBegin = iPos + strCut.length();
	}
}

/*消除字符串后面的空格*/
void GlobalMethod::RTrim(char* szBuffer)
{
	int i;
	for (i = strlen(szBuffer) - 1; i > 0; i--)
	{
		if (szBuffer[i] == ' ')
		{
			szBuffer[i] = 0;

		}
		else
		{
			return;
		}
	}
}

time_t GlobalMethod::ConvertTimeStamp(char* szTime)
{
	struct tm tm_t;
	int iRt = sscanf(szTime, "%d-%d-%d %d:%d:%d", &(tm_t.tm_year), &(tm_t.tm_mon), &(tm_t.tm_mday), &(tm_t.tm_hour), &(tm_t.tm_min), &(tm_t.tm_sec));
	if (iRt != 6)
	{
		return -1;
	}
	tm_t.tm_mon -= 1;
	tm_t.tm_year -= 1900;
	time_t iTm = mktime(&tm_t);
	return iTm;
}

void GlobalMethod::GetNowTimeString(char* szTime)
{
	time_t theTime = time(NULL);
	struct tm* timenow;
	timenow = localtime(&theTime);
	int year = 1900 + timenow->tm_year;
	int month = 1 + timenow->tm_mon;
	int day = timenow->tm_mday;
	int iHour = timenow->tm_hour;
	int iMinute = timenow->tm_min;
	int iSec = timenow->tm_sec;
	sprintf(szTime, "%d-%02d-%02d %2d:%2d:%2d", year, month, day, iHour, iMinute, iSec);
}

bool GlobalMethod::JudgeIfGapDay(time_t tm1, time_t tm2)
{
	int iDateFlag1 = GetDayTimeFlag(tm1);
	int iDateFlag2 = GetDayTimeFlag(tm2);
	if (iDateFlag1 == iDateFlag2)
	{
		return false;
	}
	return true;
}

int GlobalMethod::GetDayGap(time_t tm1, time_t tm2)
{
	struct tm* tm_tm1;
	tm_tm1 = localtime(&tm1);
	int iTmFlag1 = GetPointDayStartTimestamp(tm_tm1->tm_year + 1900, tm_tm1->tm_mon + 1, tm_tm1->tm_mday);

	struct tm* tm_tm2;
	tm_tm2 = localtime(&tm2);
	int iTmFlag2 = GetPointDayStartTimestamp(tm_tm2->tm_year + 1900, tm_tm2->tm_mon + 1, tm_tm2->tm_mday);

	int iDayGap = (iTmFlag2 - iTmFlag1) / (24 * 3600);
	return abs(iDayGap);
}

bool GlobalMethod::JudgeIfOpenTimeLimit(int iBeginTime, int iLongTime)
{
	bool bifLimit = false;
	time_t now_time;
	struct tm* tm_t;
	time(&now_time);
	tm_t = localtime(&now_time);
	if (iLongTime == 24)//全天开放
	{
		bifLimit = false;
	}
	else if ((iBeginTime + iLongTime) >= 24)
	{
		if ((tm_t->tm_hour < iBeginTime)
			&& (tm_t->tm_hour >= (iBeginTime + iLongTime) % 24))
		{
			bifLimit = true;
		}
	}
	else
	{
		if ((tm_t->tm_hour < iBeginTime)
			|| (tm_t->tm_hour >= (iBeginTime + iLongTime)))
		{
			bifLimit = true;
		}
	}
	return bifLimit;
}