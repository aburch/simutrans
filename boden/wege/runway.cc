/*
 * runwayn für Simutrans
 *
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "../../simdebug.h"
#include "../../simworld.h"
#include "../grund.h"
#include "../../dataobj/loadsave.h"
#include "../../utils/cbuffer_t.h"
#include "../../bauer/wegbauer.h"
#include "../../besch/weg_besch.h"

#include "runway.h"

const weg_besch_t *runway_t::default_runway=NULL;


runway_t::runway_t(karte_t *welt) : weg_t(welt)
{
	setze_besch(default_runway);
}


runway_t::runway_t(karte_t *welt, loadsave_t *file) : weg_t(welt)
{
	rdwr(file);
}



void
runway_t::rdwr(loadsave_t *file)
{
	weg_t::rdwr(file);

	if(file->is_saving()) {
		const char *s = gib_besch()->gib_name();
		file->rdwr_str(s, "\n");
	}
	else {
		char bname[128];
		file->rd_str_into(bname, "\n");
		const weg_besch_t *besch = wegbauer_t::gib_besch(bname);
		if(besch==NULL) {
			int old_max_speed=gib_max_speed();
			besch = wegbauer_t::weg_search(air_wt,old_max_speed>0 ? old_max_speed : 20, 0, (weg_t::system_type)(old_max_speed>250) );
			dbg->warning("strasse_t::rwdr()", "Unknown channel %s replaced by a channel %s (old_max_speed %i)", bname, besch->gib_name(), old_max_speed );
		}
		setze_besch(besch);
	}
}
