/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simdebug.h"

#include "../simworld.h"

#include "building_desc.h"
#include "../network/checksum.h"
#include "../bauer/goods_manager.h"



/*
 *  Autor:
 *      Volker Meyer
 *
 *  Description:
 *      Calculate which layout the tile belongs to from the index.
 */
uint8 building_tile_desc_t::get_layout() const
{
	koord size = get_desc()->get_size();
	return index / (size.x * size.y);
}


/*
 *  Autor:
 *      Volker Meyer
 *
 *  Description:
 *      Return the relative position of an image in the whole building image
 */
koord building_tile_desc_t::get_offset() const
{
	const building_desc_t *desc = get_desc();
	koord size = desc->get_size(get_layout());	// rotate if necessary
	return koord( index % size.x, (index / size.x) % size.y );
}



waytype_t building_desc_t::get_finance_waytype() const
{
	switch( get_type() )
	{
		case dock:				return water_wt;
		case flat_dock:			return water_wt;
		case depot:
		case generic_stop:
		case generic_extension:
			return (waytype_t) get_extra();
		default:							return ignore_wt;
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
		case city_ind: return level / 2;
	}
}



/**
 * true, if this building needs a connection with a town
 * @author prissi
 */
bool building_desc_t::is_connected_with_town() const
{
	switch (get_type()) {
		case city_res:
		case city_com:
		case city_ind:    // normal town buildings (RES, COM, IND)
		case monument:     // monuments
		case attraction_city: // city attractions
		case townhall:     // townhalls
		case headquarters:  // headquarters
			return true;
		default:
			return false;
	}
}


/*
 *  Autor:
 *      Volker Meyer
 *
 *  Description:
 *      Returns the correct tile image on that position depending on the layout
 */
const building_tile_desc_t *building_desc_t::get_tile(int layout, int x, int y) const
{
	layout = adjust_layout(layout);
	koord dims = get_size(layout);

	if(layout < 0  ||  x < 0  ||  y < 0  ||  layout >= layouts  ||  x >= get_x(layout)  ||  y >= get_y(layout)) {
	dbg->fatal("building_tile_desc_t::get_tile()",
			   "invalid request for l=%d, x=%d, y=%d on building %s (l=%d, x=%d, y=%d)",
		   layout, x, y, get_name(), layouts, size.x, size.y);
	}
	return get_tile(layout * dims.x * dims.y + y * dims.x + x);
}



/*
 *  Autor:
 *      Volker Meyer
 *
 *  Description:
 *      Layout normalisation. Returns number of different layouts
 */
int building_desc_t::adjust_layout(int layout) const
{
	if(layout >= 4 && layouts <= 4) {
		layout -= 4;
	}
	if(layout >= 2 && layouts <= 2) {
		// if layouts C and D are not defined, we use A and B as substitutesormalisation. Returns number of different layouts
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

void building_desc_t::fix_number_of_classes()
{
	uint8 number_of_classes = goods_manager_t::passengers->get_number_of_classes();

	while (class_proportions.get_count() < number_of_classes)
	{
		class_proportions.append(class_proportions_sum);
	}
	if (class_proportions.get_count() > number_of_classes)
	{
		class_proportions.set_count(number_of_classes);
		class_proportions[number_of_classes-1] = class_proportions_sum;
	}

	while (class_proportions_jobs.get_count() < number_of_classes)
	{
		class_proportions_jobs.append(class_proportions_sum_jobs);
	}
	if (class_proportions_jobs.get_count() > number_of_classes)
	{
		class_proportions.set_count(number_of_classes);
		class_proportions_jobs[number_of_classes-1] += class_proportions_sum_jobs;
	}
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
	chk->input(distribution_weight);
	chk->input((uint8)allowed_climates);
	chk->input(allowed_regions);
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

	//Extended settings
	chk->input(is_control_tower);
	chk->input(radius);
	chk->input(class_proportions.get_count());
	chk->input(class_proportions_jobs.get_count());
}

uint8 building_desc_t::city_building_max_size = 1;
