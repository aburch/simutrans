#include "../../simtypes.h"
#include "../../simdebug.h"
#include "../grund.h"
#include "../../bauer/wegbauer.h"
#include "../../besch/weg_besch.h"

#include "narrowgauge.h"

const weg_besch_t *narrowgauge_t::default_narrowgauge=NULL;



narrowgauge_t::narrowgauge_t(karte_t *welt, loadsave_t *file) : schiene_t(welt)
{
	rdwr(file);
}



void
narrowgauge_t::rdwr(loadsave_t *file)
{
	schiene_t::rdwr(file);

	if(gib_besch()->gib_wtyp()!=narrowgauge_wt) {
		int old_max_speed = gib_max_speed();
		const weg_besch_t *besch = wegbauer_t::weg_search( narrowgauge_wt, (old_max_speed>0 ? old_max_speed : 120), 0, (weg_t::system_type)((gib_besch()->gib_styp()==weg_t::type_elevated)*weg_t::type_elevated) );
		dbg->warning("narrowgauge_t::rwdr()", "Unknown way replaced by narrow gauge %s (old_max_speed %i)", besch->gib_name(), old_max_speed );
		setze_besch(besch);
		if(old_max_speed>0) {
			setze_max_speed(old_max_speed);
		}
	}
}
