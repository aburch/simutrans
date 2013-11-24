/**
 * Moving objects for Simutrans.
 * Transport vehicles are defined in simvehikel.h, because they greatly
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
#include "../simtools.h"
#include "../display/simimg.h"
#include "../simunits.h"
#include "../simtypes.h"

#include "simverkehr.h"
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

#include "../besch/stadtauto_besch.h"
#include "../besch/roadsign_besch.h"


#include "../utils/cbuffer_t.h"

/**********************************************************************************************************************/
/* Road users (Verkehrsteilnehmer) basis class from here on */


verkehrsteilnehmer_t::verkehrsteilnehmer_t() :
	vehikel_basis_t()
{
	set_besitzer( welt->get_spieler(1) );
	time_to_life = 0;
	weg_next = 0;
}


/**
 * Ensures that this object is removed correctly from the list
 * of sync step-able things!
 * @author Hj. Malthaner
 */
verkehrsteilnehmer_t::~verkehrsteilnehmer_t()
{
	mark_image_dirty( get_bild(), 0 );
}


verkehrsteilnehmer_t::verkehrsteilnehmer_t(grund_t* bd, uint16 random) :
	vehikel_basis_t(bd ? bd->get_pos() : koord3d::invalid)
{
	ribi_t::ribi road_ribi = bd->get_weg_ribi(road_wt);

	weg_next = random;

	// randomized offset
	uint8 offset = random & 3;
	fahrtrichtung = ribi_t::nsow[offset];

	grund_t *to = NULL;
	for(uint8 r = 0; r < 4; r++) {
		ribi_t::ribi ribi = ribi_t::nsow[ (r + offset) &3];
		if( (ribi & road_ribi)!=0  &&  bd->get_neighbour(to, road_wt, ribi)) {
			fahrtrichtung = ribi;
			break;
		}
	}

	switch(fahrtrichtung) {
		case ribi_t::nord:
			dx = 2;
			dy = -1;
			break;
		case ribi_t::sued:
			dx = -2;
			dy = 1;
			break;
		case ribi_t::ost:
			dx = 2;
			dy = 1;
			break;
		case ribi_t::west:
			dx = -2;
			dy = -1;
			break;
	}
	hoff = 0;

	if(to) {
		pos_next = to->get_pos();
	}
	else {
		pos_next = welt->lookup_kartenboden(get_pos().get_2d() + koord(fahrtrichtung))->get_pos();
	}
	set_besitzer( welt->get_spieler(1) );
}


/**
 * Open a new observation window for the object.
 * @author Hj. Malthaner
 */
void verkehrsteilnehmer_t::zeige_info()
{
	if(env_t::verkehrsteilnehmer_info) {
		obj_t::zeige_info();
	}
}


