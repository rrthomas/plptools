/*--*-c++-*-------------------------------------------------------------
 *  $Id$
 *---------------------------------------------------------------------*/

#include "Enum.h"

void EnumBase::i2sMapper::add(long i, const char* s) {
	stringMap.insert(pair<long, const char* const>(i, s));
}

string EnumBase::i2sMapper::lookup (long i) const {
	i2s_map_t::const_iterator searchPtr = stringMap.find(i);

	if (searchPtr == stringMap.end())
		return "[OUT-OF-RANGE]";
	/*
	 * now combine the probably the multiple strings belonging to this
	 * integer
	 */
	string result;
	for (i = stringMap.count(i); i > 0 ; --i, ++searchPtr) {
		// this should be the case:
		assert(searchPtr != stringMap.end());
		if (result.length() != 0) 
			result += string(",");
		result += string(searchPtr->second);
	}
	return result;
}


long EnumBase::i2sMapper::lookup (const char *s) const {
	/*
	 * lookup a specific string.
	 * Since speed does not matter, we just do an exhaustive
	 * search.
	 * Otherwise we would have to maintain another map
	 * mapping strings to ints .. but its not worth the memory
	 */
	i2s_map_t::const_iterator run = stringMap.begin();
	while (run != stringMap.end() && strcmp(s, run->second)) {
		++run;
	}
	if (run == stringMap.end())
		return  -1;    // FIXME .. maybe throw an exception ?
	return run->first;
}

bool EnumBase::i2sMapper::inRange (long i) const {
	return (stringMap.find(i) != stringMap.end());
}

/* 
 * Local variables:
 * c-basic-offset: 8
 * End:
 */
