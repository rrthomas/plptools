#ifndef _SISLANGRECORD_H
#define _SISLANGRECORD_H

#include "sistypes.h"

/**
 * A simple language record, only containing the epoc specific 16 bit
 * language number.
 */
class SISLangRecord
{
public:

	/**
	 * Populate the fields.
	 */
	void fillFrom(uchar* buf, int* base);

	uint16 m_lang;
};

#endif

