#ifndef _linkchan_h_
#define _linkchan_h_

#include "channel.h"

class linkChan : public channel {
public:
  linkChan(ncp *ncpController);
  
  void ncpDataCallback(bufferStore &a);
  const char *getNcpConnectName();
  void ncpConnectAck();
  void ncpConnectTerminate();
};

#endif
