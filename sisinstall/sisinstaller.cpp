
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
				fprintf(stderr, "Checking for existance of %s\n", filename);
//			if (!m_psion->dirExists(filename))
				{
				if (logLevel >= 1)
					fprintf(stderr, "Creating dir %s\n", filename);
				Enum<rfsv::errs> res;
				res = m_psion->mkdir(filename);
				if ((res != rfsv::E_PSI_GEN_NONE) &&
					(res != rfsv::E_PSI_FILE_EXIST))
					{
#if HAVE_LIBNEWT
					if (!m_useNewt)
#endif
						fprintf(stderr, " -> Failed: %s\n", (const char*)res);
					}
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
			fprintf(stderr, "Setting drive at index %d to %c\n",
				   destptr, m_drive);
		}

	char msgbuf[1024];
	sprintf(msgbuf,
			"Copying %d bytes to %s\n",
			fileRecord->m_fileLengths[m_fileNo], dest);
#if HAVE_LIBNEWT
		if (m_useNewt)
			{
			newtPushHelpLine(msgbuf);
			newtRefresh();
			}
		else
#endif
			{
			printf("%s\n", msgbuf);
			}

	copyBuf(fileRecord->getFilePtr(m_fileNo),
			fileRecord->m_fileLengths[m_fileNo],
			dest);

#if HAVE_LIBNEWT
	if (m_useNewt)
		{
		newtPopHelpLine();
		newtRefresh();
		}
#endif
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
#if HAVE_LIBNEWT
		if (!m_useNewt)
#endif
			fprintf(stderr,
					"Couldn't create temp file: %s\n", strerror(errno));
		return;
		}
	Enum<rfsv::errs> res;
	if (logLevel >= 2)
		fprintf(stderr, "Storing in %s\n", srcName);
	write(fd, buf, len);
	close(fd);
	continueRunning = 1;
	res = m_psion->copyToPsion(srcName, name, NULL, checkAbortHash);
	if (res == rfsv::E_PSI_GEN_NONE)
		{
		if (logLevel >= 1)
			fprintf(stderr, " -> Success.\n");
		}
	else
		{
#if HAVE_LIBNEWT
		if (!m_useNewt)
#endif
			{
			fprintf(stderr, " -> Fail: %s\n", (const char*)res);
			}
		}
	unlink(srcName);
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

#if HAVE_LIBNEWT
			if (m_useNewt)
				{
				int len = fileRecord->m_fileLengths[m_fileNo];
				char* msgbuf = new char[len + 1];
				memcpy(msgbuf, fileRecord->getFilePtr(m_fileNo), len);
				msgbuf[len] = 0;
				int j = 0;
				for (int i = 0; i < len; ++i)
					{
					if (msgbuf[i] != '\r')
						msgbuf[j++] = msgbuf[i];
					}
				msgbuf[j] = 0;
				int cols = 0;
				int rows = 0;
				char* reflowedText =
					newtReflowText(msgbuf, 40, 20, 30, &cols, &rows);
				int flags = NEWT_FLAG_WRAP;
				if (rows > 15)
					{
					rows = 15;
					flags |= NEWT_FLAG_SCROLL;
					}
				newtComponent text = newtTextbox(0, 0, cols, rows, flags);
				newtTextboxSetText(text, reflowedText);
				newtOpenWindow(10, 5, cols + 2, rows + 5, "Info");
				newtComponent form = newtForm(NULL, NULL, 0);
				newtFormAddComponent(form, text);
				newtComponent button1;
				newtComponent button2 = 0;
				const char* b1;
				const char* b2;
				int rc = FILE_OK;
				switch (fileRecord->m_fileDetails)
					{
					case 0:
						b1 = _("Continue");
						button1 = newtButton((cols - strlen(b1)) / 2 - 1,
											 rows + 1, b1);
						break;
					case 1:
						b1 = _("Yes");
						b2 = _("No");
						button1 = newtButton(cols / 3 - strlen(b1) / 2 - 1,
											 rows + 1, b1);
						button2 = newtButton(cols / 3 * 2 - strlen(b2) / 2 - 1,
											 rows + 1, b2);
						rc = FILE_SKIP;
						break;
					case 2:
						b1 = _("Yes");
						b2 = _("No");
						button1 = newtButton(cols / 3 - strlen(b1) / 2 - 1,
											 rows + 1, b1);
						button2 = newtButton(cols / 3 * 2 - strlen(b2) / 2 - 1,
											 rows + 1, b2);
						rc = FILE_ABORT;
						break;
					}
				newtFormAddComponent(form, button1);
				if (button2)
					newtFormAddComponent(form, button2);
				newtComponent ender = newtRunForm(form);
				if (ender == button1)
					rc = FILE_OK;
				newtFormDestroy(form);
				newtPopWindow();
				delete reflowedText;
				delete msgbuf;
				return rc;
				}
			else
