/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <math.h>

#include "../simdebug.h"
#include "../world/simworld.h"
#include "../obj/simobj.h"
#include "../display/simimg.h"
#include "../player/simplay.h"
#include "../simtypes.h"
#include "../simunits.h"

#include "../ground/grund.h"

#include "../descriptor/groundobj_desc.h"

#include "../utils/cbuffer.h"
#include "../utils/simrandom.h"
#include "../utils/simstring.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../obj/baum.h"

#include "movingobj.h"

/******************************** static stuff: desc management ****************************************************************/

vector_tpl<const groundobj_desc_t *> movingobj_t::movingobj_typen(0);

stringhashtable_tpl<groundobj_desc_t *> movingobj_t::desc_table;


bool compare_groundobj_desc(const groundobj_desc_t* a, const groundobj_desc_t* b);


bool movingobj_t::successfully_loaded()
{
	movingobj_typen.resize(desc_table.get_count());
	for(auto const& i : desc_table) {
		movingobj_typen.insert_ordered(i.value, compare_groundobj_desc);
	}
	// iterate again to assign the index
	for(auto const& i : desc_table) {
		i.value->index = movingobj_typen.index_of(i.value);
	}

	if(desc_table.empty()) {
		movingobj_typen.append( NULL );
		DBG_MESSAGE("movingobj_t", "No movingobj found - feature disabled");
	}
	return true;
}



bool movingobj_t::register_desc(groundobj_desc_t *desc)
{
	// remove duplicates
	if(  desc_table.remove( desc->get_name() )  ) {
		dbg->doubled( "movingobj", desc->get_name() );
	}
	desc_table.put(desc->get_name(), desc );
	return true;
}




/** also checks for distribution values
 */
const groundobj_desc_t *movingobj_t::random_movingobj_for_climate(climate cl)
{
	// none there
	if(  desc_table.empty()  ) {
		return NULL;
	}

	int weight = 0;

	for(groundobj_desc_t const* const i : movingobj_typen) {
		if (i->is_allowed_climate(cl) ) {
			weight += i->get_distribution_weight();
		}
	}

	// now weight their distribution
	if (weight > 0) {
		const int w=simrand(weight);
		weight = 0;
		for(groundobj_desc_t const* const i : movingobj_typen) {
			if (i->is_allowed_climate(cl)) {
				weight += i->get_distribution_weight();
				if(weight>=w) {
					return i;
				}
			}
		}
	}
	return NULL;
}



/******************************* end of static ******************************************/



// recalculates only the seasonal image
void movingobj_t::calc_image()
{
	const groundobj_desc_t *desc=get_desc();
	const uint8 seasons = desc->get_seasons()-1;
	uint8 season = 0;

	switch(  seasons  ) {
		case 0: { // summer only
			season = 0;
			break;
		}
		case 1: { // summer, snow
			season = welt->get_snowline() <= get_pos().z  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate;
			break;
		}
		case 2: { // summer, winter, snow
			season = welt->get_snowline() <= get_pos().z  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate ? 2 : welt->get_season() == 1;
			break;
		}
		default: {
			if(  welt->get_snowline() <= get_pos().z  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate  ) {
				season = seasons;
			}
			else {
				// resolution 1/8th month (0..95)
				const uint32 yearsteps = (welt->get_current_month()%12)*8 + ((welt->get_ticks()>>(welt->ticks_per_world_month_shift-3))&7) + 1;
				season = (seasons * yearsteps - 1) / 96;
			}
			break;
		}
	}
	set_image( get_desc()->get_image( season, ribi_t::get_dir(get_direction()) )->get_id() );
}


movingobj_t::movingobj_t(loadsave_t *file) : vehicle_base_t()
{
	rdwr(file);
	if(get_desc()) {
		welt->sync.add( this );
	}
}


movingobj_t::movingobj_t(koord3d pos, const groundobj_desc_t *b ) : vehicle_base_t(pos)
{
	movingobjtype = movingobj_typen.index_of(b);
	weg_next = 0;
	timetochange = 0; // will do random direct change anyway during next step
	direction = calc_set_direction( koord3d(0,0,0), koord3d(koord::west,0) );
	calc_image();
	welt->sync.add( this );
}


movingobj_t::~movingobj_t()
{
	welt->sync.remove( this );
}


bool movingobj_t::check_season(const bool)
{
	const image_id old_image = get_image();
	calc_image();

	if(  get_image() != old_image  ) {
		mark_image_dirty( get_image(), 0 );
	}
	return true;
}


void movingobj_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "movingobj_t" );

	vehicle_base_t::rdwr(file);

	file->rdwr_enum(direction);
	if (file->is_loading()) {
		// restore dxdy information
		dx = dxdy[ ribi_t::get_dir(direction)*2];
		dy = dxdy[ ribi_t::get_dir(direction)*2+1];
	}

	file->rdwr_byte(steps);
	file->rdwr_byte(steps_next);

	pos_next.rdwr(file);

	koord p = pos_next_next.get_2d();
	p.rdwr(file);
	if(file->is_loading()) {
		pos_next_next = koord3d(p, 0);
		// z-coordinate will be restored in hop_check
	}

	file->rdwr_short(timetochange);

	if(file->is_saving()) {
		const char *s = get_desc()->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));
		groundobj_desc_t *desc = desc_table.get(bname);
		if(  desc_table.empty()  ||  desc==NULL  ) {
			movingobjtype = simrand(movingobj_typen.get_count());
		}
		else {
			movingobjtype = (uint8)desc->get_index();
		}
		// if not there, desc will be zero

		use_calc_height = true;
	}

	if (file->is_version_atleast(122, 1)) {
		file->rdwr_long(weg_next);
	}
	else if (file->is_loading()) {
		weg_next = 0;
	}
}


