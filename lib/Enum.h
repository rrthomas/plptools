/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 2000 Henner Zeller <hzeller@to.com>
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
#ifndef _ENUM_H_
#define _ENUM_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_LIBINTL_H
#include <intl.h>
#endif

#include <map>
#include <string>

/**
 * the Base for the Enum template.
 * currently, only purpose is to provide a class type for mapping
 * integer enum values to strings in the Enumeration class.
 * The mapping is done basically with a STL-multimap.
 *
 * @author Henner Zeller
 */
class EnumBase {
protected:
    /**
    * maps integers (typically: enumeration values) to
    * Strings. Takes care of the fact, that an Integer may map
    * to multiple strings (sometimes multiple enumeration values
    * represent the same integer).
    *
    * Provides a means to get the string representation of an
    * integer and vice versa.
    *
    * @author Henner Zeller
    */
    class i2sMapper {
    private:
	/**
	* there can be one value, mapping to multiple
	* strings. Therefore, we need a multimap.
	*/
	typedef multimap<long, const char*> i2s_map_t;

	/**
	* just for the record. Mapping back a string to the
	* Integer value in question. Since Symbols must be unique,
	* there is only a 1:1 relation as opposed to i2s_map_t. So
	* we can use a normal map here.
	*
	* Since in the usual application, mapping a string back
	* to its value is not important performance wise (typically
	* in a frontend), so it is implemented as exhaustive search,
	* not as extra map. Saves some bits of memrory ..
	*/
	//typedef map<const char*, long> s2i_map_t;

	i2s_map_t stringMap;
    public:
	/**
	* adds a new int -> string mapping
	* Does NOT take over responsibility for the
	* pointer (i.e. it is not freed), so it is save
	* to add constant strings provided in the program code.
	*/
	void add(long, const char*);

	/**
	* returns the string representation for this integer.
	* If there are multiple strings for this integer,
	* return a comma delimited list.
	*/
	string lookup(long) const;

	/**
	* returns the integer associated with the
	* given string or -1 if the value
	* is not found (XXX: this should throw
	* an exception).
	*/
	long lookup (const char *) const;

	/**
	* returns true, if we have an representation for
	* the given integer.
	*/
	bool inRange(long) const;
    };
};

/**
 * Wrapper class featuring range-checking and string
 * representation of enumerated values.
 *
 * The string representation capability is needed to provide a
 * generic input frontend for any Enumeration because text labels
 * are needed in GUIs, and, of course, aids debugging, because you
 * can provide a readable presentation of an entry if something
 * goes wrong.
 *
 * NOTE, that wrapping an enumeration with this class
 * does not mean any performance overhead at all since the
 * compiler optimizes the member calls away. Nor does an instance of
 * this class use more memory than the use of an usual Enumeration.
 * (i.e. sizeof(E) == sizeof(Enum<E>)).
 *
 * Besides that, it provides a great opportunity to check the
 * code and make it bug free, esp. if you've to read the
 * Enumeration from some dubious integer source
 * (.. to make the dubious integer source bug free ;-)
 *
 * So there is no reason, <b>not</b> to use this class.
 * Alas, you've to provide a StringRepresentation for it, which may
 * be tedious for large enumerations. To make the Definition easier
 * and more readable, an ENUM_DEFINITION() macro is provided.
 *
 * @see #ENUM_DEFINITION
 * @author Henner Zeller
 */
template<typename E>
class Enum : private EnumBase {
private:
    struct sdata {
	/**
	 * The constructor of the static data part.
	 * You've to provide a constructor for each Enumeration
	 * you want to wrap with this class. Initializes
	 * the string Representation map, the readable name
	 * of this Enumeration and a default value.
	 *
	 * The constructor is called automatically on definition,
	 * so this makes sure, that the static part is initialized
	 * properly before the program starts.
	 */
	sdata();
	i2sMapper stringRep;
	string    name;
	E         defaultValue;
    };
    static sdata staticData;

    /**
    * The actual value hold by this instance
    */
    E value;

public:
    /**
    * default constructor.
    * Initialize with default value.
    */
    Enum() : value(staticData.defaultValue) {}

