#ifndef __MsgHandleBase_H__
#define __MsgHandleBase_H__

#include <vector>
#include "hiredis.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MySQLConnection.h"
#include "main.h"
using namespace std;

extern SystemConfigBaseInfoDef stSysConfigBaseInfo;		/*系统配置*/


class CharMemManage
{
public:
	CharMemManage()
	{

	}
	~CharMemManage()
	{

	}
	virtual CharMemManage* operator->()
	{
		return this;
	}
	char* pBuffer;
	int iLen;
	CharMemManage  *pNext;             //指向前一节点
	CharMemManage  *pPrev;             //指向后一节点
};
/*
	消息接收器基类
*/
class MsgHandleBase
{
public:
	MsgHandleBase()
	{
		m_redisCt = NULL;
		m_pMemUse = NULL;
		m_pMemFree = NULL;
		m_pMySqlUser = NULL;
		m_pMySqlGame = NULL;
	}
	virtual void HandleMsg(int iMsgType, char* szMsg, int iLen,int iSocketIndex) = 0;
	virtual vector<int> GetHandleMsgTypes() = 0;//需要接收的消息类型
	virtual void SetRedisContext(redisContext* redisCt)
	{
		m_redisCt = redisCt;
	}
	virtual void SetMySqlConnect(CMySQLConnection* pMySqlUser)
	{
		m_pMySqlUser = pMySqlUser;
	}
	virtual void SetMySqlGame(CMySQLConnection* pMySqlGame)
	{
		m_pMySqlGame = pMySqlGame;
	}
	virtual void CallBackNextDay(int iTimeBeforeFlag,int iTimeNowFlag){};
	virtual void CallBackOnTimer(int iTimeNow){};
private:
	//有些类内有临时分配内存的需求，这里统一做个内存管理
	//严格来讲需要在析构函数内释放这些内存的，不过这些都是进程启动后就一直在运行的线程，
	//这个类对应的子类不会被delete，直到整个进程被kill，这里就不加释放这些内存的代码了
	CharMemManage* m_pMemUse;
	CharMemManage* m_pMemFree;
public:
	//找到合适的内存
	CharMemManage* GetCharMem(int iNeedLen)
	{
		//test
		/*if (m_pMemFree != NULL)
		{
			CharMemManage* pTempCharMem = m_pMemFree;
			while (pTempCharMem)
			{
				_log(_DEBUG, "MsgHandleBase", "GetCharMem:[0X%x]--[%d]", pTempCharMem->pBuffer, pTempCharMem->iLen);
				pTempCharMem = pTempCharMem->pNext;
			}
		}
		*/
		CharMemManage* pCharMem = NULL;
		if (m_pMemFree == NULL)
		{
			pCharMem = new CharMemManage();
			pCharMem->pBuffer = new char[iNeedLen+1];//+1个，多分配一个以免没有给终止符空间
			pCharMem->iLen = iNeedLen;
		}
		else
		{
			CharMemManage* pTempCharMem = m_pMemFree;
			while (pTempCharMem)
			{
				if (pTempCharMem->iLen >= iNeedLen)
				{
					pCharMem = pTempCharMem;
					//将pCharMem从m_pMemFree移掉
					if (pCharMem->pNext)  //若当前节点后有节点，让后节点的前指针指向前节点
					{
						pCharMem->pNext->pPrev = pCharMem->pPrev;
					}
					if (pCharMem->pPrev)  //若当前节点前有节点，让前节点的后指针指向后节点
					{
						pCharMem->pPrev->pNext = pCharMem->pNext;
					}
					if (pCharMem == m_pMemFree)
					{
						m_pMemFree = pCharMem->pNext;
						if (m_pMemFree)
						{
							m_pMemFree->pPrev = 0;
						}
					}
					break;
				}
				pTempCharMem = pTempCharMem->pNext;
			}
			if (pCharMem == NULL)
			{
				pCharMem = new CharMemManage();
				pCharMem->pBuffer = new char[iNeedLen+1];//+1个，多分配一个以免没有给终止符空间
				pCharMem->iLen = iNeedLen;
			}
		}
		if (pCharMem)
		{
			//插入到“使用中的节点”
			memset(pCharMem->pBuffer, 0, pCharMem->iLen);
			if (m_pMemUse) m_pMemUse->pPrev = pCharMem;
			pCharMem->pNext = m_pMemUse;
			pCharMem->pPrev = 0;
			m_pMemUse = pCharMem;
		}
		else
		{
			_log(_ERROR, "MsgHandleBase", "GetCharMem: Cannot get free charMem");
			return NULL;
		}
		//test
		//_log(_DEBUG, "MsgHandleBase", "GetCharMem return:[0X%x],len[%d]", pCharMem->pBuffer, iNeedLen, pCharMem->iLen);
		return pCharMem;
	}
	void ReleaseCharMem(CharMemManage* pCharMem)
	{		
		if (pCharMem->pNext)  //若当前节点后有节点，让后节点的前指针指向前节点
		{
			pCharMem->pNext->pPrev = pCharMem->pPrev;
		}

		if (pCharMem->pPrev)  //若当前节点前有节点，让前节点的后指针指向后节点
		{
			pCharMem->pPrev->pNext = pCharMem->pNext;
		}
		if (pCharMem == m_pMemUse) //如果是首节点，首节点后移
		{
			m_pMemUse = m_pMemUse->pNext;
			if (m_pMemUse)
			{
				m_pMemUse->pPrev = 0;
			}
		}
		//将释放的节点加入到“空闲节点”中去
		if (m_pMemFree) //已经存在“空闲节点”
		{
			m_pMemFree->pPrev = pCharMem;
		}
		pCharMem->pNext = m_pMemFree;
		pCharMem->pPrev = 0;
		m_pMemFree = pCharMem;
	}

