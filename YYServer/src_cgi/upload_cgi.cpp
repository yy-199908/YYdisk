#include <stdlib.h>
#include <string.h>
#include "make_log.h"
#include "util_cgi.h"
#include "YYMysql.h"
#include "YYDATA.h"
#include "JSON.h"
#include "MD5.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <fstream>
#include <iostream>
#include <sstream>
#include<stdint.h>
#include<ctype.h>
#include<errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>//�����ṩ����pid_t��size_t�Ķ���
#include<sys/epoll.h>
#include<sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include "fcgi_config.h"
#include "fcgi_stdio.h"

#define module "cgi"
#define pro "upload"

using namespace std;
using namespace YY;



int get_fdfs_path(string &fdfs_conf)
{
	//��ȡjson�����ļ���ȡfastdfs_tracker������Ϣ
	int ret = 0;
	YYjson j;
	fstream fp("./conf/cfg.json", ios::in);
	if (!fp.is_open())
	{
		LOG(module, pro, "[ERROR]json config open error!\n");
		ret = -1;
		return ret;
	}
	json_value v;
	fp.seekg(0, ios::end);
	int len = fp.tellg();
	fp.seekg(0, ios::beg);

	char buff[len + 1];
	buff[len] = '\0';
	fp.read(buff, len);

	j.parse(&v, buff);
	json_value* dfs_path;
	dfs_path = j.find_object(&v, "dfs_path",8);
	fdfs_conf = j.find_object(dfs_path, "client", 6)->u.s.s;
	fp.close();
	j.json_free(&v);
	return ret;


}



int get_Mysql_info(string& ip,int& port,string& user,string& pwd,string& db)
{
	//��ȡjson�����ļ���ȡfastdfs_tracker������Ϣ
	int ret = 0;
	YYjson j;
	fstream fp("./conf/cfg.json", ios::in);
	if (!fp.is_open())
	{
		LOG(module, pro, "[ERROR]json config open error!\n");
		ret = -1;
		return ret;
	}
	json_value v;
	fp.seekg(0, ios::end);
	int len = fp.tellg();
	fp.seekg(0, ios::beg);

	char buff[len + 1];
	buff[len] = '\0';
	fp.read(buff, len);

	j.parse(&v, buff);
	json_value* mysql;
	mysql = j.find_object(&v, "mysql", 5);
	ip = j.find_object(mysql, "ip", 2)->u.s.s;
	pwd = j.find_object(mysql, "password", 8)->u.s.s;
	user = j.find_object(mysql, "user", 4)->u.s.s;
	db = j.find_object(mysql, "database", 8)->u.s.s;
	port = atoi(j.find_object(mysql, "port", 4)->u.s.s);
	fp.close();
	j.json_free(&v);
	return ret;


}




int recv_save_file(long len, string& user, string& filename, string& md5, long& size)
{
	int ret = 0;
	char* file_buf = NULL;
	char* begin = NULL;
	file_buf = (char*)malloc(len);
	if (file_buf == NULL)
	{
		LOG(module, pro, "[ERROR]malloc error! file size is to big!!!!\n");
		ret= -1;
		return -1;
	}
	int ret2 = fread(file_buf, 1, len, stdin);//��ȡ����
	if (ret2 == 0)//��ȡʧ��
	{
		LOG(module, pro, "[ERROR]fread error\n");
		ret= -1;
		free(file_buf);
		return ret;
	}
	/*
	  ------WebKitFormBoundary88asdgewtgewx\r\n
	  Content-Disposition: form-data; user="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
	  Content-Type: application/octet-stream\r\n
	  \r\n
	  �������ļ�����\r\n
	  ------WebKitFormBoundary88asdgewtgewx
	  */
	  //get boundary �õ��ֽ���, ------WebKitFormBoundary88asdgewtgewx
	char* r_beg, * r_end;
	r_beg = file_buf;
	r_end = file_buf;
	int i = 0;
	r_end = strstr(file_buf,"\r\n");
	r_end+=2;
	r_beg = r_end;//�����ڶ���
	while (*r_end != '\r' and *(r_end + 1) != '\n') r_end++;

	*r_end = '\0';
	
	string header = r_beg;

	stringstream hh(header);
//��ȡ�������ڶ������ݣ�
	
	string header_split;
	while (getline(hh, header_split, ';'))
	{
		if (header_split.find("Content") != string::npos) continue;
		else if (header_split.find("user") != string::npos)
		{
			int pos = header_split.find("\"");
			user = header_split.substr(pos + 1, header_split.size() - pos-2);
		}
		else if (header_split.find("filename") != string::npos)
		{
			int pos = header_split.find("\"");
			filename = header_split.substr(pos + 1, header_split.size() - pos-2);
		}
		else if (header_split.find("md5") != string::npos)
		{
			int pos = header_split.find("\"");
			md5 = header_split.substr(pos + 1, header_split.size() - pos - 2);
		}
		else if (header_split.find("size") != string::npos)
		{
			int pos = header_split.find("=");
			size = atol(header_split.substr(pos + 1, header_split.size() - pos - 1).c_str());
		}
	}


	//���������к͵�����
	r_end += 2;//������
	while (*r_end != '\r' and *(r_end + 1) != '\n') r_end++;
	r_end += 2;//������
	while (*r_end != '\r' and *(r_end + 1) != '\n') r_end++;
	r_end += 2;//������
	

	r_beg = r_end;//�ļ���ʼ


	//д�뱾���ļ�
	int fd = 0;
	fd = open(filename.c_str(), O_CREAT | O_WRONLY, 0644);
	if (fd < 0)
	{
		LOG(module, pro, "[ERROR]open %s error!\n",filename.c_str());
		ret = -1;
		free(file_buf);
		return ret;
	}
	//ftruncate�Ὣ����fdָ�����ļ���С��Ϊ����lengthָ���Ĵ�С
	ftruncate(fd, size);
	char* file = (char*)malloc(size);
	memcpy(file, r_beg, size);
	write(fd,file, size);
	close(fd);

	free(file);
	free(file_buf);
	return ret;

}



