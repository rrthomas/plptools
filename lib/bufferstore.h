#ifndef _bufferstore_h
#define _bufferstore_h

#include "bool.h"
class ostream;

class bufferStore {
public:
  bufferStore();
  bufferStore(const unsigned char *, long);
  ~bufferStore();
  bufferStore(const bufferStore &);
  bufferStore &operator =(const bufferStore &);

  // Reading Utils
  unsigned long getLen() const;
  unsigned char getByte(long pos) const;
  unsigned int getWord(long pos) const;
  unsigned int getDWord(long pos) const;
  const char* getString(long pos=0) const;
  void discardFirstBytes(int);
  friend ostream &operator<<(ostream &, const bufferStore &);
  bool empty() const;

  // Writing utils
  void init();
  void init(const unsigned char*, long);
  void addByte(unsigned char);
  void addWord(int);
  void addDWord(long);
  void addString(const char*);
  void addStringT(const char*);
  void addBytes(const unsigned char*, int);
  void addBuff(const bufferStore &, long maxLen=-1);
  
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
