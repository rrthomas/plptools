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

#include "sisreqrecord.h"
#include "sisfile.h"

#include <stdio.h>

SisRC
SISReqRecord::fillFrom(uint8_t* buf, int* base, off_t len, SISFile* sisFile)
{
	int n = sisFile->m_header.m_nreqs;
	if (*base + 12 + n * 4 * 2 > len)
		return SIS_TRUNCATED;

	uint8_t* p = buf + *base;
	int size = 0;

	m_uid = read32(p);
	m_major = read16(p + 4);
	m_minor = read16(p + 6);
	m_variant = read32(p + 8);
	m_nameLengths = new uint32_t[n];
	m_namePtrs = new uint32_t[n];

	// First read lengths.
	//
	size = 12;
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
		if (m_namePtrs[i] + m_nameLengths[i] > len)
			{
			printf("Position/length too large for req record %d.\n", i);
			return SIS_CORRUPTED;
			}
		size += 4;
		if (logLevel >= 2)
			printf("Name %d (for %s) is %.*s\n",
				   i,
				   sisFile->getLanguage(i)->m_name,
				   m_nameLengths[i],
				   buf + m_namePtrs[i]);
		}
	if (logLevel >= 1)
		printf("%d .. %d (%d bytes): Req record\n",
			   *base, *base + size, size);
	*base += size;
	return SIS_OK;
}

