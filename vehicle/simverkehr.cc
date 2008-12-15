/**
 * Bewegliche Objekte fuer Simutrans.
 * Transportfahrzeuge sind in simvehikel.h definiert, da sie sich
 * stark von den hier definierten Fahrzeugen fuer den Individualverkehr
 * unterscheiden.
 *
 * Hj. Malthaner
 *
 * April 2000
 */

#include "../simdebug.h"
#include "../simgraph.h"
#include "../simmesg.h"
#include "../simworld.h"
#include "../simtools.h"
#include "../simmem.h"
#include "../simimg.h"
#include "../simconst.h"
#include "../simtypes.h"

#include "simverkehr.h"
#ifdef DESTINATION_CITYCARS
// for final citcar destinations
#include "simpeople.h"
#endif

#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/umgebung.h"

#include "../dings/crossing.h"
#include "../dings/roadsign.h"

#include "../boden/grund.h"
#include "../boden/wege/weg.h"

#include "../besch/stadtauto_besch.h"
#include "../besch/roadsign_besch.h"


#include "../utils/cbuffer_t.h"

/**********************************************************************************************************************/
/* Verkehrsteilnehmer (basis class) from here on */


verkehrsteilnehmer_t::verkehrsteilnehmer_t(karte_t *welt) :
   vehikel_basis_t(welt)
{
	setze_besitzer( welt->gib_spieler(1) );
	time_to_life = -1;
	weg_next = 0;
}


/**
 * Ensures that this object is removed correctly from the list
 * of sync steppable things!
 * @author Hj. Malthaner
 */
verkehrsteilnehmer_t::~verkehrsteilnehmer_t()
{
	mark_image_dirty( gib_bild(), 0 );
	// first: release crossing
	grund_t *gr = welt->lookup(gib_pos());
	if(gr  &&  gr->ist_uebergang()) {
		gr->find<crossing_t>(2)->release_crossing(this);
	}

	// just to be sure we are removed from this list!
	if(time_to_life>=0) {
		welt->sync_remove(this);
	}
}




verkehrsteilnehmer_t::verkehrsteilnehmer_t(karte_t *welt, koord3d pos) :
   vehikel_basis_t(welt, pos)
{
	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *from = welt->lookup(pos);
	grund_t *to;

	// int ribi = from->gib_weg_ribi(road_wt);
	ribi_t::ribi liste[4];
	int count = 0;

	weg_next = simrand(65535);
	hoff = 0;

	// verfügbare ribis in liste eintragen
	for(int r = 0; r < 4; r++) {
		if(from->get_neighbour(to, road_wt, koord::nsow[r])) {
			liste[count++] = ribi_t::nsow[r];
		}
	}
	fahrtrichtung = count ? liste[simrand(count)] : ribi_t::nsow[simrand(4)];

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
	if(count) {
		from->get_neighbour(to, road_wt, fahrtrichtung);
		pos_next = to->gib_pos();
	} else {
		pos_next = welt->lookup(pos.gib_2d() + koord(fahrtrichtung))->gib_kartenboden()->gib_pos();
	}
	setze_besitzer( welt->gib_spieler(1) );
}


/**
 * Öffnet ein neues Beobachtungsfenster für das Objekt.
 * @author Hj. Malthaner
 */
void verkehrsteilnehmer_t::zeige_info()
{
	if(umgebung_t::verkehrsteilnehmer_info) {
		ding_t::zeige_info();
	}
}


