#include "redis.h"
#include <string>
#include <iostream>
using namespace std;

int redis::connect(const char *pwd)
{
	int ret;
	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	c = redisConnectWithTimeout(redis_server_ip.c_str(), port, timeout);
	if (c == NULL || c->err)
	{
		if (c)
		{
			LOG("database","redis","[ERROR]connect %s:%d err: %s\n", redis_server_ip.c_str(),port, c->errstr);
			redisFree(c);
		}
		else {

			LOG("database", "redis","[ERROR]Connection error: can't allocate redis context\n");
		}
		return -1;
	}

	reply = (redisReply*)redisCommand(c, "AUTH %s", pwd);
	{
		if (reply->type == REDIS_REPLY_ERROR) {
			LOG("database", "redis","[ERROR] auth redis failed! %s\n",reply->str);
			freeReplyObject(reply);
			redisFree(c);
			return -1;
		}
	}
	LOG("database", "redis", "[INFO]connect %s:%d success\n", redis_server_ip.c_str(), port, c->errstr);
	return 0;
}


int redis::connect()
{
	int ret;
	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	c = redisConnectWithTimeout(redis_server_ip.c_str(), port, timeout);
	if (c == NULL || c->err)
	{
		if (c)
		{
			LOG("database", "redis", "[ERROR]connect %s:%d err: %s\n", redis_server_ip.c_str(), port, c->errstr);
			redisFree(c);
		}
		else {

			LOG("database", "redis", "[ERROR]Connection error: can't allocate redis context\n");
		}
		return -1;
	}

	LOG("database", "redis", "[INFO]connect %s:%d success\n", redis_server_ip.c_str(), port, c->errstr);
	return 0;
}






int redis::select_db( unsigned int db_no)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR][ERROR]miss connection\n\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c,"select %d",db_no);
	if (reply == NULL)
	{
		LOG("database", "redis", "[ERROR]Select database %d error! %s\n", db_no, c->errstr);
		return -1;

	}
	LOG("database", "redis", "[INFO]Select database %d success!\n", db_no);
	freeReplyObject(reply);
	return 0;
}


int redis::flush_db()
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR][ERROR]miss connection\n\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "FLUSHDB");
	if (reply == NULL)
	{
		LOG("database", "redis", "[ERROR]Clear all data error\n");
		return -1;

	}
	LOG("database", "redis", "[INFO]Clear all data sucess!!\n");
	freeReplyObject(reply);
	return 0;
}

int redis::is_key(string key)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR][ERROR]miss connection\n\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "EXISTS %s",key.c_str());
	if (reply ->type!= REDIS_REPLY_INTEGER)
	{
		LOG("database", "redis", "[ERROR]is key exist get wrong type!\n");
		freeReplyObject(reply);
		return -1;

	}
	if (reply->integer == 1) {

		freeReplyObject(reply);
		return  1;
	}
	freeReplyObject(reply);
	return 0;
	
}


int  redis::del_key(string key)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR][ERROR]miss connection\n\n");
		return -1;
	}
	if (is_key(key) != 1)
	{
		LOG("database", "redis", "[ERROR]no key %s exists!",key.c_str());
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
	if (reply->type != REDIS_REPLY_INTEGER) {
		
		LOG("database", "redis", " [ERROR]DEL key %s error! %s\n", key.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}

	if (reply->integer > 0) {
		freeReplyObject(reply);
		return 0;
	}
	freeReplyObject(reply);
	return -1;
}


int redis::set_key_lifetime(string key, time_t delete_time)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR][ERROR]miss connection\n\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "EXPIREAT %s %d", key.c_str(), delete_time);
	if (reply->type != REDIS_REPLY_INTEGER) {
		LOG("database", "redis", "[ERROR]Set key:%s delete time error! %s\n", key.c_str(), c->errstr);
		freeReplyObject(reply);
		return  -1;
	}
	if (reply->integer == 1) {
		freeReplyObject(reply);
		return  0;
	}
	
	freeReplyObject(reply);
	return  -1;
}

