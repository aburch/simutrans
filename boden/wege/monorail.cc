#include "../../simtypes.h"
#include "../../simdebug.h"
#include "../grund.h"
#include "../../bauer/wegbauer.h"
#include "../../besch/weg_besch.h"

#include "monorail.h"

const weg_besch_t *monorail_t::default_monorail=NULL;



monorail_t::monorail_t(karte_t *welt, loadsave_t *file) : schiene_t(welt)
{
	rdwr(file);
}



void monorail_t::rdwr(loadsave_t *file)
{
	schiene_t::rdwr(file);

	if(gib_besch()->gib_wtyp()!=monorail_wt) {
		int old_max_speed = gib_max_speed();
		const weg_besch_t *besch = wegbauer_t::weg_search( monorail_wt, (old_max_speed>0 ? old_max_speed : 120), 0, (weg_t::system_type)((gib_besch()->gib_styp()==weg_t::type_elevated)*weg_t::type_elevated) );
		dbg->warning("monorail_t::rwdr()", "Unknown way replaced by monorail %s (old_max_speed %i)", besch->gib_name(), old_max_speed );
		setze_besch(besch);
		if(old_max_speed>0) {
			setze_max_speed(old_max_speed);
		}
	}
}
