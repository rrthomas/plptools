/* -*-c++-*-
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
	SisRC fillFrom(uint8_t* buf, int* base, off_t len, SISFile* sisFile);

	/**
	 * 1 if multiple language versions, otherwise 0.
	 */
	uint32_t m_flags;

	/**
	 * Type of file.
	 *
	 *  - 0. Standard file.
	 *  - 1. Text file to display during installation.
	 *  - 2. SIS component.
	 *  - 3. File to run during installation/removal.
	 *  - 4. Does not exist yet, but will be created when app is run, so
	 *       it should not be removed during an upgrade.
	 */
	uint32_t m_fileType;

	/**
	 * If file type is 1:
	 *
	 *  - 0. Continue.
	 *  - 1. Yes, No (skip next file).
	 *  - 2. Yes, No (abort installation).
	 *
	 * If file type is 3:
	 *
	 *  - 0. Run during installation.
	 *  - 1. Run during removal.
	 *  - 2. Run during both installation and removal.
	 */
	uint32_t m_fileDetails;

	uint32_t m_sourceLength;
	uint32_t m_sourcePtr;
	uint32_t m_destLength;
	uint32_t m_destPtr;
	uint32_t* m_fileLengths;
	uint32_t* m_filePtrs;
};

#endif

