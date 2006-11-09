/**
 * simverkehr.cc
 *
 * Bewegliche Objekte fuer Simutrans.
 * Transportfahrzeuge sind in simvehikel.h definiert, da sie sich
 * stark von den hier definierten Fahrzeugen fuer den Individualverkehr
 * unterscheiden.
 *
 * Hj. Malthaner
 *
 * April 2000
 */

#include "simdebug.h"
#include "simmesg.h"
#include "simworld.h"
#include "simverkehr.h"
#include "simtools.h"
#include "simmem.h"
#include "simimg.h"
#include "simconst.h"

#ifdef DESTINATION_CITYCARS
// for final citcar destinations
#include "simpeople.h"
#endif

#include "tpl/slist_mit_gewichten_tpl.h"

#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "dataobj/umgebung.h"

#include "dings/roadsign.h"

#include "boden/grund.h"
#include "boden/wege/weg.h"

#include "besch/stadtauto_besch.h"
#include "besch/roadsign_besch.h"


/**********************************************************************************************************************/
/* Verkehrsteilnehmer (basis class) from here on */


void
verkehrsteilnehmer_t::calc_current_speed()
{
	const weg_t * weg = welt->lookup(gib_pos())->gib_weg(road_wt);
	const int speed_limit = weg ? kmh_to_speed(weg->gib_max_speed()) : max_speed;
	current_speed += max_speed>>2;
	if(current_speed > max_speed) {
		current_speed = max_speed;
	}
	if(current_speed > speed_limit) {
		current_speed = speed_limit;
	}
}


verkehrsteilnehmer_t::verkehrsteilnehmer_t(karte_t *welt) :
   vehikel_basis_t(welt)
{
	setze_besitzer( welt->gib_spieler(1) );
	max_speed = 0;
	current_speed = 1;
	step_frequency = 0;
	time_to_life = -1;
}


/**
 * Ensures that this object is removed correctly from the list
 * of sync steppable things!
 * @author Hj. Malthaner
 */
verkehrsteilnehmer_t::~verkehrsteilnehmer_t()
{
	// just to be sure we are removed from this list!
	step_frequency = 0;
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

    max_speed = kmh_to_speed(80);
    current_speed = 50;
	current_speed = 1;
	step_frequency = 0;

    weg_next = simrand(1024);
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


char *
verkehrsteilnehmer_t::info(char *buf) const
{
	*buf = 0;
	return buf;
}


void
verkehrsteilnehmer_t::hop()
{
	// will ignore roadigns
	if(pos_next.z == -1) {
		// Altes Savegame geladen
		pos_next = welt->lookup(pos_next.gib_2d())->gib_kartenboden()->gib_pos();
	}
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
		fahrtrichtung = calc_richtung(gib_pos().gib_2d(), pos_next.gib_2d(), dx, dy);
	} else if(count==1) {
		pos_next = liste[0]->gib_pos();
		fahrtrichtung = calc_richtung(gib_pos().gib_2d(), pos_next.gib_2d(), dx, dy);
	}
	else {
		fahrtrichtung = gegenrichtung;
		current_speed = 1;
		dx = -dx;
		dy = -dy;
		pos_next = gib_pos();
	}

	verlasse_feld();
	setze_pos(from->gib_pos());
	calc_current_speed();
	calc_bild();
	betrete_feld();
}



bool
verkehrsteilnehmer_t::sync_step(long delta_t)
{
	if(time_to_life<0) {
		// remove obj
		step_frequency = 0;
//DBG_MESSAGE("verkehrsteilnehmer_t::sync_step()","stopped");
  		return false;
	}

	if(step_frequency>0) {
		// step will check for traffic jams
		return true;
	}
	time_to_life -= delta_t;
	weg_next += (current_speed*delta_t) / 64;

	// Funktionsaufruf vermeiden, wenn möglich
	// if ist schneller als Aufruf!
	if(hoff) {
		setze_yoff( gib_yoff() - hoff );
	}

	while(1024 < weg_next) {
		weg_next -= 1024;
		fahre();
	}

	hoff = calc_height();

	// Funktionsaufruf vermeiden, wenn möglich
	// if ist schneller als Aufruf!
	if(hoff) {
		setze_yoff( gib_yoff() + hoff );
	}

	return true;
}



