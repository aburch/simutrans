/*
* Copyright (c) 1997 - 2001 Hansjörg Malthaner
*
* This file is part of the Simutrans project under the artistic licence.
* (see licence.txt)
*/

#include <string.h>
#include <ctype.h>
#include <algorithm>

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t sync_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t add_to_city_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#include "../bauer/hausbauer.h"
#include "../bauer/goods_manager.h"
#include "../gui/money_frame.h"
#include "../simworld.h"
#include "../simobj.h"
#include "../simfab.h"
#include "../display/simimg.h"
#include "../display/simgraph.h"
#include "../simhalt.h"
#include "../gui/simwin.h"
#include "../simcity.h"
#include "../player/simplay.h"
#include "../utils/simrandom.h"
#include "../simdebug.h"
#include "../simintr.h"
#include "../simskin.h"
#include "../simsignalbox.h"
#include "../utils/simstring.h"

#include "../boden/grund.h"
#include "../boden/wege/strasse.h"

#include "../descriptor/building_desc.h"
#include "../descriptor/intro_dates.h"

#include "../descriptor/ground_desc.h"

#include "../utils/cbuffer_t.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/settings.h"
#include "../dataobj/environment.h"

#include "../gui/obj_info.h"

#include "gebaeude.h"


/**
* Initializes all variables with safe, usable values
* @author Hj. Malthaner
*/
void gebaeude_t::init()
{
	tile = NULL;
	anim_time = 0;
	sync = false;
	zeige_baugrube = false;
	remove_ground = true;
	is_factory = false;
	season = 0;
	background_animated = false;
	remove_ground = true;
	anim_frame = 0;
	//	purchase_time = 0; // init in set_tile()
	ptr.fab = NULL;
	passengers_generated_commuting = 0;
	passengers_succeeded_commuting = 0;
	passenger_success_percent_last_year_commuting = 65535;
	passengers_generated_visiting = 0;
	passengers_succeeded_visiting = 0;
	passenger_success_percent_last_year_visiting = 65535;
	available_jobs_by_time = -9223372036854775808ll;
	is_in_world_list = 0;
	loaded_passenger_and_mail_figres = false;
}


gebaeude_t::gebaeude_t(obj_t::typ type) :
#ifdef INLINE_OBJ_TYPE
	obj_t(type)
#else
	obj_t()
#endif
{
	init();
}


gebaeude_t::gebaeude_t(loadsave_t *file, bool do_not_add_to_world_list) :
#ifdef INLINE_OBJ_TYPE
	obj_t(obj_t::gebaeude)
#else
	obj_t()
#endif
{
	init();
	if (do_not_add_to_world_list)
	{
		is_in_world_list = -1;
	}
	rdwr(file);
	if (file->get_version()<88002) {
		set_yoff(0);
	}
	if (tile  &&  tile->get_phases()>1) {
		welt->sync_eyecandy.add(this);
		sync = true;
	}
}



#ifdef INLINE_OBJ_TYPE
gebaeude_t::gebaeude_t(obj_t::typ type, koord3d pos, player_t *player, const building_tile_desc_t *t) :
	obj_t(type, pos)
{
	init(player, t);
}

gebaeude_t::gebaeude_t(koord3d pos, player_t *player, const building_tile_desc_t *t) :
	obj_t(obj_t::gebaeude, pos)
{
	init(player, t);
}

void gebaeude_t::init(player_t *player, const building_tile_desc_t *t)
#else
gebaeude_t::gebaeude_t(koord3d pos, player_t *player, const building_tile_desc_t *t) :
	obj_t(pos)
#endif
{
	set_owner(player);

	init();
	if (t)
	{
		set_tile(t, true);	// this will set init time etc.
		sint64 maint;
		if (tile->get_desc()->get_base_maintenance() == PRICE_MAGIC)
		{
			maint = welt->get_settings().maint_building*tile->get_desc()->get_level();
		}
		else
		{
			maint = tile->get_desc()->get_maintenance();
		}
		player_t::add_maintenance(get_owner(), maint, tile->get_desc()->get_finance_waytype());
	}

	const building_desc_t::btype type = tile->get_desc()->get_type();

	if (type == building_desc_t::city_res)
	{
		const uint16 population = tile->get_desc()->get_population_and_visitor_demand_capacity();
		people.population = population == 65535 ? tile->get_desc()->get_level() * welt->get_settings().get_population_per_level() : population;
		adjusted_people.population = welt->calc_adjusted_monthly_figure(people.population);
		if (people.population > 0 && adjusted_people.population == 0)
		{
			adjusted_people.population = 1;
		}
	}
	else if (type == building_desc_t::city_ind)
	{
		people.visitor_demand = adjusted_people.visitor_demand = 0;
	}
	else if (tile->get_desc()->is_factory() && tile->get_desc()->get_population_and_visitor_demand_capacity() == 65535)
	{
		adjusted_people.visitor_demand = 65535;
	}
	else
	{
		const uint16 population_and_visitor_demand_capacity = tile->get_desc()->get_population_and_visitor_demand_capacity();
		people.visitor_demand = population_and_visitor_demand_capacity == 65535 ? tile->get_desc()->get_level() * welt->get_settings().get_visitor_demand_per_level() : population_and_visitor_demand_capacity;
		adjusted_people.visitor_demand = welt->calc_adjusted_monthly_figure(people.visitor_demand);
		if (people.visitor_demand > 0 && adjusted_people.visitor_demand == 0)
		{
			adjusted_people.visitor_demand = 1;
		}
	}

	jobs = tile->get_desc()->get_employment_capacity() == 65535 ? (is_monument() || type == building_desc_t::city_res) ? 0 : tile->get_desc()->get_level() * welt->get_settings().get_jobs_per_level() : tile->get_desc()->get_employment_capacity();
	mail_demand = tile->get_desc()->get_mail_demand_and_production_capacity() == 65535 ? is_monument() ? 0 : tile->get_desc()->get_level() * welt->get_settings().get_mail_per_level() : tile->get_desc()->get_mail_demand_and_production_capacity();

	adjusted_jobs = welt->calc_adjusted_monthly_figure(jobs);
	if (jobs > 0 && adjusted_jobs == 0)
	{
		adjusted_jobs = 1;
	}

	adjusted_mail_demand = welt->calc_adjusted_monthly_figure(mail_demand);
	if (mail_demand > 0 && adjusted_mail_demand == 0)
	{
		adjusted_mail_demand = 1;
	}

	// get correct y offset for bridges
	grund_t *gr = welt->lookup(get_pos());
	if (gr  &&  gr->get_weg_hang() != gr->get_grund_hang()) {
		set_yoff(-gr->get_weg_yoff());
	}

	check_road_tiles(false);

	// This sets the number of jobs per building at initialisation to zero. As time passes,
	// more jobs become available. This is necessary because, if buildings are initialised
	// with their maximum number of jobs, there will be too many jobs available by a factor
	// of two. This is because, for any given time period in which the total population is
	// equal to the total number of jobs available, X people will arrive and X job slots
	// will be created. The sum total of this should be zero, but if buildings start with
	// their maximum number of jobs, this ends up being the base line number, effectively
	// doubling the number of available jobs.
	available_jobs_by_time = welt->get_ticks();
}

stadt_t* gebaeude_t::get_stadt() const
{
	return ptr.fab != NULL ? is_factory ? ptr.fab->get_city() : ptr.stadt : NULL;
}

/**
* Destructor. Removes this from the list of sync objects if necessary.
*
* @author Hj. Malthaner
*/
gebaeude_t::~gebaeude_t()
{
	if (welt->is_destroying())
	{
		return;
		// avoid book-keeping
	}

	stadt_t* our_city = get_stadt();
	const bool has_city_defined = our_city != NULL;
	if (!our_city /* && tile->get_desc()->get_type() == building_desc_t::townhall*/)
	{
		our_city = welt->get_city(get_pos().get_2d());
	}
	if (!our_city)
	{
		our_city = welt->get_city(get_first_tile()->get_pos().get_2d());
	}
	if (our_city)
	{
		our_city->remove_gebaeude_from_stadt(this, !has_city_defined, false);
	}
	else if(is_in_world_list > 0)
	{
		welt->remove_building_from_world_list(this);
	}


	if (sync) {
		sync = false;
		welt->sync_eyecandy.remove(this);
	}


	// tiles might be invalid, if no description is found during loading
	if (tile && tile->get_desc())
	{
		check_road_tiles(true);
		if (tile->get_desc()->is_attraction())
		{
			welt->remove_attraction(this);
		}
	}

	if (tile)
	{
		sint64 maint;
		if (tile->get_desc()->get_base_maintenance() == PRICE_MAGIC)
		{
			maint = welt->get_settings().maint_building * tile->get_desc()->get_level();
		}
		else
		{
			maint = tile->get_desc()->get_maintenance();
		}
		player_t::add_maintenance(get_owner(), -maint);
	}

	const weighted_vector_tpl<stadt_t*>& staedte = welt->get_cities();
	for (weighted_vector_tpl<stadt_t*>::const_iterator j = staedte.begin(), end = staedte.end(); j != end; ++j)
	{
		(*j)->remove_connected_attraction(this);
	}
}


