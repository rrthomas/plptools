#ifndef _link_h_
#define _link_h_

#include "bool.h"
#include "bufferstore.h"
#include "bufferarray.h"
class packet;
class IOWatch;

class link {
public:
  link(const char *fname, int baud, IOWatch &iow, bool s5, bool _verbose = false);
  ~link();
  void send(const bufferStore &buff);
  bufferArray poll();
  bool stuffToSend();
  bool hasFailed();
  
private:
  packet *p;
  int idSent;
  int countToResend;
  int timesSent;
  bufferArray sendQueue;
  bufferStore toSend;
  int idLastGot;
  bool newLink;
  bool verbose;
  bool somethingToSend;
  bool failed;
  bool s5;
};

#endif