void
verkehrsteilnehmer_t::hop()
{
	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *from = welt->lookup(pos_next);
	grund_t *to;

	if(!from) {
		time_to_life = 0;
		return;
	}

	grund_t *liste[4];
	int count = 0;

	// 1) find the allowed directions
	const weg_t *weg = from->gib_weg(road_wt);
	if(weg==NULL) {
		// no gound here any more?
		pos_next = gib_pos();
		from = welt->lookup(pos_next);
		if(!from  ||  !from->hat_weg(road_wt)) {
			// destroy it
			time_to_life = 0;
		}
		return;
	}

	// add all good ribis here
	ribi_t::ribi gegenrichtung = ribi_t::rueckwaerts( gib_fahrtrichtung() );
	int ribi = weg->gib_ribi_unmasked();
	for(int r = 0; r < 4; r++) {
		if(  (ribi & ribi_t::nsow[r])!=0  &&  (ribi_t::nsow[r]&gegenrichtung)==0 &&
			from->get_neighbour(to, road_wt, koord::nsow[r])
		) {
			// check, if this is just a single tile deep
			int next_ribi =  to->gib_weg(road_wt)->gib_ribi_unmasked();
			if((ribi&next_ribi)!=0  ||  !ribi_t::ist_einfach(next_ribi)) {
				liste[count++] = to;
			}
		}
	}

	if(count > 1) {
		pos_next = liste[simrand(count)]->gib_pos();
		fahrtrichtung = calc_set_richtung(gib_pos().gib_2d(), pos_next.gib_2d());
	} else if(count==1) {
		pos_next = liste[0]->gib_pos();
		fahrtrichtung = calc_set_richtung(gib_pos().gib_2d(), pos_next.gib_2d());
	}
	else {
		fahrtrichtung = gegenrichtung;
		dx = -dx;
		dy = -dy;
		pos_next = gib_pos();
	}

	verlasse_feld();
	setze_pos(from->gib_pos());
	calc_bild();
	betrete_feld();
}



void verkehrsteilnehmer_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "verkehrsteilnehmer_t" );

	// correct old offsets ... REMOVE after savegame increase ...
	if(file->get_version()<99018  &&  file->is_saving()) {
		dx = dxdy[ ribi_t::gib_dir(fahrtrichtung)*2 ];
		dy = dxdy[ ribi_t::gib_dir(fahrtrichtung)*2+1 ];
		sint8 i = steps/16;
		setze_xoff( gib_xoff() + i*dx );
		setze_yoff( gib_yoff() + i*dy + hoff );
	}

	vehikel_basis_t::rdwr(file);

	if(file->get_version() < 86006) {
		sint32 l;
		file->rdwr_long(l, "\n");
		file->rdwr_long(l, " ");
		file->rdwr_long(weg_next, "\n");
		file->rdwr_long(l, " ");
		dx = (sint8)l;
		file->rdwr_long(l, "\n");
		dy = (sint8)l;
		file->rdwr_enum(fahrtrichtung, " ");
		file->rdwr_long(l, "\n");
		hoff = (sint8)l;
	}
	else {
		if(file->get_version()<99005) {
			sint32 dummy32;
			file->rdwr_long(dummy32, "\n");
		}
		file->rdwr_long(weg_next, "\n");
		if(file->get_version()<99018) {
			file->rdwr_byte(dx, " ");
			file->rdwr_byte(dy, "\n");
		}
		else {
			file->rdwr_byte(steps, " ");
			file->rdwr_byte(steps_next, "\n");
		}
		file->rdwr_enum(fahrtrichtung, " ");
		dx = dxdy[ ribi_t::gib_dir(fahrtrichtung)*2];
		dy = dxdy[ ribi_t::gib_dir(fahrtrichtung)*2+1];
		if(file->get_version()<99005  ||  file->get_version()>99016) {
			sint16 dummy16 = ((16*(sint16)hoff)/TILE_STEPS);
			file->rdwr_short(dummy16, "\n");
			hoff = (sint8)((TILE_STEPS*(sint16)dummy16)/16);
		}
		else {
			file->rdwr_byte(hoff, "\n");
		}
	}
	pos_next.rdwr(file);

	// convert steps to position
	if(file->get_version()<99018) {
		sint8 ddx=gib_xoff(), ddy=gib_yoff()-hoff;
		sint8 i=0;

		while(  !is_about_to_hop(ddx+dx*i,ddy+dy*i )  &&  i<16 ) {
			i++;
		}
		setze_xoff( ddx-(16-i)*dx );
		setze_yoff( ddy-(16-i)*dy );
		if(file->is_loading()) {
			if(dx*dy) {
				steps = min( 255, 255-(i*16) );
				steps_next = 255;
			}
			else {
				steps = min( 127, 128-(i*16) );
				steps_next = 127;
			}
		}
	}

	// the lifetime in ms
	if(file->get_version()>89004) {
		file->rdwr_long( time_to_life, "t" );
	}

	// Hajo: avoid endless growth of the values
	// this causes lockups near 2**32
	weg_next &= 65535;
}



