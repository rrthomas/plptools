#ifndef _bufferarray_h
#define _bufferarray_h

#include "bool.h"
class bufferStore;

/**
 * An array of bufferStores
 */
class bufferArray {
public:
	/**
	 * constructs a new bufferArray.
	 * A minimum of @ref ALLOC_MIN
	 * elements is allocated.
	 */
	bufferArray();

	/**
	 * Constructs a new bufferArray.
	 *
	 * @param a The initial contents for this array.
	 */
	bufferArray(const bufferArray &a);

	/**
	 * Destroys the bufferArray.
	 */
	~bufferArray();

	/**
	 * Copys the bufferArray.
	 */
	bufferArray &operator =(const bufferArray &a);

	/**
	 * Checks if this bufferArray is empty.
	 *
	 * @return true if the bufferArray is empty.
	 */
	bool empty() const;

	/**
	 * Retrieves the bufferStore at given index.
	 *
	 * @return The bufferStore at index.
	 */
	bufferStore &operator [](const unsigned long index);

	/**
	 * Appends a bufferStore to a bufferArray.
	 *
	 * @param s The bufferStore to be appended.
	 *
	 * @returns A new bufferArray with bufferStore appended to.
	 */
	bufferArray operator +(const bufferStore &s);

	/**
	 * Concatenates two bufferArrays.
	 *
	 * @param a The bufferArray to be appended.
	 *
	 * @returns A new bufferArray consisting with a appended.
	 */
	bufferArray operator +(const bufferArray &a);

	/**
	 * Appends a bufferStore to current instance.
	 *
	 * @param s The bufferStore to append.
	 *
	 * @returns A reference to the current instance with s appended.
	 */
	bufferArray &operator +=(const bufferStore &s);

	/**
	 * Appends a bufferArray to current instance.
	 *
	 * @param a The bufferArray to append.
	 *
	 * @returns A reference to the current instance with a appended.
	 */
	bufferArray &operator +=(const bufferArray &a);

	/**
	 * Removes the first bufferStore.
	 *
	 * @return The removed bufferStore.
	 */
	bufferStore pop(void);

	/**
	 * Inserts a bufferStore at index 0.
	 *
	 * @param b The bufferStore to be inserted.
	 */
	void push(const bufferStore& b);

	/**
	 * Appends a bufferStore.
	 *
	 * @param b The bufferStore to be appended.
	 */
	void append(const bufferStore& b);

	/**
	 * Evaluates the current length.
	 *
	 * @return The current number of bufferStores
	 */
	long length(void);

	/**
	 * Empties the bufferArray.
	 */
	void clear(void);

private:
	/**
	 * Minimum number of bufferStores to
	 * allocate.
	 */
	static const long ALLOC_MIN = 5;

	/**
	 * The current number of bufferStores in
	 * this bufferArray.
	 */
	long len;

	/**
	 * The current number of bufferStores
	 * allocated.
	 */
	long lenAllocd;

	/**
	 * The content.
	 */
	bufferStore* buff;
};

inline bool bufferArray::empty() const { return len == 0; }

#endif
