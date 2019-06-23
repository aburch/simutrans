/*
 * Eine Sorte Water die zu einer Haltestelle gehört
 *
 * Revised January 2001
 * Hj. Malthaner
 */

#include <stdio.h>

#include "../../simworld.h"
#include "../../display/simimg.h"

#include "../../descriptor/ground_desc.h"
#include "../../descriptor/way_desc.h"

#include "../../bauer/wegbauer.h"
#include "../../dataobj/translator.h"
#include "kanal.h"

const way_desc_t *kanal_t::default_kanal=NULL;



kanal_t::kanal_t(loadsave_t *file) :  weg_t()
{
	rdwr(file);
}



kanal_t::kanal_t() : weg_t()
{
	set_desc(default_kanal);
}



void kanal_t::rdwr(loadsave_t *file)
{
	weg_t::rdwr(file);

	if(file->is_version_less(87, 1)) {
		set_desc(default_kanal);
		return;
	}

	if(file->is_saving()) {
		const char *s = get_desc()->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));

		const way_desc_t *desc = way_builder_t::get_desc(bname);
		int old_max_speed = get_max_speed();
		if(desc==NULL) {
			desc = way_builder_t::get_desc(translator::compatibility_name(bname));
			if(desc==NULL) {
				desc = default_kanal;
				welt->add_missing_paks( bname, karte_t::MISSING_WAY );
			}
			dbg->warning("kanal_t::rdwr()", "Unknown channel %s replaced by %s (old_max_speed %i)", bname, desc->get_name(), old_max_speed );
		}
		set_desc(desc);
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
	}
}
