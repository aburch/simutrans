/*
 * Eine Sorte Wasser die zu einer Haltestelle gehört
 *
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "../../simworld.h"
#include "../../display/simimg.h"

#include "../../besch/grund_besch.h"
#include "../../besch/weg_besch.h"

#include "../../bauer/wegbauer.h"
#include "../../dataobj/translator.h"
#include "kanal.h"

const weg_besch_t *kanal_t::default_kanal=NULL;



kanal_t::kanal_t(loadsave_t *file) :  weg_t()
{
	rdwr(file);
}



kanal_t::kanal_t() : weg_t()
{
	set_besch(default_kanal);
}



void kanal_t::rdwr(loadsave_t *file)
{
	weg_t::rdwr(file);

	if(file->get_version() <= 87000) {
		set_besch(default_kanal);
		return;
	}

	if(file->is_saving()) {
		const char *s = get_besch()->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));

		const weg_besch_t *besch = wegbauer_t::get_besch(bname);
		int old_max_speed = get_max_speed();
		if(besch==NULL) {
			besch = wegbauer_t::get_besch(translator::compatibility_name(bname));
			if(besch==NULL) {
				besch = default_kanal;
				welt->add_missing_paks( bname, karte_t::MISSING_WAY );
			}
			dbg->warning("kanal_t::rdwr()", "Unknown channel %s replaced by %s (old_max_speed %i)", bname, besch->get_name(), old_max_speed );
		}
		set_besch(besch);
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
	}
}
