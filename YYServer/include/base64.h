#ifndef BASE64_H
#define BASE64_H
#include <string.h>

class base64
{
public:
	char* base64_encode(const unsigned char* bindata, int binlength, char* base64);
	int base64_decode(const char* base64, unsigned char* bindata);

private:
	const char* base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
};


#endif

