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

#ifndef _SISREQRECORD_H
#define _SISREQRECORD_H

#include "sistypes.h"

#include <sys/types.h>

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
	SisRC fillFrom(uchar* buf, int* base, off_t len, SISFile* file);

	uint32 m_uid;
	uint16 m_major;
	uint16 m_minor;
	uint32 m_variant;
	uint32* m_nameLengths;
	uint32* m_namePtrs;
};

#endif

