#ifndef _iowatch_h
#define _iowatch_h

#include "bool.h"

class IOWatch {
public:
  IOWatch();
  ~IOWatch();
  
  void addIO(int a);
  void remIO(int a);
  bool watch(long secs, long usecs);
private:

  enum consts { MAX_IO = 20 };
  int *io;
  int num;
};

#endif
