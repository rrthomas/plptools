#ifndef _bufferarray_h
#define _bufferarray_h

#include "bool.h"
class bufferStore;

class bufferArray {
	public:
		bufferArray();
		bufferArray(const bufferArray &a);
		~bufferArray();
		bufferArray &operator =(const bufferArray &a);
		bool empty() const;

		// this is NOT a real push as with a FIFO but
		// appends the bufferStore.
		void pushBuffer(const bufferStore& b);
		bufferStore popBuffer(void);

		// new API (push() now behaves like a FIFO, more operators
		bufferStore &operator [](const unsigned long index);
		bufferArray &operator +(const bufferStore &);  // append
		bufferArray &operator +(const bufferArray &);  // append
		bufferArray &operator +=(const bufferStore &b); // append
		bufferStore pop(void);
		void push(const bufferStore& b);
		void append(const bufferStore& b);
		long length(void);
		void clear(void);

private:
		static const long ALLOC_MIN = 5;
		long len;
		long lenAllocd;
		bufferStore* buff;
};

inline bool bufferArray::empty() const { return len == 0; }

#endif
