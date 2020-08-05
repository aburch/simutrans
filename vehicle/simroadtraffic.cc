/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simdebug.h"
#include "../display/simgraph.h"
#include "../simmesg.h"
#include "../simworld.h"
#include "../utils/simrandom.h"
#include "../display/simimg.h"
#include "../display/viewport.h"
#include "../simunits.h"
#include "../simtypes.h"
#include "../simcity.h"
#include "../simconvoi.h"
#include "../player/simplay.h"

#include "simroadtraffic.h"
#include "simpeople.h"

#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"

#include "../obj/crossing.h"
#include "../obj/roadsign.h"

#include "../boden/grund.h"
#include "../boden/wege/weg.h"
#include "../boden/wege/strasse.h"
#include "../boden/wege/strasse.h"
#include "../descriptor/citycar_desc.h"
#include "../descriptor/roadsign_desc.h"


#include "../utils/cbuffer_t.h"

/**********************************************************************************************************************/
/* Road users (Verkehrsteilnehmer) basis class from here on */

#ifdef INLINE_OBJ_TYPE
road_user_t::road_user_t(typ type) :
	vehicle_base_t(type)
#else
road_user_t::road_user_t() :
	vehicle_base_t()
#endif
{
	set_owner( welt->get_public_player() );
	time_to_life = 0;
	weg_next = 0;
	tiles_since_last_increment = 0;
}


/**
 * Ensures that this object is removed correctly from the list
 * of sync step-able things!
 * @author Hj. Malthaner
 */
road_user_t::~road_user_t()
{
	mark_image_dirty( get_image(), 0 );
}


#ifdef INLINE_OBJ_TYPE
road_user_t::road_user_t(typ type, grund_t* bd, uint16 random) :
	vehicle_base_t(type, bd ? bd->get_pos() : koord3d::invalid)
#else
road_user_t::road_user_t(grund_t* bd, uint16 random) :
	vehicle_base_t(welt, bd ? bd->get_pos() : koord3d::invalid)
#endif
	, tiles_since_last_increment(0)
{
	ribi_t::ribi road_ribi = bd ? bd->get_weg_ribi(road_wt) : ribi_t::all;

	weg_next = random;

	// randomized offset
	uint8 offset = random & 3;
	direction = ribi_t::nsew[offset];

	grund_t *to = NULL;
	for(uint8 r = 0; r < 4; r++) {
		ribi_t::ribi ribi = ribi_t::nsew[ (r + offset) &3];
		if( (ribi & road_ribi)!=0  &&  bd->get_neighbour(to, road_wt, ribi)) {
			direction = ribi;
			break;
		}
	}

	switch(direction) {
		case ribi_t::north:
			dx = 2;
			dy = -1;
			break;
		case ribi_t::south:
			dx = -2;
			dy = 1;
			break;
		case ribi_t::east:
			dx = 2;
			dy = 1;
			break;
		case ribi_t::west:
			dx = -2;
			dy = -1;
			break;
	}

	set_xoff((dx<0) ? OBJECT_OFFSET_STEPS : -OBJECT_OFFSET_STEPS);
	set_yoff((dy<0) ? OBJECT_OFFSET_STEPS / 2 : -OBJECT_OFFSET_STEPS / 2);

	if(to) {
		pos_next = to->get_pos();
	}
	else {
		const grund_t* gr = welt->lookup_kartenboden(get_pos().get_2d() + koord(direction));
		pos_next = gr ? gr->get_pos() : koord3d::invalid;
		if (!gr)
		{
			time_to_life = 0; // Destroy invalid objects
		}
	}
	set_owner( welt->get_public_player() );
}


/**
 * Open a new observation window for the object.
 * @author Hj. Malthaner
 */
void road_user_t::show_info()
{
	if(env_t::road_user_info) {
		obj_t::show_info();
	}
}

uint32 private_car_t::get_max_speed()
{
	return get_desc()->get_topspeed();
}


void road_user_t::hop(grund_t *)
{
	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *from = welt->lookup(pos_next);

	if(!from) {
		time_to_life = 0;
		return;
	}

	// It is not clear why this is necessary in
	// Extended but not in Standard, but without
	// this, remnents of images will appear
	// whenever vehicles move off or reverse.
	from->mark_image_dirty();

	grund_t *list[4];
	int count = 0;

	// 1) find the allowed directions
	weg_t *weg = from->get_weg(road_wt);
	if(weg==NULL) {
		// no gound here any more?
		pos_next = get_pos();
		from = welt->lookup(pos_next);
		if(!from  ||  !from->hat_weg(road_wt)) {
			// destroy it
			time_to_life = 0;
		}
		return;
	}

	grund_t *to;
	// add all good ribis here
	ribi_t::ribi reverse_direction = ribi_t::backward( get_direction() );
	int ribi = weg->get_ribi_unmasked();
	for(int r = 0; r < 4; r++) {
		if(  (ribi & ribi_t::nsew[r])!=0  &&  (ribi_t::nsew[r]&reverse_direction)==0 &&
			from->get_neighbour(to, road_wt, ribi_t::nsew[r])
		) {
			// check, if this is just a single tile deep
			int next_ribi =  to->get_weg(road_wt)->get_ribi_unmasked();
			if((ribi&next_ribi)!=0  ||  !ribi_t::is_single(next_ribi)) {
				list[count++] = to;
			}
		}
	}

	if(count > 1) {
#ifdef DEBUG_SIMRAND_CALLS
		char buf[256];
		sprintf(buf, "road_user_t::hop() %s", typeid(*this).name());
		pos_next = list[simrand(count, buf)]->get_pos();
#else
		pos_next = list[simrand(count, "road_user_t::hop()")]->get_pos();
#endif
		direction = calc_set_direction(get_pos(), pos_next);
	} else if(count==1) {
		pos_next = list[0]->get_pos();
		direction = calc_set_direction(get_pos(), pos_next);
	}
	else {
		direction = reverse_direction;
		dx = -dx;
		dy = -dy;
		pos_next = get_pos();
	}
	leave_tile();
	set_pos(from->get_pos());
	calc_image();
}



