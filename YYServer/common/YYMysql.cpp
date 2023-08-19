#include "YYMysql.h"
#include <mysql/mysql.h>
#include <iostream>
#include<string.h>

using namespace std;
namespace YY {
	bool YYMysql::Init()
	{
		Close();
		mysql = mysql_init(0);
		if (!mysql)
		{
			cerr << "YYmysql init failed!" << endl;
			return false;
		}
		return true;

	}


	void YYMysql::Close()
	{
		FreeResult();
		if (mysql)
		{
			mysql_close(mysql);
			mysql = nullptr;
		}

	}


	bool YYMysql::Connect(const char* host, const char* user, const char* passwd, const char* db, unsigned short port, unsigned long flag)
	{

		if (!mysql and !Init()) {
			cerr << "connect failed! mysql is null." << endl;
			return false;
		}
		if (!mysql_real_connect(mysql, host, user, passwd, db, port, 0, flag))
		{
			cerr << mysql_error(mysql) << endl;
			return false;
		}
		cout << "connect seccuss!" << endl;
		return true;
	}



	bool YYMysql::Query(const char* sql, unsigned long sqllen)
	{
		if (!mysql) {
			cerr << "Quert failed! mysql is null." << endl;
			return false;
		};
		if (!sql or sqllen < 0)
		{
			cerr << "Quert failed! sql or sqllen<0 is null." << endl;
			return false;
		};

		if (mysql_real_query(mysql, sql, sqllen))
		{
			cerr << "Quert failed!. " << mysql_error(mysql) << endl;
			return false;
		}

		return true;

	}
	bool YYMysql::Query(const char* sql)
	{
		if (!sql)
		{
			cerr << "Quert failed! sql is null." << endl;
			return false;
		};
		unsigned long sqllen = (unsigned long)strlen(sql);
		if (sqllen <= 0)
		{
			cerr << "Quert failed! sql length <=0." << endl;
			return false;
		};
		if (mysql_real_query(mysql, sql, sqllen))
		{
			cerr << "Quert failed!. " << mysql_error(mysql) << endl;
			return false;
		}
		return true;

	}