void verkehrsteilnehmer_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "verkehrsteilnehmer_t" );

	// correct old offsets ... REMOVE after savegame increase ...
	if(file->get_version()<99018  &&  file->is_saving()) {
		dx = dxdy[ ribi_t::get_dir(fahrtrichtung)*2 ];
		dy = dxdy[ ribi_t::get_dir(fahrtrichtung)*2+1 ];
		sint8 i = steps/16;
		set_xoff( get_xoff() + i*dx );
		set_yoff( get_yoff() + i*dy + hoff );
	}

	vehikel_basis_t::rdwr(file);

	if(file->get_version() < 86006) {
		sint32 l;
		file->rdwr_long(l);
		file->rdwr_long(l);
		file->rdwr_long(weg_next);
		file->rdwr_long(l);
		dx = (sint8)l;
		file->rdwr_long(l);
		dy = (sint8)l;
		file->rdwr_enum(fahrtrichtung);
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
		file->rdwr_enum(fahrtrichtung);
		dx = dxdy[ ribi_t::get_dir(fahrtrichtung)*2];
		dy = dxdy[ ribi_t::get_dir(fahrtrichtung)*2+1];
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
			if(dx*dy) {
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

	// Hajo: avoid endless growth of the values
	// this causes lockups near 2**32
	weg_next &= 65535;
}



/**********************************************************************************************************************/
/* statsauto_t (city cars) from here on */


static weighted_vector_tpl<const stadtauto_besch_t*> liste_timeline;
stringhashtable_tpl<const stadtauto_besch_t *> stadtauto_t::table;

bool stadtauto_t::register_besch(const stadtauto_besch_t *besch)
{
	if(  table.remove(besch->get_name())  ) {
		dbg->warning( "stadtauto_besch_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
	}
	table.put(besch->get_name(), besch);
	return true;
}



bool stadtauto_t::alles_geladen()
{
	if(table.empty()) {
		DBG_MESSAGE("stadtauto_t", "No citycars found - feature disabled");
	}
	return true;
}


static bool compare_stadtauto_besch(const stadtauto_besch_t* a, const stadtauto_besch_t* b)
{
	int diff = a->get_intro_year_month() - b->get_intro_year_month();
	if (diff == 0) {
		diff = a->get_geschw() - b->get_geschw();
	}
	if (diff == 0) {
		/* same Level - we introduce an artificial, but unique resort
		 * on the induced name. */
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}


void stadtauto_t::built_timeline_liste(karte_t *welt)
{
	// this list will contain all citycars
	liste_timeline.clear();
	vector_tpl<const stadtauto_besch_t*> temp_liste(0);
	if(  !table.empty()  ) {
		const int month_now = welt->get_current_month();
//DBG_DEBUG("stadtauto_t::built_timeline_liste()","year=%i, month=%i", month_now/12, month_now%12+1);

		// check for every citycar, if still ok ...
		FOR(stringhashtable_tpl<stadtauto_besch_t const*>, const& i, table) {
			stadtauto_besch_t const* const info = i.value;
			const int intro_month = info->get_intro_year_month();
			const int retire_month = info->get_retire_year_month();

			if (!welt->use_timeline() || (intro_month <= month_now && month_now < retire_month)) {
				temp_liste.insert_ordered( info, compare_stadtauto_besch );
			}
		}
	}
	liste_timeline.resize( temp_liste.get_count() );
	FOR(vector_tpl<stadtauto_besch_t const*>, const i, temp_liste) {
		liste_timeline.append(i, i->get_gewichtung());
	}
}



bool stadtauto_t::list_empty()
{
	return liste_timeline.empty();
}



stadtauto_t::~stadtauto_t()
{
	// first: release crossing
	grund_t *gr = welt->lookup(get_pos());
	if(gr  &&  gr->ist_uebergang()) {
		gr->find<crossing_t>(2)->release_crossing(this);
	}

	// just to be sure we are removed from this list!
	if(time_to_life>0) {
		welt->sync_remove(this);
	}
	welt->buche( -1, karte_t::WORLD_CITYCARS );
}


stadtauto_t::stadtauto_t(loadsave_t *file) :
	verkehrsteilnehmer_t()
{
	rdwr(file);
	ms_traffic_jam = 0;
	if(besch) {
		welt->sync_add(this);
	}
	welt->buche( +1, karte_t::WORLD_CITYCARS );
}


stadtauto_t::stadtauto_t(grund_t* gr, koord const target) :
	verkehrsteilnehmer_t(gr, simrand(65535)),
	besch(liste_timeline.empty() ? 0 : pick_any_weighted(liste_timeline))
{
	pos_next_next = koord3d::invalid;
	time_to_life = welt->get_settings().get_stadtauto_duration() << welt->ticks_per_world_month_shift;
	current_speed = 48;
	ms_traffic_jam = 0;
#ifdef DESTINATION_CITYCARS
	this->target = target;
#else
	(void)target;
#endif
	calc_bild();
	welt->buche( +1, karte_t::WORLD_CITYCARS );
}


bool stadtauto_t::sync_step(long delta_t)
{
	time_to_life -= delta_t;
	if(  time_to_life<=0  ) {
		return false;
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
					welt->get_message()->add_message( translator::translate("To heavy traffic\nresults in traffic jam.\n"), get_pos().get_2d(), message_t::traffic_jams, COL_ORANGE );
				}
			}
		}
		weg_next = 0;
	}
	else {
		weg_next += current_speed*delta_t;
		const uint32 distance = fahre_basis( weg_next );
		// hop_check could have set weg_next to zero, check for possible underflow here
		if (weg_next > distance) {
			weg_next -= distance;
		}
		else {
			weg_next = 0;
		}
	}

	return time_to_life>0;
}


