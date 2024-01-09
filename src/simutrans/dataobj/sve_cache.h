/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_SVE_CACHE_H
#define DATAOBJ_SVE_CACHE_H


#include "../simtypes.h"
#include "../tpl/stringhashtable_tpl.h"

#include <string>


class loadsave_t;


class sve_info_t
{
public:
	std::string pak;
	sint64 mod_time;
	sint32 file_size;
	bool file_exists;

public:
	sve_info_t();
	sve_info_t(const char *pak_, time_t mod_, sint32 fs);

public:
	bool operator==(const sve_info_t &) const;
	void rdwr(loadsave_t *file);
};


class sve_cache_t
{
	static stringhashtable_tpl<sve_info_t *> cached_info;

public:
	static void load_cache();
	static void write_cache();

	// if the file is not already in the cache, it is added.
	static const char *get_info(const std::string &fname);

	static std::string get_most_recent_compatible_save();
};


#endif
