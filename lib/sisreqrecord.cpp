
#include "sisreqrecord.h"
#include "sisfile.h"

#include <stdio.h>

void
SISReqRecord::fillFrom(uchar* buf, int* base, SISFile* sisFile)
{
	uchar* p = buf + *base;
	int size = 0;

	m_uid = read32(p);
	m_major = read16(p + 4);
	m_minor = read16(p + 6);
	m_variant = read32(p + 8);
	int n = sisFile->m_header.m_nreqs;
	m_nameLengths = new uint32[n];
	m_namePtrs = new uint32[n];

	// First read lengths.
	//
	size = 12;
	for (int i = 0; i < n; ++i)
		{
		m_nameLengths[i] = read32(p + size);
		size += 4;
		}

	// Then read ptrs.
	//
	for (int i = 0; i < n; ++i)
		{
		m_namePtrs[i] = read32(p + size);
		size += 4;
		if (logLevel >= 2)
			printf("Name %d (for %s) is %.*s\n",
				   i,
				   sisFile->getLanguage(i)->m_name,
				   m_nameLengths[i],
				   buf + m_namePtrs[i]);
		}
	if (logLevel >= 1)
		printf("%d .. %d (%d bytes): Req record\n",
			   *base, *base + size, size);
	*base += size;
}

