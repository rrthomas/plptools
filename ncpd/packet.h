#ifndef _packet_h
#define _packet_h

#include <stdio.h>

#include "bool.h"
class psiEmul;
class bufferStore;
class IOWatch;

#define PKT_DEBUG_LOG  1
#define PKT_DEBUG_DUMP 2

class packet {
	public:
		packet(const char *fname, int baud, IOWatch &iow, short int verbose = 0);
		~packet();
		void send(unsigned char type, const bufferStore &b);
		bool get(unsigned char &type, bufferStore &b);
		void setVerbose(short int);
		short int getVerbose();
  
	private:
		bool terminated();
		void addToCrc(unsigned short a, unsigned short *crc);
		void opByte(unsigned char a);
		void realWrite();

		unsigned short crcOut;
		unsigned short crcIn;
		unsigned char *inPtr;
		unsigned char *outPtr;
		unsigned char *endPtr;
		unsigned char *inBuffer;
		unsigned char *outBuffer;
		bufferStore rcv;
		int inLen;
		int outLen;
		int termLen;
		int fd;
		bool verbose;
		bool esc;
		char *devname;
		int baud;
		IOWatch &iow;
};

#endif
