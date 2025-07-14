/*+==================================================================
* CopyRight (c) 2019 icerlion
* All rights reserved
* This program is UNPUBLISHED PROPRIETARY property.
* Only for internal distribution.
*
* FileName: MySQLConnection.h
*
* Purpose:  Implement mysql connection
*
* Author:   JhonFrank(icerlion@163.com)
*
* Modify:   2013/12/14 15:04
==================================================================+*/
#ifndef _MYSQL_CONNECTION_H_
#define _MYSQL_CONNECTION_H_
#include <string>
#include "MySQLTableStruct.h"
#ifdef WIN32
#include <Windows.h>
#endif // WIN32
#include "emutex.h"

//////////////////////////////////////////////////////////////////////////
// MySQLConfig
struct MySQLConfig
{
    std::string strHost;
    int nPort = 3306;
    std::string strUser;
    std::string strPassword;
    std::string strDBName;
    std::string strSafeConnStr;
};

//////////////////////////////////////////////////////////////////////////
// MySQL connection
class CMySQLConnection
{
public:
    // Constructor
    CMySQLConnection();

    // Destructor
    ~CMySQLConnection();

    // Run SQL
    inline bool RunSQL(const std::string& strSQL, bool bWriteFlag = true)
    {
        CMySQLTableResult hMySQLTableResult;
        return RunSQL(strSQL, hMySQLTableResult, bWriteFlag);
    }

    // Run SQL
    bool RunSQL(const std::string& strSQL, CMySQLTableResult& hMySQLTableResult, bool bWriteFlag = false);

    // Get insert record id
    inline long long GetInsertId() const
    {
        return m_nInsertId;
    }

    // Get MYSQL handle
    inline MYSQL& GetMYSQLHandle()
    {
        return m_hMySQL;
    }

    // Get Affected Rows
    inline int GetAffectedRows() const
    {
        return m_nAffectedRows;
    }

    void SetConnectionConfig(const MySQLConfig& stMySQLConfig);

    inline void SetConnectionName(const std::string& strName)
    {
		m_strConnectionName = strName;
    }

	std::string GetConnectionName()
	{
		return m_strConnectionName;
	}

    // Init connection
    bool OpenConnection();

    // Close connection
    void CloseConnection();

    // Get timestamp of run sql time
    inline time_t GetRunSQLTimestamp() const
    {
        return m_nRunSQLTimestamp;
    }

    // Probe mysql connection
    static bool ProbeMySQLConnection(const MySQLConfig& stMySQLConfig);

private:
    // Check safe boundary
    bool CheckSafeBoundary();

    bool CallMySQLInit();

private:
    // MYSQL handle
    MYSQL m_hMySQL;
    // Safe Boundary
    char m_chSafeBoundary[1024];
    // MySQL config
    MySQLConfig m_stMySQLConfig;
    // available only for insert sql
    long long m_nInsertId = 0;
    // affected rows
    int m_nAffectedRows = 0;
    // Pre Stat SQL time stamp
    time_t m_nSQLStatTime = 0;
    // Run SQL count : one second
    unsigned int m_nRunSQLSpeed = 0;
    // Connection name
    std::string m_strConnectionName;
    // Timestamp of run sql
    time_t m_nRunSQLTimestamp = 0;

    //mysql多线程使用时init时需要加锁
    static EMutex ms_mutexInitMySQL;
};
#endif // _MYSQL_CONNECTION_H_
