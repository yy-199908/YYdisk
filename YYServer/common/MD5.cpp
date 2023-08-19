#include "MD5.h"
#include <iostream>
using namespace std;
vector<unsigned int>md5::padding(const char* plain,unsigned int len)
{
	// 以512位,64个字节为一组
	unsigned int num = ((len + 8) / 64) + 1;
	vector<unsigned int> rec(num * 16);
	for (unsigned int i = 0; i < len; i++) {
		// 一个unsigned int对应4个字节，保存4个字符信息
		rec[i >> 2] |= (int)(plain[i]) << ((i % 4) * 8);
	}
	// 补充1000...000
	rec[len >> 2] |= (0x80 << ((len % 4) * 8));
	// 填充原文长度
	rec[rec.size() - 2] = (len << 3);
	return rec;
}



void md5::iterateFunc(unsigned int* X, std::vector<unsigned int>& key, int size)
{
	unsigned int a = key[0],
		b = key[1],
		c = key[2],
		d = key[3],
		rec = 0,
		g, k;
	for (int i = 0; i < 64; i++) {
		if (i < 16) {
			// F迭代
			g = F(b, c, d);
			k = i;
		}
		else if (i < 32) {
			// G迭代
			g = G(b, c, d);
			k = (1 + 5 * i) % 16;
		}
		else if (i < 48) {
			// H迭代
			g = H(b, c, d);
			k = (5 + 3 * i) % 16;
		}
		else {
			// I迭代
			g = I(b, c, d);
			k = (7 * i) % 16;
		}
		rec = d;
		d = c;
		c = b;
		b = b + shift(a + g + X[k] + T[i], s[i]);
		a = rec;
	}
	key[0] += a;
	key[1] += b;
	key[2] += c;
	key[3] += d;

}

unsigned int  md5::trans_small(unsigned a)
{
	return ((a << 24) & 0xff000000) | ((a << 8) & 0x00ff0000) | ((a >> 8) & 0x0000ff00) | ((a >> 24) & 0x000000ff);
}

string md5::encode(const char* plain, unsigned int len)
{
	vector<unsigned int> rec = padding(plain, len);
	vector<unsigned int> out = { A,B,C,D };
	for (unsigned int i = 0; i <rec.size() / 16; i++)
	{
		unsigned int num[16];
		for (int j = 0; j < 16; j++) {
			num[j] = rec[i * 16 + j];
		}
		iterateFunc(num, out);
	}
	//unsigned int out[4] = { trans_small(key[0] + A), trans_small(key[1] + B), trans_small(key[2] + C),trans_small( key[3] + D) };
	return format(out[0])+ format(out[1])+ format(out[2])+ format(out[3]);
}





string md5::format(unsigned int num) {
	string res = "";
	unsigned int base = 1 << 8;
	for (int i = 0; i < 4; i++) {
		string tmp = "";
		unsigned int b = (num >> (i * 8)) % base & 0xff;
		for (int j = 0; j < 2; j++) {
			tmp = str16[b % 16] + tmp;
			b /= 16;
		}
		res += tmp;
	}
	return res;
}