//获取共享文件列表
#include <stdlib.h>
#include <string>
#include <string.h>
#include "make_log.h"
#include "util_cgi.h"
#include "YYMysql.h"
#include "YYDATA.h"
#include "JSON.h"
#include "MD5.h"
#include "redis.h"
#include <vector>
#include <unordered_map>
#include <utility>
#include <unistd.h>
#include <sys/time.h>
#include <fstream>
#include "fcgi_config.h"
#include "fcgi_stdio.h"


#define module "cgi"
#define pro "sharefiles"

using namespace std;
using namespace YY;

void read_mysql_cfg(string & mysql_ip,string &mysql_user,string &mysql_pwd,string &mysql_db,int &mysql_port)
{
    fstream fp("./conf/cfg.json",ios::in);
    if(!fp.is_open())
    {
        LOG(module,pro,"[ERROR]open cfg.json failed!\n");
        return;
    }
    fp.seekg(0,ios::end);
    int len=fp.tellg();
    fp.seekg(0,ios::beg);
    char buff[len+1];
    buff[len]='\0';
    fp.read(buff,len);
    fp.close();
    json_value v;
    json_value *mysql;
    YYjson j;
    j.parse(&v,buff);
    mysql=j.find_object(&v,"mysql",5);
    mysql_ip=j.find_object(mysql,"ip",2)->u.s.s;
    mysql_user=j.find_object(mysql,"user",4)->u.s.s;
    mysql_pwd=j.find_object(mysql,"password",8)->u.s.s;
    mysql_db=j.find_object(mysql,"database",8)->u.s.s;
    mysql_port=atoi(j.find_object(mysql,"port",4)->u.s.s);
    j.json_free(&v);
}

int get_json_info(const char *buff,int &start,int &count)
{
    YYjson j;
    json_value v;
    int ret=j.parse(&v,buff);
    if(ret!=0)
    {
        LOG(module,pro,"[ERROR]parse input json failed! %s,%d\n",buff,ret);
        j.json_free(&v);
        return -1;
    }
   
    start=j.find_object(&v,"start",5)->u.n;
    
    count=j.find_object(&v,"count",5)->u.n;
   

    j.json_free(&v);
    return 0;
}


int get_redis_info(string &ip,int &port)
{
    YYjson j;
    json_value v;
    json_value *redis;
    fstream fp("./conf/cfg.json",ios::in);
    if(!fp.is_open())
    {
        LOG(module,pro,"[ERROR]redis connect failed!\n");
        return -1;
    }
    fp.seekg(0,ios::end);
    int len=fp.tellg();
    fp.seekg(0,ios::beg);
    char buff[len+1];
    buff[len]='\0';
    fp.read(buff,len);
    fp.close();
    j.parse(&v,buff);
    redis=j.find_object(&v,"redis",5);
    ip=j.find_object(redis,"ip",2)->u.s.s;
     port=atoi(j.find_object(redis,"port",4)->u.s.s);
    j.json_free(&v);
    return 0;

}



void get_share_files_count(string mysql_ip,string mysql_user,string mysql_pwd,string mysql_db,int mysql_port)
{
    char sql_cmd[512] = {0};;
    long line=0;
    YYMysql my;
    my.Init();
    my.SetReconnect();
    if(!my.Connect(mysql_ip.c_str(),mysql_user.c_str(),mysql_pwd.c_str(),mysql_db.c_str(),mysql_port,0))
    {
        LOG(module,pro,"[ERROR]connect mysql failed!\n");
        printf("{\"code\":\"015\"}");
        return;
    }
    my.Query("set names utf8");

    sprintf(sql_cmd, "select * from share_file_list");
    YYROWS rows=my.GetResult(sql_cmd);
    line=rows.size();
    printf("%ld",line);
    my.Close();
}



void get_share_filelist(int start, int count,string mysql_ip,string mysql_user,string mysql_pwd,string mysql_db, int mysql_port) //获取共享文件列表
{
    char sql_cmd[512] = {0};
    YYMysql my;
    my.Init();
    my.SetReconnect();
    if(!my.Connect(mysql_ip.c_str(),mysql_user.c_str(),mysql_pwd.c_str(),mysql_db.c_str(),mysql_port,0))
    {
        LOG(module,pro,"[ERROR]connect mysql failed!\n");
        printf("{\"code\":\"015\"}");
        return;
    }
    my.Query("set names utf8");

    //判断redis与mysql文件是否相等
    
    
    sprintf(sql_cmd, "select share_file_list.*, file_info.url, file_info.size, file_info.type from file_info, share_file_list where file_info.md5 = share_file_list.md5 limit %d, %d", start, count);
    YYROWS rows = my.GetResult(sql_cmd);
	if (rows.empty())
	{
		LOG(module,pro, "[ERROR]get_user_filelist(msyql) failed!%s\n",sql_cmd);
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
			item += "\"pv\":";
			item += row[4].data;
			//LOG("cgi", "myfiles", "[INFO] %s\n", row[4].data);
			item += ",";
		}


		//-- url �ļ�url
		if (row[5].data != NULL)
		{
			item += "\"url\":\"";
			item += row[5].data;
			item += "\",";
		}

		//-- size �ļ���С, ���ֽ�Ϊ��λ
		if (row[6].data != NULL)
		{
			item += "\"size\":";
			item += row[6].data;
			item += ",";
		}

		//-- type �ļ����ͣ� png, zip, mp4����
		if (row[7].data != NULL)
		{
			item += "\"type\":\"";
			item += row[7].data;
			item += "\"}";
		}
		item += ",";
		json_value += item;

	}
	json_value = json_value.substr(0, json_value.size()-1);
	json_value += "]}";
	LOG(module,pro, "[INFO] %s\n", json_value.c_str());
	printf(json_value.c_str());
	my.Close();

}





