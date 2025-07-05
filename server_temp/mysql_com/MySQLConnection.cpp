/*+==================================================================
* CopyRight (c) 2019 icerlion
* All rights reserved
* This program is UNPUBLISHED PROPRIETARY property.
* Only for internal distribution.
*
* FileName: MySQLConnection.cpp
*
* Purpose:  Implement CMySQLConnection::
*
* Author:   JhonFrank(icerlion@163.com)
*
* Modify:   2013/12/14 12:06
==================================================================+*/
#include "MySQLConnection.h"
#include <cstring>
#include <time.h>

EMutex CMySQLConnection::ms_mutexInitMySQL;

CMySQLConnection::CMySQLConnection()
{
    memset(m_chSafeBoundary, 1, sizeof(m_chSafeBoundary));
}

CMySQLConnection::~CMySQLConnection()
{
    CloseConnection();
}

bool CMySQLConnection::RunSQL(const std::string& strSQL, CMySQLTableResult& hMySQLTableResult, bool bWriteFlag /*= false*/)
{
    // 1. Check connection
    m_nInsertId = 0;
    m_nAffectedRows = 0;
    int nRetCode = mysql_ping(&m_hMySQL);
    if (0 != nRetCode)
    {
        // 2. If the connection is error, just re-init the connection
        printf("%s: Failed on mysql_ping, res: [%d], msg: [%s]\n", m_strConnectionName.c_str(), nRetCode, mysql_error(&m_hMySQL));
        if (!OpenConnection())
        {
            printf("%s: Failed to connect mysql, sql: [%s]\n", m_strConnectionName.c_str(), strSQL.c_str());
            return false;
        }
    }
    // 3. query mysql 
    nRetCode = mysql_real_query(&m_hMySQL, strSQL.c_str(), (unsigned long)strSQL.length());
    time_t nCurTimestamp = time(nullptr);
    m_nRunSQLTimestamp = nCurTimestamp;
    if (nCurTimestamp != m_nSQLStatTime)
    {
        //printf("MySQLConnection,%s,RunSQLSpeed,%d\n", m_strConnectionName.c_str(), m_nRunSQLSpeed);
        m_nRunSQLSpeed = 0;
        m_nSQLStatTime = nCurTimestamp;
    }
    ++m_nRunSQLSpeed;
    // Output all SQL for DBA
    if (bWriteFlag)
    {
        printf("SQLScript,%s,%s\n", m_strConnectionName.c_str(), strSQL.c_str());
    }
    if (0 != nRetCode)
    {
        printf("%s: Failed on mysql_query, res: [%d], msg: [%s], sql: [%s]\n", m_strConnectionName.c_str(), nRetCode, mysql_error(&m_hMySQL), strSQL.c_str());
        return false;
    }
    // 4. get result
    m_nAffectedRows = (int)mysql_affected_rows(&m_hMySQL);
    m_nInsertId = mysql_insert_id(&m_hMySQL);

    // 5. get query result
    MYSQL_RES* pMySqlResultSet = mysql_store_result(&m_hMySQL);
    bool bResult = true;
    if (nullptr != pMySqlResultSet)
    {
        // 6. get number or rows
        auto nNumRows = mysql_num_rows(pMySqlResultSet);
        // 7. get fields number
        int nNumFields = mysql_num_fields(pMySqlResultSet);
        // 8. Set field name
        MYSQL_FIELD* pMySQLFields = mysql_fetch_fields(pMySqlResultSet);
        if (nullptr == pMySQLFields)
        {
            printf("%s: SQL: [%s], nullptr == pMySQLFields\n", m_strConnectionName.c_str(), strSQL.c_str());
            return false;
        }
        // 9. Output mysql result
        std::vector<CMySQLTableRecord> vecMySQLTableRecord;
        vecMySQLTableRecord.reserve(nNumRows);
        for (MYSQL_ROW row = mysql_fetch_row(pMySqlResultSet); row != nullptr; row = mysql_fetch_row(pMySqlResultSet))
        {
            // 8.2 Reset the current record
            CMySQLTableRecord hMySQLTableRecord;
            for (int nFieldIndex = 0; nFieldIndex < nNumFields; ++nFieldIndex)
            {
                // 8.3 Init a field to store the data from mysql
                const MYSQL_FIELD& stCurField = pMySQLFields[nFieldIndex];
                const char* pValue = row[nFieldIndex];
                std::string strFieldName;
                std::string strFieldValue;
                if (nullptr != stCurField.name)
                {
                    strFieldName = stCurField.name;
                }
                if (nullptr != pValue)
                {
                    strFieldValue = pValue;
                }
                // 8.5 Assign the data to field, return false if failed, max buff length was 512 bytes
                if (!hMySQLTableRecord.PushBackFieldData(strFieldName, strFieldValue))
                {
                    printf("%s: Failed on hMySQLTableRecord.PushBackFieldData, sql: [%s]\n", m_strConnectionName.c_str(), strSQL.c_str());
                    bResult = false;
                    break;
                }
            }
            vecMySQLTableRecord.push_back(hMySQLTableRecord);
            if (!bResult)
            {
                break;
            }
        }
        hMySQLTableResult.SetQueryResultList(vecMySQLTableRecord);
        // 9. free result
        mysql_free_result(pMySqlResultSet);
    }
    // 10. Free the other result set
    for (; ;)
    {
        // more results? -1 = no, >0 = error, 0 = yes (keep looping)
        nRetCode = mysql_next_result(&m_hMySQL);
        if (-1 == nRetCode)
        {
            break;
        }
        if (nRetCode > 0)
        {
            printf("%s: Failed on mysql_next_result, res: [%d], msg: [%s], sql: [%s]\n", m_strConnectionName.c_str(), nRetCode, mysql_error(&m_hMySQL), strSQL.c_str());
            bResult = false;
            break;
        }
        MYSQL_RES* pMySQLRes = mysql_store_result(&m_hMySQL);
        if (nullptr != pMySQLRes)
        {
            mysql_free_result(pMySQLRes);
        }
    }
    return bResult;
}

