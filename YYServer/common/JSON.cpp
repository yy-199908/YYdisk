#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "JSON.h"
using namespace std;

int YYjson::parse(json_value* v, const char* json)
{

	assert(v != NULL);
	myjson = json;
	v->type = JSON_NULL;
	parse_whitespace(myjson);
	int ret = parse_value(myjson, v);
	if(ret == PARSE_OK)
	{
		parse_whitespace(myjson);
		if (*myjson != '\0')
			return PARSE_ROOT_NOT_SINGULAR;
	}
	return ret;
}
//��ȡ����
json_type YYjson::get_type(const json_value* v)
{
	return v->type;
}


 void YYjson::parse_whitespace(const char*& c) {
	const char* p = c;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
		p++;
	c= p;
}


int YYjson::parse_literal(const char*& c, json_value* v,json_type type) {
	const char* literal;
	if (type == JSON_FALSE) literal = "false";
	else if (type == JSON_NULL) literal = "null";
	else if (type == JSON_TRUE) literal = "true";
	else literal = "";
	assert(*c == literal[0]);
	c++;
	int i = 0;
	for ( i = 0; literal[i + 1]; i++)
	{
		if (c[i] != literal[i+1])
			return PARSE_INVALID_VALUE;
	}
	c += i;
	v->type = type;
	return PARSE_OK;
}



int YYjson::parse_value(const char* &c, json_value* v) {
	switch (*c) {
	case 'n':  return parse_literal(c, v,JSON_NULL);
	case 'f':  return parse_literal(c, v,JSON_FALSE);
	case 't':  return parse_literal(c, v, JSON_TRUE);
	case '"':  return parse_string(c, v);
	case '[':  return parse_array(c, v);
	case '{':  return parse_object(c, v);
	case '\0': return PARSE_EXPECT_VALUE;
	
	default:   return parse_number(c,v);
	}
}




int YYjson::parse_number(const char*& c, json_value* v)
{
	const char* p = c;
	if (*p == '-')p++;
	if (*p == '0')
	{
		p++;
		if (*p != '\0' and  *p != ']' and *p != '.' and *p != ',' and *p != ' ' and *p != '\r' and *p != '\n')
			return PARSE_INVALID_VALUE;
	}
	else {
		if (*p < '1' or *p>'9') return PARSE_INVALID_VALUE;
		for (; (*p >= '0' and *p <= '9'); p++);
	}

	if (*p == '.')
	{
		p++;
		if (*p < '0' or *p>'9') return PARSE_INVALID_VALUE;
		for (; (*p >= '0' and *p <= '9'); p++);
	}
	if (*p == 'e' || *p == 'E') {
		p++;
		if (*p == '+' || *p == '-') p++;
		if (*p < '0' or *p>'9') return PARSE_INVALID_VALUE;
		for (p++; (*p >= '0' and *p <= '9'); p++);
	}
	errno = 0;
	
	
	
	v->u.n = strtod(c,NULL);
	c = p;
	if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL)) return PARSE_NUMBER_TOO_BIG;
	v->type = JSON_NUMBER;

	return PARSE_OK;
}



void YYjson::init(json_value* v) {
	v->type = JSON_NULL;
}



void YYjson::json_free(json_value* v) {
	assert(v != NULL);
	if (v->type == JSON_STRING)
		free(v->u.s.s);
	if (v->type == JSON_ARRAY)
	{
		for (int i = 0; i < v->u.a.size; i++)
			json_free(&v->u.a.e[i]);
		free(v->u.a.e);
	}

	if (v->type == JSON_OBJECT)
	{
		for (int i = v->u.o.size-1; i >=0; i--)
		{
			json_free(&(v->u.o.m[i].v));
			free(v->u.o.m[i].k);
			
		}
		free(v->u.o.m);
	}
	v->type = JSON_NULL;
}


int  YYjson::parse_array(const char*& c, json_value* v)
{
	int ret=0;
	int size=0;
	assert(*c == '[');
	c++;
	parse_whitespace(c);
	if (*c == ']') {//������
		c++;
		v->type = JSON_ARRAY;
		v->u.a.size = 0;
		v->u.a.e = NULL;
		return PARSE_OK;
	}
	vector<json_value> array;
	
	while (1)
	{
		json_value e;
		init(&e);
		if (ret = parse_value(c, &e) != PARSE_OK) return ret;
		size++;
		parse_whitespace(c);
		array.push_back(e);
		if (*c == ',')
		{
			c++;
			parse_whitespace(c);
		}
		else if (*c == ']')
		{
			c++;
			v->type = JSON_ARRAY;
			v->u.a.size = size;
			v->u.a.e = (json_value*)malloc(size * sizeof(json_value));
			json_value *tmp= v->u.a.e;
			for (int i = 0; i < size; i++)
			{
				memcpy(tmp, &array[i], sizeof(json_value));
				tmp++;
			}
			return PARSE_OK;

		}
		else 
			return PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
	}


}