	bool YYMysql::SetConnectTimeout(int sec)
	{
		if (!mysql)
		{
			cerr << "SetConnectTimeout failed! mysql is null." << endl;
			return false;
		};
		if (!mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, &sec))
			return false;
		return true;
	}

	bool YYMysql::SetReconnect(bool is)
	{
		if (!mysql)
		{
			cerr << "SetReconnect failed! mysql is null." << endl;
			return false;
		};
		if (!mysql_options(mysql, MYSQL_OPT_RECONNECT, &is))
			return false;
		return true;


	}


	bool YYMysql::StoreResult()
	{
		FreeResult();
		if (!mysql)
		{
			cerr << "StoreResult failed! mysql is null." << endl;
			return false;
		};
		res = mysql_store_result(mysql);
		if (!res)
		{
			cerr << "StoreResult failed!" << mysql_error(mysql) << endl;
			return false;
		};
		return true;

	}
	bool YYMysql::UseResult()
	{
		FreeResult();
		if (!mysql)
		{
			cerr << "UseResult failed! mysql is null." << endl;
			return false;
		};
		res = mysql_use_result(mysql);
		if (!res)
		{
			cerr << "StoreResult failed!" << mysql_error(mysql) << endl;
			return false;
		};
		return true;
	}


	void YYMysql::FreeResult()
	{
		if (res)
		{
			mysql_free_result(res);
			res = NULL;
		}
	}



	std::vector<YYDATA> YYMysql::FetchRow()
	{
		std::vector<YYDATA> re;
		if (!res) return re;
		MYSQL_ROW row = mysql_fetch_row(res);
		if (!row) return re;
		//列数
		int column_num = mysql_num_fields(res);
		unsigned long* lens = mysql_fetch_lengths(res);
		for (int i = 0; i < column_num; i++)
		{
			YYDATA rr;
			rr.data = row[i];
			rr.size = lens[i];
			auto field = mysql_fetch_field_direct(res, i);
			rr.type = (MYSQL_TYPES)field->type;
			re.push_back(rr);
		}

		return re;
	}



	std::string YYMysql::GetInsetSql(YDATA& kv, std::string& table)
	{
		string sql;
		if (kv.empty() or table.empty()) return sql;
		//insert into t_test(name,size) values('name1','1024');
		sql = "insert into `";
		sql += table;
		sql += "`(";
		string keys, values;
		for (auto it = kv.begin(); it != kv.end(); it++)
		{

			if (it->first[0] == '@')
			{
				keys += "`";
				keys += it->first.substr(1, it->first.length() - 1);
				keys += "`,";


				values += it->second.data;
				values += ",";
			}
			else
			{
				keys += "`";
				keys += it->first;
				keys += "`,";

				values += "'";
				values += it->second.data;
				values += "',";
			}





		}
		//去除结尾多余的，
		keys[keys.size() - 1] = ' ';
		values[values.size() - 1] = ' ';
		sql += keys;
		sql += ") values(";
		sql += values;
		sql += ")";
		return sql;
	}



	bool YYMysql::Insert(YDATA& kv, std::string table)
	{
		if (!mysql)
		{
			cerr << "insert failed! mysql is null." << endl;
			return false;
		};

		string sql = GetInsetSql(kv, table);
		
		if (!Query(sql.c_str())) return false;
		if (mysql_affected_rows(mysql) <= 0) return false;
		return true;
	}




	bool YYMysql::InsertBin(YDATA& kv, std::string table)
	{
		string sql;
		if (kv.empty() or table.empty() or !mysql) return false;
		//insert into t_test(name,data) values(?,?);
		sql = "insert into `";
		sql += table;
		sql += "`(";
		string keys, values;

		MYSQL_BIND bind[3] = { 0 };
		int i = 0;
		for (auto it = kv.begin(); it != kv.end(); it++)
		{

			keys += "`";
			keys += it->first;
			keys += "`,";
			values += "?,";
			bind[i].buffer = (char*)it->second.data;
			bind[i].buffer_length = it->second.size;
			bind[i].buffer_type = (enum_field_types)it->second.type;
			i++;
		}


		//去除结尾多余的，
		keys[keys.size() - 1] = ' ';
		values[values.size() - 1] = ' ';
		sql += keys;
		sql += ") values(";
		sql += values;
		sql += ")";

		//预处理sql语句
		MYSQL_STMT* stmt = mysql_stmt_init(mysql);
		if (!stmt)
		{
			cerr << "mysql_stmt_init failed!" << mysql_error(mysql) << endl;
			return false;
		}
		if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()))
		{
			mysql_stmt_close(stmt);
			cerr << "mysql_stmt_prepare failed!" << mysql_stmt_error(stmt) << endl;
			return false;
		}

		if (mysql_stmt_bind_param(stmt, bind))
		{
			mysql_stmt_close(stmt);
			cerr << "mysql_stmt_bind_param failed!" << mysql_stmt_error(stmt) << endl;
			return false;
		}
		if (mysql_stmt_execute(stmt))
		{
			mysql_stmt_close(stmt);
			cerr << "mysql_stmt_execute failed!" << mysql_stmt_error(stmt) << endl;
			return false;
		}

		mysql_stmt_close(stmt);
		return true;
	}



	std::string YYMysql::GeyUpdateSql(YDATA& kv, std::string& table, std::string& where)
	{
		string sql;
		if (kv.empty() || table.empty())
			return sql;
		//update `t_test` set name='update001',data='xxx' where id=10;
		sql = "update `";
		sql += table;
		sql += "` set ";
		for (auto it = kv.begin(); it != kv.end(); it++)
		{
			sql += "`";
			sql += it->first;
			sql += "`='";
			sql += it->second.data;
			sql += "',";
		}
		sql[sql.size() - 1] = ' ';
		sql += where;
		return sql;
	}

	int YYMysql::Update(YDATA& kv, std::string table, std::string where)
	{
		if (!mysql) return -1;
		string sql = GeyUpdateSql(kv, table, where);
		if (sql.empty()) return -1;
		if (!Query(sql.c_str())) return -1;
		return  mysql_affected_rows(mysql);

	}


	int YYMysql::UpdateBin(YDATA& kv, std::string table, std::string where)
	{
		if (!mysql or kv.empty() or table.empty())  return -1;
		string sql = "";
		sql = "update `";
		sql += table;
		sql += "` set ";
		MYSQL_BIND bind[256] = { 0 };
		int i = 0;
		for (auto it = kv.begin(); it != kv.end(); it++)
		{
			sql += "`";
			sql += it->first;
			sql += "`=";
			sql += "?,";
			bind[i].buffer = (char*)it->second.data;
			bind[i].buffer_length = it->second.size;
			bind[i].buffer_type = (enum_field_types)it->second.type;
			i++;
		}
		sql[sql.size() - 1] = ' ';
		sql += where;

		MYSQL_STMT* stmt = mysql_stmt_init(mysql);
		if (!stmt)
		{
			cerr << "mysql_stmt_init failed!" << mysql_error(mysql) << endl;
			return -1;
		}
		if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()))
		{
			mysql_stmt_close(stmt);
			cerr << "mysql_stmt_prepare failed!" << mysql_stmt_error(stmt) << endl;
			return -1;
		}

		if (mysql_stmt_bind_param(stmt, bind))
		{
			mysql_stmt_close(stmt);
			cerr << "mysql_stmt_bind_param failed!" << mysql_stmt_error(stmt) << endl;
			return -1;
		}
		if (mysql_stmt_execute(stmt))
		{
			mysql_stmt_close(stmt);
			cerr << "mysql_stmt_execute failed!" << mysql_stmt_error(stmt) << endl;
			return -1;
		}

		mysql_stmt_close(stmt);
		return mysql_affected_rows(mysql);



	}

	bool YYMysql::StartTransaction()
	{
		return Query("set autocommit=0");
	}

	//结束
	bool YYMysql::StopTransaction()
	{
		return Query("set autocommit=1");
	}
	//commit
	bool YYMysql::Commit()
	{

		return Query("commit");

	}
	//回滚
	bool YYMysql::Rollback()
	{
		return Query("rollback");
	}


