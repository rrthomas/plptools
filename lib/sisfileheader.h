/** -*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 2002 Daniel Brahneborg <basic.chello@se>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _SISFILEHEADER_H
#define _SISFILEHEADER_H

#include <sys/types.h>

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
	SisRC fillFrom(uchar* buf, int* base, off_t len);

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

