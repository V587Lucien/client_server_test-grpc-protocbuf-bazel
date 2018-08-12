#ifndef _BASE64_H
#define _BASE64_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_MIME_LINE_LEN	76
#define MAX_ENCODEDWORD_LEN	75
#define min(a,b) (((a) < (b)) ? (a) : (b))

class CBase64
{
public:
	CBase64()
	{
		m_pbInput = NULL;
		m_nInputSize = 0;
		m_bAddLineBreak = true;
	}
	~CBase64(){}
	int Decode(const char *pInput, unsigned char* pbOutput, int nMaxSize);
	int Encode(const char *pInput, unsigned char* pbOutput, int nMaxSize);
	int Encode_turn(const char *pInput, unsigned char* pbOutput, int nMaxSize);
protected:
	int DecodeBase64Char(unsigned int nCode);
private:
	unsigned char *m_pbInput;
	int m_nInputSize;
	bool m_bAddLineBreak;	
};

#endif //_BASE64_H
