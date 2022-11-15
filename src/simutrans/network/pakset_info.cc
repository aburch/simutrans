/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "pakset_info.h"
#include "../simdebug.h"
#include "../dataobj/translator.h"
#include "../dataobj/pakset_manager.h"
#include "../tpl/vector_tpl.h"
#include "../utils/cbuffer.h"

stringhashtable_tpl<checksum_t*> pakset_info_t::info;
checksum_t pakset_info_t::general;

void pakset_info_t::append(const char* name, const char* type, checksum_t *chk)
{
	chk->finish();

	static cbuffer_t buf;
	buf.clear();
	buf.printf("%s:%s", type, name);

	checksum_t *old = info.set( strdup( (const char*)buf), chk );

	if (old) {
		if (*chk == *old) {
			dbg->warning("pakset_info_t::append", "Object %s::%s is overlaid by identical object, using data of new object", type, name);
		}
		else {
			dbg->warning("pakset_info_t::append", "Object %s::%s is overlaid by different object, using data of new object", type, name);
			pakset_manager_t::doubled(type, name);
		}
	}
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
