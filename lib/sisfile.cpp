
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

