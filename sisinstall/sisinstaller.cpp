
#include "sisinstaller.h"

#include "sisfile.h"
#include "sisfilelink.h"
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
		if (logLevel >= 1)
			{
			printf("#");
			fflush(stdout);
			}
		}
	return continueRunning;
}

SISInstaller::SISInstaller()
{
	m_installed = 0;
	m_ownInstalled = false;
}

SISInstaller::~SISInstaller()
{
	if (m_ownInstalled)
		{
		SISFileLink* curr = m_installed;
		while (curr)
			{
			delete curr->m_file;
			SISFileLink* next = curr->m_next;
			delete curr;
			curr = next;
			}
		}
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
	uint8_t* destptr = fileRecord->getDestPtr();
	if (destptr == 0)
		return;
	if (destptr[0] == '!')
		{
		if (m_drive == 0)
			selectDrive();
		m_file->setDrive(m_drive);
		}
	int len = fileRecord->m_destLength;
	char dest[256];
	memcpy(dest, destptr, len);
	dest[len] = 0;
	if (dest[0] == '!')
		{
		dest[0] = m_drive;
		fileRecord->setMainDrive(m_drive);
		if (logLevel >= 2)
			printf("Setting drive at index %d to %c\n",
				   destptr, m_drive);
		}
	printf("Copying %d bytes to %s\n",
		   fileRecord->m_fileLengths[m_fileNo], dest);
	copyBuf(fileRecord->getFilePtr(m_fileNo),
			fileRecord->m_fileLengths[m_fileNo],
			dest);
}

void
SISInstaller::copyBuf(const uint8_t* buf, int len, char* name)
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
		if (logLevel >= 1)
			printf(" -> Success.\n");
		}
	else
		{
		printf(" -> Fail: %s\n", (const char*)res);
		}
	unlink(srcName);
	//sleep(10);
}

int
SISInstaller::installFile(SISFileRecord* fileRecord)
{
	char readbuf[8];
	switch (fileRecord->m_fileType)
		{
		case 0:
			copyFile(fileRecord);
			break;
		case 1:
			printf("Info:\n%.*s\n",
				   fileRecord->m_fileLengths[m_fileNo],
				   fileRecord->getFilePtr(m_fileNo));
			switch (fileRecord->m_fileDetails)
				{
				case 0:
					printf("Continue\n");
					fgets(readbuf, sizeof(readbuf), stdin);
					break;
				case 1:
					printf("(Install next file?) [Y]es/No\n");
					fgets(readbuf, sizeof(readbuf), stdin);
					if (strchr("Nn", readbuf[0]))
						{
						return FILE_SKIP;
						}
					break;
				case 2:
					printf("(Continue installation?) [Y]es/No\n");
					fgets(readbuf, sizeof(readbuf), stdin);
					if (strchr("Nn", readbuf[0]))
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
			uint8_t* buf2 = fileRecord->getFilePtr(m_fileNo);
			off_t len = fileRecord->m_fileLengths[m_fileNo];
//			if (m_lastSisFile < fileptr + len)
//				m_lastSisFile = fileptr + len;
			SisRC rc = sisFile.fillFrom(buf2, len);
			if (rc != SIS_OK)
				{
				printf("Could not read contained sis file, rc = %d\n", rc);
				break;
				}
			SISInstaller installer;
			installer.setPsion(m_psion);
			installer.setInstalled(m_installed);
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
				   fileRecord->m_destLength, fileRecord->getDestPtr());
			break;
		case 4:
			if (logLevel >= 2)
				printf("Running the app will create %.*s\n",
					   fileRecord->m_destLength,
					   fileRecord->getDestPtr());
			break;
		}
	return FILE_OK;
}

#define SYSTEMINSTALL "c:\\system\\install\\"

