
#include "siscomponentrecord.h"
#include "sisfile.h"

#include <stdio.h>
#include <string.h>

SISComponentNameRecord::~SISComponentNameRecord()
{
	delete[] m_names;
}

void
SISComponentNameRecord::fillFrom(uchar* buf, int base, SISFile* sisFile)
{
	int ix = base;

	int n = sisFile->m_header.m_nlangs;
	m_nameLengths = new uint32[n];
	m_namePtrs = new uint32[n];
	m_names = new uchar*[n];

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
		int len = m_nameLengths[i];
		m_names[i] = new uchar[len + 1];
		memcpy(m_names[i], buf + m_namePtrs[i], len);
		m_names[i][len] = 0;
		}
	if (logLevel >= 1)
		printf("%d .. %d (%d bytes): Name records\n", base, ix, ix - base);
}

uchar*
SISComponentNameRecord::getName(int no)
{
	return m_names[no];
}

