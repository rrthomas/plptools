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
	SisRC fillFrom(uint8_t* buf, int base, off_t len, SISFile* sisFile);

	/**
	 * Return the name for the given language.
	 * The number is the sequence number in the list of language records
	 * in the sis file.
	 */
	uint8_t* getName(int no);

private:

	uint32_t* m_nameLengths;
	uint32_t* m_namePtrs;

	/**
	 * The extracted names.
	 */
	uint8_t** m_names;

};

#endif
