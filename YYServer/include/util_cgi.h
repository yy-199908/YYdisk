#ifndef UTIL_CGI_H
#define UTIL_CGI_H
#include <string>
#include <unordered_map>
#include <string.h>
#include "YYMysql.h"
#include "YYDATA.h"

//ȥ��һ���ַ������ߵĿհ�
int trim_space(std::string& inbuf);


//����full_data�ַ�����substr��һ�γ��ֵ�λ��
char* memstr(char* fulldata, int full_data_len, char* substr);

int connect_Mysql(YY::YYMysql &my);

/**
*@brief  ����url query ���� abc = 123 & bbb = 456 �ַ���
* ����һ��key, �õ���Ӧ��value
* @returns
* 0 �ɹ�, -1 ʧ��
*/

int query_parse_key_value(const char* query, std::unordered_map<std::string,std::string> &map);

//ͨ���ļ���file_name�� �õ��ļ���׺�ַ���, ������suffix ����Ƿ��ļ���׺,����"null"
int get_file_suffix(const char* file_name, std::string &suffix);



//�ַ���strSrc�е��ִ�strFind���滻ΪstrReplace
int str_replace(std::string& strSrc, std::string strFind, std::string strReplace);


////��֤��½token���ɹ�����0��ʧ��-1
int verify_token(std::string user, std::string token);

#endif

