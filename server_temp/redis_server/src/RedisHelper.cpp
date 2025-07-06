#include "RedisHelper.h"
#include "log.h"
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <utility>

#define _VCFormat1(VEC)	 VEC[0].c_str()
#define _VCFormat2(VEC)	 _VCFormat1(VEC),VEC[1].c_str()
#define _VCFormat3(VEC)	 _VCFormat2(VEC),VEC[2].c_str()
#define _VCFormat4(VEC)	 _VCFormat3(VEC),VEC[3].c_str()
#define _VCFormat5(VEC)	 _VCFormat4(VEC),VEC[4].c_str()
#define _VCFormat6(VEC)	 _VCFormat5(VEC),VEC[5].c_str()
#define _VCFormat7(VEC)	 _VCFormat6(VEC),VEC[6].c_str()
#define _VCFormat8(VEC)	 _VCFormat7(VEC),VEC[7].c_str()
#define _VCFormat9(VEC)	 _VCFormat8(VEC),VEC[8].c_str()
#define _VCFormat10(VEC) _VCFormat9(VEC),VEC[9].c_str()
#define _VCFormat11(VEC) _VCFormat10(VEC),VEC[10].c_str()
#define _VCFormat12(VEC) _VCFormat11(VEC),VEC[11].c_str()
#define _VCFormat13(VEC) _VCFormat12(VEC),VEC[12].c_str()
#define _VCFormat14(VEC) _VCFormat13(VEC),VEC[13].c_str()
#define _VCFormat15(VEC) _VCFormat14(VEC),VEC[14].c_str()
#define _VCFormat16(VEC) _VCFormat15(VEC),VEC[15].c_str()
#define _VCFormat17(VEC) _VCFormat16(VEC),VEC[16].c_str()
#define _VCFormat18(VEC) _VCFormat17(VEC),VEC[17].c_str()
#define _VCFormat19(VEC) _VCFormat18(VEC),VEC[18].c_str()
#define _VCFormat20(VEC) _VCFormat19(VEC),VEC[19].c_str()
#define _VCFormat21(VEC) _VCFormat20(VEC),VEC[20].c_str()
#define _VCFormat22(VEC) _VCFormat21(VEC),VEC[21].c_str()
#define _VCFormat23(VEC) _VCFormat22(VEC),VEC[22].c_str()
#define _VCFormat24(VEC) _VCFormat23(VEC),VEC[23].c_str()
#define _VCFormat25(VEC) _VCFormat24(VEC),VEC[24].c_str()
#define _VCFormat26(VEC) _VCFormat25(VEC),VEC[25].c_str()
#define _VCFormat27(VEC) _VCFormat26(VEC),VEC[26].c_str()
#define _VCFormat28(VEC) _VCFormat27(VEC),VEC[27].c_str()
#define _VCFormat29(VEC) _VCFormat28(VEC),VEC[28].c_str()
#define _VCFormat30(VEC) _VCFormat29(VEC),VEC[29].c_str()
#define _VCFormat31(VEC) _VCFormat30(VEC),VEC[30].c_str()
#define _VCFormat32(VEC) _VCFormat31(VEC),VEC[31].c_str()
#define _VCFormat33(VEC) _VCFormat32(VEC),VEC[32].c_str()
#define _VCFormat34(VEC) _VCFormat33(VEC),VEC[33].c_str()
#define _VCFormat35(VEC) _VCFormat34(VEC),VEC[34].c_str()
#define _VCFormat36(VEC) _VCFormat35(VEC),VEC[35].c_str()
#define _VCFormat37(VEC) _VCFormat36(VEC),VEC[36].c_str()
#define _VCFormat38(VEC) _VCFormat37(VEC),VEC[37].c_str()
#define _VCFormat39(VEC) _VCFormat38(VEC),VEC[38].c_str()
#define _VCFormat40(VEC) _VCFormat39(VEC),VEC[39].c_str()
#define _VCFormat41(VEC) _VCFormat40(VEC),VEC[40].c_str()
#define _VCFormat42(VEC) _VCFormat41(VEC),VEC[41].c_str()
#define _VCFormat43(VEC) _VCFormat42(VEC),VEC[42].c_str()
#define _VCFormat44(VEC) _VCFormat43(VEC),VEC[43].c_str()
#define _VCFormat45(VEC) _VCFormat44(VEC),VEC[44].c_str()
#define _VCFormat46(VEC) _VCFormat45(VEC),VEC[45].c_str()
#define _VCFormat47(VEC) _VCFormat46(VEC),VEC[46].c_str()
#define _VCFormat48(VEC) _VCFormat47(VEC),VEC[47].c_str()
#define _VCFormat49(VEC) _VCFormat48(VEC),VEC[48].c_str()
#define _VCFormat50(VEC) _VCFormat49(VEC),VEC[49].c_str()


void* RedisHelper::FinalCmd(redisContext *context, const char* cmd, const std::string &key, const std::vector<std::string>& v)
{
	redisReply* reply = NULL;

	switch (v.size())
	{
	case 1: {reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat1(v)); break;}
	case 2:	{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat2(v)); break;}
	case 3:	{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat3(v)); break;}
	case 4:	{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat4(v)); break;}
	case 5:	{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat5(v)); break;}
	case 6:	{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat6(v)); break;}
	case 7:	{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat7(v)); break;}
	case 8:	{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat8(v)); break;}
	case 9:	{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat9(v)); break;}
	case 10:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat10(v));break;}
	case 11:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat11(v));break;}
	case 12:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat12(v));break;}
	case 13:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat13(v));break;}
	case 14:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat14(v));break;}
	case 15:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat15(v));break;}
	case 16:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat16(v));break;}
	case 17:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat17(v));break;}
	case 18:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat18(v));break;}
	case 19:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat19(v));break;}
	case 20:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat20(v));break;}
	case 21:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat21(v));break;}
	case 22:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat22(v));break;}
	case 23:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat23(v));break;}
	case 24:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat24(v));break;}
	case 25:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat25(v));break;}
	case 26:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat26(v));break;}
	case 27:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat27(v));break;}
	case 28:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat28(v));break;}
	case 29:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat29(v));break;}
	case 30:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat30(v));break;}
	case 31:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat31(v));break;}
	case 32:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat32(v));break;}
	case 33:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat33(v));break;}
	case 34:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat34(v));break;}
	case 35:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat35(v));break;}
	case 36:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat36(v));break;}
	case 37:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat37(v));break;}
	case 38:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat38(v));break;}
	case 39:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat39(v));break;}
	case 40:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat40(v));break;}
	case 41:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat41(v));break;}
	case 42:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat42(v));break;}
	case 43:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat43(v));break;}
	case 44:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat44(v));break;}
	case 45:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat45(v));break;}
	case 46:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat46(v));break;}
	case 47:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat47(v));break;}
	case 48:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat48(v));break;}
	case 49:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat49(v));break;}
	case 50:{reply = (redisReply*)redisCommand(context, cmd, key.c_str(), _VCFormat50(v));break;}
	default:
		break;
	}
	return reply;
}

std::string RedisHelper::hget(redisContext* context,const std::string& key,const std::string& field)
{
	char szCmd[512];
	sprintf(szCmd, "hget %s %s", key.c_str(), field.c_str());
	std::string rt = "0";
	int type = -1;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "hget %s %s", key.c_str(), field.c_str());
	if (reply != NULL)
	{
		type = reply->type;
		if (reply->type == REDIS_REPLY_STRING)
		{
			rt = reply->str;
		}
		else if (reply->type == REDIS_REPLY_INTEGER)
		{
			sprintf(szCmd,"%lld",reply->integer);
			rt = szCmd;
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			rt = "";
		}
		else if(reply->type == REDIS_REPLY_ERROR)
		{
			rt = "";
			_log(_ERROR, "rediserr", "%s = %s", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "hget reply == NULL %s szCmd = %s", szCmd, context->errstr);
	}
	return rt;
}

