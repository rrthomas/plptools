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

PlpDrive::PlpDrive() {
}

PlpDrive::PlpDrive(const PlpDrive &other) {
}

void PlpDrive::
setMediaType(u_int32_t type) {
	mediatype = type;
}

void PlpDrive::
setDriveAttribute(u_int32_t attr) {
	driveattr = attr;
}

void PlpDrive::
setMediaAttribute(u_int32_t attr) {
	mediaattr = attr;
}

void PlpDrive::
setUID(u_int32_t attr) {
	uid = attr;
}

void PlpDrive::
setSize(u_int32_t sizeLo, u_int32_t sizeHi) {
	size = ((unsigned long long)sizeHi << 32) + sizeLo;
}

void PlpDrive::
setSpace(u_int32_t spaceLo, u_int32_t spaceHi) {
	space = ((unsigned long long)spaceHi << 32) + spaceLo;
}

void PlpDrive::
setName(char drive, const char * const volname) {
	drivechar = drive;
	name = "";
	name += volname;
}

u_int32_t PlpDrive::
getMediaType() {
	return mediatype;
}

static const char * const media_types[] = {
	"Not present",
	"Unknown",
	"Floppy",
	"Disk",
	"CD-ROM",
	"RAM",
	"Flash Disk",
	"ROM",
	"Remote",
};

void PlpDrive::
getMediaType(string &ret) {
	ret = media_types[mediatype];
}

u_int32_t PlpDrive::
getDriveAttribute() {
	return driveattr;
}

static void
appendWithDelim(string &s1, const char * const s2) {
	if (!s1.empty())
		s1 += ',';
	s1 += s2;
}

void PlpDrive::
getDriveAttribute(string &ret) {
	ret = "";
	if (driveattr & 1)
		appendWithDelim(ret, "local");
	if (driveattr & 2)
		appendWithDelim(ret, "ROM");
	if (driveattr & 4)
		appendWithDelim(ret, "redirected");
	if (driveattr & 8)
		appendWithDelim(ret, "substituted");
	if (driveattr & 16)
		appendWithDelim(ret, "internal");
	if (driveattr & 32)
		appendWithDelim(ret, "removable");
}

u_int32_t PlpDrive::
getMediaAttribute() {
	return mediaattr;
}

void PlpDrive::
getMediaAttribute(string &ret) {
	ret = "";

	if (mediaattr & 1)
		appendWithDelim(ret, "variable size");
	if (mediaattr & 2)
		appendWithDelim(ret, "dual density");
	if (mediaattr & 4)
		appendWithDelim(ret, "formattable");
	if (mediaattr & 8)
		appendWithDelim(ret, "write protected");
}

u_int32_t PlpDrive::
getUID() {
	return uid;
}

u_int64_t PlpDrive::
getSize() {
	return size;
}

u_int64_t PlpDrive::
getSpace() {
	return space;
}

string PlpDrive::
getName() {
	return name;
}

char PlpDrive::
getDrivechar() {
	return drivechar;
}