	void CheckDBConnect()
	{
		if (m_pMySqlUser != NULL)
		{
			bool bConnectOK = true;
			CMySQLTableResult hMySQLTableResult;
			m_pMySqlUser->RunSQL("SELECT 1 FROM DUAL", hMySQLTableResult);
			const CMySQLTableRecord& hMySQLTableRecord = hMySQLTableResult.GetOneRecord();
			if (!hMySQLTableRecord.IsAvailable())
			{
				bConnectOK = false;
			}
			if (1 != hMySQLTableRecord.GetFieldAsInt32("1"))
			{
				bConnectOK = false;
			}
			if (!bConnectOK)
			{
				MySQLConfig stMySQLConfig;
				stMySQLConfig.strHost = stSysConfigBaseInfo.szDBUserHost;
				stMySQLConfig.strDBName = stSysConfigBaseInfo.szDBUserDataBase;
				stMySQLConfig.nPort = stSysConfigBaseInfo.iDBUserPort;
				stMySQLConfig.strUser = stSysConfigBaseInfo.szDBUser;
				stMySQLConfig.strPassword = stSysConfigBaseInfo.szDBUserPasswd;
				// ProbeMySQLConnection can tell the status of mysql
				CMySQLConnection::ProbeMySQLConnection(stMySQLConfig);
				CMySQLConnection hMySQLConnection;
				hMySQLConnection.SetConnectionConfig(stMySQLConfig);
				hMySQLConnection.SetConnectionName(m_pMySqlUser->GetConnectionName());
				bool bReConnectOK = hMySQLConnection.OpenConnection();
				if (!bReConnectOK)
				{
					_log(_ERROR, "DB", "cant't reconnect to mySql[%s]:[%s]", stSysConfigBaseInfo.szDBUserDataBase, stSysConfigBaseInfo.szDBUser);
					exit(0);
				}
				m_pMySqlUser->CloseConnection();
				m_pMySqlUser = &hMySQLConnection;
			}
		}

		if (m_pMySqlGame != NULL)
		{
			bool bConnectOK = true;
			CMySQLTableResult hMySQLTableResult;
			m_pMySqlGame->RunSQL("SELECT 1 FROM DUAL", hMySQLTableResult);
			const CMySQLTableRecord& hMySQLTableRecord = hMySQLTableResult.GetOneRecord();
			if (!hMySQLTableRecord.IsAvailable())
			{
				bConnectOK = false;
			}
			if (1 != hMySQLTableRecord.GetFieldAsInt32("1"))
			{
				bConnectOK = false;
			}
			if (!bConnectOK)
			{
				MySQLConfig stMySQLConfig;
				stMySQLConfig.strHost = stSysConfigBaseInfo.szDBGameHost;
				stMySQLConfig.strDBName = stSysConfigBaseInfo.szDBGameDataBase;
				stMySQLConfig.nPort = stSysConfigBaseInfo.iDBGamePort;
				stMySQLConfig.strUser = stSysConfigBaseInfo.szDBGame;
				stMySQLConfig.strPassword = stSysConfigBaseInfo.szDBGamePasswd;
				// ProbeMySQLConnection can tell the status of mysql
				CMySQLConnection::ProbeMySQLConnection(stMySQLConfig);
				CMySQLConnection hMySQLConnection;
				hMySQLConnection.SetConnectionConfig(stMySQLConfig);
				hMySQLConnection.SetConnectionName(m_pMySqlGame->GetConnectionName());
				bool bReConnectOK = hMySQLConnection.OpenConnection();
				if (!bReConnectOK)
				{
					_log(_ERROR, "DB", "cant't reconnect to mySql[%s]:[%s]", stSysConfigBaseInfo.szDBGameDataBase, stSysConfigBaseInfo.szDBGame);
					exit(0);
				}
				m_pMySqlGame->CloseConnection();
				m_pMySqlGame = &hMySQLConnection;
			}
		}
	}
protected:
	redisContext* m_redisCt;

	CMySQLConnection* m_pMySqlUser;
	CMySQLConnection* m_pMySqlGame;
};

#endif // !__MsgHandleBase_H__