#endif
				{
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
				}
			break;
		case 2:
			{
			if (logLevel >= 1)
				fprintf(stderr, "Recursive sis file...\n");
			SISFile sisFile;
			uint8_t* buf2 = fileRecord->getFilePtr(m_fileNo);
			off_t len = fileRecord->m_fileLengths[m_fileNo];
//			if (m_lastSisFile < fileptr + len)
//				m_lastSisFile = fileptr + len;
			SisRC rc = sisFile.fillFrom(buf2, len);
			if (rc != SIS_OK)
				{
				fprintf(stderr,
						"Could not read contained sis file, rc = %d\n", rc);
				break;
				}
			SISInstaller installer;
			installer.setPsion(m_psion);
#if HAVE_LIBNEWT
			installer.useNewt(m_useNewt);
#endif
			installer.setInstalled(m_installed);
			rc = installer.run(&sisFile, buf2, len, m_file);
			if (0 == m_drive)
				{
				m_drive = sisFile.m_header.m_installationDrive;
				m_file->setDrive(m_drive);
				if (logLevel >= 1)
					fprintf(stderr,
							"Updated drive to %c from recursive sis file\n",
							m_drive);
				}
			break;
			}
		case 3:
			if (logLevel >= 1)
				fprintf(stderr, "Run %.*s during installation/remove\n",
						fileRecord->m_destLength, fileRecord->getDestPtr());
			break;
		case 4:
			if (logLevel >= 2)
				fprintf(stderr, "Running the app will create %.*s\n",
						fileRecord->m_destLength, fileRecord->getDestPtr());
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
#if HAVE_LIBNEWT
		if (m_useNewt)
			{
			newtPushHelpLine(_("Loading installed sis files."));
			newtRefresh();
			}
#endif
		while (!files.empty())
			{
			PlpDirent file = files[0];
			if (logLevel >= 1)
				fprintf(stderr, "Loading sis file `%s'\n", file.getName());
			char sisname[256];
			sprintf(sisname, "%s%s", SYSTEMINSTALL, file.getName());
			loadPsionSis(sisname);
			files.pop_front();
			}
#if HAVE_LIBNEWT
		if (m_useNewt)
			{
			newtPopHelpLine();
			newtRefresh();
			}
