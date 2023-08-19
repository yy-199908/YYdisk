/*�û���¼cgi������*/
#include <stdlib.h>
#include <string>
#include <string.h>
#include "make_log.h"
#include "util_cgi.h"
#include "YYMysql.h"
#include "YYDATA.h"
#include "JSON.h"
#include "des.h"
#include "MD5.h"
#include "base64.h"
#include "redis.h"
#include <vector>
#include <unordered_map>
#include <utility>
#include <unistd.h>
#include <sys/time.h>
#include <fstream>
#include "fcgi_config.h"
#include "fcgi_stdio.h"

using namespace std;
using namespace YY;



//ȥ���ݿ�����û�����passwd�Ƿ�ƥ��
//����ֵ ��-1 �������� -2 user�зǷ��ַ� 0������û������� 1 ��¼�ɹ�
int check_user_password(const char* buff)
{
	//��ȡjson�����ļ�
	YYjson j;
	md5 m;
	json_value v;
	YYMysql my;
	fstream fp("./conf/cfg.json", ios::in);
	if (!fp.is_open())
	{
		LOG("cgi", "login", "[ERROR]open ./conf/cfg.json failed!\n");
		return -1;
	}
	fp.seekg(0, ios::end);
	int len = fp.tellg();
	fp.seekg(0,ios::beg);

	char json[len + 1];
	json[len] = '\0';
	fp.read(json,len);
	j.parse(&v,json);
	json_value* mysql;
	mysql = j.find_object(&v,"mysql",5);
	string ip= j.find_object(mysql, "ip", 2)->u.s.s;
	string database = j.find_object(mysql, "database", 8)->u.s.s;
	string user = j.find_object(mysql, "user", 4)->u.s.s;
	string password = j.find_object(mysql, "password", 8)->u.s.s;
	int port = atoi(j.find_object(mysql, "port", 4)->u.s.s);

	j.json_free(&v);
	//����mysql
	my.Init();
	my.SetReconnect(1);
	if (!my.Connect(ip.c_str(), user.c_str(), password.c_str(), database.c_str(), port, 0))
	{
		LOG("cgi", "login", "[ERROR]Mysql connect failed\n");
		return -1;
	}

	my.Query("set names utf8");

	//��ȡ�û���Ϣ
	unordered_map<string, string> map;
	j.parse(&v,buff);

	map.insert({ "user", j.find_object(&v, "user", 4)->u.s.s });
	map.insert({ "pwd", j.find_object(&v, "pwd", 3)->u.s.s });
	j.json_free(&v);


	//����ע�빥�����û���ֻ���� ��Сд��ĸ�����ֺ�_
	
	string allow = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

	for (char s : map["user"])
	{
		int pos = allow.find(s);
		if (pos == string::npos)
		{
			LOG("cgi", "login", "[ERROR]user %s has a illegal character\n",map["user"].c_str());
			return -2;
		}
			
	}
	string sql="select id from `user` where `name`='";
	sql += map["user"];
	sql += "' and `password`='";
	sql += map["pwd"];
	sql += "'";
	YYROWS rows = my.GetResult(sql.c_str());
	
	my.Close();
	if (rows.size() == 1)
	{
		LOG("cgi", "login", "[info]user %s login!\n", map["user"].c_str());
		return 1;
	}
	else {
		return 0;
	}


}






//�����¼�ɹ���������token����redis�У���������ʱ��Ϊ1�죬�����䷵�ؿͻ���


