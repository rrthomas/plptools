
#include "sisinstaller.h"

#include "sisfile.h"
#include "sisfilerecord.h"
#include "sisreqrecord.h"
#include "psion.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>

static int continueRunning;

static int
checkAbortHash(void *, u_int32_t)
{
	if (continueRunning)
		{
		printf("#");
		fflush(stdout);
		}
	return continueRunning;
}

void
SISInstaller::createDirs(char* filename)
{
	char* end = filename + strlen(filename);
	while (--end > filename)
		{
		char ch = *end;
		if ((ch == '/') || (ch == '\\'))
			{
			*end = 0;
			if (logLevel >= 1)
				printf("Checking for existance of %s\n", filename);
//			if (!m_psion->dirExists(filename))
				{
				printf("Creating dir %s\n", filename);
				Enum<rfsv::errs> res;
				res = m_psion->mkdir(filename);
				if ((res != rfsv::E_PSI_GEN_NONE) &&
					(res != rfsv::E_PSI_FILE_EXIST))
					printf(" -> Failed: %s\n", (const char*)res);
				}
			*end = ch;
			return;
			}
		}
}

void
SISInstaller::copyFile(SISFileRecord* fileRecord)
{
	if (m_buf[fileRecord->m_destPtr] == '!')
		{
		if (m_drive == 0)
			selectDrive();
		m_file->setDrive(m_drive);
		}
	int len = fileRecord->m_destLength;
	char* dest = new char[len + 1];
	memcpy(dest, m_buf + fileRecord->m_destPtr, len);
	dest[len] = 0;
	if (dest[0] == '!')
		{
		dest[0] = m_drive;
		m_buf[fileRecord->m_destPtr] = m_drive;
		}
	printf("Copying %d bytes to %s\n",
		   fileRecord->m_fileLengths[m_fileNo], dest);
	copyBuf(m_buf + fileRecord->m_filePtrs[m_fileNo],
			fileRecord->m_fileLengths[m_fileNo],
			dest);
	delete[] dest;
}

void
SISInstaller::copyBuf(const uchar* buf, int len, char* name)
{
	createDirs(name);
	char srcName[32];
	strcpy(srcName, "/tmp/plptools-sis-XXXXXX");
	int fd = mkstemp(srcName);
	if (-1 == fd)
		{
		printf("Couldn't create temp file: %s\n", strerror(errno));
		return;
		}
	Enum<rfsv::errs> res;
	if (logLevel >= 2)
		printf("Storing in %s\n", srcName);
	write(fd, buf, len);
	close(fd);
	continueRunning = 1;
	res = m_psion->copyToPsion(srcName, name, NULL, checkAbortHash);
	if (res == rfsv::E_PSI_GEN_NONE)
		{
		printf(" -> Success.\n");
		}
	else
		{
		printf(" -> Fail: %s\n", (const char*)res);
		}
	unlink(srcName);
}

int
SISInstaller::installFile(SISFileRecord* fileRecord)
{
	char readbuf[2];
	switch (fileRecord->m_fileType)
		{
		case 0:
			copyFile(fileRecord);
			break;
		case 1:
			printf("Info:\n%.*s\n",
				   fileRecord->m_fileLengths[m_fileNo],
				   m_buf + fileRecord->m_filePtrs[m_fileNo]);
			switch (fileRecord->m_fileDetails)
				{
				case 0:
					printf("Continue\n");
					fgets(readbuf, 2, stdin);
					break;
				case 1:
					printf("(Install next file?) [Y]es/No\n");
					fgets(readbuf, 2, stdin);
					if (strchr("Nn", readbuf[0]))
						{
						return FILE_SKIP;
						}
					break;
				case 2:
					printf("(Continue installation?) [Y]es/No\n");
					fgets(readbuf, 2, stdin);
					if (!strchr("Nn", readbuf[0]))
						{
						// Watch out if we have copied any files
						// already.
						//
						return FILE_ABORT;
						}
					break;
				}
			break;
		case 2:
			{
			if (logLevel >= 1)
				printf("Recursive sis file...\n");
			SISFile sisFile;
			uchar* buf2 = m_buf + fileRecord->m_filePtrs[m_fileNo];
			off_t len = fileRecord->m_fileLengths[m_fileNo];
			SisRC rc = sisFile.fillFrom(buf2, len);
			if (rc != SIS_OK)
				{
				printf("Could not read contained sis file, rc = %d\n", rc);
				break;
				}
			SISInstaller installer;
			installer.setPsion(m_psion);
			rc = installer.run(&sisFile, buf2, len, m_file);
			if (0 == m_drive)
				{
				m_drive = sisFile.m_header.m_installationDrive;
				m_file->setDrive(m_drive);
				if (logLevel >= 1)
					printf("Updated drive to %c from recursive sis file\n",
						   m_drive);
				}
			break;
			}
		case 3:
			printf("Run %.*s during installation/remove\n",
				   fileRecord->m_destLength, m_buf + fileRecord->m_destPtr);
			break;
		case 4:
			if (logLevel >= 2)
				printf("Running the app will create %.*s\n",
					   fileRecord->m_destLength,
					   m_buf + fileRecord->m_destPtr);
			break;
		}
	return FILE_OK;
}

void
SISInstaller::setPsion(Psion* psion)
{
	m_psion = psion;
}

SisRC
SISInstaller::run(SISFile* file, uchar* buf, off_t len)
{
	return run(file, buf, len, 0);
}

