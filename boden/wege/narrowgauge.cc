#include "../../bauer/wegbauer.h"
#include "../../besch/weg_besch.h"

#include "narrowgauge.h"

const weg_besch_t *narrowgauge_t::default_narrowgauge=NULL;



narrowgauge_t::narrowgauge_t(loadsave_t *file) : schiene_t()
{
	rdwr(file);
}


void narrowgauge_t::rdwr(loadsave_t *file)
{
	schiene_t::rdwr(file);

	if(get_besch()->get_wtyp()!=narrowgauge_wt) {
		int old_max_speed = get_max_speed();
		const weg_besch_t *besch = wegbauer_t::weg_search( narrowgauge_wt, (old_max_speed>0 ? old_max_speed : 120), 0, (weg_t::system_type)((get_besch()->get_styp()==weg_t::type_elevated)*weg_t::type_elevated) );
		if (besch==NULL) {
			dbg->fatal("narrowgauge_t::rwdr()", "No narrowgauge way available");
		}
		dbg->warning("narrowgauge_t::rwdr()", "Unknown way replaced by narrow gauge %s (old_max_speed %i)", besch->get_name(), old_max_speed );
		set_besch(besch);
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
	}
}
