/*
 * Strassen für Simutrans
 *
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "strasse.h"
#include "../../simworld.h"
#include "../../obj/bruecke.h"
#include "../../obj/tunnel.h"
#include "../../dataobj/loadsave.h"
#include "../../besch/weg_besch.h"
#include "../../besch/tunnel_besch.h"
#include "../../bauer/wegbauer.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/ribi.h"
#include "../../utils/cbuffer_t.h"
#include "../../vehicle/simvehicle.h" /* for calc_direction */
#include "../../obj/wayobj.h"

const weg_besch_t *strasse_t::default_strasse=NULL;


void strasse_t::set_gehweg(bool janein)
{
	grund_t *gr = welt->lookup(get_pos());
	wayobj_t *wo = gr ? gr->get_wayobj(road_wt) : NULL;

	if (wo && wo->get_besch()->is_noise_barrier()) {
		janein = false;
	}

	weg_t::set_gehweg(janein);
	if(janein && get_besch() && get_besch()->get_topspeed() > welt->get_city_road()->get_topspeed()) 
	{
		set_max_speed(welt->get_city_road()->get_topspeed());
	}
	if(!janein && get_besch()) {
		set_max_speed(get_besch()->get_topspeed());
	}
	if(gr) {
		gr->calc_image();
	}
}



strasse_t::strasse_t(loadsave_t *file) : weg_t(road_wt)
{
	rdwr(file);
}


strasse_t::strasse_t() : weg_t(road_wt)
{
	set_gehweg(false);
	set_besch(default_strasse);
}



void strasse_t::rdwr(loadsave_t *file)
{
	xml_tag_t s( file, "strasse_t" );

	weg_t::rdwr(file);

	if(file->get_version()<89000) {
		bool gehweg;
		file->rdwr_bool(gehweg);
		set_gehweg(gehweg);
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
				besch = default_strasse;
				welt->add_missing_paks( bname, karte_t::MISSING_WAY );
			}
			dbg->warning("strasse_t::rdwr()", "Unknown street %s replaced by %s (old_max_speed %i)", bname, besch->get_name(), old_max_speed );
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
		const grund_t* gr = welt->lookup(get_pos());
		const bruecke_t *bridge = gr ? gr->find<bruecke_t>() : NULL;
		const tunnel_t *tunnel = gr ? gr->find<tunnel_t>() : NULL;
		const hang_t::typ hang = gr ? gr->get_weg_hang() : hang_t::flach;

		if(hang != hang_t::flach) 
		{
			const uint slope_height = (hang & 7) ? 1 : 2;
			if(slope_height == 1)
			{
				if(bridge)
				{
					set_max_speed(min(besch->get_topspeed_gradient_1(), bridge->get_besch()->get_topspeed_gradient_1()));
				}
				else if(tunnel)
				{
					set_max_speed(min(besch->get_topspeed_gradient_1(), tunnel->get_besch()->get_topspeed_gradient_1()));
				}
				else
				{
					set_max_speed(besch->get_topspeed_gradient_1());
				}
			}
			else
			{
				if(bridge)
				{
					set_max_speed( min(besch->get_topspeed_gradient_2(), bridge->get_besch()->get_topspeed_gradient_2()));
				}
				else if(tunnel)
				{
					set_max_speed(min(besch->get_topspeed_gradient_2(), tunnel->get_besch()->get_topspeed_gradient_2()));
				}
				else
				{
					set_max_speed(besch->get_topspeed_gradient_2());
				}
			}
		}
		else
		{
			if(bridge)
				{
					set_max_speed(min(besch->get_topspeed(), bridge->get_besch()->get_topspeed()));
				}
			else if(tunnel)
				{
					set_max_speed(min(besch->get_topspeed(), tunnel->get_besch()->get_topspeed()));
				}
				else
				{
					set_max_speed(besch->get_topspeed());
				}
		}
 
		if(hat_gehweg() && besch->get_wtyp() == road_wt)
		{
			set_max_speed(min(get_max_speed(), 50));
		}
	}
}
