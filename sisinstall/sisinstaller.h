#ifndef _SISINSTALLER_H
#define _SISINSTALLER_H

#include "sistypes.h"

#include <sys/types.h>

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

	SisRC run(SISFile* file, uint8_t* buf, off_t len);

	SisRC run(SISFile* file, uint8_t* buf, off_t len, SISFile* parent);

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

	uint8_t* m_buf;

	SISFile* m_file;

	enum {
		FILE_OK,
		FILE_SKIP,
		FILE_ABORT,
	};

	/**
	 * Store the contents of a buffer in a file on the Psion.
	 */
	void copyBuf(const uint8_t* buf, int len, char* name);

	/**
	 * Copy a file to the Psion.
	 */
	void copyFile(SISFileRecord* fileRecord);

	void createDirs(char* filename);

	int installFile(SISFileRecord* fileRecord);

	void uninstall(SISFile* sisFile);

};

#endif

