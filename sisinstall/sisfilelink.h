#ifndef _SISFILELINK_H
#define _SISFILELINK_H

class SISFile;

class SISFileLink
{
public:
	SISFileLink(SISFile* file);

	SISFileLink* m_next;
	SISFile* m_file;
};

#endif

