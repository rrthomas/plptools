#ifndef _SISINSTALLER_H
#define _SISINSTALLER_H

#include "sistypes.h"

class Psion;
class SISFile;
class SISFileRecord;

/**
 * A minimal SIS installer.
 * Handles recursive sis files.
 */
class SISInstaller
{
public:

	void run(SISFile* file, uchar* buf);

	void run(SISFile* file, uchar* buf, SISFile* parent);

	/**
	 * Ask the user which drive to install to.
	 */
	void selectDrive();

	/**
	 * Set the Psion manager.
	 */
	void setPsion(Psion* psion);

private:

	char m_drive;

	int m_fileNo;

	Psion* m_psion;

	uchar* m_buf;

	SISFile* m_file;

	enum {
		FILE_OK,
		FILE_SKIP,
		FILE_ABORT,
	};

	/**
	 * Store the contents of a buffer in a file on the Psion.
	 */
	void copyBuf(const uchar* buf, int len, char* name);

	/**
	 * Copy a file to the Psion.
	 */
	void copyFile(SISFileRecord* fileRecord);

	void createDirs(char* filename);

	int installFile(SISFileRecord* fileRecord);

};

#endif

