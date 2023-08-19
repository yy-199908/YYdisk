#include "YYDATA.h"
#include <fstream>
#include <iostream>
#include <string.h>
using namespace std;
namespace YY
{
	YYDATA::YYDATA(const char* data)
	{
		if (!data) return;
		this->data = data;
		this->size = strlen(data);
		this->type = YY_TYPE_STRING;
	}

	YYDATA::YYDATA(string &data)
	{
		if (data.size()==0) return;
		this->data = data.c_str();
		this->size = data.size();
		this->type = YY_TYPE_STRING;
	}

	YYDATA::YYDATA(const int *d)
	{
		this->data = (const char *)d;
		this->size = sizeof(int);
		this->type = YY_TYPE_LONG;
	}


	bool YYDATA::LoadFile(const char* filename)
	{
		if (!filename) return false;
		fstream in(filename, ios::in | ios::binary);
		if (!in.is_open())
		{
			cerr << "LoadFile " << filename << " failed!" << endl;
		}
		//获取文件大小
		in.seekg(0, ios::end);
		size = in.tellg();
		in.seekg(0, ios::beg);
		if(size <= 0)
		{
			return false;
		}
		char* filedata = new char[size];
		int readed = 0;
		while (!in.eof())
		{
			in.read(filedata + readed, size - readed);
			if (in.gcount() <= 0) break;
			readed += in.gcount();
		}
		data = (const char*) filedata;
		this->type = YY_TYPE_LONG_BLOB;
		in.close();
		return true;

	}

	void YYDATA::Drop()
	{
		delete data;
		data = NULL;
	}


	bool YYDATA::SaveFile(const char* filename)
	{
		if (!data or size <= 0) return false;
 		fstream out(filename, ios::out | ios::binary);
		if (!out.is_open())
		{
			cerr << "SaveFile open failed!" << endl;
			return false;
		}
		out.write(data, size);
		out.close();
		return true;
	}
}