int redis::show_keys(string pattern, vector<string>& keys)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	
	reply = (redisReply*)redisCommand(c, "keys %s", pattern.c_str());
	if (reply->type != REDIS_REPLY_ARRAY) {
		
		LOG("database", "redis", "[ERROR]show all keys error! %s\n", c->errstr);
		freeReplyObject(reply);
		return  -1;
	}

	string key;
	for (int i = 0; i < reply->elements; ++i) {
		key = reply->element[i]->str;
		keys.push_back(key);
	}
	freeReplyObject(reply);
	return  0;
}



int redis::redis_do(vector<string> cmds)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	int ret;
	for (int i = 0; i < cmds.size(); ++i) {
		ret = redisAppendCommand(c, cmds[i].c_str());
		if (ret != REDIS_OK) {
			
			LOG("database", "redis", "[ERROR]Append Command: %s error! %s\n", cmds[i].c_str(), c->errstr);
			return -1;
			
		}
		ret = 0;
	}

	for (int i = 0; i < cmds.size(); ++i) {
		ret = redisGetReply(c, (void**)&reply);
		if (ret != REDIS_OK) {
			LOG("database", "redis", "[ERROR]Commit Command:%s error %s\n", cmds[i].c_str(), c->errstr);
			freeReplyObject(reply);
			return -1;
		}
		freeReplyObject(reply);
		ret = 0;
	}
	return 0;
}


int redis::redis_do(string cmd)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}

	reply = (redisReply*)redisCommand(c, cmd.c_str());
	if (reply == NULL) {
		LOG("database", "redis", "[ERROR]Command : %s error!%s\n", cmd.c_str(), c->errstr);
		return  -1;
	}

	freeReplyObject(reply);

	return 0;
}



string redis::hmset_command(string  key, vector<pair<string, string>> kv)
{
	string cmd="hmset ";
	cmd += key;
	cmd += " ";
	for (int i = 0; i < kv.size(); i++)
	{
		cmd += kv[i].first;
		cmd += " ";
		cmd += kv[i].second;
		cmd += " ";
	}
	return cmd.substr(0,cmd.size()-1);

}


int redis::hash_create(string key, vector<pair<string,string>> kv)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	string cmd = hmset_command(key,kv);


	if (cmd.size()==0) {
		LOG("database", "redis", "[ERROR]create hash table %s error\n", key.c_str());
		return -1;
		
	}
	reply = (redisReply*)redisCommand(c, cmd.c_str());
	if (strcmp(reply->str, "OK") != 0) {
		LOG("database", "redis", "[ERROR]Create hash table %s error! %s %s\n", key.c_str(), reply->str, c->errstr);
		freeReplyObject(reply);
		return  -1;
	}
	return 0;
}


int redis::hash_append(string key, string field, string value)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}

	reply = (redisReply*)redisCommand(c, "hset %s %s %s", key.c_str(), field.c_str(), value.c_str());
	if (reply == NULL || reply->type != REDIS_REPLY_INTEGER) {
		LOG("database", "redis", "[ERROR]hset %s %s %s error %s\n", key.c_str(), field.c_str(), value.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}
	freeReplyObject(reply);
	return 0;
}

int redis::hash_append(string key, vector<pair<string, string>> kv)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	int ret = 0;
	for (int i = 0; i < kv.size(); i++)
	{
		ret=hash_append(key,kv[i].first,kv[i].second);
		if (ret == -1)
			return -1;
	}
	return 0;
}


int redis::hash_get(string key, string field, string& value)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}

	reply = (redisReply*)redisCommand(c, "hget %s %s", key.c_str(), field.c_str());
	if (reply == NULL || reply->type != REDIS_REPLY_STRING) {
		LOG("database", "redis", "[ERROR]hget %s %s  error %s\n", key.c_str(), field.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}
	value = reply->str;
	freeReplyObject(reply);
	return 0;
}