void gebaeude_t::check_road_tiles(bool del)
{
	const building_desc_t *bdsc = tile->get_desc();
	const koord3d pos = get_pos() - koord3d(tile->get_offset(), 0);
	koord size = bdsc->get_size(tile->get_layout());
	koord k;
	grund_t* gr_this;

	vector_tpl<gebaeude_t*> building_list;
	building_list.append(this);

	for (k.y = 0; k.y < size.y; k.y++)
	{
		for (k.x = 0; k.x < size.x; k.x++)
		{
			koord3d k_3d = koord3d(k, 0) + pos;
			grund_t *gr = welt->lookup(k_3d);
			if (gr)
			{
				gebaeude_t *gb_part = gr->find<gebaeude_t>();
				// there may be buildings with holes
				if (gb_part && gb_part->get_tile()->get_desc() == bdsc)
				{
					building_list.append_unique(gb_part);
				}
			}
		}
	}

	FOR(vector_tpl<gebaeude_t*>, gb, building_list)
	{
		for (uint8 i = 0; i < 8; i++)
		{
			/* This is tricky: roads can change height, and we're currently
			* not keeping track of when they do. We might show
			* up as connecting to a road that's no longer at the right
			* height. Therefore, iterate over all possible road levels when
			* removing, but not when adding new connections. */
			koord pos_neighbour = gb->get_pos().get_2d() + (gb->get_pos().get_2d().neighbours[i]);
			if (del)
			{
				const planquadrat_t *plan = welt->access(pos_neighbour);
				if (!plan)
				{
					continue;
				}
				for (int j = 0; j<plan->get_boden_count(); j++)
				{
					grund_t *bd = plan->get_boden_bei(j);
					strasse_t *str = (strasse_t *)bd->get_weg(road_wt);

					if (str)
					{
						str->connected_buildings.remove(this);
					}
				}
			}
			else
			{
				koord3d pos3d(pos_neighbour, gb->get_pos().z);

				// Check for connected roads. Only roads in immediately neighbouring tiles
				// and only those on the same height will register a connexion.
				gr_this = welt->lookup(pos3d);

				if (!gr_this)
				{
					continue;
				}
				strasse_t* str = (strasse_t*)gr_this->get_weg(road_wt);
				if (str)
				{
					str->connected_buildings.append_unique(this);
				}
			}
		}
	}
}

void gebaeude_t::rotate90()
{
	obj_t::rotate90();

	// must or can rotate?
	const building_desc_t* const building_desc = tile->get_desc();
	if (building_desc->get_all_layouts() > 1 || building_desc->get_x() * building_desc->get_y() > 1) {
		uint8 layout = tile->get_layout();
		koord new_offset = tile->get_offset();

		if (building_desc->get_type() == building_desc_t::unknown || building_desc->get_all_layouts() <= 4) {
			layout = (layout & 4) + ((layout + 3) % building_desc->get_all_layouts() & 3);
		}
		else {
			static uint8 layout_rotate[16] = { 1, 8, 5, 10, 3, 12, 7, 14, 9, 0, 13, 2, 11, 4, 15, 6 };
			layout = layout_rotate[layout] % building_desc->get_all_layouts();
		}
		// have to rotate the tiles :(
		if (!building_desc->can_rotate() && building_desc->get_all_layouts() == 1) {
			if ((welt->get_settings().get_rotation() & 1) == 0) {
				// rotate 180 degree
				new_offset = koord(building_desc->get_x() - 1 - new_offset.x, building_desc->get_y() - 1 - new_offset.y);
			}
			// do nothing here, since we cannot fix it properly
		}
		else {
			// rotate on ...
			new_offset = koord(building_desc->get_y(tile->get_layout()) - 1 - new_offset.y, new_offset.x);
		}

		// such a tile exist?
		if (building_desc->get_x(layout) > new_offset.x  &&  building_desc->get_y(layout) > new_offset.y) {
			const building_tile_desc_t* const new_tile = building_desc->get_tile(layout, new_offset.x, new_offset.y);
			// add new tile: but make them old (no construction)
			sint64 old_purchase_time = purchase_time;
			set_tile(new_tile, false);
			purchase_time = old_purchase_time;
			if (building_desc->get_type() != building_desc_t::dock && !tile->has_image()) {
				// may have a rotation, that is not recoverable
				if (!is_factory  &&  new_offset != koord(0, 0)) {
					welt->set_nosave_warning();
				}
				if (is_factory) {
					// there are factories with a broken tile
					// => this map rotation cannot be reloaded!
					welt->set_nosave();
				}
			}
		}
		else {
			welt->set_nosave();
		}
	}

	// These will be re-initialised where necessary.
	building_tiles.clear();
}



/* sets the corresponding pointer to a factory
* @author prissi
*/
void gebaeude_t::set_fab(fabrik_t *fd)
{
	// sets the pointer in non-zero
	if (fd) {
		if (!is_factory  &&  ptr.stadt != NULL) {
			dbg->fatal("gebaeude_t::set_fab()", "building already bound to city!");
		}
		is_factory = true;
		ptr.fab = fd;
		if (adjusted_people.visitor_demand == 65535)
		{
			// We cannot set this until we know what sort of factory that this is.
			// If it is not an end consumer, do not allow any visitor demand by default.
			if (fd->is_end_consumer())
			{
				people.visitor_demand = tile->get_desc()->get_level() * welt->get_settings().get_visitor_demand_per_level();
				adjusted_people.visitor_demand = welt->calc_adjusted_monthly_figure(people.visitor_demand);
			}
			else
			{
				adjusted_people.visitor_demand = people.visitor_demand = 0;
			}
		}
	}
	else if (is_factory) {
		ptr.fab = NULL;
	}
}



/* sets the corresponding city
* @author prissi
*/
void gebaeude_t::set_stadt(stadt_t *s)
{
	if (is_factory && ptr.fab != NULL)
	{
		if (s == NULL)
		{
			return;
		}
		dbg->fatal("gebaeude_t::set_stadt()", "building at (%s) already bound to factory!", get_pos().get_str());
	}
	// sets the pointer in non-zero
	is_factory = false;
	ptr.stadt = s;
}


/* make this building without construction */
void gebaeude_t::add_alter(sint64 a)
{
	purchase_time -= min(a, purchase_time);
}


void gebaeude_t::set_tile(const building_tile_desc_t *new_tile, bool start_with_construction)
{
	purchase_time = welt->get_ticks();

	if (!zeige_baugrube  &&  tile != NULL) {
		// mark old tile dirty
		mark_images_dirty();
	}

	zeige_baugrube = !new_tile->get_desc()->no_construction_pit() && start_with_construction;
	if (sync) {
		if (new_tile->get_phases() <= 1 && !zeige_baugrube) {
			// need to stop animation
#ifdef MULTI_THREAD
			pthread_mutex_lock(&sync_mutex);
#endif
			welt->sync_eyecandy.remove(this);
			sync = false;
			anim_frame = 0;
#ifdef MULTI_THREAD
			pthread_mutex_unlock(&sync_mutex);
#endif
		}
	}
	else if ((new_tile->get_phases()>1 && (!is_factory || get_fabrik()->is_currently_producing())) || zeige_baugrube) {
		// needs now animation
#ifdef MULTI_THREAD
		pthread_mutex_lock(&sync_mutex);
#endif
		anim_frame = sim_async_rand(new_tile->get_phases());
		anim_time = 0;
		welt->sync_eyecandy.add(this);
		sync = true;
#ifdef MULTI_THREAD
		pthread_mutex_unlock(&sync_mutex);
#endif
	}
	tile = new_tile;
	remove_ground = tile->has_image() && !tile->get_desc()->needs_ground();
	set_flag(obj_t::dirty);
}


sync_result gebaeude_t::sync_step(uint32 delta_t)
{
	if (purchase_time > welt->get_ticks())
	{
		// There were some integer overflow issues with
		// this when some intermediate values were uint32.
		purchase_time = welt->get_ticks() - 5000ll;
	}
	if (zeige_baugrube) {
		// still under construction?
		if (welt->get_ticks() - purchase_time > 5000) {
			set_flag(obj_t::dirty);
			mark_image_dirty(get_image(), 0);
			zeige_baugrube = false;
			if (tile->get_phases() <= 1) {
				sync = false;
				return SYNC_REMOVE;
			}
		}
	}
	else {
		if (!is_factory || get_fabrik()->is_currently_producing()) {
			// normal animated building
			anim_time += delta_t;
			if (anim_time > tile->get_desc()->get_animation_time()) {
				anim_time -= tile->get_desc()->get_animation_time();

				// old positions need redraw
				if (background_animated) {
					set_flag(obj_t::dirty);
					mark_images_dirty();
				}
				else {
					// try foreground
					image_id image = tile->get_foreground(anim_frame, season);
					mark_image_dirty(image, 0);
				}

				anim_frame++;
				if (anim_frame >= tile->get_phases()) {
					anim_frame = 0;
				}

				if (!background_animated) {
					// next phase must be marked dirty too ...
					image_id image = tile->get_foreground(anim_frame, season);
					mark_image_dirty(image, 0);
				}
			}
		}
	}
	return SYNC_OK;
}



