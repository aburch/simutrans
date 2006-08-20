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
#include "boden/grund.h"
#include "boden/wege/weg.h"
#include "simworld.h"
#include "simverkehr.h"
#include "simtools.h"
#include "simmem.h"
#include "tpl/slist_mit_gewichten_tpl.h"

#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "dataobj/umgebung.h"

#include "dings/roadsign.h"

#include "besch/stadtauto_besch.h"
#include "besch/roadsign_besch.h"

#include "simimg.h"

slist_mit_gewichten_tpl<const stadtauto_besch_t *> stadtauto_t::liste_timeline;
slist_mit_gewichten_tpl<const stadtauto_besch_t *> stadtauto_t::liste;
stringhashtable_tpl<const stadtauto_besch_t *> stadtauto_t::table;

void stadtauto_t::built_timeline_liste()
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
			const int intro_month = info->get_intro_year() * 12 + info->get_intro_month();
			const int retire_month = info->get_retire_year() * 12 + info->get_retire_month();

//DBG_DEBUG("stadtauto_t::built_timeline_liste()","iyear=%i, imonth=%i", intro_month/12, intro_month%12+1);
//DBG_DEBUG("stadtauto_t::built_timeline_liste()","ryear=%i, rmonth=%i", retire_month/12, retire_month%12+1);

			if(	umgebung_t::use_timeline == false || (intro_month<=month_now  &&  retire_month>month_now)  ) {
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

stadtauto_t::stadtauto_t(karte_t *welt, koord3d pos)
 : verkehrsteilnehmer_t(welt, pos)
{
	besch = liste_timeline.gib_gewichted(simrand(liste_timeline.gib_gesamtgewicht()));
	time_to_life = umgebung_t::stadtauto_duration;
	current_speed = 48;
	step_frequency = 0;
	setze_max_speed( besch->gib_geschw() );
}


/**
 * Ensures that this object is removed correctly from the list
 * of sync steppable things!
 * @author Hj. Malthaner
 */
stadtauto_t::~stadtauto_t()
{
	// just to be sure we are removed from this list!
	step_frequency = 0;
	if(time_to_life>0) {
		welt->sync_remove(this);
	}
DBG_MESSAGE("stadtauto_t::~stadtauto_t()","called");
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
			dbg->warning("stadtauto_t::rdwr()", "Object '%s' not found in table, trying first stadtauto object type",s);
			besch = liste_timeline.gib_gewichted(simrand(liste_timeline.gib_gesamtgewicht()));
		}
		guarded_free(const_cast<char *>(s));

		if(besch == 0) {
			dbg->fatal("stadtauto_t::rdwr()", "loading game with private cars, but no private car objects found in PAK files.");
		}
		setze_max_speed( besch->gib_geschw() );
		setze_bild(0,besch->gib_bild_nr(ribi_t::gib_dir(gib_fahrtrichtung())));
	}

	if(file->get_version() <= 86001) {
		time_to_life = simrand(100000);
	}
	else {
		file->rdwr_long(time_to_life, "\n");
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
	if(gr==NULL) {
		return false;
	}

	if(gr->obj_count()>200) {
		// too many cars here
		return false;
	}

	// pruefe auf Schienenkreuzung
	if(gr->gib_weg(weg_t::schiene) && gr->gib_weg(weg_t::strasse)) {
		// das ist eine Kreuzung, ist sie frei ?

		if(gr->suche_obj_ab(ding_t::waggon,PRI_RAIL_AND_ROAD)) {
			return false;
		}
	}

	// check for traffic lights
	ding_t *dt = gr->obj_bei(0);
	if(dt  &&  dt->gib_typ()==ding_t::roadsign) {
		const roadsign_t *rs = (roadsign_t *)dt;
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
	// this will automatically give the right order
	grund_t *gr = welt->lookup( gib_pos() );

	uint8 offset;
	if(gr->gib_weg(weg_t::schiene)) {
		offset = (gib_fahrtrichtung()<4)^(umgebung_t::drive_on_left==false) ? PRI_ROAD_S_W_SW_SE : PRI_ROAD_AND_RAIL_N_E_NE_NW;
	}
	else {
		offset = (gib_fahrtrichtung()<4)^(umgebung_t::drive_on_left==false) ? PRI_ROAD_S_W_SW_SE : PRI_ROAD_N_E_NE_NW;
	}
	bool ok = (gr->obj_pri_add(this, offset) != 0);

	if(!ok) {
		dbg->error("stadtauto_t::betrete_feld()","vehicel '%s' %p could not be added to %d, %d, %d",gib_pos().x, gib_pos().y, gib_pos().z);
	}
}



bool
stadtauto_t::step(long /*delta_t*/)
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
		if(ms_traffic_jam<welt->gib_zeit_ms()) {
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
				ms_traffic_jam = welt->gib_zeit_ms()+(3<<karte_t::ticks_bits_per_tag);
				koord about_pos = gib_pos().gib_2d();
				message_t::get_instance()->add_message(
					translator::translate("To heavy traffic\nresults in traffic jam.\n"),
					koord(about_pos.x&0xFFF4,about_pos.y&0xFFF4) ,message_t::problems,ORANGE );
				// still stucked ...
			}
		}
	}
	return true;
}