int upload_to_storage(string filename, string& fileid)
{
	
	//���͸�fdfs_server
	int fd[2];
	pid_t pid;
	if (pipe(fd) < 0)
	{
		LOG(module, pro, "[ERROR]pipe() error!\n");
		
		return -1;
	}
	
	pid = fork();
	if (pid < 0)
	{
		LOG(module, pro, "[ERROR]fork() error!\n");
		return -1;
	}
	
	if (pid == 0)//�ӽ���,�ϴ��ļ�
	{
		dup2(fd[1],1);
		close(fd[0]);
		string fdfs_conf_path;
		get_fdfs_path(fdfs_conf_path);
		execlp("fdfs_upload_file", "fdfs_upload_file", fdfs_conf_path.c_str(),filename.c_str(),NULL);
		LOG(module, pro, "[ERROR]fdfs_upload_file() error!\n");
		return -1;

	}
	else//�����̣���ȡ�ļ�id
	{
		close(fd[1]);
		char id[1024];
		memset(id,'\0',1024);
		read(fd[0],id,1024);
		fileid = id;
		fileid=fileid.substr(0,fileid.size()-1);
		//trim_space(fileid);
		if (fileid.size() == 0)
		{
			LOG(module, pro, "[ERROR]upload failed!\n");
			wait(NULL);
			close(fd[0]);
			return-1;
		}
		wait(NULL);//�ȴ��ӽ��̻�����Դ
		close(fd[0]);
	}
	
	return 0;
}

int get_file_url(string file_id, string& url)
{
	int ret = 0;
	pid_t pid;
	int fd[2];
	if (pipe(fd) < 0)
	{
		LOG(module, pro, "[ERROR]pipe() failed!\n");
	
		return -1;
	}
	
	pid = fork();
	if (pid < 0)
	{
		LOG(module, pro, "[ERROR]fork() failed!\n");
		return -1;
	}
	
	if (pid == 0)//�ӽ���  ִ��fdfs_file_info
	{
		close(fd[0]);
		dup2(fd[1], 1);
		string fdfs_conf;
		get_fdfs_path(fdfs_conf);
		execlp("fdfs_file_info","fdfs_file_info",fdfs_conf.c_str(), file_id.c_str(),NULL);
		LOG(module, pro, "[ERROR]execlp failed!\n");
		ret = -1;
		close(fd[1]);
	}
	else//�����̣���ȡfdfs_file_info���ص����ݣ�ƴ��url
	{
		//url:http://192.168.234.100/group1/M00/00.....
		close(fd[1]);
		char buff[1024];
		memset(buff,'\0',1024);
		read(fd[0], buff, 1024);
		wait(NULL);
		
		close(fd[0]);
		string info = buff;
		stringstream ss(info);
		string ip;
		getline(ss,ip);
		getline(ss, ip);//�ڶ��Ų���source ip address: 192.168.234.100
		
		int pos = ip.find(":");
		ip = ip.substr(pos + 2, ip.size() - pos - 2);
		url = "http://";
		url += ip;
		url += "/";
		url += file_id;
	}
	return ret;
}