int redis::hash_del(string key, string field)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "hdel %s %s", key.c_str(), field.c_str());
	if (reply->integer != 1)
	{
		LOG("database", "redis", "[ERROR]hdel %s %s error %s\n", key.c_str(), field.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}
	freeReplyObject(reply);
	return 0;

}


int redis::hash_number(string key, string field, int num)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	
	
		reply = (redisReply*)redisCommand(c, "HINCRBY %s %s %d", key.c_str(), field.c_str(), num);
	
	
	

	if (reply == NULL) {
		LOG("database", "redis", "[ERROR]hash_number %s %s %d error %s\n", key.c_str(), field.c_str(),num, c->errstr);
		freeReplyObject(reply);
		return -1;
	}

	freeReplyObject(reply);
	return 0;

}


//插入一个链表头部
int redis::list_rappend(string key, string str)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "LPUSH %s %s", key.c_str(), str.c_str());
	//rop_test_reply_type(reply);	
	if (reply->type != REDIS_REPLY_INTEGER) {
		LOG("database", "redis", "[ERROR] LPUSH %s %s error! %s\n", key.c_str(), str.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}

	freeReplyObject(reply);
	return 0;


}
//插入一个链表尾部
int redis::list_append(string key, string str)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "RPUSH %s %s", key.c_str(), str.c_str());
	//rop_test_reply_type(reply);	
	if (reply->type != REDIS_REPLY_INTEGER) {
		LOG("database", "redis", "[ERROR]RPUSH %s %s error! %s\n", key.c_str(), str.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}

	freeReplyObject(reply);
	return 0;
}
//批量插入链表头部
int redis::list_rappend(string key, vector<string> strs)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	int ret = 0;
	for (int i = 0; i < strs.size(); i++)
	{
		ret = list_rappend(key, strs[i]);
		if (ret == -1)
			return -1;
	}
	return 0;
}

//批量插入链表尾部
int redis::list_append(string key, vector<string> strs)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	int ret = 0;
	for (int i = 0; i < strs.size(); i++)
	{
		ret = list_append(key, strs[i]);
		if (ret == -1)
			return -1;
	}
	return 0;
}

int  redis::list_len(string key)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}

	

	reply = (redisReply*)redisCommand(c, "LLEN %s", key.c_str());
	if (reply->type != REDIS_REPLY_INTEGER) {
		LOG("database", "redis", "[ERROR]LLEN %s error %s\n", key.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}

	freeReplyObject(reply);
	return reply->integer;
}


int redis::list_trim(string key, int begin, int end)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}

	reply = (redisReply*)redisCommand(c, "LTRIM %s %d %d", key.c_str(), begin, end);
	if (reply->type != REDIS_REPLY_STATUS) {
		LOG("database", "redis", "[ERROR]LTRIM %s %d %d error! %s\n", key.c_str(), begin, end, c->errstr);
		freeReplyObject(reply);
		return -1;
	}

	freeReplyObject(reply);
	return 0;
}


int redis::list_range(string key, int begin, int end, vector<string>& strs)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "LRANGE %s %d %d", key.c_str(), begin, end);
	if (reply->type != REDIS_REPLY_ARRAY || reply->elements == 0) {
		LOG("database", "redis", "[ERROR]LRANGE %s %d %d  error! %s\n", key.c_str(),begin,end, c->errstr);
		freeReplyObject(reply);
		return -1;
	}
	string str;
	for (int i = 0; i < reply->elements; i++)
	{
		str = reply->element[i]->str;
		strs.push_back(str);
	}
	freeReplyObject(reply);
	return 0;

}



int redis::str_set(string key, string value)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "SET %s %s ", key.c_str(), value.c_str());
	if (reply==NULL || strcmp(reply->str,"OK")!=0) 
	{
		LOG("database", "redis", "[ERROR]SET %s %s error %s ", key.c_str(), value.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}
	freeReplyObject(reply);
	return 0;
}
int redis::str_set(pair<string, string> kv)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	return str_set(kv.first,kv.second);
}



