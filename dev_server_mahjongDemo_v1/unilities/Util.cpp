#include "Util.h"
#include <stdio.h>
#include <stdlib.h>
#include <iconv.h>
#include <string.h>
#include "log.h"

int Util::GetDayTimeFlag(const time_t& theTime)
{
	struct tm* timenow;
	timenow = localtime(&theTime);
	int year = 1900 + timenow->tm_year;
	int month = 1 + timenow->tm_mon;
	int day = timenow->tm_mday;
	return year * 10000 + month * 100 + day;
}

int Util::GetDayTimeFlag(char* szTime)
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

void Util::GetTodayTimeString(char* szTime)
{
	time_t theTime = time(NULL);
	struct tm* timenow;
	timenow = localtime(&theTime);
	int year = 1900 + timenow->tm_year;
	int month = 1 + timenow->tm_mon;
	int day = timenow->tm_mday;
	sprintf(szTime, "%d-%02d-%02d", year, month, day);
}

int Util::GetTodayLeftSeconds()
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

int Util::code_convert(char *from_charset, char *to_charset, char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
	iconv_t cd;
	int rc;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open(to_charset, from_charset);
	if (cd == 0) return -1;
	memset(outbuf, 0, outlen);
	if (iconv(cd, pin, &inlen, pout, &outlen) == -1)
	{
		iconv_close(cd);
		return -1;
	}
	iconv_close(cd);
	return 0;
}

time_t Util::GetTodayStartTimestamp()
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

time_t Util::GetWeekStartTimestamp()
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

time_t Util::GetNowHourStartTimeStamp()
{
	time_t theTime = time(NULL);
	struct tm* tm_t;
	tm_t = localtime(&theTime);
	tm_t->tm_min = 0;
	tm_t->tm_sec = 0;
	time_t iEndTm = mktime(tm_t);
	return iEndTm;
}

time_t  Util::GetPointDayStartTimestamp(char* szTimeIn)
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

time_t Util::GetPointDayStartTimestamp(int iYear, int iMonth, int iDay)
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

time_t Util::GetPointDayStartTimestamp(int iDateFlag)
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

time_t  Util::GetPointDayEndTimestamp(char* szTimeIn)
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

time_t Util::GetTodayPointTimestamp(char* szHourMinSec)
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

void Util::ConvertSpeDayFlag(int iSpeDayFlag, int& iYear, int& iMonth, int& iDay)
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


time_t Util::GetTimeFlag(char* szTime)
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

void Util::ParaseParamValue(char* pValue, int iVale[], const char* pFlag)
{
	char* cToken = strtok(pValue, pFlag);
	int iIndex = 0;
	while (cToken != NULL)
	{
		iVale[iIndex] = atoi(cToken);
		cToken = strtok(NULL, pFlag);
		iIndex++;
	}
}


void Util::CutIntString(vector<int>& vcStrOut, string strIn, string strCut)
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


void Util::CutString(vector<string>& vcStrOut, string strIn, string strCut)
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
void Util::RTrim(char *szBuffer)
{
	int i;
	for (i = strlen(szBuffer) - 1; i>0; i--)
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

time_t Util::ConvertTimeStamp(char* szTime)
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

void Util::GetNowTimeString(char* szTime)
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
	sprintf(szTime, "%d-%02d-%02d %02d:%02d:%02d", year, month, day, iHour, iMinute, iSec);
}

bool Util::JudgeIfGapDay(time_t tm1, time_t tm2)
{
	int iDateFlag1 = GetDayTimeFlag(tm1);
	int iDateFlag2 = GetDayTimeFlag(tm2);
	if (iDateFlag1 == iDateFlag2)
	{
		return false;
	}
	return true;
}

int Util::GetDayGap(time_t tm1, time_t tm2)
{
	struct tm* tm_tm1;
	tm_tm1 = localtime(&tm1);
	int iTmFlag1 = Util::GetPointDayStartTimestamp(tm_tm1->tm_year + 1900, tm_tm1->tm_mon + 1, tm_tm1->tm_mday);

	struct tm* tm_tm2;
	tm_tm2 = localtime(&tm2);
	int iTmFlag2 = Util::GetPointDayStartTimestamp(tm_tm2->tm_year + 1900, tm_tm2->tm_mon + 1, tm_tm2->tm_mday);

	int iDayGap = (iTmFlag2 - iTmFlag1) / (24 * 3600);
	return abs(iDayGap);
}