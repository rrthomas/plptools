#ifndef _SISFILERECORD_H
#define _SISFILERECORD_H

#include "sistypes.h"

class SISFile;

/**
 * Information about a file component in a SIS file.
 *
 * The file can be for multiple languages, in which case a single
 * instance holds pointers to contents for all languages.
 */
class SISFileRecord
{
public:

	/**
	 * Populate the fields.
	 */
	void fillFrom(uchar* buf, int* base, SISFile* sisFile);

	/**
	 * 1 if multiple lang versions, otherwise 0.
	 */
	uint32 m_flags;

	/**
	 * Type of file.
	 *
	 *  - 0: Standard file.
	 *  - 1: Text file to display during installation.
	 *  - 2: SIS component.
	 *  - 3: File to run during installation/removal.
	 *  - 4: Does not exist yet, but will be created when app is run, so
	 *       it should not be removed during an upgrade.
	 */
	uint32 m_fileType;

	/**
	 * If file type is 1:
	 *
	 *  - 0: Continue.
	 *  - 1: Yes, No (skip next file).
	 *  - 2: Yes, No (abort installation).
	 *
	 * If file type is 3:
	 *
	 *  - 0: Run during installation.
	 *  - 1: Run during removal.
	 *  - 2: Run during both installation and removal.
	 */
	uint32 m_fileDetails;

	uint32 m_sourceLength;
	uint32 m_sourcePtr;
	uint32 m_destLength;
	uint32 m_destPtr;
	uint32* m_fileLengths;
	uint32* m_filePtrs;
};

#endif

