/*文件操作cgi 包括分享、删除、以及client直接访问storange下载后更新mysql pv字段
url=/dealfile?cmd=share   
 
    {
        "user": "yoyo",
        "token": "xxxx",
        "md5": "xxx",
        "filename": "xxx"
    }
    
返回code 010-成功 011-失败 012-别人已经分享 111-token验证失败，登录过期



url=/dealfile?cmd=del"
/*
{
    "user": "yoyo",
    "token": "xxxx",
    "md5": "xxx",
    "filename": "xxx"
}

删除文件：
    成功：{"code":"013"}
    失败：{"code":"014"}

url=/dealfile?cmd=pv

{
    "user": "yoyo",
    "token": "xxx",
    "md5": "xxx",
    "filename": "xxx"
}
下载文件pv字段处理
                成功：{"code":"016"}
                失败：{"code":"017"}
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
#define pro "dealfile"

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




void add_pv(string mysql_ip,string mysql_user,string mysql_pwd,string mysql_db,int mysql_port,string user,string md5,string filename)
{
    YYMysql my;
    int pv;
    my.Init();
    my.SetReconnect();
    if(!my.Connect(mysql_ip.c_str(),mysql_user.c_str(),mysql_pwd.c_str(),mysql_db.c_str(),mysql_port,0))
    {
        LOG(module,pro,"[ERROR]connect mysql failed!\n");
        printf("{\"code\":\"017\"}");
        return;
    }
    my.Query("set names utf8");

    char sql_cmd[512]={0};
    sprintf(sql_cmd, "select pv from user_file_list where user = '%s' and md5 = '%s' and filename = '%s' for update", user.c_str(), md5.c_str(), filename.c_str());
    YYROWS rows=my.GetResult(sql_cmd);
    if(rows.size()==1)
    {
        pv=atoi(rows[0][0].data);
    }
    else{
        LOG(module,pro,"[ERROR]mysql query failed! %s, %s\n",sql_cmd,my.get_error().c_str());
        printf("{\"code\":\"017\"}");
        my.Close();
        return;
   
    }
     sprintf(sql_cmd, "update user_file_list set pv = %d where user = '%s' and md5 = '%s' and filename = '%s'", pv+1, user.c_str(), md5.c_str(), filename.c_str());

    if (!my.Query(sql_cmd) )
    {
         LOG(module,pro,"[ERROR] mysql query failed! %s, %s\n",sql_cmd,my.get_error().c_str());
        printf("{\"code\":\"017\"}");
        my.Close();
        return;
    }
   
   sprintf(sql_cmd, "update share_file_list set pv = %d where user = '%s' and md5 = '%s' and filename = '%s'", pv+1, user.c_str(), md5.c_str(), filename.c_str());
     if (!my.Query(sql_cmd) )
    {
         LOG(module,pro,"[ERROR] mysql query failed! %s, %s\n",sql_cmd,my.get_error().c_str());
        printf("{\"code\":\"017\"}");
        my.Close();
        return;
    }
    my.Close();

    string redis_ip;
    int redis_port;
    get_redis_info(redis_ip,redis_port);
    //1.连接redis并判断分享集合有无该文件,member为MD5
    redis r(redis_ip.c_str(),redis_port);
    r.connect();
    //文件标示，md5+文件名
    char fileid[512];
    sprintf(fileid,"%s%s",md5.c_str(),filename.c_str());
    if(r.zset_exit("share_zset",fileid)==1)
    {
        r.zset_score("share_zset",fileid,1);
    }
    printf("{\"code\":\"016\"}");
}


void share_file(string mysql_ip,string mysql_user,string mysql_pwd,string mysql_db,int mysql_port,string user,string md5,string filename)
{
    //code 010-成功 011-失败 012-别人已经分享 111-token验证失败，登录过期
    char sql_cmd[512] = {0};
    YYMysql my;
    my.Init();
    my.SetReconnect(1);
    if(!my.Connect(mysql_ip.c_str(),mysql_user.c_str(),mysql_pwd.c_str(),mysql_db.c_str(),mysql_port,0))
    {
        LOG(module,pro,"[ERROR]connect mysql failed!\n");
        printf("{\"code\":\"011\"}");
        return;
    }
    my.Query("set names utf8");
    string redis_ip;
    int redis_port;
    get_redis_info(redis_ip,redis_port);

    //1.连接redis并判断分享集合有无该文件,member为MD5
    redis r(redis_ip.c_str(),redis_port);
    r.connect();
    //文件标示，md5+文件名
    char fileid[512];
    sprintf(fileid,"%s%s",md5.c_str(),filename.c_str());
    int ret=r.zset_exit("share_zset",fileid);
    if(ret==-1)//查找失败
    {
        LOG(module,pro,"[ERROR]zset_exit failed!\n");
        my.Close();
        printf("{\"code\":\"011\"}");
        return;
    }
    else if(ret==1)
    {
        
        LOG(module, pro, "[info]other share this file\n");
        my.Close();
        printf("{\"code\":\"012\"}");
        return;
    }
    else if(ret==0)
    {
        //2.集合没有此元素，有可能是redistribution中没有记录，再从mysql中查找，若mysql中也没有分享，那则分享该文件。如果mysql中有记录，则redis保存信息，并中断
        sprintf(sql_cmd, "select * from share_file_list where md5 = '%s' and filename = '%s'", md5.c_str(), filename.c_str());
        YYROWS rows=my.GetResult(sql_cmd);
        if(rows.size()!=0)//mysql中有分享
        {
            r.zset_add("share_zset",0,fileid);
            LOG(module, pro, "[info]other share this file\n");
            my.Close();
            printf("{\"code\":\"012\"}");
            return;
        }
        else//mysql中没有分享
        {
            r.zset_add("share_zset",0,fileid);
            sprintf(sql_cmd, "update user_file_list set shared_status = 1 where user = '%s' and md5 = '%s' and filename = '%s'", user.c_str(), md5.c_str(), filename.c_str());
            if (!my.Query(sql_cmd))
            {
                LOG(module, pro, "[ERROR]mysql query failed! %s,%s\n", sql_cmd, my.get_error().c_str());
                my.Close();
                printf("{\"code\":\"011\"}");
                return;
            }
            sprintf(sql_cmd, "select createtime,pv from user_file_list where user='%s' and md5='%s' and filename='%s'", user.c_str(), md5.c_str(), filename.c_str());
            rows=my.GetResult(sql_cmd);
            if(rows.size()==0)//
            {
             
                LOG(module, pro, "[ERROR]mysql select failed\n");
                my.Close();
                printf("{\"code\":\"011\"}");
                return;
            }
           
            sprintf(sql_cmd, "insert into share_file_list(user,md5,createtime,filename,pv) values('%s','%s','%s','%s','%s') ", user.c_str(), md5.c_str(),rows[0][0].data, filename.c_str(),rows[0][1].data);
            if (!my.Query(sql_cmd) )
            {
                LOG(module,pro,"[ERROR] mysql query failed! %s, %s\n",sql_cmd,my.get_error().c_str());
                printf("{\"code\":\"011\"}");
                my.Close();
                return;
        
            }


            r.hash_append("share_hash",fileid,filename);
             printf("{\"code\":\"010\"}");
             my.Close();
        }
    }



}



int remove_file_from_storage(const char *fileid)
{
    int ret = 0;
    char cmd[1024*2] = {0};
    //读取fdfs client 配置文件的路径
    YYjson j;
    json_value v;
    json_value *fdfs;
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
    fdfs=j.find_object(&v,"dfs_path",8);
    string path=j.find_object(fdfs,"client",6)->u.s.s;
    j.json_free(&v);


    
    sprintf(cmd, "fdfs_delete_file %s %s", path.c_str(), fileid);

    ret = system(cmd);
    if(ret!=0)
    LOG(module, pro, "[ERROR]remove_file_from_storage ret = %d\n", ret);

    return ret;
}






void del_file (string mysql_ip,string mysql_user,string mysql_pwd,string mysql_db,int mysql_port,string user,string md5,string filename)
{
    /*
    a)先判断此文件是否已经分享
    b)判断集合有没有这个文件，如果有，说明别人已经分享此文件(redis操作)
    c)如果集合没有此元素，可能因为redis中没有记录，再从mysql中查询，如果mysql也没有，说明真没有(mysql操作)
    d)如果mysql有记录，而redis没有记录，那么分享文件处理只需要处理mysql (mysql操作)
    e)如果redis有记录，mysql和redis都需要处理，删除相关记录

    删除文件：
    成功：{"code":"013"}
    失败：{"code":"014"}
    */

   
   char sql_cmd[512] = {0};
   string redis_ip;
   int redis_port;
   get_redis_info(redis_ip,redis_port);
   char fileid[1024];
  
   int count = 0;
    int share = 0;  //共享状态
    int flage = 0; //标志redis是否有记录
   //连接redis
   redis r(redis_ip.c_str(),redis_port);
   if(r.connect()==-1)
   {
    LOG(module, pro, "[ERROR]redis connect failed! \n");
    printf("{\"code\":\"014\"}");
    return;
   }
   //连接mysql
    YYMysql my;
    my.Init();
    my.SetReconnect();
    if(!my.Connect(mysql_ip.c_str(),mysql_user.c_str(),mysql_pwd.c_str(),mysql_db.c_str(),mysql_port,0))
    {
        LOG(module,pro,"[ERROR]connect mysql failed!\n");
        printf("{\"code\":\"014\"}");
        return;
    }
    my.Query("set names utf8");

    sprintf(fileid, "%s%s", md5.c_str(), filename.c_str());

    //===1、先判断此文件是否已经分享，判断集合有没有这个文件，如果有，说明别人已经分享此文件
    int ret=r.zset_exit("share_zset",fileid);
    if(ret==1)//存在
    {
        share=1;
        flage=1;
    }
    else if(ret == 0) //不存在
    {//===2、如果集合没有此元素，可能因为redis中没有记录，再从mysql中查询，如果mysql也没有，说明真没有(mysql操作)

        //sql语句
        //查看该文件是否已经分享了
        sprintf(sql_cmd, "select shared_status from user_file_list where user = '%s' and md5 = '%s' and filename = '%s'", user.c_str(), md5.c_str(), filename.c_str());

        //返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
        YYROWS rows=my.GetResult(sql_cmd); //执行sql语句
        if(rows.size()!= 0)
        {
            share = atoi(rows[0][0].data); //shared_status字段
        }
        
    }
    else//查询失败
    {
        LOG(module, pro, "[ERROR]redis zset_exit() failed!\n");
        my.Close();
        printf("{\"code\":\"014\"}");
        return;
    }
    //说明此文件被分享，删除分享列表(share_file_list)的数据
    if(share==1)
    {
        //===3、如果mysql有记录，删除相关分享记录 (mysql操作)
        //删除在共享列表的数据
        sprintf(sql_cmd, "delete from share_file_list where user = '%s' and md5 = '%s' and filename = '%s'", user.c_str(), md5.c_str(), filename.c_str());
        if (!my.Query(sql_cmd))
        {
            LOG(module, pro, "[ERROR]mysql query failed! %s,%s\n", sql_cmd, my.get_error().c_str());
            my.Close();
            printf("{\"code\":\"014\"}");
            return;
        }
        //===4、如果redis有记录，redis需要处理，删除相关记录
        if(1 == flage)
        {
            //有序集合删除指定成员
            r.zset_del( "share_zset", fileid);

            //从hash移除相应记录
            r.hash_del("share_hash", fileid);
        }        
    }




    //用户文件数量-1
    //查询用户文件数量

    sprintf(sql_cmd, "select count from user_file_count where user = '%s'", user.c_str());
    YYROWS rows=my.GetResult(sql_cmd); //执行sql语句
    if(rows.size()!= 0)
    {
        count = atoi(rows[0][0].data); //shared_status字段
    }
    else
    {
        LOG(module, pro, "[ERROR]mysql query failed! %s,%s\n", sql_cmd, my.get_error().c_str());
        my.Close();
        printf("{\"code\":\"014\"}");
        return;
    }
    if(count==1)
    {
        sprintf(sql_cmd, "delete from user_file_count where user = '%s'", user.c_str());
    }
    else{
        sprintf(sql_cmd, "update user_file_count set count = %d where user = '%s'", count-1, user.c_str());
    }

    if (!my.Query(sql_cmd))
    {
        LOG(module, pro, "[ERROR]mysql query failed! %s,%s\n", sql_cmd, my.get_error().c_str());
        my.Close();
        printf("{\"code\":\"014\"}");
        return;
    }




    //删除用户文件列表数据
    sprintf(sql_cmd, "delete from user_file_list where user = '%s' and md5 = '%s' and filename = '%s'", user.c_str(), md5.c_str(), filename.c_str());
    if (!my.Query(sql_cmd))
    {
        LOG(module, pro, "[ERROR]mysql query failed! %s,%s\n", sql_cmd, my.get_error().c_str());
        my.Close();
        printf("{\"code\":\"014\"}");
        return;
    }



    //文件信息表(file_info)的文件引用计数count，减去1
    //查看该文件文件引用计数
    sprintf(sql_cmd, "select count from file_info where md5 = '%s'", md5.c_str());
     rows=my.GetResult(sql_cmd); //执行sql语句
    if(rows.size()!= 0)
    {
        count = atoi(rows[0][0].data); //shared_status字段
    }
    else
    {
        LOG(module, pro, "[ERROR]mysql query failed! %s,%s\n", sql_cmd, my.get_error().c_str());
        my.Close();
        printf("{\"code\":\"014\"}");
        return;
    }
    if(count==1)
    {
        string file_id;
        sprintf(sql_cmd, "select file_id from file_info where md5 = '%s'", md5.c_str());
        YYROWS rows=my.GetResult(sql_cmd); //执行sql语句
        if(rows.size()!= 0)
        {
            file_id=rows[0][0].data; //shared_status字段
        }
        else
        {
            LOG(module, pro, "[ERROR]mysql query failed! %s,%s\n", sql_cmd, my.get_error().c_str());
            my.Close();
            printf("{\"code\":\"014\"}");
            return;
        }
        //删除文件信息表中该文件的信息
        sprintf(sql_cmd, "delete from file_info where md5 = '%s'", md5.c_str());
        if (!my.Query(sql_cmd))
        {
            LOG(module, pro, "[ERROR]mysql query failed! %s,%s\n", sql_cmd, my.get_error().c_str());
            my.Close();
            printf("{\"code\":\"014\"}");
            return;
        }


        //从storage服务器删除此文件，参数为为文件id
        if( remove_file_from_storage(file_id.c_str()) != 0)
        {
            LOG(module, pro, "remove_file_from_storage err\n");
             my.Close();
            printf("{\"code\":\"014\"}");
            return;
        }
        


    }

    else
    {
        sprintf(sql_cmd, "update file_info set count=%d where md5 = '%s'", --count, md5.c_str());
        if (!my.Query(sql_cmd))
        {
            LOG(module, pro, "[ERROR]mysql query failed! %s,%s\n", sql_cmd, my.get_error().c_str());
            my.Close();
            printf("{\"code\":\"014\"}");
            return;
        }
    }
    my.Close();
    printf("{\"code\":\"013\"}");
    return;

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
        char *query_string=getenv("QUERY_STRING");
        while(*query_string!='=') query_string++;
        query_string++;
        string cmd=query_string;
        int len=atoi(getenv("CONTENT_LENGTH"));
        
        if(len<=0)
        {
            LOG(module,pro,"[ERROR]len=0,no data from input!\n");
            continue;
        }
        char buff[len+1];
        buff[len]='\0';
        int ret=fread(buff,1,len,stdin);
       
        if(ret==0)
        {
            LOG(module,pro,"[ERROR] fread(buff,1,len,stdin) err\n");
            continue;
        }
       
        ret=get_json_info(buff,user,token,md5,filename);
        LOG(module,pro,"[TEST] %s,%s,%s,%s\n",user.c_str(),md5.c_str(),filename.c_str(),cmd.c_str());
        if(ret==-1)
        continue;
        ret=verify_token(user,token);
        if(ret==0)
        {
             LOG(module,pro,"[ERROR] verify_token failed!\n");
             printf("{\"code\":\"111\"}");
            continue;
        }
        if(cmd=="pv")
        {
            add_pv(mysql_ip,mysql_user,mysql_pwd,mysql_db,mysql_port,user,md5,filename);
        }
        else if(cmd=="share")
        {
            share_file(mysql_ip,mysql_user,mysql_pwd,mysql_db,mysql_port,user,md5,filename);
        }
        else{
            del_file(mysql_ip,mysql_user,mysql_pwd,mysql_db,mysql_port,user,md5,filename);
        }



    }
}