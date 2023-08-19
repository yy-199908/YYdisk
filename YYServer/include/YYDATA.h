#pragma once
#ifndef YYDATA_H_
#define YYDATA_H_

#include <map>
#include <string>
#include <vector>






namespace YY {

	enum MYSQL_TYPES

	{
		YY_TYPE_DECIMAL,
		YY_TYPE_TINY,
		YY_TYPE_SHORT,
		YY_TYPE_LONG,
		YY_TYPE_FLOAT,
		YY_TYPE_DOUBLE,
		YY_TYPE_NULL,
		YY_TYPE_TIMESTAMP,
		YY_TYPE_LONGLONG,
		YY_TYPE_INT24,
		YY_TYPE_DATE,
		YY_TYPE_TIME,
		YY_TYPE_DATETIME,
		YY_TYPE_YEAR,
		YY_TYPE_NEWDATE, /**< Internal to MySQL. Not used in protocol */
		YY_TYPE_VARCHAR,
		YY_TYPE_BIT,
		YY_TYPE_TIMESTAMP2,
		YY_TYPE_DATETIME2,   /**< Internal to MySQL. Not used in protocol */
		YY_TYPE_TIME2,       /**< Internal to MySQL. Not used in protocol */
		YY_TYPE_TYPED_ARRAY, /**< Used for replication only */
		YY_TYPE_JSON = 245,
		YY_TYPE_NEWDECIMAL = 246,
		YY_TYPE_ENUM = 247,
		YY_TYPE_SET = 248,
		YY_TYPE_TINY_BLOB = 249,
		YY_TYPE_MEDIUM_BLOB = 250,
		YY_TYPE_LONG_BLOB = 251,
		YY_TYPE_BLOB = 252,
		YY_TYPE_VAR_STRING = 253,
		YY_TYPE_STRING = 254,
		YY_TYPE_GEOMETRY = 255
	};


	struct YYDATA
	{
		YYDATA(const char* data=0);
		YYDATA(const int *d);
		YYDATA(std::string &data);
		//读取文件内容写入data中。size存储大小,会在堆中申请data空间，需要Drop释放
		bool LoadFile(const char* filename);
		bool SaveFile(const char* filename);
		const char* data = nullptr;
		int size = 0;
		MYSQL_TYPES type;
		void Drop();//释放loadfile申请的空间
	};

	//插入和更新数据的数据结构
	typedef std::map<std::string, YYDATA> YDATA;
	//数据列表
	typedef std::vector<std::vector<YYDATA>> YYROWS;

}

#endif


