#ifndef __RedisHelper_H__
#define __RedisHelper_H__

#include "hiredis.h"
#include <string.h>
#include <iostream>
#include <iterator>
#include <time.h>
#include <vector>
#include <map>

typedef struct ZScorePair
{
	std::string member;
	std::string value;
}ZScorePairDef;

typedef std::vector<ZScorePairDef> VCZScore;

class RedisHelper
{
public:	
    static std::string hget(redisContext *context,const std::string &key,const std::string &field); //hget ? c++??string
	static bool hset(redisContext *context,const std::string &key,const std::string &field,const std::string &value);

	//hset的批量获取,减少命令
	static bool hmget(redisContext *context,const std::string &key,std::map<std::string,std::string>& mapHash);
	static void hmset(redisContext *context,const std::string &key,std::map<std::string, std::string>& mapValues);
	static bool hdel(redisContext *context, const std::string &key, const std::string &field);
	static bool hgetAll(redisContext *context, const std::string &key, std::map<std::string, std::string>& mapValues);

	static long long hlen(redisContext *context, const std::string &key);

    static long long hincrby(redisContext *context,const std::string &key,const std::string &field, long long increment); //hincrby c++????
	static std::string get(redisContext *context,const std::string &key);
	static std::string set(redisContext *context, const std::string &key, const std::string &member);

    static long long zcard(redisContext *context,const std::string &key);    //??????
    static std::string zadd(redisContext *context,const std::string &key, long long score,const std::string &member);       //order set ??????member ???score? ????????
	static std::string zadddouble(redisContext* context, const std::string& key, double score, const std::string& member);	//提供浮点数的score
	static std::string zincrby(redisContext *context,const std::string &key, long long score,const std::string &member);
    static void zremrangeByScore(redisContext *context,const std::string &key, long long min, long long max);   //order set ???????????????????
	static long long zrem(redisContext *context, const std::string &key, std::vector<std::string>vcMember);
	static long long zrem(redisContext *context, const std::string &key, std::string strMember);
	static void zrangeByScore(redisContext *context, const std::string &key, long long min, long long max, std::vector<std::string>& vecStr);
	static void zrangeByIndex(redisContext *context, const std::string &key, long long min, long long max, std::vector<std::string>& vecStr);
	static std::string zscore(redisContext *context, const std::string &key, const std::string &strMember);

	static VCZScore zrevrangeWithScores(redisContext* context,const std::string& key, int start, int stop);

	static std::map<std::string,int> zrangeByScore(redisContext* context, const std::string& key, long long min, long long max, long long offset = 0, long long count = 0);

	static void expire(redisContext* context,const std::string &key, int seconds);

	static long long ttl(redisContext* context, const std::string &key);

	static void test(redisContext* context, int iRand);

	static void del(redisContext* context, const std::string &key);


	static bool exists(redisContext* context, const std::string &key);

	static int lpush(redisContext *context, const std::string &key, const std::vector<std::string>& vecStr);
	static int lpush(redisContext *context, const std::string &key, const std::string &strValue);

	static int rpush(redisContext *context, const std::string &key, std::vector<std::string>& vecStr);
	static int rpush(redisContext *context, const std::string &key, const std::string &strValue);

	static int getListLen(redisContext *context, const std::string &key);
	static bool lrange(redisContext *context, const std::string &key, int iStart,int iEnd, std::vector<std::string>& vecStr);
	static bool ltrim(redisContext *context, const std::string &key, int iStart, int iEnd);

	static int sadd(redisContext *context, const std::string &key, const std::string &strValue);
	static int sadd(redisContext *context, const std::string &key, std::vector<std::string>& vecStr);
	static int srem(redisContext *context, const std::string &key, const std::string &strValue);
	static int scard(redisContext *context, const std::string &key);
	static bool spop(redisContext *context, const std::string &key,int iRandCount, std::vector<std::string>& vecStr);
	static bool smembers(redisContext *context, const std::string &key, std::vector<std::string>& vecStr, int iNeedNewLen);
	static void CutString(std::vector<std::string>& vcStr, std::string& str, const char *cFlag);
private:
	static void * FinalCmd(redisContext * context, const char *cmd, const std::string & key, const std::vector<std::string>& vecStr);
	static void excuteCmd(redisContext* context, std::string cmd);
};

#endif