void get_ranking_filelist(int start, int count,string mysql_ip,string mysql_user,string mysql_pwd,string mysql_db,int mysql_port)
{
      /*
    a) mysql共享文件数量和redis共享文件数量对比，判断是否相等
    b) 如果不相等，清空redis数据，从mysql中导入数据到redis (mysql和redis交互)
    c) 从redis读取数据，给前端反馈相应信息
    */
    char sql_cmd[512] = {0};
    YYMysql my;
    char fileid[512];
    my.Init();
    my.SetReconnect();
    if(!my.Connect(mysql_ip.c_str(),mysql_user.c_str(),mysql_pwd.c_str(),mysql_db.c_str(),mysql_port,0))
    {
        LOG(module,pro,"[ERROR]connect mysql failed!\n");
        printf("{\"code\":\"015\"}");
        return;
    }
    my.Query("set names utf8");



    string redis_ip;
    int redis_port;
    get_redis_info(redis_ip,redis_port);
    redis r(redis_ip.c_str(),redis_port);
    if(r.connect()!=0)
    {
       LOG(module,pro,"[ERROR]connect redis failed!\n");
        printf("{\"code\":\"015\"}");
        my.Close();
        return; 
    }
    
    sprintf(sql_cmd, "select count(*) from share_file_list");
    YYROWS rows = my.GetResult(sql_cmd);
	if (rows.empty())
	{
		LOG(module,pro, "[ERROR]get_user_filelist(msyql) failed!%s %s\n",sql_cmd, my.get_error().c_str());
		my.Close();
		printf("{\"code\": \"015\"}");
		return;
	}
    int my_count=atoi(rows[0][0].data);
    int re_count=r.zset_len("share_zset");

    if(my_count!=re_count)
    {
        r.zset_del_all("share_zset");
        sprintf(sql_cmd, "select md5, filename, pv from share_file_list order by pv desc");
    
        rows = my.GetResult(sql_cmd);
        if (rows.empty())
        {
            LOG(module,pro, "[ERROR]get_user_filelist(msyql) failed!%s %s\n",sql_cmd, my.get_error().c_str());
            my.Close();
            printf("{\"code\": \"015\"}");
            return;
        }
        for(int i=0;i<rows.size();i++)
        {
            sprintf(fileid, "%s%s", rows[i][0].data, rows[i][1].data);
            r.zset_add("share_zset",atoi(rows[i][2].data),fileid);
            r.hash_append("share_hash",fileid,rows[i][1].data); 
        }

    }

    //===5、从redis读取数据，给前端反馈相应信息

    int n=0;
    int end=start+count-1;
    vector<string> share_list;
    if(r.zset_rerange("share_zset",start,end,share_list)!=0)
    {
        LOG(module,pro, "[ERROR]zset_rerange(redis) failed!\n");
        my.Close();
        printf("{\"code\": \"015\"}");
        return;
    }
    string json="{\"code\":\"000\",\"files\":[";
    for(int i=0;i<share_list.size();i++)
    {
        json+="{\"filename\":\"";
        string fn;
        r.hash_get("share_hash",share_list[i],fn);
        json+=fn;
        json+="\",\"pv\":";
        long a;
        r.zset_get_score("share_zset",share_list[i],a);
        json+=to_string(a);
        json+="},";
    }
    json=json.substr(0,json.size()-1);
    json+="]}";

    LOG(module,pro, "[INFO]%s\n",json.c_str());
    printf("%s",json.c_str());
    my.Close();




}









int main()
{
    string mysql_ip;
    string mysql_user;
    string mysql_pwd;
    string mysql_db;
    int mysql_port;
    read_mysql_cfg(mysql_ip,mysql_user,mysql_pwd,mysql_db,mysql_port);
    while(FCGI_Accept()>=0)
    { 
        printf("Content-type: text/html\r\n\r\n");
        string query = getenv("QUERY_STRING");
        int pos=query.find("=");
        query=query.substr(pos+1,query.size()-pos-1);
        if(query=="count")
        {
            get_share_files_count(mysql_ip,mysql_user,mysql_pwd,mysql_db,mysql_port);
        }
        else
        {
            int len=atoi(getenv("CONTENT_LENGTH"));
            if (len <= 0)
            {
                printf("No data from standard input.<p>\n");
                LOG(module, pro, "[ERROR]len = 0, No data from standard input\n");
                continue;
            }
            char buf[4*1024] = {0};
            int ret = 0;
            ret = fread(buf, 1, len, stdin); //从标准输入(web服务器)读取内容
            if(ret == 0)
            {
                LOG(module, pro, "[ERROR]fread(buf, 1, len, stdin) err\n");
                continue;
            }
            LOG(module, pro, "[info]buf = %s\n", buf);
            //获取共享文件信息 127.0.0.1:80/sharefiles&cmd=normal
            //按下载量升序 127.0.0.1:80/sharefiles?cmd=pvasc
            //按下载量降序127.0.0.1:80/sharefiles?cmd=pvdesc
            int start; //文件起点
            int count; //文件个数
            get_json_info(buf, start, count); //通过json包获取信息
            LOG(module, pro, "[info]start = %d, count = %d\n", start, count);

            if(query=="normal")
            get_share_filelist(start, count,mysql_ip,mysql_user,mysql_pwd,mysql_db,mysql_port); //获取共享文件列表
            else 
           {
                get_ranking_filelist(start, count,mysql_ip,mysql_user,mysql_pwd,mysql_db,mysql_port);
           }



        }

    }
}

