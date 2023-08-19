//��ȡ�û��ļ��б�



#include <stdlib.h>
#include <string>
#include <string.h>
#include "make_log.h"
#include "util_cgi.h"
#include "YYMysql.h"
#include "YYDATA.h"
#include "JSON.h"
#include "MD5.h"
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

//����json�ַ������token��user
int get_token(char* buff, string& user, string& token)
{
	/*json��������
   {
	   "token": "9e894efc0b2a898a82765d0a7f2c94cb",
	   user:xxxx
   }
   */
	YYjson j;
	json_value v;
	j.parse(&v,buff);
	user = j.find_object(&v,"user",4)->u.s.s;
	if (user.empty())
	{
		LOG("cgi","myfiles", "json get user err\n");
		j.json_free(&v);
		return -1;
	}
	token= j.find_object(&v, "token", 5)->u.s.s;
	if (token.empty())
	{
		LOG("cgi", "myfiles", "json get token err\n");
		j.json_free(&v);
		return -1;
	}
	j.json_free(&v);
	return 0;

}


void get_file_count(string user,YYMysql &my)
{
	string sql;
	connect_Mysql(my);
	my.Query("set names utf");
	sql = "select `count` from `user_file_count` where `user`='";
	sql += user;
	sql += "'";
	YYROWS rows = my.GetResult(sql.c_str());
	if (rows.size() == 0)
	{
		LOG("cgi","myfiles"," [ERROR]Mysql query %s error\n", sql.c_str());
		printf("{\
				\"num\":\"0\",\
				\"code\":\"111\"\
				}");//ʧ��
		my.Close();
		
	}
	else
	{
		printf("{\
				\"num\":\"%s\",\
				\"code\":\"110\"\
				}",rows[0][0].data);
		my.Close();
		
	}
}


int get_filelist_token(const char *buff,string &user, string &token,int &start,int& count)
{
	/*json��������
   {
	   "user": "yoyo"
	   "token": xxxx
	   "start": 0
	   "count": 10
   }
   */
	YYjson j;
	json_value v;
	if (j.parse(&v, buff))
	{
		LOG("cgi", "myfiles", "[ERROR]Parse input json error %s!\n",buff);
		j.json_free(&v);
		return -1;
	}
	user = j.find_object(&v,"user",4)->u.s.s;
	token = j.find_object(&v, "token", 5)->u.s.s;
	start = j.find_object(&v, "start", 5)->u.n;
	count = j.find_object(&v, "count", 5)->u.n;
	j.json_free(&v);
	return 0;
}



