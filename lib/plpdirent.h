#ifndef _PLP_DIRENT_H_
#define _PLP_DIRENT_H_

#include <string>
#include <psitime.h>
#include <rfsv.h>

/**
 * A class, representing a directory entry of the Psion.
 * Objects of this type are used by @ref rfsv::readdir and
 * @ref rfsv::dir for returning the entries of a directory.
 */
class PlpDirent {
	friend class rfsv32;
	friend class rfsv16;

public:
	/**
	 * Default constructor
	 */
	PlpDirent() : size(0), attr(0), name(""), time(0L), attrstr("") { };

	/**
	 * A copy constructor.
	 * Mainly used by STL container classes.
	 *
	 * @param d The object to be used as initializer.
	 */
	PlpDirent(const PlpDirent &d);

	/**
	 * Default destructor.
	 */
	~PlpDirent() {};

	/**
	 * Retrieves the file size of a directory entry.
	 *
	 * @returns The file size in bytes.
	 */
	long getSize();

	/**
	 * Retrieves the file attributes of a directory entry.
	 *
	 * @returns The generic attributes ( @ref rfsv:file_attribs ).
	 */
	long getAttr();

	/**
	 * Retrieves the UIDs of a directory entry.
	 * This method returns always 0 with a Series3.
	 *
	 * @param uididx The index of the UID to retrieve (0 .. 2).
	 *
	 * @returns The selected UID or 0 if the index is out of range.
	 */
	long getUID(int uididx);

	/**
	 * Retrieve the file name of a directory entry.
	 *
	 * @returns The name of the file.
	 */
	const char *getName();

	/**
	 * Retrieve the modification time of a directory entry.
	 *
	 * @returns A @ref PsiTime object, representing the time.
	 */
	PsiTime getPsiTime();

	/**
	 * Set the file name of a directory entry.
	 * This is currently used in plpbackup only for
	 * changing the name to the full path. It does NOT
	 * change the name of the corresponding file on
	 * the Psion.
	 *
	 * @param str The new name of the file.
	 */
	void setName(const char *str);

	/**
	 * Assignment opreator
	 * Mainly used by STL container classes.
	 *
	 * @param e The new value to assign.
	 *
	 * @returns The modified object.
	 */
	PlpDirent &operator=(const PlpDirent &e);

	/**
	 * Prints the object contents.
	 * The output is in human readable similar to the
	 * output of a "ls" command.
	 */
	friend ostream &operator<<(ostream &o, const PlpDirent &e);

private:
	long    size;
	long    attr;
	long    uid[3];
	PsiTime time;
	string  attrstr;
	string  name;
};
#endif