void verkehrsteilnehmer_t::rdwr(loadsave_t *file)
{
	step_frequency = 0;
	vehikel_basis_t::rdwr(file);

	if(file->get_version() < 86006) {
		long l;
		file->rdwr_long(max_speed, "\n");
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
		file->rdwr_long(max_speed, "\n");
		file->rdwr_long(weg_next, "\n");
		file->rdwr_byte(dx, " ");
		file->rdwr_byte(dy, "\n");
		file->rdwr_enum(fahrtrichtung, " ");
		sint16 dummy16=hoff;
		file->rdwr_short(dummy16, "\n");
		hoff = (sint8)dummy16;
	}
	pos_next.rdwr(file);

	// the lifetime in ms
	if(file->get_version()>89004) {
		file->rdwr_long( time_to_life, "t" );
	}

	// Hajo: avoid endless growth of the values
	// this causes lockups near 2**32
	weg_next &= 1023;

	if(file->is_loading()) {
		// Hajo: pre-speed limit game had city cars with max speed of
		// 60 km/h, since city raods now have a speed limit of 50 km/h
		// we can use 80 for the cars so that they speed up on
		// intercity roads
		if(max_speed==kmh_to_speed(60)) {
			max_speed = kmh_to_speed(80);
		}
	}
}



/**********************************************************************************************************************/
/* statsauto_t (city cars) from here on */


slist_mit_gewichten_tpl<const stadtauto_besch_t *> stadtauto_t::liste_timeline;
slist_mit_gewichten_tpl<const stadtauto_besch_t *> stadtauto_t::liste;
stringhashtable_tpl<const stadtauto_besch_t *> stadtauto_t::table;

void stadtauto_t::built_timeline_liste(karte_t *welt)
{
	if(liste.count()>0) {
		// this list will contain all citycars
		liste_timeline.clear();
		const int month_now = welt->get_current_month();

//DBG_DEBUG("stadtauto_t::built_timeline_liste()","year=%i, month=%i", month_now/12, month_now%12+1);

		// check for every citycar, if still ok ...
		slist_iterator_tpl <const stadtauto_besch_t *> iter (liste);
		while(iter.next()) {
			const stadtauto_besch_t *info = iter.get_current();
			const int intro_month = info->get_intro_year_month();
			const int retire_month = info->get_retire_year_month();

//DBG_DEBUG("stadtauto_t::built_timeline_liste()","iyear=%i, imonth=%i", intro_month/12, intro_month%12+1);
//DBG_DEBUG("stadtauto_t::built_timeline_liste()","ryear=%i, rmonth=%i", retire_month/12, retire_month%12+1);

			if(	welt->use_timeline() == false || (intro_month<=month_now  &&  retire_month>month_now)  ) {
				liste_timeline.append(info);
//DBG_DEBUG("stadtauto_t::built_timeline_liste()","adding %s to liste",info->gib_name());
			}
		}
	}
}



bool stadtauto_t::register_besch(const stadtauto_besch_t *besch)
{
	liste.append(besch);
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
	if(liste.count() == 0) {
		DBG_MESSAGE("stadtauto_t", "No citycars found - feature disabled");
	}
	return true;
}

int stadtauto_t::gib_anzahl_besch()
{
    return liste_timeline.count();
}


stadtauto_t::stadtauto_t(karte_t *welt, loadsave_t *file)
 : verkehrsteilnehmer_t(welt)
{
	rdwr(file);
	step_frequency = 0;
	welt->sync_add(this);
}

