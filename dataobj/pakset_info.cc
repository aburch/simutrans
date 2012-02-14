#include "pakset_info.h"
#include "../simdebug.h"
#include "../dataobj/translator.h"
#include "../tpl/vector_tpl.h"

stringhashtable_tpl<checksum_t*> pakset_info_t::info;
checksum_t pakset_info_t::general;

void pakset_info_t::append(const char* name, checksum_t *chk)
{
	chk->finish();

	checksum_t *old = info.set(name, chk);
	if (old) {
		delete old;
	}
}


void pakset_info_t::debug()
{
	FOR(stringhashtable_tpl<checksum_t*>, const& i, info) {
		dbg->message("pakset_info_t::debug", "%.30s -> sha1 = %s", i.key, i.value->get_str());
	}
}


/**
 * order all pak checksums by name
 */
struct entry_t {
	entry_t(const char* n=NULL, const checksum_t* i=NULL) : name(n), chk(i) {}
	const char* name;
	const checksum_t* chk;
};

bool entry_cmp(entry_t a, entry_t b)
{
	return strcmp(a.name, b.name) < 0;
}



void pakset_info_t::calculate_checksum()
{
	general.reset();

	// first sort all the besch's
	vector_tpl<entry_t> sorted(info.get_count());
	FOR(stringhashtable_tpl<checksum_t*>, const& i, info) {
		sorted.insert_ordered(entry_t(i.key, i.value), entry_cmp);
	}
	// now loop
	FOR(vector_tpl<entry_t>, const& i, sorted) {
		i.chk->calc_checksum(&general);
	}
	general.finish();
}