void movingobj_t::show_info()
{
	if(env_t::tree_info) {
		obj_t::show_info();
	}
}


void movingobj_t::info(cbuffer_t & buf) const
{
	obj_t::info(buf);

	buf.append(translator::translate(get_desc()->get_name()));
	if (char const* const maker = get_desc()->get_copyright()) {
		buf.append("\n");
		buf.printf(translator::translate("Constructed by %s"), maker);
	}
	buf.append("\n");
	buf.append(translator::translate("cost for removal"));
	char buffer[128];
	money_to_string( buffer, get_desc()->get_price()/100.0 );
	buf.append( buffer );
}


void movingobj_t::cleanup(player_t *player)
{
	player_t::book_construction_costs(player, -get_desc()->get_price(), get_pos().get_2d(), ignore_wt);
	mark_image_dirty( get_image(), 0 );
}


sync_result movingobj_t::sync_step(uint32 delta_t)
{
	weg_next += get_desc()->get_speed() * delta_t;
	weg_next -= do_drive( weg_next );
	return SYNC_OK;
}


/* essential to find out about next step
 * returns true, if we can go here
 * (identical to fahrer)
 */
bool movingobj_t::check_next_tile( const grund_t *gr ) const
{
	if(gr==NULL) {
		// no ground => we cannot check further
		return false;
	}

	const groundobj_desc_t *desc = get_desc();
	if( !desc->is_allowed_climate( welt->get_climate(gr->get_pos().get_2d()) ) ) {
		// not an allowed climate zone!
		return false;
	}

	if(desc->get_waytype()==road_wt) {
		// can cross roads
		if(gr->get_typ()!=grund_t::boden  ||  !slope_t::is_way(gr->get_grund_hang())) {
			return false;
		}
		// only on roads, do not walk in cities
		if(gr->hat_wege()  &&  (!gr->hat_weg(road_wt)  ||  gr->get_weg(road_wt)->hat_gehweg())) {
			return false;
		}
		if(!desc->can_build_trees_here()) {
			return gr->find<baum_t>()==NULL;
		}
	}
	else if(desc->get_waytype()==air_wt) {
		// avoid towns to avoid flying through houses
		return gr->get_typ()==grund_t::boden  ||  gr->get_typ()==grund_t::wasser;
	}
	else if(desc->get_waytype()==water_wt) {
		// floating object
		return gr->get_typ()==grund_t::wasser  ||  gr->hat_weg(water_wt);
	}
	else if(desc->get_waytype()==ignore_wt) {
		// crosses nothing
		if(!gr->ist_natur()  ||  !slope_t::is_way(gr->get_grund_hang())) {
			return false;
		}
		if(!desc->can_build_trees_here()) {
			return gr->find<baum_t>()==NULL;
		}
	}
	return true;
}



grund_t* movingobj_t::hop_check()
{
	/* since we may be going diagonal without any road
	 * determining the next koord is a little tricky:
	 * If it is a diagonal, pos_next_next is calculated from current pos,
	 * Else pos_next_next is a single step from pos_next.
	 * otherwise objects would jump left/right on some diagonals
	 */
	if (timetochange != 0) {
		koord k(direction);
		if(k.x&k.y) {
			pos_next_next = get_pos() + k;
		}
		else {
			pos_next_next = pos_next + k;
		}

		grund_t *gr = welt->lookup_kartenboden(pos_next_next.get_2d());
		if (check_next_tile(gr)) {
			pos_next_next = gr->get_pos();
		}
		else {
			timetochange = 0;
		}
	}

	if (timetochange==0) {
		// direction change needed
		timetochange = simrand(speed_to_kmh(get_desc()->get_speed())/3);
		const koord pos=pos_next.get_2d();
		const grund_t *to[4];
		uint8 until=0;
		// find all tiles we can go
		for(  int i=0;  i<4;  i++  ) {
			const grund_t *check = welt->lookup_kartenboden(pos+koord::nesw[i]);
			if(check_next_tile(check)  &&  check->get_pos()!=get_pos()) {
				to[until++] = check;
			}
		}
		// if nothing found, return
		if(until==0) {
			pos_next_next = get_pos();
			// (better would be destruction?)
		}
		else {
			// else prepare for direction change
			const grund_t *next = to[simrand(until)];
			pos_next_next = next->get_pos();
		}
	}
	else {
		timetochange--;
	}

	return welt->lookup(pos_next);
}



void movingobj_t::hop(grund_t* gr)
{
	leave_tile();

	if(pos_next.get_2d()==get_pos().get_2d()) {
		direction = ribi_t::backward(direction);
		dx = -dx;
		dy = -dy;
		calc_image();
	}
	else {
		ribi_t::ribi old_dir = direction;
		direction = calc_set_direction( get_pos(), pos_next_next );
		if(old_dir!=direction) {
			calc_image();
		}
	}

	set_pos(pos_next);
	enter_tile(gr);

	// next position
	pos_next = pos_next_next;
}



void *movingobj_t::operator new(size_t /*s*/)
{
	return freelist_t::gimme_node(sizeof(movingobj_t));
}



void movingobj_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(movingobj_t),p);
}
