#ifndef MAKE_LOG_H
#define MAKE_LOG_H

#include<fstream>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>
#include<sys/stat.h>
#include <unistd.h>

#ifndef _LOG
#define LOG(module_name, proc_name, x...) \
        do{ \
		make_log log;\
		log.dumpmsg_to_file(module_name, proc_name, __FILE__, __LINE__, __FUNCTION__, ##x);\
	}while(0)

#endif // !



class make_log
{
public:
	~make_log() {};
	make_log() {};
	
	//写入内容
	int out_put_file(char* path, char* buf);
	//创建目录
	int make_path(char* path, const char* module_name, const char* proc_name); 

	//创建目录并写入内容
	int dumpmsg_to_file(const char* module_name, const char* proc_name, const char* filename,
		int line, const char* funcname, char* fmt, ...);
private:
	pthread_mutex_t ca_log_lock = PTHREAD_MUTEX_INITIALIZER;
};

#endif

