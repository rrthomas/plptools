#ifndef _SISREQRECORD_H
#define _SISREQRECORD_H

#include "sistypes.h"

class SISFile;

/**
 * Information about an application that must be installed prior to the
 * current one.
 */
class SISReqRecord
{
public:

	/**
	 * Populate the fields.
	 */
	void fillFrom(uchar* buf, int* base, SISFile* file);

	uint32 m_uid;
	uint16 m_major;
	uint16 m_minor;
	uint32 m_variant;
	uint32* m_nameLengths;
	uint32* m_namePtrs;
};

#endif

