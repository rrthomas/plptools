#ifndef _SISINSTALLER_H
#define _SISINSTALLER_H

#include "sistypes.h"

class Psion;
class SISFile;

/**
 * A minimal SIS installer.
 * Handles recursive sis files.
 */
class SISInstaller
{
public:

	void run(SISFile* file, uchar* buf);

	void run(SISFile* file, uchar* buf, SISFile* parent);

	void setPsion(Psion* psion);

private:

	Psion* m_psion;

};

#endif

