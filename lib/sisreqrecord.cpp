
#include "sisreqrecord.h"
#include "sisfile.h"

#include <stdio.h>

void
SISReqRecord::fillFrom(uchar* buf, int* base, SISFile* sisFile)
{
	int ix = *base;

	m_uid = read32(buf, &ix);
	m_major = read16(buf, &ix);
	m_minor = read16(buf, &ix);
	m_variant = read32(buf, &ix);
	int n = sisFile->m_header.m_nreqs;
	m_nameLengths = new uint32[n];
	m_namePtrs = new uint32[n];

	// First read lengths.
	//
	for (int i = 0; i < n; ++i)
		{
		m_nameLengths[i] = read32(buf, &ix);
		}

	// Then read ptrs.
	//
	for (int i = 0; i < n; ++i)
		{
		m_namePtrs[i] = read32(buf, &ix);
		if (logLevel >= 2)
			printf("Name %d (for %s) is %.*s\n",
				   i,
				   sisFile->getLanguage(i)->m_name,
				   m_nameLengths[i],
				   buf + m_namePtrs[i]);
		}
	if (logLevel >= 1)
		printf("%d .. %d (%d bytes): Req record\n", *base, ix, ix - *base);
	*base = ix;
}