SisRC
SISInstaller::loadInstalled()
{
	PlpDir files;
	Enum<rfsv::errs> res;

	if ((res = m_psion->dir(SYSTEMINSTALL, files)) != rfsv::E_PSI_GEN_NONE)
		{
		return SIS_FAILED;
		}
	else
		{
		while (!files.empty())
			{
			PlpDirent file = files[0];
			if (logLevel >= 1)
				printf("Loading sis file `%s'\n", file.getName());
			char sisname[256];
			sprintf(sisname, "%s%s", SYSTEMINSTALL, file.getName());
			loadPsionSis(sisname);
			files.pop_front();
			}
		}
}

void
SISInstaller::loadPsionSis(const char* name)
{
	char srcName[32];
	strcpy(srcName, "/tmp/plptools-sis-XXXXXX");
	int fd = mkstemp(srcName);
	if (-1 == fd)
		{
		printf("Couldn't create temp file: %s\n", strerror(errno));
		return;
		}
	Enum<rfsv::errs> res;
	continueRunning = 1;
	if (logLevel >= 2)
		printf("Copying from %s to temp file %s\n", name, srcName);
	res = m_psion->copyFromPsion(name, fd, checkAbortHash);
	if (res == rfsv::E_PSI_GEN_NONE)
		{
		off_t fileLen = lseek(fd, 0, SEEK_END);
		if (logLevel >= 2)
			printf("Read %d bytes from the Psion file %s\n",
				   (int)fileLen, name);
		lseek(fd, SEEK_SET, 0);
		uint8_t* sisbuf = new uint8_t[fileLen];
		int rc = read(fd, sisbuf, fileLen);
		if (rc == fileLen)
			{
			SISFile* sisFile = new SISFile();
			SisRC rc2 = sisFile->fillFrom(sisbuf, fileLen);
			if (rc2 == SIS_OK)
				{
				if (logLevel >= 1)
					printf(" Ok.\n");
				SISFileLink* link = new SISFileLink(sisFile);
				link->m_next = m_installed;
				m_ownInstalled = true;
				m_installed = link;
				sisFile->ownBuffer();
				}
			else
				{
				delete sisFile;
				delete[] sisbuf;
				}
			}
		}
	close(fd);
	unlink(srcName);
	//sleep(10);
}

void
SISInstaller::removeFile(SISFileRecord* fileRecord)
{
	int len = fileRecord->m_destLength;
	char dest[256];
	memcpy(dest, fileRecord->getDestPtr(), len);
	dest[len] = 0;
	if (logLevel >= 1)
		printf("Removing file component %s.\n", dest);
	m_psion->remove(dest);
}

SisRC
SISInstaller::run(SISFile* file, uint8_t* buf, off_t len)
{
	return run(file, buf, len, 0);
}