void road_user_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "road_user_t" );

	sint8 hoff = file->is_saving() ? get_hoff() : 0;

	// correct old offsets ... REMOVE after savegame increase ...
	if(file->get_version()<99018  &&  file->is_saving()) {
		dx = dxdy[ ribi_t::get_dir(direction)*2 ];
		dy = dxdy[ ribi_t::get_dir(direction)*2+1 ];
		sint8 i = steps/16;
		set_xoff( get_xoff() + i*dx );
		set_yoff( get_hoff() + i*dy + hoff );
	}

	vehicle_base_t::rdwr(file);

	if(file->get_version() < 86006) {
		sint32 l;
		file->rdwr_long(l);
		file->rdwr_long(l);
		file->rdwr_long(weg_next);
		file->rdwr_long(l);
		dx = (sint8)l;
		file->rdwr_long(l);
		dy = (sint8)l;
		file->rdwr_enum(direction);
		file->rdwr_long(l);
		hoff = (sint8)l;
	}
	else {
		if(file->get_version()<99005) {
			sint32 dummy32;
			file->rdwr_long(dummy32);
		}
		file->rdwr_long(weg_next);
		if(file->get_version()<99018) {
			file->rdwr_byte(dx);
			file->rdwr_byte(dy);
		}
		else {
			file->rdwr_byte(steps);
			file->rdwr_byte(steps_next);
		}
		file->rdwr_enum(direction);
		dx = dxdy[ ribi_t::get_dir(direction)*2];
		dy = dxdy[ ribi_t::get_dir(direction)*2+1];
		if(file->get_version()<99005  ||  file->get_version()>99016) {
			sint16 dummy16 = ((16*(sint16)hoff)/OBJECT_OFFSET_STEPS);
			file->rdwr_short(dummy16);
			hoff = (sint8)((OBJECT_OFFSET_STEPS*(sint16)dummy16)/16);
		}
		else {
			file->rdwr_byte(hoff);
		}
	}
	pos_next.rdwr(file);

	// convert steps to position
	if(file->get_version()<99018) {
		sint8 ddx=get_xoff(), ddy=get_hoff()-hoff;
		sint8 i=0;

		while(  !is_about_to_hop(ddx+dx*i,ddy+dy*i )  &&  i<16 ) {
			i++;
		}
		set_xoff( ddx-(16-i)*dx );
		set_yoff( ddy-(16-i)*dy );
		if(file->is_loading()) {
			if(dx && dy) {
				steps = min( VEHICLE_STEPS_PER_TILE - 1, VEHICLE_STEPS_PER_TILE - 1 - (i*16) );
				steps_next = VEHICLE_STEPS_PER_TILE - 1;
			}
			else {
				steps = min( VEHICLE_STEPS_PER_TILE/2 - 1, VEHICLE_STEPS_PER_TILE / 2 -(i*16) );
				steps_next = VEHICLE_STEPS_PER_TILE/2 ;
			}
		}
	}

	// the lifetime in ms
	if(file->get_version()>89004) {
		file->rdwr_long(time_to_life);
	}

	if(file->get_extended_version() >= 11)
	{
		file->rdwr_byte(tiles_since_last_increment);
	}
	else if(env_t::networkmode)
	{
		tiles_since_last_increment = 0;
	}

	// there might be crashes if world is destroyed after loading
	// without a sync-step being performed
	if(file->is_loading()  &&  time_to_life<=0) {
		time_to_life = 1;
	}

	// Hajo: avoid endless growth of the values
	// this causes lockups near 2**32
	weg_next &= 65535;
}

void road_user_t::finish_rd()
{
	calc_height(NULL);
	calc_image();
}



/**********************************************************************************************************************/
/* private_car_t (city cars) from here on */


static weighted_vector_tpl<const citycar_desc_t*> liste_timeline;
stringhashtable_tpl<const citycar_desc_t *> private_car_t::table;

bool private_car_t::register_desc(const citycar_desc_t *desc)
{
	if(  table.remove(desc->get_name())  ) {
		dbg->warning( "citycar_desc_t::register_desc()", "Object %s was overlaid by addon!", desc->get_name() );
	}
	table.put(desc->get_name(), desc);
	return true;
}



bool private_car_t::successfully_loaded()
{
	if(table.empty()) {
		DBG_MESSAGE("private_car_t", "No citycars found - feature disabled");
	}
	return true;
}


