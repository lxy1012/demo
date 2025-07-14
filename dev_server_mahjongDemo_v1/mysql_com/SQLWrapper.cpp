/*+==================================================================
* CopyRight (c) 2022 Boke
* All rights reserved
* This program is UNPUBLISHED PROPRIETARY property.
* Only for internal distribution.
*
* FileName: SQLWrapper.cpp
*
* Purpose:  Build SQL statement
*
* Author:   fengjianhua@boke.com
*
* Modify:   2022/03/01 12:04
==================================================================+*/
#include "SQLWrapper.h"
#include <cstring>
#include <time.h>

#ifdef WIN32
#define a_sprintf sprintf_s
#else
#define a_sprintf sprintf
#endif

const char* CSQLWrapper::ConvertRelationType(ERelationType nRelationType) const
{
	const char* pResult = nullptr;
	switch (nRelationType)
	{
	case ERelationType::Equal:
		pResult = "=";
		break;
	case ERelationType::Greater:
		pResult = ">";
		break;
	case ERelationType::Less:
		pResult = "<";
		break;
	case ERelationType::NumIn:
	case ERelationType::StringIn:
		pResult = "in (";
		break;
	default:
		pResult = "!=";
		break;
	}
	return pResult;
}

CSQLWrapper::CSQLWrapper(EQueryType nQueryType, const std::string& strHeadName, MYSQL& refMySQL)
	:m_refMySQL(refMySQL),
	m_nQueryType(nQueryType),
	m_strHeadName(strHeadName)
{
	memset(m_arrayEscapeBuff, 0, sizeof(m_arrayEscapeBuff));
}

void CSQLWrapper::SetFieldValueExpr(const std::string& strField, const std::string& strValue)
{
	m_mapFieldValue.emplace(strField, std::make_pair(strValue, 0));
}

void CSQLWrapper::SetFieldValueString(const std::string& strField, const std::string& strValue, bool bPrimaryKey/* = false*/)
{
    m_mapFieldValue.emplace(strField, std::make_pair(GetSQLEscapeString(strValue), 3));
    if (bPrimaryKey)
    {
        m_setPrimaryKey.emplace(strField);
    }
}

void CSQLWrapper::SetFieldValueNumber(const std::string& strField, int64 nValue, bool bPrimaryKey/* = false*/)
{
	m_mapFieldValue.emplace(strField, std::make_pair(std::to_string(nValue), 1));
	if (bPrimaryKey)
	{
		m_setPrimaryKey.emplace(strField);
	}
}

void CSQLWrapper::SetFieldValueTimestamp(const std::string& strField, time_t nTime, bool bPrimaryKey/* = false*/)
{
	m_mapFieldValue.emplace(strField, std::make_pair(FormatMySQLUTCTimestamp(nTime), 3));
	if (bPrimaryKey)
	{
		m_setPrimaryKey.emplace(strField);
	}
}

void CSQLWrapper::AddFieldValueNumber(const std::string& strField, int64 nValue, bool bPrimaryKey/* = false*/)
{
	m_mapFieldValue.emplace(strField, std::make_pair(std::to_string(nValue), 2));
	if (bPrimaryKey)
	{
		m_setPrimaryKey.emplace(strField);
	}
}

void CSQLWrapper::SubFieldValueNumber(const std::string& strField, int64 nValue, bool bPrimaryKey/* = false*/)
{
	m_mapFieldValue.emplace(strField, std::make_pair(std::to_string(nValue), 4));
	if (bPrimaryKey)
	{
		m_setPrimaryKey.emplace(strField);
	}
}

void CSQLWrapper::AddQueryConditionStr(const std::string& strFieldName, const std::string& strValue, ERelationType nRelationType /*= CSQLWrapper::ERelationType::Equal*/)
{
	QueryCondition stQueryCondition;
	stQueryCondition.strFieldName = strFieldName;
	stQueryCondition.strFieldValue = GetSQLEscapeString(strValue);
	stQueryCondition.bValueIsString = true;
	stQueryCondition.nRelationType = nRelationType;
	m_vecQueryCondition.emplace_back(stQueryCondition);
}