void gebaeude_t::calc_image()
{
	grund_t *gr = welt->lookup(get_pos());
	// need no ground?
	if (remove_ground  && gr && gr->get_typ() == grund_t::fundament) {
		gr->set_image(IMG_EMPTY);
	}

	static uint8 effective_season[][5] = { { 0,0,0,0,0 },{ 0,0,0,0,1 },{ 0,0,0,0,1 },{ 0,1,2,3,2 },{ 0,1,2,3,4 } };  // season image lookup from [number of images] and [actual season/snow]

	if (gr && (gr->ist_tunnel() && !gr->ist_karten_boden()) || tile->get_seasons() < 2) {
		season = 0;
	}
	else if (get_pos().z - (get_yoff() / TILE_HEIGHT_STEP) >= welt->get_snowline() || welt->get_climate(get_pos().get_2d()) == arctic_climate) {
		// snowy winter graphics
		season = effective_season[tile->get_seasons() - 1][4];
	}
	else if (get_pos().z - (get_yoff() / TILE_HEIGHT_STEP) >= welt->get_snowline() - 1 && welt->get_season() == 0) {
		// snowline crossing in summer
		// so at least some weeks spring/autumn
		season = effective_season[tile->get_seasons() - 1][welt->get_last_month() <= 5 ? 3 : 1];
	}
	else {
		season = effective_season[tile->get_seasons() - 1][welt->get_season()];
	}

	background_animated = tile->is_background_animated(season);
}


image_id gebaeude_t::get_image() const
{
	if (env_t::hide_buildings != 0 && tile->has_image()) {
		// opaque houses
		if (is_city_building()) {
			return env_t::hide_with_transparency ? skinverwaltung_t::fussweg->get_image_id(0) : skinverwaltung_t::construction_site->get_image_id(0);
		}
		else if ((env_t::hide_buildings == env_t::ALL_HIDDEN_BUILDING  &&  tile->get_desc()->get_type() < building_desc_t::others)) {
			// hide with transparency or tile without information
			if (env_t::hide_with_transparency) {
				if (tile->get_desc()->get_type() == building_desc_t::factory  &&  ptr.fab->get_desc()->get_placement() == factory_desc_t::Water) {
					// no ground tiles for water things
					return IMG_EMPTY;
				}
				return skinverwaltung_t::fussweg->get_image_id(0);
			}
			else {
				int kind = skinverwaltung_t::construction_site->get_count() <= tile->get_desc()->get_type() ? skinverwaltung_t::construction_site->get_count() - 1 : tile->get_desc()->get_type();
				return skinverwaltung_t::construction_site->get_image_id(kind);
			}
		}
	}

	// winter for buildings only above snowline
	if (zeige_baugrube) {
		return skinverwaltung_t::construction_site->get_image_id(0);
	}
	else {
		return tile->get_background(anim_frame, 0, season);
	}
}


image_id gebaeude_t::get_outline_image() const
{
	if (env_t::hide_buildings != 0 && env_t::hide_with_transparency && !zeige_baugrube) {
		// opaque houses
		return tile->get_background(anim_frame, 0, season);
	}
	return IMG_EMPTY;
}


/* gives outline colour and plots background tile if needed for transparent view */
PLAYER_COLOR_VAL gebaeude_t::get_outline_colour() const
{
	COLOR_VAL colours[] = { COL_BLACK, COL_YELLOW, COL_YELLOW, COL_PURPLE, COL_RED, COL_GREEN };
	PLAYER_COLOR_VAL disp_colour = 0;
	if (env_t::hide_buildings != env_t::NOT_HIDE) {
		if (is_city_building()) {
			disp_colour = colours[0] | TRANSPARENT50_FLAG | OUTLINE_FLAG;
		}
		else if (env_t::hide_buildings == env_t::ALL_HIDDEN_BUILDING && tile->get_desc()->get_type() < building_desc_t::others) {
			// special building
			disp_colour = colours[tile->get_desc()->get_type()] | TRANSPARENT50_FLAG | OUTLINE_FLAG;
		}
	}
	return disp_colour;
}


image_id gebaeude_t::get_image(int nr) const
{
	if (zeige_baugrube || env_t::hide_buildings) {
		return IMG_EMPTY;
	}
	else {
		return tile->get_background(anim_frame, nr, season);
	}
}


image_id gebaeude_t::get_front_image() const
{
	if (zeige_baugrube) {
		return IMG_EMPTY;
	}
	if (env_t::hide_buildings != 0 && tile->get_desc()->get_type() < building_desc_t::others) {
		return IMG_EMPTY;
	}
	else {
		// Show depots, station buildings etc.
		return tile->get_foreground(anim_frame, season);
	}
}
/**
* @return eigener Name oder Name der Fabrik falls Teil einer Fabrik
* @author Hj. Malthaner
*/
const char *gebaeude_t::get_name() const
{
	if (is_factory  &&  ptr.fab) {
		return ptr.fab->get_name();
	}

	switch (tile->get_desc()->get_type()) {
	case building_desc_t::attraction_city:   return "Besonderes Gebaeude";
	case building_desc_t::attraction_land:   return "Sehenswuerdigkeit";
	case building_desc_t::monument:           return "Denkmal";
	case building_desc_t::townhall:           return "Rathaus";
	case building_desc_t::signalbox:
	case building_desc_t::depot:			  return tile->get_desc()->get_name();
	default: break;
	}
	return "Gebaeude";
}


/**
* waytype associated with this object
*/
waytype_t gebaeude_t::get_waytype() const
{
	const building_desc_t *desc = tile->get_desc();
	waytype_t wt = invalid_wt;

	const building_desc_t::btype type = tile->get_desc()->get_type();
	if (type == building_desc_t::depot || type == building_desc_t::generic_stop || type == building_desc_t::generic_extension) {
		wt = (waytype_t)desc->get_extra();
	}
	return wt;
}


bool gebaeude_t::is_townhall() const
{
	return tile->get_desc()->is_townhall();
}


bool gebaeude_t::is_monument() const
{
	return tile->get_desc()->get_type() == building_desc_t::monument;
}


bool gebaeude_t::is_headquarter() const
{
	return tile->get_desc()->is_headquarters();
}

bool gebaeude_t::is_attraction() const
{
	return tile->get_desc()->is_attraction();
}


bool gebaeude_t::is_city_building() const
{
	return tile->get_desc()->is_city_building();
}


void gebaeude_t::show_info()
{
	// TODO: Add code for signalbox dialoguess here
	if (get_fabrik()) {
		ptr.fab->show_info();
		return;
	}
	int old_count = win_get_open_count();
	bool special = is_headquarter() || is_townhall();

	if (is_headquarter()) {
		create_win(new money_frame_t(get_owner()), w_info, magic_finances_t + get_owner()->get_player_nr());
	}
	else if (is_townhall()) {
		get_stadt()->show_info();
	}

	if (!tile->get_desc()->no_info_window()) {
		if (!special || (env_t::townhall_info  &&  old_count == win_get_open_count())) {
			// open info window for the first tile of our building (not relying on presence of (0,0) tile)
			access_first_tile()->obj_t::show_info();
		}
	}
}


bool gebaeude_t::is_same_building(gebaeude_t* other) const
{
	return (other != NULL) && (get_tile()->get_desc() == other->get_tile()->get_desc())
		&& (get_first_tile() == other->get_first_tile());
}


const gebaeude_t* gebaeude_t::get_first_tile() const
{
	if (tile)
	{
		const building_desc_t* const building_desc = tile->get_desc();
		const uint8 layout = tile->get_layout();
		koord k;
		for (k.x = 0; k.x<building_desc->get_x(layout); k.x++) {
			for (k.y = 0; k.y<building_desc->get_y(layout); k.y++) {
				const building_tile_desc_t *tile = building_desc->get_tile(layout, k.x, k.y);
				if (tile == NULL || !tile->has_image()) {
					continue;
				}
				if (grund_t *gr = welt->lookup(get_pos() - get_tile()->get_offset() + k))
				{
					gebaeude_t* gb;
					if (tile->get_desc()->is_signalbox())
					{
						gb = gr->get_signalbox();
					}
					else
					{
						gb = gr->find<gebaeude_t>();
					}
					if (gb && gb->get_tile() == tile)
					{
						return gb;
					}
				}
			}
		}
	}
	return this;
}

