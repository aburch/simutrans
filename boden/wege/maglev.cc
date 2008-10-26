#include "../../simtypes.h"
#include "../../simdebug.h"
#include "../grund.h"
#include "../../bauer/wegbauer.h"
#include "../../besch/weg_besch.h"

#include "maglev.h"

const weg_besch_t *maglev_t::default_maglev=NULL;



/**
 * File loading constructor.
 * @author prissi
 */
maglev_t::maglev_t(karte_t *welt, loadsave_t *file) : schiene_t(welt)
{
	rdwr(file);
}



void
maglev_t::rdwr(loadsave_t *file)
{
	schiene_t::rdwr(file);

	if(gib_besch()->gib_wtyp()!=maglev_wt) {
		int old_max_speed = gib_max_speed();
		const weg_besch_t *besch = wegbauer_t::weg_search( maglev_wt, (old_max_speed>0 ? old_max_speed : 120), 0, (weg_t::system_type)((gib_besch()->gib_styp()==weg_t::type_elevated)*weg_t::type_elevated) );
		dbg->warning("monorail_t::rwdr()", "Unknown way replaced by maglev %s (old_max_speed %i)", besch->gib_name(), old_max_speed );
		setze_besch(besch);
		if(old_max_speed>0) {
			setze_max_speed(old_max_speed);
		}
	}
}
