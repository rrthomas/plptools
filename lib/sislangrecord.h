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

#ifndef _SISLANGRECORD_H
#define _SISLANGRECORD_H

#include <sys/types.h>

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
	SisRC fillFrom(uchar* buf, int* base, off_t len);

	uint16 m_lang;
};

#endif

