#include <plpdirent.h>
#include <stream.h>
#include <iomanip>

PlpUID::PlpUID() {
	memset(uid, 0, sizeof(uid));
}

PlpUID::PlpUID(const long u1, const long u2, const long u3) {
	uid[0] = u1; uid[1] = u2; uid[2] = u3;
}

long PlpUID::
operator[](int idx) {
	assert ((idx > -1) && (idx < 3));
	return uid[idx];
}

PlpDirent::PlpDirent(const PlpDirent &e) {
	size    = e.size;
	attr    = e.attr;
	time    = e.time;
	UID     = e.UID;
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
		return UID[uididx];
	return 0;
}

PlpUID &PlpDirent::
getUID() {
	return UID;
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
	UID     = e.UID;
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
