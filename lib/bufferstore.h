#ifndef _bufferstore_h
#define _bufferstore_h

#include "bool.h"
class ostream;

class bufferStore {
public:
  bufferStore();
  bufferStore(const unsigned char*buff, long len);
  ~bufferStore();
  bufferStore(const bufferStore &a);
  void operator =(const bufferStore &a);

  // Reading Utils
  unsigned long getLen() const;
  unsigned char getByte(long pos) const;
  unsigned int getWord(long pos) const;
  unsigned int getDWord(long pos) const;
  const char* getString(long pos=0) const;
  void discardFirstBytes(int n);
  friend ostream &operator<<(ostream &s, const bufferStore &m);
  bool empty() const;

  // Writing utils
  void init();
  void init(const unsigned char*buff, long len);
  void addByte(unsigned char c);
  void addWord(int a);
  void addDWord(long a);
  void addString(const char* s);
  void addStringT(const char* s);
  void addBuff(const bufferStore &s, long maxLen=-1);
  
private:
  void checkAllocd(long newLen);

  long len;
  long lenAllocd;
  long start;
  unsigned char* buff;

  enum c { MIN_LEN = 300 };
};

inline bool bufferStore::empty() const {
  return (len-start) == 0;
}

#endif