int YYjson::parse_string_raw(const char*& c, char** strr, size_t* len)
{
	assert(*c == '\"');
	c++;
	string str;
	unsigned u, u2;
	const char* p = c;
	while (1)
	{
		char ch = *p++;
		switch (ch)
		{
		case '\"':
			*strr = (char*)malloc((str.size()+1) * sizeof(char));
			memcpy(*strr,str.c_str(), str.size());
			(*strr)[str.size()] = '\0';
			*len = str.size();
			c = p;
			return PARSE_OK;
		case '\\'://ע��һ��\����Ҳ��Ҫת��
			switch (*p++)
			{
			case '\"': str += '\"'; break;
			case '\\': str += '\\'; break;
			case '/': str += '/'; break;
			case 'b': str += '\b'; break;
			case 'f': str += '\f';  break;
			case 'n': str += '\n'; break;
			case 't': str += '\t'; break;
			case 'r': str += '\r'; break;
			case 'u'://utf-8�������
				if (!(p = parse_hex4(p, &u)))
					return PARSE_INVALID_UNICODE_HEX;
				if (u >= 0xD800 and u <= 0xDBFF)//�ߴ���
				{
					if (*p++ != '\\')
						return PARSE_INVALID_UNICODE_SURROGATE;
					if (*p++ != 'u')
						return PARSE_INVALID_UNICODE_SURROGATE;
					if (!(p = parse_hex4(p, &u2)))
						return PARSE_INVALID_UNICODE_HEX;
					if (u2 < 0xDC00 || u2 > 0xDFFF)
						return PARSE_INVALID_UNICODE_SURROGATE;
					u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
				}
				encode_utf8(u, str);
			default:
				return PARSE_INVALID_STRING_ESCAPE;;
			}
			break;
		case '\0':
			return PARSE_MISS_QUOTATION_MARK;
		default:
			if ((unsigned char)ch < 0x20) {
				return PARSE_INVALID_STRING_CHAR;
			}
			str += ch;
		}

	}
}




int YYjson::parse_string(const char* &c, json_value* v)
{
	int ret;
	char* s;
	size_t len;
	if ((ret = parse_string_raw(c, &s, &len)) == PARSE_OK)
	{
		set_string(v, s, len);
		free(s);
	}
	
	
	return ret;
}







int YYjson::parse_object(const char*& c, json_value* v)
{
	size_t size;
	json_member m;
	int ret;
	assert(*c == '{');
	c++;
	parse_whitespace(c);
	if (*c == '}')
	{
		c++;
		v->type = JSON_OBJECT;
		v->u.o.m = 0;
		v->u.o.size = 0;
		return PARSE_OK;
	}
	m.k = NULL;
	size = 0;
	vector<json_member> sss;
	while (1)
	{
		char* str;
		init(&m.v);
		if (*c != '"')
		{
			ret = PARSE_MISS_KEY;
			break;
		}
		if ((ret = parse_string_raw(c, &str, &m.klen)) != PARSE_OK)
			break;
		//��ֵkey
		m.k = (char*)malloc(m.klen + 1);
		memcpy(m.k, str, m.klen);
		m.k[m.klen] = '\0';
		free(str);

		parse_whitespace(c);
		if (*c!= ':') {
			ret = PARSE_MISS_COLON;
			break;
		}
		c++;
		parse_whitespace(c);
		//��ȡֵ,��������
		if ((ret = parse_value(c, &m.v)) != PARSE_OK)
			break;
		sss.push_back(m);
		size++;
		parse_whitespace(c);
		if (*c == ',')
		{
			c++;
			parse_whitespace(c);
		}
		else if (*c == '}')
		{
			*c++;
			v->type = JSON_OBJECT;
			v->u.o.size = size;
			v->u.o.m = (json_member*)malloc(size*sizeof(json_member));
			json_member* tmp = v->u.o.m;
			for (int i = 0; i < size; i++)
			{
				memcpy(tmp,&sss[i], sizeof(json_member));
				tmp++;
			}
			return PARSE_OK;
		}
		else
		{
			ret = PARSE_MISS_COMMA_OR_CURLY_BRACKET;
			break;
		}
	}
	
	free(m.k);
	return ret;


}




















