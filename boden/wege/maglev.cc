#include "../../simtypes.h"
#include "../../bauer/wegbauer.h"
#include "../../descriptor/way_desc.h"

#include "maglev.h"

const way_desc_t *maglev_t::default_maglev=NULL;



/**
 * File loading constructor.
 * @author prissi
 */
maglev_t::maglev_t(loadsave_t *file) : schiene_t(maglev_wt)
{
	rdwr(file);
}


void maglev_t::rdwr(loadsave_t *file)
{
	schiene_t::rdwr(file);

	if(get_desc()->get_wtyp()!=maglev_wt) {
		int old_max_speed = get_max_speed();
		int old_max_axle_load = get_max_axle_load();
		const way_desc_t *desc = way_builder_t::weg_search( maglev_wt, (old_max_speed>0 ? old_max_speed : 120), (old_max_axle_load > 0 ? old_max_axle_load : 10), 0, (systemtype_t)((get_desc()->get_styp()==type_elevated)*type_elevated), 1);
		if (desc==NULL) {
			dbg->fatal("maglev_t::rwdr()", "No maglev way available");
		}
		dbg->warning("maglev_t::rwdr()", "Unknown way replaced by maglev %s (old_max_speed %i)", desc->get_name(), old_max_speed );
		set_desc(desc, file->get_extended_version() >= 12);
		if(old_max_speed > 0) {
			set_max_speed(old_max_speed);
		}
		if(old_max_axle_load > 0)
		{
			set_max_axle_load(old_max_axle_load);
		}
	}
}
