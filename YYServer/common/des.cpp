#include "des.h"
#include <string.h>
#include <iostream>
using namespace std;



bitset<64> des::charToBitset(const char s[8])
{
	bitset<64> bits;
	for (int i = 0; i < 8; ++i)
		for (int j = 0; j < 8; ++j)
			bits[8 * i + j] = ((s[i] >> j) & 1);
	return bits;
}





bitset<32> des::f(bitset<32> R, bitset<48> k)
{
	bitset<48> expandR;
	//1.E盒扩展 32->48
	for (int i = 0; i < 48; ++i)
		expandR[47 - i] = R[32 - E[i]];

	//2.xor
	expandR ^= k;
	//3.S盒置换 48->32
	bitset<32> output;
	int x = 0;
	for (int i = 0; i < 48; i += 6)
	{
		int row = expandR[47 - i] * 2 + expandR[47 - i - 5];
		int col = expandR[47 - i - 1] * 8 + expandR[47 - i - 2] * 4 + expandR[47 - i - 3] * 2 + expandR[47 - i - 4];
		int num = S_BOX[i / 6][row][col];
		bitset<4> binary(num);
		output[31 - x] = binary[3];
		output[31 - x - 1] = binary[2];
		output[31 - x - 2] = binary[1];
		output[31 - x - 3] = binary[0];
		x += 4;
	}

	//4.P置换 32->32
	bitset<32> tmp = output;
	for (int i = 0; i < 32; ++i)
		output[31 - i] = tmp[32 - P[i]];
	return output;

}




bitset<28> des::leftShift(bitset<28> k, int shift)
{
	bitset<28> tmp = k;
	for (int i = 27; i >= 0; --i)
	{
		if (i - shift < 0)
			k[i] = tmp[i - shift + 28];
		else
			k[i] = tmp[i - shift];
	}
	return k;
}


void des::generateKeys()
{
	bitset<56> realKey;
	bitset<28> left;
	bitset<28> right;
	bitset<48> compressKey;

	//1.64->56 去掉奇偶校验位
	for (int i = 0; i < 56; ++i)
		realKey[55 - i] = key[64 - PC_1[i]];

	//2.生成子密钥
	for (int round = 0; round < 16; round++)
	{
		for (int i = 0; i < 28; i++)
		{
			right[i] = realKey[i];
			left[i] = realKey[i + 28];
		}
		//左移
		left = leftShift(left, shiftBits[round]);
		right = leftShift(right, shiftBits[round]);

		//3.压缩置换
		for (int i = 0; i < 28; i++)
		{
			realKey[i] = right[i];
			realKey[i+28] = left[i];
		}
		for(int i=0;i<48;i++)
			compressKey[47 - i] = realKey[56 - PC_2[i]];
		subKey[round] = compressKey;

	}


}



int  des::encoder(const char* p,char *code)
{

	bitset<64> cipher;
	bitset<64> currentBits;
	bitset<32> left;
	bitset<32> right;
	bitset<32> newLeft;
	bitset<64> plain = charToBitset(p);
	//1. IP置换
	for (int i = 0; i < 64; ++i)
		currentBits[63 - i] = plain[64 - IP[i]];
	//2. 分为左右两部分
	for (int i = 0; i < 32; ++i)
	{
		right[i] = currentBits[i];
		left[i] = currentBits[i+32];
	}


	//3.16轮迭代
	for (int round = 0; round < 16; ++round)
	{
		newLeft = right;
		right = left ^ f(right, subKey[round]);
		left = newLeft;
	}

	//4.合并L与R，合并为R16L16
	for (int i = 0; i < 32; ++i)
	{
		cipher[i] = left[i];
		cipher[i+32] = right[i];
	}
	//4.ip反置换
	currentBits = cipher;
	for (int i = 0; i < 64; ++i)
		cipher[63 - i] = currentBits[64 - IP_1[i]];
	// 返回密文
	memcpy(code, &cipher, 8);
	
	return 0;
}
//解密
int des::decoder(const char* code,char *decode)
{
	bitset<64> plain;
	bitset<64> currentBits;
	bitset<32> left;
	bitset<32> right;
	bitset<32> newLeft;
	bitset<64> cipher = charToBitset(code);
	// 第一步：初始置换IP
	for (int i = 0; i < 64; ++i)
		currentBits[63 - i] = cipher[64 - IP[i]];
	// 第二步：获取 Li 和 Ri
	for (int i = 32; i < 64; ++i)
		left[i - 32] = currentBits[i];
	for (int i = 0; i < 32; ++i)
		right[i] = currentBits[i];
	// 第三步：共16轮迭代（子密钥逆序应用）
	for (int round = 0; round < 16; ++round)
	{
		newLeft = right;
		right = left ^ f(right, subKey[15 - round]);
		left = newLeft;
	}
	// 第四步：合并L16和R16，注意合并为 R16L16
	for (int i = 0; i < 32; ++i)
	{
		plain[i] = left[i];
		plain[i+32] = right[i];
	}
		
	// 第五步：结尾置换IP-1
	currentBits = plain;
	for (int i = 0; i < 64; ++i)
		plain[63 - i] = currentBits[64 - IP_1[i]];
	// 返回明文
	memcpy(decode, &plain, 8);
	return 0;

}



int des::des_encode(const char* p, int plen, char** code)
{
	
	int len = plen / 8;
	int s = plen % 8;
	int real_len;
	if (s)
	{
		*code = (char*)malloc((len + 1) * 8);
		real_len = (len + 1) * 8;
	}
	else
	{
		*code = (char*)malloc(plen);
		real_len = plen;
	}
	char* tmp = *code;
	char a = (char)(8 - s);
	char str[8];
	for (int i = 0; i <= len; i++)
	{
		if (i == len)
		{
			memset(str,a,8);
			memcpy(str,&p[i*8],s);
		}
		else
		{
			memcpy(str, &p[i * 8], 8);
		}
		 encoder(str,tmp);
		 tmp += 8;
		
	}
	return real_len;
	



}
int des::des_decode(const char* code, int clen, char**out)
{
	
	char *temp = (char*)malloc(clen);

	char* tmp = temp;

	for (int i = 0; i < clen; i+=8)
	{
		decoder(&code[i], &temp[i]);
		tmp += 8;
	}
	char add = (temp[clen-1]);
	if (add <= 7 and add >= 1)
	{
		int ad = (int)(add);
		int sub_len = ad;
		for (int i = 0; i < ad; i++)
		{
			if (temp[clen - ad] != add) sub_len = 0;
		}
		*out = (char*)malloc(clen - sub_len+1);
		(*out)[clen - sub_len] = '\0';
		memcpy(*out, temp, clen - sub_len);

		free(temp);
		return clen - sub_len;
	}
	else
	{
		*out = (char*)malloc(clen+1 );
		(*out)[clen] = '\0';
		memcpy(*out, temp, clen );
		return clen;

	}
	
	
	

}