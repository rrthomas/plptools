#ifndef _socketchan_h_
#define _socketchan_h_

#include "bool.h"
#include "channel.h"
class ppsocket;
class IOWatch;

class socketChan : public channel {
public:
  socketChan(ppsocket* comms, ncp* ncpController, IOWatch &iow);
  virtual ~socketChan();

  void ncpDataCallback(bufferStore& a);
  const char* getNcpConnectName();
  void ncpConnectAck();
  void ncpConnectTerminate();

  bool isConnected() const;
  void socketPoll();
private:
  ppsocket* skt;
  IOWatch &iow;
  char* connectName;
  bool connected;
};

#endif
