#ifndef _SISFILEHEADER_H
#define _SISFILEHEADER_H

#include "sistypes.h"

/**
 * The first part of a SIS file.
 */
class SISFileHeader
{
public:

	/**
	 * Populate the fields.
	 */
	void fillFrom(uchar* buf, int* base);

	/**
	 * Update the drive letter, and patch the parsed buffer.
	 */
	void setDrive(char drive);

	/**
	 * Update the number of installed files, and patch the parsed buffer.
	 */
	void setFiles(int nFiles);

	enum FileOptions {
		op_isUnicode = 1,
		op_isDistributable = 2,
#ifdef EPOC6
		op_noCompress = 8,
		op_shutdownApps = 16,
#endif
	};

	enum FileType {
		FT_App = 0,
#ifdef EPOC6
		FT_System = 1,
		FT_Option = 2,
		FT_Config = 3,
		FT_Patch = 4,
		FT_Upgrade = 5,
#endif
	};

	uint32 m_uid1;
	uint32 m_uid2;
	uint32 m_uid3;
	uint32 m_uid4;
	uint16 m_crc;
	uint16 m_nlangs;
	uint16 m_nfiles;
	uint16 m_nreqs;
	uint16 m_installationLanguage;
	uint16 m_installationFiles;
	uint32 m_installationDrive;
	uint32 m_installerVersion;
	uint16 m_options;
	uint16 m_type;
	uint16 m_major;
	uint16 m_minor;
	uint32 m_variant;
	uint32 m_languagePtr;
	uint32 m_filesPtr;
	uint32 m_reqPtr;
	uint32 m_unknown;
	uint32 m_componentPtr;

private:

	uchar* m_buf;

};

#endif

