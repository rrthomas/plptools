#ifndef _PSITIME_H_
#define _PSITIME_H_

#include <sys/time.h>
#include <unistd.h>

#include <ostream.h>

/**
 * Holds a Psion time value.
 * Psion time values are 64 bit
 * integers describing the time
 * since 01.01.0001 in microseconds.
 */
typedef struct psi_timeval_t {
	/**
	 * Prints a psi_timeval in human readable format.
	 */
	friend ostream &operator<<(ostream &o, const psi_timeval_t &ptv) {
		ostream::fmtflags old = o.flags();
		unsigned long long micro = ptv.tv_high;
		micro = (micro << 32) | ptv.tv_low;
		micro /= 1000000;
		int s = micro % 60;
		micro /= 60;
		int m = micro % 60;
		micro /= 60;
		int h = micro % 24;
		micro /= 24;
		int d = micro % 365;
		micro /= 365;
		int y = micro;
		o << dec;
		if (y > 0)
			o << y << " year" << ((y > 1) ? "s " : " ");
		if (d > 0)
			o << d << " day" << ((d > 1) ? "s " : " ");
		if (h > 0)
			o << h << " hour" << ((h != 1) ? "s " : " ");
		if (m > 0)
			o << m << " minute" << ((m != 1) ? "s " : " ");
		o << s << " second" << ((s != 1) ? "s" : "");
		o.flags(old);
		return o;
	}
	/**
	 * The lower 32 bits
	 */
	unsigned long tv_low;
	/**
	 * The upper 32 bits
	 */
	unsigned long tv_high;
} psi_timeval;

/**
 * holds a Psion time zone description.
 */
typedef struct psi_timezone_t {
	friend ostream &operator<<(ostream &s, const psi_timezone_t &ptz) {
		ostream::fmtflags old = s.flags();
		int h = ptz.utc_offset / 3600;
		int m = ptz.utc_offset % 3600;
		s << "offs: " << dec << h << "h";
		if (m != 0)
			s << ", " << m << "m";
		s.flags(old);
		return s;
	}
	unsigned long utc_offset;
	unsigned long dst_zones;
	unsigned long home_zone;
} psi_timezone;

/**
 * Psion time related utility class.
 *
 * PsiTime provides easy access to the time format, used
 * when communicating with a Psion. Internally, the time
 * is always normalized to GMT. The time value can be set
 * and retrieved in both Unix and Psion formats. This
 * allows easy conversion between both formats.
 * NOTE: For proper conversion, the current timezone of
 * the Psion has to be set. For EPOC devices, the
 * timezone can be evaluated using
 * @ref rpcs::getMachineInfo. For SIBO devices,
 * unfortunately there is no known method of retrieving
 * this information. Therefore, if the timezone is
 * <em>not</em> set, a fallback using the environment
 * variable <em>PSI_TZ</em> is provided. Users should
 * set this variable to the offset of their time zone
 * in seconds. If <em>PSI_TZ</em> is net set, a second
 * fallback uses the local machine's setup, which assumes
 * that both Psion and local machine have the same
 * time zone and daylight settings.
 */
class PsiTime {
public:
	/**
	 * Contructs a new instance.
	 * 
	 * @param _utv A Unix time value for initialization.
	 * @param _utz A Unix timezone for initialization.
	 */
	PsiTime(struct timeval *_utv, struct timezone *_utz = 0L);

	/**
	 * Contructs a new instance.
	 * 
	 * @param time A Unix time value for initialization.
	 */
	PsiTime(time_t time);

	/**
	 * Contructs a new instance.
	 * 
	 * @param _ptv A Psion time value for initialization.
	 * @param _ptz A Psion timezone for initialization.
	 */
	PsiTime(psi_timeval *_ptv, psi_timezone *_ptz = 0L);

	/**
	 * Contructs a new instance.
	 * 
	 * @param _ptvHi The high 16 bits of a Psion time value for initialization.
	 * @param _ptvLo The low 16 bits of a Psion time value for initialization.
	 */
	PsiTime(const unsigned long _ptvHi, const unsigned long _ptvLo);

	/**
	 * Constructs a new instance, initializing to now.
	 */
	PsiTime(void);

	/**
	 * A copy-constructor
	 */
	PsiTime(const PsiTime &t);

	/**
	 * Destroys the instance.
	 */
	~PsiTime();

	/**
	 * Modifies the value of this instance.
	 *
	 * @param _ptv The new Psion time representation.
	 */
	void setPsiTime(psi_timeval *_ptv);

	/**
	 * Modifies the value of this instance.
	 *
	 * @param _ptvHi The high 32 bits of a Psion time.
	 * @param _ptvLo The low 32 bits of a Psion time.
	 */
	void setPsiTime(const unsigned long _ptvHi, const unsigned long _ptvLo);

	/**
	 * Sets the Psion time zone of this instance.
	 *
	 * @param _ptz The new Psion time zone.
	 */
	void setPsiZone(psi_timezone *_ptz);

	/**
	 * Sets the value of this instance.
	 *
	 * @param _utv The new Unix time representation.
	 */
	void setUnixTime(struct timeval *_utv);

	/**
	 * Sets the value of this instance.
	 *
	 * @param _utv The new Unix time representation.
	 */
	void setUnixTime(time_t time);

	/**
	 * Sets the value of this instance to the
	 * current time of the Unix machine.
	 */
	void setUnixNow(void);

	/**
	 * Retrieves the instance's current value
	 * in Unix time format.
	 *
	 * @returns The instance's current time as Unix struct timeval.
	 */
	struct timeval &getTimeval(void);

	/**
	 * Retrieves the instance's current value
	 * in Unix time format.
	 *
	 * @returns The instance's current time as Unix time_t.
	 */
	time_t getTime(void);

	/**
	 * Retrieves the instance's current value
	 * in Psion time format.
	 *
	 * @returns The instance's current time a Psion struct psi_timeval_t.
	 */
	psi_timeval &getPsiTimeval(void);

	/**
	 * Retrieves the instance's current value
	 * in Psion time format, high 32 bits.
	 *
	 * @returns The instance's current time as lower 32 bits of a Psion struct psi_timeval_t.
	 */
	const unsigned long getPsiTimeLo(void);

	/**
	 * Retrieves the instance's current value
	 * in Psion time format, low 32 bits.
	 *
	 * @returns The instance's current time as upper 32 bits of a Psion struct psi_timeval_t.
	 */
	const unsigned long getPsiTimeHi(void);

	/**
	 * Prints the instance's value in human readable format.
	 * This function uses the current locale setting for
	 * formatting the time.
	 *
	 * @param s The stream to be written.
	 * @param t The instance whose value should be displayed.
	 *
	 * @returns The stream.
	 */
	friend ostream &operator<<(ostream &s, const PsiTime &t);

	/**
	 * Assignment operator
	 */
	PsiTime &operator=(const PsiTime &t);

	enum zone {
		PSI_TZ_NONE = 0,
		PSI_TZ_EUROPEAN = 1,
		PSI_TZ_NORTHERN = 2,
		PSI_TZ_SOUTHERN = 4,
		PSI_TZ_HOME = 0x40000000,
	};

private:
	void psi2unix(void);
	void unix2psi(void);

	psi_timeval ptv;
	psi_timezone ptz;
	struct timeval utv;
	struct timezone utz;
	bool ptzValid;
};
#endif
