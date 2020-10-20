/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../../simworld.h"
#include "../../bauer/wegbauer.h"
#include "../../descriptor/way_desc.h"
#include "../../dataobj/loadsave.h"

#include "runway.h"

const way_desc_t *runway_t::default_runway=NULL;


runway_t::runway_t() : schiene_t(air_wt)
{
	set_desc(default_runway);
}


runway_t::runway_t(loadsave_t *file) : schiene_t(air_wt)
{
	rdwr(file);
}


void runway_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "runway_t" );

	weg_t::rdwr(file);

	if(file->is_saving())
	{
		const char *s = get_desc()->get_name();
		file->rdwr_str(s);
		if(file->get_extended_version() >= 12)
		{
			if(!replacement_way)
			{
				s = "";
			}
			else
			{
				s = replacement_way->get_name();
			}
			file->rdwr_str(s);
		}
	}
	else
	{
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));
		const way_desc_t *desc = way_builder_t::get_desc(bname);

#ifndef SPECIAL_RESCUE_12_3
		const way_desc_t* loaded_replacement_way = NULL;
		if(file->get_extended_version() >= 12)
		{
			char rbname[128];
			file->rdwr_str(rbname, lengthof(rbname));
			loaded_replacement_way = way_builder_t::get_desc(rbname);
		}
#endif

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

		set_desc(desc, file->get_extended_version() >= 12);
#ifndef SPECIAL_RESCUE_12_3
		if(file->get_extended_version() >= 12)
		{
			replacement_way = loaded_replacement_way;
		}
#endif
	}

	if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 20)
	{
		uint16 reserved_index = reserved.get_id();
		file->rdwr_short(reserved_index);
		reserved.set_id(reserved_index);
	}
}
