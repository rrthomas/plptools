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

#include <stdio.h>

void
SISFile::fillFrom(uchar* buf)
{
	int ix = 0;
	m_header.fillFrom(buf, &ix);
	if (logLevel >= 2)
		printf("Ate header, got ix = %d\n", ix);
	int n;

	// Read languages.
	//
	n = m_header.m_nlangs;
	m_langRecords = new SISLangRecord[n];
	ix = m_header.m_languagePtr;
	for (int i = 0; i < n; ++i)
		m_langRecords[i].fillFrom(buf, &ix);

	// Read requisites.
	//
	n = m_header.m_nreqs;
	m_reqRecords = new SISReqRecord[n];
	ix = m_header.m_reqPtr;
	for (int i = 0; i < n; ++i)
		m_reqRecords[i].fillFrom(buf, &ix, this);

	// Read component names, by language.
	//
	ix = m_header.m_componentPtr;
	m_componentRecord.fillFrom(buf, ix, this);

	// Read files.
	//
	n = m_header.m_nfiles;
	m_fileRecords = new SISFileRecord[n];
	ix = m_header.m_filesPtr;
	for (int i = 0; i < n; ++i)
		m_fileRecords[i].fillFrom(buf, &ix, this);

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

uchar*
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