string set_token(const char *buff)
{
	//��ȡjson�����ļ�
	YYjson j;

	json_value v;
	YYMysql my;
	
	fstream fp("./conf/cfg.json", ios::in);
	if (!fp.is_open())
	{
		LOG("cgi", "login", "[ERROR]open ./conf/cfg.json failed!\n");
		return "";
	}
	fp.seekg(0, ios::end);
	int len = fp.tellg();
	fp.seekg(0, ios::beg);
	char json[len + 1];
	json[len] = '\0';
	fp.read(json, len);
	j.parse(&v, json);
	json_value* json_redis;
	json_redis = j.find_object(&v, "redis", 5);
	string ip = j.find_object(json_redis, "ip", 2)->u.s.s;
	int port = atoi(j.find_object(json_redis, "port", 4)->u.s.s);
	string redis_pass = j.find_object(json_redis, "password", 8)->u.s.s;
	
	
	j.json_free(&v);
	//��ȡbuff
	unordered_map<string, string>map;
	j.parse(&v, buff);
	map.insert({ "user", j.find_object(&v, "user", 4)->u.s.s });
	map.insert({ "pwd", j.find_object(&v, "pwd", 3)->u.s.s });
	j.json_free(&v);
	
	
	//����4�������

	int rand_num[4];
	int i = 0;
	srand((unsigned int)time(NULL));//�����������
	string tmp = map["user"];
	for (i = 0; i < 4; i++)
	{
		rand_num[i] = rand() % 1000;
		tmp += to_string(rand_num[i]);
	}
	LOG("cgi","login", "[INFO]token tmp = %s\n", tmp.c_str());
	//���ܣ���des���ܣ���base64���ܣ���md5
	des d;
	char* des_code;
	int des_len= d.des_encode(tmp.c_str(), tmp.size(), &des_code);
	//base64
	base64 ba;
	char ba_code[1024 *3] = { 0 };
	ba.base64_encode((const unsigned char*)des_code, des_len, ba_code);
	//md5
	md5 m;
	string token = m.encode(ba_code, strlen(ba_code));

	//���䱣�浽redis �û���:token
	redis r(ip.c_str(),port);
	if (r.connect() != 0)
	{
		LOG("cgi", "login", "[ERROR]ip: %s, port:%d redis connect error!\n",ip.c_str(),port);
		return "";
	}
	
	if (r.str_set_timeout(map["user"], token,86400) != 0)
	{
		LOG("cgi", "login", "[ERROR]redis store token error!\n");
		return "";
	}
	free(des_code);


	return token;
}











int main()
{
	while (FCGI_Accept() >= 0)
	{
		printf("Content-type: text/html\r\n\r\n");
		int ret;
		string method = getenv("REQUEST_METHOD");
		
		if (method == "GET" or method == "get")
		{
			 
			 string buff = getenv("QUERY_STRING");
			 if (buff.size()==0)
			 {
				 LOG("cgi", "login", "[ERROR]len = 0, No data from standard input\n");
				 continue;
			 }

			 ret = check_user_password(buff.c_str());
			 if (ret == -1)
			 {
				 printf("something error!<p>\n");
			 }
			 else if (ret == -2)
			 {
				 printf("illegal username!<p>\n");
			 }
			 else if (ret == 0)
			 {
				 printf("{\"code\":\"001\"}<p>\n");
				 printf("{\"token\":\"fail\"}<p>\n");;
			 }
			 else
			 {
				 string token = set_token(buff.c_str());
				 if (token.size() != 0)
				 {
					 printf("{\"code\":\"000\"}<p>\n");
					 printf("{\"token\":\"%s\"}<p>\n", token.c_str());;
				 }
				 else
				 {
					 printf("set token error!<p>\n");
				 }
			 }
		}
		else if (method == "POST" or method == "post")
		{
			char *conten_length=getenv("CONTENT_LENGTH");
			int len = atoi(conten_length);
			if (len <= 0)
			{
				LOG("cgi", "login", "[ERROR]len = 0\n");
				continue;
			}
			char buff[len + 1];
			buff[len] = '\0';
			ret=fread(buff,1,len,stdin);//�˴�stdin��fcgi_stdio.h���ض����
			if (ret == 0)
			{
				LOG("cgi","login", "fread(buf, 1, len, stdin) err\n");
				continue;
			}
			
			ret = check_user_password(buff);
			if (ret == -1)
			{
				printf("something error!<p>\n");
			}
			else if (ret == -2)
			{
				printf("illegal username!<p>\n");
			}
			else if (ret == 0)
			{
				printf("{\"code\":\"001\",\n");
				printf("\"token\":\"fail\"}");;

			}
			else
			{
				string token = set_token(buff);
				if (token.size() != 0)
				{
					printf("{\"code\":\"000\",\n");
					printf("\"token\":\"%s\"}", token.c_str());;
				}
				else
				{
					printf("set token error!<p>\n");
				}
			}

		}
		else
		{
			printf("Wrong method!<p>\n");
			LOG("cgi", "login", "[ERROR]Wrong method!\n");
			continue;
		}
		

		



	}
	return 0;
}
