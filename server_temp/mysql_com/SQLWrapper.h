/*+==================================================================
* CopyRight (c) 2022 Boke
* All rights reserved
* This program is UNPUBLISHED PROPRIETARY property.
* Only for internal distribution.
*
* FileName: SQLWrapper.h
* 
* Purpose:  Build SQL statement
*
* Author:   fengjianhua@boke.com
* 
* Modify:   2022/03/01 12:04
==================================================================+*/
#ifndef _SQL_WRAPPER_H_
#define _SQL_WRAPPER_H_

#include <string>
#include <map>
#include <vector>
#include <set>
#ifdef WIN32
#include <windows.h>
#endif // WIN32
#include "mysql/mysql.h"

using int64 = long long;
static_assert(8 == sizeof(int64), "int64 is not 8 bytes");

// Interface for different SQL statement
class CSQLWrapper
{
public:
    // Query type
    enum class EQueryType : int
    {
        Select = 1,
        Insert = 2,
        Delete = 3,
        Update = 4
    };

    // Relation type of query condition.
    enum class ERelationType : int
    {
        Equal = 1,
        Greater = 2,
        Less = 3,
        NotEqual = 4,
        NumIn = 5, //(xx in(a,b,c),xx should be number,added by sff)
        StringIn = 6,//(xx in(a,b,c),xx should be string,added by sff)
    };

public:
    /**
     * Description: Constructor
     * Parameter EQueryType nQueryType      [in]: query type
     * Parameter const std::string& strHeadName  [in]: table name or sp name
     * Parameter MYSQL & refMySQL           [in]: mysql handle
     */
    CSQLWrapper(EQueryType nQueryType, const std::string& strHeadName, MYSQL& refMySQL);

    // Destructor
    virtual ~CSQLWrapper(void)
    {
    }

    // Disable
    CSQLWrapper(const CSQLWrapper&) = delete;
    // Disable
    CSQLWrapper& operator=(const CSQLWrapper&) = delete;

    // Get SQL string
    inline const std::string& GetFinalSQLString()
    {
        if (!m_bEndCalled)
        {
            EndOfSQL();
            m_bEndCalled = true;
        }
        return m_strSQL;
    }

    // Get EQueryType
    inline EQueryType GetQueryType() const
    {
        return m_nQueryType;
    }

    // For insert and replace
    void SetFieldValueExpr(const std::string& strField, const std::string& strValue);
    void SetFieldValueString(const std::string& strField, const std::string& strValue, bool bPrimaryKey = false);
    void SetFieldValueNumber(const std::string& strField, int64 nValue, bool bPrimaryKey = false);
    void SetFieldValueTimestamp(const std::string& strField, time_t nTime, bool bPrimaryKey = false);
    //added by sff for update number 2022/03/14(update????????????????????)
    void AddFieldValueNumber(const std::string& strField, int64 nValue, bool bPrimaryKey = false);
	//added by zwr for update number 2023/06/05(update????????????§Þ?????)
	void SubFieldValueNumber(const std::string& strField, int64 nValue, bool bPrimaryKey = false);

    template<typename EnumType>
    void SetFieldValueEnum(const std::string& strField, EnumType nValue, bool bPrimaryKey = false)
    {
        SetFieldValueNumber(strField, static_cast<int64>(nValue), bPrimaryKey);
    }

    // For where condition, update, select using this function
    void AddQueryConditionStr(const std::string& strFieldName, const std::string& strValue, ERelationType nRelationType = CSQLWrapper::ERelationType::Equal);//nRelationType?NumIn/StirngIn?,strValue???&???????????????????select???
    void AddQueryConditionNumber(const std::string& strFieldName, int64 nValue, ERelationType nRelationType = CSQLWrapper::ERelationType::Equal);
    void AddQueryConditionTimestamp(const std::string& strFieldName, time_t nTime, ERelationType nRelationType = CSQLWrapper::ERelationType::Equal);
    void AddQueryConditionDate(const std::string& strFieldName, time_t nTime, ERelationType nRelationType = CSQLWrapper::ERelationType::Equal);

    static std::string FormatMySQLUTCTimestamp(time_t nUTCTime);
    static std::string FormatMySQLDateStr(time_t nUTCTime);

