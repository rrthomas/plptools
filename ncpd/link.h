#ifndef _link_h_
#define _link_h_

#include "bool.h"
#include "bufferstore.h"
#include "bufferarray.h"

#define LNK_DEBUG_LOG  1
#define LNK_DEBUG_DUMP 2

class packet;
class IOWatch;

class link {
	public:
  		link(const char *fname, int baud, IOWatch &iow, unsigned short _verbose = 0);
  		~link();
  		void send(const bufferStore &buff);
  		bufferArray poll();
  		bool stuffToSend();
  		bool hasFailed();
		void reset();
		void setVerbose(short int);
		short int getVerbose();
		void setPktVerbose(short int);
		short int getPktVerbose();
  
	private:
  		packet *p;
  		int idSent;
  		int countToResend;
  		int timesSent;
  		bufferArray sendQueue;
  		bufferStore toSend;
  		int idLastGot;
  		bool newLink;
  		unsigned short verbose;
  		bool somethingToSend;
  		bool failed;
};

#endif
