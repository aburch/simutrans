/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "pedestrian.h"

#include "../simdebug.h"
#include "../world/simworld.h"
#include "../display/simimg.h"

#include "../utils/simrandom.h"
#include "../ground/grund.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/pakset_manager.h"

#include "../utils/cbuffer.h"
#include "../descriptor/pedestrian_desc.h"

#include <cstdio>


freelist_iter_tpl<pedestrian_t> pedestrian_t::fl;

static uint32 const strecke[] = { 6000, 11000, 15000, 20000, 25000, 30000, 35000, 40000 };

static weighted_vector_tpl<const pedestrian_desc_t*> list_timeline;
stringhashtable_tpl<const pedestrian_desc_t *> pedestrian_t::table;


bool pedestrian_t::register_desc(const pedestrian_desc_t *desc)
{
	if(  table.remove(desc->get_name())  ) {
		pakset_manager_t::doubled( "pedestrian", desc->get_name() );
	}
	table.put(desc->get_name(), desc);
	return true;
}


bool pedestrian_t::successfully_loaded()
{
	if (table.empty()) {
		DBG_MESSAGE("pedestrian_t", "No pedestrians found - feature disabled");
	}
	return true;
}


static bool compare_pedestrian_desc(const pedestrian_desc_t* a, const pedestrian_desc_t* b)
{
	int diff = a->get_intro_year_month() - b->get_intro_year_month();
	if (diff == 0) {
		/* same Level - we introduce an artificial, but unique resort
		* on the induced name. */
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}


pedestrian_t::pedestrian_t(loadsave_t *file)
 : road_user_t()
{
	animation_steps = 0;
	on_left = false;
	steps_offset = 0;
	rdwr(file);
	if(desc) {
		ped_offset = desc->get_offset();
	}
	calc_disp_lane();
}


void pedestrian_t::build_timeline_list(karte_t *welt)
{
	// this list will contain all citycars
	list_timeline.clear();
	vector_tpl<const pedestrian_desc_t*> temp_liste(0);
	if(  !table.empty()  ) {
		const int month_now = welt->get_current_month();

		// check for every citycar, if still ok ...
		for(auto const& i : table) {
			pedestrian_desc_t const* const info = i.value;
			const int intro_month = info->get_intro_year_month();
			const int retire_month = info->get_retire_year_month();

			if (!welt->use_timeline() || (intro_month <= month_now && month_now < retire_month)) {
				temp_liste.insert_ordered( info, compare_pedestrian_desc );
			}
		}
	}
	list_timeline.resize( temp_liste.get_count() );
	for(pedestrian_desc_t const* const i : temp_liste) {
		list_timeline.append(i, i->get_distribution_weight());
	}
}



bool pedestrian_t::list_empty()
{
	return list_timeline.empty();
}



pedestrian_t::pedestrian_t(grund_t *gr) :
	road_user_t(gr, simrand(65535)),
	desc(pick_any_weighted(list_timeline))
{
	animation_steps = 0;
	on_left = simrand(2) > 0;
	steps_offset = 0;
	time_to_life = pick_any(strecke);
	ped_offset = desc->get_offset();
	calc_image();
	calc_disp_lane();
}


void pedestrian_t::calc_image()
{
	set_image(desc->get_image_id(ribi_t::get_dir(get_direction())));
}


image_id pedestrian_t::get_image() const
{
	if (desc->get_steps_per_frame() > 0) {
		uint16 frame = ((animation_steps + steps) / desc->get_steps_per_frame()) % desc->get_animation_count(ribi_t::get_dir(direction));
		return desc->get_image_id(ribi_t::get_dir(get_direction()), frame);
	}
	else {
		return image;
	}
}


void pedestrian_t::rdwr(loadsave_t *file)
{
	xml_tag_t f( file, "fussgaenger_t" );

	road_user_t::rdwr(file);

	if(!file->is_loading()) {
		const char *s = desc->get_name();
		file->rdwr_str(s);
	}
	else {
		char s[256];
		file->rdwr_str(s, lengthof(s));
		desc = table.get(s);
		// unknown pedestrian => create random new one
		if(desc == NULL  &&  !list_timeline.empty()  ) {
			desc = pick_any_weighted(list_timeline);
		}
	}

	if(file->is_version_less(89, 4)) {
		time_to_life = pick_any(strecke);
	}

	if (file->is_version_atleast(120, 6)) {
		file->rdwr_short(steps_offset);
		file->rdwr_bool(on_left);
	}

	if (file->is_loading()) {
		calc_disp_lane();
	}
}

void pedestrian_t::calc_disp_lane()
{
	// walking in the back or the front
	ribi_t::ribi test_dir = on_left ? ribi_t::northeast : ribi_t::southwest;
	disp_lane = direction & test_dir ? 0 : 4;
}


void pedestrian_t::rotate90()
{
	road_user_t::rotate90();
	calc_disp_lane();
}

// create a number (count) of pedestrians (if possible)
void pedestrian_t::generate_pedestrians_at(const koord3d k, int &count)
{
	if (list_timeline.empty()) {
		return;
	}

	grund_t* bd = welt->lookup(k);
	if (bd) {
		const weg_t* weg = bd->get_weg(road_wt);

		// we do not start on crossings (not overrunning pedestrians please
		if (weg && ribi_t::is_twoway(weg->get_ribi_unmasked())) {
			// we create maximal 4 pedestrians here for performance reasons
			for (int i = 0; i < 4 && count > 0; i++) {
				if (bd->get_top() >= 240) {
					// tile too full
					return;
				}

				pedestrian_t* fg = new pedestrian_t(bd);
				if (bd->obj_add(fg) != 0) {
					fg->calc_height(bd);
					if (i > 0) {
						// walk a little
						fg->sync_step( (i & 3) * 64 * 24);
					}
					count--;
				}
				else {
					// delete it, if we could not put it on the map
					fg->set_flag(obj_t::not_on_map);
					// do not try to delete it from sync-list
					fg->time_to_life = 0;
					return; // it is pointless to try for more here
				}
			}
		}
	}
}


sync_result pedestrian_t::sync_step(uint32 delta_t)
{
	time_to_life -= delta_t;

	if (time_to_life>0) {
		weg_next += 128*delta_t;
		weg_next -= do_drive( weg_next );
		return time_to_life>0 ? SYNC_OK : SYNC_DELETE;
	}
	return SYNC_DELETE;
}


grund_t* pedestrian_t::hop_check()
{
	grund_t *from = welt->lookup(pos_next);
	if(!from) {
		time_to_life = 0;
		return NULL;
	}

	// find the allowed directions
	const weg_t *weg = from->get_weg(road_wt);
	if(weg==NULL) {
		// no road anymore: destroy it
		time_to_life = 0;
		return NULL;
	}

	if (from->get_top() >= 240) {
		time_to_life = 0;
		return NULL; // target tile full, just die
	}

	return from;
}


void pedestrian_t::hop(grund_t *gr)
{
	const koord3d from = get_pos();

	// hop
	leave_tile();
	set_pos(gr->get_pos());
	// no need to call enter_tile()

	// if this fails, the target tile is full, but this should already have been checked in hop_check
	const bool ok = gr->obj_add(this);
	assert(ok); (void)ok;

	// determine pos_next
	const weg_t *weg = gr->get_weg(road_wt);
	// new target
	grund_t *to = NULL;
	// current single direction
	ribi_t::ribi current_direction = ribi_type(from, get_pos());
	// ribi opposite to current direction
	ribi_t::ribi reverse_direction = ribi_t::reverse_single( current_direction );
	// all possible directions
	ribi_t::ribi ribi = weg->get_ribi_unmasked() & (~reverse_direction);
	// randomized offset
	const uint8 offset = (ribi > 0  &&  ribi_t::is_single(ribi)) ? 0 : simrand(4);

	ribi_t::ribi new_direction = ribi_t::none;
	for(uint r = 0; r < 4; r++) {
		new_direction = ribi_t::nesw[ (r+offset) & 3];

		if(  (ribi & new_direction)!=0  &&  gr->get_neighbour(to, road_wt, new_direction) ) {
			// this is our next target
			break;
		}
	}
	steps_offset = 0;

	if (to) {
		pos_next = to->get_pos();

		if (new_direction == current_direction) {
			// going straight
			direction = calc_set_direction(from, pos_next);
		}
		else {
			ribi_t::ribi turn_ribi = on_left ? ribi_t::rotate90l(current_direction) : ribi_t::rotate90(current_direction);

			if (turn_ribi == new_direction) {
				// short diagonal (turn but do not cross street)
				direction = calc_set_direction(from, pos_next);
				steps_next = (ped_offset*181) / 128; // * sqrt(2)
				steps_offset = 0;
			}
			else {
				// do not cross street diagonally, change side
				on_left = !on_left;
				direction = calc_set_direction(get_pos(), pos_next);
			}
		}
	}
	else {
		// dead end, turn
		pos_next = from;
		direction = calc_set_direction(get_pos(), pos_next);
		steps_offset = VEHICLE_STEPS_PER_TILE - ped_offset;
		steps_next   = ped_offset;
		on_left = !on_left;
	}

	calc_disp_lane();
	// carry over remainder to next tile for continuous animation during straight movement
	uint16 steps_per_animation = desc->get_steps_per_frame() * desc->get_animation_count(ribi_t::get_dir(direction));
	if (steps_per_animation > 0) {
		animation_steps = (animation_steps + steps_next + 1) % steps_per_animation;
	}
	else {
		animation_steps = 0;
	}

	calc_image();
}

void pedestrian_t::get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const
{
	// vehicles needs finer steps to appear smoother
	sint32 display_steps = (uint32)(steps + steps_offset)*(uint16)raster_width;
	if(  dx && dy  ) {
		display_steps &= 0xFFFFFC00;
	}
	else {
		display_steps = (display_steps*diagonal_multiplier)>>10;
	}
	xoff += (display_steps*dx) >> 10;
	yoff += ((display_steps*dy) >> 10) + (get_hoff(raster_width))/(4*16);

	if (on_left) {
		sint32 left_off_steps = ( (VEHICLE_STEPS_PER_TILE - 2*ped_offset)*(uint16)raster_width ) & 0xFFFFFC00;

		if (dx*dy==0) {
			// diagonal
			left_off_steps /= 2;
		}
		// turn left (dx,dy) increments
		xoff += (left_off_steps*2*dy) >> 10;
		yoff -= (left_off_steps*dx) >> (10+1);
	}
}


void pedestrian_t::info(cbuffer_t & buf) const
{
	char const* const owner = translator::translate("Kein Besitzer\n");
	buf.append(owner);

	if (char const* const maker = get_desc()->get_copyright()) {
		buf.printf(translator::translate("Constructed by %s"), maker);
	}
}
