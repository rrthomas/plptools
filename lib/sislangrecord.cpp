
#include "sislangrecord.h"

#include <stdio.h>

void
SISLangRecord::fillFrom(uchar* buf, int* base)
{
	m_lang = read16(buf + *base);
	if (logLevel >= 2)
		printf("Got language %d (%s)\n", m_lang, langTable[m_lang].m_name);
	if (logLevel >= 1)
		printf("%d .. %d (%d bytes): Language record for %s\n",
			   *base, *base + 2, 2, langTable[m_lang].m_name);
	*base += 2;
}

