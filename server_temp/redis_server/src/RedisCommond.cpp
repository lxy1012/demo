#include "RedisCommond.h"
#include <time.h>
#include "log.h"
#if _WIN32
#include "hiredis/hiredis.h"
#else
#include "hiredis.h"
#endif


void RedisCommond::redisError(const char* method, void* _replay, int iParent /* = 0 */)
{
	char szValue[32];
	sprintf(szValue, "RC<%s:%d>", method, iParent);
	redisReply* reply = (redisReply*)_replay;
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_STRING)
		{
			_log(_ERROR, szValue, "redisError REDIS_REPLY_STRING %s", reply->str);
		}
		else if (reply->type == REDIS_REPLY_INTEGER)
		{
			_log(_ERROR, szValue, "redisError REDIS_REPLY_INTEGER %lld", reply->integer);
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			_log(_ERROR, szValue, "redisError REDIS_REPLY_NIL");
		}
		else if (reply->type == REDIS_REPLY_ERROR)
		{
			_log(_ERROR, szValue, "redisError REDIS_REPLY_ERROR %s", reply->str);
		}
		else if (reply->type == REDIS_REPLY_ARRAY)
		{
			_log(_WARN, szValue, "redisError REDIS_REPLY_ARRAY %lld", (long long)reply->elements);
			iParent++;
			for (size_t i = 0; i < reply->elements; ++i)
			{
				redisReply* child = reply->element[i];
				redisError("redisError", child, iParent);
			}
		}
	}
	else
	{
		_log(_DEBUG, szValue, "redisError null replay");
	}
}

long long RedisCommond::GetLongResByCmdV(void* _redisContext, const char* format, ...)
{
	//准备
	long long rt = 0;
	redisContext* c = (redisContext*)_redisContext;
	//执行
	va_list ap;
	va_start(ap, format);
	redisReply* reply = (redisReply*)redisvCommand(c, format, ap);
	va_end(ap);
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			rt = reply->integer;
		}
		else
		{
			redisError("GetLongResByCmdV", reply);
		}
		freeReplyObject(reply);
	}
	//返回
	return rt;
}

std::string RedisCommond::GetStringResByCmdV(void* _redisContext, const char* format, ...)
{
	//准备
	std::string rt = "";
	redisContext* c = (redisContext*)_redisContext;
	//执行
	va_list ap;
	va_start(ap, format);
	redisReply* reply = (redisReply*)redisvCommand(c, format, ap);
	va_end(ap);
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_STRING)
		{
			rt = reply->str;
		}
		else
		{
			redisError("GetStringResByCmdV", reply);
		}
		freeReplyObject(reply);
	}
	//返回
	return rt;
}

VCKVPair RedisCommond::GetVCKVPairByCmdV(void* _redisContext, const char* format, ...)
{
	VCKVPair rt;
	RedisResKVPairDef kv;
	redisContext* c = (redisContext*)_redisContext;
	//执行
	va_list ap;
	va_start(ap, format);
	redisReply* reply = (redisReply*)redisvCommand(c, format, ap);
	va_end(ap);
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			for (size_t i = 0; i < reply->elements; i += 2)
			{
				redisReply* keyReplay = reply->element[i];
				redisReply* valueReplay = reply->element[i + 1];
				if (keyReplay == NULL || valueReplay == NULL)
				{
					redisError("GetVCKVPairByCmdV", keyReplay, 1);
					redisError("GetVCKVPairByCmdV", valueReplay, 1);
					break;
				}
				if (keyReplay->type != REDIS_REPLY_STRING || valueReplay->type != REDIS_REPLY_STRING)
				{
					redisError("GetVCKVPairByCmdV", keyReplay, 1);
					redisError("GetVCKVPairByCmdV", valueReplay, 1);
					break;
				}
				kv.member = keyReplay->str;
				kv.value = valueReplay->str;
				rt.push_back(kv);
			}
		}
		else
		{
			redisError("GetVCKVPairByCmdV", reply);
		}
		freeReplyObject(reply);
	}
	//返回
	return rt;
}

long long RedisCommond::GetLongResByCmd(void* _redisContext, const char* _cmd)
{
	//准备
	long long rt = 0;
	redisContext* c = (redisContext*)_redisContext;
	//执行
	redisReply* reply = (redisReply*)redisCommand(c, _cmd);
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			rt = reply->integer;
		}
		else
		{
			redisError("GetLongResByCmd", reply);
		}
		freeReplyObject(reply);
	}
	//返回
	_log(_DEBUG, "RC", "GetLongResByCmd %lld = %s", rt, _cmd);
	return rt;
}

std::string RedisCommond::GetStringResByCmd(void* _redisContext, const char* _cmd)
{
	//准备
	std::string rt = "";
	redisContext* c = (redisContext*)_redisContext;
	//执行
	redisReply* reply = (redisReply*)redisCommand(c, _cmd);
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_STRING)
		{
			rt = reply->str;
		}
		else
		{
			redisError("GetStringResByCmd", reply);
		}
		freeReplyObject(reply);
	}
	//返回
	_log(_DEBUG, "RC", "GetStringResByCmd %s = %s", rt.c_str(), _cmd);
	return rt;
}

VCKVPair RedisCommond::GetVCKVPairByCmd(void* _redisContext, const char* _cmd)
{
	VCKVPair rt;
	RedisResKVPairDef kv;
	redisContext* c = (redisContext*)_redisContext;
	//执行
	redisReply* reply = (redisReply*)redisCommand(c, _cmd);
	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			if (reply->elements % 2 == 0 && reply->elements > 1)
			{
				for (size_t i = 0; i < reply->elements; i += 2)
				{
					redisReply* keyReplay = reply->element[i];
					redisReply* valueReplay = reply->element[i + 1];
					//if (keyReplay == NULL || valueReplay == NULL)
					//{
					//	redisError(keyReplay, 1);
					//	redisError(valueReplay, 1);
					//	break;
					//}
					//if (keyReplay->type != REDIS_REPLY_STRING || valueReplay->type != REDIS_REPLY_STRING)
					//{
					//	redisError(keyReplay, 1);
					//	redisError(valueReplay, 1);
					//	break;
					//}
					kv.member = keyReplay->str;
					kv.value = valueReplay->str;
					rt.push_back(kv);
				}
			}
			else
			{
				redisError("GetVCKVPairByCmd", reply);
			}
		}
		else
		{
			redisError("GetVCKVPairByCmd", reply);
		}
		freeReplyObject(reply);
	}
	//返回
	_log(_DEBUG, "RC", "GetVCKVPairByCmd %lld = %s", (long long)rt.size(), _cmd);
	return rt;
}
