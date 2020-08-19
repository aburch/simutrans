/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../../bauer/wegbauer.h"
#include "../../descriptor/way_desc.h"

#include "narrowgauge.h"

const way_desc_t *narrowgauge_t::default_narrowgauge=NULL;


narrowgauge_t::narrowgauge_t(loadsave_t *file) : schiene_t(narrowgauge_wt)
{
	rdwr(file);
}


void narrowgauge_t::rdwr(loadsave_t *file)
{
	schiene_t::rdwr(file);

	if(get_desc()->get_wtyp()!=narrowgauge_wt) {
		int old_max_speed = get_max_speed();
		int old_max_axle_load = get_max_axle_load();
		const way_desc_t *desc = way_builder_t::weg_search(narrowgauge_wt, (old_max_speed>0 ? old_max_speed : 120), (old_max_axle_load > 0 ? old_max_axle_load : 10), 0, (systemtype_t)((get_desc()->get_styp()==type_elevated)*type_elevated), 1);
		if (desc==NULL) {
			dbg->fatal("narrowgauge_t::rwdr()", "No narrowgauge way available");
		}
		dbg->warning("narrowgauge_t::rwdr()", "Unknown way replaced by narrow gauge %s (old_max_speed %i)", desc->get_name(), old_max_speed );
		set_desc(desc, file->get_extended_version() >= 12);
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
		if(old_max_axle_load > 0)
		{
			set_max_axle_load(old_max_axle_load);
		}
	}
}
