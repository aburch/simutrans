/**
 * Moving objects for Simutrans.
 * Transport vehicles are defined in simvehicle.h, because they greatly
 * differ from the vehicles defined herein for the individual traffic
 * (pedestrians, citycars, movingobj aka flock of sheep).
 *
 * Hj. Malthaner
 *
 * April 2000
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
#include "../simconvoi.h"

#include "simroadtraffic.h"
#ifdef DESTINATION_CITYCARS
// for final citycar destinations
#include "simpeople.h"
#endif

#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"

#include "../obj/crossing.h"
#include "../obj/roadsign.h"

#include "../boden/grund.h"
#include "../boden/wege/weg.h"
#include "../boden/wege/strasse.h"

#include "../descriptor/citycar_desc.h"
#include "../descriptor/roadsign_desc.h"


#include "../utils/cbuffer_t.h"

#define NUM_LOOK_FORWARD 10

/**********************************************************************************************************************/
/* Road users (private cars and pedestrians) basis class from here on */


road_user_t::road_user_t() :
	vehicle_base_t()
{
	set_owner( welt->get_public_player() );
	time_to_life = 0;
	weg_next = 0;
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


road_user_t::road_user_t(grund_t* bd, uint16 random) :
	vehicle_base_t(bd ? bd->get_pos() : koord3d::invalid)
{
	ribi_t::ribi road_ribi = bd->get_weg_ribi(road_wt);

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

	set_xoff( (dx<0) ? OBJECT_OFFSET_STEPS : -OBJECT_OFFSET_STEPS );
	set_yoff( (dy<0) ? OBJECT_OFFSET_STEPS/2 : -OBJECT_OFFSET_STEPS/2 );

	if(to) {
		pos_next = to->get_pos();
	}
	else {
		pos_next = welt->lookup_kartenboden(get_pos().get_2d() + koord(direction))->get_pos();
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


void road_user_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "verkehrsteilnehmer_t" );

	sint8 hoff = file->is_saving() ? get_hoff() : 0;

	// correct old offsets ... REMOVE after savegame increase ...
	if(file->get_version()<99018  &&  file->is_saving()) {
		dx = dxdy[ ribi_t::get_dir(direction)*2 ];
		dy = dxdy[ ribi_t::get_dir(direction)*2+1 ];
		sint8 i = steps/16;
		set_xoff( get_xoff() + i*dx );
		set_yoff( get_yoff() + i*dy + hoff );
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
		sint8 ddx=get_xoff(), ddy=get_yoff()-hoff;
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


// this function returns an index value that matches to scope.
uint8 idx_in_scope(uint8 org, sint8 offset) {
	if(  (sint8)org + offset < 0  ) {
		return org+NUM_LOOK_FORWARD+offset;
	} else if(  org+offset < NUM_LOOK_FORWARD  ) {
		return org+offset;
	} else {
		return (org+offset)%NUM_LOOK_FORWARD;
	}
}


/**********************************************************************************************************************/
/* statsauto_t (city cars) from here on */


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
//DBG_DEBUG("private_car_t::built_timeline_liste()","year=%i, month=%i", month_now/12, month_now%12+1);

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
	
	// unreserve tiles
	unreserve_all_tiles();

	// just to be sure we are removed from this list!
	if(time_to_life>0) {
		welt->sync.remove(this);
	}
	welt->buche( -1, karte_t::WORLD_CITYCARS );
}


private_car_t::private_car_t(loadsave_t *file) :
	road_user_t()
{
	route.clear();
	route.resize(NUM_LOOK_FORWARD);
	// initialize route
	for(uint8 i=0; i<NUM_LOOK_FORWARD; i++) {
		route.append(koord3d::invalid);
	}
	reserving_tiles.clear();
	rdwr(file);
	ms_traffic_jam = 0;
	max_power_speed = 0; // should be calculated somehow!
	calc_disp_lane();
	if(desc) {
		welt->sync.add(this);
	}
	welt->buche( +1, karte_t::WORLD_CITYCARS );
}


private_car_t::private_car_t(grund_t* gr, koord const target) :
	road_user_t(gr, simrand(65535)),
	desc(liste_timeline.empty() ? 0 : pick_any_weighted(liste_timeline))
{
	route_index = 0;
	route.resize(NUM_LOOK_FORWARD);
	route.clear();
	// initialize route
	for(uint8 i=0; i<NUM_LOOK_FORWARD; i++) {
		route.append(koord3d::invalid);
	}
	route[0] = pos_next;
	reserving_tiles.clear();
	lane_affinity = 0;
	lane_affinity_end_index = -1;
	next_cross_lane = false;
	time_to_life = welt->get_settings().get_stadtauto_duration() << 20;  // ignore welt->ticks_per_world_month_shift;
	current_speed = 48;
	ms_traffic_jam = 0;
	max_power_speed = 0; // should be calculated somehow!
#ifdef DESTINATION_CITYCARS
	this->target = target;
#else
	(void)target;
#endif
	calc_image();
	calc_disp_lane();
	welt->buche( +1, karte_t::WORLD_CITYCARS );
}


sync_result private_car_t::sync_step(uint32 delta_t)
{
	time_to_life -= delta_t;
	if(  time_to_life<=0  ) {
		return SYNC_DELETE;
	}

	if(  current_speed==0  ) {
		// stuck in traffic jam
		uint32 old_ms_traffic_jam = ms_traffic_jam;
		ms_traffic_jam += delta_t;
		// check only every 1.024 s if stopped
		if(  (ms_traffic_jam>>10) != (old_ms_traffic_jam>>10)  ) {
			//pos_next_next = koord3d::invalid; <- should be deleted?
			if(  hop_check()  ) {
				ms_traffic_jam = 0;
				current_speed = 48;
			}
			else {
				if(  ms_traffic_jam > welt->ticks_per_world_month  &&  old_ms_traffic_jam<=welt->ticks_per_world_month  ) {
					// message after two month, reset waiting timer
					welt->get_message()->add_message( translator::translate("To heavy traffic\nresults in traffic jam.\n"), get_pos().get_2d(), message_t::traffic_jams, color_idx_to_rgb(COL_ORANGE) );
				}
			}
		}
		weg_next = 0;
	}
	else {
		weg_next += current_speed*delta_t;
		const uint32 distance = do_drive( weg_next );
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
	xml_tag_t s( file, "stadtauto_t" );

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
		time_to_life = simrand(1000000)+10000;
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
	
	if(  file->get_OTRP_version() < 16  ) {
		// construct route from old data structure
		route_index = 0;
		route[0] = pos_next;
		if(  file->get_version() > 99010  ) {
			koord3d dummy = route[1];
			dummy.rdwr(file);
			route[1] = dummy;
		}
		lane_affinity = 0;
		lane_affinity_end_index = -1;
		next_cross_lane = false;
	}
	else {
		file->rdwr_byte(route_index);
		for(uint8 i=0; i<NUM_LOOK_FORWARD; i++) {
			koord3d dummy = route[i];
			dummy.rdwr(file);
			route[i] = dummy;
		}
		file->rdwr_byte(lane_affinity);
		file->rdwr_byte(lane_affinity_end_index);
		file->rdwr_bool(next_cross_lane);
	}

	// overtaking status
	if(file->get_version()<100001) {
		set_tiles_overtaking( 0 );
	}
	else {
		file->rdwr_byte(tiles_overtaking);
		set_tiles_overtaking( tiles_overtaking );
	}
	// do not start with zero speed!
	current_speed ++;
}


bool private_car_t::ist_weg_frei(grund_t *gr)
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
	
	// first: check roadsigns
	const roadsign_t *rs = NULL;
	if(  str->has_sign()  ) {
		rs = gr->find<roadsign_t>();
		const roadsign_desc_t* rs_desc = rs->get_desc();
		const uint8 direction90 = ribi_type(get_pos(), pos_next);
		if(rs_desc->is_traffic_light()  &&  (rs->get_dir()&direction90)==0) {
			direction = direction90;
			calc_image();
			// wait here
			current_speed = 48;
			weg_next = 0;
			return false;
		}
	}

	const uint8 overtaking_mode = str->get_overtaking_mode();
	const koord3d pos_next_next = route[idx_in_scope(route_index,1)];
	const strasse_t* current_str = (strasse_t*)(welt->lookup(get_pos())->get_weg(road_wt));
	
	// At an intersection, decide whether the convoi should go on passing lane.
	// side road -> main road from passing lane side: vehicle should enter passing lane on main road.
	next_lane = 0;
	if(  str->get_overtaking_mode() <= oneway_mode  ) {
		const strasse_t* str_next = (strasse_t*)(welt->lookup(pos_next)->get_weg(road_wt));
		const bool left_driving = welt->get_settings().is_drive_left();
		if(current_str && str_next && current_str->get_overtaking_mode() > oneway_mode  && str_next->get_overtaking_mode() <= oneway_mode) {
			if(  (!left_driving  &&  ribi_t::rotate90l(get_90direction()) == calc_direction(pos_next,pos_next_next))  ||  (left_driving  &&  ribi_t::rotate90(get_90direction()) == calc_direction(pos_next,pos_next_next))  ) {
				// next: enter passing lane.
				next_lane = 1;
			}
		}
	}
	
	// When overtaking_mode changes from inverted_mode to others, no cars blocking must work as the car is on traffic lane. Otherwise, no_cars_blocking cannot recognize vehicles on the traffic lane of the next tile.
	//next_lane = -1 does NOT mean that the vehicle must go traffic lane on the next tile.
	if(  current_str  &&  current_str->get_overtaking_mode()==inverted_mode  ) {
		if(  str->get_overtaking_mode()<inverted_mode  ) {
			next_lane = -1;
		}
	}
	
	if(  current_str->get_overtaking_mode()<=oneway_mode  &&  str->get_overtaking_mode()>oneway_mode  ) {
		next_lane = -1;
	}
	
	vehicle_base_t *obj = NULL;
	bool turning = false;
	
	// way should be clear for overtaking: we checked previously
	// Now we have to consider even if convoi is overtaking!
	// calculate new direction
	koord3d next = pos_next_next!=koord3d::invalid ? pos_next_next : pos_next;
	ribi_t::ribi curr_direction   = get_direction();
	ribi_t::ribi curr_90direction = calc_direction(get_pos(), pos_next);
	ribi_t::ribi next_direction   = calc_direction(get_pos(), next);
	ribi_t::ribi next_90direction = calc_direction(pos_next, next);
		
	if(  get_pos()==pos_next_next  ) {
		// turning around => single check
		turning = true;
		const uint8 next_direction = ribi_t::backward(curr_direction);
		obj = no_cars_blocking( gr, NULL, next_direction, next_direction, next_direction, this, 0 );

		// do not block railroad crossing
		/*
		if(dt==NULL  &&  str->is_crossing()) {
			const grund_t *gr = welt->lookup(get_pos());
			dt = no_cars_blocking( gr, NULL, next_direction, next_direction, next_direction, this, 0 );
		}
		*/
	} else {
		obj = no_cars_blocking( gr, NULL, curr_direction, next_direction, next_90direction, this, next_lane );
	}
	
	// If the next tile is an intersection, we have to refer the reservation.
	if(  str->get_overtaking_mode()<=oneway_mode  &&  ribi_t::is_threeway(str->get_ribi_unmasked())  ) {
		// try to reserve tiles
		bool overtaking_on_tile = is_overtaking();
		if(  next_lane==1  ) {
			overtaking_on_tile = true;
		} else if(  next_lane==-1  ) {
			overtaking_on_tile = false;
		}
		// since reserve function modifies variables of the instance...
		strasse_t* s = (strasse_t *)gr->get_weg(road_wt);
		if(  !s  ||  !s->reserve(this, overtaking_on_tile, get_pos(), next)  ) {
			if(  obj  &&  obj->is_stuck()  ) {
				// because the blocking vehicle is stuck too...
				current_speed = 48;
				weg_next = 0;
			} else {
				current_speed =  current_speed*3/4;
			}
			return false;
		}
		// now we succeeded in reserving the road. register it.
		reserving_tiles.append(gr->get_pos());
	}
	
	// do not block intersections
	const bool drives_on_left = welt->get_settings().is_drive_left();
	bool int_block = ribi_t::is_threeway(str->get_ribi_unmasked())  &&  (((drives_on_left ? ribi_t::rotate90l(curr_90direction) : ribi_t::rotate90(curr_90direction)) & str->get_ribi_unmasked())  ||  curr_90direction != next_90direction  ||  (rs  &&  rs->get_desc()->is_traffic_light()));
	
	// stop for intersection feature is deprecated...
	
	// If this car is overtaking, the car must avoid head-on crash.
	//If this car is overtaking, the car must avoid a head-on crash.
	if(  is_overtaking()  &&  current_str  &&  current_str->get_overtaking_mode()!=inverted_mode  ) {
		grund_t* sg[2];
		sg[0] = welt->lookup(pos_next);
		sg[1] = welt->lookup(pos_next_next);
		for(uint8 i = 0; i < 2; i++) {
			if(  !sg[i]  ) {
				break;
			}
			for(  uint8 pos=1;  pos<(volatile uint8)sg[i]->get_top();  pos++  ) {
				if(  vehicle_base_t* const v = obj_cast<vehicle_base_t>(sg[i]->obj_bei(pos))  ) {
					if(  v->get_typ()==obj_t::pedestrian  ) {
						continue;
					}
					ribi_t:: ribi other_direction = 255;
					if(  road_vehicle_t const* const at = obj_cast<road_vehicle_t>(v)  ) {
						if(  !at->get_convoi()->is_overtaking()  ) {
							other_direction = at->get_direction();
						}
					}
					else if(  private_car_t* const caut = obj_cast<private_car_t>(v)  ) {
						if(  !caut->is_overtaking()  &&  caut!=this  ) {
							other_direction = caut->get_direction();
						}
					}
					if(  other_direction!=255  ) {
						//There is another car. We have to check if this car is facing or not.
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
	
	uint32 test_index = idx_in_scope(route_index,1);
	// we have to assume the lane that this vehicle goes in the intersection.
	sint8 lane_of_the_tile = next_lane;
	overtaking_mode_t mode_of_start_point = str->get_overtaking_mode();
	bool enter_passing_lane_from_side_road = false;
	// check exit from crossings and intersections, allow to proceed after 4 consecutive
	for(uint8 c=0; !obj   &&  (str->is_crossing()  ||  int_block)  &&  c<4; c++) {
		if(  route[test_index]==koord3d::invalid  ) {
			// coordinate is not determined. stop inspection.
			break;
		}
		
		if(  str->is_crossing()  ) {
			crossing_t* cr = gr->find<crossing_t>(2);
			if(  !cr->request_crossing(this)  ) {
				// wait here
				current_speed = 48;
				weg_next = 0;
				return false;
			}
		}
		
		// test next position
		gr = welt->lookup(route[test_index]);
		if(  !gr  ) {
			// way (weg) not existent (likely destroyed)
			time_to_life = 0;
			return false;
		}
		
		str = (strasse_t *)gr->get_weg(road_wt);
		if(  !str  ||  gr->get_top() > 250  ) {
			// too many cars here or no street
			time_to_life = 0;
			return false;
		}
		
		if(  mode_of_start_point<=oneway_mode  &&  str->get_overtaking_mode()>oneway_mode  ) {
			lane_of_the_tile = -1;
		}
		else if(  str->get_overtaking_mode()==inverted_mode  ) {
			lane_of_the_tile = 1;
		}
		else if(  str->get_overtaking_mode()<=oneway_mode  &&  enter_passing_lane_from_side_road  ) {
			lane_of_the_tile = 1;
		}
		
		// Decide whether the convoi should go on passing lane.
		// side road -> main road from passing lane side: vehicle should enter passing lane on main road.
		if(   ribi_t::is_threeway(str->get_ribi_unmasked())  &&  str->get_overtaking_mode() <= oneway_mode  ) {
			const strasse_t* str_prev = (strasse_t *)welt->lookup(route[idx_in_scope(test_index,-1)])->get_weg(road_wt);
			if(  str_prev  &&  str_prev->get_overtaking_mode() > oneway_mode  &&  route[idx_in_scope(test_index,1)]!=koord3d::invalid  ) {
				ribi_t::ribi dir_1 = calc_direction(route[idx_in_scope(test_index,-1)], route[test_index]);
				ribi_t::ribi dir_2 = calc_direction(route[test_index], route[idx_in_scope(test_index,1)]);
				if(  (!welt->get_settings().is_drive_left()  &&  ribi_t::rotate90l(dir_1) == dir_2)  ||  (welt->get_settings().is_drive_left()  &&  ribi_t::rotate90(dir_1) == dir_2)  ) {
					// next: enter passing lane.
					lane_of_the_tile = 1;
					enter_passing_lane_from_side_road = true;
				}
			}
		}
		
		// check cars
		curr_direction   = next_direction;
		curr_90direction = next_90direction;
		if(  route[idx_in_scope(test_index,1)]!=koord3d::invalid  ) {
			next             = route[idx_in_scope(test_index,1)];
			next_direction   = calc_direction(route[idx_in_scope(test_index,-1)], next);
			next_90direction = calc_direction(route[test_index], next);
			obj = no_cars_blocking( gr, NULL, curr_direction, next_direction, next_90direction, this, lane_of_the_tile );
		}
		else {
			next                 = route[test_index];
			next_90direction = calc_direction(route[idx_in_scope(test_index,-1)], next);
			if(  curr_direction == next_90direction  ||  !gr->is_halt()  ) {
				// check cars but allow to enter intersection if we are turning even when a car is blocking the halt on the last tile of our route
				// preserves old bus terminal behaviour
				obj = no_cars_blocking( gr, NULL, curr_direction, next_90direction, ribi_t::none, this, lane_of_the_tile );
			}
		}
		
		// check roadsigns
		if(  str->has_sign()  ) {
			rs = gr->find<roadsign_t>();
			if(  rs  ) {
				// since at the corner, our direction may be diagonal, we make it straight
				if(  rs->get_desc()->is_traffic_light()  &&  (rs->get_dir() & curr_90direction)==0  ) {
					// wait here
					current_speed = 48;
					weg_next = 0;
					return false;
				}
			}
		}
		else {
			rs = NULL;
		}
		
		// TODO: reservation system!
		if(  str->get_overtaking_mode()<=oneway_mode  &&  ribi_t::is_threeway(str->get_ribi_unmasked())  ) {
			// try to reserve tiles
			bool overtaking_on_tile = is_overtaking();
			if(  lane_of_the_tile==1  ) {
				overtaking_on_tile = true;
			} else if(  lane_of_the_tile==-1  ) {
				overtaking_on_tile = false;
			}
			// since reserve function modifies variables of the instance...
			strasse_t* s = (strasse_t *)gr->get_weg(road_wt);
			if(  !s  ||  !s->reserve(this, overtaking_on_tile, route[idx_in_scope(test_index,-1)], next)  ) {
				if(  obj  &&  obj->is_stuck()  ) {
					// because the blocking vehicle is stuck too...
					current_speed = 48;
					weg_next = 0;
				} else {
					current_speed =  current_speed*3/4;
				}
				return false;
			}
			// now we succeeded in reserving the road. register it.
			reserving_tiles.append(gr->get_pos());
		}
		
		// check for blocking intersection
		int_block = ribi_t::is_threeway(str->get_ribi_unmasked())  &&  (((drives_on_left ? ribi_t::rotate90l(curr_90direction) : ribi_t::rotate90(curr_90direction)) & str->get_ribi_unmasked())  ||  curr_90direction != next_90direction  ||  (rs  &&  rs->get_desc()->is_traffic_light()));
		
		test_index = idx_in_scope(test_index,1);
	}
	
	if(  obj  ) {
		// Process is different whether the road is for one-way or two-way
		sint8 overtaking_mode = str->get_overtaking_mode();
		if(  overtaking_mode <= oneway_mode  ) {
			// road is one-way.
			bool can_judge_overtaking = (test_index == idx_in_scope(route_index,1));
			// The overtaking judge method itself works only when test_index==route_index+1, that means the front tile is not an intersection.
			if(  !can_judge_overtaking  &&  test_index == route_index + 2u  &&  overtaking_mode == halt_mode  ) {
				can_judge_overtaking = true;
			}
			if(  can_judge_overtaking  ) {
				// no intersections or crossings, we might be able to overtake this one ...
				overtaker_t *over = obj->get_overtaker();
				if(  over  ) {
					// not overtaking/being overtake: we need to make a more thought test!
					if(  road_vehicle_t const* const car = obj_cast<road_vehicle_t>(obj)  ) {
						convoi_t* const ocnv = car->get_convoi();
						// yielding vehicle should not be overtaken by the vehicle whose maximum speed is same.
						bool yielding_factor = true;
						if(  ocnv->get_yielding_quit_index() != -1  &&  this->get_speed_limit() - ocnv->get_speed_limit() < kmh_to_speed(10)  ) {
							yielding_factor = false;
						}
						if(  lane_affinity != -1  &&  next_lane<1  &&  !is_overtaking()  &&  !other_lane_blocked(false)  &&  yielding_factor  &&  can_overtake( ocnv, (ocnv->get_state()==convoi_t::LOADING ? 0 : ocnv->get_akt_speed()), ocnv->get_length_in_steps()+ocnv->get_vehikel(0)->get_steps())  ) {
							// this vehicle changes lane. we have to unreserve tiles.
							unreserve_all_tiles();
							return true;
						}
						strasse_t *str=(strasse_t *)gr->get_weg(road_wt);
						sint32 cnv_max_speed = min(get_speed_limit(), str->get_max_speed()*kmh_to_speed(1));
						sint32 other_max_speed = min(ocnv->get_speed_limit(), str->get_max_speed()*kmh_to_speed(1));
						if(  is_overtaking() && kmh_to_speed(10) <  cnv_max_speed - other_max_speed  ) {
							//If the convoi is on passing lane and there is slower convoi in front of this, this convoi request the slower to go to traffic lane.
							ocnv->set_requested_change_lane(true);
						}
						//For the case that the faster convoi is on traffic lane.
						if(  lane_affinity != -1  &&  next_lane<1  &&  !is_overtaking() && kmh_to_speed(10) <  cnv_max_speed - other_max_speed  ) {
							if(  vehicle_base_t* const br = car->other_lane_blocked()  ) {
								if(  road_vehicle_t const* const blk = obj_cast<road_vehicle_t>(br)  ) {
									if(  car->get_direction() == blk->get_direction() && abs(car->get_convoi()->get_speed_limit() - blk->get_convoi()->get_speed_limit()) < kmh_to_speed(5)  ){
										//same direction && (almost) same speed vehicle exists.
										ocnv->yield_lane_space();
									}
								}
							}
						}
					}
					else if(  private_car_t* const caut = obj_cast<private_car_t>(obj)  ) {
						if(  lane_affinity != -1  &&  next_lane<1  &&  !is_overtaking()  &&  !other_lane_blocked(false)  &&  can_overtake(caut, caut->get_current_speed(), VEHICLE_STEPS_PER_TILE)  ) {
							// this vehicle changes lane. we have to unreserve tiles.
							unreserve_all_tiles();
							return true;
						}
					}
				}
			}
			// we have to wait ...
			if(  obj->is_stuck()  ) {
				// end of traffic jam, but no stuck message, because previous vehicle is stuck too
				current_speed = 48;
				weg_next = 0;
				if(  is_overtaking()  &&  other_lane_blocked(false) == NULL  ) {
					set_tiles_overtaking(0);
				}
				return false;
			}
			else {
				current_speed = (current_speed*3)/4;
			}
		}
		else if(  overtaking_mode <= loading_only_mode  ) {
			// road is two-way and overtaking is allowed on the stricter condition.
			if(  obj->is_stuck()  ) {
				// end of traffic jam, but no stuck message, because previous vehicle is stuck too
				current_speed = 48;
				weg_next = 0;
				return false;
			}
			else {
				if(  test_index == route_index + 1u  ) {
					// no intersections or crossings, we might be able to overtake this one ...
					overtaker_t *over = obj->get_overtaker();
					if(  over  &&  !over->is_overtaken()  ) {
						if(  over->is_overtaking()  ) {
							// otherwise we would stop every time being overtaken
							return true;
						}
						// not overtaking/being overtake: we need to make a more thought test!
						if(  road_vehicle_t const* const car = obj_cast<road_vehicle_t>(obj)  ) {
							convoi_t* const ocnv = car->get_convoi();
							if(  can_overtake( ocnv, (ocnv->get_state()==convoi_t::LOADING ? 0 : over->get_max_power_speed()), ocnv->get_length_in_steps()+ocnv->get_vehikel(0)->get_steps())  ) {
								// this vehicle changes lane. we have to unreserve tiles.
								unreserve_all_tiles();
								return true;
							}
						}
						else if(  private_car_t* const caut = obj_cast<private_car_t>(obj)  ) {
							if(  can_overtake(caut, caut->get_desc()->get_topspeed(), VEHICLE_STEPS_PER_TILE)  ) {
								// this vehicle changes lane. we have to unreserve tiles.
								unreserve_all_tiles();
								return true;
							}
						}
					}
				}
				current_speed = (current_speed*3)/4;
			}
		}
		else {
			// lane change is prohibited.
			if(  obj->is_stuck()  ) {
				// end of traffic jam, but no stuck message, because previous vehicle is stuck too
				current_speed = 48;
				weg_next = 0;
				return false;
			}
			else {
				// we have to wait ...
				current_speed = (current_speed*3)/4;
			}
		}
		// If this vehicle is on passing lane and the next tile prohibites overtaking, this vehicle must wait until traffic lane become safe.
		// When condition changes, overtaking should be quitted once.
		if(  (is_overtaking()  &&  str->get_overtaking_mode()==prohibited_mode)  ||  (is_overtaking()  &&  str->get_overtaking_mode()>oneway_mode  &&  str->get_overtaking_mode()<inverted_mode  &&  static_cast<strasse_t*>(welt->lookup(get_pos())->get_weg(road_wt))->get_overtaking_mode()<=oneway_mode)  ) {
			if(  vehicle_base_t* v = other_lane_blocked(false)  ) {
				if(  v->get_waytype() == road_wt  ) {
					ms_traffic_jam = 0;
					current_speed = 48;
					next_cross_lane = true;
					return false;
				}
			}
			// There is no vehicle on traffic lane.
			// cnv->set_tiles_overtaking(0); is done in enter_tile()
		}
		// If this vehicle is on traffic lane and the next tile forces to go passing lane, this vehicle must wait until passing lane become safe.
		if(  !is_overtaking()  &&  str->get_overtaking_mode() == inverted_mode  ) {
			if(  vehicle_base_t* v = other_lane_blocked(false)  ) {
				if(  v->get_waytype() == road_wt  ) {
					ms_traffic_jam = 0;
					current_speed = 48;
					next_cross_lane = true;
					return false;
				}
			}
			// There is no vehicle on passing lane.
			next_lane = 1;
			return true;
		}
		// If the next tile is a intersection, lane crossing must be checked before entering.
		// The other vehicle is ignored if it is stopping to avoid stuck.
		const ribi_t::ribi way_ribi = str ? str->get_ribi_unmasked() : ribi_t::none;
		if(  str  &&  str->get_overtaking_mode() <= oneway_mode  &&  (way_ribi == ribi_t::all  ||  ribi_t::is_threeway(way_ribi))  ) {
			if(  const vehicle_base_t* v = other_lane_blocked(true)  ) {
				if(  road_vehicle_t const* const at = obj_cast<road_vehicle_t>(v)  ) {
					if(  at->get_convoi()->get_akt_speed()>kmh_to_speed(1)  &&  judge_lane_crossing(calc_direction(get_pos(),pos_next), calc_direction(pos_next,pos_next_next), at->get_90direction(), is_overtaking(), false)  ) {
						// vehicle must stop.
						ms_traffic_jam = 0;
						current_speed = 48;
						next_cross_lane = true;
						return false;
					}
				}
				else if(  private_car_t const* const pcar = obj_cast<private_car_t>(v)  ) {
					if(  pcar->get_current_speed()>kmh_to_speed(1)  &&  judge_lane_crossing(calc_direction(get_pos(),pos_next), calc_direction(pos_next,pos_next_next), pcar->get_90direction(), is_overtaking(), false)  ) {
						// vehicle must stop.
						ms_traffic_jam = 0;
						current_speed = 48;
						next_cross_lane = true;
						return false;
					}
				}
			}
		}
		// TODO: write yielding system for citycars!
		// For the case that this vehicle is fixed to passing lane and is on traffic lane.
		if(  str->get_overtaking_mode() <= oneway_mode  &&  lane_affinity == 1  &&  !is_overtaking()  ) {
			if(  vehicle_base_t* v = other_lane_blocked(false)  ) {
				if(  road_vehicle_t const* const car = obj_cast<road_vehicle_t>(v)  ) {
					convoi_t* ocnv = car->get_convoi();
					if(  ocnv  &&  abs(get_speed_limit() - ocnv->get_speed_limit()) < kmh_to_speed(5)  ) {
						// cnv->yield_lane_space();
					}
				}
				// citycars do not have the yielding mechanism.
			}
			else {
				// go on passing lane.
				set_tiles_overtaking(3);
			}
		}
		// If there is a vehicle that requests lane crossing, this vehicle must stop to yield space.
		if(  vehicle_base_t* v = other_lane_blocked(true)  ) {
			if(  road_vehicle_t const* const at = obj_cast<road_vehicle_t>(v)  ) {
				if(  at->get_convoi()->get_next_cross_lane()  &&  at==at->get_convoi()->back()  ) {
					// vehicle must stop.
					ms_traffic_jam = 0;
					current_speed = 48;
					return false;
				}
			}
			else if(  private_car_t const* const pcar = obj_cast<private_car_t>(v)  ) {
				if(  pcar->get_next_cross_lane()  ) {
					// vehicle must stop.
					ms_traffic_jam = 0;
					current_speed = 48;
					return false;
				}
			}
		}
	}
	
	return obj==NULL;
}

void private_car_t::leave_tile() {
	vehicle_base_t::leave_tile();
	// unreserve the tile
	grund_t* gr = welt->lookup(get_pos());
	strasse_t* str = gr ? (strasse_t*)(gr->get_weg(road_wt)) : NULL;
	if(  str  ) {
		str->unreserve(this);
		reserving_tiles.remove(gr->get_pos());
	}
}

void private_car_t::enter_tile(grund_t* gr, koord3d prev)
{
#ifdef DESTINATION_CITYCARS
	if(  target!=koord::invalid  &&  koord_distance(pos_next.get_2d(),target)<10  ) {
		// delete it ...
		time_to_life = 0;
		// was generating pedestrians gere, but not possible with new sync system
	}
#endif
	ribi_t::ribi enter_direction = calc_direction(get_pos(), gr->get_pos());
	vehicle_base_t::enter_tile(gr);
	calc_disp_lane();
	strasse_t* str = (strasse_t*) gr->get_weg(road_wt);
	str->book(1, WAY_STAT_CONVOIS, enter_direction);
	update_tiles_overtaking();
	if(  next_lane==1  ) {
		set_tiles_overtaking(3);
		next_lane = 0;
	}
	//decide if overtaking citycar should go back to the traffic lane.
	if(  get_tiles_overtaking() == 1  &&  str->get_overtaking_mode() <= oneway_mode  ){
		vehicle_base_t* v = NULL;
		if(  lane_affinity==1  ||  (v = other_lane_blocked(false))!=NULL  ||  str->is_reserved_by_others(this, false, prev, pos_next)  ) {
			//lane change denied
			set_tiles_overtaking(3);
		}
		else {
			// lane change accepted
		}
	}
	if(  str->get_overtaking_mode() == inverted_mode  ) {
		set_tiles_overtaking(1);
	}
	next_cross_lane = false; // since this car moved...
	// If there is one-way sign, calc lane_affinity.
	if(  roadsign_t* rs = gr->find<roadsign_t>()  ) {
		if(  rs->get_desc()->is_single_way()  ) {
			if(  calc_lane_affinity(rs->get_lane_affinity())  ) {
				// write debug code here.
			}
		}
	}
	// note that route_index is already forwarded
	if(  lane_affinity_end_index==idx_in_scope(route_index,-1)  ) {
		lane_affinity = 0;
	}
	// If this tile is two-way ~ prohibited and the previous tile is oneway, the convoy have to move on traffic lane. Safety is confirmed in ist_weg_frei().
	grund_t* prev_gr = welt->lookup(prev);
	strasse_t* prev_str = prev_gr ? (strasse_t*)(prev_gr->get_weg(road_wt)) : NULL;
	if(  (prev_str  &&  (prev_str->get_overtaking_mode()<=oneway_mode  &&  str->get_overtaking_mode()>oneway_mode  &&  str->get_overtaking_mode()<inverted_mode))  ||  str->get_overtaking_mode()==prohibited_mode  ){
			set_tiles_overtaking(0);
	}
}

// find destination for given index
koord3d private_car_t::find_destination(uint8 target_index) {
	// assume target_index != route_index
	grund_t* gr = welt->lookup(route[idx_in_scope(target_index,-1)]);
	strasse_t* weg = gr ? (strasse_t*) (gr->get_weg(road_wt)) : NULL;
	
	if(  weg==NULL  ) {
		// not searchable...
		return koord3d::invalid;
	}
	
	// so we can check for valid directions
	koord3d pos_prev2;
	// calculate previous direction
	if(  target_index==idx_in_scope(route_index,1)  ) {
		// we have to use current pos
		pos_prev2 = get_pos();
	} else {
		// we can calculate from route
		pos_prev2 = route[idx_in_scope(target_index,-2)];
	}
	const ribi_t::ribi direction90 = ribi_type(pos_prev2, route[idx_in_scope(target_index,-1)]);
	ribi_t::ribi ribi = weg->get_ribi() & (~ribi_t::backward(direction90));

	if(  weg->get_ribi()==0  ) {
		// this can go to nowhere!
		return koord3d::invalid;
	}
	else if(  weg->get_ribi()==ribi_t::backward(direction90)  ) {
		// we have no choice but to return to pos_prev2
		return pos_prev2;
	}

	static weighted_vector_tpl<koord3d> poslist(4);
	poslist.clear();
	for(uint8 r = 0; r < 4; r++) {
		if(  (ribi&ribi_t::nsew[r])!=0  ) {
			grund_t *to;
			if(  gr->get_neighbour(to, road_wt, ribi_t::nsew[r])  ) {
				// check, if this is just a single tile deep after a crossing
				weg_t *w = to->get_weg(road_wt);
				// TODO: maybe this condition should be removed!
				/*
				if(  ribi_t::is_single(w->get_ribi())  &&  (w->get_ribi()&ribi_t::nsew[r])==0  &&  !ribi_t::is_single(ribi)  ) {
					ribi &= ~ribi_t::nsew[r];
					continue;
				}
				*/
				// check, if roadsign forbid next step ...
				if(w->has_sign()) {
					const roadsign_t* rs = to->find<roadsign_t>();
					const roadsign_desc_t* rs_desc = rs->get_desc();
					if(rs_desc->get_min_speed()>desc->get_topspeed()  ||  (rs_desc->is_private_way()  &&  (rs->get_player_mask()&2)==0)  ) {
						// not allowed to go here
						ribi &= ~ribi_t::nsew[r];
						continue;
					}
				}
#ifdef DESTINATION_CITYCARS
				uint32 dist=koord_distance( to->get_pos().get_2d(), target );
				poslist.append( to->get_pos(), dist*dist );
#else
				// determine weight
				// making a sharp turn is not preferred
				if(  target_index!=idx_in_scope(route_index,1)  &&   target_index!=idx_in_scope(route_index,2)  ) {
					// sharp turn?
					const ribi_t::ribi dir1 = ribi_type(to->get_pos(),route[idx_in_scope(target_index,-1)]);
					const ribi_t::ribi dir2 = ribi_type(route[idx_in_scope(target_index,-1)],route[idx_in_scope(target_index,-2)]);
					const ribi_t::ribi dir3 = ribi_type(route[idx_in_scope(target_index,-2)],route[idx_in_scope(target_index,-3)]);
					if(  (ribi_t::rotate90(dir3)==dir2  &&  ribi_t::rotate90(dir2)==dir1)  ||  (ribi_t::rotate90l(dir3)==dir2  &&  ribi_t::rotate90l(dir2)==dir1)) {
						// reduce possibility
						poslist.append(to->get_pos(), 1);
						continue;
					}
				}
				poslist.append(to->get_pos(), 10);
#endif
			}
			else {
				// not connected?!? => ribi likely wrong
				ribi &= ~ribi_t::nsew[r];
			}
		}
	}
	if (!poslist.empty()) {
		route[target_index] = pick_any_weighted(poslist);
	}
	else if(  weg->get_ribi() & ribi_t::backward(direction90)  ) {
		return pos_prev2;
	}
	return koord3d::invalid;
}


grund_t* private_car_t::hop_check()
{
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
	const ribi_t::ribi direction90 = ribi_type(get_pos(), pos_next);
	
	// try to find route
	for(uint8 i=1; i<NUM_LOOK_FORWARD; i++) {
		uint8 idx = (route_index+i)%NUM_LOOK_FORWARD;
		if(  route[idx]!=koord3d::invalid  ) {
			// route is already determined.
			continue;
		}
		koord3d dest = find_destination(idx);
		if(  dest==koord3d::invalid  ) {
			// could not find destination
			break;
		} else {
			route[idx] = dest;
		}
	}

	if(from  &&  ist_weg_frei(from)) {
		// ok, this direction is fine!
		ms_traffic_jam = 0;
		if(current_speed<48) {
			current_speed = 48;
		}
		return from;
	}
	// no free tiles => assume traffic jam ...
	current_speed = 0;
	return NULL;
}



void private_car_t::hop(grund_t* to)
{
	leave_tile();

	const koord3d pos_next_next = route[(route_index+1)%NUM_LOOK_FORWARD];
	if(pos_next_next==get_pos()) {
		direction = calc_set_direction( pos_next, pos_next_next );
		steps_next = 0;	// mark for starting at end of tile!
	}
	else {
		direction = calc_set_direction( get_pos(), pos_next_next );
	}
	calc_image();

	koord3d p = get_pos();

	// and add to next tile
	set_pos(pos_next);
	// proceed route_index
	route[route_index] = koord3d::invalid;
	route_index = (route_index+1)%NUM_LOOK_FORWARD;
	pos_next = route[route_index];
	enter_tile(to,p);
	if(to->ist_uebergang()) {
		to->find<crossing_t>(2)->add_to_crossing(this);
	}
	calc_current_speed(to);
}


void private_car_t::calc_image()
{
	set_image(desc->get_image_id(ribi_t::get_dir(get_direction())));
}


void private_car_t::calc_current_speed(grund_t* gr)
{
	const weg_t * weg = gr->get_weg(road_wt);
	const sint32 max_speed = desc->get_topspeed();
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
	buf.printf(translator::translate("%s\nspeed %i\nmax_speed %i\ndx:%i dy:%i"), translator::translate(desc->get_name()), speed_to_kmh(current_speed), speed_to_kmh(desc->get_topspeed()), dx, dy);
}


// to make smaller steps than the tile granularity, we have to use this trick
void private_car_t::get_screen_offset( int &xoff, int &yoff, const sint16 raster_width, bool prev_based ) const
{
	vehicle_base_t::get_screen_offset( xoff, yoff, raster_width );

	if(  welt->get_settings().is_drive_left()  ) {
		const int drive_left_dir = ribi_t::get_dir(get_direction());
		xoff += tile_raster_scale_x( driveleft_base_offsets[drive_left_dir][0], raster_width );
		yoff += tile_raster_scale_y( driveleft_base_offsets[drive_left_dir][1], raster_width );
	}

	// eventually shift position to take care of overtaking
	sint8 tiles_overtaking = prev_based ? get_prev_tiles_overtaking() : get_tiles_overtaking();
	if(  tiles_overtaking>0  ) {  /* This means the car is overtaking other vehicles. */
		xoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][0], raster_width);
		yoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][1], raster_width);
	}
	else if(  tiles_overtaking<0  ) {  /* This means the car is overtaken by other vehicles. */
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
	//Overtaking info (0 = condition for one-way road, 1 = condition for two-way road, 2 = overtaking a loading convoy only, 3 = overtaking is completely forbidden, 4 = vehicles can go only on passing lane)
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

	if(  overtaking_mode == loading_only_mode  ) {
		// since other vehicle is moving...
		return false;
	}
	// On one-way road, other_speed is current speed. Otherwise, other_speed is the theoretical max power speed.
	sint32 diff_speed = (sint32)current_speed - other_speed;
	if(  diff_speed < kmh_to_speed(5)  ) {
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
		if(  overtaking_mode >= twoway_mode  &&  desc->get_topspeed() > kmh_to_speed(str->get_max_speed())  ) {
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
						return false;
					}
				}
				else if(  v->get_waytype()==road_wt  &&  v->get_typ()!=obj_t::pedestrian  ) {
					return false;
				}
			}
		}

		gr = to;
		check_pos = to->get_pos();

		direction = ~ribi_type( check_pos, pos_prev ) & str->get_ribi();
	}

	// Second phase: only facing traffic is forbidden
	//   Since street speed can change, we do the calculation with time.
	//   Each empty tile will subtract tile_dimension/max_street_speed.
	//   If time is exhausted, we are guaranteed that no facing traffic will
	//   invade the dangerous zone.
	time_overtaking = (time_overtaking << 16)/(sint32)current_speed;
	do {
		// we can allow crossings or traffic lights here, since they will stop also oncoming traffic
		if(  ribi_t::is_straight(str->get_ribi())  ) {
			time_overtaking -= (VEHICLE_STEPS_PER_TILE<<16) / kmh_to_speed(str->get_max_speed());
		}
		else {
			time_overtaking -= (diagonal_vehicle_steps_per_tile<<16) / kmh_to_speed(str->get_max_speed());
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

		direction = ~ribi_type( check_pos, pos_prev ) & str->get_ribi();
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
			if(  gr->get_neighbour(to, road_wt, ribi_t::nsew[r])  ) {
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
	for(  uint8 pos=1;  pos<(volatile uint8)gr->get_top();  pos++  ) {
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

void private_car_t::refresh(sint8 prev_tiles_overtaking, sint8 current_tiles_overtaking) {
	if(  (prev_tiles_overtaking==0)^(current_tiles_overtaking==0)  ){
		int xpos=0, ypos=0;
		get_screen_offset( xpos, ypos, get_tile_raster_width(), true );
		viewport_t *vp = welt->get_viewport();
		scr_coord scr_pos = vp->get_screen_coord(get_pos(), koord(get_xoff(), get_yoff()));
		display_mark_img_dirty( image, scr_pos.x + xpos, scr_pos.y + ypos);
		if(  !get_flag(obj_t::dirty)  ) {
		set_flag( obj_t::dirty );
		}
	}
}

bool private_car_t::calc_lane_affinity(uint8 lane_affinity_sign)
{
	if(  lane_affinity_sign==0  ||  lane_affinity_sign>=4  ) {
		return false;
	}
	// test_index starts from route_index+1 because we cannot calculate turning direction when route_index==test_index
	for(uint8 c=1; c<NUM_LOOK_FORWARD-1; c++) {
		uint8 test_index = idx_in_scope(route_index,c);
		if(  route[test_index]==koord3d::invalid  ) {
			// cannot obtain coordinate
			return false;
		}
		grund_t *gr = welt->lookup(route[test_index]);
		if(  !gr  ) {
			// way (weg) not existent (likely destroyed)
			return false;
		}
		strasse_t *str = (strasse_t *)gr->get_weg(road_wt);
		if(  !str  ||  gr->get_top() > 250  ||  str->get_overtaking_mode() > oneway_mode  ) {
			// too many cars here or no street or not one-way road
			return false;
		}
		ribi_t::ribi str_ribi = str->get_ribi_unmasked();
		if(  str_ribi == ribi_t::all  ||  ribi_t::is_threeway(str_ribi)  ) {
			// It's a intersection.
			if(  route[idx_in_scope(test_index,1)]==koord3d::invalid  ) {
				// cannot calculate prev_dir or next_dir
				return false;
			}
			ribi_t::ribi prev_dir = vehicle_base_t::calc_direction(welt->lookup(route[idx_in_scope(test_index,-1)])->get_pos(),welt->lookup(route[test_index])->get_pos());
			ribi_t::ribi next_dir = vehicle_base_t::calc_direction(welt->lookup(route[test_index])->get_pos(),welt->lookup(route[idx_in_scope(test_index,+1)])->get_pos());
			ribi_t::ribi str_left = (ribi_t::rotate90l(prev_dir) & str_ribi) == 0 ? prev_dir : ribi_t::rotate90l(prev_dir);
			ribi_t::ribi str_right = (ribi_t::rotate90(prev_dir) & str_ribi) == 0 ? prev_dir : ribi_t::rotate90(prev_dir);
			if(  next_dir == str_left  &&  (lane_affinity_sign & 1) != 0  ) {
				// fix to left lane
				if(  welt->get_settings().is_drive_left()  ) {
					lane_affinity = -1;
				}
				else {
					lane_affinity = 1;
				}
				lane_affinity_end_index = test_index;
				return true;
			}
			else if(  next_dir == str_right  &&  (lane_affinity_sign & 2) != 0  ) {
				// fix to right lane
				if(  welt->get_settings().is_drive_left()  ) {
					lane_affinity = 1;
				}
				else {
					lane_affinity = -1;
				}
				lane_affinity_end_index = test_index;
				return true;
			}
			else {
				return false;
			}
		}
	}
	return false;
}

uint16 private_car_t::get_speed_limit() const {
	grund_t* gr = welt->lookup(get_pos());
	weg_t* weg = gr ? gr->get_weg(road_wt) : NULL;
	if(  !weg  ) {
		// no way on the tile!?
		return 0;
	}
	return max(weg->get_max_speed(), desc->get_topspeed());
}

void private_car_t::unreserve_all_tiles() {
	for(uint32 i=0; i<reserving_tiles.get_count(); i++) {
		grund_t* gr = welt->lookup(reserving_tiles[i]);
		if(  gr  ) {
			strasse_t* str = (strasse_t*) (gr->get_weg(road_wt));
			if(  str  ) {
				str->unreserve(this);
			}
		}
	}
	reserving_tiles.clear();
}