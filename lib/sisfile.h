#ifndef _SISFILE_H
#define _SISFILE_H

#include "sistypes.h"
#include "sisfileheader.h"
#include "siscomponentrecord.h"

class SISLangRecord;
class SISFileRecord;
class SISReqRecord;

/**
 * The top level container of a SIS file.
 * Based on documentation by Alexander Thoukydides <alex@thouky.co.uk>.
 *
 * @author Daniel Brahneborg, 2002
 */
class SISFile
{
public:
	/**
	 * Populate the fields.
	 */
	void fillFrom(uchar* buf);

	int getLanguage();

	/**
	 * Find a language entry, based on the sequence number in the SISLangRecord
	 * part of the file.
	 */
	LangTableEntry* getLanguage(int i);

	/**
	 * Get the name of this component, in the selected language.
	 */
	uchar* getName();

	/**
	 * Set the installed drive.
	 */
	void setDrive(char drive);

	/**
	 * Set the number of installed files.
	 */
	void setFiles(int nFiles);

	/**
	 * Set the selected installation language.
	 */
	void setLanguage(int lang);

	SISFileHeader m_header;
	SISLangRecord* m_langRecords;
	SISFileRecord* m_fileRecords;
	SISReqRecord* m_reqRecords;

private:

	SISComponentNameRecord m_componentRecord;

};

#endif