void stadtauto_t::rdwr(loadsave_t *file)
{
	xml_tag_t s( file, "stadtauto_t" );

	verkehrsteilnehmer_t::rdwr(file);

	if(file->is_saving()) {
		const char *s = besch->get_name();
		file->rdwr_str(s);
	}
	else {
		char s[256];
		file->rdwr_str(s, lengthof(s));
		besch = table.get(s);

		if(  besch == 0  &&  !liste_timeline.empty()  ) {
			dbg->warning("stadtauto_t::rdwr()", "Object '%s' not found in table, trying random stadtauto object type",s);
			besch = pick_any_weighted(liste_timeline);
		}

		if(besch == 0) {
			dbg->warning("stadtauto_t::rdwr()", "loading game with private cars, but no private car objects found in PAK files.");
		}
		else {
			set_bild(besch->get_bild_nr(ribi_t::get_dir(get_fahrtrichtung())));
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

	// do not start with zero speed!
	current_speed ++;
}


bool stadtauto_t::ist_weg_frei(grund_t *gr)
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
	const uint8 this_fahrtrichtung = get_fahrtrichtung();
	bool frei = false;
	if(  get_pos()==pos_next_next  ) {
		// turning around => single check
		const uint8 next_fahrtrichtung = ribi_t::rueckwaerts(this_fahrtrichtung);
		frei = (NULL == no_cars_blocking( gr, NULL, next_fahrtrichtung, next_fahrtrichtung, next_fahrtrichtung ));

		// do not block railroad crossing
		if(frei  &&  str->is_crossing()) {
			const grund_t *gr = welt->lookup(get_pos());
			frei = (NULL == no_cars_blocking( gr, NULL, next_fahrtrichtung, next_fahrtrichtung, next_fahrtrichtung ));
		}
	}
	else {
		// driving on: check for crossings etc. too
		const uint8 next_fahrtrichtung = this->calc_richtung(get_pos().get_2d(), pos_next_next.get_2d());

		// do not block this crossing (if possible)
		if(ribi_t::is_threeway(str->get_ribi_unmasked())) {
			// but leaving from railroad crossing is more important
			grund_t *gr_here = welt->lookup(get_pos());
			if(  gr_here  &&  gr_here->ist_uebergang()  ) {
				return true;
			}
			grund_t *test = welt->lookup(pos_next_next);
			if(  test  ) {
				uint8 next_90fahrtrichtung = this->calc_richtung(pos_next.get_2d(), pos_next_next.get_2d());
				frei = (NULL == no_cars_blocking( gr, NULL, this_fahrtrichtung, next_fahrtrichtung, next_90fahrtrichtung ));
				if(  frei  ) {
					// check, if it can leave this crossings
					frei = (NULL == no_cars_blocking( test, NULL, next_fahrtrichtung, next_90fahrtrichtung, next_90fahrtrichtung ));
				}
			}
			// this fails with two crossings together; however, I see no easy way out here ...
		}
		else {
			// not a crossing => skip 90° check!
			frei = true;
			// Overtaking vehicles shouldn't have anything blocking them
			if(  !is_overtaking()  ) {
				// not a crossing => skip 90° check!
				vehikel_basis_t *dt = no_cars_blocking( gr, NULL, this_fahrtrichtung, next_fahrtrichtung, next_fahrtrichtung );
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
								if(  automobil_t const* const car = obj_cast<automobil_t>(dt)  ) {
									convoi_t* const ocnv = car->get_convoi();
									if(  ocnv==NULL  ||  !can_overtake( ocnv, (ocnv->get_state()==convoi_t::LOADING ? 0 : over->get_max_power_speed()), ocnv->get_length_in_steps()+ocnv->get_vehikel(0)->get_steps())  ) {
										frei = false;
									}
								} else if(  stadtauto_t* const caut = obj_cast<stadtauto_t>(dt)  ) {
									if(  !can_overtake(caut, caut->get_besch()->get_geschw(), VEHICLE_STEPS_PER_TILE)  ) {
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
					// should not reach here ! (z9999)
					break;
				}
				const uint8 next_fahrtrichtung = ribi_typ(dir);
				const uint8 nextnext_fahrtrichtung = ribi_typ(dir);
				// test next field after way crossing
				if(no_cars_blocking( test, NULL, next_fahrtrichtung, nextnext_fahrtrichtung, nextnext_fahrtrichtung )) {
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
					if(  (test->get_weg_ribi(road_wt)&next_fahrtrichtung) == 0  ) {
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


grund_t* stadtauto_t::betrete_feld()
{
#ifdef DESTINATION_CITYCARS
	if(target!=koord::invalid  &&  koord_distance(pos_next.get_2d(),target)<10) {
		// delete it ...
		time_to_life = 0;
		fussgaenger_t::erzeuge_fussgaenger_an(welt, get_pos(), 2);
	}
#endif
	grund_t *gr = vehikel_basis_t::betrete_feld();
	gr->get_weg(road_wt)->book(1, WAY_STAT_CONVOIS);
	return gr;
}



bool stadtauto_t::hop_check()
{
	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *from = welt->lookup(pos_next);
	if(from==NULL) {
		// nothing to go? => destroy ...
		time_to_life = 0;
		return false;
	}

	// find the allowed directions
	const weg_t *weg = from->get_weg(road_wt);
	if(weg==NULL) {
		// nothing to go? => destroy ...
		time_to_life = 0;
		return false;
	}

	// traffic light phase check (since this is on next tile, it will always be necessary!)
	const ribi_t::ribi fahrtrichtung90 = ribi_typ(get_pos().get_2d(),pos_next.get_2d());

	if(  weg->has_sign(  )) {
		const roadsign_t* rs = from->find<roadsign_t>();
		const roadsign_besch_t* rs_besch = rs->get_besch();
		if(rs_besch->is_traffic_light()  &&  (rs->get_dir()&fahrtrichtung90)==0) {
			fahrtrichtung = fahrtrichtung90;
			calc_bild();
			// wait here
			current_speed = 48;
			weg_next = 0;
			return false;
		}
	}

	// next tile unknown => find next tile
	if(pos_next_next==koord3d::invalid) {

		// ok, nobody did delete the road in front of us
		// so we can check for valid directions
		ribi_t::ribi ribi = weg->get_ribi() & (~ribi_t::rueckwaerts(fahrtrichtung90));

		// cul de sac: return
		if(ribi==0) {
			pos_next_next = get_pos();
			return ist_weg_frei(from);
		}

#ifdef DESTINATION_CITYCARS
		static weighted_vector_tpl<koord3d> posliste(4);
		posliste.clear();
		const uint8 offset = ribi_t::ist_einfach(ribi) ? 0 : simrand(4);
		for(uint8 r = 0; r < 4; r++) {
			if(  get_pos().get_2d()==koord::nsow[r]+pos_next.get_2d()  ) {
				continue;
			}
#else
		const uint8 offset = ribi_t::ist_einfach(ribi) ? 0 : simrand(4);
		for(uint8 i = 0; i < 4; i++) {
			const uint8 r = (i+offset)&3;
#endif
			if(  (ribi&ribi_t::nsow[r])!=0  ) {
				grund_t *to;
				if(  from->get_neighbour(to, road_wt, ribi_t::nsow[r])  ) {
					// check, if this is just a single tile deep after a crossing
					weg_t *w = to->get_weg(road_wt);
					if(  ribi_t::ist_einfach(w->get_ribi())  &&  (w->get_ribi()&ribi_t::nsow[r])==0  &&  !ribi_t::ist_einfach(ribi)  ) {
						ribi &= ~ribi_t::nsow[r];
						continue;
					}
					// check, if roadsign forbid next step ...
					if(w->has_sign()) {
						const roadsign_t* rs = to->find<roadsign_t>();
						const roadsign_besch_t* rs_besch = rs->get_besch();
						if(rs_besch->get_min_speed()>besch->get_geschw()  ||  (rs_besch->is_private_way()  &&  (rs->get_player_mask()&2)==0)  ) {
							// not allowed to go here
							ribi &= ~ribi_t::nsow[r];
							continue;
						}
					}
#ifdef DESTINATION_CITYCARS
					unsigned long dist=koord_distance( to->get_pos().get_2d(), target );
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
						return true;
					}
					else {
						pos_next_next = koord3d::invalid;
					}
#endif
				}
				else {
					// not connected?!? => ribi likely wrong
					ribi &= ~ribi_t::nsow[r];
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
			return true;
		}
#else
		// only stumps at single way crossing, all other blocked => turn around
		if(ribi==0) {
			pos_next_next = get_pos();
			return ist_weg_frei(from);
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
			return true;
		}
	}
	// no free tiles => assume traffic jam ...
	pos_next_next = koord3d::invalid;
	current_speed = 0;
	set_tiles_overtaking( 0 );
	return false;
}



grund_t* stadtauto_t::hop()
{
	verlasse_feld();

	if(pos_next_next==get_pos()) {
		fahrtrichtung = calc_set_richtung( pos_next, pos_next_next );
		steps_next = 0;	// mark for starting at end of tile!
	}
	else {
		fahrtrichtung = calc_set_richtung( get_pos(), pos_next_next );
	}
	calc_bild();

	// and add to next tile
	set_pos(pos_next);
	grund_t *to = betrete_feld();

	calc_current_speed(to);

	update_tiles_overtaking();
	if(to->ist_uebergang()) {
		to->find<crossing_t>(2)->add_to_crossing(this);
	}
	pos_next = pos_next_next;
	pos_next_next = koord3d::invalid;

	return to;
}



void stadtauto_t::calc_bild()
{
	set_bild(besch->get_bild_nr(ribi_t::get_dir(get_fahrtrichtung())));
	drives_on_left = welt->get_settings().is_drive_left();	// reset driving settings
}



void stadtauto_t::calc_current_speed(grund_t* gr)
{
	const weg_t * weg = gr->get_weg(road_wt);
	const sint32 max_speed = besch->get_geschw();
	const sint32 speed_limit = weg ? kmh_to_speed(weg->get_max_speed()) : max_speed;
	current_speed += max_speed>>2;
	if(current_speed > max_speed) {
		current_speed = max_speed;
	}
	if(current_speed > speed_limit) {
		current_speed = speed_limit;
	}
}


void stadtauto_t::info(cbuffer_t & buf) const
{
	buf.printf(translator::translate("%s\nspeed %i\nmax_speed %i\ndx:%i dy:%i"), translator::translate(besch->get_name()), speed_to_kmh(current_speed), speed_to_kmh(besch->get_geschw()), dx, dy);
}



// to make smaller steps than the tile granularity, we have to use this trick
void stadtauto_t::get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const
{
	vehikel_basis_t::get_screen_offset( xoff, yoff, raster_width );

	// eventually shift position to take care of overtaking
	if(  is_overtaking()  ) {
		xoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_fahrtrichtung())][0], raster_width);
		yoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_fahrtrichtung())][1], raster_width);
	}
	else if(  is_overtaken()  ) {
		xoff -= tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_fahrtrichtung())][0], raster_width)/5;
		yoff -= tile_raster_scale_x(overtaking_base_offsets[ribi_t::get_dir(get_fahrtrichtung())][1], raster_width)/5;
	}
}



/**
 * conditions for a city car to overtake another overtaker.
 * The city car is not overtaking/being overtaken.
 * @author isidoro
 */
bool stadtauto_t::can_overtake( overtaker_t *other_overtaker, sint32 other_speed, sint16 steps_other)
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

		const ribi_t::ribi direction = get_fahrtrichtung() & str->get_ribi();
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
				if(  vehikel_basis_t* const v = obj_cast<vehikel_basis_t>(gr->obj_bei(j))  ) {
					// check for other traffic on the road
					const overtaker_t *ov = v->get_overtaker();
					if(ov) {
						if(this!=ov  &&  other_overtaker!=ov) {
							return false;
						}
					}
					else if(  v->get_waytype()==road_wt  &&  v->get_typ()!=obj_t::fussgaenger  ) {
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
	sint32 distance = current_speed*((10<<4)+steps_other)/max(besch->get_geschw()-other_speed,diff_speed);

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
	ribi_t::ribi direction = get_fahrtrichtung();
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
				const roadsign_besch_t *rb = rs->get_besch();
				if(rb->get_min_speed()>besch->get_geschw()  ||  rb->is_private_way()  ||  rb->is_traffic_light()  ) {
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
		if(  besch->get_geschw() > kmh_to_speed(str->get_max_speed())  ) {
			return false;
		}

		int d = ribi_t::ist_gerade(str->get_ribi()) ? VEHICLE_STEPS_PER_TILE : diagonal_vehicle_steps_per_tile;
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
				if(  (rib&ribi_t::nsow[r])==0  ||  check_pos.get_2d()+koord::nsow[r]==pos_prev) {
					continue;
				}
				if(gr->get_neighbour(to, road_wt, ribi_t::nsow[r])) {
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
			if(  vehikel_basis_t* const v = obj_cast<vehikel_basis_t>(gr->obj_bei(j))  ) {
				// check for other traffic on the road
				const overtaker_t *ov = v->get_overtaker();
				if(ov) {
					if(this!=ov  &&  other_overtaker!=ov) {
						return false;
					}
				}
				else if(  v->get_waytype()==road_wt  &&  v->get_typ()!=obj_t::fussgaenger  ) {
					return false;
				}
			}
		}

		gr = to;
		check_pos = to->get_pos();

		direction = ~ribi_typ( check_pos.get_2d(),pos_prev ) & str->get_ribi();
	}

	// Second phase: only facing traffic is forbidden
	//   Since street speed can change, we do the calculation with time.
	//   Each empty tile will subtract tile_dimension/max_street_speed.
	//   If time is exhausted, we are guaranteed that no facing traffic will
	//   invade the dangerous zone.
	time_overtaking = (time_overtaking << 16)/(sint32)current_speed;
	do {
		// we can allow crossings or traffic lights here, since they will stop also oncoming traffic
		if(  ribi_t::ist_gerade(str->get_ribi())  ) {
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
				if(check_pos.get_2d()+koord::nsow[r]==pos_prev) {
					continue;
				}
				if(gr->get_neighbour(to, road_wt, ribi_t::nsow[r])) {
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
		ribi_t::ribi their_direction = ribi_t::rueckwaerts(calc_richtung( pos_prev_prev, to->get_pos().get_2d() ));
		const uint8 top = gr->get_top();
		for(  uint8 j=1;  j<top;  j++ ) {
			vehikel_basis_t* const v = obj_cast<vehikel_basis_t>(gr->obj_bei(j));
			if(  v  &&  v->get_fahrtrichtung() == their_direction  ) {
				// check for car
				if(v->get_overtaker()) {
					return false;
				}
			}
		}

		gr = to;
		check_pos = to->get_pos();

		direction = ~ribi_typ( check_pos.get_2d(),pos_prev ) & str->get_ribi();
	} while( time_overtaking > 0 );

	set_tiles_overtaking( 1+n_tiles );
	other_overtaker->set_tiles_overtaking( -1-(n_tiles/2) );

	return true;
}
