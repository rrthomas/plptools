#ifndef _linkchan_h_
#define _linkchan_h_

#include "channel.h"

#define LINKCHAN_DEBUG_LOG  1
#define LINKCHAN_DEBUG_DUMP 2

class linkChan : public channel {
	public:
		linkChan(ncp *ncpController);
  
		void ncpDataCallback(bufferStore &a);
		const char *getNcpConnectName();
		void ncpConnectAck();
		void ncpConnectTerminate();
};

#endif
