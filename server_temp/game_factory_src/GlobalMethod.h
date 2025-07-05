#ifndef __GLOBAL_METHOD__
#define __GLOBAL_METHOD__

#include <vector>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

class GlobalMethod
{
	
public:
	GlobalMethod();
	~GlobalMethod();

	static int code_convert(char *from_charset,char *to_charset,char *inbuf,size_t inlen,char *outbuf,size_t outlen);//代码转换:从一种编码转为另一种编码

	static int GetDayTimeFlag(const time_t& theTime);//返回20190109
	static int GetDayTimeFlag(char* szTime);//将MM/DD/YYYY hh24:mi:ss格式的时间转换成年月日整形值20190109
	static void GetTodayTimeString(char* szTime);//"yyyy-MM-dd"格式
	static int GetTodayLeftSeconds();//获取今天剩余时间
	static time_t GetTodayStartTimestamp();//获取今日凌晨时间戳
	static time_t GetWeekStartTimestamp();//获取本周凌晨时间戳
	static time_t GetNowHourStartTimeStamp();//获取当前整点时间戳
	static time_t GetPointDayStartTimestamp(char* szTimeIn);//获取"yyyy/MM/dd"格式日期的当日凌晨时间戳
	static time_t GetPointDayStartTimestamp(int iYear, int iMonth, int iDay);//获取"yyyy/MM/dd"格式日期的当日凌晨时间戳
	static time_t GetPointDayStartTimestamp(int iDateFlag);//获取形如"20190109"格式日期的当日凌晨时间戳
	static time_t GetPointDayEndTimestamp(char* szTimeIn);//获取"yyyy/MM/dd"格式日期的当日结束时间戳
	static time_t GetTodayPointTimestamp(char* szHourMinSec);//获取今日指定时分秒"hh:mm:ss"的时间戳

	static void ConvertSpeDayFlag(int iSpeDayFlag, int& iYear, int& iMonth, int& iDay);//iSpeDayFlag格式位210821表示2021年8月21
	static time_t GetTimeFlag(char* szTime);//获取MM/DD/YYYY hh24:mi:ss格式的整形时间戳

	static void CutString(vector<string>& vcStrOut, string strIn, string strCut);
	static void CutIntString(vector<int>& vcStrOut, string strIn, string strCut);
	static void ParaseParamValue(char* pValue, int iValue[], const char* pFlag);
	static void ParaseParamValue(char* pValue, int iValue[],int iMaxValueCnt, const char* pFlag);

	static void RTrim(char* szBuffer);

	static time_t ConvertTimeStamp(char* szTime);//将MM-DD-YYYY hh24:mi:ss格式的时间转成时间戳
	static void GetNowTimeString(char* szTime);

	static bool JudgeIfGapDay(time_t tm1, time_t tm2);//判断2个时间戳是否是跨天（true跨天，false同一天)
	static int GetDayGap(time_t tm1, time_t tm2);

	static bool JudgeIfOpenTimeLimit(int iBeginTime,int iLongTime);
};


#endif //__ANNIVERSARY_LOTTERY__