    static struct tm UTCTimeToLocalTime(time_t nTime);

protected:
    // End of SQL
    virtual void EndOfSQL() = 0;

    // Get SQL Escape string
    const char* GetSQLEscapeString(const std::string& strInput);

    // Convert relation type to SQL string
    const char* ConvertRelationType(ERelationType nRelationType) const;

protected:
    // mysql handle reference
    MYSQL& m_refMySQL;
    // sql statement
    std::string m_strSQL;
    // query type
    EQueryType m_nQueryType = EQueryType::Select;
    // table name or sp name
    std::string m_strHeadName;
    // buff of escape string
    char m_arrayEscapeBuff[4096] = { 0 };
    // end of called
    bool m_bEndCalled = false;
    //////////////////////////////////////////////////////////////////////////
    struct QueryCondition
    {
        std::string strFieldName;
        bool bValueIsString = true;
        std::string strFieldValue;
        ERelationType nRelationType = CSQLWrapper::ERelationType::Equal;
    };
    std::vector<QueryCondition> m_vecQueryCondition;
    // Key: field name
    // Value.first: field value
    // Value.second: int =0 is string, =1 is int && replace old value = 2 int and add to old value
    std::map<std::string, std::pair<std::string, int> > m_mapFieldValue;
    // Used by insert
    std::set<std::string> m_setPrimaryKey;
};

//////////////////////////////////////////////////////////////////////////
// Select SQL Wrapper
class CSQLWrapperSelect : public CSQLWrapper
{
public:
    /**
     * Description: Constructor
     * Parameter const std::string& strTableName [in]: table name
     * Parameter MYSQL & refMySQL [in]: mysql handle
     * Returns :
     */
    CSQLWrapperSelect(const std::string& strTableName, MYSQL& refMySQL)
        :CSQLWrapper(CSQLWrapper::EQueryType::Select, strTableName, refMySQL)
    {
    }

    // Only for select
    void AddQueryField(const std::string& strFieldName, bool bWrapper = true);
    inline void SetQueryResultLimit(int nNum)
    {
        m_nLimitValue = nNum;
    }

protected:
    // End of SQL
    virtual void EndOfSQL() override final;

private:
    // Limit of query result
    int m_nLimitValue = 0;
    // Only for select
    // first: field name
    // second: wrapper field by `
    std::vector< std::pair<std::string, bool> > m_vecQueryField;

    static void CutString(std::vector<std::string>& vcStrOut, std::string strIn, std::string strCut);
    static void CutIntString(std::vector<int>& vcStrOut, std::string strIn, std::string strCut);//added by sff
};

//////////////////////////////////////////////////////////////////////////
// Insert SQL wrapper
class CSQLWrapperInsert : public CSQLWrapper
{
public:
    /**
     * Description: Constructor
     * Parameter const std::string& strTableName  [in]: table name
     * Parameter MYSQL & refMySQL           [in]: mysql handle
     * Returns :
     */
    CSQLWrapperInsert(const std::string& strTableName, MYSQL& refMySQL)
        :CSQLWrapper(CSQLWrapper::EQueryType::Insert, strTableName, refMySQL)
    {
    }

    void AddOnDuplicateUpdateStr(const std::string& strField, const std::string& strValue);
    void AddOnDuplicateUpdateExpr(const std::string& strField, const std::string& strValue);
    void AddOnDuplicateUpdateNumber(const std::string& strField, int nValue);

protected:
    // End of SQL
    virtual void EndOfSQL() override final;

private:
    // for on duplicate key update
    std::map<std::string, std::string> m_mapDuplicateValue;
};

//////////////////////////////////////////////////////////////////////////
// Update SQL wrapper
class CSQLWrapperUpdate : public CSQLWrapper
{
public:
    /**
     * Description: Constructor
     * Parameter const std::string& strTableName  [in]: table name
     * Parameter MYSQL & refMySQL           [in]: mysql handle
     * Returns :
     */
    CSQLWrapperUpdate(const std::string& strTableName, MYSQL& refMySQL)
        :CSQLWrapper(CSQLWrapper::EQueryType::Update, strTableName, refMySQL)
    {
    }

protected:
    // End of SQL
    virtual void EndOfSQL() override final;

};

#endif // End of _SQL_WRAPPER_H_