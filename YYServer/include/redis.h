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
	//ѡ�����ݿ�
	int select_db(unsigned int db_no);
	//��յ�ǰ���ݿ�
	int flush_db();
	//�ж�key�Ƿ����
	int is_key(std::string key);
	//ɾ��һ��key
	int del_key(std::string key);
	//����һ��key���Զ�ɾ��ʱ��
	int set_key_lifetime(std::string key, time_t delete_time);
	//��ȡ����ƥ��pattern������key
	int show_keys(std::string patter, std::vector<std::string>& keys);
	//ִ�з�װ�õ���������
	int redis_do(std::vector<std::string> cmds);
	//ִ�е�������
	int redis_do(std::string cmd);

	//hash�����
	//����һ��hash�����ñ��Ѵ��ڣ��򸲸�
	int hash_create(std::string key,std::vector<std::pair<std::string ,std::string>> kv);
	//��hash�������һ��key-value����
	int hash_append(std::string key,std::string field,std::string value );
	//��hash������Ӷ�������
	int hash_append(std::string key,std::vector<std::pair<std::string ,std::string>> kv);
	//��hash����ȡ��һ����ֵ��
	int hash_get(std::string key, std::string field, std::string& value);
	//��hash����ɾ��ָ���ֶ�
	int hash_del(std::string key, std::string field);
	//����hash��filed��Ӧ��value�������Լ�,num>0Ϊ�ӣ�num<0Ϊ��
	int hash_number(std::string key, std::string field, int num);
	
	
	
	
	//list����
	
	//����һ������ͷ��
	int list_rappend(std::string key, std::string str);
	//����һ������β��
	 int list_append(std::string key, std::string str);
	//������������ͷ��
	 int list_rappend(std::string key, std::vector<std::string> strs);
	 //������������β��
	 int list_append(std::string key, std::vector<std::string> strs);
	 //�õ�list�е�Ԫ�ظ���
	 int list_len(std::string key);
	 //�������ķ�Χ�ض�list��begin 0��end -1
	 int list_trim(std::string key,int begin,int end);
	 //��ȡ�����е�����
	 int list_range(std::string key, int begin, int end, std::vector<std::string>& strs);


	 //string����

	 //����һ��string����
	 int str_set(std::string key,std::string value);
	 int str_set(std::pair<std::string, std::string> kv);
	 //����string���Ͳ����ù���ʱ��
	 int str_set_timeout(std::string key, std::string value, unsigned int seconds);
	 //��ȡstr
	 int str_get(std::string key, std::string& str);
	 //ͬʱ������string�ַ���
	 int str_mset(std::vector<std::pair<std::string, std::string>> kv);
	 //ͬʱ ��ȡ����ַ���
	 int str_mget(std::vector<std::string> keys,std::vector<std::pair<std::string, std::string>> &kv);




	 //SortedSet����
	 //��ӳ�Ա����set�������򴴽�
	 int zset_add(std::string key,long score,std::string member);
	 //������ӳ�Ա
	 int zset_add(std::string key, std::vector<std::pair<std::string ,long>>kv);
	 //ɾ��ָ���ĳ�Ա
	 int zset_del(std::string key, std::string member);
	 //ɾ�����г�Ա
	 int zset_del_all(std::string key);
	 //���򷵻����򼯺�Ԫ��
	 int zset_rerange(std::string key,int begin,int end,std::vector<std::string> &member);
	 //���򷵻����򼯺�Ԫ��
	  int zset_range(std::string key,int begin,int end,std::vector<std::string> &member);
	 //ָ�����򼯺���ĳԪ�ؼӼ�
	 int zset_score(std::string key, std::string member, long add);
	 //��ȡ���ϳ���
	 int zset_len(std::string key);
	 //��ȡһ��member��score
	 int zset_get_score(std::string key, std::string member,long &a);
	 //�ж�ĳ����Ա�Ƿ����,0�����ڣ�1���ڣ�-1����
	 int zset_exit(std::string key, std::string member);
	 //������ָ����zset����Ӧ�ĳ�Աֵ�ı�
	 int zset_score(std::string key,std::vector<std::pair<std::string,long>> kv);
	 //����zset������member�� ֵ
	 int zset_get_all(std::string key, std::vector<std::pair<std::string, long>>& kv);


private:

	std::string  redis_server_ip = "127.0.0.1";
	int  port = 6379;
	redisContext* c;
	redisReply* reply;

	//��������
	std::string hmset_command(std::string key, std::vector<std::pair<std::string, std::string>> kv);

};

#endif

