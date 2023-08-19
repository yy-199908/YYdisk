//验证上传文件的md5是否以存在于数据库中，如果存在，则证明有其他用户上传了同一个文件
//没必要再次上传，只需要数据库操作即可
//接收json数据包格式

            /*
             * {
                user:xxxx,
                token: xxxx,
                md5:xxx,
                fileName: xxx
               }
             */
            
//返回json格式
/*
    秒传文件：
        文件已存在：{"code":"005"}
        秒传成功：  {"code":"006"}
        秒传失败：  {"code":"007"}

    */




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


#define module "cgi"
#define pro "md5"

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

int get_json_info(const char *buff,string &user,string&token,string&md5,string&filename)
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
    token=j.find_object(&v,"token",5)->u.s.s;
    filename=j.find_object(&v,"fileName",8)->u.s.s;

    j.json_free(&v);
    return 0;
}


void deal_md5(string user,string md5,string filename,string mysql_ip,string mysql_user,string mysql_pwd,string mysql_db,int mysql_port)
{
    YYMysql my;
    my.Init();
    my.SetReconnect(1);
    if(!my.Connect(mysql_ip.c_str(), mysql_user.c_str(), mysql_pwd.c_str(), mysql_db.c_str(),mysql_port,0))
    {
        LOG(module,pro,"[ERROR] mysql connect error! %s\n",my.get_error().c_str());
        return;
    }

    my.Query("set names utf8");
        /*
        秒传文件：
            文件已存在：{"code":"005"}
            秒传成功：  {"code":"006"}
            秒传失败：  {"code":"007"}
        token验证失败：{"code":"111"}

        */
   string sql="select count from file_info where md5='";
   sql+=md5;
   sql+="' for update";
   YYROWS rows=my.GetResult(sql.c_str());
   if(rows.size()==0)
   {
        printf("{\"code\":\"007\"}");
        my.Close();
        return;
   }
   
    int count=atoi(rows[0][0].data);
    //文件存在
    //查看当前用户是否有此文件
    sql="select * from user_file_list where user='";
    sql+=user;
    sql+="' and md5=";
    sql+=md5;
    sql+="' and filename='";
    sql+=filename;
    sql+="'";
    YYROWS rows2=my.GetResult(sql.c_str());
    if(rows2.size()!=0)
    {
        printf("{\"code\":\"005\"}");
        return;
    }
    //1、修改file_info中的count字段，+1 （count 文件引用计数）
    sql="update file_info set count='";
    sql+=to_string(++count);
    sql+="' where md5='";
    sql+=md5;
    sql+="'";
    if(!my.Query(sql.c_str()))
    {
        LOG(module,pro,"[ERROR] mysql %s error! %s\n",sql.c_str(),my.get_error().c_str());
        printf("{\"code\":\"007\"}");
        return;
    }
    
    //2.user_file_list, 用户文件列表插入一条数据

    struct timeval tv;
    struct tm* ptm;
    char time_str[128];

    //使用函数gettimeofday()函数来得到时间。它的精度可以达到微妙
    gettimeofday(&tv, NULL);
    ptm = localtime(&tv.tv_sec);//把从1970-1-1零点零分到当前时间系统所偏移的秒数时间转换为本地时间
    //strftime() 函数根据区域设置格式化本地时间/日期，函数的功能将时间格式化，或者说格式化一个时间字符串
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ptm);
    sql="insert into user_file_list(user, md5, createtime, filename, shared_status, pv) values ('";
    sql+=user;
    sql+="','";
    sql+=md5;
    sql+="','";
    sql+=time_str;
    sql+="','";
    sql+=filename;
    sql+="','0','0')";
    if(!my.Query(sql.c_str()))
    {
        LOG(module,pro,"[ERROR] mysql %s error! %s\n",sql.c_str(),my.get_error().c_str());
        printf("{\"code\":\"007\"}");
        return;
    }

    //3.更新用户文件数量
    sql="select count from user_file_count where user ='";
    sql+=user;
    sql+="' for update";
    YYROWS rows3=my.GetResult(sql.c_str());
    if(rows3.size()==0)//没有则插入
    {
        sql=" insert into user_file_count (user, count) values('";
        sql+=user;
        sql+="','1')";
         if(!my.Query(sql.c_str()))
        {
            LOG(module,pro,"[ERROR] mysql %s error! %s\n",sql.c_str(),my.get_error().c_str());
            printf("{\"code\":\"007\"}");
            return;
        }

    }
    else
    {
        sql="update user_file_count set count = '";
        sql+=to_string(atoi(rows[0][0].data)+1);
        sql+="' where user='";
        sql+=user;
        sql+="'";
        if(!my.Query(sql.c_str()))
        {
            LOG(module,pro,"[ERROR] mysql %s error! %s\n",sql.c_str(),my.get_error().c_str());
            printf("{\"code\":\"007\"}");
            return;
        }

    }
    printf("{\"code\":\"006\"}");

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
    string token;
    string filename;
    while(FCGI_Accept()>=0)
    {
        printf("Content-type:text/html\r\n\r\n");
        user.clear();md5.clear();token.clear();filename.clear();
        char *contentLength=getenv("CONTENT_LENGTH");
        int len=atoi(contentLength);
        if(len==0)
        {
            LOG(module,pro,"[ERROR]len==0,No data from input!\n");
            continue;
        }
        char buff[len+1];
        buff[len]='\0';
        int ret=0;
        ret=fread(buff,1,len,stdin);
        if(ret==0)
        {
            LOG(module,pro,"[ERROR] fread(buff,1,len,stdin) err\n");
            continue;
        }
       
        ret=get_json_info(buff,user,token,md5,filename);
      
        if(ret==-1)
        continue;
        ret=verify_token(user,token);
        if(ret==0)
        {
             LOG(module,pro,"[ERROR] very token failed!\n");
             printf("{\"code\":\"111\"}");

        }
        else
        {
            deal_md5(user,md5,filename,mysql_ip,mysql_user,mysql_pwd,mysql_db,mysql_port);
        }


    }
}