#ifdef DESTINATION_CITYCARS
stadtauto_t::stadtauto_t(karte_t *welt, koord3d pos, koord target)
#else
stadtauto_t::stadtauto_t(karte_t *welt, koord3d pos, koord )
#endif
 : verkehrsteilnehmer_t(welt, pos)
{
	besch = liste_timeline.gib_gewichted(simrand(liste_timeline.gib_gesamtgewicht()));
	if(!besch) {
		besch = liste.gib_gewichted(simrand(liste.gib_gesamtgewicht()));
	}
	time_to_life = umgebung_t::stadtauto_duration;
	current_speed = 48;
	step_frequency = 0;
#ifdef DESTINATION_CITYCARS
	this->target = target;
#endif
	setze_max_speed( besch->gib_geschw() );
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

		if(besch == 0 && liste_timeline.count() > 0) {
			dbg->warning("stadtauto_t::rdwr()", "Object '%s' not found in table, trying random stadtauto object type",s);
			besch = liste_timeline.gib_gewichted(simrand(liste_timeline.gib_gesamtgewicht()));
		}
		if(besch == 0 && liste.count() > 0) {
			dbg->warning("stadtauto_t::rdwr()", "Object '%s' not found in table, trying random stadtauto object type",s);
			besch = liste.gib_gewichted(simrand(liste.gib_gesamtgewicht()));
		}
		guarded_free(const_cast<char *>(s));

		if(besch == 0) {
			dbg->fatal("stadtauto_t::rdwr()", "loading game with private cars, but no private car objects found in PAK files.");
		}
		setze_max_speed( besch->gib_geschw() );
		setze_bild(0,besch->gib_bild_nr(ribi_t::gib_dir(gib_fahrtrichtung())));
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
		file->rdwr_long(current_speed, "\n");
	}
	// do not start with zero speed!
	current_speed ++;
}


bool
stadtauto_t::ist_weg_frei()
{
	// no change, or invalid
	if(pos_next==gib_pos()) {
		return true;
	}

	const grund_t *gr = welt->lookup(pos_next);
	if(gr==NULL  ||  gr->obj_count()>200) {
		return false;
	}

	weg_t *str=gr->gib_weg(road_wt);
	if(!str) {
		return false;
	}


	if(gr->hat_weg(track_wt)) {
		// railway crossing
		if(gr->suche_obj_ab(ding_t::waggon,PRI_RAIL_AND_ROAD)) {
			return false;
		}
	}

	// check for traffic lights
	if(str->has_sign()) {
		roadsign_t *rs = (roadsign_t *)gr->suche_obj(ding_t::roadsign);
		const int richtung = ribi_typ(gib_pos().gib_2d(),pos_next.gib_2d());
		if(rs->gib_besch()->is_traffic_light()  &&  (rs->get_dir()&richtung)==0) {
			fahrtrichtung = richtung;
			calc_bild();
			// wait here
			return false;
		}
	}

	// calculate new direction
	sint8 dx, dy;	// dummies
	// are we just turning around?
	//	const uint8 this_fahrtrichtung = (gib_pos()==pos_next) ? gib_fahrtrichtung() :  ribi_t::rueckwaerts(this_fahrtrichtung);
	const uint8 this_fahrtrichtung = gib_fahrtrichtung();
	// next direction
	const uint8 next_fahrtrichtung = (gib_pos()==pos_next) ? this_fahrtrichtung : this->calc_richtung(gib_pos().gib_2d(), pos_next.gib_2d(), dx, dy);
	bool frei = true;

	// suche vehikel
	const uint8 top = gr->gib_top();
	for(  uint8 pos=0;  pos<top  && frei;  pos++ ) {
		ding_t *dt = gr->obj_bei(pos);
		if(dt) {
			uint8 other_fahrtrichtung=255;

			// check for car
			if(dt->gib_typ()==ding_t::automobil) {
				automobil_t *at = (automobil_t *)dt;
				other_fahrtrichtung = at->gib_fahrtrichtung();
			}

			// check for city car
			if(dt->gib_typ()==ding_t::verkehr) {
				vehikel_basis_t *v = (vehikel_basis_t *)dt;
				other_fahrtrichtung = v->gib_fahrtrichtung();
			}

			// ok, there is another car ...
			if(other_fahrtrichtung!=255) {

				if(other_fahrtrichtung==next_fahrtrichtung  ||  other_fahrtrichtung==this_fahrtrichtung ) {
					// this goes into the same direction as we, so stopp and save a restart speed
					frei = false;
				} else if(ribi_t::ist_orthogonal(other_fahrtrichtung,this_fahrtrichtung )) {
					// there is a car orthogonal to us
					frei = false;
				}
			}
		}
	}

	return frei;
}



