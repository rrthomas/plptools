
#include "sisfile.h"
#include "sisinstaller.h"
#include "psion.h"

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
	fprintf(stderr, "Got errno = %d (%s) on line %d\n",
			errno, strerror(errno), line);
	exit(1);
}

void main(int argc, char* argv[])
{
	char* filename = 0;
	if (argc > 1)
		filename = argv[1];
	else
		return 0;
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
	Psion psion;
#if 0
	if (!psion.connect())
		{
		printf("Couldn't connect with the Psion\n");
		exit(1);
		}
#endif
	createCRCTable();
	SISFile sisFile;
	sisFile.fillFrom(buf);
	SISInstaller installer;
	installer.setPsion(&psion);
	installer.run(&sisFile, buf);
#if 0
	psion.disconnect();
#endif

	exit(0);
}