gebaeude_t* gebaeude_t::access_first_tile()
{
	if (tile)
	{
		const building_desc_t* const building_desc = tile->get_desc();
		const uint8 layout = tile->get_layout();
		koord k;
		for (k.x = 0; k.x<building_desc->get_x(layout); k.x++) {
			for (k.y = 0; k.y<building_desc->get_y(layout); k.y++) {
				const building_tile_desc_t *tile = building_desc->get_tile(layout, k.x, k.y);
				if (tile == NULL || !tile->has_image()) {
					continue;
				}
				if (grund_t *gr = welt->lookup(get_pos() - get_tile()->get_offset() + k))
				{
					gebaeude_t* gb;
					if (tile->get_desc()->is_signalbox())
					{
						gb = gr->get_signalbox();
					}
					else
					{
						gb = gr->find<gebaeude_t>();
					}
					if (gb && gb->get_tile() == tile)
					{
						return gb;
					}
				}
			}
		}
	}
	return this;
}

void gebaeude_t::get_description(cbuffer_t & buf) const
{
	if (is_factory && ptr.fab != NULL)
	{
		buf.append(ptr.fab->get_name());
	}
	else if (zeige_baugrube)
	{
		buf.append(translator::translate("Baustelle"));
		buf.append("\n");
	}
	else
	{
		const char *desc = tile->get_desc()->get_name();
		if (desc != NULL)
		{
			const char *trans_desc = translator::translate(desc);
			if (trans_desc == desc)
			{
				// no description here
				switch (tile->get_desc()->get_type()) {
				case building_desc_t::city_res:
					trans_desc = translator::translate("residential house");
					break;
				case building_desc_t::city_ind:
					trans_desc = translator::translate("industrial building");
					break;
				case building_desc_t::city_com:
					trans_desc = translator::translate("shops and stores");
					break;
				default:
					// use file name
					break;
				}
				buf.append(trans_desc);
			}
			else
			{
				// since the format changed, we remove all but double newlines
				char *text = new char[strlen(trans_desc) + 1];
				char *dest = text;
				const char *src = trans_desc;
				while (*src != 0)
				{
					*dest = *src;
					if (src[0] == '\n')
					{
						if (src[1] == '\n')
						{
							src++;
							dest++;
							*dest = '\n';
						}
						else
						{
							*dest = ' ';
						}
					}
					src++;
					dest++;
				}
				// remove double line breaks at the end
				*dest = 0;
				while (dest>text  &&  *--dest == '\n')
				{
					*dest = 0;
				}

				buf.append(text);
				delete[] text;
			}
		}
		else
		{
			buf.append("unknown");
		}
	}
}


void gebaeude_t::info(cbuffer_t & buf, bool dummy) const
{
	obj_t::info(buf);

	get_description(buf);

	if (!is_factory && !zeige_baugrube)
	{
		buf.append("\n");

		// belongs to which city?
		if (get_stadt() != NULL)
		{
			buf.printf(translator::translate("Town: %s\n"), ptr.stadt->get_name());
			buf.append("\n");
		}

		if (tile->get_desc()->is_signalbox())
		{
			signalbox_t* sb = (signalbox_t*)get_first_tile();
			buf.printf("%s: %d/%d\n", translator::translate("Signals"), sb->get_number_of_signals_controlled_from_this_box(), tile->get_desc()->get_capacity());

			buf.printf("%s: ", translator::translate("radius"));
			uint32 radius = tile->get_desc()->get_radius();
			if (radius == 0)
			{
				buf.append(translator::translate("infinite_range"));
			}
			else if (radius < 1000)

			{
				buf.append(radius);
				buf.append("m");
			}

			else
			{
				uint n_max;
				const double max_dist = (double)radius / 1000;
				if (max_dist < 20)
				{
					n_max = 1;
				}
				else
				{
					n_max = 0;
				}
				char number_max[10];
				number_to_string(number_max, max_dist, n_max);
				buf.append(number_max);
				buf.append("km");
			}
			buf.append("\n\n");
		}
		if (get_tile()->get_desc()->get_type() == building_desc_t::city_res)
		{
			buf.printf("%s: %d\n", translator::translate("citicens"), get_adjusted_population());
		}
		buf.printf("%s: %d\n", translator::translate("Visitor demand"), get_adjusted_visitor_demand());
#ifdef DEBUG
		buf.printf("%s (%s): %d (%d)\n", translator::translate("Jobs"), translator::translate("available"), get_adjusted_jobs(), check_remaining_available_jobs());
#else
		buf.printf("%s (%s): %d (%d)\n", translator::translate("Jobs"), translator::translate("available"), get_adjusted_jobs(), max(0, check_remaining_available_jobs()));
#endif
		buf.printf("%s: %d\n", translator::translate("Mail demand/output"), get_adjusted_mail_demand());


		building_desc_t const& h = *tile->get_desc();

		// Now all class related stuff that we just pickup from the function below:
		get_class_percentage(buf);
		buf.append("\n");



		if (get_tile()->get_desc()->get_type() == building_desc_t::city_res)
		{
			buf.printf("%s", translator::translate("Passenger success rate this year (local):"));
			if (get_passenger_success_percent_this_year_commuting() < 65535)
			{
				buf.printf(" %i%%", get_passenger_success_percent_this_year_commuting());
			}
			else
			{
				buf.printf(" 0%%");
			}

			buf.printf("\n");
			buf.printf("%s", translator::translate("Passenger success rate this year (non-local):"));
			if (get_passenger_success_percent_this_year_visiting() < 65535)
			{
				buf.printf(" %i%%", get_passenger_success_percent_this_year_visiting());
			}
			else
			{
				buf.printf(" 0%%");
			}
			buf.printf("\n");

			if (get_passenger_success_percent_last_year_commuting() < 65535)
			{
				buf.printf(translator::translate("Passenger success rate last year (local):"));
				buf.printf(" %i%%", get_passenger_success_percent_last_year_commuting());
				buf.printf("\n");
			}

			if (get_passenger_success_percent_last_year_visiting() < 65535)
			{
				buf.printf(translator::translate("Passenger success rate last year (non-local):"));
				buf.printf(" %i%%", get_passenger_success_percent_last_year_visiting());
				buf.printf("\n");
			}
		}
		else
		{
			buf.printf("%s %i\n", translator::translate("Visitors this year:"), passengers_succeeded_visiting);
			buf.printf("%s %i\n", translator::translate("Commuters this year:"), passengers_succeeded_commuting);

			if (passenger_success_percent_last_year_commuting < 65535)
			{
				buf.printf("\n%s %i\n", translator::translate("Visitors last year:"), passenger_success_percent_last_year_visiting);
			}
			if (passenger_success_percent_last_year_visiting < 65535)
			{
				buf.printf("%s %i\n", translator::translate("Commuters last year:"), passenger_success_percent_last_year_commuting);
			}
		}

		// List of stops potentially within walking distance.
		const planquadrat_t* plan = welt->access(get_pos().get_2d());
		const nearby_halt_t *const halt_list = plan->get_haltlist();
		bool any_suitable_stops_passengers = false;
		bool any_suitable_stops_mail = false;
		int total_stop_entries = plan->get_haltlist_count() - 1;
		int max_stop_entries = 6;
		int stop_entry_counter;
		uint16 max_walking_time;
		uint32 max_tiles_to_halt;
		const double km_per_tile = welt->get_settings().get_meters_per_tile() / 1000.0;

		if (plan->get_haltlist_count() > 0)
		{
			buf.append("\n");
			stop_entry_counter = 0;
			max_walking_time = 0;
			max_tiles_to_halt = 0;

			for (int h = 0; h < plan->get_haltlist_count(); h++)
			{
				const halthandle_t halt = halt_list[h].halt;
				if (halt->is_enabled(goods_manager_t::passengers))
				{
					const uint16 walking_time = welt->walking_time_tenths_from_distance(halt_list[h].distance);
					const uint32 tiles_to_halt = halt_list[h].distance;
					if (stop_entry_counter < max_stop_entries)
					{
						if (!any_suitable_stops_passengers)
						{
							buf.append(translator::translate("Stops potentially within walking distance:"));
							buf.printf("\n(%s)", translator::translate("Passagiere"));
							any_suitable_stops_passengers = true;
						}
						char walking_time_as_clock[32];
						welt->sprintf_time_tenths(walking_time_as_clock, sizeof(walking_time_as_clock), walking_time);

						buf.printf("\n  %s\n    %s: %s ", halt->get_name(), translator::translate("Walking time"), walking_time_as_clock);

						buf.append("(");
						const double km_to_halt = (double)tiles_to_halt * km_per_tile;
						if (km_to_halt < 1)
						{
							float m_to_halt = km_to_halt * 1000;
							buf.append(m_to_halt);
							buf.append("m");
						}
						else
						{
							char number_actual[10];
							number_to_string(number_actual, km_to_halt, 1);
							buf.append(number_actual);
							buf.append("km");
						}
						buf.append(")");

					}
					if (walking_time > max_walking_time)
					{
						max_walking_time = walking_time;
					}
					if (tiles_to_halt > max_tiles_to_halt)
					{
						max_tiles_to_halt = tiles_to_halt;
					}
					stop_entry_counter++;
				}
			}
			if (stop_entry_counter > max_stop_entries)
			{
				char walking_time_as_clock[32];
				welt->sprintf_time_tenths(walking_time_as_clock, sizeof(walking_time_as_clock), max_walking_time);
				buf.printf("\n");
				buf.printf(translator::translate("%i_more_stops,_max_walking_time:_%s"), stop_entry_counter - max_stop_entries, walking_time_as_clock);
				buf.append(" (");
				const double km_to_halt = (double)max_tiles_to_halt * km_per_tile;
				if (km_to_halt < 1)
				{
					float m_to_halt = km_to_halt * 1000;
					buf.append(m_to_halt);
					buf.append("m");
				}
				else
				{
					char number_actual[10];
					number_to_string(number_actual, km_to_halt, 1);
					buf.append(number_actual);
					buf.append("km");
				}
				buf.append(")");
			}

			if (any_suitable_stops_passengers)
			{
				buf.append("\n");
			}
			stop_entry_counter = 0;
			max_walking_time = 0;
			max_tiles_to_halt = 0;

			for (int h = 0; h < plan->get_haltlist_count(); h++)
			{
				const halthandle_t halt = halt_list[h].halt;
				if (halt->is_enabled(goods_manager_t::mail))
				{
					const uint16 walking_time = welt->walking_time_tenths_from_distance(halt_list[h].distance);
					const uint32 tiles_to_halt = halt_list[h].distance;
					if (stop_entry_counter <= max_stop_entries)
					{
						if (!any_suitable_stops_mail)
						{
							if (!any_suitable_stops_passengers)
							{
								buf.append(translator::translate("Stops potentially within walking distance:"));
							}
							buf.printf("\n(%s)", translator::translate("Post"));
							any_suitable_stops_mail = true;
						}
						char walking_time_as_clock[32];
						welt->sprintf_time_tenths(walking_time_as_clock, sizeof(walking_time_as_clock), walking_time);
						buf.printf("\n  %s\n    %s: %s ", halt->get_name(), translator::translate("Walking time"), walking_time_as_clock);

						buf.append("(");
						const double km_to_halt = (double)tiles_to_halt * km_per_tile;
						if (km_to_halt < 1)
						{
							float m_to_halt = km_to_halt * 1000;
							buf.append(m_to_halt);
							buf.append("m");
						}
						else
						{
							char number_actual[10];
							number_to_string(number_actual, km_to_halt, 1);
							buf.append(number_actual);
							buf.append("km");
						}
						buf.append(")");
					}
					if (walking_time > max_walking_time)
					{
						max_walking_time = walking_time;
					}
					if (tiles_to_halt > max_tiles_to_halt)
					{
						max_tiles_to_halt = tiles_to_halt;
					}
					stop_entry_counter++;
				}
			}
			if (stop_entry_counter > max_stop_entries)
			{
				char walking_time_as_clock[32];
				welt->sprintf_time_tenths(walking_time_as_clock, sizeof(walking_time_as_clock), max_walking_time);
				buf.printf("\n");
				buf.printf(translator::translate("%i_more_stops,_max_walking_time:_%s"), stop_entry_counter - max_stop_entries, walking_time_as_clock);
				buf.append(" (");
				const double km_to_halt = (double)max_tiles_to_halt * km_per_tile;
				if (km_to_halt < 1)
				{
					float m_to_halt = km_to_halt * 1000;
					buf.append(m_to_halt);
					buf.append("m");
				}
				else
				{
					char number_actual[10];
					number_to_string(number_actual, km_to_halt, 1);
					buf.append(number_actual);
					buf.append("km");
				}
				buf.append(")");
			}
			if (any_suitable_stops_mail)
			{
				buf.printf("\n");
			}
		}

		if (!any_suitable_stops_passengers)
		{
			buf.append(translator::translate("\nNo passenger stops within walking distance"));
		}

		if (!any_suitable_stops_mail)
		{
			buf.append(translator::translate("\nNo postboxes within walking distance"));
		}
		buf.printf("\n");

		buf.printf("%s%u", translator::translate("\nBauzeit von"), h.get_intro_year_month() / 12);
		if (h.get_retire_year_month() != DEFAULT_RETIRE_DATE * 12) {
			buf.printf("%s%u", translator::translate("\nBauzeit bis"), h.get_retire_year_month() / 12);
		}
		buf.append("\n");
		if (get_owner() == NULL) {
			buf.append(translator::translate("Wert"));
			buf.append(": ");
			// The land value calculation below will need modifying if multi-tile city buildings are ever introduced.
			buf.append(-(welt->get_land_value(get_pos())*(tile->get_desc()->get_level()) / 100) * 5);
			buf.append("$\n");
		}

		if (char const* const maker = tile->get_desc()->get_copyright()) {
			buf.printf(translator::translate("Constructed by %s"), maker);
		}

	}
}