void
stadtauto_t::betrete_feld()
{
#ifdef DESTINATION_CITYCARS
	if(target!=koord::invalid  &&  abs_distance(pos_next.gib_2d(),target)<10) {
		// delete it ...
		time_to_life = 0;

		fussgaenger_t *fg = new fussgaenger_t(welt, pos_next);
		bool ok = welt->lookup(pos_next)->obj_add(fg) != 0;
		for(int i=0; i<(fussgaenger_t::count & 3); i++) {
			fg->sync_step(64*24);
		}
		welt->sync_add( fg );
	}
#endif
	vehikel_basis_t::betrete_feld();

	welt->lookup( gib_pos() )->gib_weg(road_wt)->book(1, WAY_STAT_CONVOIS);
}



bool
stadtauto_t::step(long delta_t)
{
	// if we get here, we are stucked somewhere
	// since this test takes some time, we do not do it too often
	if(ist_weg_frei()) {
		// drive on
		step_frequency = 0;
		current_speed = 48;
	}
	else {
		// still stucked
		ms_traffic_jam -= delta_t;
		if(ms_traffic_jam<0) {
			// try to turn around
			fahrtrichtung = ribi_t::rueckwaerts( fahrtrichtung );
			koord3d old_pos_next = pos_next;
			pos_next = gib_pos();
			setze_pos( pos_next+koord(-1,0) );
			if(ist_weg_frei()) {
				// drive on
				setze_pos( pos_next );
				step_frequency = 0;
				current_speed = 48;
			}
			else {
				// old direction again and report traffic jam
				fahrtrichtung = ribi_t::rueckwaerts( fahrtrichtung );
				setze_pos( pos_next );
				pos_next = old_pos_next;
				ms_traffic_jam = (3<<karte_t::ticks_bits_per_tag);
				koord about_pos = gib_pos().gib_2d();
				message_t::get_instance()->add_message(
					translator::translate("To heavy traffic\nresults in traffic jam.\n"),
					koord(about_pos.x&0xFFF4,about_pos.y&0xFFF4) ,message_t::problems,COL_ORANGE );
				// still stucked ...
			}
		}
	}
	return true;
}



