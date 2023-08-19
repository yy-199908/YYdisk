#ifndef REDIS_H
#define REDIS_H
#include <string>
#include <vector>
#include <unordered_map>
#include <stdio.h>
#include <stdlib.h>
#include <hiredis.h>
#include <utility>
#include "make_log.h"
class redis
{
public:
	redis() {}
	redis(const char *ip, int port) :redis_server_ip(ip), port(port) {}
	int connect();
	int connect(const char* pwd);
	~redis() {
		if (c) redisFree(c);
	}
	//选择数据库
	int select_db(unsigned int db_no);
	//清空当前数据库
	int flush_db();
	//判断key是否存在
	int is_key(std::string key);
	//删除一个key
	int del_key(std::string key);
	//设置一个key的自动删除时间
	int set_key_lifetime(std::string key, time_t delete_time);
	//获取库中匹配pattern的所有key
	int show_keys(std::string patter, std::vector<std::string>& keys);
	//执行封装好的批量命令
	int redis_do(std::vector<std::string> cmds);
	//执行单条命令
	int redis_do(std::string cmd);

	//hash表操作
	//创建一个hash表，若该表已存在，则覆盖
	int hash_create(std::string key,std::vector<std::pair<std::string ,std::string>> kv);
	//在hash表中添加一条key-value数据
	int hash_append(std::string key,std::string field,std::string value );
	//在hash表中添加多条数据
	int hash_append(std::string key,std::vector<std::pair<std::string ,std::string>> kv);
	//从hash表中取出一个键值对
	int hash_get(std::string key, std::string field, std::string& value);
	//从hash表中删除指定字段
	int hash_del(std::string key, std::string field);
	//给定hash表filed对应的value自增或自减,num>0为加，num<0为减
	int hash_number(std::string key, std::string field, int num);
	
	
	
	
	//list操作
	
	//插入一个链表头部
	int list_rappend(std::string key, std::string str);
	//插入一个链表尾部
	 int list_append(std::string key, std::string str);
	//批量插入链表头部
	 int list_rappend(std::string key, std::vector<std::string> strs);
	 //批量插入链表尾部
	 int list_append(std::string key, std::vector<std::string> strs);
	 //得到list中的元素个数
	 int list_len(std::string key);
	 //按给定的范围截断list，begin 0，end -1
	 int list_trim(std::string key,int begin,int end);
	 //获取链表中的数据
	 int list_range(std::string key, int begin, int end, std::vector<std::string>& strs);


	 //string操作

	 //设置一个string类型
	 int str_set(std::string key,std::string value);
	 int str_set(std::pair<std::string, std::string> kv);
	 //设置string类型并设置过期时间
	 int str_set_timeout(std::string key, std::string value, unsigned int seconds);
	 //获取str
	 int str_get(std::string key, std::string& str);
	 //同时插入多个string字符串
	 int str_mset(std::vector<std::pair<std::string, std::string>> kv);
	 //同时 获取多个字符串
	 int str_mget(std::vector<std::string> keys,std::vector<std::pair<std::string, std::string>> &kv);




	 //SortedSet操作
	 //添加成员，若set不存在则创建
	 int zset_add(std::string key,long score,std::string member);
	 //批量添加成员
	 int zset_add(std::string key, std::vector<std::pair<std::string ,long>>kv);
	 //删除指定的成员
	 int zset_del(std::string key, std::string member);
	 //删除所有成员
	 int zset_del_all(std::string key);
	 //降序返回有序集合元素
	 int zset_rerange(std::string key,int begin,int end,std::vector<std::string> &member);
	 //升序返回有序集合元素
	  int zset_range(std::string key,int begin,int end,std::vector<std::string> &member);
	 //指定有序集合中某元素加减
	 int zset_score(std::string key, std::string member, long add);
	 //获取集合长度
	 int zset_len(std::string key);
	 //获取一个member的score
	 int zset_get_score(std::string key, std::string member,long &a);
	 //判断某个成员是否存在,0不存在，1存在，-1出错
	 int zset_exit(std::string key, std::string member);
	 //批量将指定的zset表，对应的成员值改变
	 int zset_score(std::string key,std::vector<std::pair<std::string,long>> kv);
	 //返回zset表所有member和 值
	 int zset_get_all(std::string key, std::vector<std::pair<std::string, long>>& kv);


private:

	std::string  redis_server_ip = "127.0.0.1";
	int  port = 6379;
	redisContext* c;
	redisReply* reply;

	//辅助函数
	std::string hmset_command(std::string key, std::vector<std::pair<std::string, std::string>> kv);

};

#endif

