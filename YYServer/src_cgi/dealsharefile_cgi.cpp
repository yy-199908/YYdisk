/**
 * @file dealsharefile_cgi.c
 * @brief  共享文件pv字段处理、取消分享、转存文件cgi程序
 */

#include <stdlib.h>
#include <string>
#include <string.h>
#include "make_log.h"
#include "util_cgi.h"
#include "YYMysql.h"
#include "YYDATA.h"
#include "redis.h"
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


#define module "cgi"
#define pro "dealsharefile"

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

int get_json_info(const char *buff,string &user,string&md5,string&filename)
{
    YYjson j;
    json_value v;
    if(j.parse(&v,buff)!=0)
    {
        LOG(module,pro,"[ERROR]parse input json failed!\n");
        j.json_free(&v);
        return -1;
    }

    user=j.find_object(&v,"user",4)->u.s.s;
    md5=j.find_object(&v,"md5",3)->u.s.s;
    filename=j.find_object(&v,"filename",8)->u.s.s;

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


void pv_file(string md5,string filename,string mysql_ip,string mysql_user,string mysql_pwd,string mysql_db,int mysql_port)
{
     /*
    下载文件pv字段处理
        成功：{"code":"016"}
        失败：{"code":"017"}
    */
    int ret = 0;
    char sql_cmd[512] = {0};
    YYMysql my;
    my.Init();
    my.SetReconnect();
    if(!my.Connect(mysql_ip.c_str(),mysql_user.c_str(),mysql_pwd.c_str(),mysql_db.c_str(),mysql_port,0))
    {
        LOG(module,pro,"[ERROR]connect mysql failed!\n");
        printf("{\"code\":\"017\"}");
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
        printf("{\"code\":\"017\"}");
        my.Close();
        return; 
    }

    char fileid[1024] = {0};
    sprintf(fileid, "%s%s", md5.c_str(), filename.c_str());
    //===1、mysql的下载量+1(mysql操作)
    //sql语句
    //查看该共享文件的pv字段
    sprintf(sql_cmd, "select pv from share_file_list where md5 = '%s' and filename = '%s' for update", md5.c_str(), filename.c_str());
    YYROWS rows = my.GetResult(sql_cmd);
	if (rows.empty())
	{
		LOG(module,pro, "[ERROR]get_user_filelist(msyql) failed!%s %s\n",sql_cmd, my.get_error().c_str());
		my.Close();
		printf("{\"code\": \"017\"}");
		return;
	}
    int pv=atoi(rows[0][0].data);
    sprintf(sql_cmd, "update share_file_list set pv = %d where md5 = '%s' and filename = '%s'", pv+1, md5.c_str(), filename.c_str());
    if (!my.Query(sql_cmd) )
    {
         LOG(module,pro,"[ERROR] mysql query failed! %s,%s\n",sql_cmd,my.get_error().c_str());
        printf("{\"code\":\"017\"}");
        my.Close();
        return;
   
    }


    sprintf(sql_cmd, "update user_file_list set pv = %d where md5 = '%s' and filename = '%s'", pv+1, md5.c_str(), filename.c_str());
    if (!my.Query(sql_cmd) )
    {
         LOG(module,pro,"[ERROR] mysql query failed! %s,%s\n",sql_cmd,my.get_error().c_str());
        printf("{\"code\":\"017\"}");
        my.Close();
        return;
   
    }



     //===2、判断元素是否在集合中(redis操作)
    ret = r.zset_exit("share_zset", fileid);
    if(ret==1)//存在
    {
        r.zset_score("share_zset", fileid,1);
    }
    else if(ret==0)//不存在
    {
        r.zset_add("share_zset", pv+1, fileid);

        //===6、redis对应的hash也需要变化 (redis操作)
        //     fileid ------>  filename
        r.hash_append("share_hash", fileid, filename);

    }

    else
    {
        LOG(module,pro,"[ERROR] zset_exit() failed! \n");
        printf("{\"code\":\"017\"}");
        my.Close();
        return;
    }
    printf("{\"code\":\"016\"}");
    my.Close();
}

void cancel_share_file(string md5,string user,string filename,string mysql_ip,string mysql_user,string mysql_pwd,string mysql_db,int mysql_port)
{
    /*
    取消分享：
        成功：{"code":"018"}
        失败：{"code":"019"}
    */
   LOG(module, pro, "[TEST]cancel_share_file\n");
    int ret = 0;
    char sql_cmd[152] = {0};
    YYMysql my;
    my.Init();
    my.SetReconnect();
    if(!my.Connect(mysql_ip.c_str(),mysql_user.c_str(),mysql_pwd.c_str(),mysql_db.c_str(),mysql_port,0))
    {
        LOG(module,pro,"[ERROR]connect mysql failed!\n");
        printf("{\"code\":\"019\"}");
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
        printf("{\"code\":\"019\"}");
        my.Close();
        return; 
    }

    char fileid[1024] = {0};
    sprintf(fileid, "%s%s", md5.c_str(), filename.c_str());
    //===1、mysql记录操作
    //sql语句
    sprintf(sql_cmd, "update user_file_list set shared_status = 0 where user = '%s' and md5 = '%s' and filename = '%s'", user.c_str(), md5.c_str(), filename.c_str());
    if (!my.Query(sql_cmd) )
    {
         LOG(module,pro,"[ERROR] mysql query failed! %s,%s\n",sql_cmd,my.get_error().c_str());
        printf("{\"code\":\"019\"}");
        my.Close();
        return;
    }
    sprintf(sql_cmd, "delete from share_file_list  where user = '%s' and md5 = '%s' and filename = '%s'", user.c_str(), md5.c_str(), filename.c_str());
    if (!my.Query(sql_cmd) )
    {
         LOG(module,pro,"[ERROR] mysql query failed! %s,%s\n",sql_cmd,my.get_error().c_str());
        printf("{\"code\":\"019\"}");
        my.Close();
        return;
    }

    //===2、redis记录操作
    //有序集合删除指定成员
    ret=r.zset_del("share_zset",fileid);
    if(ret!=0)
    {
        LOG(module,pro,"[ERROR] zset_del() failed! \n");
        printf("{\"code\":\"019\"}");
        my.Close();
        return;
    }
    ret=r.hash_del("share_hash",fileid);
    if(ret!=0)
    {
        LOG(module,pro,"[ERROR] hash_del() failed! \n");
        printf("{\"code\":\"019\"}");
        my.Close();
        return;
    }
    printf("{\"code\":\"018\"}");
    my.Close();



}






void save_file(string user,string md5,string filename,string mysql_ip,string mysql_user,string mysql_pwd,string mysql_db,int mysql_port)
{
    /*
    返回值：0成功，-1转存失败，-2文件已存在
    转存文件：
        成功：{"code":"020"}
        文件已存在：{"code":"021"}
        失败：{"code":"022"}
    */
    int ret = 0;
    char sql_cmd[512] = {0};
    YYMysql my;
    my.Init();
    my.SetReconnect();
    if(!my.Connect(mysql_ip.c_str(),mysql_user.c_str(),mysql_pwd.c_str(),mysql_db.c_str(),mysql_port,0))
    {
        LOG(module,pro,"[ERROR]connect mysql failed!\n");
        printf("{\"code\":\"022\"}");
        return;
    }
    my.Query("set names utf8");
    //查看此用户，文件名和md5是否存在，如果存在说明此文件存在
    sprintf(sql_cmd, "select * from user_file_list where user = '%s' and md5 = '%s' and filename = '%s'", user.c_str(), md5.c_str(), filename.c_str());
    YYROWS rows = my.GetResult(sql_cmd);
	if (!rows.empty())
	{
		LOG(module,pro, "[ERROR]user have this file!\n");
		my.Close();
		printf("{\"code\": \"021\"}");
		return;
	}
    //文件信息表，查找该文件的计数器
    sprintf(sql_cmd, "select count from file_info where md5 = '%s' for update", md5.c_str());
    rows = my.GetResult(sql_cmd);
	if (rows.empty())
	{
		LOG(module,pro, "[ERROR]%s,%s!\n",sql_cmd,my.get_error().c_str());
		my.Close();
		printf("{\"code\": \"022\"}");
		return;
    }
    int count=atoi(rows[0][0].data);
     sprintf(sql_cmd, "update file_info set count = %d where md5 = '%s'", count+1, md5.c_str());
    if (!my.Query(sql_cmd) )
    {
        LOG(module,pro,"[ERROR] mysql query failed! %s,%s\n",sql_cmd,my.get_error().c_str());
        printf("{\"code\":\"022\"}");
        my.Close();
        return;
   
    }

    //2、user_file_list插入一条数据
    struct timeval tv;
    struct tm* ptm;
    char time_str[128];

    //使用函数gettimeofday()函数来得到时间。它的精度可以达到微妙
    gettimeofday(&tv, NULL);
    ptm = localtime(&tv.tv_sec);//把从1970-1-1零点零分到当前时间系统所偏移的秒数时间转换为本地时间
    //strftime() 函数根据区域设置格式化本地时间/日期，函数的功能将时间格式化，或者说格式化一个时间字符串
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ptm);
    sprintf(sql_cmd, "insert into user_file_list(user, md5, createtime, filename, shared_status, pv) values ('%s', '%s', '%s', '%s', %d, %d)", user.c_str(), md5.c_str(), time_str, filename.c_str(), 0, 0);
    if (!my.Query(sql_cmd) )
    {
        LOG(module,pro,"[ERROR] mysql query failed! %s,%s\n",sql_cmd,my.get_error().c_str());
        printf("{\"code\":\"022\"}");
        my.Close();
        return;
   
    }

    //3、查询用户文件数量，更新该字段

    sprintf(sql_cmd, "select count from user_file_count where user = '%s' for update", user.c_str());
    rows = my.GetResult(sql_cmd);
	if (rows.empty())
	{
		 sprintf(sql_cmd, " insert into user_file_count (user, count) values('%s', %d)", user.c_str(), 1);
         if (!my.Query(sql_cmd) )
        {
            LOG(module,pro,"[ERROR] mysql query failed! %s,%s\n",sql_cmd,my.get_error().c_str());
            printf("{\"code\":\"022\"}");
            my.Close();
            return;

         }
	}
    else
    {
        count=atoi(rows[0][0].data);
        sprintf(sql_cmd, "update user_file_count set count = %d where user = '%s'", count+1, user.c_str());
        if (!my.Query(sql_cmd) )
        {
            LOG(module,pro,"[ERROR] mysql query failed! %s,%s\n",sql_cmd,my.get_error().c_str());
            printf("{\"code\":\"022\"}");
            my.Close();
            return;

        }
    }
    my.Close();
    printf("{\"code\":\"020\"}");


}
int main()
{


    string mysql_ip;
    string mysql_user;
    string mysql_pwd;
    string mysql_db;
    int mysql_port;
    read_mysql_cfg(mysql_ip,mysql_user,mysql_pwd,mysql_db,mysql_port);
    string user;
    string md5;
    string filename;
    while(FCGI_Accept()>=0)
    {
        printf("Content-type:text/html\r\n\r\n");
        user.clear();md5.clear();filename.clear();
        string query = getenv("QUERY_STRING");
        int pos=query.find("=");
        query=query.substr(pos+1,query.size()-pos-1);

        int len=atoi(getenv("CONTENT_LENGTH"));
        if (len <= 0)
        {
            printf("No data from standard input.<p>\n");
            LOG(module, pro, "[ERROR]len = 0, No data from standard input\n");
            continue;
        }
        char buff[len+1];
        buff[len]='\0';
        int ret=fread(buff, 1, len, stdin);
        if(ret == 0)
        {
            LOG(module, pro, "fread(buf, 1, len, stdin) err\n");
            continue;
        }
        LOG(module, pro, "buf = %s\n", buff);
         get_json_info(buff, user, md5, filename);
          LOG(module, pro, "[TEST] %s,%s,%s\n", user,md5,filename);
        if(query== "pv") //文件下载标志处理
        {
            pv_file(md5, filename,mysql_ip,mysql_user,mysql_pwd,mysql_db,mysql_port);
        }
        else if(query== "cancel") //取消分享文件
        {
            cancel_share_file(md5,user,  filename,mysql_ip,mysql_user,mysql_pwd,mysql_db,mysql_port);
        }
        else if(query== "save") //转存文件
        {
            save_file(user, md5, filename,mysql_ip,mysql_user,mysql_pwd,mysql_db,mysql_port);
        }


    }
    return 0;
}