bool RedisHelper::hset(redisContext *context, const std::string &key, const std::string &field, const std::string &value)
{
	bool bSuccess = true;
	std::string strResult = "";
	redisReply* reply = (redisReply*)redisCommand(context, "hset %s %s %s", key.c_str(), field.c_str(), value.c_str());
	int type = 0;
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_ERROR)
		{
			strResult = reply->str;
			_log(_ERROR, "hset", "key[%s] field[%s] = %s", key.c_str(), field.c_str(), strResult.c_str());
		}
		else if (reply->type == REDIS_REPLY_STRING)
		{
			strResult = reply->str;
		}
		else if (reply->type == REDIS_REPLY_INTEGER)
		{
			//sprintf((char*)strResult.c_str(),"%lld",reply->integer);	//这种写法会导致内存越界
			char cTemp[32];
			sprintf(cTemp, "%lld", reply->integer);
			strResult = cTemp;
		}
		type = reply->type;
		freeReplyObject(reply);
	}
	else
	{
		_log(_ERROR, "rediscmd", "hset errtype[%d],err[%s]", context->err, context->errstr);
	}
	//_log(_DEBUG, "RedisHelper::hset", "szCmd[%s] strResult[%s] type = %d", szCmd.c_str(), strResult.c_str(),type);
	return bSuccess;
}

bool RedisHelper::hmget(redisContext *context,const std::string &key,std::map<std::string,std::string>& mapHash)
{
	std::string szCmd = "hmget %s";
	std::string szLog = "hmget "+ key;
	int iAllSize = mapHash.size();
	std::map<std::string,std::string>::iterator itermHas = mapHash.begin();

	std::vector<std::string> vcField;
	bool bSuccess = true;
	while (itermHas != mapHash.end())
	{
		szCmd += " %s";
		szLog += itermHas->first;
		vcField.push_back(itermHas->first);
		itermHas++;
		if (vcField.size() >= 50 || itermHas == mapHash.end())
		{
			redisReply* reply;
			std::string strInfo = "";
			reply = (redisReply*)FinalCmd(context, szCmd.c_str(), key, vcField);
			if (reply != NULL)
			{
				if (reply->type == REDIS_REPLY_ARRAY)
				{
					for (int i = 0; i < reply->elements; ++i)
					{
						redisReply* element = reply->element[i];

						if (element->type == REDIS_REPLY_STRING)
						{
							strInfo = element->str;
						}
						else if (element->type == REDIS_REPLY_INTEGER)
						{
							char cValue[32] = { 0 };
							sprintf(cValue, "%lld", element->integer);
							strInfo = cValue;
							//strArrValue[i] = strInfo;
						}
						else
						{
							strInfo = "nil";
							//strArrValue[i] = "nil";
						}

						mapHash[vcField[i]] = strInfo;
						//_log(_DEBUG, "hmget", "type[%d]  %s = %s  %s ", element->type, vcField[i].c_str(), strInfo.c_str(), mapHash[vcField[i]].c_str());
					}
				}
				else if (reply->type == REDIS_REPLY_ERROR)
				{
					//printf("szLog  = %s %d\n ", szLog.c_str(), mapHash.size());
					_log(_ERROR, "hmget", "error[%s]", reply->str);
					bSuccess = false;
					return bSuccess;
				}
				freeReplyObject(reply);
			}
			else
			{
				_log(_ERROR, "hmget", "reply == null %s szLog = %s", context->errstr, szLog.c_str());
			}

			vcField.clear();
			szCmd = "hmget %s";
			szLog = "hmget " + key;
		}
	}
	
	itermHas = mapHash.begin();
	while (itermHas != mapHash.end())
	{
		if (itermHas->second == "nil")
		{
			itermHas->second = "";
		}
		//_log(_DEBUG, "hmget", "  %s = %s ", itermHas->first.c_str(), itermHas->second.c_str());
		itermHas++;
	}
	
	return bSuccess;
}

void RedisHelper::hmset(redisContext *context,const std::string &key,std::map<std::string,std::string>& mapValues)
{
	std::string szLog = "hmset " + key;
	std::string szCmd = "hmset %s";
	std::map<std::string,std::string>::iterator iterValue = mapValues.begin();
	std::vector<std::string> vcField;
	while (iterValue != mapValues.end())
	{
		szCmd += " %s";
		szLog += " " + iterValue->first;
		vcField.push_back(iterValue->first);
		szCmd += " %s";
		szLog += " " + iterValue->second;
		vcField.push_back(iterValue->second);
		iterValue++;
		if (vcField.size() >= 50 || iterValue == mapValues.end())
		{
			redisReply* reply;
			reply = (redisReply*)FinalCmd(context, szCmd.c_str(), key, vcField);
			if (reply != NULL)
			{
				if (reply->type == REDIS_REPLY_ERROR)
				{
					_log(_ERROR, "hmset", "error szCmd[%s] ", szLog.c_str());
					_log(_ERROR, "hmset", "error error=[%s]", reply->str);
				}
				else
				{
					//_log(_DEBUG, "hmset","szCmd[%s] result=[%s]",szCmd,reply->str);
				}
				freeReplyObject(reply);
			}
			else
			{
				_log(_ERROR, "RedisHelper", " hmset reply = nullptr %s %s", szLog.c_str(), context->errstr);
			}
			vcField.clear();
			szLog = "hmset " + key;
			szCmd = "hmset %s";
		}
	}
}
bool RedisHelper::hdel(redisContext *context, const std::string &key, const std::string &field)
{
	char szCmd[512];
	sprintf(szCmd, "hdel %s %s", key.c_str(), field.c_str());
	redisReply* reply;
	bool bSuccess = true;
	std::string strResult = "";

	reply = (redisReply*)redisCommand(context, "hdel %s %s", key.c_str(), field.c_str());
	int type = 0;
	if (reply != NULL)
	{
		/*if (reply->type == REDIS_REPLY_ERROR)
		{
			strResult = reply->str;
			_log(_ERROR, "hset", "%s = %s", szCmd, strResult);
		}
		else if (reply->type == REDIS_REPLY_STRING)
		{
			strResult = reply->str;
		}
		else if (reply->type == REDIS_REPLY_INTEGER)
		{
			sprintf((char*)strResult.c_str(), "%lld", reply->integer);
		}*/
		type = reply->type;
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "hdel reply == NULL szCmd = %s", szCmd);
	}
	//_log(_DEBUG, "RedisHelper::hdel", "szCmd[%s] strResult[%s] type = %d", szCmd, strResult.c_str(), type);
	return bSuccess;
}
bool RedisHelper::hgetAll(redisContext *context, const std::string &key, std::map<std::string, std::string>& mapValues)
{
	char szCmd[2048] = { 0 };
	sprintf(szCmd, "hgetall %s", key.c_str());
	bool bSuccess = true;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "hgetall %s", key.c_str());
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			for (int i = 0; i < reply->elements ; i = i+2)
			{
				redisReply* element = reply->element[i];
				if (element->type == REDIS_REPLY_STRING)
				{
					std::string str = element->str;
					std::string strValue = "";
					if (reply->element[i + 1]->type == REDIS_REPLY_STRING)
					{
						strValue = reply->element[i + 1]->str;
						mapValues[str] = strValue;
					}
					else if (reply->element[i + 1]->type == REDIS_REPLY_INTEGER)
					{
						char szValue[64] = { 0 };
						sprintf(szValue, "%lld", reply->element[i + 1]->integer);
						strValue = szValue;
						mapValues[str] = strValue;
					}
					//itermHas->second = element->str;
				}
				else if (element->type == REDIS_REPLY_INTEGER)
				{
					char szValue[64] = { 0 };
					sprintf(szValue, "%lld", element->integer);
					std::string str = szValue;
					std::string strValue = "";
					if (reply->element[i + 1]->type == REDIS_REPLY_STRING)
					{
						strValue = reply->element[i + 1]->str;
						mapValues[str] = strValue;
					}
					else if (reply->element[i + 1]->type == REDIS_REPLY_INTEGER)
					{
						char szValue1[64] = { 0 };
						sprintf(szValue1, "%lld", reply->element[i + 1]);
						strValue = szValue1;
						mapValues[str] = strValue;
					}
				}
				//_log(_DEBUG, "hmget", "type[%d] ", element->type);
				//_log(_DEBUG,"hmget","type[%d]  %s = %s",element->type,itermHas->first.c_str(), itermHas->second.c_str());
				//itermHas++;
			}
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "hmget", "error szCmd[%s],error[%s]", szCmd, reply->str);
			bSuccess = false;
		}
		freeReplyObject(reply);
	}
	else
	{
		_log(_ERROR, "hmget", "reply == null %s", context->errstr);
	}
	return bSuccess;
}
long long RedisHelper::hincrby(redisContext* context,const std::string& key,const std::string& field, long long increment)
{
	char szCmd[512];
	sprintf(szCmd, "hincrby %s %s %lld", key.c_str(), field.c_str(), (long long)increment);
	long long rt = 0;
	int type = -1;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "hincrby %s %s %lld", key.c_str(), field.c_str(), increment);
	if (reply != NULL)
	{
		type = reply->type;
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			rt = reply->integer;
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "rediserr", "szCmd[%s] = %s", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "hincrby errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	//_log(_ERROR, "rediscmd", "szCmd[%s] rt = %lld  type = %d", szCmd, (long long)rt, type);
	return rt;
}

