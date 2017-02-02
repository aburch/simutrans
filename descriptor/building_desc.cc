/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */
#include "../simdebug.h"

#include "../simworld.h"

#include "building_desc.h"
#include "../network/checksum.h"



/*
 *  Author:
 *      Volker Meyer
 *
 *  Description:
 *      Rechnet aus dem Index das Layout aus, zu dem diese Tile gehört.
 */
uint8 building_tile_desc_t::get_layout() const
{
	koord size = get_desc()->get_size();
	return index / (size.x * size.y);
}



/*
 *  Author:
 *      Volker Meyer
 *
 *  Description:
 *      Bestimmt die Relativ-Position des Einzelbildes im Gesamtbild des
 *	Gebäudes.
 */
koord building_tile_desc_t::get_offset() const
{
	const building_desc_t *desc = get_desc();
	koord size = desc->get_size(get_layout());	// ggf. gedreht
	return koord( index % size.x, (index / size.x) % size.y );
}



waytype_t building_desc_t::get_finance_waytype() const
{
	switch( get_type() )
	{
		case dock:         return water_wt;
		case flat_dock:    return water_wt;
		case depot:
		case generic_stop:
		case generic_extension:
			return (waytype_t) get_extra();
		default: return ignore_wt;
	}
}



/**
 * Mail generation level
 * @author Hj. Malthaner
 */
uint16 building_desc_t::get_mail_level() const
{
	switch (type) {
		default:
		case city_res:   return level;
		case city_com:   return level * 2;
		case city_ind:   return level / 2;
	}
}



/**
 * true, if this building needs a connection with a town
 * @author prissi
 */
bool building_desc_t::is_connected_with_town() const
{
	switch(get_type()) {
		case city_res:
		case city_com:
		case city_ind:    // normal town buildings (RES, COM, IND)
		case monument:     // monuments
		case townhall:     // townhalls
		case headquarter:  // headquarter
			return true;
		default:
			return false;
	}
}



/*
 *  Author:
 *      Volker Meyer
 *
 *  Description:
 *      Abhängig von Position und Layout ein tile zurückliefern
 */
const building_tile_desc_t *building_desc_t::get_tile(uint8 layout, sint16 x, sint16 y) const
{
	layout = adjust_layout(layout);
	koord dims = get_size(layout);

	if(  x < 0  ||  y < 0  ||  layout >= layouts  ||  x >= get_x(layout)  ||  y >= get_y(layout)  ) {
	dbg->fatal("building_tile_desc_t::get_tile()",
			   "invalid request for l=%d, x=%d, y=%d on building %s (l=%d, x=%d, y=%d)",
		   layout, x, y, get_name(), layouts, size.x, size.y);
	}
	return get_tile(layout * dims.x * dims.y + y * dims.x + x);
}



/*
 *  Author:
 *      Volker Meyer
 *
 *  Description:
 *      Layout normalisieren.
 */
uint8 building_desc_t::adjust_layout(uint8 layout) const
{
	if(layout >= 4 && layouts <= 4) {
		layout -= 4;
	}
	if(layout >= 2 && layouts <= 2) {
		// Sind Layout C und D nicht definiert, nehemen wir ersatzweise A und B
		layout -= 2;
	}
	if(layout > 0 && layouts <= 1) {
		// Ist Layout B nicht definiert und das Teil quadratisch, nehmen wir ersatzweise A
		if(size.x == size.y) {
			layout--;
		}
	}
	return layout;
}


void building_desc_t::calc_checksum(checksum_t *chk) const
{
	obj_desc_timelined_t::calc_checksum(chk);
	chk->input((uint8)type);
	chk->input(animation_time);
	chk->input(extra_data);
	chk->input(size.x);
	chk->input(size.y);
	chk->input((uint8)flags);
	chk->input(level);
	chk->input(layouts);
	chk->input(enables);
	chk->input(chance);
	chk->input((uint8)allowed_climates);
	chk->input(maintenance);
	chk->input(price);
	chk->input(capacity);
	chk->input(allow_underground);
	// now check the layout
	for(uint8 i=0; i<layouts; i++) {
		sint16 b=get_x(i);
		for(sint16 x=0; x<b; x++) {
			sint16 h=get_y(i);
			for(sint16 y=0; y<h; y++) {
				if (get_tile(i,x,y)  &&  get_tile(i,x,y)->has_image()) {
					chk->input((sint16)(x+y+i));
				}
			}
		}
	}
}


/**
* @station get functions - see building_desc.h for variable information
* @author jamespetts
*/

sint32 building_desc_t::get_maintenance(karte_t *welt) const
{
	if(  maintenance == COST_MAGIC  ) {
		return welt->get_settings().maint_building*get_level();
	}
	else {
		return maintenance;
	}
}


sint32 building_desc_t::get_price(karte_t *welt) const
{
	if(  price == COST_MAGIC  ) {
		settings_t const& s = welt->get_settings();
		switch (get_type()) {
			case dock:
			case flat_dock:
				return -s.cst_multiply_dock * get_level();

			case generic_extension:
				return -s.cst_multiply_post * get_level();

			case generic_stop:
				switch(get_extra()) {
					case road_wt:        return -s.cst_multiply_roadstop * get_level();
					case track_wt:
					case monorail_wt:
					case maglev_wt:
					case narrowgauge_wt:
					case tram_wt:        return -s.cst_multiply_station * get_level();
					case water_wt:       return -s.cst_multiply_dock * get_level();
					case air_wt:         return -s.cst_multiply_airterminal * get_level();
					case 0:              return -s.cst_multiply_post * get_level();
				}
			case depot:
				switch(get_extra()) {
					case road_wt:        return -s.cst_depot_road;
					case track_wt:
					case monorail_wt:
					case tram_wt:
					case maglev_wt:
					case narrowgauge_wt: return -s.cst_depot_rail;
					case water_wt:       return -s.cst_depot_ship;
					case air_wt:         return -s.cst_depot_air;
					default:             return 0;
				}
			case headquarter:
				return -s.cst_multiply_headquarter * get_level();
			default:
				return -s.cst_multiply_remove_haus * get_level();
		}
	}
	else {
		return price;
	}
	return 0;
}
