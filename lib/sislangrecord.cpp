
#include "sislangrecord.h"

#include <stdio.h>

void
SISLangRecord::fillFrom(uchar* buf, int* base)
{
	int ix = *base;
	m_lang = read16(buf, &ix);
	if (logLevel >= 2)
		printf("Got language %d (%s)\n", m_lang, langTable[m_lang].m_name);
	if (logLevel >= 1)
		printf("%d .. %d (%d bytes): Language record for %s\n",
			   *base, ix, ix - *base, langTable[m_lang].m_name);
	*base = ix;
}