SisRC
SISInstaller::run(SISFile* file, uchar* buf, off_t len, SISFile* parent)
{
	int n;
	int lang;
	m_file = file;
	m_buf = buf;
	if (parent == 0)
		{
		n = m_file->m_header.m_nlangs;
		if (n == 1)
			{
			printf("You have only one language: %s\n",
				   m_file->getLanguage(0)->m_name);
			lang = 0;
			}
		else
			{
			printf("Select a language (%d alternatives):\n", n);
			for (int i = 0; i < n; ++i)
				{
				printf(" %d. %s\n", i, m_file->getLanguage(i)->m_name);
				}
			lang = 0;
			}
		}
	else
		{
		lang = parent->getLanguage();
		if (logLevel >= 1)
			printf("Forcing language to %d\n", lang);
		}
	m_file->setLanguage(lang);
	uchar* compName = m_file->getName();
	printf("Installing component: `%s'\n", compName);

	// Check Requisites.
	//
	n = m_file->m_header.m_nreqs;
	if (logLevel >= 1)
		printf("Found %d requisites, of some sort.\n", n);
	for (int i = 0; i < n; ++i)
		{
		SISReqRecord* reqRecord = &m_file->m_reqRecords[i];
		printf(" Check if app with uid %08x exists with version >= %d.%d\n",
		   reqRecord->m_uid,
		   reqRecord->m_major,
		   reqRecord->m_minor);
		}

	// Check previous version.
	//
	printf(
  "Checking if this app (uid %08x) exists with a version less than %d.%d.\n",
		   m_file->m_header.m_uid1,
		   m_file->m_header.m_major,
		   m_file->m_header.m_minor);

	// Install file components.
	//
	n = m_file->m_header.m_nfiles;
	if (logLevel >= 1)
		printf("Found %d files.\n", n);
	m_drive = (parent == 0) ? 0 : parent->m_header.m_installationDrive;
	int nCopiedFiles = 0;
	int firstFile = -1;
	bool skipnext = false;
	while (n-- > 0)
		{
		SISFileRecord* fileRecord = &m_file->m_fileRecords[n];
		m_fileNo = (fileRecord->m_flags & 1) ? lang : 0;
		char ch;
#if 0
		printf("FirstFile = %d, ptr = %d, length = %d\n",
			   firstFile,
			   fileRecord->m_filePtrs[m_fileNo],
			   fileRecord->m_fileLengths[m_fileNo]);
#endif
		if ((firstFile == -1) ||
			(firstFile >= fileRecord->m_filePtrs[m_fileNo]))
			firstFile = fileRecord->m_filePtrs[m_fileNo];

//		 We can only do this if we search all files...
//		 fileRecord->m_filePtrs[m_fileNo] + fileRecord->m_fileLengths[m_fileNo]

		if (skipnext)
			{
			skipnext = false;
			}
		else
			{
			switch (installFile(fileRecord))
				{
				case FILE_OK:
					break;
				case FILE_SKIP:
					skipnext = true;
					break;
				case FILE_ABORT:
					break;
				}
			}

		nCopiedFiles++;
		}
	m_file->setFiles(nCopiedFiles);
	if (logLevel >= 1)
		printf("Installed %d files of %d, cutting at offset %d\n",
			   m_file->m_header.m_installationFiles,
			   m_file->m_header.m_nfiles,
			   firstFile);

	// Copy the updated sis file to the epoc machine.
	//
	char* resname = new char[256];
	int namelen = 0;
	while (compName[namelen] != 0)
		{
		if (compName[namelen] == ' ')
			break;
		namelen++;
		}
	sprintf(resname, "C:\\System\\Install\\%.*s.sis", namelen, compName);
	printf("Creating residual sis file %s\n", resname);
	copyBuf(buf, firstFile, resname);
	delete[] resname;
	return SIS_OK;
}

void
SISInstaller::selectDrive()
{
	u_int32_t devbits = 0;
	Enum<rfsv::errs> res;
	char drivelist[26];
	int ndrives = 0;
	if ((res = m_psion->devlist(devbits)) == rfsv::E_PSI_GEN_NONE)
		{
		for (int i = 0; i < 26; i++)
			{
			PlpDrive plpdrive;
			if (((devbits & 1) != 0) &&
				(m_psion->devinfo(i + 'A', plpdrive) == rfsv::E_PSI_GEN_NONE))
				{
				u_int32_t mediaType = plpdrive.getMediaType();
				if ((mediaType == 3) || (mediaType == 5))
					{
					drivelist[ndrives++] = 'A' + i;
					printf("%c: %d bytes free, %d bytes total\n",
						   'A' + i,
						   plpdrive.getSpace(),
						   plpdrive.getSize());
					}
				}
			devbits >>= 1;
			}
		}
	drivelist[ndrives] = 0;
	if (ndrives == 0)
		{
		m_drive = 'C';
		printf("Selecting the only drive %c\n", m_drive);
		}
	else if (ndrives == 1)
		{
		m_drive = drivelist[0];
		printf("Selecting the only drive %c\n", m_drive);
		}
	else
		{
		printf("Please select a drive\n");
		char ch;
		char readbuf[2];
		while (m_drive == 0)
			{
			fgets(readbuf, 2, stdin);
			ch = readbuf[0];
			if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
				{
				m_drive = toupper(ch);
				if (!strchr(drivelist, m_drive))
					{
					m_drive = 0;
					printf("Please select a valid drive: %s\n", drivelist);
					}
				}
			}
		}
}

