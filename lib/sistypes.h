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

#ifndef _SISTYPES_H
#define _SISTYPES_H

typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned char uchar;

extern uint16 read16(uchar* p);

extern uint32 read32(uchar* p);

extern void write16(uchar* p, int val);

extern void createCRCTable();

extern uint16 updateCrc(uint16 crc, uchar value);

extern int logLevel;

/**
 * Holder of a language entry, translating from language numbers to
 * names.
 */
struct LangTableEntry
{
	uint16 m_no;
	char   m_code[3];
	char*  m_name;
};

extern LangTableEntry langTable[];

#endif