void gebaeude_t::get_class_percentage(cbuffer_t & buf) const
{
	building_desc_t const& h = *tile->get_desc();
	uint8 pass_classes = goods_manager_t::passengers->get_number_of_classes();
	uint8 class_percentage[256] = { 0 };
	uint8 class_percentage_job[256] = { 0 };

	// Does this building have any class related stuff assigned?
	if (h.get_class_proportions_sum() == 0)
	{
		for (int i = 0; i < pass_classes; i++)
		{
			class_percentage[i] = 100 / pass_classes;
		}
	}

	// Apparently it does (if it continues past this point), so let's get on with the calculations!
	else
	{
		int class_proportions_sum = h.get_class_proportions_sum();
		int count_to_hundred = 0;

		// Calculate how much each class is as a percentage of the total amount
		// Remember, each class proportion is *cumulative* with all previous class proportions.
		int last_class_proportion = 0;
		for (int i = 0; i < pass_classes; i++)
		{
			class_percentage[i] = (h.get_class_proportion(i) - last_class_proportion) * 100 / class_proportions_sum;
			last_class_proportion = h.get_class_proportion(i);
			count_to_hundred += class_percentage[i];
		}

		//  We rounded down, so let's increase some of the figures to make the sum 100%
		while (count_to_hundred < 100)
		{
			for (int i = 0; i < pass_classes; i++)
			{
				if (h.get_class_proportion(i) != 0)
				{
					class_percentage[i]++;
					count_to_hundred++;
				}
				if (count_to_hundred >= 100)
				{
					break;
				}
			}
		}
	}


	// And now the poor commuters deserves the same threatment..
	if (h.get_class_proportions_sum_jobs() == 0)
	{
		for (int i = 0; i < pass_classes; i++)
		{
			class_percentage_job[i] = 100 / pass_classes;
		}
	}
	else
	{
		int class_proportions_sum = h.get_class_proportions_sum_jobs();
		int count_to_hundred = 0;
		int last_class_proportion = 0;

		for (int i = 0; i < pass_classes; i++)
		{
			class_percentage_job[i] = (h.get_class_proportion_jobs(i) - last_class_proportion) * 100 / class_proportions_sum;
			last_class_proportion = h.get_class_proportion_jobs(i);
			count_to_hundred += class_percentage_job[i];
		}

		//  We rounded down, so lets increase some of the figures to make the sum 100%
		while (count_to_hundred < 100)
		{
			for (int i = 0; i < pass_classes; i++)
			{
				if (h.get_class_proportion_jobs(i) != 0)
				{
					class_percentage_job[i]++;
					count_to_hundred++;
				}
				if (count_to_hundred >= 100)
				{
					break;
				}
			}
		}
	}


	int condition = 0; // 1 = visitors only, 2 = visitors + commuters, 3 = commuters only

	if (get_tile()->get_desc()->get_type() == building_desc_t::city_res)
	{
		buf.printf("%s:\n", translator::translate("residents_wealth"));
		condition = 1;
	}
	else if (get_adjusted_visitor_demand() > 0 && get_adjusted_jobs() > 0)
	{
		buf.printf("%s:\n", translator::translate("wealth_of_visitors_/_commuters"));
		condition = 2;
	}
	else if (get_adjusted_visitor_demand() == 0 && get_adjusted_jobs() > 0)
	{
		buf.printf("%s:\n", translator::translate("wealth_of_commuters"));
		condition = 3;
	}
	else if (get_adjusted_visitor_demand() > 0 && get_adjusted_jobs() == 0)
	{
		buf.printf("%s:\n", translator::translate("wealth_of_visitors"));
		condition = 1;
	}
	for (int i = 0; i < pass_classes; i++)
	{
		char class_name_untranslated[32];
		sprintf(class_name_untranslated, "p_class[%u]", i);
		const char* class_name = translator::translate(class_name_untranslated);
		if (condition == 1)
		{
			buf.printf("  %i%% %s\n", class_percentage[i], class_name);
		}
		else if (condition == 2)
		{
			buf.printf("  %i%% / %i%% %s\n", class_percentage[i], class_percentage_job[i], class_name);
		}
		if (condition == 3)
		{
			buf.printf("  %i%% %s\n", class_percentage_job[i], class_name);
		}
	}
}


