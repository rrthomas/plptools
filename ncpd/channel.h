#ifndef _channel_h_
#define _channel_h_

#include <stdio.h>

#include "bool.h"

class ncp;
class bufferStore;

class channel {
public:
  channel(ncp *ncpController);
  void newNcpController(ncp *ncpController);

  void setNcpChannel(int chan);
  void ncpSend(bufferStore &a);
  virtual void ncpDataCallback(bufferStore &a) = NULL;
  virtual const char *getNcpConnectName() = NULL;
  void ncpConnect();
  virtual void ncpConnectAck() = NULL;
  virtual void ncpConnectTerminate() = NULL;
  void ncpDisconnect();

  // The following two calls are used for destructing an instance
  virtual bool terminate(); // Mainloop will terminate this class if true
  void terminateWhenAsked();
private:
  ncp *ncpController;
  bool _terminate;
  int ncpChannel;
};

#endif