static bool compare_stadtauto_desc(const citycar_desc_t* a, const citycar_desc_t* b)
{
	int diff = a->get_intro_year_month() - b->get_intro_year_month();
	if (diff == 0) {
		diff = a->get_topspeed() - b->get_topspeed();
	}
	if (diff == 0) {
		/* same Level - we introduce an artificial, but unique resort
		 * on the induced name. */
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}


void private_car_t::build_timeline_list(karte_t *welt)
{
	// this list will contain all citycars
	liste_timeline.clear();
	vector_tpl<const citycar_desc_t*> temp_liste(0);
	if(  !table.empty()  ) {
		const int month_now = welt->get_current_month();
//DBG_DEBUG("private_car_t::build_timeline_list()","year=%i, month=%i", month_now/12, month_now%12+1);

		// check for every citycar, if still ok ...
		FOR(stringhashtable_tpl<citycar_desc_t const*>, const& i, table) {
			citycar_desc_t const* const info = i.value;
			const int intro_month = info->get_intro_year_month();
			const int retire_month = info->get_retire_year_month();

			if (!welt->use_timeline() || (intro_month <= month_now && month_now < retire_month)) {
				temp_liste.insert_ordered( info, compare_stadtauto_desc );
			}
		}
	}
	liste_timeline.resize( temp_liste.get_count() );
	FOR(vector_tpl<citycar_desc_t const*>, const i, temp_liste) {
		liste_timeline.append(i, i->get_distribution_weight());
	}
}

bool private_car_t::list_empty()
{
	return liste_timeline.empty();
}


private_car_t::~private_car_t()
{
	// first: release crossing
	grund_t *gr = welt->lookup(get_pos());
	if(gr  &&  gr->ist_uebergang()) {
		gr->find<crossing_t>(2)->release_crossing(this);
	}

	if (gr)
	{
		gr->mark_image_dirty();
	}

	// just to be sure we are removed from this list!
	if(time_to_life>0) {
		welt->sync.remove(this);
	}
}


private_car_t::private_car_t(loadsave_t *file) :
#ifdef INLINE_OBJ_TYPE
	road_user_t(obj_t::road_user)
#else
	road_user_t()
#endif
{
	rdwr(file);

	if(desc) {
		welt->sync.add(this);
	}
	reset_measurements();
}


private_car_t::private_car_t(grund_t* gr, koord const target) :
#ifdef INLINE_OBJ_TYPE
	road_user_t(obj_t::road_user, gr, simrand(65535, "private_car_t::private_car_t (weg_next)")),
#else
	road_user_t(gr, simrand(65535, "private_car_t::private_car_t (weg_next)")),
#endif
	desc(liste_timeline.empty() ? 0 : pick_any_weighted(liste_timeline))
{
	pos_next_next = koord3d::invalid;
	time_to_life = welt->get_settings().get_stadtauto_duration() << welt->ticks_per_world_month_shift;
	current_speed = 48;
	ms_traffic_jam = 2147483647;
	this->target = target;
	calc_image();
	origin = gr ? gr->get_pos().get_2d() : koord::invalid;
	last_tile_marked_as_stopped = koord3d::invalid;
	reset_measurements();
}


sync_result private_car_t::sync_step(uint32 delta_t)
{
	time_to_life -= delta_t;
	if(  time_to_life<=0 ) {
		return SYNC_DELETE;
	}

	if(  current_speed==0  ) {
		// stuck in traffic jam
		uint32 old_ms_traffic_jam = ms_traffic_jam;
		ms_traffic_jam += delta_t;
		// check only every 1.024 s if stopped
		if(  (ms_traffic_jam>>10) != (old_ms_traffic_jam>>10)  ) {
			pos_next_next = koord3d::invalid;
			if(  hop_check()  ) {
				ms_traffic_jam = 0;
				current_speed = 48;
			}
			else {
				if(ms_traffic_jam>welt->ticks_per_world_month  &&  old_ms_traffic_jam<=welt->ticks_per_world_month)
				{
					// message after two month, reset waiting timer
					welt->get_message()->add_message( translator::translate("To heavy traffic\nresults in traffic jam.\n"), get_pos().get_2d(), message_t::traffic_jams, COL_ORANGE );
				}
			}
		}
		weg_next = 0;
	}
	else
	{
		if(ms_traffic_jam == 2147483647)
		{
			ms_traffic_jam = 0;
			// If this is the first step, get the car moving at a sensible speed as soon as possible.
			weg_next += kmh_to_speed(welt->get_settings().get_town_road_speed_limit()) * delta_t;
			steps_next = 1;
		}
		else
		{
			weg_next += current_speed * delta_t;
		}
		const uint32 distance = do_drive( weg_next );
		add_distance(distance);
		// hop_check could have set weg_next to zero, check for possible underflow here
		if (weg_next > distance) {
			weg_next -= distance;
		}
		else {
			weg_next = 0;
		}
	}

	return time_to_life>0 ? SYNC_OK : SYNC_DELETE;
}


void private_car_t::rdwr(loadsave_t *file)
{
	xml_tag_t s( file, "private_car_t" );

	road_user_t::rdwr(file);

	if(file->is_saving()) {
		const char *s = desc->get_name();
		file->rdwr_str(s);
	}
	else {
		char s[256];
		file->rdwr_str(s, lengthof(s));
		desc = table.get(s);

		if(  desc == 0  &&  !liste_timeline.empty()  ) {
			dbg->warning("private_car_t::rdwr()", "Object '%s' not found in table, trying random stadtauto object type",s);
			desc = pick_any_weighted(liste_timeline);
		}

		if(desc == 0) {
			dbg->warning("private_car_t::rdwr()", "loading game with private cars, but no private car objects found in PAK files.");
		}
		else {
			set_image(desc->get_image_id(ribi_t::get_dir(get_direction())));
		}
	}

	if(file->get_version() <= 86001) {
		time_to_life = simrand(1000000, "void private_car_t::rdwr")+10000;
	}
	else if(file->get_version() <= 89004) {
		file->rdwr_long(time_to_life);
		time_to_life *= 10000;	// converting from hops left to ms since start
	}

	if(file->get_version() <= 86004) {
		// default starting speed for old games
		if(file->is_loading()) {
			current_speed = 48;
		}
	}
	else {
		sint32 dummy32=current_speed;
		file->rdwr_long(dummy32);
		current_speed = dummy32;
	}

	if(file->get_version() <= 99010) {
		pos_next_next = koord3d::invalid;
	}
	else {
		pos_next_next.rdwr(file);
	}

	// overtaking status
	if(file->get_version()<100001) {
		set_tiles_overtaking( 0 );
	}
	else {
		file->rdwr_byte(tiles_overtaking);
		set_tiles_overtaking( tiles_overtaking );
	}

	if(file->get_extended_version() >= 10 && file->get_version() >= 111002)
	{
		file->rdwr_long(ms_traffic_jam);
		target.rdwr(file);
		origin.rdwr(file);
	}

	else if(file->is_loading())
	{
		ms_traffic_jam = 0;
		target = koord::invalid;
		origin = koord::invalid;
	}

	if (file->get_extended_version() >= 15 || (file->get_extended_version() == 14 && file->get_extended_revision() >= 19))
	{
		last_tile_marked_as_stopped.rdwr(file);
	}
	else if (file->is_loading())
	{
		last_tile_marked_as_stopped = koord3d::invalid;
	}

	// do not start with zero speed!
	current_speed ++;

	reset_measurements();
}


bool private_car_t::can_enter_tile(grund_t *gr)
{
	if(gr->get_top()>200) {
		// already too many things here
		return false;
	}

	// road still there?
	strasse_t * str = (strasse_t*)gr->get_weg(road_wt);
	if(str==NULL) {
		time_to_life = 0;
		return false;
	}

	const uint8 overtaking_mode = str->get_overtaking_mode();

	// Is this road a bridleway?
	// An axle limit of zero prevents all private car traffic.
	if(str->get_max_axle_load() < 1)
	{
		time_to_life = 0;
		return false;
	}

	const player_t *player = str->get_owner();

	if(player != NULL && player->get_player_nr() != 1 && !player->allows_access_to(1))
	{
		// Private cas should have the same restrictions as to the roads on which to travel
		// as players' vehicles.
		time_to_life = 0;
		return false;
	}


	// calculate new direction
	// are we just turning around?
	const uint8 this_direction = get_direction();
	bool frei = false;
	vehicle_base_t *dt = NULL;
	const strasse_t* current_str = (strasse_t*)(welt->lookup(get_pos())->get_weg(road_wt));
	if(  get_pos()==pos_next_next  ) {
		// turning around => single check
		const uint8 next_direction = ribi_t::backward(this_direction);
		dt = no_cars_blocking( gr, NULL, next_direction, next_direction, next_direction, this, 0 );

		// do not block railroad crossing
		if(dt==NULL  &&  str->is_crossing()) {
			const grund_t *gr = welt->lookup(get_pos());
			dt = no_cars_blocking( gr, NULL, next_direction, next_direction, next_direction, this, 0 );
		}
	}
	else {
		// driving on: check for crossings etc. too
		const uint8 next_direction = this->calc_direction(get_pos(), pos_next_next);
		uint8 next_90direction = this->calc_direction(pos_next, pos_next_next);
		// do not block this crossing (if possible)
		if(ribi_t::is_threeway(str->get_ribi_unmasked())) {
			// but leaving from railroad crossing is more important
			grund_t *gr_here = welt->lookup(get_pos());
			if(  gr_here  &&  gr_here->ist_uebergang()  ) {
				return true;
			}
			// At a crossing, decide whether the convoi should go on passing lane.
			// side road -> main road from passing lane side: vehicle should enter passing lane on main road.
			next_lane = 0;
			if(  str->get_overtaking_mode() <= oneway_mode  ) {
				const strasse_t* str_prev = (strasse_t*)(welt->lookup(get_pos())->get_weg(road_wt));
				const strasse_t* str_next = (strasse_t*)(welt->lookup(pos_next)->get_weg(road_wt));
				const bool left_driving = welt->get_settings().is_drive_left();
				if(str_prev && str_next && str_prev->get_overtaking_mode() > oneway_mode  && str_next->get_overtaking_mode() <= oneway_mode) {
					if(  (!left_driving  &&  ribi_t::rotate90l(get_90direction()) == calc_direction(pos_next,pos_next_next))  ||  (left_driving  &&  ribi_t::rotate90(get_90direction()) == calc_direction(pos_next,pos_next_next))  ) {
						// next: enter passing lane.
						next_lane = 1;
					}
				}
			}

			grund_t *test = welt->lookup(pos_next_next);
			if(  test  ) {
				next_90direction = this->calc_direction(pos_next, pos_next_next);
				dt = no_cars_blocking( gr, NULL, this_direction, next_direction, next_90direction, this, next_lane);
				if(  !dt  ) {
					// This possibly made traffic too cautious at junctions, causing delays. However, precisely what this did
					// and why it did it remain unclear, so retaining for reference.
					//dt = no_cars_blocking( test, NULL, next_direction, next_90direction, next_90direction, this, next_lane);
				}
			}
			// this fails with two crossings together; however, I see no easy way out here ...
		}
		else {
			// not a crossing => skip 90° check!
			dt = no_cars_blocking( gr, NULL, this_direction, next_direction, next_90direction, this, next_lane );
			frei = true;
		}
		//If this car is overtaking, the car must avoid a head-on crash.
		if(  is_overtaking()  &&  current_str  ) {
			grund_t* sg[2];
			sg[0] = welt->lookup(pos_next);
			sg[1] = welt->lookup(pos_next_next);
			for(uint8 i = 0; i < 2; i++) {
				for(  uint8 pos=1;  sg[i] && pos < sg[i]->get_top();  pos++  ) {
					if(  vehicle_base_t* const v = obj_cast<vehicle_base_t>(sg[i]->obj_bei(pos))  ) {
						ribi_t:: ribi other_direction = 255;
						if(  road_vehicle_t const* const at = obj_cast<road_vehicle_t>(v)  ) {
							if(  at && !at->get_convoi()->is_overtaking()  ) {
								other_direction = at->get_direction();
							}
						}
						else if(  private_car_t* const caut = obj_cast<private_car_t>(v)  ) {
							if(  !caut->is_overtaking()  ) {
								other_direction = caut->get_direction();
							}
						}
						if(  other_direction!=255  ) {
							if(  i==0  &&  ribi_t::reverse_single(get_90direction())==other_direction  ) {
								set_tiles_overtaking(0);
							}
							if(  i==1  &&  ribi_t::reverse_single(ribi_type(pos_next, pos_next_next))==other_direction  ) {
								set_tiles_overtaking(0);
							}
						}
					}
				}
			}
		}
		// Overtaking vehicles shouldn't have anything blocking them
		if(  overtaking_mode <= oneway_mode  ) {
			if(  dt  ) {
				overtaker_t *over = dt->get_overtaker();
				if(over) {
					if(!over->is_overtaking()) {
						// otherwise the overtaken car would stop for us ...
						if(  road_vehicle_t const* const car = obj_cast<road_vehicle_t>(dt)  ) {
							convoi_t* const ocnv = car->get_convoi();
							if(  ocnv  ) {
								if(  next_lane<1  &&  !is_overtaking()  &&  !other_lane_blocked(false)  &&  !ocnv->is_overtaking()   &&  can_overtake( ocnv, (ocnv->is_loading() ? 0 : ocnv->get_akt_speed()), ocnv->get_length_in_steps()+ocnv->get_vehicle(0)->get_steps())  ) {
									if(current_speed==0) {
										ms_traffic_jam = 0;
										current_speed = 48;
									}
									return true;
								}
							}
						} else if(  private_car_t* const caut = obj_cast<private_car_t>(dt)  ) {
							if(  caut  ) {
								if(  next_lane<1  &&  !is_overtaking()  &&  !other_lane_blocked(false)  &&  !caut->is_overtaking()  &&  can_overtake(caut, caut->get_current_speed(), VEHICLE_STEPS_PER_TILE)  ) {
									if(current_speed==0) {
										ms_traffic_jam = 0;
										current_speed = 48;
									}
									return true;
								}
							}
						}
					}
				}
				else {
					// movingobj ... block road totally
					frei = false;
				}
			}
		}
		else {
			if(  !is_overtaking()  ) {
				if(  dt  ) {
					if(dt->is_stuck()) {
						// previous vehicle is stuck => end of traffic jam ...
						frei = false;
					}
					else {
						overtaker_t *over = dt->get_overtaker();
						if(over) {
							if(!over->is_overtaking()) {
								// otherwise the overtaken car would stop for us ...
								if(  road_vehicle_t const* const car = obj_cast<road_vehicle_t>(dt)  ) {
									convoi_t* const ocnv = car->get_convoi();
									if(  ocnv  ) {
										if(  can_overtake( ocnv, (ocnv->is_loading() ? 0 : over->get_max_power_speed()), ocnv->get_length_in_steps()+ocnv->get_vehicle(0)->get_steps())  ) {
											if(current_speed==0) {
												ms_traffic_jam = 0;
												current_speed = 48;
											}
											return true;
										}
									}
								} else if(  private_car_t* const caut = obj_cast<private_car_t>(dt)  ) {
									if(  caut  ) {
										if(can_overtake(caut, caut->get_desc()->get_topspeed(), VEHICLE_STEPS_PER_TILE)) {
											if(current_speed==0) {
												ms_traffic_jam = 0;
												current_speed = 48;
											}
											return true;
										}
									}
								}
							}
						}
						else {
							// movingobj ... block road totally
							frei = false;
						}
					}
				}
			}
		}

		// do not block railroad crossing
		if(  frei  &&  str->is_crossing()  ) {
			// can we cross?
			crossing_t* cr = gr->find<crossing_t>(2);
			if(  cr && !cr->request_crossing(this)) {
				// approaching railway crossing: check if empty
				return false;
			}
			// no further check, when already entered a crossing (to allow leaving it)
			grund_t *gr_here = welt->lookup(get_pos());
			if(gr_here  &&  gr_here->ist_uebergang()) {
				return true;
			}
			// ok, now check for free exit
			koord dir = pos_next.get_2d()-get_pos().get_2d();
			koord3d checkpos = pos_next+dir;
			uint8 number_reversed = 0;
			while(  number_reversed<2  ) {
				const grund_t *test = welt->lookup(checkpos);
				if(!test) {
					// should not reach here ! (z9999)
					break;
				}
				const uint8 next_direction = ribi_type(dir);
				const uint8 nextnext_direction = ribi_type(dir);
				// test next field after way crossing
				if(no_cars_blocking( test, NULL, next_direction, nextnext_direction, nextnext_direction, this, next_lane )) {
					return false;
				}
				// ok, left the crossing
				if(!test->find<crossing_t>(2)) {
					// approaching railway crossing: check if empty
					crossing_t* cr = gr->find<crossing_t>(2);
					return cr->request_crossing( this );
				}
				else {
					// seems to be a dead-end.
					if(  (test->get_weg_ribi(road_wt)&next_direction) == 0  ) {
						// will be going back
						pos_next_next=get_pos();
						// check also opposite direction are free
						dir = -dir;
						number_reversed ++;
					}
				}
				checkpos += dir;
			}
		}
	}

	// If this car is on passing lane and the next tile prohibites overtaking, this vehicle must wait until traffic lane become safe.
	if(  is_overtaking()  &&  str->get_overtaking_mode() == prohibited_mode  ) {
		if(  vehicle_base_t* v = other_lane_blocked(false)  ) {
			if(  v->get_waytype() == road_wt  &&  judge_lane_crossing(get_90direction(), calc_direction(pos_next,pos_next_next), v->get_90direction(), true, true)) {
				return false;
			}
		}
		// There is no vehicle on traffic lane.
		set_tiles_overtaking(0);
		if(current_speed==0) {
			ms_traffic_jam = 0;
			current_speed = 48;
		}
		return true;
	}
	// If this vehicle is forced to go back to traffic lane at the next tile and traffic lane is not safe to change lane, this vehicle should wait.
	if(  str->get_overtaking_mode() > oneway_mode  &&  str->get_overtaking_mode() <= prohibited_mode  &&  get_tiles_overtaking() == 1  ) {
		if(  vehicle_base_t* v = other_lane_blocked(false)  ) {
			if(  v->get_waytype() == road_wt  &&  judge_lane_crossing(get_90direction(), calc_direction(pos_next,pos_next_next), v->get_90direction(), true, true)) {
				return false;
			}
		}
	}
	// If there is a vehicle that requests lane crossing, this vehicle must stop to yield space.
	if(  vehicle_base_t* v = other_lane_blocked(true)  ) {
		if(  road_vehicle_t const* const at = obj_cast<road_vehicle_t>(v)  ) {
			if(  at->get_convoi()->get_next_cross_lane()  &&  at==at->get_convoi()->back()  ) {
				// vehicle must stop.
				return false;
			}
		}
	}

	if(dt==NULL  &&  current_speed==0) {
		ms_traffic_jam = 0;
		current_speed = 48;
	}

	return dt==NULL;
}


void private_car_t::enter_tile(grund_t* gr)
{
	// Destination city car code revived from an older version of Simutrans.
	// (Thanks to Prissi for finding this older code).
	if(target!=koord::invalid  &&  shortest_distance(pos_next.get_2d(),target)<10) {
		// delete it ...
		time_to_life = 0;
		uint32 number = 2;
		pedestrian_t::generate_pedestrians_at(get_pos(), number);
	}
	vehicle_base_t::enter_tile(gr);
	get_weg()->book(1, WAY_STAT_CONVOIS);
}

grund_t* private_car_t::hop_check()
{
	// TODO: Consider multi-threading this. This only ultimately
	// affects pos_next_next, an object stored in this individual
	// private car, so these can be processed in non-overlapping
	// batches in parallell. This is a sync_step task, so probably
	// cannot be concurrent. Query whether this is likely to be
	// worthwhile. This takes circa 5-9% of all CPU time on a large
	// game (768 towns) in the modern era (2004) of Pak128.Britain-Ex.

	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *const from = welt->lookup(pos_next);
	if(from==NULL) {
		// nothing to go? => destroy ...
		time_to_life = 0;
		return NULL;
	}

	// find the allowed directions
	const weg_t *weg = from->get_weg(road_wt);
	if(weg==NULL) {
		// nothing to go? => destroy ...
		time_to_life = 0;
		return NULL;
	}

	// traffic light phase check (since this is on next tile, it will always be necessary!)
	const ribi_t::ribi direction90 = ribi_type(get_pos(),pos_next);

	if(  weg->has_sign(  )) {
		const roadsign_t* rs = from->find<roadsign_t>();
		const roadsign_desc_t* rs_desc = rs->get_desc();
		if(rs_desc->is_traffic_light()  &&  (rs->get_dir()&direction90)==0) {
			direction = direction90;
			calc_image();
			// wait here
			current_speed = 48;
			weg_next = 0;
			return NULL;
		}
	}

	// If so, check for private car routes to our destination.
	if (target != koord::invalid && !welt->get_settings().get_assume_everywhere_connected_by_road())
	{
		// Check every tile of the route, since this is faster than the
		// call to get_neighbour() in the heuristic mode.

		// The target is an individual tile. If we are going to a destination in
		// a city, then we need the route to the city, not the tile.

		const grund_t* gr_check = welt->lookup_kartenboden(target);
		const gebaeude_t* gb = gr_check ? gr_check->get_building() : NULL;
		const stadt_t* destination_city = gb ? gb->get_stadt() : NULL;

		koord check_target = target;
		const planquadrat_t* tile = welt->access(pos_next.get_2d());
		stadt_t* current_city = tile ? tile->get_city() : NULL;

		// On the last tile of the route, this will give koord3d::invalid,
		// thus invoking the semi-random mode below.

		// We need to check here, as the hashtable will give us a 0,0,0 koord rather
		// than koord::invalid if this be not contained in the hashtable.
		bool found_route = false;
		found_route = weg->private_car_routes[weg_t::private_car_routes_currently_reading_element].is_contained(check_target);
		if (!found_route)
		{
			if (!current_city || current_city != destination_city)
			{
				// Only follow a route inside a city if:
				// (1) we are not in our destination city; or
				// (2) there is a route to the individual destination building in the city.
				check_target = destination_city ? destination_city->get_townhall_road() : koord::invalid;
				found_route = weg->private_car_routes[weg_t::private_car_routes_currently_reading_element].is_contained(check_target);
			}
		}

		if (found_route)
		{
			pos_next_next = weg->private_car_routes[weg_t::private_car_routes_currently_reading_element].get(check_target);

			// Check whether we are at the end of the route (i.e. the destination)
			if ((current_city == destination_city) && pos_next_next == koord3d::invalid)
			{
				time_to_life = 0;
				return NULL;
			}

			// Check whether the way has been deleted in the meantime.
			const grund_t* next_gr = welt->lookup(pos_next_next);
			const weg_t* next_way = next_gr ? next_gr->get_weg(road_wt) : NULL;
			if (!next_way)
			{
				pos_next_next = koord3d::invalid;

				// We also need to invalidate the route.
				const planquadrat_t* tile = welt->access(origin);
				stadt_t* origin_city = tile ? tile->get_city() : NULL;
				if (origin_city)
				{
					//welt->add_queued_city(origin_city); // Prioritise re-checking this city even if already re-checked in this cycle.
				}
			}
			else
			{
				const ribi_t::ribi current_dir = ribi_type(get_pos(), pos_next);
				const ribi_t::ribi dir_next = ribi_type(pos_next, pos_next_next);
				const strasse_t* str = static_cast<const strasse_t *>(next_way);

				const bool backwards = dir_next == ribi_t::backward(current_dir);

				bool direction_allowed = str->get_ribi() & dir_next;
				if (!direction_allowed)
				{
					// Check whether the private car is allowed on the subsequent way's direction
					const koord3d pos_next_next_next = next_way->private_car_routes[weg_t::private_car_routes_currently_reading_element].get(check_target);
					if (pos_next_next_next != koord3d::invalid)
					{
						const ribi_t::ribi dir_next_next = ribi_type(pos_next_next, pos_next_next_next);
						direction_allowed = str->get_ribi() & dir_next_next;
					}
					else
					{
						direction_allowed = true;
					}
				}

				if (!direction_allowed)
				{
					pos_next_next = koord3d::invalid;

					// We also need to invalidate the route.
					const planquadrat_t* tile = welt->access(origin);
					stadt_t* origin_city = tile ? tile->get_city() : NULL;
					if (origin_city)
					{
						//welt->add_queued_city(origin_city); // Prioritise re-checking this city even if already re-checked in this cycle.
					}
				}
				else
				{
					// Check whether the tile is passable: do not drive onto an impassible tile.
					if (!(next_way && next_way->get_max_speed() > 0 && next_way->get_max_axle_load() > 0 && (next_way->get_owner() == NULL || next_way->get_owner()->allows_access_to(welt->get_public_player()->get_player_nr()))))
					{
						// Next tile not passable even though this is on a route: mothballed, or made private. Revert to heuristic mode.
						pos_next_next = koord3d::invalid;
					}
				}

				if (backwards)
				{
					// Forbid u-turns
					pos_next_next = koord3d::invalid;
				}
			}
		}
	}

	// next tile unknown => find next tile
	if(pos_next_next==koord3d::invalid) {

		// ok, nobody did delete the road in front of us
		// so we can check for valid directions
		ribi_t::ribi ribi = weg->get_ribi() & (~ribi_t::backward(direction90));

		// cul de sac: return
		if(ribi==0) {
			pos_next_next = get_pos();
			return can_enter_tile(from) ? from : NULL;
		}

		static weighted_vector_tpl<koord3d> poslist(4);
		poslist.clear();
		bool city_exit = false;
		for (uint32 n = 0; n < 2; n++)
		{
			if (!poslist.empty())
			{
				break;
			}
			for (uint8 r = 0; r < 4; r++)
			{
				if (get_pos().get_2d() == koord::nsew[r] + pos_next.get_2d())
				{
					continue;
				}
				if ((ribi & ribi_t::nsew[r]) != 0)
				{
					grund_t* to;
					if (from->get_neighbour(to, road_wt, ribi_t::nsew[r]))
					{
						// check, if this is just a single tile deep after a crossing
						weg_t* w = to->get_weg(road_wt);
						if (ribi_t::is_single(w->get_ribi()) && (w->get_ribi() & ribi_t::nsew[r]) == 0 && !ribi_t::is_single(ribi))
						{
							ribi &= ~ribi_t::nsew[r];
							continue;
						}
						// check, if roadsign forbid next step ...
						if (w->has_sign()) {
							const roadsign_t* rs = to->find<roadsign_t>();
							const roadsign_desc_t* rs_desc = rs->get_desc();
							if (rs_desc->get_min_speed() > desc->get_topspeed() || (rs_desc->is_private_way() && (rs->get_player_mask() & 2) == 0))
							{
								// not allowed to go here
								ribi &= ~ribi_t::nsew[r];
								continue;
							}
						}

						// If we are not on a route to our destination, do not leave a city if we are in one, unless it is our destination city.
						// However, do not allow this sysem to prevent a car from going anywhere if the only way that it can go is out of the city
						// (e.g. if there is a one way road).
						const planquadrat_t* tile = welt->access(pos_next.get_2d());
						const stadt_t* current_city = tile ? tile->get_city() : NULL;
						if (current_city && (n == 0 || city_exit) && !welt->get_settings().get_assume_everywhere_connected_by_road())
						{
							planquadrat_t* tile = welt->access(to->get_pos().get_2d());
							const stadt_t* next_tile_city = tile ? tile->get_city() : NULL;

							const grund_t* gr_check = welt->lookup_kartenboden(target);
							const gebaeude_t* gb = gr_check ? gr_check->get_building() : NULL;
							const stadt_t* destination_city = gb ? gb->get_stadt() : NULL;

							if (next_tile_city != current_city && (!destination_city || next_tile_city != destination_city))
							{
								// We have checked whether this is on a route above, so if we reach here, we assume that this
								// city exit tile is not on a route.
								weg = from->get_weg(road_wt);
								grund_t* gr_backwards;
								if (city_exit)
								{
									// We have already been here once, so there is probably not an alternative. Can we at least make a u-turn?
									const ribi_t::ribi backwards = ribi_t::backward(ribi_type(get_pos(), pos_next));

									const bool backwards_allowed_this_tile = w ? w->get_ribi() & backwards : false;
									bool backwards_allowed_next_tile = false;

									const bool backwards_way = from->get_neighbour(gr_backwards, road_wt, backwards);

									if (backwards_way)
									{
										const weg_t* last_way = gr_backwards ? gr_backwards->get_weg(road_wt) : NULL;
										backwards_allowed_next_tile = last_way ? last_way->get_ribi() & backwards : false;
									}

									if (backwards_allowed_this_tile && backwards_allowed_next_tile)
									{
										const uint32 dist = 8192 / max(1, shortest_distance(to->get_pos().get_2d(), target));
										poslist.append(gr_backwards->get_pos(), dist);
										continue;
									}
								}
								else
								{
									city_exit = true;
									continue;
								}
							}
						}

						// Check whether the tile is passable: do not drive onto an impassible tile.
						const weg_t* next_way = to->get_weg(road_wt);
						if (next_way && next_way->get_max_speed() > 0 && next_way->get_max_axle_load() > 0 && (next_way->get_owner() == NULL || next_way->get_owner()->allows_access_to(welt->get_public_player()->get_player_nr())))
						{
							const uint32 dist = 8192 / max(1, shortest_distance(to->get_pos().get_2d(), target));
							poslist.append(to->get_pos(), dist);
						}
					}
					else
					{
						// not connected?!? => ribi likely wrong
						ribi &= ~ribi_t::nsew[r];
					}
				}
			}
		}
		if (!poslist.empty())
		{
			pos_next_next = pick_any_weighted(poslist);
		}
		else
		{
			pos_next_next = get_pos();
		}
		if(can_enter_tile(from)) {
			// ok, this direction is fine!
			ms_traffic_jam = 0;
			if(current_speed<48) {
				current_speed = 48;
			}
			return from;
		}
	}
	else {
		if(from  &&  can_enter_tile(from)) {
			// ok, this direction is fine!
			ms_traffic_jam = 0;
			if(current_speed<48) {
				current_speed = 48;
			}
			return from;
		}
	}
	// no free tiles => assume traffic jam ...
	pos_next_next = koord3d::invalid;
	current_speed = 0;
	return NULL;
}



void private_car_t::hop(grund_t* to)
{

	// Check whether this private car should pay a road toll.

	//weg_t* const way = get_weg(); // Occasionally, the way returned here was corrupt (possibly deleted)

	weg_t* const way = to->get_weg(road_wt);
	const uint32 tiles_per_km = 1000 / welt->get_settings().get_meters_per_tile();
	if(way && tiles_since_last_increment++ > tiles_per_km)
	{
		tiles_since_last_increment -= tiles_per_km;
		player_t *player = way->get_owner();
		if(player && player->get_player_nr() != 1)
		{
			const sint64 toll = welt->get_settings().get_private_car_toll_per_km();
			player->book_toll_received(toll, road_wt);
		}
	}

	grund_t* gr = get_grund();
	if(gr)
	{
		strasse_t* str = (strasse_t*)gr->get_weg(road_wt);
		if(str)
		{
			flush_travel_times(str);
		}
	}

	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	if(to==NULL) {
		time_to_life = 0;
		return;
	}

	if(way)
	{
		way->wear_way(welt->get_settings().get_citycar_way_wear_factor());
	}

	leave_tile();

	if(pos_next_next==get_pos()) {
		direction = calc_set_direction( pos_next, pos_next_next );
		steps_next = 0;	// mark for starting at end of tile!
	}
	else {
		direction = calc_set_direction( get_pos(), pos_next_next );
	}
	calc_image();

	// and add to next tile
	set_pos(pos_next);
	enter_tile(to);

	calc_current_speed(to);

	strasse_t *str = (strasse_t*)(to->get_weg(road_wt));
	//decide if overtaking citycar should go back to the traffic lane.
	if(  get_tiles_overtaking() == 1  &&  str->get_overtaking_mode() <= oneway_mode  ){
		vehicle_base_t* v = NULL;
		if(  (v = other_lane_blocked(false))  ) {
			//lane change denied
			set_tiles_overtaking(3);
		}
	}
	update_tiles_overtaking();
	if(to->ist_uebergang()) {
		to->find<crossing_t>(2)->add_to_crossing(this);
	}
	if(  next_lane==1  ) {
		set_tiles_overtaking(3);
		next_lane = 0;
	}
	pos_next = pos_next_next;
	pos_next_next = koord3d::invalid;
}



void private_car_t::calc_image()
{
	set_image(desc->get_image_id(ribi_t::get_dir(get_direction())));
	drives_on_left = welt->get_settings().is_drive_left();	// reset driving settings
}



void private_car_t::calc_current_speed(grund_t *)
{
	const weg_t * weg = get_weg();
	sint32 max_speed = desc ? desc->get_topspeed() : kmh_to_speed(90);
	const sint32 speed_limit = weg ? kmh_to_speed(weg->get_max_speed()) : max_speed;
	current_speed += max_speed>>2;
	if(current_speed > max_speed) {
		current_speed = max_speed;
	}
	if(current_speed > speed_limit) {
		current_speed = speed_limit;
	}
}


void private_car_t::info(cbuffer_t & buf) const
{
	const stadt_t* const origin_city = welt->get_city(origin);
	// We cannot get an origin name as the origin is the starting road tile, not building
	const stadt_t* const destination_city = welt->get_city(target);
	const grund_t* gr_target = welt->lookup_kartenboden(target);
	const gebaeude_t* destination_building = gr_target ? gr_target->get_building() : NULL;
	const char* origin_city_name = origin_city ? origin_city->get_name() : translator::translate("keine");
	const char* destination_name = destination_building ? translator::translate(destination_building->get_individual_name()) : translator::translate("keine");
	const char* destination_city_name = destination_city ? destination_city->get_name() : translator::translate("keine");
	buf.printf(translator::translate("%s\nspeed %i\nmax_speed %i\ndx:%i dy:%i"), translator::translate(desc->get_name()), speed_to_kmh(current_speed), speed_to_kmh(desc->get_topspeed()), dx, dy);
	buf.printf(translator::translate("\nOrigin: %s\nDestination: %s (%s)"), origin_city_name, destination_name, destination_city_name);
}



// to make smaller steps than the tile granularity, we have to use this trick
void private_car_t::get_screen_offset( int &xoff, int &yoff, const sint16 raster_width, bool prev_based ) const
{
	vehicle_base_t::get_screen_offset( xoff, yoff, raster_width );

	// eventually shift position to take care of overtaking
	sint8 tiles_overtaking = prev_based ? get_prev_tiles_overtaking() : get_tiles_overtaking();
	if(  tiles_overtaking>0  ) {
		/* This means the car is overtaking other vehicles. */
		xoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][0], raster_width);
		yoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][1], raster_width);
	}
	else if(  tiles_overtaking<0  ) {
		/* This means the car is overtaken by other vehicles. */
		xoff -= tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][0], raster_width)/5;
		yoff -= tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][1], raster_width)/5;
	}
}

