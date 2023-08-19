#ifndef UTIL_CGI_H
#define UTIL_CGI_H
#include <string>
#include <unordered_map>
#include <string.h>
#include "YYMysql.h"
#include "YYDATA.h"

//去掉一个字符串两边的空白
int trim_space(std::string& inbuf);


//查找full_data字符串中substr第一次出现的位置
char* memstr(char* fulldata, int full_data_len, char* substr);

int connect_Mysql(YY::YYMysql &my);

/**
*@brief  解析url query 类似 abc = 123 & bbb = 456 字符串
* 传入一个key, 得到相应的value
* @returns
* 0 成功, -1 失败
*/

int query_parse_key_value(const char* query, std::unordered_map<std::string,std::string> &map);

//通过文件名file_name， 得到文件后缀字符串, 保存在suffix 如果非法文件后缀,返回"null"
int get_file_suffix(const char* file_name, std::string &suffix);



//字符串strSrc中的字串strFind，替换为strReplace
int str_replace(std::string& strSrc, std::string strFind, std::string strReplace);


////验证登陆token，成功返回0，失败-1
int verify_token(std::string user, std::string token);

#endif