std::string RedisHelper::get(redisContext *context,const std::string &key)
{
	char szCmd[512];
	sprintf(szCmd, "get %s", key.c_str());
	std::string rt = "";
	int type = -1;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context,"get %s", key.c_str());
	if (reply != NULL)
	{
		type = reply->type;
		if (reply->type == REDIS_REPLY_STRING)
		{
			rt = reply->str;
		}
		else if (reply->type == REDIS_REPLY_INTEGER)
		{
			sprintf(szCmd, "%lld", reply->integer);
			rt = szCmd;
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			_log(_ERROR, "rediserr", "%s = nil", szCmd);
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "rediserr", "%s = %s", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "get errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	//_log(_DEBUG, " RedisHelper::get rediscmd"," szCmd[%s] rt = %s  type = %d", szCmd, rt.c_str(), type);
	return rt;
}

std::string RedisHelper::set(redisContext *context,const std::string &key, const std::string &member)
{
	char szCmd[512];
	sprintf(szCmd, "set %s %s", key.c_str(), member.c_str());
	std::string strCmd = szCmd;
	std::string rt = "";
	int type = -1;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "set %s %s", key.c_str(), member.c_str());
	if (reply != NULL)
	{
		type = reply->type;
		if (reply->type == REDIS_REPLY_STRING)
		{
			rt = reply->str;
		}
		else if (reply->type == REDIS_REPLY_INTEGER)
		{
			sprintf(szCmd, "%lld", reply->integer);
			rt = szCmd;
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			rt = "nil";
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			rt = reply->str;
			_log(_ERROR, "rediserr", "%s = %s", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "set errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	//_log(_DEBUG, "RedisHelper::set rediscmd", "szCmd[%s] rt = %s type = %d", strCmd.c_str(), rt.c_str(),type);
	return rt;
}

long long RedisHelper::zcard(redisContext* context,const std::string& key)
{
	char szCmd[512];
	sprintf(szCmd, "zcard %s", key.c_str());
	long long rt = 0;
	int type = -1;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "zcard %s", key.c_str());
	if (reply != NULL)
	{
		type = reply->type;
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			rt = reply->integer;
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "rediserr", "%s = %s", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "zcard errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	//_log(_DEBUG, "rediscmd", "szCmd[%s] rt = %lld  type = %d", szCmd, (long long)rt, type);
	return rt;
}

std::string RedisHelper::zadd(redisContext* context,const std::string& key, long long score,const std::string& member)
{
	char szCmd[512];
	sprintf(szCmd, "zadd %s %lld %s ", key.c_str(), (long long)score, member.c_str());
	std::string strCmd = szCmd;
	std::string rt = "";
	int type = -1;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "zadd %s %lld %s", key.c_str(), (long long)score, member.c_str());
	if (reply != NULL)
	{
		type = reply->type;
		if (reply->type == REDIS_REPLY_STRING)
		{
			rt = reply->str;
		}
		else if (reply->type == REDIS_REPLY_INTEGER)
		{
			sprintf(szCmd, "%lld", reply->integer);
			rt = szCmd;
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			rt = "nil";
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			rt = reply->str;
			_log(_ERROR, "rediserr", "%s = %s", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "zadd errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	//_log(_DEBUG, "rediscmd", "szCmd[%s] rt = %s  type = %d", strCmd.c_str(), rt.c_str(), type);
	return rt;
}

std::string RedisHelper::zadddouble(redisContext* context, const std::string& key, double score, const std::string& member)
{
	char szCmd[512];
	sprintf(szCmd, "zadd %s %lf %s ", key.c_str(), (double)score, member.c_str());
	char szScore[32];
	sprintf(szScore, "%lf", (double)score);
	std::string strCmd = szCmd;
	std::string rt = "";
	int type = -1;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "zadd %s %s %s", key.c_str(), szScore, member.c_str());
	if (reply != NULL)
	{
		type = reply->type;
		if (reply->type == REDIS_REPLY_STRING)
		{
			rt = reply->str;
		}
		else if (reply->type == REDIS_REPLY_INTEGER)
		{
			sprintf(szCmd, "%lld", reply->integer);
			rt = szCmd;
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			rt = "nil";
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			rt = reply->str;
			_log(_ERROR, "rediserr", "%s = %s", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "zadd errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	//_log(_DEBUG, "rediscmd", "szCmd[%s] rt = %s  type = %d", strCmd.c_str(), rt.c_str(), type);
	return rt;
}

std::string RedisHelper::zincrby(redisContext* context,const std::string& key, long long score,const std::string& member)
{
	char szCmd[512];
	sprintf(szCmd, "zincrby %s %lld %s ", key.c_str(), (long long)score, member.c_str());
	std::string strCmd = szCmd;
	std::string rt = "";
	int type = -1;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "zincrby %s %lld %s", key.c_str(), (long long)score, member.c_str());
	if (reply != NULL)
	{
		type = reply->type;
		if (reply->type == REDIS_REPLY_STRING)
		{
			rt = reply->str;
		}
		else if (reply->type == REDIS_REPLY_INTEGER)
		{
			sprintf(szCmd, "%lld", reply->integer);
			rt = szCmd;
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			rt = "nil";
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			rt = "";
			_log(_ERROR, "rediserr", "%s = %s", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "zincrby errorstr[%s] cmd[%s] ", context->errstr,szCmd);
	}
	//_log(_DEBUG, "rediscmd", "szCmd[%s] rt = %s  type = %d", strCmd.c_str(), rt.c_str(), type);
	return rt;
}

void RedisHelper::zremrangeByScore(redisContext* context,const std::string& key, long long min, long long max)
{
	char szCmd[512];
	
	sprintf(szCmd, "zremrangeByScore %s %lld %lld", key.c_str(), min, max);
	
	
	//_log(_DEBUG, "rediscmd", "%s", szCmd);
	redisReply* reply;
	int type = -1;
	reply = (redisReply*)redisCommand(context, "zremrangeByScore %s %lld %lld", key.c_str(), min, max);
	if (reply != NULL)
	{
		type = reply->type;
		if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "rediserr", "%s = %s", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "zremrangeByScore errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
}

long long RedisHelper::zrem(redisContext *context, const std::string &key, std::vector<std::string>vcMember)
{
	std::string	strLog = "ZREM " + key;
	std::string	strCmd = "ZREM %s";
	long long rt = 0;
	int type = -1;
	std::vector<std::string> vcCmdMemb;
	int iSuccNum = 0;
	for (int i = 0; i < vcMember.size(); i++)
	{
		strLog += " "+ vcMember[i];
		strCmd += " %s";
		vcCmdMemb.push_back(vcMember[i]);
		if (vcCmdMemb.size() >= 50 || i == vcMember.size() - 1)
		{
			redisReply* reply;
			reply = (redisReply*)FinalCmd(context, strCmd.c_str(), key, vcCmdMemb);
			if (reply != NULL)
			{
				type = reply->type;
				if (reply->type == REDIS_REPLY_INTEGER)
				{
					rt = reply->integer;
					iSuccNum += rt;
					/*return rt;*/
				}
				else if (reply->type == REDIS_REPLY_ERROR)
				{
					_log(_ERROR, "rediserr", "zremVc szCmd[%s] = %s", strLog.c_str(), reply->str);
				}
				freeReplyObject(reply);
			}
			if (reply == NULL)
			{
				_log(_ERROR, "rediscmd", "zremVc errorstr[%s] cmd[%s] ", context->errstr, strLog.c_str());
			}
			//_log(_DEBUG, "rediscmd", "szCmd[%s] rt = %lld  type = %d", strCmd.c_str(), rt, type);
			strLog = "ZREM " + key;
			strCmd = "ZREM %s";
			vcCmdMemb.clear();
		}
	}
	return iSuccNum;
}

long long RedisHelper::zrem(redisContext *context, const std::string &key, std::string strMember)
{
	char szCmd[512];
	sprintf(szCmd, "zrem %s %s", key.c_str(), strMember.c_str());
	std::string strCmd = szCmd;
	long long rt = 0;
	int type = -1;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "zrem %s %s", key.c_str(), strMember.c_str());
	if (reply != NULL)
	{
		type = reply->type;
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			rt = reply->integer;
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "rediserr", "szCmd[%s] = %s", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "zrem errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	//_log(_DEBUG, "rediscmd", "szCmd[%s] rt = %s  type = %d", strCmd.c_str(), rt.c_str(), type);
	return rt;
}

void RedisHelper::zrangeByScore(redisContext *context, const std::string &key, long long min, long long max, std::vector<std::string>& vecStr)
{
	char szCmd[512];
	sprintf(szCmd, "zrangebyscore %s %lld %lld ", key.c_str(), min, max);
	char szValue[32] = { 0 };
	//_log(_DEBUG, "rediscmd", "%s", szCmd);
	redisReply* reply;
	int type = -1;
	reply = (redisReply*)redisCommand(context, "zrangebyscore %s %lld %lld ", key.c_str(), min, max);
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			//_log(_DEBUG, "zrangeByScore", "szCmd[%s],elements[%d]", szCmd, reply->elements);
			for (int i = 0; i < reply->elements; ++i)
			{
				redisReply* element = reply->element[i];
				if (element->type == REDIS_REPLY_STRING)
				{
					vecStr.push_back(element->str);
				}
				else if (element->type == REDIS_REPLY_INTEGER)
				{
					sprintf(szValue, "%lld", element->integer);
					vecStr.push_back(szValue);
				}
				//_log(_DEBUG, "zrangebyscore", "[%d]-[%d]:type[%d]  str[%s]", i, vecStr.size(), element->type, vecStr.empty() ? "null" : vecStr[vecStr.size() - 1].c_str());
			}
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "lrange", "error szCmd[%s],error[%s]", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "zrangebyscore errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
}

void RedisHelper::zrangeByIndex(redisContext *context, const std::string &key, long long min, long long max, std::vector<std::string>& vecStr)
{
	char szCmd[512];
	sprintf(szCmd, "zrange %s %lld %lld ", key.c_str(), min, max);
	char szValue[32] = { 0 };
	//_log(_DEBUG, "rediscmd", "%s", szCmd);
	redisReply* reply;
	int type = -1;
	reply = (redisReply*)redisCommand(context, "zrange %s %lld %lld ", key.c_str(), min, max);
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			//_log(_DEBUG, "zrangeByIndex", "szCmd[%s],elements[%d]", szCmd, reply->elements);
			for (int i = 0; i < reply->elements; ++i)
			{
				redisReply* element = reply->element[i];
				if (element->type == REDIS_REPLY_STRING)
				{
					vecStr.push_back(element->str);
				}
				else if (element->type == REDIS_REPLY_INTEGER)
				{
					sprintf(szValue, "%lld", element->integer);
					vecStr.push_back(szValue);
				}
				//_log(_DEBUG, "zrangeByIndex", "[%d]-[%d]:type[%d]  str[%s]", i, vecStr.size(), element->type, vecStr.empty() ? "null" : vecStr[vecStr.size() - 1].c_str());
			}
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "lrange", "error szCmd[%s],error[%s]", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "zrangeByIndex errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
}

std::string RedisHelper::zscore(redisContext *context, const std::string &key, const std::string &strMember)
{
	char szCmd[512];
	sprintf(szCmd, "zscore %s %s ", key.c_str(), strMember.c_str());
	std::string rt = "";
	int type = -1;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "zscore %s %s", key.c_str(), strMember.c_str());
	if (reply != NULL)
	{
		type = reply->type;
		if (reply->type == REDIS_REPLY_STRING)
		{
			rt = reply->str;
		}
		else if (reply->type == REDIS_REPLY_INTEGER)
		{
			sprintf(szCmd, "%lld", reply->integer);
			rt = szCmd;
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			rt = "nil";
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "rediserr", "%s = %s", szCmd, reply->str);
			rt = "nil";
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "zscore errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	//_log(_DEBUG, "rediscmd", "szCmd[%s] rt = %s  type = %d", strCmd.c_str(), rt.c_str(), type);
	return rt;
}

VCZScore RedisHelper::zrevrangeWithScores(redisContext* context,const std::string& key, int start, int stop)
{
	VCZScore rt;
	ZScorePairDef pair;

	char szCmd[512];
	sprintf(szCmd, "zrevrange %s %d %d withscores", key.c_str(), (int)start, (int)stop);
	//_log(_DEBUG, "rediscmd", "%s start", szCmd);
	int type = -1;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "zrevrange %s %d %d withscores", key.c_str(), (int)start, (int)stop);
	if (reply != NULL)
	{
		type = reply->type;
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			//printf("1 reply->elements=%d\n", reply->elements);
			if (reply->elements % 2 == 0 && reply->elements > 1)
			{
				int num = 0;
				for (int i = 0; i < reply->elements; i += 2)
				{
					pair.member = reply->element[i]->str;
					pair.value = reply->element[i + 1]->str;
					rt.push_back(pair);
					//_log(_DEBUG, "rediscmd", "%s %d= (%s, %s)", szCmd, num, pair.member.c_str(), pair.value.c_str());
					//printf("L childL->str=%s\n", childL->str);
					//printf("R childR->str=%s\n", childR->str);
					num++;
				}
				if(num == 0)
				{
					_log(_DEBUG, "rediscmd", "%s error elements=%d", szCmd, reply->elements);
				}
			}
			else
			{
				_log(_DEBUG, "rediscmd", "%s elements=%d", szCmd, reply->elements);
			}
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "rediserr", "szCmd[%s]  rt = %s", szCmd, reply->str);
		}
		else
		{
			_log(_DEBUG, "rediscmd", "szCmd[%s] not array type=%d(4=nil)", szCmd, reply->type);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "zrevrange errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	//_log(_DEBUG, "rediscmd", "%s end", szCmd);
	return rt;
}
std::map<std::string, int> RedisHelper::zrangeByScore(redisContext* context, const std::string& key, long long min, long long max, long long offset/* = 0*/, long long count /*= 0*/)
{
	std::map<std::string, int> vcMember;
	char szCmd[512];
	if (offset == 0 && count == 0)
	{
		sprintf(szCmd, "zrangebyscore %s %d %d withscores", key.c_str(), (int)min, (int)max);
	}
	else
	{
		sprintf(szCmd, "zrangebyscore %s %d %d withscores limit %d %d", key.c_str(), (int)min, (int)max, (int)offset, (int)count);
	}
	
	//_log(_DEBUG, "rediscmd", "%s start", szCmd);
	int type = -1;
	redisReply* reply;
	if (offset == 0 && count == 0)
	{
		reply = (redisReply*)redisCommand(context, "zrangebyscore %s %d %d withscores", key.c_str(), (int)min, (int)max);
	}
	else
	{
		reply = (redisReply*)redisCommand(context, "zrangebyscore %s %d %d withscores limit %d %d", key.c_str(), (int)min, (int)max, (int)offset, (int)count);
	}
	if (reply != NULL)
	{
		type = reply->type;
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			//printf("1 reply->elements=%d\n", reply->elements);
			if (reply->elements % 2 == 0 && reply->elements > 1)
			{
				int num = 0;
				for (int i = 0; i < reply->elements; i += 2)
				{
					vcMember[reply->element[i]->str] = atoi(reply->element[i + 1]->str);
					//vcMember.push_back(reply->element[i]->str);
					_log(_DEBUG, "rediscmd", "%s %d= (%s) (%s)", szCmd, num, reply->element[i]->str, reply->element[i + 1]->str);
					//printf("L childL->str=%s\n", childL->str);
					//printf("R childR->str=%s\n", childR->str);
					num++;
				}
				if (num == 0)
				{
					_log(_DEBUG, "rediscmd", "%s error elements=%d", szCmd, reply->elements);
				}
			}
			else
			{
				_log(_DEBUG, "rediscmd", "%s elements=%d", szCmd, reply->elements);
			}
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "rediserr", "szCmd[%s]  rt = %s", szCmd, reply->str);
		}
		else
		{
			_log(_DEBUG, "rediscmd", "szCmd[%s] not array type=%d(4=nil)", szCmd, reply->type);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "zrangebyscore errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}

	return vcMember;
}
void RedisHelper::expire(redisContext* context,const std::string &key, int seconds)
{
	char szCmd[512];
	sprintf(szCmd, "expire %s %d", key.c_str(), seconds);
	std::string strCmd = szCmd;
	std::string rt = "";
	int type = -1;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "expire %s %d", key.c_str(), seconds);
	if(reply != NULL)
	{
		type = reply->type;
		if(reply->type == REDIS_REPLY_INTEGER)
		{
			sprintf(szCmd, "%lld", (long long)reply->integer);
			rt = szCmd;
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			rt = reply->str;
			_log(_ERROR, "rediserr", "%s = %s", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_DEBUG, "rediscmd", "expire reply == NULL %s", context->errstr);
	}
	//_log(_DEBUG, "rediscmd", "szCmd[%s] rt = %s type = %d", strCmd.c_str(), rt.c_str(), type);
}

long long RedisHelper::ttl(redisContext* context, const std::string &key)
{
	char szCmd[512];
	sprintf(szCmd, "ttl %s", key.c_str());
	std::string strCmd = szCmd;
	long long iRt = -1;
	int type = -1;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "ttl %s", key.c_str());
	if (reply != NULL)
	{
		type = reply->type;
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			//sprintf(szCmd, "%lld", (long long)reply->integer);
			//rt = szCmd;
			iRt = reply->integer;
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			//rt = reply->str;
			_log(_ERROR, "rediserr", "%s = %s", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "ttl errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	//_log(_DEBUG, "rediscmd", "szCmd[%s] rt = %lld type = %d", strCmd.c_str(), iRt, type);
	return iRt;
}
void RedisHelper::del(redisContext* context, const std::string &key)
{
	char szCmd[512];
	sprintf(szCmd, "del %s", key.c_str());
	std::string strCmd = szCmd;
	std::string rt = "";
	int type = -1;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "del %s", key.c_str());
	if(reply != NULL)
	{
		type = reply->type;
		if(reply->type == REDIS_REPLY_INTEGER)
		{
			sprintf(szCmd, "%lld", (long long)reply->integer);
			rt = szCmd;
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			rt = reply->str;
			_log(_ERROR, "rediserr", "%s = %s", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "del errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	//_log(_DEBUG, "rediscmd", "szCmd[%s] rt = %s type = %d", strCmd.c_str(), rt.c_str(), type);
}


void RedisHelper::test(redisContext* context, int iRand)
{
	//char szKey[32];
	//std::string key;

	//sprintf(szKey, "h:test:c:%d", iRand);
	//key = szKey;

	//excuteCmd(context, "hset " + key + " field 0");
	//excuteCmd(context, "expire " + key + " 120");

	// excuteCmd(context, "hget " + key + " field");
	// excuteCmd(context, "hincrby " + key + " field 120");
	// excuteCmd(context, "hget " + key + " field");

	// excuteCmd(context, "hdel " + key + " field1");
	// excuteCmd(context, "hget " + key + " field1");
	// excuteCmd(context, "hincrby " + key + " field1 120");
	// excuteCmd(context, "hget " + key + " field1");

	// excuteCmd(context, "expire " + key + " 120");

	// sprintf(szKey, "z:test:c:%d", iRand);
	// key = szKey;
	// excuteCmd(context, "zadd " + key + " 0 member");
	// excuteCmd(context, "zincrby " + key + " 100 member");

	// excuteCmd(context, "zincrby " + key + " 100 member1");
	// excuteCmd(context, "zincrby " + key + " 100 member2");
	// excuteCmd(context, "zscore " + key + " member2");

	// excuteCmd(context, "expire " + key + " 120");

	// test code

	/*char cTmp[256] = { 0 };
	bool bhSet = RedisHelper::hset(m_redisCt, "TEST HSET", "test feild 1", "1");
	std::string strHget = RedisHelper::hget(m_redisCt, "TEST HSET", "test feild 1");
	_log(_DEBUG, "TEST", "strHget = [%s]", strHget.c_str());

	RedisHelper::hset(m_redisCt, "TEST HSET", "test feild 2", "2");
	bool bhDel = RedisHelper::hdel(m_redisCt, "TEST HSET", "test feild 1");

	std::map<std::string, std::string> maphmgetall;
	bool bHgetAll = RedisHelper::hgetAll(m_redisCt, "TEST HMSET", maphmgetall);

	long long ihlen = RedisHelper::hlen(m_redisCt, "TEST HMSET");

	RedisHelper::hincrby(m_redisCt, "TEST HMSET", "test feild 2", 1);

	std::string strset = RedisHelper::set(m_redisCt, "TEST SET", "1", 36000);
	std::string strget = RedisHelper::get(m_redisCt, "TEST SET");
	_log(_DEBUG, "TEST", "strget = [%s]", strget.c_str());

	std::vector<std::string> vcZrem;
	for (int i = 0; i < 55; i++)
	{
		sprintf(cTmp, "mem %d", i);
		RedisHelper::zadddouble(m_redisCt, "TEST ZADDDOUBLE", i, cTmp);
		RedisHelper::zincrby(m_redisCt, "TEST ZADDDOUBLE", 1, cTmp);
		if (i % 2 == 0)
		{
			vcZrem.push_back(cTmp);
		}
	}
	RedisHelper::zremrangeByScore(m_redisCt, "TEST ZADDDOUBLE", 30, 100);
	RedisHelper::zrem(m_redisCt, "TEST ZADDDOUBLE", vcZrem);
	RedisHelper::zrem(m_redisCt, "TEST ZADDDOUBLE", "mem 3");

	vcZrem.clear();
	RedisHelper::zrangeByScore(m_redisCt, "TEST ZADDDOUBLE", 0, 10, vcZrem);

	vcZrem.clear();
	RedisHelper::zrangeByIndex(m_redisCt, "TEST ZADDDOUBLE", 0, 10, vcZrem);

	sprintf(cTmp, "mem %d", 13);
	std::string strZScore = RedisHelper::zscore(m_redisCt, "TEST ZADDDOUBLE", cTmp);
	_log(_DEBUG, "TEST", "strZScore = [%s]", strZScore.c_str());

	VCZScore vcScore = RedisHelper::zrevrangeWithScores(m_redisCt, "TEST ZADDDOUBLE", 0, 20);

	std::map<std::string, int> mapzrangeByScore;
	mapzrangeByScore = RedisHelper::zrangeByScore(m_redisCt, "TEST ZADDDOUBLE", 0, 10);

	RedisHelper::expire(m_redisCt, "TEST ZADDDOUBLE", 36000);
	long long iexpire = RedisHelper::ttl(m_redisCt, "TEST ZADDDOUBLE");
	_log(_DEBUG, "TEST", "iexpire = [%lld]", iexpire);

	RedisHelper::hset(m_redisCt, "TEST DEL", "test", "1");
	bool bDelBefore = RedisHelper::exists(m_redisCt, "TEST DEL");
	_log(_DEBUG, "TEST", "bDelBefore = [%d]", bDelBefore);
	RedisHelper::del(m_redisCt, "TEST DEL");
	bool bDelEnd = RedisHelper::exists(m_redisCt, "TEST DEL");
	_log(_DEBUG, "TEST", "bDelEnd = [%d]", bDelEnd);

	RedisHelper::lpush(m_redisCt, "TEST PUSH", "test member lpush");
	RedisHelper::rpush(m_redisCt, "TEST PUSH", "test member rpush");

	int iListLen = RedisHelper::getListLen(m_redisCt, "TEST PUSH");
	_log(_DEBUG, "TEST", "iListLen = [%d]", iListLen);

	std::vector<std::string>vclrange;
	RedisHelper::lrange(m_redisCt, "TEST PUSH", 1, 10, vclrange);

	RedisHelper::ltrim(m_redisCt, "TEST PUSH", 1, 10);

	RedisHelper::sadd(m_redisCt, "TEST SADD", "test member 999");
	RedisHelper::srem(m_redisCt, "TEST SADD", "test member 999");

	int iscard = RedisHelper::scard(m_redisCt, "TEST SADD");
	_log(_DEBUG, "TEST", "iscard = [%d]", iscard);

	std::vector<std::string>vcpop;
	bool bpop = RedisHelper::spop(m_redisCt, "TEST SADD", 10, vcpop);

	vcpop.clear();
	bool bsmember = RedisHelper::smembers(m_redisCt, "TEST SADD", vcpop, 0);*/
}

void RedisHelper::excuteCmd(redisContext* context, std::string cmd)
{
	redisReply* reply = NULL;
	do
	{
		reply = (redisReply*)redisCommand(context, cmd.c_str());
		if (reply == NULL)
		{
			_log(_ERROR, "redistest", "%s=replay null", cmd.c_str());
			break;
		}

		switch (reply->type)
		{
			case REDIS_REPLY_STRING:
			{
				_log(_ERROR, "redistest", "%s=REDIS_REPLY_STRING %s", cmd.c_str(), reply->str);
				break;
			}
			case REDIS_REPLY_ARRAY:
			{
				_log(_ERROR, "redistest", "%s=REDIS_REPLY_ARRAY %d", cmd.c_str(), reply->elements);
				break;
			}
			case REDIS_REPLY_INTEGER:
			{
				_log(_ERROR, "redistest", "%s=REDIS_REPLY_INTEGER %lld", cmd.c_str(), (long long)reply->integer);
				break;
			}
			case REDIS_REPLY_NIL:
			{
				_log(_ERROR, "redistest", "%s=REDIS_REPLY_NIL ", cmd.c_str());
				break;
			}
			case REDIS_REPLY_STATUS:
			{
				_log(_ERROR, "redistest", "%s=REDIS_REPLY_STATUS %s", cmd.c_str(), reply->str);
				break;
			}
			case REDIS_REPLY_ERROR:
			{
				_log(_ERROR, "redistest", "%s=REDIS_REPLY_ERROR %s", cmd.c_str(), reply->str);
				break;
			}
			default:
			{
				_log(_ERROR, "redistest", "%s=undef type[%d] %s", cmd.c_str(), reply->type, reply->str);
				break;
			}
		}

	} while (0);
	if (reply)
	{
		freeReplyObject(reply);
	}
}


bool RedisHelper::exists(redisContext* context, const std::string &key)
{
	char szCmd[512];
	sprintf(szCmd, "exists %s", key.c_str());
	bool bRt = false;
	int type = -1;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "exists %s", key.c_str());
	if (reply != NULL)
	{
		type = reply->type;
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			bRt = reply->integer == 1 ? true : false;
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			bRt = false;
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "exists errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	//_log(_DEBUG, "rediscmd", "szCmd[%s]  type = %d,bRt=[%d]", szCmd, type, bRt);
	return bRt;
}

 int RedisHelper::lpush(redisContext *context, const std::string &key, const std::vector<std::string>& vecStr)
{
	int rt = 0;
	std::string	strLog = "lpush " + key;
	std::string	strCmd = "lpush %s";

	std::vector<std::string> vecFeild;
	for (int i = 0; i < vecStr.size(); i++)
	{
		strLog += " "+ vecStr[i];
		strCmd += " %s";
		vecFeild.push_back(vecStr[i]);
		if (vecFeild.size() >= 50 || i == vecStr.size() - 1)
		{
			redisReply* reply;
			reply = (redisReply*)FinalCmd(context, strCmd.c_str(), key, vecFeild);
			if (reply != NULL)
			{
				if (reply->type == REDIS_REPLY_INTEGER)
				{
					//_log(_DEBUG, "lpush", "szCmd[%s] result=[%lld]", szCmd, reply->integer);
					rt = (int)reply->integer;
				}
				else if (reply->type == REDIS_REPLY_ERROR)
				{
					_log(_ERROR, "lpush", "error szCmd[%s] error=[%s]", strLog.c_str(), reply->str);
				}
				else
				{
					_log(_DEBUG, "lpush", "szCmd[%s] result=[%s]", strLog.c_str(), reply->str);
				}
				freeReplyObject(reply);
			}
			if (reply == NULL)
			{
				_log(_ERROR, "rediscmd", "lpushVc errorstr[%s] cmd[%s] ", context->errstr, strLog.c_str());
			}
			vecFeild.clear();
			strLog = "lpush " + key;
			strCmd = "lpush %s";
		}
	}
	return rt;
}
 int RedisHelper::lpush(redisContext *context, const std::string &key, const std::string &strValue)
{
	int rt = 0;
	char szCmd[2048] = { 0 };
	sprintf(szCmd, "lpush %s %s", key.c_str(), strValue.c_str());
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "lpush %s %s", key.c_str(), strValue.c_str());
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			//_log(_DEBUG, "lpush", "szCmd[%s] result=[%lld]", szCmd, reply->integer);
			rt = (int)reply->integer;
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "lpush", "error szCmd[%s] error=[%s]", szCmd, reply->str);
		}
		else
		{
			_log(_DEBUG, "lpush", "szCmd[%s] result=[%s]", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "lpush errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	return rt;
}

int RedisHelper::rpush(redisContext *context, const std::string &key, std::vector<std::string>& vecStr)
{
	int rt = 0;
	std::string	strLog = "rpush " + key;
	std::string	strCmd = "rpush %s";

	std::vector<std::string> vecFeild;
	for (int i = 0; i < vecStr.size(); i++)
	{
		strLog += " " + vecStr[i];
		strCmd += " %s";
		vecFeild.push_back(vecStr[i]);
		if (vecFeild.size() >= 50 || i == vecStr.size() - 1)
		{
			redisReply* reply;
			reply = (redisReply*)FinalCmd(context, strCmd.c_str(), key, vecFeild);
			if (reply != NULL)
			{
				if (reply->type == REDIS_REPLY_INTEGER)
				{
					//_log(_DEBUG, "rpush", "szCmd[%s] result=[%lld]", szCmd, reply->integer);
					rt = (int)reply->integer;
				}
				else if (reply->type == REDIS_REPLY_ERROR)
				{
					_log(_ERROR, "rpush", "error szCmd[%s] error=[%s]", strLog.c_str(), reply->str);
				}
				else
				{
					_log(_DEBUG, "rpush", "szCmd[%s] result=[%s]", strLog.c_str(), reply->str);
				}
				freeReplyObject(reply);
			}
			if (reply == NULL)
			{
				_log(_ERROR, "rediscmd", "rpushVc errorstr[%s] cmd[%s] ", context->errstr, strLog.c_str());
			}
			vecFeild.clear();
			strLog = "rpush " + key;
			strCmd = "rpush %s";
		}
	}
	return rt;
}

 int RedisHelper::rpush(redisContext *context, const std::string &key, const std::string &strValue)
{
	int rt = 0;
	char szCmd[2048] = { 0 };
	sprintf(szCmd, "rpush %s %s", key.c_str(), strValue.c_str());
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "rpush %s %s", key.c_str(), strValue.c_str());
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			//_log(_DEBUG, "rpush", "szCmd[%s] result=[%lld]", szCmd, reply->integer);
			rt = (int)reply->integer;
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "rpush", "error szCmd[%s] error=[%s]", szCmd, reply->str);
		}
		else
		{
			_log(_DEBUG, "rpush", "szCmd[%s] result=[%s]", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "rpush errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	return rt;
}

int RedisHelper::getListLen(redisContext *context, const std::string &key)
{
	int rt = 0;
	char szCmd[2048] = { 0 };
	sprintf(szCmd, "llen %s", key.c_str());
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "llen %s", key.c_str());
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			//_log(_DEBUG, "getListLen", "szCmd[%s] result=[%lld]", szCmd, reply->integer);
			rt = (int)reply->integer;
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "getListLen", "error szCmd[%s] error=[%s]", szCmd, reply->str);
		}
		else
		{
			_log(_DEBUG, "getListLen", "szCmd[%s] result=[%s]", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "getListLen errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	return rt;
}

bool RedisHelper::lrange(redisContext *context, const std::string &key, int iStart, int iEnd, std::vector<std::string>& vecStr)
{
	char szCmd[1024] = { 0 };
	char szValue[32] = { 0 };
	sprintf(szCmd, "lrange %s %d %d", key.c_str(), iStart, iEnd);
	bool bSuccess = true;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "lrange %s %d %d", key.c_str(), iStart, iEnd);
	if (reply != NULL)
	{
		//_log(_DEBUG, "lrange", "szCmd[%s],iStart[%d],iEnd[%d],type[%d]", szCmd, iStart, iEnd, reply->type);
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			_log(_DEBUG, "lrange", "szCmd[%s],elements[%d]", szCmd, reply->elements);
			for (int i = 0; i < reply->elements; ++i)
			{
				redisReply* element = reply->element[i];
				if (element->type == REDIS_REPLY_STRING)
				{
					vecStr.push_back(element->str);
				}
				else if (element->type == REDIS_REPLY_INTEGER)
				{
					sprintf(szValue, "%lld", element->integer);
					vecStr.push_back(szValue);
				}
				_log(_DEBUG, "lrange", "[%d]-[%d]:type[%d]  str[%s]", i, vecStr.size(),element->type, vecStr.empty()?"null": vecStr[vecStr.size()-1].c_str());
			}
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "lrange", "error szCmd[%s],error[%s]", szCmd, reply->str);
			bSuccess = false;
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "lrange errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	return bSuccess;
}

bool RedisHelper::ltrim(redisContext *context, const std::string &key, int iStart, int iEnd)
{
	char szCmd[1024] = { 0 };
	char szValue[32] = { 0 };
	sprintf(szCmd, "ltrim %s %d %d", key.c_str(), iStart, iEnd);
	bool bSuccess = true;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "ltrim %s %d %d", key.c_str(), iStart, iEnd);
	if (reply != NULL)
	{
		//_log(_DEBUG, "lrange", "szCmd[%s],iStart[%d],iEnd[%d],type[%d]", szCmd, iStart, iEnd, reply->type);
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "lrange", "error szCmd[%s],error[%s]", szCmd, reply->str);
			bSuccess = false;
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "ltrim errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	return bSuccess;
}

int RedisHelper::sadd(redisContext *context, const std::string &key, const std::string &strValue)
{
	int rt = 0;
	char szCmd[2048] = { 0 };
	sprintf(szCmd, "sadd %s %s", key.c_str(), strValue.c_str());
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "sadd %s %s", key.c_str(), strValue.c_str());
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			//_log(_DEBUG, "sadd", "szCmd[%s] result=[%lld]", szCmd, reply->integer);
			rt = (int)reply->integer;
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "sadd", "error szCmd[%s] error=[%s]", szCmd, reply->str);
		}
		else
		{
			//_log(_DEBUG, "sadd", "szCmd[%s] result=[%s]", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "sadd errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	return rt;
}

int RedisHelper::sadd(redisContext *context, const std::string &key, std::vector<std::string>& vecStr)
{
	int rt = 0;
	std::string	strLog = "sadd " + key;
	std::string	strCmd = "sadd %s";
	std::vector<std::string> vecFeild;
	for (int i = 0; i < vecStr.size(); i++)
	{
		strLog += " " + vecStr[i];
		strCmd += " %s";
		vecFeild.push_back(vecStr[i]);
		if (vecFeild.size() >= 50 || i == vecStr.size() - 1)
		{
			redisReply* reply;
			reply = (redisReply*)FinalCmd(context, strCmd.c_str(), key, vecFeild);
			if (reply != NULL)
			{
				if (reply->type == REDIS_REPLY_INTEGER)
				{
					//_log(_DEBUG, "sadd", "szCmd[%s] result=[%lld]", szCmd, reply->integer);
					rt = (int)reply->integer;
				}
				else if (reply->type == REDIS_REPLY_ERROR)
				{
					_log(_ERROR, "sadd", "error szCmd[%s] error=[%s]", strLog.c_str(), reply->str);
				}
				else
				{
					//_log(_DEBUG, "sadd", "szCmd[%s] result=[%s]", szCmd, reply->str);
				}
				freeReplyObject(reply);
			}
			if (reply == NULL)
			{
				_log(_ERROR, "rediscmd", "saddVc errorstr[%s] cmd[%s] ", context->errstr, strLog.c_str());
			}
			vecFeild.clear();
			strLog = "sadd " + key;
			strCmd = "sadd %s";
		}
	}
	return rt;
}

int RedisHelper::srem(redisContext *context, const std::string &key, const std::string &strValue)
{
	int rt = 0;
	char szCmd[2048] = { 0 };
	sprintf(szCmd, "srem %s %s", key.c_str(), strValue.c_str());
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "srem %s %s", key.c_str(), strValue.c_str());
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			//_log(_DEBUG, "sadd", "szCmd[%s] result=[%lld]", szCmd, reply->integer);
			rt = (int)reply->integer;
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "srem", "error szCmd[%s] error=[%s]", szCmd, reply->str);
		}
		else
		{
			//_log(_DEBUG, "srem", "szCmd[%s] result=[%s]", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "srem errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	return rt;
}

int RedisHelper::scard(redisContext *context, const std::string &key)
{
	int rt = 0;
	char szCmd[2048] = { 0 };
	sprintf(szCmd, "scard %s", key.c_str());
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "scard %s", key.c_str());
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			//_log(_DEBUG, "scard", "szCmd[%s] result=[%lld]", szCmd, reply->integer);
			rt = (int)reply->integer;
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "scard", "error szCmd[%s] error=[%s]", szCmd, reply->str);
		}
		else
		{
			//_log(_DEBUG, "scard", "szCmd[%s] result=[%s]", szCmd, reply->str);
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "scard errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	return rt;
}

