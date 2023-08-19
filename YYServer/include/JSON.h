#ifndef JSON_H
#define JSON_H
#include <errno.h>   /* errno, ERANGE */
#include <math.h>     /* HUGE_VAL */
#include <assert.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <regex>
#include <sstream>
typedef enum { JSON_NULL, JSON_FALSE, JSON_TRUE, JSON_NUMBER, JSON_STRING, JSON_ARRAY, JSON_OBJECT } json_type;
typedef struct json_value json_value;
typedef struct json_member json_member;

struct json_value {
	union {
		struct
		{
			json_value* e;
			int size;
		} a;//json_value ��������
		struct {
			char* s;
			int len;
		} s;//�ַ�������
		struct { 
			json_member * m;
			size_t size;
		} o;


		double n;
	} u;
	json_type type;
};


struct json_member
{
	char* k;//key
	size_t klen;//key�ĳ���
	json_value v;//value

};






enum {
	JSON_KEY_NOT_EXIST=-1,//����ֵ������
	PARSE_OK = 0,//�����ɹ�
	PARSE_EXPECT_VALUE,//jsonΪ��
	PARSE_INVALID_VALUE,//ֵ���ʹ���
	PARSE_NUMBER_TOO_BIG,//�������͹���
	PARSE_INVALID_STRING_ESCAPE,//stingδʶ���ַ���
	PARSE_MISS_QUOTATION_MARK,//ȱ��������
	PARSE_INVALID_STRING_CHAR,//�Ƿ��ַ�
	PARSE_INVALID_UNICODE_HEX,//unicode �Ƿ��ַ�
	PARSE_INVALID_UNICODE_SURROGATE,//unicode �ߵʹ����ʽ����
	PARSE_MISS_COMMA_OR_SQUARE_BRACKET,//�����д��ڴ������ͻ���ȱ��]
	PARSE_MISS_KEY,//obj����ȱ��key��key����
	PARSE_MISS_COLON,//ȱ��:ð��
	PARSE_MISS_COMMA_OR_CURLY_BRACKET,//obj���Ÿ�����ƥ��
	PARSE_ROOT_NOT_SINGULAR,//һ��ֵ֮�󣬿հ�֮���������ַ�

	
	STRINGIFY_OK//���ɳɹ�
};

#define EXPECT(c, ch) do { assert(*c== (ch)); c++; } while(0)

class YYjson
{
public:
	YYjson() {};
	~YYjson() {};


//����json�ַ���
int parse(json_value *v,const char* json);
//��ȡ����
json_type get_type(const json_value* v);

//��ʼ��json_value
void init(json_value* v);
//free json_value �ڴ�
void json_free(json_value* v);




//д��
int add_object(json_value *root, const char *key,json_value &v);



int get_boolean(const json_value* v);
void set_boolean(json_value* v, int b);

double get_number(const json_value* v);
void set_number(json_value* v, double n);


//��̬�����ڴ沢�����ַ���
void set_string( json_value* v, const char* s, int len);
const char* get_string(const json_value* v);
size_t get_string_length(const json_value* v);
size_t get_array_size(const json_value *v);
json_value* get_array_element(const json_value* v, size_t index);


size_t get_object_size(const json_value* v);
const char* get_object_key(const json_value* v, size_t index);
size_t get_object_key_length(const json_value* v, size_t index);
json_value* get_object_value(const json_value* v, size_t index);



//���json�ַ���
int stringify(const json_value* v, char** json, size_t* length);


//����
size_t find_object_index(const json_value* v, const char* key, size_t klen);
json_value *find_object(const json_value* v, const char* key, size_t klen);


//�Ƚ�
int is_equal(const json_value* lhs, const json_value* rhs);



private:
	//�����հ�
	//��ΪҪ���ڲ��޸�ָ��c������������ָ��õ�ַ��洢ֵ��������Ҫ����
	//��Ϊ����ָ��ʱ���������ָ��ͬһ��ַ���ָ�븴����ʱ�������ں�������ʱ�ͷţ��������ı�ûЧ����
	//������ָ��ĵ�ַ�����ı���Ч��
	void parse_whitespace(const char*& c);

	//�ж��Ƿ�Ϊ��Ӧtype��null��true����false
	int parse_literal(const char*& c, json_value* v, json_type type);
	//�ж�ֵ����
	int parse_value(const char*& c, json_value* v);
	//��������
	int parse_number(const char*&c, json_value* v);
	//�����ַ���
	int parse_string_raw(const char* &c, char** str, size_t* len);
	int parse_string(const char*&c,json_value*v);
	//��������
	int parse_array(const char*&c,json_value*v);
	//����obj
	int parse_object(const char*& c, json_value* v);
	//xxxx->ת��Ϊusigned int 16λ���������ִ���u��
	const char* parse_hex4(const char* p, unsigned* u);
	void encode_utf8( unsigned u,std::string& str);



	//����

	int stringify_value(std::string* c, const json_value* v);
	int	stringify_string(std::string* c, const json_value* v);
	int stringify_array(std::string* c, const json_value* v);
	int	stringify_object(std::string* c, const json_value* v);
	

	std::string doubleToString(double price);
	


	const char* myjson;


};

	



#endif