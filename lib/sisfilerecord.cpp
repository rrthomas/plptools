
#include "sisfilerecord.h"
#include "sisfile.h"

#include <stdio.h>

void
SISFileRecord::fillFrom(uchar* buf, int* base, SISFile* sisFile)
{
	int ix = *base;
	m_flags = read32(buf, &ix);
	if (logLevel >= 2)
		printf("Got flags %d\n", m_flags);
	m_fileType = read32(buf, &ix);
	if (logLevel >= 2)
		printf("Got file type %d\n", m_fileType);
	m_fileDetails = read32(buf, &ix);
	if (logLevel >= 2)
		printf("Got file details %d\n", m_fileDetails);
	m_sourceLength = read32(buf, &ix);
	m_sourcePtr = read32(buf, &ix);
//	printf("Got source length = %d, source name ptr = %d\n",
//		   m_sourceLength, m_sourcePtr);
	if (logLevel >= 2)
		if (m_sourceLength > 0)
			printf("Got source name %.*s\n", m_sourceLength, buf + m_sourcePtr);
	m_destLength = read32(buf, &ix);
	m_destPtr = read32(buf, &ix);
//	printf("Got dest length = %d, dest name ptr = %d\n",
//		   m_destLength, m_destPtr);
	if (logLevel >= 2)
		printf("Got destination name %.*s\n", m_destLength, buf + m_destPtr);
	switch (m_flags)
		{
		case 0: // Only one file.
			m_fileLengths = new uint32[1];
			m_filePtrs = new uint32[1];
			m_fileLengths[0] = read32(buf, &ix);
			m_filePtrs[0] = read32(buf, &ix);
			if (logLevel >= 2)
				printf("File is %d bytes long (at %d) (to %d)\n",
					   m_fileLengths[0], m_filePtrs[0],
					   m_fileLengths[0] + m_filePtrs[0]);
			if (logLevel >= 1)
				printf("%d .. %d (%d bytes): Single file record type %d, %.*s\n",
					   m_filePtrs[0],
					   m_filePtrs[0] + m_fileLengths[0],
					   m_fileLengths[0],
					   m_fileType,
					   m_destLength, buf + m_destPtr);
			break;

		case 1: // One file per language.
			{
			int n = sisFile->m_header.m_nlangs;
			m_fileLengths = new uint32[n];
			m_filePtrs = new uint32[n];
			for (int i = 0; i < n; ++i)
				{
				m_fileLengths[i] = read32(buf, &ix);
				}
			for (int i = 0; i < n; ++i)
				{
				m_filePtrs[i] = read32(buf, &ix);
				int len = m_fileLengths[i];
				if (logLevel >= 2)
					printf("File %d (for %s) is %d bytes long (at %d)\n",
						   i,
						   sisFile->getLanguage(i)->m_name,
						   len,
						   m_filePtrs[i]);
				if (logLevel >= 1)
					printf("%d .. %d (%d bytes): File record (%s) for %.*s\n",
						   m_filePtrs[i],
						   m_filePtrs[i] + len,
						   len,
						   sisFile->getLanguage(i)->m_name,
						   m_destLength, buf + m_destPtr);
				}
			break;
			}

		default:
			if (logLevel >= 2)
				printf("Unknown file flags %d\n", m_flags);
		}
	*base = ix;
}