void private_car_t::calc_disp_lane()
{
	// driving in the back or the front
	ribi_t::ribi test_dir = welt->get_settings().is_drive_left() ? ribi_t::northeast : ribi_t::southwest;
	disp_lane = get_direction() & test_dir ? 1 : 3;
}

void private_car_t::rotate90()
{
	road_user_t::rotate90();
	calc_disp_lane();
}


/**
 * conditions for a city car to overtake another overtaker.
 * The city car is not overtaking/being overtaken.
 * @author isidoro
 */
bool private_car_t::can_overtake( overtaker_t *other_overtaker, sint32 other_speed, sint16 steps_other)
{
	const grund_t *gr = welt->lookup(get_pos());
	if(  gr==NULL  ) {
		// should never happen, since there is a vehicle in front of us ...
		return false;
	}
	const strasse_t *str = (strasse_t*)(gr->get_weg(road_wt));
	if(  str==0  ) {
		// also this is not possible, since a car loads in front of is!?!
		return false;
	}
	const sint8 overtaking_mode = str->get_overtaking_mode();
	if(  !other_overtaker->can_be_overtaken()  &&  overtaking_mode > oneway_mode  ) {
		return false;
	}
	if(  overtaking_mode == prohibited_mode  ){
		// This road prohibits overtaking.
		return false;
	}

	if(  other_speed == 0  ) {
		/* overtaking a loading convoi
		 * => we can do a lazy check, since halts are always straight
		 */
		const ribi_t::ribi direction = get_direction() & str->get_ribi();
		koord3d check_pos = get_pos()+koord((ribi_t::ribi)(str->get_ribi()&direction));
		for(  int tiles=1+(steps_other-1)/(CARUNITS_PER_TILE*VEHICLE_STEPS_PER_CARUNIT);  tiles>=0;  tiles--  ) {
			grund_t *gr = welt->lookup(check_pos);
			if(  gr==NULL  ) {
				return false;
			}
			strasse_t *str = (strasse_t*)(gr->get_weg(road_wt));
			if(  str==0  ) {
				return false;
			}
			sint8 overtaking_mode = str->get_overtaking_mode();
			// not overtaking on railroad crossings ...
			if(  str->is_crossing() ) {
				return false;
			}
			if(  ribi_t::is_threeway(str->get_ribi())  &&  overtaking_mode > oneway_mode  ) {
				return false;
			}
			if(  overtaking_mode > oneway_mode  ) {
				// Check for other vehicles on the next tile
				const uint8 top = gr->get_top();
				for(  uint8 j=1;  j<top;  j++  ) {
					if(  vehicle_base_t* const v = obj_cast<vehicle_base_t>(gr->obj_bei(j))  ) {
						// check for other traffic on the road
						const overtaker_t *ov = v->get_overtaker();
						if(ov) {
							if(this!=ov  &&  other_overtaker!=ov) {
								return false;
							}
						}
						else if(  v->get_waytype()==road_wt  &&  v->get_typ()!=obj_t::pedestrian  ) {
							return false;
						}
					}
				}
			}
			check_pos += koord(direction);
		}
		set_tiles_overtaking( 3+(steps_other-1)/(CARUNITS_PER_TILE*VEHICLE_STEPS_PER_CARUNIT) );
		return true;
	}

	if(  overtaking_mode == twoway_mode  ) {
		// since other vehicle is moving...
		return false;
	}
	// On one-way road, other_speed is current speed. Otherwise, other_speed is the theoretical max power speed.
	sint32 diff_speed = (sint32)current_speed - other_speed;
	if(  diff_speed < kmh_to_speed(2)  ) {
		// not fast enough to overtake
		return false;
	}

	// Number of tiles overtaking will take
	int n_tiles = 0;

	/* Distance it takes overtaking (unit: vehicle steps) = my_speed * time_overtaking
	 * time_overtaking = tiles_to_overtake/diff_speed
	 * tiles_to_overtake = convoi_length + pos_other_convoi
	 * convoi_length for city cars? ==> a bit over half a tile (10)
	 */
	sint32 time_overtaking = 0;
	sint32 distance = current_speed*((10<<4)+steps_other)/max(desc->get_topspeed()-other_speed,diff_speed);

	// Conditions for overtaking:
	// Flat tiles, with no stops, no crossings, no signs, no change of road speed limit
	koord3d check_pos = get_pos();
	koord pos_prev = check_pos.get_2d();

	// we need 90 degree ribi
	ribi_t::ribi direction = get_direction();
	direction = str->get_ribi() & direction;

	while(  distance > 0  ) {

		// we allow stops and slopes, since empty stops and slopes cannot affect us
		// (citycars do not slow down on slopes!)

		// start of bridge is one level deeper
		if(gr->get_weg_yoff()>0)  {
			check_pos.z ++;
		}

		// special signs
		if(  str->has_sign()  &&  str->get_ribi()==str->get_ribi_unmasked()  ) {
			const roadsign_t *rs = gr->find<roadsign_t>(1);
			if(rs) {
				const roadsign_desc_t *rb = rs->get_desc();
				if(rb->get_min_speed()>desc->get_topspeed()  ||  rb->is_private_way()  ) {
					// do not overtake when road is closed for cars, or a too high min speed limit
					return false;
				}
				if(  rb->is_traffic_light()  &&  overtaking_mode >= twoway_mode  ) {
					//We consider traffic-lights on two-way road.
					return false;
				}
			}
		}

		// not overtaking on railroad crossings ...
		if(  str->is_crossing() ) {
			return false;
		}

		// street gets too slow (TODO: should be able to be correctly accounted for)
		//if(  desc->get_topspeed() > kmh_to_speed(str->get_max_speed())  ) {
		if(  overtaking_mode >= twoway_mode  &&  current_speed > kmh_to_speed(str->get_max_speed())  ) {
			return false;
		}

		int d = ribi_t::is_straight(str->get_ribi()) ? VEHICLE_STEPS_PER_TILE : diagonal_vehicle_steps_per_tile;
		distance -= d;
		time_overtaking += d;

		n_tiles++;

		/* Now we must check for next position:
		 * crossings are ok, as long as we cannot exit there due to one way signs
		 * much cheaper calculation: only go on in the direction of before (since no slopes allowed anyway ... )
		 */
		grund_t *to = welt->lookup( check_pos + koord((ribi_t::ribi)(str->get_ribi()&direction)) );
		if(  ribi_t::is_threeway(str->get_ribi())  ||  to==NULL) {
			// check for entries/exits/bridges, if necessary
			ribi_t::ribi rib = str->get_ribi();
			bool found_one = false;
			for(  int r=0;  r<4;  r++  ) {
				if(  (rib&ribi_t::nsew[r])==0  ||  check_pos.get_2d()+koord::nsew[r]==pos_prev) {
					continue;
				}
				if(gr->get_neighbour(to, road_wt, ribi_t::nsew[r])) {
					if(found_one) {
						// two directions to go: unexpected cars may occurs => abort
						return false;
					}
					found_one = true;
				}
			}
		}
		pos_prev = check_pos.get_2d();

		// nowhere to go => nobody can come against us ...
		if(to==NULL  ||  (str=(strasse_t*)(to->get_weg(road_wt)))==NULL) {
			return false;
		}

		// Check for other vehicles on the next tile
		const uint8 top = gr->get_top();
		for(  uint8 j=1;  j<top;  j++  ) {
			if(  vehicle_base_t* const v = obj_cast<vehicle_base_t>(gr->obj_bei(j))  ) {
				// check for other traffic on the road
				const overtaker_t *ov = v->get_overtaker();
				if(ov) {
					if(this!=ov  &&  other_overtaker!=ov) {
						if(  static_cast<strasse_t*>(gr->get_weg(road_wt))->get_overtaking_mode() <= oneway_mode  ) {
							//If ov goes same directory, should not return false
							if (v && v->get_direction() != direction && v->get_overtaker()) {
								return false;
							}
						}
						else {
							return false;
						}
					}
				}
				else if(  v->get_waytype()==road_wt  &&  v->get_typ()!=obj_t::pedestrian  ) {
					return false;
				}
			}
		}

		gr = to;
		check_pos = to->get_pos();

		direction = ~ribi_type( check_pos,pos_prev ) & str->get_ribi();
	}

	// Second phase: only facing traffic is forbidden
	//   Since street speed can change, we do the calculation with time.
	//   Each empty tile will subtract tile_dimension/max_street_speed.
	//   If time is exhausted, we are guaranteed that no facing traffic will
	//   invade the dangerous zone.
	if(current_speed == 0)
	{
		return false;
	}
	time_overtaking = (time_overtaking << 16)/(sint32)current_speed;
	do {
		// we can allow crossings or traffic lights here, since they will stop also oncoming traffic
		if(  ribi_t::is_straight(str->get_ribi())  ) {
			time_overtaking -= (VEHICLE_STEPS_PER_TILE<<16) / max(1, kmh_to_speed(str->get_max_speed()));
		}
		else {
			time_overtaking -= (diagonal_vehicle_steps_per_tile<<16) /  max(1, kmh_to_speed(str->get_max_speed()));
		}

		// start of bridge is one level deeper
		if(gr->get_weg_yoff()>0)  {
			check_pos.z ++;
		}

		// much cheaper calculation: only go on in the direction of before ...
		grund_t *to = welt->lookup( check_pos + koord((ribi_t::ribi)(str->get_ribi()&direction)) );
		if(  ribi_t::is_threeway(str->get_ribi())  ||  to==NULL  ) {
			// check for crossings/bridges, if necessary
			bool found_one = false;
			for(  int r=0;  r<4;  r++  ) {
				if(check_pos.get_2d()+koord::nsew[r]==pos_prev) {
					continue;
				}
				if(gr->get_neighbour(to, road_wt, ribi_t::nsew[r])) {
					if(found_one) {
						return false;
					}
					found_one = true;
				}
			}
		}

		koord pos_prev_prev = pos_prev;
		pos_prev = check_pos.get_2d();

		// nowhere to go => nobody can come against us ...
		if(to==NULL  ||  (str=(strasse_t*)(to->get_weg(road_wt)))==NULL) {
			break;
		}

		// Check for other vehicles in facing direction
		// now only I know direction on this tile ...
		ribi_t::ribi their_direction = ribi_t::backward(calc_direction( pos_prev_prev, to->get_pos()));
		const uint8 top = gr->get_top();
		for(  uint8 j=1;  j<top;  j++ ) {
			vehicle_base_t* const v = obj_cast<vehicle_base_t>(gr->obj_bei(j));
			if(  v  &&  v->get_direction() == their_direction  ) {
				// check for car
				if(v->get_overtaker()) {
					return false;
				}
			}
		}

		gr = to;
		check_pos = to->get_pos();

		direction = ~ribi_type( check_pos,pos_prev ) & str->get_ribi();
	} while( time_overtaking > 0 );

	set_tiles_overtaking( 1+n_tiles );

	return true;
}