void CSQLWrapper::AddQueryConditionNumber(const std::string& strFieldName, int64 nValue, ERelationType nRelationType /*= ERelationType::Equal*/)
{
	QueryCondition stQueryCondition;
	stQueryCondition.strFieldName = strFieldName;
	stQueryCondition.strFieldValue = std::to_string(nValue);
	stQueryCondition.bValueIsString = false;
	stQueryCondition.nRelationType = nRelationType;
	m_vecQueryCondition.emplace_back(stQueryCondition);
}

void CSQLWrapper::AddQueryConditionTimestamp(const std::string& strFieldName, time_t nTime, ERelationType nRelationType /*= ERelationType::Equal*/)
{
	QueryCondition stQueryCondition;
	stQueryCondition.strFieldName = strFieldName;
	stQueryCondition.strFieldValue = FormatMySQLUTCTimestamp(nTime);
	stQueryCondition.bValueIsString = true;
	stQueryCondition.nRelationType = nRelationType;
	m_vecQueryCondition.emplace_back(stQueryCondition);
}

void CSQLWrapper::AddQueryConditionDate(const std::string& strFieldName, time_t nTime, ERelationType nRelationType /*= ERelationType::Equal*/)
{
	QueryCondition stQueryCondition;
	stQueryCondition.strFieldName = strFieldName;
	stQueryCondition.strFieldValue = FormatMySQLDateStr(nTime);
	stQueryCondition.bValueIsString = true;
	stQueryCondition.nRelationType = nRelationType;
	m_vecQueryCondition.emplace_back(stQueryCondition);
}

std::string CSQLWrapper::FormatMySQLUTCTimestamp(time_t nUTCTime)
{
	time_t nFixedTime = nUTCTime;
	// Ref: http://dev.mysql.com/doc/refman/5.6/en/datetime.html
	const static int CONST_MYSQL_TIMESTAMP_MIN_VALUE = 1;                // 1970-01-01 00:00:01, GMT+0
	const static int CONST_MYSQL_TIMESTAMP_MAX_VALUE = 0x7fffffff - 10;  // 2038-01-19 03:14:07, GMT+0
	if (nUTCTime < CONST_MYSQL_TIMESTAMP_MIN_VALUE)
	{
		nFixedTime = CONST_MYSQL_TIMESTAMP_MIN_VALUE;
	}
	else if (nUTCTime > CONST_MYSQL_TIMESTAMP_MAX_VALUE)
	{
		nFixedTime = CONST_MYSQL_TIMESTAMP_MAX_VALUE;
	}
	struct tm stTM = UTCTimeToLocalTime(nFixedTime);
	const char* TIME_FORMAT = "%04d-%02d-%02d %02d:%02d:%02d";
	char buff[32] = { 0 };
	a_sprintf(buff, TIME_FORMAT,
		stTM.tm_year + 1900,
		stTM.tm_mon + 1,
		stTM.tm_mday,
		stTM.tm_hour,
		stTM.tm_min,
		stTM.tm_sec);
	return std::string(buff);
}

std::string CSQLWrapper::FormatMySQLDateStr(time_t nUTCTime)
{
	struct tm stTM = UTCTimeToLocalTime(nUTCTime);
	const char* TIME_FORMAT = "%04d-%02d-%02d";
	char buff[32] = { 0 };
	a_sprintf(buff, TIME_FORMAT,
		stTM.tm_year + 1900,
		stTM.tm_mon + 1,
		stTM.tm_mday,
		stTM.tm_hour,
		stTM.tm_min,
		stTM.tm_sec);
	return std::string(buff);
}

struct tm CSQLWrapper::UTCTimeToLocalTime(time_t nTime)
{
	struct tm stResult;
#ifdef LINUX
	struct tm* pTM = localtime(&nTime);
	if (nullptr != pTM)
	{
		stResult = *pTM;
	}
#else
	localtime_s(&stResult, &nTime);
#endif
	return stResult;
}

const char* CSQLWrapper::GetSQLEscapeString(const std::string& strInput)
{
	size_t nInputLen = strInput.length();
	if (0 == nInputLen)
	{
		return "";
	}
	if (nInputLen >= sizeof(m_arrayEscapeBuff))
	{
		printf("Error, Impossible nInputLen %zu buff buff len: %zu\n", nInputLen, sizeof(m_arrayEscapeBuff));
		return "NA";
	}
	memset(m_arrayEscapeBuff, 0, sizeof(m_arrayEscapeBuff));
	mysql_real_escape_string(&m_refMySQL, m_arrayEscapeBuff, strInput.c_str(), static_cast<int>(nInputLen));
	return m_arrayEscapeBuff;
}

