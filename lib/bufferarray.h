#ifndef _bufferarray_h
#define _bufferarray_h

#include "bool.h"
class bufferStore;

class bufferArray {
public:
  bufferArray();
  bufferArray(const bufferArray &a);
  ~bufferArray();
  void operator =(const bufferArray &a);
  
  bool empty() const;
  bufferStore popBuffer();
  void pushBuffer(const bufferStore& b);
private:
  long len;
  long lenAllocd;
  bufferStore* buff;
};

inline bool bufferArray::empty() const { return len == 0; }

#endif
