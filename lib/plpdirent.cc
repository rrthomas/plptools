#include <plpdirent.h>
#include <stream.h>
#include <iomanip>

PlpDirent::PlpDirent(const PlpDirent &e) {
	size    = e.size;
	attr    = e.attr;
	time    = e.time;
	memcpy(uid, e.uid, sizeof(uid));
	name    = e.name;
	attrstr = e.attrstr;
}

long PlpDirent::
getSize() {
	return size;
}

long PlpDirent::
getAttr() {
	return attr;
}

long PlpDirent::
getUID(int uididx) {
	if ((uididx >= 0) && (uididx < 4))
		return uid[uididx];
	return 0;
}

const char *PlpDirent::
getName() {
	return name.c_str();
}

PsiTime PlpDirent::
getPsiTime() {
	return time;
}

void PlpDirent::
setName(const char *str) {
	name = str;
}

PlpDirent &PlpDirent::
operator=(const PlpDirent &e) {
	size    = e.size;
	attr    = e.attr;
	time    = e.time;
	memcpy(uid, e.uid, sizeof(uid));
	name    = e.name;
	attrstr = e.attrstr;
	return *this;
}

ostream &
operator<<(ostream &o, const PlpDirent &e) {
	ostream::fmtflags old = o.flags();

	o << e.attrstr << " " << dec << setw(10)
	  << setfill(' ') << e.size << " " << e.time
	  << " " << e.name;
	o.flags(old);
	return o;
}