/**********************************************************************************************************************/
/* statsauto_t (city cars) from here on */


static weighted_vector_tpl<const stadtauto_besch_t*> liste_timeline;
static weighted_vector_tpl<const stadtauto_besch_t*> liste;
stringhashtable_tpl<const stadtauto_besch_t *> stadtauto_t::table;

void stadtauto_t::built_timeline_liste(karte_t *welt)
{
	if (!liste.empty()) {
		// this list will contain all citycars
		liste_timeline.clear();
		const int month_now = welt->get_current_month();

//DBG_DEBUG("stadtauto_t::built_timeline_liste()","year=%i, month=%i", month_now/12, month_now%12+1);

		// check for every citycar, if still ok ...
		for (weighted_vector_tpl<const stadtauto_besch_t*>::const_iterator i = liste.begin(), end = liste.end(); i != end; ++i) {
			const stadtauto_besch_t* info = *i;
			const int intro_month = info->get_intro_year_month();
			const int retire_month = info->get_retire_year_month();

//DBG_DEBUG("stadtauto_t::built_timeline_liste()","iyear=%i, imonth=%i", intro_month/12, intro_month%12+1);
//DBG_DEBUG("stadtauto_t::built_timeline_liste()","ryear=%i, rmonth=%i", retire_month/12, retire_month%12+1);

			if (!welt->use_timeline() || (intro_month <= month_now && month_now < retire_month)) {
				liste_timeline.append(info, info->gib_gewichtung(), 1);
//DBG_DEBUG("stadtauto_t::built_timeline_liste()","adding %s to liste",info->gib_name());
			}
		}
	}
}



bool stadtauto_t::register_besch(const stadtauto_besch_t *besch)
{
	liste.append(besch, besch->gib_gewichtung(), 1);
	table.put(besch->gib_name(), besch);
	// correct for driving on left side
	if(umgebung_t::drive_on_left) {
		const int XOFF=(12*get_tile_raster_width())/64;
		const int YOFF=(6*get_tile_raster_width())/64;

		display_set_base_image_offset( besch->gib_bild_nr(0), +XOFF, +YOFF );
		display_set_base_image_offset( besch->gib_bild_nr(1), -XOFF, +YOFF );
		display_set_base_image_offset( besch->gib_bild_nr(2), 0, +YOFF );
		display_set_base_image_offset( besch->gib_bild_nr(3), +XOFF, 0 );
		display_set_base_image_offset( besch->gib_bild_nr(4), -XOFF, -YOFF );
		display_set_base_image_offset( besch->gib_bild_nr(5), +XOFF, -YOFF );
		display_set_base_image_offset( besch->gib_bild_nr(6), 0, -YOFF );
		display_set_base_image_offset( besch->gib_bild_nr(7), -XOFF-YOFF, 0 );
	}
	return true;
}



bool stadtauto_t::laden_erfolgreich()
{
	if (liste.empty()) {
		DBG_MESSAGE("stadtauto_t", "No citycars found - feature disabled");
	}
	return true;
}


bool stadtauto_t::list_empty()
{
	return liste_timeline.empty();
}


stadtauto_t::~stadtauto_t()
{
	welt->buche( -1, karte_t::WORLD_CITYCARS );
}


stadtauto_t::stadtauto_t(karte_t *welt, loadsave_t *file)
 : verkehrsteilnehmer_t(welt)
{
	rdwr(file);
	ms_traffic_jam = 0;
	if(besch) {
		welt->sync_add(this);
	}
	welt->buche( +1, karte_t::WORLD_CITYCARS );
}