void gebaeude_t::new_year()
{
	if (get_tile()->get_desc()->get_type() == building_desc_t::city_res)
	{
		passenger_success_percent_last_year_commuting = get_passenger_success_percent_this_year_commuting();
		passenger_success_percent_last_year_visiting = get_passenger_success_percent_this_year_visiting();
	}
	else
	{
		// For non-residential buildings, these numbers are used to record only absolute numbers of visitors/commuters.
		// Accordingly, we do not make use of "generated" numbers, and the "succeeded" figures are actually records of
		// absolute numbers of visitors/commuters. Accordingly, the last year percent figures must also store the
		// absolute number of visitors/commuters rather than a percentage.
		passenger_success_percent_last_year_commuting = passengers_succeeded_commuting;
		passenger_success_percent_last_year_visiting = passengers_succeeded_visiting;
	}

	passengers_succeeded_commuting = passengers_generated_commuting = passengers_succeeded_visiting = passengers_generated_visiting = 0;
}


void gebaeude_t::rdwr(loadsave_t *file)
{
	xml_tag_t d(file, "gebaeude_t");

	obj_t::rdwr(file);

	char buf[128];
	short idx;

	if (file->is_saving()) {
		const char *s = tile->get_desc()->get_name();
		file->rdwr_str(s);
		idx = tile->get_index();
	}
	else {
		file->rdwr_str(buf, lengthof(buf));
	}
	file->rdwr_short(idx);
	if (file->get_extended_version() <= 1)
	{
		uint32 old_purchase_time = (uint32)purchase_time;
		file->rdwr_long(old_purchase_time);
		purchase_time = old_purchase_time;
	}
	else
	{
		file->rdwr_longlong(purchase_time);
	}

	if (file->get_extended_version() >= 12)
	{
		file->rdwr_longlong(available_jobs_by_time);
	}

	if (file->is_loading()) {
		tile = hausbauer_t::find_tile(buf, idx);
		if (tile == NULL) {
			// try with compatibility list first
			tile = hausbauer_t::find_tile(translator::compatibility_name(buf), idx);
			if (tile == NULL) {
				DBG_MESSAGE("gebaeude_t::rdwr()", "neither %s nor %s, tile %i not found, try other replacement", translator::compatibility_name(buf), buf, idx);
			}
			else {
				DBG_MESSAGE("gebaeude_t::rdwr()", "%s replaced by %s, tile %i", buf, translator::compatibility_name(buf), idx);
			}
		}
		if (tile == NULL) {
			// first check for special buildings
			if (strstr(buf, "TrainStop") != NULL) {
				tile = hausbauer_t::find_tile("TrainStop", idx);
			}
			else if (strstr(buf, "BusStop") != NULL) {
				tile = hausbauer_t::find_tile("BusStop", idx);
			}
			else if (strstr(buf, "ShipStop") != NULL) {
				tile = hausbauer_t::find_tile("ShipStop", idx);
			}
			else if (strstr(buf, "PostOffice") != NULL) {
				tile = hausbauer_t::find_tile("PostOffice", idx);
			}
			else if (strstr(buf, "StationBlg") != NULL) {
				tile = hausbauer_t::find_tile("StationBlg", idx);
			}
			else {
				// try to find a fitting building
				int level = atoi(buf);
				building_desc_t::btype type = building_desc_t::unknown;

				if (level>0) {
					// May be an old 64er, so we can try some
					if (strncmp(buf + 3, "WOHN", 4) == 0) {
						type = building_desc_t::city_res;
					}
					else if (strncmp(buf + 3, "FAB", 3) == 0) {
						type = building_desc_t::city_ind;
					}
					else {
						type = building_desc_t::city_com;
					}
					level--;
				}
				else if (buf[3] == '_') {
					/* should have the form of RES/IND/COM_xx_level
					* xx is usually a number by can be anything without underscores
					*/
					level = atoi(strrchr(buf, '_') + 1);
					if (level>0) {
						switch (toupper(buf[0])) {
						case 'R': type = building_desc_t::city_res; break;
						case 'I': type = building_desc_t::city_ind; break;
						case 'C': type = building_desc_t::city_com; break;
						}
					}
					level--;
				}
				// we try to replace citybuildings with their matching counterparts
				// if none are matching, we try again without climates and timeline!
				// only 1x1 buildings can fill the empty tile to avoid overlap.
				const koord single(1,1);
				switch (type) {
				case building_desc_t::city_res:
				{
					const building_desc_t *bdsc = hausbauer_t::get_residential(level, single, welt->get_timeline_year_month(), welt->get_climate_at_height(get_pos().z));
					if (bdsc == NULL) {
						bdsc = hausbauer_t::get_residential(level, single, 0, MAX_CLIMATES);
					}
					if (bdsc) {
						dbg->message("gebaeude_t::rwdr", "replace unknown building %s with residence level %i by %s", buf, level, bdsc->get_name());
						tile = bdsc->get_tile(0);
					}
				}
				break;

				case building_desc_t::city_com:
				{
					const building_desc_t *bdsc = hausbauer_t::get_commercial(level, single, welt->get_timeline_year_month(), welt->get_climate_at_height(get_pos().z));
					if (bdsc == NULL) {
						bdsc = hausbauer_t::get_commercial(level, single, 0, MAX_CLIMATES);
					}
					if (bdsc) {
						dbg->message("gebaeude_t::rwdr", "replace unknown building %s with commercial level %i by %s", buf, level, bdsc->get_name());
						tile = bdsc->get_tile(0);
					}
				}
				break;

				case building_desc_t::city_ind:
				{
					const building_desc_t *bdsc = hausbauer_t::get_industrial(level, single, welt->get_timeline_year_month(), welt->get_climate_at_height(get_pos().z));
					if (bdsc == NULL) {
						bdsc = hausbauer_t::get_industrial(level, single, 0, MAX_CLIMATES);
						if (bdsc == NULL) {
							bdsc = hausbauer_t::get_residential(level, single, 0, MAX_CLIMATES);
						}
					}
					if (bdsc) {
						dbg->message("gebaeude_t::rwdr", "replace unknown building %s with industrie level %i by %s", buf, level, bdsc->get_name());
						tile = bdsc->get_tile(0);
					}
				}
				break;

				default:
					dbg->warning("gebaeude_t::rwdr", "description %s for building at %d,%d not found (will be removed)!", buf, get_pos().x, get_pos().y);
					welt->add_missing_paks(buf, karte_t::MISSING_BUILDING);
				}
			}
		}	// here we should have a valid tile pointer or nothing ...

			/* avoid double construction of monuments:
			* remove them from selection lists
			*/
		if (tile  &&  tile->get_desc()->get_type() == building_desc_t::monument) {
			hausbauer_t::monument_erected(tile->get_desc());
		}
		if (tile) {
			remove_ground = tile->has_image() && !tile->get_desc()->needs_ground();
		}
	}

	if (file->get_version()<99006) {
		// ignore the sync flag
		uint8 dummy = sync;
		file->rdwr_byte(dummy);
	}

	if (file->get_extended_version() >= 12)
	{
		bool f = is_factory;
		file->rdwr_bool(f);
		is_factory = f;
	}

	// restore city pointer here
	if (file->get_version() >= 99014 && !is_factory)
	{
		sint32 city_index = -1;
		if (file->is_saving() && ptr.stadt != NULL)
		{
			if (welt->get_cities().is_contained(ptr.stadt))
			{
				city_index = welt->get_cities().index_of(ptr.stadt);
			}
			else
			{
				// Reaching here means that the city has been deleted.
				ptr.stadt = NULL;
			}
		}
		file->rdwr_long(city_index);
		if (file->is_loading() && city_index != -1 && (tile == NULL || tile->get_desc() == NULL || tile->get_desc()->is_connected_with_town())) {
			ptr.stadt = welt->get_cities()[city_index];
		}
	}

	if (file->get_extended_version() >= 11)
	{
		file->rdwr_short(passengers_generated_commuting);
		file->rdwr_short(passengers_succeeded_commuting);
		if (file->get_extended_version() < 12)
		{
			uint8 old_success_percent_commuting = passenger_success_percent_last_year_commuting;
			file->rdwr_byte(old_success_percent_commuting);
			passenger_success_percent_last_year_commuting = old_success_percent_commuting;
		}
		else
		{
			file->rdwr_short(passenger_success_percent_last_year_commuting);
		}

		file->rdwr_short(passengers_generated_visiting);
		file->rdwr_short(passengers_succeeded_visiting);
		if (file->get_extended_version() < 12)
		{
			uint8 old_success_percent_visiting = passenger_success_percent_last_year_visiting;
			file->rdwr_byte(old_success_percent_visiting);
			passenger_success_percent_last_year_visiting = old_success_percent_visiting;
		}
		else
		{
			file->rdwr_short(passenger_success_percent_last_year_visiting);
		}
	}

	if (file->get_extended_version() >= 12)
	{
		file->rdwr_short(people.population); // No need to distinguish the parts of the union here.
		file->rdwr_short(jobs);
		file->rdwr_short(mail_demand);
	}

	if ((file->get_extended_version() == 13 && file->get_extended_revision() >= 1) || file->get_extended_version() >= 14)
	{
		loaded_passenger_and_mail_figres = true;

		file->rdwr_short(jobs);
		file->rdwr_short(people.visitor_demand);
		file->rdwr_short(mail_demand);

		file->rdwr_short(adjusted_jobs);
		file->rdwr_short(adjusted_people.visitor_demand);
		file->rdwr_short(adjusted_mail_demand);
	}

	if (file->is_loading())
	{
		anim_frame = 0;
		anim_time = 0;
		sync = false;

		const building_desc_t* building_type = tile->get_desc();

		if (!loaded_passenger_and_mail_figres)
		{
			if (building_type->get_type() == building_desc_t::city_res)
			{
				people.population = building_type->get_population_and_visitor_demand_capacity() == 65535 ? building_type->get_level() * welt->get_settings().get_population_per_level() : building_type->get_population_and_visitor_demand_capacity();
				adjusted_people.population = welt->calc_adjusted_monthly_figure(people.population);
				if (people.population > 0 && adjusted_people.population == 0)
				{
					adjusted_people.population = 1;
				}
			}
			else if (building_type->get_type() == building_desc_t::city_ind)
			{
				people.visitor_demand = adjusted_people.visitor_demand = 0;
			}
			else
			{
				people.visitor_demand = building_type->get_population_and_visitor_demand_capacity() == 65535 ? building_type->get_level() * welt->get_settings().get_visitor_demand_per_level() : building_type->get_population_and_visitor_demand_capacity();
				adjusted_people.visitor_demand = welt->calc_adjusted_monthly_figure(people.visitor_demand);
				if (people.visitor_demand > 0 && adjusted_people.visitor_demand == 0)
				{
					adjusted_people.visitor_demand = 1;
				}
			}

			jobs = building_type->get_employment_capacity() == 65535 ? (is_monument() || building_type->get_type() == building_desc_t::city_res) ? 0 : building_type->get_level() * welt->get_settings().get_jobs_per_level() : building_type->get_employment_capacity();
			mail_demand = building_type->get_mail_demand_and_production_capacity() == 65535 ? is_monument() ? 0 : building_type->get_level() * welt->get_settings().get_mail_per_level() : building_type->get_mail_demand_and_production_capacity();

			adjusted_jobs = welt->calc_adjusted_monthly_figure(jobs);
			if (jobs > 0 && adjusted_jobs == 0)
			{
				adjusted_jobs = 1;
			}

			adjusted_mail_demand = welt->calc_adjusted_monthly_figure(mail_demand);
			if (mail_demand > 0 && adjusted_mail_demand == 0)
			{
				adjusted_mail_demand = 1;
			}
		}

		// Hajo: rebuild tourist attraction list
		if (tile && building_type->is_attraction())
		{
			welt->add_attraction(this);
		}

		if (is_in_world_list == 0)
		{
			// Do not add this to the world list when loading a building from a factory,
			// as this needs to be taken out of the world list again, and this increases
			// loading time considerably.

			// Add this here: there is no advantage to adding buildings multi-threadedly
			// to a single list, especially when that requires an insertion sort, and
			// adding it here, single-threadedly, does not.

			welt->add_building_to_world_list(this);
		}
	}
}