int  YYjson::get_boolean(const json_value* v)
{
	assert(v != NULL && (v->type == JSON_TRUE) || v->type == JSON_FALSE);
	return v->type == JSON_TRUE;
}
void  YYjson::set_boolean(json_value* v, int b)
{
	json_free(v);
	v->type = b ? JSON_TRUE : JSON_FALSE;
}


double YYjson::get_number(const json_value* v)
{
	assert(v != NULL && v->type == JSON_NUMBER);
	return v->u.n;
}

void  YYjson::set_number(json_value* v, double n)
{
	json_free(v);
	v->u.n = n;
	v->type = JSON_NUMBER;

}


const char* YYjson::get_string(const json_value* v)
{
	assert(v != NULL && v->type == JSON_STRING);
	return v->u.s.s;
}
size_t  YYjson::get_string_length(const json_value* v)
{
	assert(v != NULL && v->type == JSON_STRING);
	return v->u.s.len;
}

void YYjson::set_string( json_value* v, const char* s, int len)
{
	assert(v != NULL and (s != NULL || len == 0));
	json_free(v);
	v->u.s.s = (char*)malloc(len + 1);
	memcpy(v->u.s.s, s, len);
	v->u.s.s[len] = '\0';
	v->type = JSON_STRING;
	v->u.s.len = len;
}



const char* YYjson::parse_hex4(const char* p, unsigned* u)
{
	//xxxx->ת��16λ����������
	*u = 0;
	for (int i = 0; i < 4; i++)
	{
		char ch = *p++;
		*u <<= 4;
		if (ch >= '0' and ch <= '9') *u |= ch - '0';
		else if (ch >= 'A' and ch <= 'F') *u |= ch - ('A' - 10);
		else if (ch >= 'a' and ch <= 'f') *u |= ch - ('a' - 10);
		else return NULL;
	}
	return p;
}
void YYjson::encode_utf8( unsigned u,string &str)
{
	if (u <= 0x7F)//1�ֽڱ���
	{
		str += (char)(u & 0xff);
	}
	else if (u <= 0x7ff)//2�ֽڱ���
	{
		str += (char)(0xc0 | ((u >> 6) & 0xff));
		str += (char)(0x80 | (u & 0x3f));
	}
	else if (u <= 0xffff)
	{
		str += (char)(0xE0 | ((u >> 12) & 0xff));
		str += (char)(0x80 | ((u >> 6) & 0x3f));
		str += (char)(0x80 | (u & 0x3f));
	}
	else  
	{
		assert(u <= 0x10ffff);
		str += (char)(0xF8 | ((u >> 18) & 0xff));
		str += (char)(0x80 | ((u >> 12) & 0x3f));
		str += (char)(0x80 | ((u >> 6) & 0x3f));
		str += (char)(0x80 | (u & 0x3f));
	}
}





size_t YYjson::get_array_size(const json_value* v)
{
	assert(v != NULL and v->type == JSON_ARRAY);
	return v->u.a.size;
}
json_value* YYjson::get_array_element(const json_value* v, size_t index)
{
	assert(v != NULL and v->type == JSON_ARRAY);
	assert(index<v->u.a.size);
	return &v->u.a.e[index];
}






size_t YYjson::get_object_size(const json_value* v)
{
	assert(v!=NULL and v->type==JSON_OBJECT);
	return v->u.o.size;

}
const char* YYjson::get_object_key(const json_value* v, size_t index)
{
	assert(v != NULL && v->type == JSON_OBJECT);
	assert(index < v->u.o.size);
	return v->u.o.m[index].k;
}
size_t YYjson::get_object_key_length(const json_value* v, size_t index)
{
	assert(v != NULL && v->type == JSON_OBJECT);
	assert(index < v->u.o.size);
	return v->u.o.m[index].klen;
}
json_value* YYjson::get_object_value(const json_value* v, size_t index)
{
	assert(v != NULL && v->type == JSON_OBJECT);
	assert(index < v->u.o.size);
	return &v->u.o.m[index].v;
}








int YYjson::stringify(const json_value* v,char **json, size_t* length)
{
	int ret;
	assert(v != NULL);
	assert(json != NULL);
	string str;
	if ((ret = stringify_value(&str, v)) != STRINGIFY_OK) 
	{
		*json = NULL;
		return ret;
	}
	if (length)
	{
		*length = str.size();
	}
	*json = (char*)malloc(sizeof(char) * (str.size()+1));
	memcpy(*json, str.c_str(), str.size());
	(*json)[str.size()] = '\0';
	return ret;


}