#ifdef DESTINATION_CITYCARS
stadtauto_t::stadtauto_t(karte_t *welt, koord3d pos, koord target)
#else
stadtauto_t::stadtauto_t(karte_t *welt, koord3d pos, koord )
#endif
 : verkehrsteilnehmer_t(welt, pos)
{
	besch = liste_timeline.at_weight(simrand(liste_timeline.get_sum_weight()));
	if(!besch) {
		besch = liste.at_weight(simrand(liste.get_sum_weight()));
	}
	pos_next_next = koord3d::invalid;
	time_to_life = welt->gib_einstellungen()->gib_stadtauto_duration() << welt->ticks_bits_per_tag;
	current_speed = 48;
	ms_traffic_jam = 0;
#ifdef DESTINATION_CITYCARS
	this->target = target;
#endif
	calc_bild();
	welt->buche( +1, karte_t::WORLD_CITYCARS );
}




bool
stadtauto_t::sync_step(long delta_t)
{
	if(time_to_life<=0) {
		// remove obj
  		return false;
	}

	time_to_life -= delta_t;

	if(current_speed==0) {
		// stuck in traffic jam
		uint32 old_ms_traffic_jam = ms_traffic_jam;
		ms_traffic_jam += delta_t;
		if((ms_traffic_jam>>7)!=(old_ms_traffic_jam>>7)) {
			pos_next_next = koord3d::invalid;
			if(hop_check()) {
				ms_traffic_jam = 0;
				current_speed = 48;
			}
			else {
				if(ms_traffic_jam>welt->ticks_per_tag  &&  old_ms_traffic_jam<=welt->ticks_per_tag) {
					// message after two month, reset waiting timer
					welt->get_message()->add_message( translator::translate("To heavy traffic\nresults in traffic jam.\n"), gib_pos().gib_2d(), message_t::warnings, COL_ORANGE );
				}
			}
		}
		weg_next = 0;
	}
	else {
		weg_next += current_speed*delta_t;
		weg_next -= fahre_basis( weg_next );
	}

	return true;
}



void stadtauto_t::rdwr(loadsave_t *file)
{
	xml_tag_t s( file, "stadtauto_t" );

	verkehrsteilnehmer_t::rdwr(file);

	if(file->is_saving()) {
		const char *s = besch->gib_name();
		file->rdwr_str(s);
	}
	else {
		char s[256];
		file->rdwr_str(s, 256);
		besch = table.get(s);

		if (besch == 0 && !liste_timeline.empty()) {
			dbg->warning("stadtauto_t::rdwr()", "Object '%s' not found in table, trying random stadtauto object type",s);
			besch = liste_timeline.at_weight(simrand(liste_timeline.get_sum_weight()));
		}
		if (besch == 0 && !liste.empty()) {
			dbg->warning("stadtauto_t::rdwr()", "Object '%s' not found in table, trying random stadtauto object type",s);
			besch = liste.at_weight(simrand(liste.get_sum_weight()));
		}

		if(besch == 0) {
			dbg->warning("stadtauto_t::rdwr()", "loading game with private cars, but no private car objects found in PAK files.");
		}
		else {
			setze_bild(besch->gib_bild_nr(ribi_t::gib_dir(gib_fahrtrichtung())));
		}
	}

	if(file->get_version() <= 86001) {
		time_to_life = simrand(1000000)+10000;
	}
	else if(file->get_version() <= 89004) {
		file->rdwr_long(time_to_life, "\n");
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
		file->rdwr_long(dummy32, "\n");
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
		file->rdwr_byte( tiles_overtaking, "o" );
		set_tiles_overtaking( tiles_overtaking );
	}

	// do not start with zero speed!
	current_speed ++;
}



