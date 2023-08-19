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
		} a;//json_value 数组类型
		struct {
			char* s;
			int len;
		} s;//字符串类型
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
	size_t klen;//key的长度
	json_value v;//value

};






enum {
	JSON_KEY_NOT_EXIST=-1,//查找值不存在
	PARSE_OK = 0,//解析成功
	PARSE_EXPECT_VALUE,//json为空
	PARSE_INVALID_VALUE,//值类型错误
	PARSE_NUMBER_TOO_BIG,//数字类型过大
	PARSE_INVALID_STRING_ESCAPE,//sting未识别字符型
	PARSE_MISS_QUOTATION_MARK,//缺少右引号
	PARSE_INVALID_STRING_CHAR,//非法字符
	PARSE_INVALID_UNICODE_HEX,//unicode 非法字符
	PARSE_INVALID_UNICODE_SURROGATE,//unicode 高低代理格式错误
	PARSE_MISS_COMMA_OR_SQUARE_BRACKET,//数组中存在错误类型或者缺少]
	PARSE_MISS_KEY,//obj类型缺少key或key错误
	PARSE_MISS_COLON,//缺少:冒号
	PARSE_MISS_COMMA_OR_CURLY_BRACKET,//obj括号个数不匹配
	PARSE_ROOT_NOT_SINGULAR,//一个值之后，空白之后还有其他字符

	
	STRINGIFY_OK//生成成功
};

#define EXPECT(c, ch) do { assert(*c== (ch)); c++; } while(0)

class YYjson
{
public:
	YYjson() {};
	~YYjson() {};


//解析json字符串
int parse(json_value *v,const char* json);
//获取类型
json_type get_type(const json_value* v);

//初始化json_value
void init(json_value* v);
//free json_value 内存
void json_free(json_value* v);




//写入
int add_object(json_value *root, const char *key,json_value &v);



int get_boolean(const json_value* v);
void set_boolean(json_value* v, int b);

double get_number(const json_value* v);
void set_number(json_value* v, double n);


//动态请求内存并接收字符串
void set_string( json_value* v, const char* s, int len);
const char* get_string(const json_value* v);
size_t get_string_length(const json_value* v);
size_t get_array_size(const json_value *v);
json_value* get_array_element(const json_value* v, size_t index);


size_t get_object_size(const json_value* v);
const char* get_object_key(const json_value* v, size_t index);
size_t get_object_key_length(const json_value* v, size_t index);
json_value* get_object_value(const json_value* v, size_t index);



//输出json字符串
int stringify(const json_value* v, char** json, size_t* length);


//查找
size_t find_object_index(const json_value* v, const char* key, size_t klen);
json_value *find_object(const json_value* v, const char* key, size_t klen);


//比较
int is_equal(const json_value* lhs, const json_value* rhs);



private:
	//跳过空白
	//因为要在内部修改指针c本身，而不是其指向得地址块存储值，所以需要引用
	//因为传入指针时，传入的是指向同一地址块的指针复制临时变量，在函数结束时释放，对其做改变没效果，
	//但对其指向的地址块做改变有效果
	void parse_whitespace(const char*& c);

	//判断是否为对应type，null，true或者false
	int parse_literal(const char*& c, json_value* v, json_type type);
	//判断值类型
	int parse_value(const char*& c, json_value* v);
	//解析数字
	int parse_number(const char*&c, json_value* v);
	//解析字符串
	int parse_string_raw(const char* &c, char** str, size_t* len);
	int parse_string(const char*&c,json_value*v);
	//解析数组
	int parse_array(const char*&c,json_value*v);
	//解析obj
	int parse_object(const char*& c, json_value* v);
	//xxxx->转化为usigned int 16位二进制数字存入u中
	const char* parse_hex4(const char* p, unsigned* u);
	void encode_utf8( unsigned u,std::string& str);



	//生成

	int stringify_value(std::string* c, const json_value* v);
	int	stringify_string(std::string* c, const json_value* v);
	int stringify_array(std::string* c, const json_value* v);
	int	stringify_object(std::string* c, const json_value* v);
	

	std::string doubleToString(double price);
	


	const char* myjson;


};

	



#endif