/**
* Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
* um das Aussehen des Dings an Boden und Umgebung anzupassen
*
* "After loading is called adapting to the world - normally used to the
* look of the thing in the ground and surrounding area" (Google)
*
* @author Hj. Malthaner
*/
void gebaeude_t::finish_rd()
{
	calc_image();
	sint64 maint = tile->get_desc()->get_maintenance();
	if (maint == PRICE_MAGIC)
	{
		maint = welt->get_settings().maint_building*tile->get_desc()->get_level();
	}
	player_t::add_maintenance(get_owner(), maint, tile->get_desc()->get_finance_waytype());

	// citybuilding, but no town?
	if (tile->get_offset() == koord(0, 0))
	{
		if (tile->get_desc()->is_connected_with_town())
		{
			stadt_t *city = (ptr.stadt == NULL) ? welt->find_nearest_city(get_pos().get_2d()) : ptr.stadt;
			if (city)
			{
				if (!is_factory && ptr.stadt == NULL)
				{
					// This will save much time in looking this up when generating passengers/mail.
					ptr.stadt = welt->get_city(get_pos().get_2d());
				}
#ifdef MULTI_THREAD
				pthread_mutex_lock(&add_to_city_mutex);
#endif
				city->add_gebaeude_to_stadt(this, env_t::networkmode, true, false);
#ifdef MULTI_THREAD
				pthread_mutex_unlock(&add_to_city_mutex);
#endif
			}
		}
		else if (!is_factory && ptr.stadt == NULL)
		{
			// This will save much time in looking this up when generating passengers/mail.
			ptr.stadt = welt->get_city(get_pos().get_2d());
		}
	}
	else if(!is_factory && ptr.stadt == NULL)
	{
		// This will save much time in looking this up when generating passengers/mail.
		ptr.stadt = welt->get_city(get_pos().get_2d());
	}
}


void gebaeude_t::cleanup(player_t *player)
{
	//	DBG_MESSAGE("gebaeude_t::cleanup()","gb %i");
	// remove costs

	const building_desc_t* desc = tile->get_desc();
	sint64 cost = 0;

	if (desc->is_transport_building() || desc->is_signalbox())
	{
		if (desc->get_price() != PRICE_MAGIC)
		{
			cost = -desc->get_price() / 2;
		}
		else
		{
			cost = welt->get_settings().cst_multiply_remove_haus * (desc->get_level());
		}

		// If the player does not own the building, the land is not bought by bulldozing, so do not add the purchase cost.
		// (A player putting a marker on the tile will have to pay to buy the land again).
		// If the player does already own the building, the player is refunded the empty tile cost, as bulldozing a tile with a building
		// means that the player no longer owns the tile, and will have to pay again to purcahse it.

		if (player != get_owner() && desc->get_type() != building_desc_t::generic_stop) // A stop is built on top of a way, so building one does not require buying land, and, likewise, removing one does not involve releasing land.
		{
			const sint64 land_value = abs(welt->get_land_value(get_pos()) * desc->get_size().x * desc->get_size().y);
			player_t::book_construction_costs(get_owner(), land_value + cost, get_pos().get_2d(), tile->get_desc()->get_finance_waytype());
		}
		else
		{
			player_t::book_construction_costs(player, cost, get_pos().get_2d(), tile->get_desc()->get_finance_waytype());
		}
	}
	else
	{
		// Station buildings (not extension buildings, handled elsewhere) are built over existing ways, so no need to account for the land cost.

		// tearing down halts is always single costs only
		cost = desc->get_price();
		// This check is necessary because the number of PRICE_MAGIC is used if no price is specified.
		if (desc->get_base_price() == PRICE_MAGIC)
		{
			// TODO: find a way of checking what *kind* of stop that this is. This assumes railway.
			cost = welt->get_settings().cst_multiply_station * desc->get_level();
		}
		// Should be cheaper to bulldoze than build.
		cost /= 2;

		// However, the land value is restored to the player who, by bulldozing, is relinquishing ownership of the land if there are not already ways on the land.
		// Note: Cost and land value are negative numbers here.

		const sint64 land_value = welt->get_land_value(get_pos()) * desc->get_size().x * desc->get_size().y;
		if (welt->lookup(get_pos()) && !welt->lookup(get_pos())->get_weg_nr(0))
		{
			if (player == get_owner())
			{
				cost += land_value;
			}
			else
			{
				player_t::book_construction_costs(get_owner(), land_value, get_pos().get_2d(), tile->get_desc()->get_finance_waytype());
			}
		}
		player_t::book_construction_costs(player, cost, get_pos().get_2d(), tile->get_desc()->get_finance_waytype());
	}

	// may need to update next buildings, in the case of start, middle, end buildings
	if (tile->get_desc()->get_all_layouts()>1 && !is_city_building()) {

		// realign surrounding buildings...
		uint32 layout = tile->get_layout();

		// detect if we are connected at far (north/west) end
		grund_t * gr = welt->lookup(get_pos());
		if (gr) {
			sint8 offset = gr->get_weg_yoff() / TILE_HEIGHT_STEP;
			gr = welt->lookup(get_pos() + koord3d((layout & 1 ? koord::east : koord::south), offset));
			if (!gr) {
				// check whether bridge end tile
				grund_t * gr_tmp = welt->lookup(get_pos() + koord3d((layout & 1 ? koord::east : koord::south), offset - 1));
				if (gr_tmp && gr_tmp->get_weg_yoff() / TILE_HEIGHT_STEP == 1) {
					gr = gr_tmp;
				}
			}
			if (gr) {
				gebaeude_t* gb = gr->find<gebaeude_t>();
				if (gb  &&  gb->get_tile()->get_desc()->get_all_layouts()>4u) {
					koord xy = gb->get_tile()->get_offset();
					uint8 layoutbase = gb->get_tile()->get_layout();
					if ((layoutbase & 1u) == (layout & 1u)) {
						layoutbase |= 4u; // set far bit on neighbour
						gb->set_tile(gb->get_tile()->get_desc()->get_tile(layoutbase, xy.x, xy.y), false);
					}
				}
			}

			// detect if near (south/east) end
			gr = welt->lookup(get_pos() + koord3d((layout & 1 ? koord::west : koord::north), offset));
			if (!gr) {
				// check whether bridge end tile
				grund_t * gr_tmp = welt->lookup(get_pos() + koord3d((layout & 1 ? koord::west : koord::north), offset - 1));
				if (gr_tmp && gr_tmp->get_weg_yoff() / TILE_HEIGHT_STEP == 1) {
					gr = gr_tmp;
				}
			}
			if (gr) {
				gebaeude_t* gb = gr->find<gebaeude_t>();
				if (gb  &&  gb->get_tile()->get_desc()->get_all_layouts()>4) {
					koord xy = gb->get_tile()->get_offset();
					uint8 layoutbase = gb->get_tile()->get_layout();
					if ((layoutbase & 1u) == (layout & 1u)) {
						layoutbase |= 2u; // set near bit on neighbour
						gb->set_tile(gb->get_tile()->get_desc()->get_tile(layoutbase, xy.x, xy.y), false);
					}
				}
			}
		}
	}
	mark_images_dirty();
}


