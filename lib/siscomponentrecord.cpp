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

SisRC
SISComponentNameRecord::fillFrom(uint8_t* buf, int base, off_t len,
								 SISFile* sisFile)
{
	int n = sisFile->m_header.m_nlangs;
	if (base + 8 + n * 4 * 2 > len)
		return SIS_TRUNCATED;

	uint8_t* p = buf + base;
	int size = 0;

	m_nameLengths = new uint32_t[n];
	m_namePtrs = new uint32_t[n];

	// First read lengths.
	//
	for (int i = 0; i < n; ++i)
		{
		m_nameLengths[i] = read32(p + size);
		if (m_nameLengths[i] > len)
			{
			printf("Length too large for name record %d.\n", i);
			return SIS_TRUNCATED;
			}
		size += 4;
		}

	// Then read ptrs.
	//
	m_names = new uint8_t*[n];
	for (int i = 0; i < n; ++i)
		{
		m_namePtrs[i] = read32(p + size);
		if (m_namePtrs[i] + m_nameLengths[i] > len)
			{
			printf("Position/length too large for name record %d.\n", i);
			return SIS_TRUNCATED;
			}
		size += 4;
		if (logLevel >= 2)
			printf("Name %d (for %s) is %.*s\n",
				   i,
				   sisFile->getLanguage(i)->m_name,
				   m_nameLengths[i],
				   buf + m_namePtrs[i]);
		int len = m_nameLengths[i];
		m_names[i] = new uint8_t[len + 1];
		memcpy(m_names[i], buf + m_namePtrs[i], len);
		m_names[i][len] = 0;
		}
	if (logLevel >= 1)
		printf("%d .. %d (%d bytes): Name records\n", base, base + size, size);
	return SIS_OK;
}

uint8_t*
SISComponentNameRecord::getName(int no)
{
	return m_names[no];
}

