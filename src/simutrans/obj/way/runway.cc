/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../../world/simworld.h"
#include "../../builder/wegbauer.h"
#include "../../descriptor/way_desc.h"
#include "../../dataobj/loadsave.h"
#include "../../dataobj/translator.h"
#include "../../utils/cbuffer.h"

#include "runway.h"

const way_desc_t *runway_t::default_runway=NULL;


runway_t::runway_t() : schiene_t()
{
	set_desc(default_runway);
}


runway_t::runway_t(loadsave_t *file) : schiene_t()
{
	rdwr(file);
}


void runway_t::info(cbuffer_t & buf) const
{
	schiene_t::info(buf);

	cbuffer_t reserved;
	if(  get_desc()->get_styp()==type_runway  ) {
		reserved.printf( "%s %i\n", translator::translate("waiting"), reservations.get_count() );
		buf.append( reserved );
	}
}


void runway_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "runway_t" );

	weg_t::rdwr(file);

	if(file->is_saving()) {
		const char *s = get_desc()->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));
		const way_desc_t *desc = way_builder_t::get_desc(bname);
		int old_max_speed=get_max_speed();
		if(desc==NULL) {
			desc = way_builder_t::weg_search(air_wt,old_max_speed>0 ? old_max_speed : 20, 0, (systemtype_t)(old_max_speed>250) );
			if(desc==NULL) {
				desc = default_runway;
				welt->add_missing_paks( bname, karte_t::MISSING_WAY );
			}
			dbg->warning("runway_t::rdwr()", "Unknown runway %s replaced by %s (old_max_speed %i)", bname, desc->get_name(), old_max_speed );
		}
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
		set_desc(desc);
	}
}
