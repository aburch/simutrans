/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simdebug.h"

#include "../world/simworld.h"
#include "../obj/way/weg.h"

#include "building_desc.h"
#include "intro_dates.h"
#include "../network/checksum.h"



/**
 * Calculate which layout the tile belongs to from the index.
 */
uint8 building_tile_desc_t::get_layout() const
{
	koord size = get_desc()->get_size();
	return index / (size.x * size.y);
}



/**
 * Return the relative position of an image in the whole building image
 */
koord building_tile_desc_t::get_offset() const
{
	const building_desc_t *desc = get_desc();
	koord size = desc->get_size(get_layout()); // rotate if necessary
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
 */
bool building_desc_t::is_connected_with_town() const
{
	switch(get_type()) {
		case city_res:
		case city_com:
		case city_ind:     // normal town buildings (RES, COM, IND)
		case monument:     // monuments
		case townhall:     // townhalls
		case headquarters: // headquarters
			return true;
		default:
			return false;
	}
}



/**
 * Returns the correct tile image on that position depending on the layout
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



/**
 * Layout normalisation. Returns number of different layouts
 */
uint8 building_desc_t::adjust_layout(uint8 layout) const
{
	if(layout >= 4 && layouts <= 4) {
		layout -= 4;
	}
	if(layout >= 2 && layouts <= 2) {
		// if layouts C and D are not defined, we use A and B as substitutes
		layout -= 2;
	}
	if(layout > 0 && layouts <= 1) {
		// if layout B is not defined and the building is squared, we us A as substitute
		if(size.x == size.y) {
			layout--;
		}
	}
	return layout;
}


void building_desc_t::calc_checksum(checksum_t *chk) const
{
	obj_desc_timelined_t::calc_checksum(chk);
#if MSG_LEVEL>0
	switch(type) {
		case city_res:     PAKSET_INFO("type=","res"); break;
		case city_com:     PAKSET_INFO("type=","com"); break;
		case city_ind:     PAKSET_INFO("type=","ind"); break;
		case attraction_land:
		case attraction_city:
		                   PAKSET_INFO("type=","cur"); break;
		case townhall:     PAKSET_INFO("type=","tow"); break;
		case monument:     PAKSET_INFO("type=","mon"); break;
		case headquarters: PAKSET_INFO("type=","hq"); break;
		case dock:         PAKSET_INFO("type=","harbour"); break;
		case flat_dock:    PAKSET_INFO("type=","dock"); break;
		case factory:      break;
		case generic_stop: PAKSET_INFO("type=","stop"); break;
		case generic_extension: PAKSET_INFO("type=","extension"); break;
		case depot:        PAKSET_INFO("type=","depot"); break;
		case others:       PAKSET_INFO("type=","any"); break;
		default:
			break;
	}
	// extra_data and level
	switch(type) {
		case city_res:
		case city_com:
		case city_ind:
			if(extra_data) PAKSET_INFO("clusters=","%d",extra_data);
			// fallthrough
		case attraction_land:
		case factory:
			if(preservation_year_month!=DEFAULT_RETIRE_YEAR*12) {
				PAKSET_INFO("preservation_year=","%d",preservation_year_month/12);
				if(preservation_year_month%12) PAKSET_INFO("preservation_month=","%d",(preservation_year_month%12)+1);
			}
			//PAKSET_INFO("level=","%d",level);
			break;
		case attraction_city:
		case townhall:
			PAKSET_INFO("passengers=","%d",level);
			PAKSET_INFO("build_time=","%d",extra_data);
			break;
		case monument:
			PAKSET_INFO("passengers=","%d",level);
			break;
		case headquarters:
			PAKSET_INFO("passengers=","%d",level);
			PAKSET_INFO("hq_level=","%d",extra_data);
			break;
		case dock:
		case flat_dock:
		case generic_stop:
		case generic_extension:
			PAKSET_INFO("capacity=","%d",capacity);
			if(enables&1) PAKSET_INFO("enables_pax=","1");
			if(enables&2) PAKSET_INFO("enables_post=","1");
			if(enables&4) PAKSET_INFO("enables_ware=","1");
			if(allow_underground) PAKSET_INFO("allow_underground=","1");
			// fallthrough
		case depot:
			PAKSET_INFO("waytype=","%s",weg_t::waytype_to_string((waytype_t)extra_data));
			break;
		case others:
		default:
			break;
	}
	if(distribution_weight!=100  &&  type!=factory) PAKSET_INFO("chance=","%d",distribution_weight);
	if(animation_time!=300) PAKSET_INFO("animation_time=","%d",animation_time);
	if(size.x*size.y*layouts>1) PAKSET_INFO("dims=","%d,%d,%d",size.x,size.y,layouts);
	if(flags&FLAG_NO_INFO) PAKSET_INFO("noinfo=","1");
	if(flags&FLAG_NO_PIT) PAKSET_INFO("noconstruction=","1");
	if(flags&FLAG_NEED_GROUND) PAKSET_INFO("needs_ground=","1");
	// climates are missing ...
#endif
	chk->input((uint8)type);
	chk->input(animation_time);
	chk->input(extra_data);
	chk->input(size.x);
	chk->input(size.y);
	chk->input((uint8)flags);
	chk->input(level);
	chk->input(layouts);
	chk->input(enables);
	chk->input(distribution_weight);
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
	chk->input(preservation_year_month);
}


/**
* get functions - see building_desc.h for variable information
*/

sint32 building_desc_t::get_maintenance(karte_t *world) const
{
	if(  maintenance == PRICE_MAGIC  ) {
		return world->get_settings().maint_building*get_level();
	}
	else {
		return maintenance;
	}
}


sint32 building_desc_t::get_price(karte_t *world) const
{
	if(  price == PRICE_MAGIC  ) {
		settings_t const& s = world->get_settings();
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
					default:             return 0;
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
			case headquarters:
				return -s.cst_multiply_headquarter * get_level();
			default:
				return -s.cst_multiply_remove_haus * get_level();
		}
	}
	else {
		return price;
	}
}
