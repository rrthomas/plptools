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

#include "sisfile.h"
#include "sislangrecord.h"
#include "sisfilerecord.h"
#include "sisreqrecord.h"
#include "plpintl.h"

#include <stdio.h>

SisRC
SISFile::compareApp(SISFile* other)
{
	return m_header.compareApp(&other->m_header);
}

SisRC
SISFile::fillFrom(uint8_t* buf, off_t len)
{
	int ix = 0;
	SisRC rc = m_header.fillFrom(buf, &ix, len);
	if (rc != SIS_OK)
		{
		printf(_("Could not read header, rc = %d\n"), rc);
		return rc;
		}
	if (logLevel >= 2)
		printf(_("Ate header, got ix = %d\n"), ix);
	int n;

	// Read languages.
	//
	n = m_header.m_nlangs;
	m_langRecords = new SISLangRecord[n];
	ix = m_header.m_languagePtr;
	for (int i = 0; i < n; ++i)
		{
		if (ix >= len)
			return SIS_TRUNCATED;
		rc = m_langRecords[i].fillFrom(buf, &ix, len);
		if (rc != SIS_OK)
			{
			printf(_("Problem reading language record %d, rc = %d.\n"), i, rc);
			return rc;
			}
		}

	// Read requisites.
	//
	n = m_header.m_nreqs;
	m_reqRecords = new SISReqRecord[n];
	ix = m_header.m_reqPtr;
	for (int i = 0; i < n; ++i)
		{
		if (ix >= len)
			return SIS_TRUNCATED;
		rc = m_reqRecords[i].fillFrom(buf, &ix, len, this);
		if (rc != SIS_OK)
			{
			printf(_("Problem reading requisite record %d, rc = %d.\n"), i, rc);
			return rc;
			}
		}

	// Read component names, by language.
	//
	ix = m_header.m_componentPtr;
	rc = m_componentRecord.fillFrom(buf, ix, len, this);
	if (rc != SIS_OK)
		{
		printf(_("Problem reading the name record, rc = %d.\n"), rc);
		return rc;
		}

	// Read files.
	//
	n = m_header.m_nfiles;
	m_fileRecords = new SISFileRecord[n];
	ix = m_header.m_filesPtr;
	for (int i = 0; i < n; ++i)
		{
		if (ix >= len)
			return SIS_TRUNCATED;
		rc = m_fileRecords[i].fillFrom(buf, &ix, len, this);
		if (rc != SIS_OK)
			{
			printf(_("Problem reading file record %d, rc = %d.\n"), i, rc);
			return rc;
			}
		}

	return SIS_OK;
}

int
SISFile::getLanguage()
{
	return m_header.m_installationLanguage;
}

LangTableEntry*
SISFile::getLanguage(int i)
{
	return &langTable[m_langRecords[i].m_lang];
}

uint8_t*
SISFile::getName()
{
	return m_componentRecord.getName(m_header.m_installationLanguage);
}

void
SISFile::setDrive(char drive)
{
	m_header.setDrive(drive);
}

void
SISFile::setFiles(int nFiles)
{
	m_header.setFiles(nFiles);
}

void
SISFile::setLanguage(int lang)
{
	m_header.m_installationLanguage = lang;
}

