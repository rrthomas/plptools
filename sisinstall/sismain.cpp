
#include "sisfile.h"
#include "sisinstaller.h"
#include "psion.h"
#include "fakepsion.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

static void error(int line)
{
	fprintf(stderr, "Got errno = %d on line %d\n", errno, line);
	exit(1);
}

void main(int argc, char* argv[])
{
	char* filename = 0;
	char option;
	bool dryrun = false;
	while ((option = getopt(argc, argv, "nl:")) != -1)
		{
		switch (option)
			{
			case 'l':
				logLevel = atoi(optarg);
				break;
			case 'n':
				dryrun = true;
				break;
			}
		}
	if (optind < argc)
		{
		filename = argv[optind];
		printf("Installing sis file %s%s\n", filename,
			   dryrun ? ", not really" : "");
		}
	struct stat st;
	if (-1 == stat(filename, &st))
		error(__LINE__);
	off_t len = st.st_size;
	if (logLevel >= 2)
		printf("File is %d bytes long\n", len);
	uchar* buf = new uchar[len];
	int fd = open(filename, O_RDONLY);
	if (-1 == fd)
		error(__LINE__);
	if (-1 == read(fd, buf, len))
		error(__LINE__);
	close(fd);
	Psion* psion;
	if (dryrun)
		psion = new FakePsion();
	else
		psion = new Psion();
	if (!psion->connect())
		{
		printf("Couldn't connect with the Psion\n");
		exit(1);
		}
	createCRCTable();
	SISFile sisFile;
	SisRC rc = sisFile.fillFrom(buf, len);
	if (rc == SIS_OK)
		{
		if (!dryrun)
			{
			SISInstaller installer;
			installer.setPsion(psion);
			installer.run(&sisFile, buf, len);
			}
		}
	else
		{
		printf("Could not parse the sis file.\n");
		}
	psion->disconnect();

	exit(0);
}