void
stadtauto_t::hop()
{
	if(pos_next.z == -1) {
		// Altes Savegame geladen
		pos_next = welt->lookup(pos_next.gib_2d())->gib_kartenboden()->gib_pos();
	}
	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *from = welt->lookup(pos_next);
	grund_t *to;

	static weighted_vector_tpl<grund_t *> liste(4);
	liste.clear();

	// 1) find the allowed directions
	const weg_t *weg = from->gib_weg(road_wt);
	if(weg==NULL) {
		// no gound here any more?
		pos_next = gib_pos();
		return;
	}

	ribi_t::ribi ribi = weg->gib_ribi() & ribi_t::gib_forward(fahrtrichtung);
	if(ribi_t::ist_einfach(ribi)  &&  from->get_neighbour(to, road_wt, koord(ribi))) {
		// we should add here
		bool add=true;
		// check, if roadsign forbid next step ...
		if(to->gib_weg(road_wt)->has_sign()) {
			const roadsign_besch_t *rs_besch = ((roadsign_t *)to->suche_obj(ding_t::roadsign))->gib_besch();
			add = (rs_besch->is_traffic_light()  ||  rs_besch->gib_min_speed()<=gib_max_speed())  &&  !rs_besch->is_private_way();
		}
		if(add) {
			pos_next = to->gib_pos();
			fahrtrichtung = calc_richtung(gib_pos().gib_2d(), pos_next.gib_2d(), dx, dy);
		}
		else {
			// turn around
			fahrtrichtung = ribi_t::rueckwaerts(fahrtrichtung);
			pos_next = gib_pos();
			current_speed = 1;
			dx = -dx;
			dy = -dy;
		}
	}
	else if(ribi==ribi_t::keine) {
		fahrtrichtung = ribi_t::rueckwaerts(fahrtrichtung);
		pos_next = gib_pos();
		current_speed = 1;
		dx = -dx;
		dy = -dy;
	}
	else {
		// add all good ribis here
		for(int r = 0; r < 4; r++) {
			if(  (ribi&ribi_t::nsow[r])!=0  &&  from->get_neighbour(to, road_wt, koord::nsow[r]) ) {
				// check, if this is just a single tile deep
				weg_t *w=to->gib_weg(road_wt);
				int next_ribi =  w->gib_ribi();
				if((ribi&next_ribi)!=0  ||  !ribi_t::ist_einfach(next_ribi)) {
					bool add=true;
					// check, if roadsign forbid next step ...
					if(w->has_sign()) {
						const roadsign_besch_t *rs_besch = ((roadsign_t *)to->suche_obj(ding_t::roadsign))->gib_besch();
						add = (rs_besch->is_traffic_light()  ||  rs_besch->gib_min_speed()<=gib_max_speed())  &&  !rs_besch->is_private_way();
//DBG_MESSAGE("stadtauto_t::hop()","roadsign");
					}
					// ok;
					if(add) {
#ifdef DESTINATION_CITYCARS
						unsigned long dist=abs_distance( to->gib_pos().gib_2d(), target );
						liste.append( to, dist*dist );
#else
						liste.append( to, 1 );
#endif
					}
				}
			}
		}

		if(liste.get_count()>1) {
#ifdef DESTINATION_CITYCARS
			if(target!=koord::invalid) {
				pos_next = liste.at_weight(simrand(liste.get_sum_weight()))->gib_pos();
			}
			else
#endif
			{
				pos_next = liste.get(simrand(liste.get_count()))->gib_pos();
			}
			fahrtrichtung = calc_richtung(gib_pos().gib_2d(), pos_next.gib_2d(), dx, dy);
		} else if(liste.get_count()==1) {
			pos_next = liste.get(0)->gib_pos();
			fahrtrichtung = calc_richtung(gib_pos().gib_2d(), pos_next.gib_2d(), dx, dy);
		}
		else {
			fahrtrichtung = ribi_t::rueckwaerts(fahrtrichtung);
			current_speed = 1;
			dx = -dx;
			dy = -dy;
			pos_next = gib_pos();
		}
	}

	verlasse_feld();
	setze_pos(from->gib_pos());
	calc_current_speed();
	calc_bild();
	betrete_feld();
}



bool
stadtauto_t::hop_check()
{
	bool frei = ist_weg_frei();
	if(!frei) {
		ms_traffic_jam = (3<<karte_t::ticks_bits_per_tag);
		step_frequency = 1;
	}
	return frei;
}



void
stadtauto_t::calc_bild()
{
	if(welt->lookup(gib_pos())->ist_im_tunnel()) {
		setze_bild(0, IMG_LEER);
	}
	else {
		setze_bild(0,besch->gib_bild_nr(ribi_t::gib_dir(gib_fahrtrichtung())));
	}
}