int YYjson::stringify_value(string * c, const json_value* v) {
	size_t i;
	int ret;
	switch (v->type) {
	case JSON_NULL:   *c +="null"; break;
	case JSON_FALSE:  *c += "false"; break;
	case JSON_TRUE:   *c += "true"; break;
	case JSON_STRING:  stringify_string(c, v); break;
	case JSON_NUMBER: *c+=doubleToString(v->u.n); break;
	case JSON_ARRAY:   stringify_array(c,v); break;
	case JSON_OBJECT:  stringify_object(c, v); break;
		/* ... */
	}
	return STRINGIFY_OK;
}




int YYjson::stringify_string(string* c, const json_value* v)
{
	*c += '"';
	*c += v->u.s.s;
	*c += '"';
	return STRINGIFY_OK;
}

int YYjson::stringify_array(string* c, const json_value* v)
{
	*c += "[";
	for (int i = 0; i < v->u.a.size;i++)
	{
		stringify_value(c,&v->u.a.e[i]);
		*c += ",";
	}
	*c = c->substr(0,c->size()-1);
	*c += "]";
	return STRINGIFY_OK;
}
int	YYjson::stringify_object(string* c, const json_value* v)
{
	*c += "{";
	
	for (int i = 0; i < v->u.o.size; i++)
	{
		*c += '"';
		*c += v->u.o.m[i].k;
		*c += '"';
		*c += ":";
		stringify_value(c, &v->u.o.m[i].v);
		*c += ",";
	}
	*c = c->substr(0, c->size() - 1);
	*c += "}";
	return STRINGIFY_OK;
}



size_t YYjson::find_object_index(const json_value* v, const char* key, size_t klen) {
	size_t i;
	assert(v != NULL && v->type == JSON_OBJECT && key != NULL);
	for (i = 0; i < v->u.o.size; i++)
		if (v->u.o.m[i].klen == klen && memcmp(v->u.o.m[i].k, key, klen) == 0)
			return i;
	return JSON_KEY_NOT_EXIST;
}


json_value* YYjson::find_object(const json_value* v, const char* key, size_t klen) {
	int i = find_object_index(v, key, klen);
	if (i == -1)
		return nullptr;
	return get_object_value(v,i);
}


int YYjson::is_equal(const json_value* lhs, const json_value* rhs) {
	assert(lhs != NULL && rhs != NULL);
	if (lhs->type != rhs->type)
		return 0;
	switch (lhs->type) {
	case JSON_STRING:
		return lhs->u.s.len == rhs->u.s.len &&
			memcmp(lhs->u.s.s, rhs->u.s.s, lhs->u.s.len) == 0;
	case JSON_NUMBER:
		return lhs->u.n == rhs->u.n;
	case JSON_ARRAY:
		if (lhs->u.a.size != rhs->u.a.size) return false;
		
		for (int i = 0; i < lhs->u.a.size;i++)
		{
			if (!is_equal(&lhs->u.a.e[i], &rhs->u.a.e[i]))
				return 0;
		}
		return 1;

	case JSON_OBJECT:
		if (lhs->u.o.size != rhs->u.o.size) return false;
		for (int i = 0; i < lhs->u.o.size; i++)
		{
			if (strcmp(lhs->u.o.m[i].k, rhs->u.o.m[i].k)!=0)
				return 0;
			if (!is_equal(&lhs->u.o.m[i].v, &rhs->u.o.m[i].v))
				return 0;
		}
		return 1;

	default:
		return 1;
	}
}









int YYjson::add_object(json_value* root, const char* key, json_value &v)
{
	json_member* old_m = root->u.o.m;
	int old_size=root->u.o.size;
	root->u.o.size++;
	json_member* new_m = (json_member*)malloc(root->u.o.size*sizeof(json_member));
	memcpy(new_m, old_m, old_size*sizeof(json_member));
	new_m[root->u.o.size-1].k = (char*)malloc(strlen(key) + 1);
	memcpy(new_m[root->u.o.size - 1].k, key, strlen(key));
	new_m[root->u.o.size - 1].k[strlen(key)] = '\0';
	new_m[root->u.o.size-1].klen = strlen(key)+1;
	new_m[root->u.o.size-1].v = v;
	root->u.o.m = new_m;
	
	free(old_m);
	return 0;
	

}


string YYjson::doubleToString(double price) {
	auto res =to_string(price);
	const string format("$1");
	try {
		regex r("(\\d*)\\.0{6}|");
		regex r2("(\\d*\\.{1}0*[^0]+)0*");
		res = regex_replace(res, r2, format);
		res = regex_replace(res, r, format);
	}
	catch (const exception& e) {
		return res;
	}
	return res;
}
