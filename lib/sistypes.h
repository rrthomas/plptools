#ifndef _SISTYPES_H
#define _SISTYPES_H

typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned char uchar;

extern uint16 read16(uchar* p);

extern uint32 read32(uchar* p);

extern void write16(uchar* p, int val);

extern void createCRCTable();

extern uint16 updateCrc(uint16 crc, uchar value);

extern int logLevel;

/**
 * Holder of a language entry, translating from language numbers to
 * names.
 */
struct LangTableEntry
{
	uint16 m_no;
	char   m_code[3];
	char*  m_name;
};

extern LangTableEntry langTable[];

#endif

