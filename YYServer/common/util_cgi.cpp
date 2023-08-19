#include "util_cgi.h"
#include "make_log.h"
#include <fstream>
#include "JSON.h"
#include <string.h>
#include <string>
#include "redis.h"
#include <utility>
#include <sstream>
using namespace std;
int trim_space(string &inbuf)
{
	int i = 0;
	int j = inbuf.size()-1;
	while (inbuf[i] == ' ' && inbuf[i] != '\0') i++;
	while (inbuf[j] == ' ' && j>i) j--;
	inbuf = inbuf.substr(i,j-i+1);
	return 0;
}


char* memstr(char* fulldata, int full_data_len, char* substr)
{
	if (fulldata == NULL || full_data_len <= 0 || substr == NULL)
	{
		return NULL;
	}

	if (*substr == '\0')
	{
		return NULL;
	}

	string full = fulldata;
	string str = substr;
	int index = full.find(substr, 0);
	char* out = fulldata;
	out += index;
	return out;
}


int query_parse_key_value(const char* query, unordered_map<string, string>& map)
{
	if (query == NULL)
	{
		LOG("cgi", "util", "[ERROR]query is NULL\n");
		return -1;
	}
	string aa;
	stringstream ss(query);
	while (getline(ss, aa, '&'))
	{
		int pos = aa.find('=');
		if (pos == string::npos)
		{
			LOG("cgi", "util", "[ERROR]query_parse_key_value error\n");
			return -1;
		}
		map.insert({aa.substr(0,pos),aa.substr(pos+1,aa.size()-pos-1)});
	}
	return 0;

}


int get_file_suffix(const char* file_name, string& suffix)
{
	if (file_name == NULL)
	{
		LOG("cgi", "util", "[ERROR]get_file_suffix error filename==null\n");
		return -1;
	}
	string file = file_name;
	int pos = file.find('.');
	if (pos == string::npos)
	{
		LOG("cgi", "util", "[ERROR]get_file_suffix error\n");
		return -1;
	}
	suffix = file.substr(pos+1,file.size()-pos-1);
	return 0;
}


int str_replace(string & strSrc, string strFind, string strReplace)
{
	if (strSrc.empty() || strFind.empty() || strReplace.empty())
	{
		LOG("cgi", "util", "[ERROR]strSrc or strFind or strReplace is null\n");
		return -1 ;
	}

	

	int pos = strSrc.find(strFind,0);
	if (pos == string::npos)
	{
		LOG("cgi", "util", "[ERROR]cannot find %s in %s\n", strFind, strSrc);
		return -1;
	}
	string out= strSrc.substr(0,pos);
	out += strReplace;
	out += strSrc.substr(pos + strFind.size(), strSrc.size() - 1 - pos - strFind.size());
	strSrc = out;
	return 0;
}


int verify_token(string user, string token)
{
	int ret = 0;
	
	YYjson j;
	fstream fp("./conf/cfg.json",ios::in);
	fp.seekg(0, ios::end);
	int len = fp.tellg();
	fp.seekg(0, ios::beg);
	char buff[len + 1];
	buff[len] = '\0';
	fp.read(buff,len);
	json_value v;
	json_value* redis_all;
	ret=j.parse(&v,buff);
	if (ret != 0)
	{
		LOG("cgi", "util", "[ERROR]json parse error\n");
		return -1;
	}
	redis_all =j.find_object(&v,"redis",5);
	string redis_ip= j.find_object(redis_all, "ip", 2)->u.s.s;
	string redis_port = j.find_object(redis_all, "port", 4)->u.s.s;
	string redis_passwd = j.find_object(redis_all, "password", 8)->u.s.s;
	redis r(redis_ip.c_str(), atoi(redis_port.c_str()));
	ret=r.connect();
	

	//����redis���ݿ�
	if (ret != 0)
	{
		LOG("cgi", "util", "[ERROR]redis connected error\n");
		return -1;
	}

	//��ȡuser��Ӧ��value
	string tmp_token;
	ret = r.str_get(user, tmp_token);
	if (ret != 0)
	{
		LOG("cgi", "util", "[ERROR]redis query error\n");
		return -1;
	}
	if (strcmp(token.c_str(), tmp_token.c_str()) != 0) //token�����
	{
		return 0;
	}
	return 1;
}



int connect_Mysql(YY::YYMysql& my)
{
	YYjson j;
	int ret;
	fstream fp("./conf/cfg.json", ios::in);
	fp.seekg(0, ios::end);
	int len = fp.tellg();
	fp.seekg(0, ios::beg);
	char buff[len+1];
	buff[len] = '\0';
	fp.read(buff, len);
	json_value v;
	json_value* mysql;
	ret = j.parse(&v, buff);
	if (ret != 0)
	{
		LOG("cgi", "util", "[ERROR]json parse error\n");
		return -1;
	}
	 j.parse(&v,buff);
	 mysql = j.find_object(&v, "mysql", 5);
	 string ip = j.find_object(mysql, "ip", 2)->u.s.s;
	 string db = j.find_object(mysql, "database", 8)->u.s.s;
	 string user = j.find_object(mysql, "user", 4)->u.s.s;
	 string pwd = j.find_object(mysql, "password", 8)->u.s.s;
	 int port = atoi(j.find_object(mysql, "port", 4)->u.s.s);

	 my.Init();
	 my.SetReconnect(1);
	 if (my.Connect(ip.c_str(), user.c_str(), pwd.c_str(), db.c_str(), port, 0))
	 {
		 return 0;
	 }
	 else
	 {
		 LOG("cgi", "util", "[ERROR]Mysql connect %s:%d %s %s %s error\n", ip.c_str(), port, user.c_str(), pwd.c_str(), db.c_str());
		 return -1;
	 }
}