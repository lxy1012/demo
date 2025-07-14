/*+==================================================================
* CopyRight (c) 2019 Boke
* All rights reserved
* This program is UNPUBLISHED PROPRIETARY property.
* Only for internal distribution.
*
* FileName: MySQLTableStruct.h
* 
* Purpose:  Define table query result
*
* Author:   fengjianhua@boke.com
* 
* Modify:   2022/03/01 20:07
==================================================================+*/
#ifndef _MYSQLTABLESTRUCT_H_
#define _MYSQLTABLESTRUCT_H_
#include <string>
#include <vector>
#include <map>
#include "mysql/mysql.h"

//////////////////////////////////////////////////////////////////////////
// DB table record, hold some fields
class CMySQLTableRecord
{
public:
    // Constructor
    CMySQLTableRecord();

    // Destructor
    ~CMySQLTableRecord();

    // Get next available field pointer to fill the field value
    inline bool PushBackFieldData(const std::string& strFieldName, const std::string& strFieldValue)
    {
        return m_mapFieldValue.emplace(strFieldName, strFieldValue).second;
    }

    // Get field as string
    const std::string& GetFieldAsString(const std::string& strFieldName) const;

    // Get field as int
    inline long long GetFieldAsInt64(const std::string& strFieldName) const
    {
        return static_cast<long long>(strtoul(GetFieldAsString(strFieldName).c_str(), 0, 10));
    }

    // Get field as unsigned int
    inline int GetFieldAsInt32(const std::string& strFieldName) const
    {
        return atoi(GetFieldAsString(strFieldName).c_str());
    }

    template<typename EnumType>
    inline EnumType GetFieldAsEnum(const std::string& strFieldName) const
    {
        return static_cast<EnumType>(GetFieldAsInt32(strFieldName));
    }

    // Get field count
    inline unsigned int GetFieldCount() const
    {
        return (unsigned int)m_mapFieldValue.size();
    }

    inline bool IsAvailable() const 
    {
        return !m_mapFieldValue.empty();
    }

private:
    // Key: field name
    // Value: field value
    std::map<std::string, std::string> m_mapFieldValue;
};

#define RETURN_ON_UNAVAILABLE_MYSQL_TABLE_RECORD(RET_VAL) if (!hRecord.IsAvailable()) { return RET_VAL; }

//////////////////////////////////////////////////////////////////////////
// Hold mysql query result, has some record
class CMySQLTableResult
{
public:
    // Define mysql table record
    // Constructor
    CMySQLTableResult();

    // Destructor
    ~CMySQLTableResult();

    inline void SetQueryResultList(std::vector<CMySQLTableRecord>& vecRecord)
    {
        m_vecRecord.swap(vecRecord);
    }

    // Get record list
    inline const std::vector<CMySQLTableRecord>& GetRecordList() const
    {
        return m_vecRecord;
    }

    // Get one record
    inline const CMySQLTableRecord& GetOneRecord() const
    {
        if (!m_vecRecord.empty())
        {
            return (m_vecRecord[0]);
        }
        static CMySQLTableRecord shRecord;
        return shRecord;
    }

    inline bool RecordIsEmpty() const
    {
        return m_vecRecord.empty();
    }

private:
    // Record list
    std::vector<CMySQLTableRecord> m_vecRecord;
};

#endif // _MYSQLTABLESTRUCT_H_
