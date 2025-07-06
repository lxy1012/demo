#ifndef _DB_COMMON_DATE_H_ 
#define _DB_COMMON_DATE_H_ 


#include "time.h"

long long GetTimeNowUs();	//获取当前微秒数

class DBDate
{
public:
	static DBDate GetDate(int iYear,int iMonth,int iDay,int iHour,int iMin,int iSec);
	static DBDate GetDate(char *strDate);	//注意，格式必须为"YYYY/MM/DD hh24:mi:ss"
	static DBDate GetDate(time_t ttDate);
	
	int getYear(){return m_iYear;};
	int getMonth(){return m_iMonth;};
	int getDay(){return m_iDay;};
	int getHour(){return m_iHour;};
	int getMin(){return m_iMin;};
	int getSec(){return m_iSec;};
	void setInt(int iYear,int iMonth,int iDay,int iHour,int iMin,int iSec);
	
	char* getStr(){return m_strDate;}
	void setStr(char *strDate);
	
	time_t getTT(){return m_ttDate;}
	void setTT(time_t ttDate);
	
	DBDate(); 
  virtual ~DBDate(){}; 
private:
	int m_iYear;
	int m_iMonth;
	int m_iDay;
	int m_iHour;
	int m_iMin;
	int m_iSec;
	char m_strDate[32];
	time_t m_ttDate;
	
};


#endif  