void CMySQLConnection::SetConnectionConfig(const MySQLConfig& stMySQLConfig)
{
    m_stMySQLConfig = stMySQLConfig;
}

bool CMySQLConnection::OpenConnection()
{
    //1. init mysql, mysql_init is not thread-safe
    if (!CallMySQLInit())
    {
        printf("%s: MySQLConnectionFailed: [%s] when call mysql_init, msg: [%s]\n", m_strConnectionName.c_str(), m_stMySQLConfig.strSafeConnStr.c_str(), mysql_error(&m_hMySQL));
        return false;
    }
    CheckSafeBoundary();
    // 2. set mysql options
    const static unsigned int nTimeOut = 60; //seconds of six months
    if (0 != mysql_options(&m_hMySQL, MYSQL_OPT_CONNECT_TIMEOUT, &nTimeOut))
    {
        printf("%s: MySQLConnectionFailed: [%s] when call mysql_options.MYSQL_OPT_CONNECT_TIMEOUT, msg: [%s]\n", m_strConnectionName.c_str(), m_stMySQLConfig.strSafeConnStr.c_str(), mysql_error(&m_hMySQL));
        return false;
    }
    const static unsigned int nReConnect = 1;   // enable re-connect
    if (0 != mysql_options(&m_hMySQL, MYSQL_OPT_RECONNECT, &nReConnect))
    {
        printf("%s: MySQLConnectionFailed: [%s] when call mysql_options.MYSQL_OPT_RECONNECT, msg: [%s]\n", m_strConnectionName.c_str(), m_stMySQLConfig.strSafeConnStr.c_str(), mysql_error(&m_hMySQL));
        return false;
    }
    // 3. real connect to mysql
    if (nullptr == mysql_real_connect(&m_hMySQL, m_stMySQLConfig.strHost.c_str(), m_stMySQLConfig.strUser.c_str(), m_stMySQLConfig.strPassword.c_str(), m_stMySQLConfig.strDBName.c_str(), m_stMySQLConfig.nPort, nullptr, CLIENT_MULTI_STATEMENTS))
    {
        printf("%s: MySQLConnectionFailed: [%s] when call mysql_real_connect, msg: [%s]\n", m_strConnectionName.c_str(), m_stMySQLConfig.strSafeConnStr.c_str(), mysql_error(&m_hMySQL));
        return false;
    }
    // 4. set character set
    if (0 != mysql_set_character_set(&m_hMySQL, "utf8"))
    {
        printf("%s: MySQLConnectionFailed: [%s] when call mysql_set_character_set, msg: [%s]\n", m_strConnectionName.c_str(), m_stMySQLConfig.strSafeConnStr.c_str(), mysql_error(&m_hMySQL));
        return false;
    }
    m_nRunSQLTimestamp = time(nullptr);
    printf("%s: InitMySQLConnection: [%s]", m_strConnectionName.c_str(), m_stMySQLConfig.strSafeConnStr.c_str());
    return true;
}


void CMySQLConnection::CloseConnection()
{
    mysql_close(&m_hMySQL);
}

bool CMySQLConnection::ProbeMySQLConnection(const MySQLConfig& stMySQLConfig)
{
    CMySQLConnection hMySQLConnection;
    hMySQLConnection.SetConnectionName("ProbeMySQL");
    hMySQLConnection.SetConnectionConfig(stMySQLConfig);
    if (!hMySQLConnection.OpenConnection())
    {
        return false;
    }
    CMySQLTableResult hMySQLTableResult;
    hMySQLConnection.RunSQL("SELECT 1 FROM DUAL", hMySQLTableResult);
    const CMySQLTableRecord& hMySQLTableRecord = hMySQLTableResult.GetOneRecord();
    if (!hMySQLTableRecord.IsAvailable())
    {
        return false;
    }
    if (1 != hMySQLTableRecord.GetFieldAsInt32("1"))
    {
        return false;
    }
    hMySQLConnection.CloseConnection();
    return true;
}

bool CMySQLConnection::CheckSafeBoundary()
{
    bool bResult = true;
    int nIndex = 0;
    for (char ch : m_chSafeBoundary)
    {
        if (1 != ch)
        {
            printf("%s: Index: [%d], Safe Boundary Is Breaken\n", m_strConnectionName.c_str(), nIndex);
            bResult = false;
            break;
        }
        ++nIndex;
    }
    return bResult;
}

bool CMySQLConnection::CallMySQLInit()
{
    ms_mutexInitMySQL.lock();
    MYSQL* pRt = mysql_init(&m_hMySQL);
    ms_mutexInitMySQL.unlock();
    return nullptr != pRt;
}