vehicle_base_t* private_car_t::other_lane_blocked(const bool only_search_top) const{
	//This function calculate whether the car can change lane.
	// only_search_top == false: check only heading direction
	// only_search_top == true: check all directions
	grund_t *gr = welt->lookup(pos_next);
	if(  gr  ) {
		if(  vehicle_base_t* v = is_there_car(gr)  ) {
			return v;
		}
	}
	if(  !only_search_top  ) {
		gr = welt->lookup(get_pos());
		if(  gr  ) {
			if(  vehicle_base_t* v = is_there_car(gr)  ) {
				return v;
			}
		}
		// rear check should be written here...
		for(uint8 r = 0; r < 4; r++) {
			grund_t *to = NULL;
			if( gr && gr->get_neighbour(to, road_wt, ribi_t::nsew[r])  ) {
				if(  to  ) {
					if(  vehicle_base_t* v = is_there_car(gr)  ) {
						return v;
					}
				}
			}
		}
	}
	return NULL;
}

vehicle_base_t* private_car_t::is_there_car (grund_t *gr) const
{
	if(  gr == NULL  ) {
		dbg->error( "private_car_t::is_there_car", "grund is invalid!" );
	}
	assert(  gr  );
	for(  uint8 pos=1;  pos < gr->get_top();  pos++  ) {
		if(  vehicle_base_t* const v = obj_cast<vehicle_base_t>(gr->obj_bei(pos))  ) {
			if(  v->get_typ()==obj_t::pedestrian  ) {
				continue;
			}
			if(  road_vehicle_t const* const at = obj_cast<road_vehicle_t>(v)  ) {
				if(  is_overtaking() && at->get_convoi()->is_overtaking()  ){
					continue;
				}
				if(  !is_overtaking() && !(at->get_convoi()->is_overtaking())  ){
					//Prohibit going on passing lane when facing traffic exists.
					ribi_t::ribi other_direction = at->get_direction();
					if(  ribi_t::backward(get_direction()) == other_direction  ) {
						return v;
					}
					continue;
				}
				// speed zero check must be done by parent function.
				return v;
			}
			else if(  private_car_t* const caut = obj_cast<private_car_t>(v)  ) {
				if(  is_overtaking() && caut->is_overtaking()  ){
					continue;
				}
				if(  !is_overtaking() && !(caut->is_overtaking())  ){
					//Prohibit going on passing lane when facing traffic exists.
					ribi_t::ribi other_direction = caut->get_direction();
					if(  ribi_t::backward(get_direction()) == other_direction  ) {
						return v;
					}
					continue;
				}
				// speed zero check must be done by parent function.
				return v;
			}
		}
	}
	return NULL;
}

void private_car_t::reflesh(sint8 prev_tiles_overtaking, sint8 current_tiles_overtaking) {
	if(  (prev_tiles_overtaking==0)^(current_tiles_overtaking==0)  ){
		if(  !get_flag(obj_t::dirty)  ) {
			mark_image_dirty( image, 0 );
			set_flag( obj_t::dirty );
		}
	}
}

void *private_car_t::operator new(size_t /*s*/)
{
	return freelist_t::gimme_node(sizeof(private_car_t));
}


void private_car_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(private_car_t),p);
}