void CSQLWrapperSelect::AddQueryField(const std::string& strFieldName, bool bWrapper /*= true*/)
{
	m_vecQueryField.emplace_back(std::make_pair(strFieldName, bWrapper));
}

//////////////////////////////////////////////////////////////////////////
void CSQLWrapperSelect::EndOfSQL()
{
	m_strSQL.append("select ");
	for (auto& stQueryField : m_vecQueryField)
	{
		if (stQueryField.second)
		{
			m_strSQL.append("`").append(stQueryField.first).append("`, ");
		}
		else
		{
			m_strSQL.append("").append(stQueryField.first).append(", ");
		}
	}
	// Pop last " ,";
	m_strSQL.pop_back();
	m_strSQL.pop_back();
	// Append table name
	m_strSQL.append(" from `").append(m_strHeadName).append("` ");
	if (!m_vecQueryCondition.empty())
	{
		m_strSQL.append("where ");
		for (auto& stCondition : m_vecQueryCondition)
		{
			m_strSQL.append("`").append(stCondition.strFieldName).append("` ").append(ConvertRelationType(stCondition.nRelationType)).append(" ");
			if (stCondition.nRelationType == ERelationType::NumIn)
			{
				std::vector<int> vcNumber;
				CutIntString(vcNumber, stCondition.strFieldValue, "&");
				for (auto& iNum : vcNumber)
				{
					m_strSQL.append(std::to_string(iNum)).append(", ");
				}
				// Pop last ", "
				m_strSQL.erase(m_strSQL.size() - 2, 2);
				m_strSQL.append(" ) and ");
			}
			else if (stCondition.nRelationType == ERelationType::StringIn)
			{
				std::vector<std::string> vcValue;
				CutString(vcValue, stCondition.strFieldValue, "&");
				for (auto& strValue : vcValue)
				{
					m_strSQL.append("'").append(strValue).append("'").append(", ");
				}
				// Pop last ", "
				m_strSQL.erase(m_strSQL.size() - 2, 2);
				m_strSQL.append(" ) and ");
			}
			else
			{
				if (stCondition.bValueIsString)
				{
					m_strSQL.append("'").append(stCondition.strFieldValue).append("'");
				}
				else
				{
					m_strSQL.append(stCondition.strFieldValue);
				}

				m_strSQL.append(" and ");
			}
		}
		// Pop last " and "
		m_strSQL.erase(m_strSQL.size() - 5, 5);
	}
	if (0 != m_nLimitValue)
	{
		m_strSQL.append(" limit ").append(std::to_string(m_nLimitValue));
	}
	return;
}

void CSQLWrapperSelect::CutString(std::vector<std::string>& vcStrOut, std::string strIn, std::string strCut)
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
		if (iPos == std::string::npos)
		{
			std::string strTemp = strIn.substr(iBegin);
			if (strTemp.length() > 0)  //做下判断,避免末位加了分隔符
				vcStrOut.push_back(strTemp);
			break;
		}
		vcStrOut.push_back(strIn.substr(iBegin, iPos - iBegin));
		iBegin = iPos + strCut.length();
	}
}

void CSQLWrapperSelect::CutIntString(std::vector<int>& vcStrOut, std::string strIn, std::string strCut)
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
		if (iPos == std::string::npos)
		{
			std::string strTemp = strIn.substr(iBegin);
			if (strTemp.length() > 0)  //做下判断,避免末位加了分隔符
			{
				vcStrOut.push_back(atoi(strTemp.c_str()));
			}
			break;
		}
		std::string strTemp2 = strIn.substr(iBegin, iPos - iBegin);
		vcStrOut.push_back(atoi(strTemp2.c_str()));
		iBegin = iPos + strCut.length();
	}
}
//////////////////////////////////////////////////////////////////////////
void CSQLWrapperInsert::AddOnDuplicateUpdateStr(const std::string& strField, const std::string& strValue)
{
	const char* pszSafeValue = GetSQLEscapeString(strValue);
	std::string strSafeValue;
	strSafeValue.append(1, '"').append(pszSafeValue).append(1, '"');
	m_mapDuplicateValue.emplace(strField, strSafeValue);
}

