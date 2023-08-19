#include "make_log.h"
#include <iostream>
using namespace std;

//写入内容
int make_log::out_put_file(char* path, char* buf)
{
	int fd;
	fd = open(path, O_RDWR | O_CREAT | O_APPEND, 0777);//可读可写，不存在自动建立，追加

	if (write(fd, buf, strlen(buf)) != (int)strlen(buf)) {
		fprintf(stderr, "write error!\n");
		close(fd);
	}
	else {
		//write(fd, "\n", 1);
		close(fd);
	}

	return 0;


}
//创建目录
int make_log::make_path(char* path, const char* module_name, const char* proc_name)
{
	time_t t;
	struct tm* now = NULL;
	char top_dir[1024] = { "." };
	char second_dir[1024] = { "./logs" };
	char third_dir[1024] = { 0 };
	char y_dir[1024] = { 0 };
	char m_dir[1024] = { 0 };
	char d_dir[1024] = { 0 };
	time(&t);
	now = localtime(&t);
	snprintf(path, 1024, "./logs/%s/%04d/%02d/%s-%02d.log", module_name, now->tm_year + 1900, now->tm_mon + 1, proc_name, now->tm_mday);
	sprintf(third_dir,"%s/%s",second_dir,module_name);
	sprintf(y_dir,"%s/%04d/",third_dir,now->tm_year+1900);
	sprintf(m_dir, "%s/%02d/", y_dir, now->tm_mon+1);
	sprintf(d_dir, "%s/%02d/", m_dir, now->tm_mday);
	if (access(top_dir, 0) == -1)//判断当前用户是否拥有存在
	{
		if (mkdir(top_dir, 0777) == -1)
		{
			cerr << "create " << top_dir << " failed!" << endl;
		}
		else if (mkdir(third_dir, 0777) == -1) {
			cerr << top_dir << " create " << third_dir << " failed!" << endl;
		}
		else if (mkdir(y_dir, 0777) == -1) {
			cerr << top_dir << " create " << y_dir << " failed!" << endl;
		}
		else if (mkdir(m_dir, 0777) == -1) {
			cerr << top_dir << " create " << m_dir << " failed!" << endl;
		}
	}
	else if (access(second_dir, 0) == -1)
	{
		if (mkdir(second_dir, 0777) == -1)
		{
			cerr << "create " << second_dir << " failed!" << endl;
		}
		else if (mkdir(third_dir, 0777) == -1) {
			cerr << second_dir << " create " << third_dir << " failed!" << endl;
		}
		else if (mkdir(y_dir, 0777) == -1) {
			cerr << second_dir << " create " << y_dir << " failed!" << endl;
		}
		else if (mkdir(m_dir, 0777) == -1) {
			cerr << second_dir << " create " << m_dir << " failed!" << endl;
		}
	}
	else if (access(third_dir, 0) == -1)
	{

		if (mkdir(third_dir, 0777) == -1)
		{
			cerr << "create " << third_dir << " failed!" << endl;
		}
		else if (mkdir(y_dir, 0777) == -1) {
			cerr << third_dir << " create " << y_dir << " failed!" << endl;
		}
		else if (mkdir(m_dir, 0777) == -1) {
			cerr << third_dir << " create " << m_dir << " failed!" << endl;
		}
	}
	else if (access(y_dir, 0) == -1)
	{

		if (mkdir(y_dir, 0777) == -1)
		{
			cerr << "create " << y_dir << " failed!" << endl;
		}
		else if (mkdir(m_dir, 0777) == -1) {
			cerr << y_dir << " create " << m_dir << " failed!" << endl;
		}
	}
	else if (access(m_dir, 0) == -1)
	{
		if (mkdir(m_dir, 0777) == -1)
		{
			cerr << "create " << m_dir << " failed!" << endl;
		}
	}
	return 0;
}

//创建目录并写入内容
int make_log::dumpmsg_to_file(const char* module_name, const char* proc_name, const char* filename,
	int line, const char* funcname, char* fmt, ...)
{
	char mesg[4096] = { 0 };
	char buf[4096] = { 0 };
	char filepath[1024] = { 0 };
	time_t t = 0;
	struct tm* now = NULL;
	va_list ap;
	//struct file_path *path;
	//path = (struct file_path*)paths;
	time(&t);
	now = localtime(&t);
	va_start(ap, fmt);
	vsprintf(mesg, fmt, ap);
	va_end(ap);
	
	snprintf(buf, 4096, "[%04d-%02d-%02d %02d:%02d:%02d]--[%s:%d]--%s",
		now->tm_year + 1900, now->tm_mon + 1,
		now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec,
		filename, line, mesg);
	
	
	make_path(filepath, module_name, proc_name);
	pthread_mutex_lock(&ca_log_lock);//线程锁
	out_put_file(filepath, buf);
	pthread_mutex_unlock(&ca_log_lock);//解锁

	return 0;

}