int redis::str_set_timeout(string key, string value, unsigned int seconds)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	
	reply = (redisReply*)redisCommand(c, "setex %s %u %s", key.c_str(), seconds, value.c_str());
	//rop_test_reply_type(reply);
	if (reply==NULL or strcmp(reply->str, "OK") != 0) {
		LOG("database", "redis", "[ERROR]setex %s %u %s error %s  %s\n", key.c_str(), seconds, value.c_str(), c->errstr,reply->str);
		freeReplyObject(reply);
		return -1;
	}
	freeReplyObject(reply);
	return 0;

}



int redis::str_get(string key, string& str)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "get %s", key.c_str());
	//rop_test_reply_type(reply);
	if (reply == NULL || reply->type!= REDIS_REPLY_STRING) {
		LOG("database", "redis", "[ERROR]get %s error %s", key.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}
	str = reply->str;
	freeReplyObject(reply);
	return 0;
}

int redis::str_mset(vector<pair<string, string>> kv)
{
	string cmd = "MSET ";
	for (int i = 0; i < kv.size(); i++)
	{
		cmd += kv[i].first;
		cmd += " ";
		cmd += kv[i].second;
		cmd += " ";
	}
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c,cmd.c_str());
	//rop_test_reply_type(reply);
	if (reply == NULL or strcmp(reply->str, "OK") != 0) {
		LOG("database", "redis", "[ERROR] mset error %s", c->errstr);
		freeReplyObject(reply);
		return -1;
	}
	freeReplyObject(reply);
	return 0;
}
int redis::str_mget(vector<string> keys, vector<pair<string, string>>& kv)
{
	string cmd = "MGET ";
	for (int i = 0; i < keys.size(); i++)
	{
		cmd += keys[i];
		cmd += " ";
	}
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, cmd.c_str());
	//rop_test_reply_type(reply);
	if (reply->type!= REDIS_REPLY_ARRAY) {
		LOG("database", "redis", "[ERROR]mget error %s", c->errstr);
		freeReplyObject(reply);
		return -1;
	}
	pair<string, string>a;
	for (int i = 0; i < reply->elements; i++)
	{
		a.first = keys[i];
		a.second = reply->element[i]->str;
		kv.push_back(a);
	}
	freeReplyObject(reply);
	return 0;
}



int redis::zset_add(string key, long score, string member)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "ZADD %s %ld %s", key.c_str(), score, member.c_str());
	if (reply->integer != 1)
	{
		LOG("database", "redis", "[ERROR] ZADD: %s %s error %s %s\n", key.c_str(), member.c_str(), reply->str, c->errstr);
		freeReplyObject(reply);
		return -1;

	}
	freeReplyObject(reply);
	return 0;
}


int redis::zset_add(std::string key, std::vector<std::pair<std::string, long>>kv)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	int ret = 0;
	for (int i = 0; i < kv.size(); i++)
	{
		ret = zset_add(key,kv[i].second,kv[i].first);
		if (ret == -1)
			return -1;
	}
	return 0;
}


int redis::zset_del(string key, string member)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "ZREM %s %s", key.c_str(), member.c_str());
	if (reply->integer != 1)
	{
		LOG("database", "redis", "[ERROR]ZREM %s %s error %s\n", key.c_str(), member.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;

	}
	freeReplyObject(reply);
	return 0;
}



int redis::zset_del_all(string key)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}

	//执行命令
	reply = (redisReply*)redisCommand(c, "ZREMRANGEBYRANK %s 0 -1", key.c_str());
	//rop_test_reply_type(reply);

	if (reply->type != REDIS_REPLY_INTEGER) //如果不是整型
	{
		LOG("database", "redis", "[ERROR]ZREMRANGEBYRANK: %s error %s %s\n", key.c_str(), reply->str, c->errstr);
		freeReplyObject(reply);
		return -1;
	}
	freeReplyObject(reply);
	return 0;
}