bool stadtauto_t::ist_weg_frei(grund_t *gr)
{
	if(gr->gib_top()>200) {
		// already too many things here
		return false;
	}
	weg_t * str = gr->gib_weg(road_wt);
	if(str==NULL) {
		time_to_life = 0;
		return false;
	}

	// calculate new direction
	// are we just turning around?
	const uint8 this_fahrtrichtung = gib_fahrtrichtung();
	bool frei = false;
	if(gib_pos()==pos_next_next) {
		// turning around => single check
		const uint8 next_fahrtrichtung = ribi_t::rueckwaerts(this_fahrtrichtung);
		frei = (NULL == no_cars_blocking( gr, NULL, next_fahrtrichtung, next_fahrtrichtung, next_fahrtrichtung ));

	}
	else {
		// driving on: check for crossongs etc. too
		const uint8 next_fahrtrichtung = this->calc_richtung(gib_pos().gib_2d(), pos_next_next.gib_2d());

		// do not block this crossing (if possible)
		if(ribi_t::is_threeway(str->gib_ribi_unmasked())) {
			grund_t *test = welt->lookup(pos_next_next);
			if(test) {
				uint8 next_90fahrtrichtung = this->calc_richtung(pos_next.gib_2d(), pos_next_next.gib_2d());
				frei = (NULL == no_cars_blocking( gr, NULL, this_fahrtrichtung, next_fahrtrichtung, next_90fahrtrichtung ));
				if(frei) {
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
						// previous vehicle is stucked => end of traffic jam ...
						frei = false;
					}
					else {
						overtaker_t *over = dt->get_overtaker();
						if(over) {
							if(!over->is_overtaking()) {
								// otherwise the overtaken car would stop for us ...
								if(  dt->gib_typ()==ding_t::automobil  ) {
									convoi_t *cnv=static_cast<automobil_t *>(dt)->get_convoi();
									if(  cnv==NULL  ||  !can_overtake( cnv, cnv->gib_min_top_speed(), cnv->get_length()*16, diagonal_length)  ) {
										frei = false;
									}
								}
								else if(  dt->gib_typ()==ding_t::verkehr  ) {
									stadtauto_t *caut = static_cast<stadtauto_t *>(dt);
									if ( !can_overtake(caut, caut->gib_besch()->gib_geschw(), 256, diagonal_length) ) {
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
		if(frei  &&  str->is_crossing()) {
			// can we cross?
			crossing_t* cr = gr->find<crossing_t>(2);
			if(cr) {
				// approaching railway crossing: check if empty
				return cr->request_crossing( this );
			}
			// no further check, when already entered a crossing (to alloew leaving it)
			grund_t *gr_here = welt->lookup(gib_pos());
			if(gr_here  &&  gr_here->ist_uebergang()) {
				return true;
			}
			// ok, now check for free exit
			koord dir = pos_next.gib_2d()-gib_pos().gib_2d();
			koord3d checkpos = pos_next+dir;
			const uint8 nextnext_fahrtrichtung = ribi_typ(dir);
			while(1) {
				const grund_t *test = welt->lookup(checkpos);
				if(!test) {
					break;
				}
				// test next field after way crossing
				if(no_cars_blocking( test, NULL, next_fahrtrichtung, nextnext_fahrtrichtung, nextnext_fahrtrichtung )) {
					return false;
				}
				// ok, left the crossint
				if(!test->find<crossing_t>(2)) {
					// approaching railway crossing: check if empty
					crossing_t* cr = gr->find<crossing_t>(2);
					return cr->request_crossing( this );
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



void
stadtauto_t::betrete_feld()
{
	vehikel_basis_t::betrete_feld();
	welt->lookup( gib_pos() )->gib_weg(road_wt)->book(1, WAY_STAT_CONVOIS);
}



bool
stadtauto_t::hop_check()
{
	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *from = welt->lookup(pos_next);
	if(from==NULL) {
		// nothing to go? => destroy ...
		time_to_life = 0;
		return false;
	}

	// find the allowed directions
	const weg_t *weg = from->gib_weg(road_wt);
	if(weg==NULL) {
		// nothing to go? => destroy ...
		time_to_life = 0;
		return false;
	}

	// traffic light phase check (since this is on next tile, it will always be neccessary!)
	const ribi_t::ribi fahrtrichtung90 = ribi_typ(gib_pos().gib_2d(),pos_next.gib_2d());

	if(weg->has_sign()) {
		const roadsign_t* rs = from->find<roadsign_t>();
		const roadsign_besch_t* rs_besch = rs->gib_besch();
		if(rs_besch->is_traffic_light()  &&  (rs->get_dir()&fahrtrichtung90)==0) {
			fahrtrichtung = fahrtrichtung90;
			calc_bild();
			// wait here
			current_speed = 48;
			weg_next = 0;
			return false;
		}
	}

	// next tile unknow => find next tile
	if(pos_next_next==koord3d::invalid) {

		// ok, nobody did delete the road in front of us
		// so we can check for valid directions
		ribi_t::ribi ribi = weg->gib_ribi() & (~ribi_t::rueckwaerts(fahrtrichtung90));

		// cul de sac: return
		if(ribi==0) {
			pos_next_next = gib_pos();
			return ist_weg_frei(from);
		}

		const uint8 offset = ribi_t::ist_einfach(ribi) ? 0 : simrand(4);
		for(uint8 i = 0; i < 4; i++) {
			const uint8 r = (i+offset)&3;
			if(  (ribi&ribi_t::nsow[r])!=0  ) {
				grund_t *to;
				if(from->get_neighbour(to, road_wt, koord::nsow[r])) {
					// check, if this is just a single tile deep after a crossing
					weg_t *w=to->gib_weg(road_wt);
					if(ribi_t::ist_einfach(w->gib_ribi())  &&  (w->gib_ribi()&ribi_t::nsow[r])==0  &&  !ribi_t::ist_einfach(ribi)) {
						ribi &= ~ribi_t::nsow[r];
						continue;
					}
					// check, if roadsign forbid next step ...
					if(w->has_sign()) {
						const roadsign_besch_t* rs_besch = to->find<roadsign_t>()->gib_besch();
						if(rs_besch->gib_min_speed()>besch->gib_geschw()  ||  rs_besch->is_private_way()) {
							// not allowed to go here
							ribi &= ~ribi_t::nsow[r];
							continue;
						}
					}
					// ok, now check if we are allowed to go here (i.e. no cars blocking)
					pos_next_next = to->gib_pos();
					if(ist_weg_frei(from)) {
						// ok, this direction is fine!
						ms_traffic_jam = 0;
						if(current_speed<48) {
							current_speed = 48;
						}
						return true;
					}
					else {
						pos_next_next == koord3d::invalid;
					}
				}
				else {
					// not connected?!? => ribi likely wrong
					ribi &= ~ribi_t::nsow[r];
				}
			}
		}
		// only stumps at single way crossing, all other blocked => turn around
		if(ribi==0) {
			pos_next_next = gib_pos();
			return ist_weg_frei(from);
		}
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



void
stadtauto_t::hop()
{
	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *to = welt->lookup(pos_next);
	if(to==NULL) {
		time_to_life = 0;
		return;
	}
	verlasse_feld();
	if(pos_next_next==gib_pos()) {
		fahrtrichtung = calc_set_richtung( pos_next.gib_2d(), pos_next_next.gib_2d() );
		current_speed = 48;
		steps_next = 0;	// mark for starting at end of tile!
	}
	else {
		fahrtrichtung = calc_set_richtung( gib_pos().gib_2d(), pos_next_next.gib_2d() );
		calc_current_speed();
	}
	// and add to next tile
	setze_pos(pos_next);
	calc_bild();
	betrete_feld();
	update_tiles_overtaking();
	if(to->ist_uebergang()) {
		to->find<crossing_t>(2)->add_to_crossing(this);
	}
	pos_next = pos_next_next;
	pos_next_next = koord3d::invalid;
}



void
stadtauto_t::calc_bild()
{
	if(welt->lookup(gib_pos())->ist_im_tunnel()) {
		setze_bild( IMG_LEER);
	}
	else {
		setze_bild(besch->gib_bild_nr(ribi_t::gib_dir(gib_fahrtrichtung())));
	}
}



void
stadtauto_t::calc_current_speed()
{
	const weg_t * weg = welt->lookup(gib_pos())->gib_weg(road_wt);
	const uint16 max_speed = besch->gib_geschw();
	const uint16 speed_limit = weg ? kmh_to_speed(weg->gib_max_speed()) : max_speed;
	current_speed += max_speed>>2;
	if(current_speed > max_speed) {
		current_speed = max_speed;
	}
	if(current_speed > speed_limit) {
		current_speed = speed_limit;
	}
}


void
stadtauto_t::info(cbuffer_t & buf) const
{
	char str[256];
	sprintf(str, translator::translate("%s\nspeed %i\nmax_speed %i\ndx:%i dy:%i"), translator::translate(besch->gib_name()), current_speed, besch->gib_geschw(), dx, dy );
	buf.append(str);
}



// to make smaller steps than the tile granularity, we have to use this trick
void stadtauto_t::get_screen_offset( int &xoff, int &yoff ) const
{
	vehikel_basis_t::get_screen_offset( xoff, yoff );

	// eventually shift position to take care of overtaking
	const int raster_width = get_tile_raster_width();
	if(  is_overtaking()  ) {
		xoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::gib_dir(gib_fahrtrichtung())][0], raster_width);
		yoff += tile_raster_scale_x(overtaking_base_offsets[ribi_t::gib_dir(gib_fahrtrichtung())][1], raster_width);
	}
	else if(  is_overtaken()  ) {
		xoff -= tile_raster_scale_x(overtaking_base_offsets[ribi_t::gib_dir(gib_fahrtrichtung())][0], raster_width)/5;
		yoff -= tile_raster_scale_x(overtaking_base_offsets[ribi_t::gib_dir(gib_fahrtrichtung())][1], raster_width)/5;
	}
}



/**
 * conditions for a city car to overtake another overtaker.
 * The city car is not overtaking/being overtaken.
 * @author isidoro
 */
bool stadtauto_t::can_overtake(overtaker_t *other_overtaker, int other_speed, int steps_other, int diagonal_length)
{
	if(!other_overtaker->can_be_overtaken()) {
		return false;
	}

	sint32 diff_speed = (sint32)current_speed - other_speed;
	if(  diff_speed < kmh_to_speed(5)  ) {
		return false;
	}

	// Number of tiles overtaking will take
	int n_tiles = 0;

	/* Distance it takes overtaking (unit:256*tile) = my_speed * time_overtaking
	 * time_overtaking = tiles_to_overtake/diff_speed
	 * tiles_to_overtake = convoi_length + pos_other_convoi
	 * convoi_length for city cars? ==> a bit over half a tile (10)
	 */
	sint32 distance = current_speed*((10<<4)+steps_other)/max(besch->gib_geschw()-other_speed,diff_speed);
	sint32 time_overtaking = 0;

	// Conditions for overtaking:
	// Flat tiles, with no stops, no crossings, no signs, no change of road speed limit
	koord3d check_pos = gib_pos();
	koord pos_prev = check_pos.gib_2d();

	grund_t *gr = welt->lookup(check_pos);
	if(  gr==NULL  ) {
		return false;
	}

	weg_t *str = gr->gib_weg(road_wt);
	if(  str==0  ) {
		return false;
	}

	// we need 90 degree ribi
	ribi_t::ribi direction = gib_fahrtrichtung();
	direction = str->gib_ribi() & direction;

	while(  distance > 0  ) {

		// we allow stops and slopes, since emtpy stops and slopes cannot affect us
		// (citycars do not slow down on slopes!)

		// start of bridge is one level deeper
		if(gr->gib_weg_yoff()>0)  {
			check_pos.z += Z_TILE_STEP;
		}

		// special signs
		if(  str->has_sign()  &&  str->gib_ribi()==str->gib_ribi_unmasked()  ) {
			const roadsign_t *rs = gr->find<roadsign_t>(1);
			if(rs) {
				const roadsign_besch_t *rb = rs->gib_besch();
				if(rb->gib_min_speed()>besch->gib_geschw()  ||  rb->is_private_way()  ||  rb->is_traffic_light()  ) {
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
		if(  besch->gib_geschw() > kmh_to_speed(str->gib_max_speed())  ) {
			return false;
		}

		int d = ribi_t::ist_gerade(str->gib_ribi()) ? 256 : diagonal_length;
		distance -= d;
		time_overtaking += d;

		n_tiles++;

		/* Now we must check for next position:
		 * crossings are ok, as long as we cannot exit there due to one way signs
		 * much cheeper calculation: only go on in the direction of before (since no slopes allowed anyway ... )
		 */
		grund_t *to = welt->lookup( check_pos + koord((ribi_t::ribi)(str->gib_ribi()&direction)) );
		if(  ribi_t::is_threeway(str->gib_ribi())  ||  to==NULL) {
			// check for entries/exits/bridges, if neccessary
			ribi_t::ribi rib = str->gib_ribi();
			bool found_one = false;
			for(  int r=0;  r<4;  r++  ) {
				if(  (rib&ribi_t::nsow[r])==0  ||  check_pos.gib_2d()+koord::nsow[r]==pos_prev) {
					continue;
				}
				if(gr->get_neighbour(to, road_wt, koord::nsow[r])) {
					if(found_one) {
						// two directions to go: unexpected cars may occurs => abort
						return false;
					}
					found_one = true;
				}
			}
		}
		pos_prev = check_pos.gib_2d();

		// nowhere to go => nobody can come against us ...
		if(to==NULL  ||  (str=to->gib_weg(road_wt))==NULL) {
			return false;
		}

		// Check for other vehicles on the next tile
		const uint8 top = gr->gib_top();
		for(  uint8 j=1;  j<top;  j++ ) {
			vehikel_basis_t *v = (vehikel_basis_t *)gr->obj_bei(j);
			if(v->is_moving()) {
				// check for other traffic on the road
				const overtaker_t *ov = v->get_overtaker();
				if(ov) {
					if(this!=ov  &&  other_overtaker!=ov) {
						return false;
					}
				}
				else if(  v->gib_waytype()==road_wt  &&  v->gib_typ()!=ding_t::fussgaenger  ) {
					return false;
				}
			}
		}

		gr = to;
		check_pos = to->gib_pos();

		direction = ~ribi_typ( check_pos.gib_2d(),pos_prev ) & str->gib_ribi();
	}

	// Second phase: only facing traffic is forbidden
	//   Since street speed can change, we do the calculation with time.
	//   Each empty tile will substract tile_dimension/max_street_speed.
	//   If time is exhausted, we are guaranteed that no facing traffic will
	//   invade the dangerous zone.
	time_overtaking = (time_overtaking << 16)/(sint32)current_speed;
	do {
		// we can allow crossings or traffic lights here, since they will stop also oncoming traffic

		time_overtaking -= (ribi_t::ist_gerade(str->gib_ribi()) ? 256<<16 : diagonal_length<<16)/kmh_to_speed(str->gib_max_speed());

		// start of bridge is one level deeper
		if(gr->gib_weg_yoff()>0)  {
			check_pos.z += Z_TILE_STEP;
		}

		// much cheeper calculation: only go on in the direction of before ...
		grund_t *to = welt->lookup( check_pos + koord((ribi_t::ribi)(str->gib_ribi()&direction)) );
		if(  ribi_t::is_threeway(str->gib_ribi())  ||  to==NULL  ) {
			// check for crossings/bridges, if neccessary
			bool found_one = false;
			for(  int r=0;  r<4;  r++  ) {
				if(check_pos.gib_2d()+koord::nsow[r]==pos_prev) {
					continue;
				}
				if(gr->get_neighbour(to, road_wt, koord::nsow[r])) {
					if(found_one) {
						return false;
					}
					found_one = true;
				}
			}
		}

		koord pos_prev_prev = pos_prev;
		pos_prev = check_pos.gib_2d();

		// nowhere to go => nobody can come against us ...
		if(to==NULL  ||  (str=to->gib_weg(road_wt))==NULL) {
			break;
		}

		// Check for other vehicles in facing directiona
		// now only I know direction on this tile ...
		ribi_t::ribi their_direction = ribi_t::rueckwaerts(calc_richtung( pos_prev_prev, to->gib_pos().gib_2d() ));
		const uint8 top = gr->gib_top();
		for(  uint8 j=1;  j<top;  j++ ) {
			vehikel_basis_t *v = (vehikel_basis_t *)gr->obj_bei(j);
			if(v->is_moving()  &&  v->gib_fahrtrichtung()==their_direction) {
				// check for car
				if(v->get_overtaker()) {
					return false;
				}
			}
		}

		gr = to;
		check_pos = to->gib_pos();

		direction = ~ribi_typ( check_pos.gib_2d(),pos_prev ) & str->gib_ribi();
	} while( time_overtaking > 0 );

	set_tiles_overtaking( 1+n_tiles );
	other_overtaker->set_tiles_overtaking( -1-(n_tiles/2) );

	return true;
}