SisRC
SISInstaller::run(SISFile* file, uint8_t* buf, off_t len, SISFile* parent)
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
	uint8_t* compName = m_file->getName();
	printf("Installing component: `%s'\n", compName);

	// In order to check requisites and previous versions, we need to
	// load all sis files from the c:/system/install directory.
	// This is the only way to find out if a specific application or
	// library has been loaded, since the sis file names could be just
	// about anything.
	//
	if (m_installed == 0)
		loadInstalled();

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
	if (logLevel >= 1)
		printf(
  "Checking if this app (uid %08x) exists with a version less than %d.%d.\n",
		   m_file->m_header.m_uid1,
		   m_file->m_header.m_major,
		   m_file->m_header.m_minor);

	bool uninstallFirst = false;
	SISFileLink* curr = m_installed;
	SISFile* oldFile = 0;
	while (curr)
		{
		SISFile* sisFile = curr->m_file;
		switch (sisFile->compareApp(m_file))
			{
			case SIS_VER_EARLIER:
				uninstallFirst = true;
				oldFile = sisFile;
				break;

			case SIS_SAME_OR_LATER:
				// Ask for confirmation.
				uninstallFirst = true;
				oldFile = sisFile;
				break;

			case SIS_OTHER_VARIANT:
				// Ask for confirmation.
				uninstallFirst = true;
				oldFile = sisFile;
				break;
			}
		curr = curr->m_next;
		}

	if (uninstallFirst)
		{
//		printf("You should uninstall the previous version first.\n");
//		if (!m_forced)
//			return SIS_ABORTED;
//		printf("Forced mode... Installing anyway!\n");
		printf("Uninstalling the previous version first.\n");
		if (oldFile == 0)
			printf("Already installed, but 0?\n");
		else
			uninstall(oldFile);
		}

	// Install file components.
	//
	n = m_file->m_header.m_nfiles;
	if (logLevel >= 1)
		printf("Found %d files.\n", n);
	m_drive = (parent == 0) ? 0 : parent->m_header.m_installationDrive;
	int nCopiedFiles = 0;
	int firstFile = -1;
	m_lastSisFile = 0;
	bool skipnext = false;
	bool aborted = false;
	while (!aborted && (n-- > 0))
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
		int fileIx = fileRecord->getFilePtr(m_fileNo) - m_buf;
		if ((firstFile == -1) || (firstFile >= fileIx))
			firstFile = fileIx;

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
					aborted = true;
					break;
				}
			}
		if (!aborted)
			nCopiedFiles++;
		}
	m_file->setFiles(nCopiedFiles);
	if (logLevel >= 1)
		printf("Installed %d files of %d, cutting at offset max(%d,%d)\n",
			   m_file->m_header.m_installationFiles,
			   m_file->m_header.m_nfiles,
			   firstFile, m_lastSisFile);
	if (firstFile < m_lastSisFile)
		firstFile = m_lastSisFile;
	if (nCopiedFiles == 0)
		{
		// There is no need to copy any uninstall information to the
		// psion, unless we've actually copied anything there.
		//
		return SIS_ABORTED;
		}

	// Copy the updated sis file to the epoc machine.
	//
	char resname[256];
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
	if (aborted)
		return SIS_ABORTED;
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

void
SISInstaller::setPsion(Psion* psion)
{
	m_psion = psion;
}

void
SISInstaller::uninstall(SISFile* file)
{
	int n = file->m_header.m_nfiles;
	int fileix = n - file->m_header.m_installationFiles;
	if (logLevel >= 1)
		printf("Uninstalling %d files, from a total of %d.\n",
			   file->m_header.m_installationFiles,
			   file->m_header.m_nfiles);
	int lang = file->getLanguage();
	while (fileix < n)
		{
		SISFileRecord* fileRecord = &file->m_fileRecords[fileix];
		m_fileNo = (fileRecord->m_flags & 1) ? lang : 0;
		char drive = file->m_header.m_installationDrive;
		fileRecord->setMainDrive(drive);
		uninstallFile(fileRecord);
		++fileix;
		}
}

void
SISInstaller::uninstallFile(SISFileRecord* fileRecord)
{
	switch (fileRecord->m_fileType)
		{
		case 0:
		case 4:
			removeFile(fileRecord);
			break;
		case 2:
			{
#if 0
			// This is messy... We can't remove the sis component unless
			// we've stored the entire component in the residual sis
			// file no the target machine.
			//
			if (logLevel >= 1)
				printf("Recursive sis file...\n");
			SISFile sisFile;
			int fileptr = fileRecord->m_filePtrs[m_fileNo];
			uint8_t* buf2 = m_buf + fileptr;
			off_t len = fileRecord->m_fileLengths[m_fileNo];
			if (m_lastSisFile < fileptr + len)
				m_lastSisFile = fileptr + len;
			SisRC rc = sisFile.fillFrom(buf2, len);
			if (rc != SIS_OK)
				{
				printf("Could not read contained sis file, rc = %d\n", rc);
				break;
				}
			SISInstaller installer;
			installer.setPsion(m_psion);
			installer.setInstalled(m_installed);
			rc = installer.run(&sisFile, buf2, len, m_file);
			if (0 == m_drive)
				{
				m_drive = sisFile.m_header.m_installationDrive;
				m_file->setDrive(m_drive);
				if (logLevel >= 1)
					printf("Updated drive to %c from recursive sis file\n",
						   m_drive);
				}
#endif
			break;
			}
		}
}