int redis::zset_rerange(string key, int begin, int end, vector<string>& member)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "ZREVRANGE %s %d %d", key.c_str(), begin, end);
	if (reply->type != REDIS_REPLY_ARRAY) //如果返回不是数组
	{
		LOG("database", "redis", "[ERROR]ZREVRANGE %s  error! %s\n", key.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}
	string out;
	for (int i = 0; i < reply->elements; i++)
	{
		out = reply->element[i]->str;
		member.push_back(out);
	}

	freeReplyObject(reply);
	return 0;
}



int redis::zset_range(std::string key, int begin, int end, std::vector<std::string>& member)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "ZRANGE %s %d %d", key.c_str(), begin, end);
	if (reply->type != REDIS_REPLY_ARRAY) //如果返回不是数组
	{
		LOG("database", "redis", "[ERROR]ZRANGE %s  error! %s\n", key.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}
	string out;
	for (int i = 0; i < reply->elements; i++)
	{
		out = reply->element[i]->str;
		member.push_back(out);
	}

	freeReplyObject(reply);
	return 0;
}
int redis::zset_score(string key, string member, long add)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "ZINCRBY %s %d %s", key.c_str(), add, member.c_str());
	if (reply->type != REDIS_REPLY_STRING) //如果返回不是数组
	{
		LOG("database", "redis", "[ERROR]ZINCRBY %s %d %s error %s", key.c_str(), add, member.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}
	freeReplyObject(reply);
	return 0;
}



int redis::zset_len(string key)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "ZCARD %s", key.c_str());
	if (reply->type != REDIS_REPLY_INTEGER)
	{
		LOG("database", "redis", "[ERROR]ZCARD %s error %s\n", key.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}

	int out = reply->integer;
	freeReplyObject(reply);
	return out;
}



int redis::zset_get_score(string key, string member,long &a)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "ZSCORE %s %s", key.c_str(),member.c_str());
	if (reply->type != REDIS_REPLY_STRING)
	{
		LOG("database", "redis", "[ERROR]ZSCORE %s %s error %s", key.c_str(), member.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}

	a = atoi(reply->str);
	freeReplyObject(reply);
	return 0;
}

int redis::zset_exit(string key, string member)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "zlexcount %s [%s [%s", key.c_str(), member.c_str(), member.c_str());
	if (reply->type != REDIS_REPLY_INTEGER)
	{
		LOG("database", "redis", "[ERROR]zlexcount %s %s error %s", key.c_str(), member.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}

	int out = reply->integer;
	freeReplyObject(reply);
	return out;
}


int redis::zset_score(string key, vector<pair<string, long>> kv)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	int ret = 0;
	for (int i = 0; i < kv.size(); i++)
	{
		ret = zset_score(key, kv[i].first, kv[i].second);
		if (ret == -1)
			return -1;
	}
	return 0;
}

int redis::zset_get_all(std::string key, std::vector<std::pair<std::string, long>>& kv)
{
	if (c == NULL)
	{
		LOG("database", "redis", "[ERROR]miss connection\n");
		return -1;
	}
	reply = (redisReply*)redisCommand(c, "ZRANGE %s 0 -1 WITHSCORES",key.c_str());
	if (reply->type != REDIS_REPLY_ARRAY)
	{
		LOG("database", "redis", "[ERROR]zlexcount %s 0 -1 error %s", key.c_str(), c->errstr);
		freeReplyObject(reply);
		return -1;
	}
	string mem;
	long val;
	for (int i = 0; i < reply->elements; i+=2)
	{
		mem = reply->element[i]->str;
		val = atoi(reply->element[i + 1]->str);
		kv.push_back(make_pair(mem, val));
	}
	freeReplyObject(reply);
	return 0;
}













