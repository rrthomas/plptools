#ifndef _ftp_h_
#define _ftp_h_

#include "bool.h"

class rfsv32;
class bufferStore;
class bufferArray;

#define DEFAULT_DRIVE "C:"
#define DEFAULT_BASE_DIRECTORY "\\"

class ftp {
	public:
	ftp();
	~ftp();
	int session(rfsv32 & a, int xargc, char **xargv);

	 private:
	void getCommand(int &argc, char **argv);

	// utilities
	bool unixDirExists(const char *dir);
	void getUnixDir(bufferArray & files);
	void resetUnixPwd();
	void usage();
	void errprint(long errcode, rfsv32 & a);
	void cd(const char *source, const char *cdto, char *dest);
	int convertName(const char *orig, char *retVal);

	char localDir[1024];
	char psionDir[1024];
};

#endif