void CSQLWrapperInsert::AddOnDuplicateUpdateExpr(const std::string& strField, const std::string& strValue)
{
	m_mapDuplicateValue.emplace(strField, strValue);
}

void CSQLWrapperInsert::AddOnDuplicateUpdateNumber(const std::string& strField, int nValue)
{
	m_mapDuplicateValue.emplace(strField, std::to_string(nValue));
}

void CSQLWrapperInsert::EndOfSQL()
{
	m_strSQL.append("insert into `").append(m_strHeadName).append("`(");
	std::string strValueList;
	strValueList.append("values(");
	for (auto& kvp : m_mapFieldValue)
	{
		m_strSQL.append("`");
		m_strSQL.append(kvp.first);
		m_strSQL.append("`,");
		const std::pair<std::string, int>& pairFieldValue = kvp.second;
		if (pairFieldValue.second == 0)
		{
			strValueList.append(pairFieldValue.first);
		}
		else
		{
			strValueList.append("'");
			strValueList.append(pairFieldValue.first);
			strValueList.append("'");
		}
		strValueList.append(", ");
	}
	m_strSQL.pop_back();
	m_strSQL.append(") ");
	// Pop last ", "
	strValueList.pop_back();
	strValueList.pop_back();
	strValueList.append(")");
	m_strSQL.append(strValueList);
	if (m_mapDuplicateValue.empty() && m_setPrimaryKey.empty())
	{
		return;
	}
	// handle on duplicate key update
	m_strSQL.append(" on duplicate key update ");
	if (!m_mapDuplicateValue.empty())
	{
		for (auto& kvp : m_mapDuplicateValue)
		{
			m_strSQL.append("`").append(kvp.first).append("`").append("=").append(kvp.second).append(",");
		}
	}
	else
	{
		for (auto& kvp : m_mapFieldValue)
		{
			if (0 == m_setPrimaryKey.count(kvp.first))
			{
				m_strSQL.append("`").append(kvp.first).append("`").append("=");
				const std::pair<std::string, bool>& pairFieldValue = kvp.second;
				if (pairFieldValue.second)
				{
					m_strSQL.append("'");
					m_strSQL.append(pairFieldValue.first);
					m_strSQL.append("',");
				}
				else
				{
					m_strSQL.append(pairFieldValue.first).append(",");
				}
			}
		}
	}
	m_strSQL.pop_back();
}

//////////////////////////////////////////////////////////////////////////
void CSQLWrapperUpdate::EndOfSQL()
{
	m_strSQL.append("update `").append(m_strHeadName).append("` set ");
	for (auto& kvp : m_mapFieldValue)
	{
		const std::string& strFieldName = kvp.first;
		m_strSQL.append("`").append(strFieldName).append("` = ");
		const std::string& strFieldValue = kvp.second.first;
		int iValueType = kvp.second.second;
		if (iValueType == 0)
		{
			m_strSQL.append(strFieldValue);
		}
		else if (iValueType == 1)
		{
			m_strSQL.append(strFieldValue);
		}
		else  if (iValueType == 2) //增量式
		{
			m_strSQL.append("`").append(strFieldName).append("` + ").append(strFieldValue);
		}
		else  if (iValueType == 3) //字符串
		{
			m_strSQL.append("'").append(strFieldValue).append("'");
		}
		else  if (iValueType == 4) //减量式...增量式不能使用负数
		{
			m_strSQL.append("`").append(strFieldName).append("` - ").append(strFieldValue);
		}
		m_strSQL.append(", ");
	}
	// Pop up last ", "
	m_strSQL.pop_back();
	m_strSQL.pop_back();
	// Proc where condition
	m_strSQL.append(" where ");
	for (auto& stCondition : m_vecQueryCondition)
	{
		m_strSQL.append("`").append(stCondition.strFieldName).append("` ").append(ConvertRelationType(stCondition.nRelationType)).append(" ");
		if (stCondition.bValueIsString)
		{
			m_strSQL.append("'").append(stCondition.strFieldValue).append("' and");
		}
		else
		{
			m_strSQL.append(stCondition.strFieldValue).append(" and");
		}
	}
	// Pop last " and"
	m_strSQL.erase(m_strSQL.size() - 4, 4);
	return;
}