int store_to_mysql(string user, string filename, string md5, long size, string fileid, string url)
{

	time_t now;
	string sql;
	string ip, my_user, pwd, db;
	int port;
	get_Mysql_info(ip,port, my_user,pwd,db);
	YYMysql my;
	my.Init();
	my.SetReconnect(1);
	
	if (!my.Connect(ip.c_str(), my_user.c_str(), pwd.c_str(), db.c_str(), port, 0))
	{
		LOG(module, pro, "[ERROR]connect mysql failed!\n");
		return -1;
	}
	string suffix;
	get_file_suffix(filename.c_str(), suffix);
	
	/*--------------�ļ���Ϣ����Ϣ-----------------
	 -- md5 �ļ�md5
       -- file_id �ļ�id
       -- url �ļ�url
       -- size �ļ���С, ���ֽ�Ϊ��λ
       -- type �ļ����ͣ� png, zip, mp4����
       -- count �ļ����ü����� Ĭ��Ϊ1�� ÿ����һ���û�ӵ�д��ļ����˼�����+1
	*/
	sql = "insert into file_info (md5,file_id,url,size,type,count) values ('";
	sql += md5;
	sql += "','";
	sql += fileid;
	sql += "','";
	sql += url;
	sql += "','";
	sql += to_string(size);
	sql += "','";
	sql += suffix;
	sql += "','1')";
	if (!my.Query(sql.c_str()))
	{
		LOG(module, pro, "[ERROR]mysql insert file_info failed! %s,%s\n",sql.c_str(),my.get_error().c_str());
		
		my.Close();
		return -1;
	}

	/*
	   -- -----------�û��ļ��б�--------------------- 
	   -- user �ļ������û�
	   -- md5 �ļ�md5
	   -- createtime �ļ�����ʱ��
	   -- filename �ļ�����
	   -- shared_status ����״̬, 0Ϊû�й����� 1Ϊ����
	   -- pv �ļ���������Ĭ��ֵΪ0������һ�μ�1
	   */

	now = time(NULL);
	char create_time[25];
	strftime(create_time,24, "%Y-%m-%d %H:%M:%S", localtime(&now));
	sql = "insert into user_file_list(user,md5,createtime,filename,shared_status,pv) values('";
	sql += user;
	sql += "','";
	sql += md5;
	sql += "','";
	sql += create_time;
	sql += "','";
	sql += filename;
	sql += "','0','0')";
	if (!my.Query(sql.c_str()))
	{
		LOG(module, pro, "[ERROR]mysql insert user_file_list failed!\n");
		
		my.Close();
		return -1;
	}


	//��ѯ�û��ļ����������û��ļ�������
	sql = "select count from user_file_count where user='";
	sql += user;
	sql += "' for update";
	YYROWS rows = my.GetResult(sql.c_str());
	if (rows.size() == 0)//���û�û�м�¼�������¼
	{
		sql = "insert into user_file_count(user,count) values('";
		sql += user.c_str();
		sql += "','1')";
		if (!my.Query(sql.c_str()))
		{
			LOG(module, pro, "[ERROR]mysql insert user_file_count failed!\n");
			
			my.Close();
			return -1;
		}
	}
	else
	{
		sql = "update user_file_count set count = '";
		sql += to_string(atoi(rows[0][0].data)+1);
		sql += "' where user='";
		sql += user;
		sql+="'";
		if (!my.Query(sql.c_str()))
		{
			LOG(module, pro, "[ERROR]mysql update user_file_count failed!\n");
			
			my.Close();
			return -1;
		}
	}



	my.Close();
	return 0;


}



/*
    ------WebKitFormBoundary88asdgewtgewx\r\n
    Content-Disposition: form-data; user="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    Content-Type: application/octet-stream\r\n
    \r\n
    �������ļ�����\r\n
    ------WebKitFormBoundary88asdgewtgewx
    */






/*
            �ϴ��ļ���
                �ɹ���{"code":"008"}
                ʧ�ܣ�{"code":"009"}
            */

int main()
{

	
	string filename,user,md5,fileid,fdfs_storage;
	YYMysql my;
	string file_url;
	long size;
	while (FCGI_Accept() >= 0)
	{
		filename.clear();
		user.clear(); md5.clear(); fileid.clear(); fdfs_storage.clear();
		char* content_length = getenv("CONTENT_LENGTH");
		long len=0;
	
		printf("Content-type:text/html\r\n\r\n");
		if (content_length != NULL)
			len = atol(content_length);
		if (len <= 0)
		{
			printf("No data from standard input\n");
			LOG(module,pro, "[ERROR]len = 0, No data from standard input\n");
			printf("{\"code\":\"009\"}");
			continue;
		}
		else
		{
			LOG(module, pro, "[test]recv_save_file\n");
			if (recv_save_file(len, user, filename, md5, size) < 0)//��ȡ�ϴ��ļ�
			{
				printf("{\"code\":\"009\"}");
				continue;
			}
			LOG(module, pro, "[INFO]%s upload %s to local! size:%ld, md5:%s\n", user.c_str(), filename.c_str(), size, md5.c_str());
			if (upload_to_storage(filename, fileid) < 0) //�ϴ���fastdfs
			{
				LOG(module, pro, "[ERROR]upload to fdfs_server failed\n");
			
				printf("{\"code\":\"009\"}");
				continue;
			}
			LOG(module, pro, "[INFO]%s upload %s to local! size:%ld, md5:%s, fileid:%s", user.c_str(), filename.c_str(), size, md5.c_str(),fileid.c_str());
			unlink(filename.c_str());
			if (get_file_url(fileid, file_url) < 0)
			{
				LOG(module, pro, "[ERROR]get_file_url() failed\n");
				printf("{\"code\":\"009\"}");
				continue;
			}
			
			if (store_to_mysql(user, filename, md5, size, fileid, file_url) < 0)
			{
				LOG(module, pro, "[ERROR]store_to_mysql() failed\n");
				printf("{\"code\":\"009\"}");
				continue;
			}
			
		}
			printf("{\"code\":\"008\"}");
	}
	return 0;
}