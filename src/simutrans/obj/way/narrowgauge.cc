/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../../builder/wegbauer.h"
#include "../../descriptor/way_desc.h"

#include "narrowgauge.h"

const way_desc_t *narrowgauge_t::default_narrowgauge=NULL;



narrowgauge_t::narrowgauge_t(loadsave_t *file) : schiene_t()
{
	rdwr(file);
}


void narrowgauge_t::rdwr(loadsave_t *file)
{
	schiene_t::rdwr(file);

	if(get_desc()->get_wtyp()!=narrowgauge_wt) {
		int old_max_speed = get_max_speed();
		const way_desc_t *desc = way_builder_t::weg_search( narrowgauge_wt, (old_max_speed>0 ? old_max_speed : 120), 0, (systemtype_t)((get_desc()->get_styp()==type_elevated)*type_elevated) );
		if (desc==NULL) {
			dbg->fatal("narrowgauge_t::rwdr()", "No narrowgauge way available");
		}
		dbg->warning("narrowgauge_t::rwdr()", "Unknown way replaced by narrow gauge %s (old_max_speed %i)", desc->get_name(), old_max_speed );
		set_desc(desc);
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
	}
}
