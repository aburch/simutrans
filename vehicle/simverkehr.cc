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
	// first: release crossing
	grund_t *gr = welt->lookup(gib_pos());
	if(gr  &&  gr->ist_uebergang()) {
		gr->find<crossing_t>(2)->release_crossing(this);
	}

	// just to be sure we are removed from this list!
	if(time_to_life>=0) {
		welt->sync_remove(this);
	}
	mark_image_dirty( gib_bild(), 0 );
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
		file->rdwr_byte(dx, " ");
		file->rdwr_byte(dy, "\n");
		file->rdwr_enum(fahrtrichtung, " ");
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

		display_set_image_offset( besch->gib_bild_nr(0), +XOFF, +YOFF );
		display_set_image_offset( besch->gib_bild_nr(1), -XOFF, +YOFF );
		display_set_image_offset( besch->gib_bild_nr(2), 0, +YOFF );
		display_set_image_offset( besch->gib_bild_nr(3), +XOFF, 0 );
		display_set_image_offset( besch->gib_bild_nr(4), -XOFF, -YOFF );
		display_set_image_offset( besch->gib_bild_nr(5), +XOFF, -YOFF );
		display_set_image_offset( besch->gib_bild_nr(6), 0, -YOFF );
		display_set_image_offset( besch->gib_bild_nr(7), -XOFF-YOFF, 0 );
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


stadtauto_t::stadtauto_t(karte_t *welt, loadsave_t *file)
 : verkehrsteilnehmer_t(welt)
{
	rdwr(file);
	ms_traffic_jam = 0;
	if(besch) {
		welt->sync_add(this);
	}
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
	time_to_life = umgebung_t::stadtauto_duration << welt->ticks_bits_per_tag;
	current_speed = 48;
	ms_traffic_jam = 0;
#ifdef DESTINATION_CITYCARS
	this->target = target;
#endif
	calc_bild();
}




bool
stadtauto_t::sync_step(long delta_t)
{
	if(time_to_life<0) {
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
					message_t::get_instance()->add_message( translator::translate("To heavy traffic\nresults in traffic jam.\n"), gib_pos().gib_2d(), message_t::warnings, COL_ORANGE );
				}
			}
		}
	}
	else {
		setze_yoff( gib_yoff() - hoff );
		weg_next += current_speed*delta_t;
		while(SPEED_STEP_WIDTH < weg_next) {
			weg_next -= SPEED_STEP_WIDTH;
			fahre_basis();
		}
		if(use_calc_height) {
			hoff = calc_height();
		}
		setze_yoff( gib_yoff() + hoff );
	}

	return true;
}



void stadtauto_t::rdwr(loadsave_t *file)
{
	verkehrsteilnehmer_t::rdwr(file);

	const char *s = NULL;
	if(file->is_saving()) {
		s = besch->gib_name();
	}

	file->rdwr_str(s, "N");
	if(file->is_loading()) {
		besch = table.get(s);

		if (besch == 0 && !liste_timeline.empty()) {
			dbg->warning("stadtauto_t::rdwr()", "Object '%s' not found in table, trying random stadtauto object type",s);
			besch = liste_timeline.at_weight(simrand(liste_timeline.get_sum_weight()));
		}
		if (besch == 0 && !liste.empty()) {
			dbg->warning("stadtauto_t::rdwr()", "Object '%s' not found in table, trying random stadtauto object type",s);
			besch = liste.at_weight(simrand(liste.get_sum_weight()));
		}
		guarded_free(const_cast<char *>(s));

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

	// do not start with zero speed!
	current_speed ++;
}



bool
stadtauto_t::ist_weg_frei(grund_t *gr)
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
			frei = (NULL == no_cars_blocking( gr, NULL, this_fahrtrichtung, next_fahrtrichtung, next_fahrtrichtung ));
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
			return false;
		}
	}

	// next tile unknow => find next tile
	if(pos_next_next==koord3d::invalid) {

		grund_t *to;

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
//					weg->setze_ribi( weg->gib_ribi_unmasked()&(~ribi_t::nsow[r]) );
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
		fahrtrichtung = ribi_t::rueckwaerts(fahrtrichtung);
		dx = -dx;
		dy = -dy;
		current_speed = 16;
	}
	else {
		fahrtrichtung = calc_set_richtung( gib_pos().gib_2d(), pos_next_next.gib_2d() );
		calc_current_speed();
	}
	// and add to next tile
	setze_pos(pos_next);
	calc_bild();
	betrete_feld();
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
	sprintf(str, translator::translate("%s\nspeed %i\nmax_speed %i\ndx:%i dy:%i"), besch->gib_name(), current_speed, besch->gib_geschw(), dx, dy );
	buf.append(str);
}