#ifndef _WIN32
	 size_t YYMysql::Convert(char* from_cha, char* to_cha, char* in, size_t inlen, char* out, size_t outlen)
	{
		//转换上下文
		iconv_t cd;
		cd = iconv_open(to_cha, from_cha);
		if (cd == 0) return -1;
		memset(out, 0, outlen);
		char** pin = &in;
		char** pout = &out;
		size_t re = iconv(cd, pin, &inlen, pout, &outlen);

		if (cd != 0)
			iconv_close(cd);
		return re;
	}
#endif


	string YYMysql::UTF8ToGBK(const char* data)
	{
		string gbk = "";
		//window版本
#ifdef _WIN32
	//UTF8转为unicode ：win utf16

	//统计转换后字节数
		int len = MultiByteToWideChar(CP_UTF8,//转换格式
			0,//默认转换方式
			data,//输入的字节
			-1,//输入字符串大小，-1找、0结束
			0,  //输出
			0   //输出的空间大小
		);
		if (len <= 0) return gbk;
		wstring udata;
		udata.resize(len);
		MultiByteToWideChar(CP_UTF8, 0, data, -1, (wchar_t*)udata.data(), len);

		//unicode转换为GBK
		len = WideCharToMultiByte(CP_ACP, 0, (wchar_t*)udata.data(), -1, 0, 0,
			0,//失败默认替换字符
			0//是否使用默认替代
		);
		if (len <= 0) return gbk;
		gbk.resize(len);
		WideCharToMultiByte(CP_ACP, 0, (wchar_t*)udata.data(), -1, (char*)gbk.data(), len, 0, 0);

#else
  //Linux 代码
		gbk.resize(1024);//最大支持1024
		int inlen = strlen(data);
		Convert((char*)"utf-8", (char*)"gbk", (char*)data, inlen, (char*)gbk.data(), gbk.size());
		int outlen = gbk.size();
		gbk.resize(outlen);
#endif
		return gbk;
	}

	string YYMysql::GBKToUTF8(char const* data)
	{
		string utf8;

#ifdef _WIN32
		////GBK转为unicode ：win utf16

		//统计转换后字节数
		int len = MultiByteToWideChar(CP_ACP,//转换格式
			0,//默认转换方式
			data,//输入的字节
			-1,//输入字符串大小，-1找、0结束
			0,  //输出
			0   //输出的空间大小
		);
		if (len <= 0) return utf8;
		wstring udata;
		udata.resize(len);
		MultiByteToWideChar(CP_ACP, 0, data, -1, (wchar_t*)udata.data(), len);


		//unicode转换为UTF8
		len = WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)udata.data(), -1, 0, 0,
			0,//失败默认替换字符
			0//是否使用默认替代
		);
		if (len <= 0) return utf8;
		utf8.resize(len);
		WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)udata.data(), -1, (char*)utf8.data(), len, 0, 0);
#else
		//Linux 代码
		utf8.resize(1024);//最大支持1024
		int inlen = strlen(data);
		Convert((char*)"gbk", (char*)"utf-8", (char*)data, inlen, (char*)utf8.data(), utf8.size());
		int outlen = utf8.size();
		utf8.resize(outlen);
#endif
		return utf8;
	}

	YYROWS YYMysql::GetResult(const char* sql)
	{
		FreeResult();
		YYROWS rows;
		if (!Query(sql))
			return rows;
		if (!StoreResult())
			return rows;
		for (;;)
		{
			auto row = FetchRow();
			if (row.empty())
				break;
			rows.push_back(row);

		}
		return rows;
	}

}