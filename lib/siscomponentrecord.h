#ifndef _SISCOMPONENTRECORD_H
#define _SISCOMPONENTRECORD_H

#include "sistypes.h"

class SISFile;

/**
 * The name of the component in this SIS file.
 * A single instance holds the names for all languages.
 */
class SISComponentNameRecord
{
public:

	virtual ~SISComponentNameRecord();

	/**
	 * Populate the fields.
	 */
	void fillFrom(uchar* buf, int base, SISFile* sisFile);

	uchar* getName(int no);

private:

	uint32* m_nameLengths;
	uint32* m_namePtrs;

	/**
	 * The extracted names.
	 */
	uchar** m_names;

};

#endif

