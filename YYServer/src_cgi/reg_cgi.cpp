
//ע��cgi������


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

int user_register(char* buff)
{

    //��ȡ�����ļ������mysql��Ϣ
    YYjson j;
    md5 m;
    fstream fp("./conf/cfg.json", ios::in);
    if (!fp.is_open())
    {
        LOG("cgi", "reg", "[ERROR]open cfg.json failed!\n");
        return -1;
    }
    fp.seekg(0,ios::end);
    int len=fp.tellg();
    fp.seekg(0, ios::beg);
    char json[len+1];
    fp.read(json, len);
    json[len] = '\0';
    json_value v;
    j.parse(&v,(const char*)json);
    
    json_value *mysql=j.find_object(&v,"mysql",5);
    string ip = j.find_object(mysql, "ip", 2)->u.s.s;
    string user = j.find_object(mysql, "user", 4)->u.s.s;
    string db = j.find_object(mysql, "database", 8)->u.s.s;
    string password = j.find_object(mysql, "password", 8)->u.s.s;
    int port = atoi(j.find_object(mysql, "port", 4)->u.s.s);


    j.json_free(&v);
    //��ȡ�û�ע����Ϣ
    unordered_map<string, string> map;
    j.parse(&v, buff);
    map.insert({ "user", j.find_object(&v, "userName", 8)->u.s.s });
    map.insert({ "pwd", j.find_object(&v, "firstPwd", 8)->u.s.s });
    map.insert({ "flower_name", j.find_object(&v, "nickName", 8)->u.s.s });
    map.insert({ "tel", j.find_object(&v, "phone", 5)->u.s.s });
    map.insert({ "email", j.find_object(&v, "email", 5)->u.s.s });
    j.json_free(&v);
    //�������ݿ�
    YYMysql my;
    my.Init();
    my.SetReconnect(1);
    if (!my.Connect(ip.c_str(), user.c_str(), password.c_str(), db.c_str(), port))
    {
        LOG("cgi", "reg", "[ERROR]mysql connect error! %s:%d %s %s %s\n",ip.c_str(),port,user.c_str(),password.c_str(),db.c_str());
        my.Close();
        return -1;
    }
    //�������ݿ����
    my.Query("set names utf8");
    //�鿴�û��Ƿ����
    string sql;
    sql = "select * from user where name=";
    sql += map["user"];;
    YYROWS rows1=my.GetResult(sql.c_str());
    sql = "select * from user where nickname";
    sql += map["flower_name"];
    YYROWS rows2 = my.GetResult(sql.c_str());
    if (rows1.size() == 1 or rows2.size() == 1)//�Ѵ���
    {
        LOG("cgi", "reg", "[ERROR] %s exits\n",user.c_str());
        my.Close();
        return -2;
    }

    //��������ڣ���ע��
    //��ȡ��ǰʱ��
    struct timeval tv;
    struct tm* ptm;
    char time_str[128];
    //��ȡ��ǰʱ�䣬���ȿɴ�΢�뼶��
    gettimeofday(&tv,NULL);
    ptm = localtime(&tv.tv_sec);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ptm);
    map["pwd"] = m.encode(map["pwd"].c_str(),map["pwd"].size());
    sql = "insert into user(name,nickname,password,phone,createtime,email) values(";
    sql += "'";
    sql += map["user"];
    sql += "',";
    sql += "'";
    sql += map["flower_name"];
    sql += "',";
    sql += "'";
    sql += map["pwd"];
    sql += "',";
    sql += "'";
    sql += map["tel"];
    sql += "',";
    sql += "'";
    sql += time_str;
    sql += "',";
    sql += "'";
    sql += map["email"];
    sql += "')";
    if (!my.Query(sql.c_str()))
    {
        LOG("cgi", "reg", "[ERROR]%s error!\n", sql.c_str());
        LOG("cgi", "reg", "[ERROR]%s", my.get_error().c_str());
        my.Close();
        return -1;
    }

    return 0;


   
    
}


int main()
{
    // FCGI_Accept()��һ����������, nginx��fastcgi���������ݵ�ʱ��������
    while (FCGI_Accept() >= 0)
    {
        // 1. ��������
        printf("Content-type: text/html\r\n\r\n");
        int ret = -1;
        char* m = getenv("REQUEST_METHOD");//p�ж���get��������post����
        string method = (const char*)m;
        if (method == "GET" or method == "get")
        {
            //get������QUERY_STRING��ȡ����
            char* q = getenv("QUERY_STRING");
            string query = (const char*)q;
            if (query.size()== 0)
            {
                printf("No data from standard input.<p>\n");
                LOG("cgi", "reg", "QUERY_STRING is null, No data from standard input\n");
                continue;
            }
             ret = user_register((char*)query.c_str());
        }
        else if (method == "POST" or method == "post")
        {   //post������stdin��ȡ
            char* content_length = getenv("CONTENT_LENGTH");
            int len = atoi(content_length);
            if (len <= 0)
            {
                printf("No data from standard input.<p>\n");
                LOG("cgi", "reg", "len = 0, No data from standard input\n");
                continue;
            }
            char buff[len+1];
            buff[len] = '\0';
            ret = fread(buff,1,len,stdin);
            if (ret== 0)
            {
                printf("fread error<p>\n");
                LOG("cgi", "reg", "[ERROR]fread(buff,1,len,stdin) error\n");
                continue;
            }
           
            ret = user_register(buff);
        }
        
        string out;
        if (ret == 0)//ע��ɹ�
        {
            out = "{\"code\":\"002\"}";
        }
        else if (ret == -1)//ע��ʧ��
        {
            out = "{\"code\":\"004\"}";
        }
        else if (ret == -2)//�û��Ѵ���
        {
            out = "{\"code\":\"003\"}";//util_cgi.h
        }   
        
       
       
        printf("%s",out.c_str());
        
    }
}