void gebaeude_t::mark_images_dirty() const
{
	// remove all traces from the screen
	image_id img;
	if (zeige_baugrube ||
		(!env_t::hide_with_transparency  &&
			env_t::hide_buildings>(is_city_building() ? env_t::NOT_HIDE : env_t::SOME_HIDDEN_BUILDING))) {
		img = skinverwaltung_t::construction_site->get_image_id(0);
	}
	else {
		img = tile->get_background(anim_frame, 0, season);
	}
	for (int i = 0; img != IMG_EMPTY; img = get_image(++i)) {
		mark_image_dirty(img, -(i*get_tile_raster_width()));
	}
}

uint16 gebaeude_t::get_weight() const
{
	return tile->get_desc()->get_level();
}

void gebaeude_t::set_commute_trip(uint16 number)
{
	// Record the number of arriving workers by encoding the earliest time at which new workers can arrive.
	const sint64 job_ticks = ((sint64)number * welt->get_settings().get_job_replenishment_ticks()) / ((sint64)adjusted_jobs < 1ll ? 1ll : (sint64)adjusted_jobs);
	const sint64 new_jobs_by_time = welt->get_ticks() - welt->get_settings().get_job_replenishment_ticks();
	available_jobs_by_time = std::max(new_jobs_by_time + job_ticks, available_jobs_by_time + job_ticks);
	add_passengers_succeeded_commuting(number);
}


uint16 gebaeude_t::get_adjusted_population() const
{
	return tile->get_desc()->get_type() == building_desc_t::city_res ? adjusted_people.population : 0;
}

uint16 gebaeude_t::get_visitor_demand() const
{
	if (tile->get_desc()->get_type() != building_desc_t::city_res)
	{
		return people.visitor_demand;
	}

	uint16 reduced_demand = people.population / 20;
	return reduced_demand > 0 ? reduced_demand : 1;
}

uint16 gebaeude_t::get_adjusted_visitor_demand() const
{
	if (tile->get_desc()->get_type() != building_desc_t::city_res)
	{
		return adjusted_people.visitor_demand;
	}

	uint16 reduced_demand = adjusted_people.population / 20;
	return reduced_demand > 0 ? reduced_demand : 1;
}

sint32 gebaeude_t::check_remaining_available_jobs() const
{
	// Commenting out the "if(!jobs_available())" code will allow jobs to be shown as negative.
	/*if(!jobs_available())
	{
	// All the jobs are taken for the time being.
	//return 0;
	}
	else
	{*/
	if (available_jobs_by_time < welt->get_ticks() - welt->get_settings().get_job_replenishment_ticks())
	{
		// Uninitialised or stale - all jobs available
		return (sint64)adjusted_jobs;
	}
	const sint64 delta_t = welt->get_ticks() - available_jobs_by_time;
	const sint64 remaining_jobs = delta_t * (sint64)adjusted_jobs / welt->get_settings().get_job_replenishment_ticks();
	return (sint32)remaining_jobs;
	//}
}

sint32 gebaeude_t::get_staffing_level_percentage() const
{
	if (adjusted_jobs == 0)
	{
		return 100;
	}
	const sint32 percentage = (adjusted_jobs - check_remaining_available_jobs()) * 100 / adjusted_jobs;
	return percentage;
}

bool gebaeude_t::jobs_available() const
{
	const sint64 ticks = welt->get_ticks();
	bool difference = available_jobs_by_time <= ticks;
	return difference;
}

uint8 gebaeude_t::get_random_class(const goods_desc_t * wtyp)
{
	// This currently simply uses the building type's proportions.
	// TODO: Allow this to be modified when dynamic building occupation
	// is introduced with the (eventual) new town growth code.

	// At present, mail classes are handled rather badly.

	const uint8 number_of_classes = goods_manager_t::passengers->get_number_of_classes();

	if (number_of_classes == 1)
	{
		return 0;
	}

	const uint32 sum = get_tile()->get_desc()->get_class_proportions_sum();

	if (sum == 0 || wtyp != goods_manager_t::passengers)
	{
		// If the building has a zero sum of class proportions, as is the default, assume
		// an equal chance of any given class being generated from here.
		// Also, we don't have sensible figures to use for mail.
		return (uint8)simrand(wtyp->get_number_of_classes(), "uint8 gebaeude_t::get_random_class() const (fixed)");
	}

	const uint16 random = simrand(sum, "uint8 gebaeude_t::get_random_class() const (multiple classes)");

	uint8 g_class = 0;

	for (uint8 i = 0; i < number_of_classes; i++)
	{
		if (random < get_tile()->get_desc()->get_class_proportion(i))
		{
			g_class = i;
			break;
		}
	}

	return g_class;
}

const minivec_tpl<const planquadrat_t*> &gebaeude_t::get_tiles()
{
	const building_tile_desc_t* tile = get_tile();
	const building_desc_t *bdsc = tile->get_desc();
	const koord size = bdsc->get_size(tile->get_layout());
	if (building_tiles.empty())
	{
#ifdef MULTI_THREAD
		int mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
#endif
		// Clear just in case another thread has just run this algorithm on the same building.
		building_tiles.clear();
		if (size == koord(1, 1))
		{
			// A single tiled building - just add the single tile.
			building_tiles.append(welt->access_nocheck(get_pos().get_2d()));
		}
		else
		{
			// A multi-tiled building: check all tiles. Any tile within the
			// coverage radius of a building connects the whole building.

			// Then, store these tiles here, as this is computationally expensive
			// and frequently requested by the passenger/mail generation algorithm.

			koord3d k = get_pos();
			const koord start_pos = k.get_2d() - tile->get_offset();
			const koord end_pos = k.get_2d() + size;

			for (k.y = start_pos.y; k.y < end_pos.y; k.y++)
			{
				for (k.x = start_pos.x; k.x < end_pos.x; k.x++)
				{
					grund_t *gr = welt->lookup(k);
					if (gr)
					{
						/* This would fail for depots, but those are 1x1 buildings */
						gebaeude_t *gb_part = gr->find<gebaeude_t>();
						// There may be buildings with holes.
						if (gb_part && gb_part->get_tile()->get_desc() == bdsc)
						{
							const planquadrat_t* plan = welt->access_nocheck(k.get_2d());
							building_tiles.append(plan);
						}
					}
				}
			}
		}
#ifdef MULTI_THREAD
		mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
#endif
	}
	return building_tiles;
}