    /**
    * initialize with Enumeration given.
    */
    Enum(E init) : value(init){
	// if this hits you and you're sure, that the
	// value is right .. is this Enum proper
	// initialized in the Enum<E>::sdata::sdata() ?
	assert(inRange(init));
    }

    /**
    * initialize with the string representation
    * XXX: throw Exception if not found ?
    */
    Enum(const string& s) : value(getValueFor(s)) {
	assert(inRange(value));
    }

    /**
    * assign an Enumeration of this type. In debug
    * version, assert, that it is really in the Range of
    * this Enumeration.
    */
    inline Enum& operator = (E setval) {
	value = setval;
	assert(inRange(setval));
	return *this;
    }

    /**
    * returns the enumeration value hold with this
    * enum.
    */
    inline operator E () const { return value; }

    /**
    * returns the String representation for the value
    * represented by this instance.
    */
    string toString() const { return getStringFor(value); }

    /**
    * returns the C string representation for the value
    * represented by this instance.
    */
    operator const char *() const { return toString().c_str(); }

    /**
    * This static member returns true, if the integer value
    * given fits int the range of this Enumeration. Use this
    * to verify input/output.
    * Fitting in the range of Enumeration here means, that
    * there actually exists a String representation for it,
    * so this Enumeration is needed to be initialized properly
    * in its Enum<E>::sdata::sdata() constructor, you've to
    * provide. For convenience, use the ENUM_DEFINITION() macro
    * for this.
    */
    static bool inRange(long i) {
	return (staticData.stringRep.inRange(i));
    }

    /**
    * returns the Name for this enumeration. Useful for
    * error reporting.
    */
    static string getEnumName() { return staticData.name; }

    /**
    * gives the String represenatation of a specific
    * value of this Enumeration.
    */
    static string getStringFor(E e) {
	return staticData.stringRep.lookup((long) e);
    }

    /**
    * returns the Value for a specific String.
    * XXX: throw OutOfRangeException ?
    */
    static E getValueFor(const string &s) {
	return (E) staticData.stringRep.lookup(s.getCStr());
    }
};

/**
 * Helper macro to construct an enumeration wrapper Enum<E> for
 * a specific enum type.
 *
 * It defines the static variable holding the static
 * information and provides the head of its Constructor.
 * You only have to provide the string-mapping additions in the
 * constructor body. This macro behaves much like a function declaration,
 * i.e. you have to start the constructor with { ..
 *
 * usage example:
 * <pre>
 * // declaration of enumeration; somewhere
 * class rfsv {
 * [...]
 * 	enum PSI_ERROR_CODES { E_PSI_GEN_NONE, E_PSI_GEN_FAIL, E_PSI_GEN_ARG };
 * [...]
 * };
 *
 * // definition of the Enum<E> with the appropriate string representations
 * ENUM_DEFINITION(rfsv::PSI_ERROR_CODES,
 *	 	   rfsv::E_PSI_GEN_NONE) {
 *	stringRep.add(rfsv::E_PSI_GEN_NONE,	"no error");
 *	stringRep.add(rfsv::E_PSI_GEN_FAIL,	"general");
 *	stringRep.add(rfsv::E_PSI_GEN_ARG,	"bad argument");
 * }
 * </pre>
 *
 * @param EnumName  The fully qualified Name of the enum type to be wrapped
 * @param init      The fully qualified Name of the initialization
 *                  value.
 *
 * @author Henner Zeller
 */
#define ENUM_DEFINITION(EnumName, initWith)			\
/**								\
  * The definition of the static variable holding the static	\
  * data for this Enumeration wrapper.				\
  */								\
Enum<EnumName>::sdata Enum<EnumName>::staticData;		\
/**								\
  * actual definition of the constructor for the static data.	\
  * This is called implicitly by the definition above.		\
  */								\
Enum<EnumName>::sdata::sdata() :				\
name(#EnumName),defaultValue(initWith)

/**
 * Writes enumeration's string representation.
 */
template <typename E>
inline ostream& operator << (ostream& out, const Enum<E> &e) {
    return out << gettext(e.toString().c_str());
}

#endif /* _ENUM_H_ */

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
