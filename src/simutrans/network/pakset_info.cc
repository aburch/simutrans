/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "pakset_info.h"
#include "../simdebug.h"
#include "../dataobj/translator.h"
#include "../tpl/vector_tpl.h"
#include "../utils/simstring.h"

stringhashtable_tpl<checksum_t*> pakset_info_t::info;
checksum_t pakset_info_t::general;

void pakset_info_t::append(const char* name, obj_type type, checksum_t *chk)
{
	chk->finish();

	char objname[256] = { char(type), char(type>>8), char(type>>16), 0 };
	tstrncpy( objname+3, name, 252 );

	checksum_t *old = info.set( strdup(objname), chk );
	delete old;
}


void pakset_info_t::debug()
{
#if MSG_LEVEL >= 4
	for(auto const& i : info) {
		DBG_DEBUG4("pakset_info_t::debug", "%.30s -> sha1 = %s", i.key, i.value->get_str());
	}
#endif
}


/**
 * order all pak checksums by name
 */
struct entry_t {
	entry_t(const char* n=NULL, const checksum_t* i=NULL) : name(n), chk(i) {}
	const char* name;
	const checksum_t* chk;
};

static bool entry_cmp(entry_t a, entry_t b)
{
	return strcmp(a.name, b.name) < 0;
}



void pakset_info_t::calculate_checksum()
{
	general.reset();

	// first sort all the desc's
	vector_tpl<entry_t> sorted(info.get_count());
	for(auto const& i : info) {
		sorted.insert_ordered(entry_t(i.key, i.value), entry_cmp);
	}
	// now loop
	for(entry_t const& i : sorted) {
		i.chk->calc_checksum(&general);
	}
	general.finish();
}