bool RedisHelper::spop(redisContext *context, const std::string &key, int iRandCount, std::vector<std::string>& vecStr)
{
	char szCmd[1024] = { 0 };
	char szValue[32] = { 0 };
	sprintf(szCmd, "spop %s %d", key.c_str(), iRandCount);
	bool bSuccess = true;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "spop %s %d", key.c_str(), iRandCount);
	if (reply != NULL)
	{
		//_log(_DEBUG, "spop", "szCmd[%s],iRandCount[%d],type[%d]", szCmd, iRandCount, reply->type);
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			//_log(_DEBUG, "spop", "szCmd[%s],elements[%d]", szCmd, reply->elements);
			for (int i = 0; i < reply->elements; ++i)
			{
				redisReply* element = reply->element[i];
				if (element->type == REDIS_REPLY_STRING)
				{
					vecStr.push_back(element->str);
				}
				else if (element->type == REDIS_REPLY_INTEGER)
				{
					sprintf(szValue, "%lld", element->integer);
					vecStr.push_back(szValue);
				}
				//_log(_DEBUG, "spop", "[%d]-[%d]:type[%d]  str[%s]", i, vecStr.size(), element->type, vecStr.empty() ? "null" : vecStr[vecStr.size() - 1].c_str());
			}
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "rediscmd", "spop error szCmd[%s],error[%s]", szCmd, reply->str);
			bSuccess = false;
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "spop errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	return bSuccess;
}
bool RedisHelper::smembers(redisContext *context, const std::string &key, std::vector<std::string>& vecStr, int iNeedNewLen)
{
	char* szCmd = new char[iNeedNewLen + 64];
	sprintf(szCmd, "smembers %s", key.c_str());
	char szValue[32] = { 0 };
	bool bSuccess = true;
	redisReply* reply;
	reply = (redisReply*)redisCommand(context, "smembers %s", key.c_str());
	if (reply != NULL)
	{
		//_log(_DEBUG, "smembers", "szCmd[%s],type[%d]", szCmd, reply->type);
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			//_log(_DEBUG, "smembers", "szCmd[%s],elements[%d]", szCmd, reply->elements);
			for (int i = 0; i < reply->elements; ++i)
			{
				redisReply* element = reply->element[i];
				if (element->type == REDIS_REPLY_STRING)
				{
					vecStr.push_back(element->str);
				}
				else if (element->type == REDIS_REPLY_INTEGER)
				{
					sprintf(szValue, "%lld", element->integer);
					vecStr.push_back(szValue);
				}
				//_log(_DEBUG, "spop", "[%d]-[%d]:type[%d]  str[%s]", i, vecStr.size(), element->type, vecStr.empty() ? "null" : vecStr[vecStr.size() - 1].c_str());
			}
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "rediscmd", "smembers error szCmd[%s],error[%s]", szCmd, reply->str);
			bSuccess = false;
		}
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "smembers errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	delete[]szCmd;
	return bSuccess;
}
long long RedisHelper::hlen(redisContext *context, const std::string &key)
{
	char szCmd[512] = { 0 };
	sprintf(szCmd, "hlen %s", key.c_str());

	long long iValue = 0;

	redisReply* reply = (redisReply*)redisCommand(context, "hlen %s", key.c_str());
	int type = 0;
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, "hlen", "%s = %s", szCmd, reply->str);
		}
		else if (reply->type == REDIS_REPLY_STRING)
		{
			_log(_ERROR, "hlen", "%s = %s", szCmd, reply->str);
		}
		else if (reply->type == REDIS_REPLY_INTEGER)
		{
			iValue = reply->integer;
		}
		type = reply->type;
		freeReplyObject(reply);
	}
	if (reply == NULL)
	{
		_log(_ERROR, "rediscmd", "hlen errorstr[%s] cmd[%s] ", context->errstr, szCmd);
	}
	//_log(_DEBUG, "RedisHelper::hlen", "szCmd[%s] iValue[%lld] type = %d", szCmd, iValue, type);

	return iValue;
}
void RedisHelper::CutString(std::vector<std::string>& vcStr, std::string& str, const char *cFlag) //分隔字符串，cFlag是标识符
{
	size_t iBeg = 0;
	while (true)
	{
		size_t iPos = str.find(cFlag, iBeg);
		if (iPos == std::string::npos)
		{
			vcStr.push_back(str.substr(iBeg));
			break;
		}
		vcStr.push_back(str.substr(iBeg, iPos - iBeg));
		iBeg = iPos + strlen(cFlag);
	}
}
