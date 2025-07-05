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
#include "MySQLTableStruct.h"

CMySQLTableRecord::CMySQLTableRecord()
{

}

CMySQLTableRecord::~CMySQLTableRecord()
{

}

const std::string& CMySQLTableRecord::GetFieldAsString(const std::string& strFieldName) const
{
    auto itFind = m_mapFieldValue.find(strFieldName);
    if (itFind == m_mapFieldValue.end())
    {
        printf("Field: [%s] Not Existed\n", strFieldName.c_str());
        static std::string CONST_EMPTY_STRING;
        return CONST_EMPTY_STRING;
    }
    return itFind->second;
}

//////////////////////////////////////////////////////////////////////////
CMySQLTableResult::CMySQLTableResult()
{

}

CMySQLTableResult::~CMySQLTableResult()
{
    m_vecRecord.clear();
}