void stadtauto_t::calc_bild()
{
	if(welt->lookup(gib_pos())->ist_im_tunnel()) {
		setze_bild(0, -1);
	} else {
		setze_bild(0,besch->gib_bild_nr(ribi_t::gib_dir(gib_fahrtrichtung())));
	}
}



void
verkehrsteilnehmer_t::calc_current_speed()
{
	const weg_t * weg = welt->lookup(gib_pos())->gib_weg(weg_t::strasse);
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
}


verkehrsteilnehmer_t::verkehrsteilnehmer_t(karte_t *welt, koord3d pos) :
   vehikel_basis_t(welt, pos)
{
    // V.Meyer: weg_position_t changed to grund_t::get_neighbour()
    grund_t *from = welt->lookup(pos);
    grund_t *to;

    // int ribi = from->gib_weg_ribi(weg_t::strasse);
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
	if(from->get_neighbour(to, weg_t::strasse, koord::nsow[r])) {
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
	from->get_neighbour(to, weg_t::strasse, fahrtrichtung);
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


bool
stadtauto_t::hop_check()
{
	bool frei = ist_weg_frei();
	if(!frei) {
		ms_traffic_jam = welt->gib_zeit_ms() + (3<<karte_t::ticks_bits_per_tag);
		step_frequency = 1;
	}
	return frei;
}



void
verkehrsteilnehmer_t::hop()
{
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
	const weg_t *weg = from->gib_weg(weg_t::strasse);
	if(weg==NULL) {
		// no gound here any more?
		pos_next = gib_pos();
		return;
	}

	int ribi = weg->gib_ribi_unmasked();
	// 2) take care of roadsigns
	if(weg->gib_ribi_maske()!=0) {
		// since routefinding is backwards, the allowed directions at a roadsign are also backwards
		ribi &= ~( ribi_t::rueckwaerts(weg->gib_ribi_maske()) );
	}
	ribi_t::ribi gegenrichtung = ribi_t::rueckwaerts( gib_fahrtrichtung() );

	ribi = ribi & (~gegenrichtung);

	// add all good ribis here
	for(int r = 0; r < 4; r++) {
		if(  (ribi & ribi_t::nsow[r])!=0  &&  (ribi_t::nsow[r]&gegenrichtung)==0 &&
			from->get_neighbour(to, weg_t::strasse, koord::nsow[r])
		) {
			// check, if this is just a single tile deep
			int next_ribi =  to->gib_weg(weg_t::strasse)->gib_ribi_unmasked();
			if((ribi&next_ribi)!=0  ||  !ribi_t::ist_einfach(next_ribi)) {
				const roadsign_t *rs = dynamic_cast<roadsign_t *>(to->obj_bei(0));
				if(rs==NULL  ||  rs->gib_besch()->is_traffic_light()  ||  rs->gib_besch()->gib_min_speed()==0  ||  rs->gib_besch()->gib_min_speed()<=gib_max_speed()) {
					liste[count++] = to;
				}
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
	age();
}



bool verkehrsteilnehmer_t::sync_step(long delta_t)
{
	if(gib_age()<=0) {
		// remove obj
		step_frequency = 0;
DBG_MESSAGE("verkehrsteilnehmer_t::sync_step()","sopped");
  		return false;
	}

	if(step_frequency>0) {
		// step will check for traffic jams
		return true;
	}
    weg_next += (current_speed*delta_t) / 64;

	// Funktionsaufruf vermeiden, wenn möglich
	// if ist schneller als Aufruf!
	if(hoff) {
		setze_yoff( gib_yoff() - hoff );
	}

	while(1024 < weg_next)
	{
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
		dx = l;
		file->rdwr_long(l, "\n");
		dy = l;
		file->rdwr_enum(fahrtrichtung, " ");
		file->rdwr_long(l, "\n");
		hoff = l;
	}
	else {
		file->rdwr_long(max_speed, "\n");
		file->rdwr_long(weg_next, "\n");
		file->rdwr_byte(dx, " ");
		file->rdwr_byte(dy, "\n");
		file->rdwr_enum(fahrtrichtung, " ");
		file->rdwr_short(hoff, "\n");
	}
	pos_next.rdwr(file);

	// Hajo: avoid endless growth of the values
	// this causes lockups near 2**32
	weg_next &= 1023;

	if(file->is_loading()) {
		// Hajo: pre-speed limit game had city cars with max speed of
		// 60 km/h, since city raods now have a speed limit of 50 km/h
		// we can use 80 for the cars so that they speed up on
		// intercity roads
		if(file->get_version() <= 84000 && max_speed==kmh_to_speed(60)) {
			max_speed = kmh_to_speed(80);
		}
	}
}
