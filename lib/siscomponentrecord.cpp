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

#include "siscomponentrecord.h"
#include "sisfile.h"

#include <stdio.h>
#include <string.h>

SISComponentNameRecord::~SISComponentNameRecord()
{
	delete[] m_names;
}

void
SISComponentNameRecord::fillFrom(uchar* buf, int base, SISFile* sisFile)
{
	uchar* p = buf + base;
	int size = 0;

	int n = sisFile->m_header.m_nlangs;
	m_nameLengths = new uint32[n];
	m_namePtrs = new uint32[n];
	m_names = new uchar*[n];

	// First read lengths.
	//
	for (int i = 0; i < n; ++i)
		{
		m_nameLengths[i] = read32(p + size);
		size += 4;
		}

	// Then read ptrs.
	//
	for (int i = 0; i < n; ++i)
		{
		m_namePtrs[i] = read32(p + size);
		size += 4;
		if (logLevel >= 2)
			printf("Name %d (for %s) is %.*s\n",
				   i,
				   sisFile->getLanguage(i)->m_name,
				   m_nameLengths[i],
				   buf + m_namePtrs[i]);
		int len = m_nameLengths[i];
		m_names[i] = new uchar[len + 1];
		memcpy(m_names[i], buf + m_namePtrs[i], len);
		m_names[i][len] = 0;
		}
	if (logLevel >= 1)
		printf("%d .. %d (%d bytes): Name records\n", base, base + size, size);
}

uchar*
SISComponentNameRecord::getName(int no)
{
	return m_names[no];
}

