
#include "psitime.h"
#include <stdlib.h>

PsiTime::PsiTime(psi_timeval *_ptv, psi_timezone *_ptz) {
	if (_ptv != 0L)
		ptv = *_ptv;
	if (_ptz != 0L) {
		ptz = *_ptz;
		ptzValid = true;
	} else
		ptzValid = false;
	/* get our own timezone */
	gettimeofday(NULL, &utz);
	psi2unix();
}

PsiTime::~PsiTime() {
}

void PsiTime::setUnixTime(struct timeval *_utv) {
	if (_utv != 0L)
		utv = *_utv;
	unix2psi();
}

void PsiTime::setUnixNow(void) {
	gettimeofday(&utv, &utz);
	unix2psi();
}


void PsiTime::setPsiTime(psi_timeval *_ptv) {
	if (_ptv != 0L)
		ptv = *_ptv;
	psi2unix();
}

void PsiTime::setPsiZone(psi_timezone *_ptz) {
	if (_ptz != 0L) {
		ptz = *_ptz;
		ptzValid = true;
	}
	psi2unix();
}

struct timeval &PsiTime::getTimeval(void) {
	return utv;
}

time_t PsiTime::getTime(void) {
	return utv.tv_sec;
}

psi_timeval &PsiTime::getPsiTimeval(void) {
	return ptv;
}

ostream &operator<<(ostream &s, const PsiTime &t) {
	const char *fmt = "%c";
	char buf[100];
	strftime(buf, sizeof(buf), fmt, localtime(&t.utv.tv_sec));
	s << buf;
	return s;
}

/**
 * The difference between
 * EPOC epoch (01.01.0001 00:00:00)
 * and Unix epoch (01.01.1970 00:00:00)
 * in microseconds.
 */
#define EPOCH_DIFF 0x00dcddb30f2f8000ULL

static unsigned long long
evalOffset(psi_timezone ptz, bool valid) {
	unsigned long long offset = 0;

	if (valid) {
		offset = ptz.utc_offset;
		if ((ptz.dst_zones & 0x40000000) || (ptz.dst_zones & ptz.home_zone))
			offset += 3600;
	} else {
		const char *offstr = getenv("PSI_TZ");
		if (offstr != 0L) {
			char *err = 0L;
			offset = strtoul(offstr, &err, 0);
			if (err != 0L)
				offset = 0;
		}
	}
	offset *= 1000000;
	return offset;
}

void PsiTime::psi2unix(void) {
	unsigned long long micro = ptv.tv_high;
	micro = (micro << 32) | ptv.tv_low;

	/* Substract Psion's idea of UTC offset */
	micro -= evalOffset(ptz, ptzValid);
	micro -= EPOCH_DIFF;

	utv.tv_sec = micro / 1000000;
	utv.tv_usec = micro % 1000000;
}

void PsiTime::unix2psi(void) {
	unsigned long long micro = utv.tv_sec * 1000000 + utv.tv_usec;

	/* Add Psion's idea of UTC offset */
	micro += EPOCH_DIFF;
	micro += evalOffset(ptz, ptzValid);

	ptv.tv_low = micro & 0x0ffffffff;
	ptv.tv_high = (micro >> 32) & 0x0ffffffff;
}