//��ȡ�û��ļ��б�
//��ȡ�û��ļ���Ϣ 127.0.0.1:80/myfiles&cmd=normal
//������������ 127.0.0.1:80/myfiles?cmd=pvasc
//������������127.0.0.1:80/myfiles?cmd=pvdesc
void get_user_filelist(string cmd, string user, int start, int count,YYMysql &my)
{
	connect_Mysql(my);
	string sql;
	my.Query("set names utf8");
	if (cmd == "normal")
	{
		sql = "select user_file_list.*, file_info.url, file_info.size,file_info.type from file_info, user_file_list where user='";
		sql += user;
		sql += "' and file_info.md5=user_file_list.md5 limit ";
		sql += to_string(start);
		sql += ",";
		sql += to_string(count);
	}
	else if (cmd == "pvasc")//������������
	{
		sql = "select user_file_list.*, file_info.url, file_info.size,file_info.type from file_info, user_file_list where user='";
		sql += user;
		sql += "' and file_info.md5=user_file_list.md5 order by pv asc limit ";
		sql += to_string(start);
		sql += ",";
		sql += to_string(count);;
	}
	else if (cmd == "pvdesc")//������������
	{
		sql = "select user_file_list.*, file_info.url, file_info.size,file_info.type from file_info, user_file_list where user='";
		sql += user;
		sql += "' and file_info.md5=user_file_list.md5 order by pv desc limit ";
		sql += to_string(start);
		sql += ",";
		sql += to_string(count);;
	}
	YYROWS rows = my.GetResult(sql.c_str());
	if (rows.empty())
	{
		LOG("cgi","myfiles", "[ERROR]get_user_filelist(msyql) failed��%s %s\n",cmd.c_str(), sql.c_str());
		my.Close();
		printf("{\"code\": \"015\"}");
		return;
	}
	string json_value="{\"code\":\"000\",\"files\":[";
	
	for (auto row : rows)
	{
		
		string item = "{";
		//-- user	�ļ������û�
		if (row[0].data!= NULL)
		{
			item += "\"user\":\"";
			item += row[0].data;
			item += "\",";
		}

		//-- md5 �ļ�md5
		if (row[1].data != NULL)
		{
			item += "\"md5\":\"";
			item += row[1].data;
			item += "\",";
		}
		//-- createtime �ļ�����ʱ��
		if (row[2].data != NULL)
		{
			item += "\"time\":\"";
			item += row[2].data;
			item += "\",";
		}

		//-- filename �ļ�����
		if (row[3].data != NULL)
		{
			item += "\"filename\":\"";
			item += row[3].data;
			item += "\",";
		}

		//-- shared_status ����״̬, 0Ϊû�й����� 1Ϊ����
		if (row[4].data != NULL)
		{
			item += "\"share_status\":";
			item += row[4].data;
			//LOG("cgi", "myfiles", "[INFO] %s\n", row[4].data);
			item += ",";
		}

		//-- pv �ļ���������Ĭ��ֵΪ0������һ�μ�1
		if (row[5].data != NULL)
		{
			item += "\"pv\":";
			item += row[5].data;
			item += ",";
		}

		//-- url �ļ�url
		if (row[6].data != NULL)
		{
			item += "\"url\":\"";
			item += row[6].data;
			item += "\",";
		}

		//-- size �ļ���С, ���ֽ�Ϊ��λ
		if (row[7].data != NULL)
		{
			item += "\"size\":";
			item += row[7].data;
			item += ",";
		}

		//-- type �ļ����ͣ� png, zip, mp4����
		if (row[8].data != NULL)
		{
			item += "\"type\":\"";
			item += row[8].data;
			item += "\"}";
		}
		item += ",";
		json_value += item;

	}
	json_value = json_value.substr(0, json_value.size()-1);
	json_value += "]}";
	LOG("cgi","myfiles", "[INFO] %s\n", json_value.c_str());
	printf(json_value.c_str());
	my.Close();

}




int main()
{
	unordered_map<string, string> map;
	string user;
	string token;
	YYMysql my;
	
	while (FCGI_Accept() >= 0)
	{
		map.clear();
		user.clear();
		token.c_str();
		printf("Content-type: text/html\r\n\r\n");
		//�鿴�ļ��б���urlһ����get����
		string query = getenv("QUERY_STRING");
		query_parse_key_value(query.c_str(),map);
		
		char* content_length = getenv("CONTENT_LENGTH");
		
		int len = atoi(content_length);
		if (len <= 0)
		{
			printf("No data from standard input.<p>\n");
			LOG("cgi","myfiles", "[ERROR]len = 0, No data from standard input\n");
			continue;
		}
		char buff[len + 1];
		buff[len] = '\0';
		int ret = 0;
		ret = fread(buff,1,len,stdin);
		
		if (ret == 0)
		{
			printf("fread(buf, 1, len, stdin) err<p>\n");
			LOG("cgi","myfiles", "[ERROR]fread(buf, 1, len, stdin) err\n");
			continue;
		}
		if (map["cmd"] == "count")
		{
			
			
			if (get_token(buff, user, token) != 0)//����user��token
				continue;
		
			ret = verify_token(user,token);//ȥredis�Ա�token
			
			if (ret == 1)
			{
				 get_file_count(user,my);
			}
			else
			{
				printf("{\"code\":\"111\"}");//ʧ��
			}
				

		}
		else
		{

			//��ȡ�û��ļ���Ϣ 127.0.0.1:80/myfiles&cmd=normal
			//������������ 127.0.0.1:80/myfiles?cmd=pvasc
			//������������127.0.0.1:80/myfiles?cmd=pvdesc
			int start;
			int count;
			if (get_filelist_token(buff, user, token, start, count) != 0)//����user��token

			{
				printf("get_filelist_token failed<p>");
				continue;
			}
			ret = verify_token(user, token);
			LOG("cgi", "myfiles", "[TEST]verify_token %s %s %d %d\n", user.c_str(),token.c_str(),start,count);
			if (ret == 1)
			{
				get_user_filelist(map["cmd"],user,start,count,my );
			}
			else
			{
				printf("{\"code\":\"111\"}");
			}

		}
		


	}
}