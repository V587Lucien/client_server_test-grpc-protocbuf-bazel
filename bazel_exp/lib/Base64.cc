#include "Base64.h"



int CBase64::DecodeBase64Char(unsigned int nCode)
{
	if (nCode >= 'A' && nCode <= 'Z')
		return nCode - 'A';
	if (nCode >= 'a' && nCode <= 'z')
		return nCode - 'a' + 26;
	if (nCode >= '0' && nCode <= '9')
		return nCode - '0' + 52;
	if (nCode == '+')
		return 62;
	if (nCode == '/')
		return 63;
	return 64;
}
//½âÃÜ
int CBase64::Decode(const char *pInput,unsigned char* pbOutput, int nMaxSize)
{
	m_pbInput = (unsigned char *)pInput;
	m_nInputSize = strlen( pInput );
	
	const unsigned char* pbData = m_pbInput;
	const unsigned char* pbEnd = m_pbInput + m_nInputSize;
	unsigned char* pbOutStart = pbOutput;
	unsigned char* pbOutEnd = pbOutput + nMaxSize;

	int nFrom = 0;
	unsigned char chHighBits = 0;

	while (pbData < pbEnd)
	{
		if (pbOutput >= pbOutEnd)
			break;

		unsigned char ch = *pbData++;
		if (ch == '\r' || ch == '\n')
			continue;
		ch = (unsigned char) DecodeBase64Char(ch);
		if (ch >= 64)				// invalid encoding, or trailing pad '='
			break;

		switch ((nFrom++) % 4)
		{
		case 0:
			chHighBits = ch << 2;
			break;

		case 1:
			*pbOutput++ = chHighBits | (ch >> 4);
			chHighBits = ch << 4;
			break;

		case 2:
			*pbOutput++ = chHighBits | (ch >> 2);
			chHighBits = ch << 6;
			break;

		default:
			*pbOutput++ = chHighBits | ch;
		}
	}

	return (int)(pbOutput - pbOutStart);
}

//¼ÓÃÜ
int CBase64::Encode(const char *pInput,unsigned char* pbOutput, int nMaxSize)
{
	m_pbInput = (unsigned char *)pInput;
	m_nInputSize = strlen( pInput );
		
	static const char* s_Base64Table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	unsigned char* pbOutStart = pbOutput;
	unsigned char* pbOutEnd = pbOutput + nMaxSize;
	int nFrom, nLineLen = 0;
	unsigned char chHigh4bits = 0;

	for (nFrom=0; nFrom<m_nInputSize; nFrom++)
	{
		if (pbOutput >= pbOutEnd)
			break;

		unsigned char ch = m_pbInput[nFrom];
		switch (nFrom % 3)
		{
		case 0:
			*pbOutput++ = s_Base64Table[ch >> 2];
			chHigh4bits = (ch << 4) & 0x30;
			break;

		case 1:
			*pbOutput++ = s_Base64Table[chHigh4bits | (ch >> 4)];
			chHigh4bits = (ch << 2) & 0x3c;
			break;

		default:
			*pbOutput++ = s_Base64Table[chHigh4bits | (ch >> 6)];
			if (pbOutput < pbOutEnd)
			{
				*pbOutput++ = s_Base64Table[ch & 0x3f];
				nLineLen++;
			}
		}

		nLineLen++;
		if (m_bAddLineBreak && nLineLen >= MAX_MIME_LINE_LEN && pbOutput+2 <= pbOutEnd)
		{
			*pbOutput++ = '\r';
			*pbOutput++ = '\n';
			nLineLen = 0;
		}
	}

	if (nFrom % 3 != 0 && pbOutput < pbOutEnd)	// 76 = 19 * 4, so the padding wouldn't exceed 76
	{
		*pbOutput++ = s_Base64Table[chHigh4bits];
		int nPad = 4 - (nFrom % 3) - 1;
		if (pbOutput+nPad <= pbOutEnd)
		{
			::memset(pbOutput, '=', nPad);
			pbOutput += nPad;
		}
	}
	if (m_bAddLineBreak && nLineLen != 0 && pbOutput+2 <= pbOutEnd)	// add CRLF
	{
		*pbOutput++ = '\r';
		*pbOutput++ = '\n';
	}
	return (int)(pbOutput - pbOutStart);
}

int CBase64::Encode_turn(const char *pInput,unsigned char* pbOutput, int nMaxSize)
{
	m_pbInput = (unsigned char *)pInput;
	m_nInputSize = strlen( pInput );
		
	static const char* s_Base64Table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	unsigned char* pbOutStart = pbOutput;
	unsigned char* pbOutEnd = pbOutput + nMaxSize;
	int nFrom, nLineLen = 0;
	unsigned char chHigh4bits = 0;

	for (nFrom=0; nFrom<m_nInputSize; nFrom++)
	{
		if (pbOutput >= pbOutEnd)
			break;

		unsigned char ch = m_pbInput[nFrom];
		switch (nFrom % 3)
		{
		case 0:
			*pbOutput++ = s_Base64Table[ch >> 2];
			chHigh4bits = (ch << 4) & 0x30;
			break;

		case 1:
			*pbOutput++ = s_Base64Table[chHigh4bits | (ch >> 4)];
			chHigh4bits = (ch << 2) & 0x3c;
			break;

		default:
			*pbOutput++ = s_Base64Table[chHigh4bits | (ch >> 6)];
			if (pbOutput < pbOutEnd)
			{
				*pbOutput++ = s_Base64Table[ch & 0x3f];
				nLineLen++;
			}
		}

		nLineLen++;
		if (m_bAddLineBreak && nLineLen >= MAX_MIME_LINE_LEN && pbOutput+2 <= pbOutEnd)
		{
			*pbOutput++ = '\\';
			*pbOutput++ = 'n';
			nLineLen = 0;
		}
	}

	if (nFrom % 3 != 0 && pbOutput < pbOutEnd)	// 76 = 19 * 4, so the padding wouldn't exceed 76
	{
		*pbOutput++ = s_Base64Table[chHigh4bits];
		int nPad = 4 - (nFrom % 3) - 1;
		if (pbOutput+nPad <= pbOutEnd)
		{
			::memset(pbOutput, '=', nPad);
			pbOutput += nPad;
		}
	}
	if (m_bAddLineBreak && nLineLen >= MAX_MIME_LINE_LEN && pbOutput+2 <= pbOutEnd)	// add CRLF
	{
		*pbOutput++ = '\\';
		*pbOutput++ = 'n';
	}
	return (int)(pbOutput - pbOutStart);
}
