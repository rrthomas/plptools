/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 2000-2002 Fritz Elfert <felfert@to.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "psitime.h"
#include <stdlib.h>
#include <plp_inttypes.h>

#define OnePM 3600 // 13:00 offset for SIBO

PsiTime::PsiTime(void) {
    ptzValid = false;
    tryPsiZone();
    setUnixNow();
}

PsiTime::PsiTime(time_t time) {
    ptzValid = false;
    gettimeofday(NULL, &utz);
    setUnixTime(time);
}

PsiTime::PsiTime(psi_timeval *_ptv, psi_timezone *_ptz) {
    if (_ptv != 0L)
	ptv = *_ptv;
    if (_ptz != 0L) {
	ptz = *_ptz;
	ptzValid = true;
    } else {
	ptzValid = false;
	tryPsiZone();
    }
    /* get our own timezone */
    gettimeofday(NULL, &utz);
    psi2unix();
}

PsiTime::PsiTime(const u_int32_t _ptvHi, const u_int32_t _ptvLo) {
    ptv.tv_high = _ptvHi;
    ptv.tv_low = _ptvLo;
    ptzValid = false;
    tryPsiZone();
    /* get our own timezone */
    gettimeofday(NULL, &utz);
    psi2unix();
}

PsiTime::PsiTime(struct timeval *_utv = 0L, struct timezone *_utz = 0L) {
    if (_utv != 0L)
	utv = *_utv;
    if (_utz != 0L)
	utz = *_utz;
    tryPsiZone();
    unix2psi();
}

PsiTime::PsiTime(const PsiTime &t) {
    utv = t.utv;
    utz = t.utz;
    ptv = t.ptv;
    ptz = t.ptz;
    ptzValid = t.ptzValid;
    tryPsiZone();
}

PsiTime::~PsiTime() {
    tryPsiZone();
}

void PsiTime::setUnixTime(struct timeval *_utv) {
    if (_utv != 0L)
	utv = *_utv;
    unix2psi();
}

void PsiTime::setUnixTime(time_t time) {
    utv.tv_sec = time;
    utv.tv_usec = 0;
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

void PsiTime::setPsiTime(const u_int32_t _ptvHi, const u_int32_t _ptvLo) {
    ptv.tv_high = _ptvHi;
    ptv.tv_low = _ptvLo;
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

const u_int32_t PsiTime::getPsiTimeLo(void) {
    return ptv.tv_low;
}

const u_int32_t PsiTime::getPsiTimeHi(void) {
    return ptv.tv_high;
}

PsiTime &PsiTime::operator=(const PsiTime &t) {
    utv = t.utv;
    utz = t.utz;
    ptv = t.ptv;
    ptz = t.ptz;
    ptzValid = t.ptzValid;
    tryPsiZone();
    return *this;
}

bool PsiTime::operator==(const PsiTime &t) {
    psi2unix();
    return ((utv.tv_sec == t.utv.tv_sec) &&
	    (utv.tv_usec == t.utv.tv_usec));
}

bool PsiTime::operator<(const PsiTime &t) {
    psi2unix();
    if (utv.tv_sec == t.utv.tv_sec)
	return (utv.tv_usec < t.utv.tv_usec);
    else
	return (utv.tv_sec < t.utv.tv_sec);
}

bool PsiTime::operator>(const PsiTime &t) {
    psi2unix();
    if (utv.tv_sec == t.utv.tv_sec)
	return (utv.tv_usec > t.utv.tv_usec);
    else
	return (utv.tv_sec > t.utv.tv_sec);
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
evalOffset(psi_timezone ptz, time_t time, bool valid) {
    s_int64_t offset = 0;

    if (valid) {
	offset = ptz.utc_offset;
	if (!(ptz.dst_zones & 0x40000000) || (ptz.dst_zones & ptz.home_zone))
	    offset -= 3600;
    } else {
	/**
	* Fallback. If no Psion zone given, use
	* environment variable PSI_TZ
	*/
	const char *offstr = getenv("PSI_TZ");
	if (offstr != 0L) {
	    char *err = 0L;
	    offset = strtoul(offstr, &err, 0);
	    if (err != 0L && *err != '\0')
		offset = 0;
	} else {
	    /**
	    * Fallback. If PSI_TZ is not set,
	    * use the local timezone. This assumes,
	    * that both Psion and local machine are
	    * configured for the same timezone and
	    * daylight saving.
	    */
	    struct tm *tm = localtime(&time);
	    offset = timezone;
	    if (tm->tm_isdst)
		offset += 3600;
	}
    }
    // Substract out local timezone, it gets added
    // later
    time_t now = ::time(0);
    struct tm *now_tm = localtime(&now);
    offset -= timezone;

    offset *= 1000000;
    return offset;
}

void PsiTime::setSiboTime(u_int32_t stime) {
    unsigned long long micro = evalOffset(ptz, time(0), false);

    micro /= 1000000;
    utv.tv_sec = stime + OnePM - micro;
    utv.tv_usec = 0;
//    unix2psi();
}

u_int32_t PsiTime::getSiboTime(void) {
    unsigned long long micro = evalOffset(ptz, time(0), false);

    micro /= 1000000;
    return utv.tv_sec - OnePM + micro;
}

void PsiTime::psi2unix(void) {
    u_int64_t micro = ptv.tv_high;
    micro = (micro << 32) | ptv.tv_low;

    /* Substract Psion's idea of UTC offset */
    micro -= EPOCH_DIFF;
    micro -= evalOffset(ptz, micro / 1000000, ptzValid);

    utv.tv_sec = micro / 1000000;
    utv.tv_usec = micro % 1000000;
}

void PsiTime::unix2psi(void) {
    u_int64_t micro = (u_int64_t)utv.tv_sec * 1000000ULL + utv.tv_usec;

    /* Add Psion's idea of UTC offset */
    micro += evalOffset(ptz, utv.tv_sec, ptzValid);
    micro += EPOCH_DIFF;

    ptv.tv_low = micro & 0x0ffffffff;
    ptv.tv_high = (micro >> 32) & 0x0ffffffff;
}

void PsiTime::tryPsiZone() {
    if (ptzValid)
	return;
    if (PsiZone::getInstance().getZone(ptz))
	ptzValid = true;
}

PsiZone *PsiZone::_instance = 0L;

PsiZone &PsiZone::
getInstance() {
    if (_instance == 0L)
	_instance = new PsiZone();
    return *_instance;
}

PsiZone::PsiZone() {
    _ptzValid = false;
}

void PsiZone::
setZone(psi_timezone &ptz) {
    _ptz = ptz;
    _ptzValid = true;
}

bool PsiZone::
getZone(psi_timezone &ptz) {
    if (_ptzValid)
	ptz = _ptz;
    return _ptzValid;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
