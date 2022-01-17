/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simdebug.h"
#include "../display/simgraph.h"
#include "../simmesg.h"
#include "../simworld.h"
#include "../utils/simrandom.h"
#include "../display/simimg.h"
#include "../simunits.h"
#include "../simtypes.h"
#include "../simconvoi.h"

#include "simroadtraffic.h"
#include "vehicle.h"

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

#include "../descriptor/citycar_desc.h"
#include "../descriptor/roadsign_desc.h"

#include "../utils/cbuffer_t.h"

#include "road_vehicle.h"


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
	direction = ribi_t::nesw[offset];

	grund_t *to = NULL;
	for(uint8 r = 0; r < 4; r++) {
		ribi_t::ribi ribi = ribi_t::nesw[ (r + offset) &3];
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
	if(file->is_version_less(99, 18)  &&  file->is_saving()) {
		dx = dxdy[ ribi_t::get_dir(direction)*2 ];
		dy = dxdy[ ribi_t::get_dir(direction)*2+1 ];
		sint8 i = steps/16;
		set_xoff( get_xoff() + i*dx );
		set_yoff( get_yoff() + i*dy + hoff );
	}

	vehicle_base_t::rdwr(file);

	if(file->is_version_less(86, 6)) {
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
		if(file->is_version_less(99, 5)) {
			sint32 dummy32;
			file->rdwr_long(dummy32);
		}
		file->rdwr_long(weg_next);
		if(file->is_version_less(99, 18)) {
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
		if(file->is_version_less(99, 5)  ||  file->is_version_atleast(99, 17)) {
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
	if(file->is_version_less(99, 18)) {
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
	if(file->is_version_atleast(89, 5)) {
		file->rdwr_long(time_to_life);
	}
	// there might be crashes if world is destroyed after loading
	// without a sync-step being performed
	if(file->is_loading()  &&  time_to_life<=0) {
		time_to_life = 1;
	}

	// avoid endless growth of the values
	// this causes lockups near 2**32
	weg_next &= 65535;
}

void road_user_t::finish_rd()
{
	calc_height(NULL);
	calc_image();
}


/**********************************************************************************************************************/
/* statsauto_t (city cars) from here on */


static weighted_vector_tpl<const citycar_desc_t*> liste_timeline;
stringhashtable_tpl<const citycar_desc_t *> private_car_t::table;

bool private_car_t::register_desc(const citycar_desc_t *desc)
{
	if(  table.remove(desc->get_name())  ) {
		dbg->doubled( "citycar", desc->get_name() );
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

	// just to be sure we are removed from this list!
	if(time_to_life>0) {
		welt->sync.remove(this);
	}
	welt->buche( -1, karte_t::WORLD_CITYCARS );
}


private_car_t::private_car_t(loadsave_t *file) :
	road_user_t()
{
	rdwr(file);
	ms_traffic_jam = 0;
	calc_disp_lane();
	if(desc) {
		welt->sync.add(this);
	}
	welt->buche( +1, karte_t::WORLD_CITYCARS );
}


private_car_t::private_car_t(grund_t* gr, koord const target, const char* name) :
	road_user_t(gr, simrand(65535))
{
	desc = name ? table.get(name) : NULL;
	if (desc == NULL) {
		desc = liste_timeline.empty() ? 0 : pick_any_weighted(liste_timeline);
	}

	pos_next_next = koord3d::invalid;
	time_to_life = welt->get_settings().get_stadtauto_duration() << 20;  // ignore welt->ticks_per_world_month_shift;
	current_speed = 48;
	ms_traffic_jam = 0;
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
			pos_next_next = koord3d::invalid;
			if(  hop_check()  ) {
				ms_traffic_jam = 0;
				current_speed = 48;
			}
			else {
				if(  ms_traffic_jam > welt->ticks_per_world_month  &&  old_ms_traffic_jam<=welt->ticks_per_world_month  ) {
					// message after two month, reset waiting timer
					welt->get_message()->add_message( translator::translate("To heavy traffic\nresults in traffic jam.\n"), get_pos().get_2d(), message_t::traffic_jams|message_t::expire_after_one_month_flag, color_idx_to_rgb(COL_ORANGE) );
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

	if(file->is_version_less(86, 2)) {
		time_to_life = simrand(1000000)+10000;
	}
	else if(file->is_version_less(89, 5)) {
		file->rdwr_long(time_to_life);
		time_to_life *= 10000; // converting from hops left to ms since start
	}

	if(file->is_version_less(86, 5)) {
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

	if(file->is_version_less(99, 10)) {
		pos_next_next = koord3d::invalid;
	}
	else {
		pos_next_next.rdwr(file);
	}

	// overtaking status
	if(file->is_version_less(100, 1)) {
		set_tiles_overtaking( 0 );
	}
	else {
		file->rdwr_byte(tiles_overtaking);
		set_tiles_overtaking( tiles_overtaking );
	}
}


bool private_car_t::ist_weg_frei(grund_t *gr)
{
	if(gr->get_top()>200) {
		// already too many things here
		return false;
	}

	// road still there?
	weg_t * str = gr->get_weg(road_wt);
	if(str==NULL) {
		time_to_life = 0;
		return false;
	}

	// calculate new direction
	// are we just turning around?
	const uint8 this_direction = get_direction();
	bool frei = false;
	if(  get_pos()==pos_next_next  ) {
		// turning around => single check
		const uint8 next_direction = ribi_t::backward(this_direction);
		frei = (NULL == no_cars_blocking( gr, NULL, next_direction, next_direction, next_direction ));

		// do not block railroad crossing
		if(frei  &&  str->is_crossing()) {
			const grund_t *gr = welt->lookup(get_pos());
			frei = (NULL == no_cars_blocking( gr, NULL, next_direction, next_direction, next_direction ));
		}
	}
	else {
		// driving on: check for crossings etc. too
		const uint8 next_direction = this->calc_direction(get_pos(), pos_next_next);

		// do not block this crossing (if possible)
		if(ribi_t::is_threeway(str->get_ribi_unmasked())) {
			// but leaving from railroad crossing is more important
			grund_t *gr_here = welt->lookup(get_pos());
			if(  gr_here  &&  gr_here->ist_uebergang()  ) {
				return true;
			}
			grund_t *test = welt->lookup(pos_next_next);
			if(  test  ) {
				uint8 next_90direction = this->calc_direction(pos_next, pos_next_next);
				frei = (NULL == no_cars_blocking( gr, NULL, this_direction, next_direction, next_90direction ));
				if(  frei  ) {
					// check, if it can leave this crossings
					frei = (NULL == no_cars_blocking( test, NULL, next_direction, next_90direction, next_90direction ));
				}
			}
			// this fails with two crossings together; however, I see no easy way out here ...
		}
		else {
			// not a crossing => skip 90 degrees check!
			frei = true;
			// Overtaking vehicles shouldn't have anything blocking them
			if(  !is_overtaking()  ) {
				// not a crossing => skip 90 degrees check!
				vehicle_base_t *dt = no_cars_blocking( gr, NULL, this_direction, next_direction, next_direction );
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
									if(  ocnv==NULL  ||  !can_overtake( ocnv, (ocnv->get_state()==convoi_t::LOADING ? 0 : over->get_max_power_speed()), ocnv->get_length_in_steps()+ocnv->get_vehicle(0)->get_steps())  ) {
										frei = false;
									}
								} else if(  private_car_t* const caut = obj_cast<private_car_t>(dt)  ) {
									if(  !can_overtake(caut, caut->get_desc()->get_topspeed(), VEHICLE_STEPS_PER_TILE)  ) {
										frei = false;
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
					// should not reach here!
					break;
				}
				const uint8 next_direction = ribi_type(dir);
				const uint8 nextnext_direction = ribi_type(dir);
				// test next field after way crossing
				if(no_cars_blocking( test, NULL, next_direction, nextnext_direction, nextnext_direction )) {
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

	if(frei  &&  current_speed==0) {
		ms_traffic_jam = 0;
		current_speed = 48;
	}

	if(!frei) {
		// not free => stop overtaking
		this->set_tiles_overtaking(0);
	}

	return frei;
}


void private_car_t::enter_tile(grund_t* gr)
{
#ifdef DESTINATION_CITYCARS
	if(  target!=koord::invalid  &&  koord_distance(pos_next.get_2d(),target)<10  ) {
		// delete it ...
		time_to_life = 0;
		// was generating pedestrians gere, but not possible with new sync system
	}
#endif
	vehicle_base_t::enter_tile(gr);
	calc_disp_lane();
	gr->get_weg(road_wt)->book(1, WAY_STAT_CONVOIS);
}


grund_t* private_car_t::hop_check()
{
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

	if(  weg->has_sign()  ) {
		const roadsign_t* rs = from->find<roadsign_t>();
		const roadsign_desc_t* rs_desc = rs->get_desc();
		if(  rs_desc->is_traffic_light()  &&  (rs->get_dir()&direction90)==0  ) {
			// red traffic light, but we go on, if we are already on a traffic light
			bool go_on = false;
			if(  const grund_t *gr_current = welt->lookup(get_pos())  ) {
				if(  const roadsign_t *rs = gr_current->find<roadsign_t>()  ) {
					go_on = rs  &&  rs->get_desc()->is_traffic_light()  &&  !from->ist_uebergang();
				}
			}
			if(  !go_on   ) {
				direction = direction90;
				calc_image();
				// wait here
				current_speed = 48;
				weg_next = 0;
				return NULL;
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
			return ist_weg_frei(from) ? from : NULL;
		}

#ifdef DESTINATION_CITYCARS
		static weighted_vector_tpl<koord3d> posliste(4);
		posliste.clear();
		const uint8 offset = ribi_t::is_single(ribi) ? 0 : simrand(4);
		for(uint8 r = 0; r < 4; r++) {
			if(  get_pos().get_2d()==koord::nesw[r]+pos_next.get_2d()  ) {
				continue;
			}
#else
		const uint8 offset = ribi_t::is_single(ribi) ? 0 : simrand(4);
		for(uint8 i = 0; i < 4; i++) {
			const uint8 r = (i+offset)&3;
#endif
			if(  (ribi&ribi_t::nesw[r])!=0  ) {
				grund_t *to;
				if(  from->get_neighbour(to, road_wt, ribi_t::nesw[r])  ) {
					// check, if this is just a single tile deep after a crossing
					weg_t *w = to->get_weg(road_wt);
					if(  ribi_t::is_single(w->get_ribi())  &&  (w->get_ribi()&ribi_t::nesw[r])==0  &&  !ribi_t::is_single(ribi)  ) {
						ribi &= ~ribi_t::nesw[r];
						continue;
					}
					// check, if roadsign forbid next step ...
					if(w->has_sign()) {
						const roadsign_t* rs = to->find<roadsign_t>();
						const roadsign_desc_t* rs_desc = rs->get_desc();
						if(rs_desc->get_min_speed()>desc->get_topspeed()  ||  (rs_desc->is_private_way()  &&  (rs->get_player_mask()&2)==0)  ) {
							// not allowed to go here
							ribi &= ~ribi_t::nesw[r];
							continue;
						}
					}
#ifdef DESTINATION_CITYCARS
					uint32 dist=koord_distance( to->get_pos().get_2d(), target );
					posliste.append( to->get_pos(), dist*dist );
#else
					// ok, now check if we are allowed to go here (i.e. no cars blocking)
					pos_next_next = to->get_pos();
					if(ist_weg_frei(from)) {
						// ok, this direction is fine!
						ms_traffic_jam = 0;
						if(current_speed<48) {
							current_speed = 48;
						}
						return from;
					}
					else {
						pos_next_next = koord3d::invalid;
					}
#endif
				}
				else {
					// not connected?!? => ribi likely wrong
					ribi &= ~ribi_t::nesw[r];
				}
			}
		}
#ifdef DESTINATION_CITYCARS
		if (!posliste.empty()) {
			pos_next_next = pick_any_weighted(posliste);
		}
		else {
			pos_next_next = get_pos();
		}
		if(ist_weg_frei(from)) {
			// ok, this direction is fine!
			ms_traffic_jam = 0;
			if(current_speed<48) {
				current_speed = 48;
			}
			return from;
		}
#else
		// only stumps at single way crossing, all other blocked => turn around
		if(ribi==0) {
			pos_next_next = get_pos();
			return ist_weg_frei(from) ? from : NULL;
		}
#endif
	}
	else {
		if(from  &&  ist_weg_frei(from)) {
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
	set_tiles_overtaking( 0 );
	return NULL;
}



void private_car_t::hop(grund_t* to)
{
	leave_tile();

	if(pos_next_next==get_pos()) {
		direction = calc_set_direction( pos_next, pos_next_next );
		steps_next = 0; // mark for starting at end of tile!
	}
	else {
		direction = calc_set_direction( get_pos(), pos_next_next );
	}
	calc_image();

	// and add to next tile
	set_pos(pos_next);
	enter_tile(to);

	calc_current_speed(to);

	update_tiles_overtaking();
	if(to->ist_uebergang()) {
		to->find<crossing_t>(2)->add_to_crossing(this);
	}
	pos_next = pos_next_next;
	pos_next_next = koord3d::invalid;
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

	if (char const* const maker = desc->get_copyright()) {
		buf.printf(translator::translate("Constructed by %s"), maker);
	}
}


// to make smaller steps than the tile granularity, we have to use this trick
void private_car_t::get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const
{
	vehicle_base_t::get_screen_offset( xoff, yoff, raster_width );

	if(  welt->get_settings().is_drive_left()  ) {
		const int drive_left_dir = ribi_t::get_dir(get_direction());
		xoff += tile_raster_scale_x( driveleft_base_offsets[drive_left_dir][0], raster_width );
		yoff += tile_raster_scale_y( driveleft_base_offsets[drive_left_dir][1], raster_width );
	}

	// eventually shift position to take care of overtaking
	if(  is_overtaking()  ) {
		xoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][0], raster_width);
		yoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_direction())][1], raster_width);
	}
	else if(  is_overtaken()  ) {
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
 */
bool private_car_t::can_overtake( overtaker_t *other_overtaker, sint32 other_speed, sint16 steps_other)
{
	if(  !other_overtaker->can_be_overtaken()  ) {
		return false;
	}

	if(  other_speed == 0  ) {
		/* overtaking a loading convoi
		 * => we can do a lazy check, since halts are always straight
		 */
		grund_t *gr = welt->lookup(get_pos());
		if(  gr==NULL  ) {
			// should never happen, since there is a vehicle in front of us ...
			return false;
		}
		weg_t *str = gr->get_weg(road_wt);
		if(  str==0  ) {
			// also this is not possible, since a car loads in front of is!?!
			return false;
		}

		const ribi_t::ribi direction = get_direction() & str->get_ribi();
		koord3d check_pos = get_pos()+koord((ribi_t::ribi)(str->get_ribi()&direction));
		for(  int tiles=1+(steps_other-1)/(CARUNITS_PER_TILE*VEHICLE_STEPS_PER_CARUNIT);  tiles>=0;  tiles--  ) {
			grund_t *gr = welt->lookup(check_pos);
			if(  gr==NULL  ) {
				return false;
			}
			weg_t *str = gr->get_weg(road_wt);
			if(  str==0  ) {
				return false;
			}
			// not overtaking on railroad crossings ...
			if(  str->is_crossing() ) {
				return false;
			}
			if(  ribi_t::is_threeway(str->get_ribi())  ) {
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
							return false;
						}
					}
					else if(  v->get_waytype()==road_wt  &&  v->get_typ()!=obj_t::pedestrian  ) {
						return false;
					}
				}
			}
			check_pos += koord(direction);
		}
		set_tiles_overtaking( 3+(steps_other-1)/(CARUNITS_PER_TILE*VEHICLE_STEPS_PER_CARUNIT) );
		return true;
	}

	sint32 diff_speed = (sint32)current_speed - other_speed;
	if( diff_speed < kmh_to_speed( 5 ) ) {
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
	sint32 distance = current_speed*((10<<4)+steps_other)/diff_speed;

	// Conditions for overtaking:
	// Flat tiles, with no stops, no crossings, no signs, no change of road speed limit
	koord3d check_pos = get_pos();
	koord pos_prev = check_pos.get_2d();

	grund_t *gr = welt->lookup(check_pos);
	if(  gr==NULL  ) {
		return false;
	}

	weg_t *str = gr->get_weg(road_wt);
	if(  str==0  ) {
		return false;
	}

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
				if(rb->get_min_speed()>desc->get_topspeed()  ||  rb->is_private_way()  ||  rb->is_traffic_light()  ) {
					// do not overtake when road is closed for cars, there is a traffic light or a too high min speed limit
					return false;
				}
			}
		}

		// not overtaking on railroad crossings ...
		if(  str->is_crossing() ) {
			return false;
		}

		// street gets too slow (TODO: should be able to be correctly accounted for)
		if(  current_speed > kmh_to_speed(str->get_max_speed())  ) {
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
				if(  (rib&ribi_t::nesw[r])==0  ||  check_pos.get_2d()+koord::nesw[r]==pos_prev) {
					continue;
				}
				if(gr->get_neighbour(to, road_wt, ribi_t::nesw[r])) {
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
		if(to==NULL  ||  (str=to->get_weg(road_wt))==NULL) {
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
				if(check_pos.get_2d()+koord::nesw[r]==pos_prev) {
					continue;
				}
				if(gr->get_neighbour(to, road_wt, ribi_t::nesw[r])) {
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
		if(to==NULL  ||  (str=to->get_weg(road_wt))==NULL) {
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
	other_overtaker->set_tiles_overtaking( -1-(n_tiles/2) );

	return true;
}
