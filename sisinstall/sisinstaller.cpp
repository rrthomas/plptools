
#include "sisinstaller.h"

#include "sisfile.h"
#include "sisfilerecord.h"
#include "sisreqrecord.h"
#include "psion.h"

#include <unistd.h>
#include <stdio.h>

void
SISInstaller::setPsion(Psion* psion)
{
	m_psion = psion;
}

void
SISInstaller::run(SISFile* file, uchar* buf)
{
	run(file, buf, 0);
}

void
SISInstaller::run(SISFile* file, uchar* buf, SISFile* parent)
{
	int n;
	int lang;
	if (parent == 0)
		{
		n = file->m_header.m_nlangs;
		if (n == 1)
			{
			printf("You have only one language: %s\n",
				   file->getLanguage(0)->m_name);
			lang = 0;
			}
		else
			{
			printf("Select a language (%d alternatives):\n", n);
			for (int i = 0; i < n; ++i)
				{
				printf(" %d. %s\n", i, file->getLanguage(i)->m_name);
				}
			lang = 0;
			}
		}
	else
		{
		lang = parent->getLanguage();
		printf("Forcing language to %d\n", lang);
		}
	file->setLanguage(lang);
	printf("Installing component: `%s'\n", file->getName());

	// Check Requisites.
	//
	n = file->m_header.m_nreqs;
	printf("Found %d requisites, of some sort.\n", n);
	for (int i = 0; i < n; ++i)
		{
		SISReqRecord* reqRecord = &file->m_reqRecords[i];
		printf(" Check if app with uid %08x exists with version >= %d.%d\n",
		   reqRecord->m_uid,
		   reqRecord->m_major,
		   reqRecord->m_minor);
		}

	// Check previous version.
	//
	printf(
  "Checking if this app (uid %08x) exists with a version less than %d.%d.\n",
		   file->m_header.m_uid1,
		   file->m_header.m_major,
		   file->m_header.m_minor);

	// Install file components.
	//
	n = file->m_header.m_nfiles;
	printf("Found %d files.\n", n);
	char drive = (parent == 0) ? 0 : parent->m_header.m_installationDrive;
	file->m_header.m_installationFiles = 0;
	int firstFile = -1;
	while (n-- > 0)
		{
		SISFileRecord* fileRecord = &file->m_fileRecords[n];
		int ix = 0;
		if (fileRecord->m_flags & 1)
			ix = lang;
		char ch;
#if 0
		printf("FirstFile = %d, ptr = %d, length = %d\n",
			   firstFile,
			   fileRecord->m_filePtrs[ix],
			   fileRecord->m_fileLengths[ix]);
#endif
		if ((firstFile == -1) ||
			(firstFile >= fileRecord->m_filePtrs[ix]))
			firstFile = fileRecord->m_filePtrs[ix];

//		 We can only do this if we search all files...
//		 fileRecord->m_filePtrs[ix] + fileRecord->m_fileLengths[ix]

		switch (fileRecord->m_fileType)
			{
			case 0:
				if (buf[fileRecord->m_destPtr] == '!')
					{
					if (drive == 0)
						{
#if 0
						u_int32_t devbits = 0;
						Enum<rfsv::errs> res;
						if ((res = m_psion->m_rfsv->devlist(devbits)) == rfsv::E_PSI_GEN_NONE)
							{
							for (int i = 0; i < 26; i++)
								{
								PlpDrive plpdrive;
								if ((devbits & 1) != 0)
									{
									u_int32_t mediaType = plpdrive.getMediaType();
									if ((mediaType == 3) || (mediaType == 5))
										{
										printf("%c: %d bytes free\n", 'A' + i, plpdrive.getSpace());
										}
									}
								devbits >>= 1;
								}
							}
#endif
						printf("Please select a drive\n");
						read(0, &ch, 1);
						if (ch >= 'a' && ch <= 'z')
							drive = ch;
						else
							drive = 'c';
						}
					file->m_header.m_installationDrive = drive;
					}
				printf("Copying %d bytes to %.*s\n",
					   fileRecord->m_fileLengths[ix],
					   fileRecord->m_destLength,
					   buf + fileRecord->m_destPtr);
				break;
			case 1:
				printf("Info:\n%.*s\n",
					   fileRecord->m_fileLengths[ix],
					   buf + fileRecord->m_filePtrs[ix]);
				switch (fileRecord->m_fileDetails)
					{
					case 0:
						printf("Continue\n");
//						read(0, &ch, 1);
						break;
					case 1:
						printf("Skip next file? Yes/No\n");
//						read(0, &ch, 1);
						break;
					case 2:
						printf("[Continue installation?] Yes/No\n");
//						read(0, &ch, 1);
						break;
					}
				break;
			case 2:
				{
				printf("Recursive sis file...\n");
				SISFile sisFile;
				uchar* buf2 = buf + fileRecord->m_filePtrs[ix];
				sisFile.fillFrom(buf2);
				SISInstaller installer;
				installer.run(&sisFile, buf2, file);
				if (0 == file->m_header.m_installationDrive)
					drive =
						file->m_header.m_installationDrive = 
						sisFile.m_header.m_installationDrive;
				break;
				}
			case 3:
				printf("Run %.*s during installation/remove\n",
					   fileRecord->m_destLength, buf + fileRecord->m_destPtr);
				break;
			case 4:
				printf("Running the app will create %.*s\n",
					   fileRecord->m_destLength, buf + fileRecord->m_destPtr);
				break;
			}

		file->m_header.m_installationFiles++;
		}
	printf("Installed %d files of %d, cutting at offset %d\n",
		   file->m_header.m_installationFiles,
		   file->m_header.m_nfiles,
		   firstFile);

	// Set the new shortened size somewhere.
	//

	// Copy the updated sis file to the epoc machine.
	//

}

