/*
 * Eine Sorte Wasser die zu einer Haltestelle gehört
 *
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "../../simworld.h"
#include "../../display/simimg.h"

#include "../../besch/grund_besch.h"
#include "../../besch/weg_besch.h"

#include "../../bauer/wegbauer.h"
#include "../../dataobj/translator.h"
#include "kanal.h"

const weg_besch_t *kanal_t::default_kanal=NULL;



kanal_t::kanal_t(loadsave_t *file) :  weg_t(water_wt)
{
	rdwr(file);
}



kanal_t::kanal_t() : weg_t (water_wt)
{
	set_besch(default_kanal);
}



void kanal_t::rdwr(loadsave_t *file)
{
	weg_t::rdwr(file);

	if(file->get_version() <= 87000) {
		set_besch(default_kanal);
		return;
	}

	if(file->is_saving()) 
	{
		const char *s = get_besch()->get_name();
		file->rdwr_str(s);
		if(file->get_experimental_version() >= 12)
		{
			s = replacement_way ? replacement_way->get_name() : ""; 
			file->rdwr_str(s);
		}
	}
	else
	{
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));
		const weg_besch_t *besch = wegbauer_t::get_besch(bname);


#ifndef SPECIAL_RESCUE_12_3
		const weg_besch_t* loaded_replacement_way = NULL;
		if(file->get_experimental_version() >= 12)
		{
			char rbname[128];
			file->rdwr_str(rbname, lengthof(rbname));
			loaded_replacement_way = wegbauer_t::get_besch(rbname);
		}
#endif

		const sint32 old_max_speed = get_max_speed();
		const uint32 old_max_axle_load = get_max_axle_load();
		const uint32 old_bridge_weight_limit = get_bridge_weight_limit();
		if(besch==NULL) {
			besch = wegbauer_t::get_besch(translator::compatibility_name(bname));
			if(besch==NULL) {
				besch = default_kanal;
				welt->add_missing_paks( bname, karte_t::MISSING_WAY );
			}
			dbg->warning("kanal_t::rdwr()", "Unknown channel %s replaced by %s (old_max_speed %i)", bname, besch->get_name(), old_max_speed );
		}

		set_besch(besch, file->get_experimental_version() >= 12);

#ifndef SPECIAL_RESCUE_12_3
		if(file->get_experimental_version() >= 12)
		{
			replacement_way = loaded_replacement_way;
		}
#endif
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
		if(old_max_axle_load > 0)
		{
			set_max_axle_load(old_max_axle_load);
		}
		if(old_bridge_weight_limit > 0)
		{
			set_bridge_weight_limit(old_bridge_weight_limit);
		}
	}
}
