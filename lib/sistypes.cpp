
#include "sistypes.h"

static unsigned int s_crcTable[256];

int logLevel = 0;

void createCRCTable()
{
	const unsigned int polynomial = 0x1021;
	unsigned int index;
	s_crcTable[0] = 0;
	for (index = 0; index < 128; index++)
		{
		unsigned int carry = s_crcTable[index] & 0x8000;
		unsigned int temp = (s_crcTable[index] << 1) & 0xffff;
		s_crcTable[index * 2 + (carry ? 0 : 1)] = temp ^ polynomial;
		s_crcTable[index * 2 + (carry ? 1 : 0)] = temp;
		}
}

uint16 updateCrc(uint16 crc, uchar value)
{
	return (crc << 8) ^ s_crcTable[((crc >> 8) ^ value) & 0xff];
}

uint16 calcCRC(uchar* data, int len)
{
	uint16 crc = 0;
	for (int i = 0; i < len; ++i)
		{
		uchar value = data[i];
		crc = (crc << 8) ^ s_crcTable[((crc >> 8) ^ value) & 0xff];
		}
	return crc;
}

uint16 read16(uchar* buf, int* ix)
{
	uchar* p = buf + *ix;
	*ix += 2;
	return p[0] | (p[1] << 8);
}

uint32 read32(uchar* buf, int* ix)
{
	uchar* p = buf + *ix;
	*ix += 4;
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

LangTableEntry langTable[] =
{
	{ 0, "", "Test" },
	{ 1, "EN", "UK English" },
	{ 2, "FR", "French" },
	{ 3, "GE", "German" },
	{ 4, "SP", "Spanish" },
	{ 5, "IT", "Italian" },
	{ 6, "SW", "Swedish" },
	{ 7, "DA", "Danish" },
	{ 8, "NO", "Norwegian" },
	{ 9, "FI", "Finnish" },
	{ 10, "AM", "American English" },
	{ 11, "SF", "Swiss French" },
	{ 12, "SG", "Swiss German" },
	{ 13, "PO", "Portuguese" },
	{ 14, "TU", "Turkish" },
	{ 15, "IC", "Icelandic" },
	{ 16, "RU", "Russian" },
	{ 17, "HU", "Hungarian" },
	{ 18, "DU", "Dutch" },
	{ 19, "BL", "Belgian Flemish" },
	{ 20, "AU", "Australian English" },
	{ 21, "BG", "Belgian French" },
	{ 22, "AS", "Austrian German" },
	{ 23, "NZ", "New Zealand" },
	{ 24, "IF", "International French" },
	{ 25, "CS", "Czech" },
	{ 26, "SK", "Slovak" },
	{ 27, "PL", "Polish" },
	{ 28, "SL", "Slovenian" },
	{ 29, "TC", "Taiwan Chinese" },
	{ 30, "HK", "Hong Kong" },
	{ 31, "ZH", "PRC Chinese" },
	{ 32, "JA", "Japanese" },
	{ 33, "TH", "Thai" },
};

