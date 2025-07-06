#ifndef __RedisCommond_H__
#define __RedisCommond_H__

#include <iostream>
#include <stdio.h>
#include <map>
#include <string.h>
#include <vector>

typedef struct RedisResKVPair
{
	std::string member;	//orderset的member 或者 hash的field （以字符串返回）
	std::string value;	//值（以字符串返回）
}RedisResKVPairDef;

typedef std::vector<RedisResKVPairDef> VCKVPair;	//键值对有序数组

class RedisCommond
{
private:
	void redisError(const char* method, void* _replay, int iParent = 0);	//打印真实类型或者错误原因，开发过程需要重点关注
	char m_szBuff[2048];	//确保单线程使用
public:
	long long GetLongResByCmdV(void* _redisContext, const char* format, ...);//执行返回longlong的请求（类型不正确会得到0）
	std::string GetStringResByCmdV(void* _redisContext, const char* format, ...);//执行返回string的请求 （类型不正确会得到空字符串）
	VCKVPair GetVCKVPairByCmdV(void* _redisContext, const char* format, ...);//执行返回键值对数组的请求 （类型不正确会得到空值）

	long long GetLongResByCmd(void* _redisContext, const char* _cmd);		//执行返回longlong的请求（类型不正确会得到0）
	std::string GetStringResByCmd(void* _redisContext, const char* _cmd);	//执行返回string的请求 （类型不正确会得到空字符串）
	VCKVPair GetVCKVPairByCmd(void* _redisContext, const char* _cmd);		//执行返回键值对有序数组的请求 （类型不正确会得到空值）
};

#endif