#endif
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
		fprintf(stderr, "Couldn't create temp file: %s\n", strerror(errno));
		return;
		}
	Enum<rfsv::errs> res;
	continueRunning = 1;
	if (logLevel >= 2)
		fprintf(stderr, "Copying from %s to temp file %s\n", name, srcName);
	res = m_psion->copyFromPsion(name, fd, checkAbortHash);
	if (res == rfsv::E_PSI_GEN_NONE)
		{
		off_t fileLen = lseek(fd, 0, SEEK_END);
		if (logLevel >= 2)
			fprintf(stderr, "Read %d bytes from the Psion file %s\n",
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
					fprintf(stderr, " Ok.\n");
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
}

void
SISInstaller::removeFile(SISFileRecord* fileRecord)
{
	int len = fileRecord->m_destLength;
	char dest[256];
	memcpy(dest, fileRecord->getDestPtr(), len);
	dest[len] = 0;
	if (logLevel >= 1)
		fprintf(stderr, "Removing file component %s.\n", dest);
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
#if HAVE_LIBNEWT
	newtComponent form, button, text, listbox;
#endif
	char msgbuf[1024];
	if (parent == 0)
		{
		n = m_file->m_header.m_nlangs;
		if (n == 1)
			{
			sprintf(msgbuf,
					_("You have only one language: %s"),
					m_file->getLanguage(0)->m_name);
#if HAVE_LIBNEWT
			if (m_useNewt)
				{
# if 0
				text = newtTextboxReflowed(1, 1, msgbuf, 37, 5, 5, 0);
				int th = newtTextboxGetNumLines(text);
				const char* okText = _("Ok");
				button = newtButton(18 - strlen(okText) / 2, th + 2, okText);
				newtOpenWindow(10, 5, 40, th + 6, "Language");
				form = newtForm(NULL, NULL, 0);
				newtFormAddComponent(form, text);
				newtFormAddComponent(form, button);
				newtRunForm(form);
				newtFormDestroy(form);
				newtPopWindow();
# endif
				}
			else
#endif
				{
				printf("%s\n", msgbuf);
				}
			lang = 0;
			}
		else
			{
#if HAVE_LIBNEWT
			if (m_useNewt)
				{
				sprintf(msgbuf, _("Select a language."));
				text = newtTextboxReflowed(1, 1, msgbuf, 37, 5, 5, 0);
				int th = newtTextboxGetNumLines(text);
				listbox = newtListbox(1, th + 2, 6,
									  NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);
				int boxwidth = 0;
				for (int i = 0; i < n; ++i)
					{
					const char* txt = m_file->getLanguage(i)->m_name;
					if (strlen(txt) > boxwidth)
						boxwidth = strlen(txt);
					newtListboxAppendEntry(listbox, txt, (void*) i);
					}
				newtOpenWindow(10, 5, 40, th + 8, "Language");
				form = newtForm(NULL, NULL, 0);
				newtFormAddComponent(form, text);
				newtFormAddComponent(form, listbox);
				newtRunForm(form);
				lang = (int)newtListboxGetCurrent(listbox);
				newtFormDestroy(form);
				newtPopWindow();
				}
			else
#endif
				{
				printf("Select a language (%d alternatives):\n", n);
				for (int i = 0; i < n; ++i)
					printf(" %d. %s\n", i, m_file->getLanguage(i)->m_name);
				lang = 0;
				}
			}
		}
	else
		{
//		This needs to check the _name_ of the language, since the
//		recursive sis file might have a different language list.
//		For now, defalt to 0.
//		lang = parent->getLanguage();
		lang = 0;
		if (logLevel >= 1)
			fprintf(stderr, "Forcing language to %d\n", lang);
		}
	m_file->setLanguage(lang);
	uint8_t* compName = m_file->getName();
	sprintf(msgbuf, _("Installing component: `%s'"), compName);
#if HAVE_LIBNEWT
	if (m_useNewt)
		{
		newtPushHelpLine(msgbuf);
		newtRefresh();
		}
	else
#endif
		{
		printf("%s\n", msgbuf);
		}

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
		fprintf(stderr, "Found %d requisites, of some sort.\n", n);
	for (int i = 0; i < n; ++i)
		{
		SISReqRecord* reqRecord = &m_file->m_reqRecords[i];
		if (logLevel >= 1)
			fprintf(stderr,
				" Check if app with uid %08x exists with version >= %d.%d\n",
				reqRecord->m_uid, reqRecord->m_major, reqRecord->m_minor);
		}

	// Check previous version.
	//
	if (logLevel >= 1)
		fprintf(stderr, 
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
		if (oldFile == 0)
			fprintf(stderr, "Already installed, but 0?\n");
		else
			{
			sprintf(msgbuf, _("Uninstalling the previous version first."));
#if HAVE_LIBNEWT
			if (m_useNewt)
				{
				newtPushHelpLine(msgbuf);
				newtRefresh();
				}
			else
#endif
				{
				printf("%s\n", msgbuf);
				}
			uninstall(oldFile);
#if HAVE_LIBNEWT
			if (m_useNewt)
				{
				newtPopHelpLine();
				newtRefresh();
				}
#endif
			}
		}

	// Install file components.
	//
	n = m_file->m_header.m_nfiles;
	if (logLevel >= 1)
		fprintf(stderr, "Found %d files.\n", n);
	m_drive = (parent == 0) ? 0 : parent->m_header.m_installationDrive;
	int nCopiedFiles = 0;
	m_lastSisFile = 0;
	bool skipnext = false;
	bool aborted = false;
	while (!aborted && (n-- > 0))
		{
		SISFileRecord* fileRecord = &m_file->m_fileRecords[n];
		m_fileNo = (fileRecord->m_flags & 1) ? lang : 0;

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
		fprintf(stderr,
				"Installed %d files of %d, cutting at offset %d.\n",
				m_file->m_header.m_installationFiles,
				m_file->m_header.m_nfiles,
				m_file->getResidualEnd());
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
	if (logLevel >= 1)
		fprintf(stderr, "Creating residual sis file %s\n", resname);
	copyBuf(buf, m_file->getResidualEnd(), resname);
#if HAVE_LIBNEWT
	if (m_useNewt)
		{
		newtPopHelpLine();
		}
#endif
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
	int space[26];
	int size[26];
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
					drivelist[ndrives] = 'A' + i;
					space[ndrives] = plpdrive.getSpace();
					size[ndrives] = plpdrive.getSize();
#if HAVE_LIBNEWT
					if (!m_useNewt)
#endif
						{
						printf("%c: %d bytes free, %d bytes total\n",
							   'A' + i,
							   plpdrive.getSpace(),
							   plpdrive.getSize());
						}
					++ndrives;
					}
				}
			devbits >>= 1;
			}
		}
	drivelist[ndrives] = 0;
	if (ndrives == 0)
		{
		m_drive = 'C';
		if (logLevel >= 1)
			printf("Selecting the only drive %c\n", m_drive);
		}
	else if (ndrives == 1)
		{
		m_drive = drivelist[0];
		if (logLevel >= 1)
			printf("Selecting the only drive %c\n", m_drive);
		}
	else
		{

#if HAVE_LIBNEWT
		if (m_useNewt)
			{
			char msgbuf[256];
			sprintf(msgbuf, _("Select a drive."));
			newtComponent text = newtTextboxReflowed(1, 1, msgbuf, 37, 5, 5, 0);
			int th = newtTextboxGetNumLines(text);
			newtComponent listbox =
				newtListbox(1, th + 2, 6,
							NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);
			int boxwidth = 0;
			for (int i = 0; i < ndrives; ++i)
				{
				char txt[256];
				sprintf(txt, "%c: %d kbytes free, %d kbytes total",
						drivelist[i], space[i] / 1024, size[i] / 1024);
				if (strlen(txt) > boxwidth)
					boxwidth = strlen(txt);
				newtListboxAppendEntry(listbox, txt, (void*) i);
				}
			newtOpenWindow(10, 5, boxwidth + 5, th + 8, "Drive");
			newtComponent form = newtForm(NULL, NULL, 0);
			newtFormAddComponent(form, text);
			newtFormAddComponent(form, listbox);
			newtRunForm(form);
			m_drive = 'A' + (int)newtListboxGetCurrent(listbox);
			newtFormDestroy(form);
			newtPopWindow();
			}
		else
#endif
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
		fprintf(stderr, "Uninstalling %d files, from a total of %d.\n",
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
				fprintf(stderr, "Recursive sis file...\n");
			SISFile sisFile;
			int fileptr = fileRecord->m_filePtrs[m_fileNo];
			uint8_t* buf2 = m_buf + fileptr;
			off_t len = fileRecord->m_fileLengths[m_fileNo];
			if (m_lastSisFile < fileptr + len)
				m_lastSisFile = fileptr + len;
			SisRC rc = sisFile.fillFrom(buf2, len);
			if (rc != SIS_OK)
				{
				fprintf(stderr,
						"Could not read contained sis file, rc = %d\n", rc);
				break;
				}
			SISInstaller installer;
			installer.setPsion(m_psion);
#if HAVE_LIBNEWT
			installer.useNewt(m_useNewt);
#endif
			installer.setInstalled(m_installed);
			rc = installer.run(&sisFile, buf2, len, m_file);
			if (0 == m_drive)
				{
				m_drive = sisFile.m_header.m_installationDrive;
				m_file->setDrive(m_drive);
				if (logLevel >= 1)
					fprintf(stderr,
							"Updated drive to %c from recursive sis file\n",
							m_drive);
				}
#endif
			break;
			}
		}
}

