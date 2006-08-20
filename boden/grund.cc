/* grund.cc
 *
 * Untergrund Basisklasse für Simutrans.
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <string.h>

#include "../simdebug.h"

#include "grund.h"
#include "fundament.h"

#include "wege/weg.h"
#include "wege/monorail.h"
#include "wege/schiene.h"
#include "wege/strasse.h"
#include "wege/kanal.h"
#include "wege/runway.h"

#include "../gui/karte.h"
#include "../gui/ground_info.h"

#include "../simworld.h"
#include "../simvehikel.h"
#include "../simplay.h"
#include "../simwin.h"
#include "../simimg.h"
#include "../simgraph.h"
#include "../simdepot.h"
#include "../simfab.h"
#include "../simhalt.h"
#include "../blockmanager.h"

#include "../dings/bruecke.h"
#include "../dings/gebaeude.h"

#include "../besch/haus_besch.h"
#include "../besch/kreuzung_besch.h"

#include "../bauer/wegbauer.h"
#include "../besch/weg_besch.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"

#include "../utils/cbuffer_t.h"

#include "../simcolor.h"
#include "../simgraph.h"


#include "../tpl/ptrhashtable_tpl.h"
#include "../tpl/inthashtable_tpl.h"
#include "../tpl/vector_tpl.h"

#include "../dings/leitung2.h"	// for construction of new ways ...

// for debugging: show which tiles are grown as dings
//#define SHOW_FORE_GRUND 1

// klassenlose funktionen und daten


image_id grund_t::gib_back_bild(int leftback) const
{
	if(back_bild_nr==0) {
		return IMG_LEER;
	}
	sint8 back_bild = abs(back_bild_nr);
	back_bild = leftback ? (back_bild/11)+11 : back_bild%11;
	if(back_bild_nr<0) {
		return grund_besch_t::fundament->gib_bild(back_bild);
	}
	else {
		return grund_besch_t::slopes->gib_bild(back_bild);
	}
}



// with double height ground tiles!
// can also happen with single height tiles
static inline uint8
get_backbild_from_diff( sint8 h1, sint8 h2 )
{
	if(h1==h2) {
		// vertical slope: which height?
		return h1*4;
	}
	else if(h1*h2<0) {
		// middle slop of double height
		return h1<0 ? 9 : 10;
	}
	else {
		return (h1>0?h1:0)+(h2>0?h2:0)*3;
	}
}


#ifndef DOUBLE_GROUNDS
#define corner1(i) (i%2)
#define corner2(i) ((i/2)%2)
#define corner3(i) ((i/4)%2)
#define corner4(i) (i/8)
#else
#define corner1(i) (i%3)
#define corner2(i) ((i/3)%3)
#define corner3(i) ((i/9)%3)
#define corner4(i) (i/27)
#endif

// artifical walls from here on ...
void grund_t::calc_back_bild(const sint8 hgt,const sint8 slope_this)
{
	sint8 back_bild_nr=0;
	sint8 is_building=0;
	const koord k = gib_pos().gib_2d();

	clear_flag(grund_t::draw_as_ding);
	bool	left_back_is_building = false;

	// check for foundation
	if(k.x>0  &&  k.y>0) {
		const grund_t *gr=welt->lookup(k+koord(-1,-1))->gib_kartenboden();
		if(gr) {
			const sint16 left_hgt=gr->gib_hoehe()/16;
			const sint8 slope=gr->gib_grund_hang();

			const sint8 diff_from_ground = left_hgt+corner2(slope)-hgt-corner4(slope_this);
			// up slope hiding something ...
			if(diff_from_ground<0)  {
				set_flag(grund_t::draw_as_ding);
			}
			else if(gr->get_flag(grund_t::draw_as_ding)  ||  gr->obj_count()>0) {
				left_back_is_building = true;
			}
		}
	}

	// now enter the left two height differences
	if(k.x>0) {
		const grund_t *gr=welt->lookup(k+koord(-1,0))->gib_kartenboden();
		if(gr) {
			const sint16 left_hgt=gr->gib_hoehe()/16;
			const sint8 slope=gr->gib_grund_hang();

			const sint8 diff_from_ground_1 = left_hgt+corner2(slope)-hgt;
			const sint8 diff_from_ground_2 = left_hgt+corner3(slope)-hgt;
			// up slope hiding something ...
			if(diff_from_ground_1-corner1(slope_this)<0  ||  diff_from_ground_2-corner4(slope_this)<0)  {
				set_flag(grund_t::draw_as_ding);
			}
			// any height difference AND something to see?
			if(  (diff_from_ground_1-corner1(slope_this)>0  ||  diff_from_ground_2-corner4(slope_this)>0)
				&&  (diff_from_ground_1>0  ||  diff_from_ground_2>0)  ) {
				back_bild_nr = get_backbild_from_diff( diff_from_ground_1, diff_from_ground_2 );
			}
			// avoid covering of slope by building ...
			if(left_back_is_building  &&  (back_bild_nr>0  ||  gr->gib_back_bild(1)!=IMG_LEER)) {
				set_flag(grund_t::draw_as_ding);
			}
			is_building = gr->gib_typ()==grund_t::fundament;
		}
	}

	// now enter the back two height differences
	if(k.y>0) {
		const grund_t *gr=welt->lookup(k+koord(0,-1))->gib_kartenboden();
		if(gr) {
			const sint16 back_hgt=gr->gib_hoehe()/16;
			const sint8 slope=gr->gib_grund_hang();

			const sint8 diff_from_ground_1 = back_hgt+corner1(slope)-hgt;
			const sint8 diff_from_ground_2 = back_hgt+corner2(slope)-hgt;
			// up slope hiding something ...
			if(diff_from_ground_1-corner4(slope_this)<0  ||  diff_from_ground_2-corner3(slope_this)<0) {
				set_flag(grund_t::draw_as_ding);
			}
			// any height difference AND something to see?
			if(  (diff_from_ground_1-corner4(slope_this)>0  ||  diff_from_ground_2-corner3(slope_this)>0)
				&&  (diff_from_ground_1>0  ||  diff_from_ground_2>0)  ) {
				back_bild_nr += get_backbild_from_diff( diff_from_ground_1, diff_from_ground_2 )*11;

			}
			is_building |= gr->gib_typ()==grund_t::fundament;
			// avoid covering of slope by building ...
			if(left_back_is_building  &&  (back_bild_nr>11  ||  gr->gib_back_bild(0)!=IMG_LEER)) {
				set_flag(grund_t::draw_as_ding);
			}
		}
	}

	// not ground -> then not draw first ...
	if(welt->lookup(k)->gib_kartenboden()!=this) {
		clear_flag(grund_t::draw_as_ding);
	}

	back_bild_nr %= 121;
	this->back_bild_nr = (is_building!=0)? -back_bild_nr : back_bild_nr;
}




/**
 * Table of ground texts
 * @author Hj. Malthaner
 */
inthashtable_tpl <unsigned long, const char*> ground_texts;


/**
 * Pointer to the world of this ground. Static to conserve space.
 * Change to instance variable once more than one world is available.
 * @author Hj. Malthaner
 */
karte_t * grund_t::welt = NULL;


ptrhashtable_tpl<grund_t *, grund_info_t *> *grund_t::grund_infos = new ptrhashtable_tpl<grund_t *, grund_info_t *> ();


void grund_t::entferne_grund_info() {
    grund_infos->remove(this);
}


hang_t::typ
grund_t::gib_grund_hang() const
{
    return welt->get_slope(pos.gib_2d());
}


/**
 * Setzt den Beschreibungstext.
 * @param text Der neue Beschreibungstext.
 * @see grund_t::text
 * @author Hj. Malthaner
 */
void grund_t::setze_text(const char *text)
{
  // printf("Height %x\n", (pos.z - welt->gib_grundwasser()) >> 4);

  const unsigned long n =
    (pos.x << 19)
    + (pos.y << 6)
    + ((pos.z - welt->gib_grundwasser()) >> 4);

  const char * old = ground_texts.get(n);

  if(old != text) {
    ground_texts.remove(n);
    clear_flag(has_text);

    if(text) {
      ground_texts.put(n, text);
      set_flag(has_text);
    }

    set_flag(new_text);
    welt->setze_dirty();
  }
}


/**
 * Gibt eine Beschreibung des Untergrundes (informell) zurueck.
 * @return Einen Beschreibungstext zum Untergrund.
 * @author Hj. Malthaner
 */
const char* grund_t::gib_text() const
{
  const char * result = 0;

  if(flags & has_text) {
    const unsigned long n =
      (pos.x << 19)
      + (pos.y << 6)
      + ((pos.z - welt->gib_grundwasser()) >> 4);

    result = ground_texts.get(n);
  }

  return result;
}


/**
 * Ermittelt den Besitzer des Untergrundes.
 * @author Hj. Malthaner
 */
spieler_t * grund_t::gib_besitzer() const
{
  return besitzer_n == -1 ? 0 : welt->gib_spieler(besitzer_n);
}


/**
 * Setze den Besitzer des Untergrundes. Gibt false zurück, wenn
 * das setzen fehlgeschlagen ist.
 * @author Hj. Malthaner
 */
bool grund_t::setze_besitzer(spieler_t *s)
{
  besitzer_n = welt->sp2num(s);
  return true;
}


grund_t::grund_t(karte_t *wl)
{
	wege[0]= wege[1] = NULL;
	welt = wl;
	flags = 0;
	bild_nr = IMG_LEER;
	back_bild_nr = 0;
	halt = halthandle_t();
}


grund_t::grund_t(karte_t *wl, loadsave_t *file)
{
	// only used for saving?
	memset(wege, 0, sizeof(wege));
	welt = wl;
	flags = 0;
	back_bild_nr = 0;
	halt = halthandle_t();
	rdwr(file);
}

void grund_t::rdwr(loadsave_t *file)
{
	file->wr_obj_id(gib_typ());
	pos.rdwr(file);
//DBG_DEBUG("grund_t::rdwr()", "pos=%d,%d,%d", pos.x, pos.y, pos.z);

	if(file->is_saving()) {
		const char *text = gib_text();
		file->rdwr_str(text, "+");
	}
	else {
		const char *text = 0;
		file->rdwr_str(text, "+");
		setze_text(text);
	}

	bool label;

	if(file->is_saving()) {
		label = welt->gib_label_list().contains(pos.gib_2d());
	}
	file->rdwr_bool(label, "\n");
	if(file->is_loading() && label) {
		welt->add_label(gib_pos().gib_2d());
	}
	sint8 dummy8=besitzer_n;
	file->rdwr_byte(dummy8, "\n");
	besitzer_n = dummy8;

    if(file->is_loading()) {
	weg_t::typ wtyp;
	int i = -1;
	do {
	    wtyp = (weg_t::typ)file->rd_obj_id();

	    if(++i < MAX_WEGE) {
		switch(wtyp) {
		default:
		  wege[i] = NULL;
		  break;
		case weg_t::strasse:
//		  DBG_DEBUG("grund_t::rdwr()", "road");
		  wege[i] = new strasse_t (welt, file);
		  break;
		case weg_t::monorail:
		  wege[i] = new monorail_t (welt, file);
//		  DBG_DEBUG("grund_t::rdwr()", "monorail");
		  break;
		case weg_t::schiene:
		{
		  schiene_t *sch = new schiene_t (welt, file);
		  if(sch->gib_besch()->gib_wtyp()==weg_t::monorail) {
		  	dbg->warning("grund_t::rdwr()", "converting railroad to monorail at (%i,%i)",gib_pos().x, gib_pos().y);
		  	// compatibility code: Convert to monorail
		  	monorail_t *w= new monorail_t(welt);
		  	w->setze_besch(sch->gib_besch());
		  	w->setze_blockstrecke(sch->gib_blockstrecke());
		  	w->setze_max_speed(sch->gib_max_speed());
		  	w->setze_ribi(sch->gib_ribi_unmasked());
		  	w->setze_ribi_maske(sch->gib_ribi_maske());
		  	delete sch;
		  	wege[i] = w;
		  }
		  else {
			  wege[i] = sch;
//		  DBG_DEBUG("grund_t::rdwr()", "railroad");
			}
		}
		  break;
		case weg_t::schiene_strab:
		  wege[i] = new schiene_t (welt, file);
//            DBG_DEBUG("grund_t::rdwr()", "tram");
		  if(wege[i]->gib_besch()->gib_styp()!=7) {
		  	wege[i]->setze_besch(wegbauer_t::weg_search(weg_t::schiene_strab,wege[i]->gib_max_speed()));
			DBG_DEBUG("grund_t::rdwr()", "tram replace");
		  }
		  break;
		case weg_t::wasser:
			// ignore old type dock ...
			if(file->get_version()>=87000) {
//			  DBG_DEBUG("grund_t::rdwr()", "Kanal");
			  wege[i] = new kanal_t (welt, file);
//		      DBG_MESSAGE("kanal_t::kanal_t()","at (%i,%i) ribi %i",gib_pos().x, gib_pos().y, wege[i]->gib_ribi());
		     }
		     else {
		     		// ignore old dock
		     		unsigned char d8;
		     		short d16;
		     		long d32;

				file->rdwr_byte(d8, "\n");
				file->rdwr_short(d16, "\n");
				file->rdwr_long(d32, "\n");
				file->rdwr_long(d32, "\n");
				file->rdwr_long(d32, "\n");
				file->rdwr_long(d32, "\n");
			      DBG_MESSAGE("grund_t::rdwr()","at (%i,%i) dock ignored",gib_pos().x, gib_pos().y);
		     	}
		  break;
		case weg_t::luft:
//			DBG_DEBUG("grund_t::rdwr()", "Runway");
			wege[i] = new runway_t (welt, file);
			DBG_MESSAGE("runway_t::runway_t()","at (%i,%i) ribi %i",gib_pos().x, gib_pos().y, wege[i]->gib_ribi());

			break;
		}
		if(wege[i]) {
		  wege[i]->setze_pos(pos);
		  if(gib_besitzer() && !ist_wasser()) {
		    gib_besitzer()->add_maintenance(wege[i]->gib_besch()->gib_wartung());
		  }
		}
	    }
	} while(wtyp != weg_t::invalid);
	while(++i < MAX_WEGE) {
	    wege[i] = NULL;
	}
  }
	else {
		// saving
		for(int i = 0; i < MAX_WEGE; i++) {
			if(wege[i]) {
			wege[i]->rdwr(file);
			}
		}
		file->wr_obj_id(-1);   // Ende der Wege
	}
	// all objects on this tile
	dinge.rdwr(welt, file,gib_pos());

	if(file->is_loading()) {
		flags |= dirty;
		// set speedlimit for birdges
		if(gib_typ()==brueckenboden) {
			bruecke_t *br=dynamic_cast<bruecke_t *>(suche_obj(ding_t::bruecke));
			if(wege[0]  &&  br) {
				wege[0]->setze_max_speed(br->gib_besch()->gib_topspeed());
			}
		}
	}
	//DBG_DEBUG("grund_t::rdwr()", "loaded at %i,%i with %i dinge bild %i.", pos.x, pos.y, obj_count(),bild_nr);
}


grund_t::grund_t(karte_t *wl, koord3d pos)
{
	this->pos = pos;
	flags = 0;
	besitzer_n = -1;

	wege[0]= wege[1] = NULL;
	welt = wl;
	setze_bild(IMG_LEER);    // setzt   flags = dirty;
	back_bild_nr = 0;
	halt = halthandle_t();
}


grund_t::~grund_t()
{
    destroy_win(grund_infos->get(this));

    // remove text from table
    ground_texts.remove((pos.x << 16) + pos.y);

    if(halt.is_bound()) {
//	printf("Enferne boden %p von Haltestelle %p\n", this, halt);fflush(NULL);
// check for memory leaks!
	halt->rem_grund(this);
	halt.unbind();
  }

    for(int i = 0; i < MAX_WEGE; i++) {
	if(wege[i]) {
	    if(gib_besitzer() && !ist_wasser() && wege[i]->gib_besch()) {
	        gib_besitzer()->add_maintenance(-wege[i]->gib_besch()->gib_wartung());
	    }

	    delete wege[i];
    	    wege[i] = NULL;
	}
  }
}



/**
 * Gibt den Namen des Untergrundes zurueck.
 * @return Den Namen des Untergrundes.
 * @author Hj. Malthaner
 */
const char* grund_t::gib_name() const {
    return "Grund";
};


void grund_t::info(cbuffer_t & buf) const
{
	if(halt.is_bound()) {
		halt->info( buf );
	}

	for(int i = 0; i < MAX_WEGE; i++) {
		if(wege[i]) {

			if(wege[i]->gib_typ()==weg_t::wasser  &&  !ist_wasser()) {
				wege[i]->info(buf);
			}
			else {
				wege[i]->info(buf);
			}
		}
	}

#if 0
	if(buf.len() == 0) {
		uint8 hang= gib_grund_hang();
		buf.append(gib_hoehe());
		buf.append("\nslope: ");
		buf.append(hang);
		buf.append("\nhang heights: ");
		buf.append(hang%3);
		buf.append(", ");
		buf.append((hang/3)%3);
		buf.append(", ");
		buf.append((hang/9)%3);
		buf.append(", ");
		buf.append((hang/27)%3);
		buf.append("\nback0: ");
		buf.append(gib_back_bild(0)-grund_besch_t::slopes->gib_bild(0));
		buf.append("\nback1: ");
		buf.append(gib_back_bild(1)-grund_besch_t::slopes->gib_bild(0));
		//    buf.append("\n");
		//    buf.append(translator::translate("Keine Info."));
	}
	buf.append("\nway slope");
	buf.append((int)gib_weg_hang());
	buf.append("\npos: ");
	buf.append(pos.x);
	buf.append(", ");
	buf.append(pos.y);
	buf.append(", ");
	buf.append(pos.z);
#endif
}



/**
 * Manche Böden können zu Haltestellen gehören.
 * Der Zeiger auf die Haltestelle wird hiermit gesetzt.
 * @author Hj. Malthaner
 */
void grund_t::setze_halt(halthandle_t halt) {
	this->halt = halt;
//	welt->lookup( pos.gib_2d() )->setze_halt(halt);
}



/**
 * grund_t::calc_bild:
 *
 * Berechnet nur das Wegbild - den Rest bitte in der Ableitung setzen
 *
 * @author V. Meyer
 */
void grund_t::calc_bild()
{
	// Hajo: recalc overhead line image if there is a overhead line here
	if(wege[0]) {
		ding_t *dt = suche_obj(ding_t::oberleitung);
		if(dt) {
			dt->calc_bild();
		}
	}
	// recalc way image
	if(ist_uebergang()) {
			ribi_t::ribi ribi0 = wege[0]->gib_ribi();
			ribi_t::ribi ribi1 = wege[1]->gib_ribi();

			if(ribi_t::ist_gerade_ns(ribi0) || ribi_t::ist_gerade_ow(ribi1)) {
				wege[0]->setze_bild( wegbauer_t::gib_kreuzung(wege[0]->gib_typ(), wege[1]->gib_typ() )->gib_bild_nr() );
			}
			else {
				wege[0]->setze_bild( wegbauer_t::gib_kreuzung(wege[1]->gib_typ(), wege[0]->gib_typ() )->gib_bild_nr() );
			}
			wege[1]->setze_bild( IMG_LEER );
	}
	else {
		// a bridge will set this to empty anyway ...
		if(wege[0]) {
			wege[0]->calc_bild(pos);
		}
		if(wege[1]) {
			wege[1]->calc_bild(pos);
		}
	}
	// Das scheint die beste Stelle zu sein
	if(ist_karten_boden()) {
		reliefkarte_t::gib_karte()->recalc_relief_farbe(gib_pos().gib_2d());
	}
}


int grund_t::ist_karten_boden() const
{
    return welt->lookup(pos.gib_2d())->gib_kartenboden() == this;
}

/**
 * Wir gehen davon aus, das pro Feld nur ein Gebauede erlaubt ist!
 */
bool grund_t::hat_gebaeude(const haus_besch_t *besch) const
{
    gebaeude_t *gb = static_cast<gebaeude_t *>(suche_obj(ding_t::gebaeude));

    return gb && gb->gib_tile()->gib_besch() == besch;
}

depot_t *grund_t::gib_depot() const
{
    ding_t *dt = obj_bei(PRI_DEPOT);

    return dynamic_cast<depot_t *>(dt);
}

const char * grund_t::gib_weg_name(weg_t::typ typ) const
{
    weg_t   *weg = (typ==weg_t::invalid) ? wege[0] : gib_weg(typ);

    if(weg) {
	return weg->gib_name();
    } else {
	return NULL;
    }
}


ribi_t::ribi grund_t::gib_weg_ribi(weg_t::typ typ) const
{
    weg_t *weg = gib_weg(typ);

    if(weg)
	return weg->gib_ribi();
    else
	return ribi_t::keine;
}


ribi_t::ribi grund_t::gib_weg_ribi_unmasked(weg_t::typ typ) const
{
    weg_t *weg = gib_weg(typ);

    if(weg)
	return weg->gib_ribi_unmasked();
    else
	return ribi_t::keine;
}


int
grund_t::text_farbe() const
{
	// if this gund belongs to a halt, the color should reflect the halt owner, not the grund owner!
	if(halt.is_bound()  &&  halt->gib_besitzer()) {
		return halt->gib_besitzer()->kennfarbe+2;
	}

	// else color according to current owner
	const spieler_t *sp = gib_besitzer();
	if(sp) {
		return sp->kennfarbe+2;
	} else {
		return 101;
	}
}


void grund_t::betrete(vehikel_basis_t *v)
{
    weg_t *weg = gib_weg(v->gib_wegtyp());

    if(weg) {
	weg->betrete(v);
    }
}

void grund_t::verlasse(vehikel_basis_t *v)
{
    weg_t *weg = gib_weg(v->gib_wegtyp());

    if(weg) {
	weg->verlasse(v);
    }
}


inline
void grund_t::do_display_boden( const int xpos, const int ypos, const bool dirty ) const
{
	// walls
	if(back_bild_nr!=0) {
		const sint8 back_bild2 = (abs(back_bild_nr)/11)+11;
		const sint8 back_bild1 = abs(back_bild_nr)%11;
		if(back_bild_nr<0) {
			display_img(grund_besch_t::fundament->gib_bild(back_bild1), xpos, ypos, dirty);
			display_img(grund_besch_t::fundament->gib_bild(back_bild2), xpos, ypos, dirty);
		}
		else {
			display_img(grund_besch_t::slopes->gib_bild(back_bild1), xpos, ypos, dirty);
			display_img(grund_besch_t::slopes->gib_bild(back_bild2), xpos, ypos, dirty);
		}
	}

	// ground
	display_img( gib_bild(), xpos, ypos, dirty);

	if(wege[0]) {
		display_img(wege[0]->gib_bild(), xpos, ypos - gib_weg_yoff(), dirty);
	}

	if(wege[1]){
		display_img(wege[1]->gib_bild(), xpos, ypos - gib_weg_yoff(), dirty);
	}
}



void
grund_t::display_boden(const int xpos, int ypos, bool dirty) const
{
	if(!get_flag(grund_t::draw_as_ding)) {
		dirty |= get_flag(grund_t::dirty);
		int raster_tile_width = get_tile_raster_width();

		ypos -= tile_raster_scale_y( gib_hoehe(), raster_tile_width);

		/* we can save some time but just don't display at all
		 * @author prissi
		 */
		if(  ypos>32-raster_tile_width  &&  ypos<display_get_height()-raster_tile_width/4) {
			do_display_boden( xpos, ypos, dirty );
		}
	}
}


void
grund_t::display_dinge(const int xpos, int ypos, bool dirty)
{
	int n;
	int raster_tile_width = get_tile_raster_width();
	ypos -= tile_raster_scale_y( gib_hoehe(), raster_tile_width);

	dirty |= get_flag(grund_t::dirty);
	clear_flag(grund_t::dirty);

	if(get_flag(grund_t::draw_as_ding)) {
		do_display_boden( xpos, ypos, dirty );
	}

	/* we can save some time but just don't display at all
	 * @author prissi
	 */
	if(  ypos>32-raster_tile_width  &&  ypos<display_get_height()+32+raster_tile_width) {

		// background
		for(n=0; n<gib_top(); n++) {
			ding_t * dt = obj_bei(n);
			if( dt ) {
				// ist dort ein objekt ?
				dt->display(xpos, ypos, dirty);
			}
		}

		// foreground
		for(n=0; n<gib_top(); n++) {
			ding_t * dt = obj_bei(n);
			if( dt ) {
				// ist dort ein vordergrund objekt ?
				dt->display_after(xpos, ypos, dirty);
				dt->clear_flag(ding_t::dirty);
			}
		}

		const char * text = gib_text();
		if(text && (umgebung_t::show_names & 1)) {
			const int width = proportional_string_width(text)+7;

			display_ddd_proportional_clip(xpos - (width - raster_tile_width)/2,
				     ypos,
				     width,
				     0,
				     text_farbe(),
				     COL_BLACK,
				     text,
				     dirty || get_flag(grund_t::new_text));

			clear_flag(grund_t::new_text);
		}

		// display station sign
		if(text && (umgebung_t::show_names & 2) && halt.is_bound()) {
			halt->display_status(xpos, ypos);
		}

		// display station owner boxes
		planquadrat_t *pl=welt->access(pos.gib_2d());
		if(umgebung_t::station_coverage_show  &&  pl->gib_kartenboden()==this) {
			int r=raster_tile_width/8;
			int x=xpos+raster_tile_width/2-r;
			int y=ypos+(raster_tile_width*3)/4-r;

			// suitable start search
			const minivec_tpl<halthandle_t> &halt_list = pl->get_haltlist();
			for(int h=halt_list.get_count()-1;  h>=0;  h--  ) {
				display_fillbox_wh_clip( x-h*2, y+h*2, r, r, halt_list.at(h)->gib_besitzer()->kennfarbe+2, dirty);
			}
		}
#ifdef SHOW_FORE_GRUND
		if(get_flag(grund_t::draw_as_ding)) {
			display_fillbox_wh_clip( xpos+raster_tile_width/2, ypos+(raster_tile_width*3)/4, 16, 16, 0, dirty);
		}
#endif
	}
}




/**
 * Bauhilfsfunktion - die ribis eines vorhandenen weges werden erweitert
 *
 * @return bool	    true, falls weg vorhanden
 * @param wegtyp    um welchen wegtyp geht es
 * @param ribi	    die neuen ribis
 *
 * @author V. Meyer
 */
bool grund_t::weg_erweitern(weg_t::typ wegtyp, ribi_t::ribi ribi)
{
	weg_t   *weg = gib_weg(wegtyp);
	if(weg) {
		weg->ribi_add(ribi);
		calc_bild();
		return true;
	}
	return false;
}

/**
 * Bauhilfsfunktion - ein neuer weg wird mit den vorgegebenen ribis
 * eingetragen und der Grund dem Erbauer zugeordnet.
 *
 * @return bool	    true, falls weg nicht vorhanden war
 * @param weg	    der neue Weg
 * @param ribi	    die neuen ribis
 * @param sp	    Spieler, dem der Boden zugeordnet wird
 *
 * @author V. Meyer
 */
bool grund_t::neuen_weg_bauen(weg_t *weg, ribi_t::ribi ribi, spieler_t *sp)
{
    // Hajo: falld das Feld noch keine Besitzer hat, nehmen wir es in Besitz
    // egal ob der Weg schon da war oder nicht.

    if(gib_besitzer() == NULL) {
	setze_besitzer( sp );
    }

	// has a powerline before
	// @author prissi
	leitung_t *lt=dynamic_cast<leitung_t *>(suche_obj(ding_t::leitung));
	spieler_t *lt_sp=NULL;
	if(lt) {
		lt_sp = lt->gib_besitzer();
		obj_remove(lt,sp);
	}

	const weg_t * alter_weg = gib_weg(weg->gib_typ());
	if(alter_weg == NULL) {
		// Hajo: nur fuer neubau 'planieren'
		if(wege[0] == 0) {
			obj_loesche_alle(sp);
		}

		for(int i = 0; i < MAX_WEGE; i++) {
			if(!wege[i]) {
				wege[i] = weg;
				if(gib_besitzer() && !ist_wasser()) {
					gib_besitzer()->add_maintenance(weg->gib_besch()->gib_wartung());
				}
				break;
			}
		}
		weg->setze_ribi(ribi);
		weg->setze_pos(pos);
		calc_bild();
		// readd powerline
		if(lt_sp!=NULL) {
			obj_add(lt);
			lt->calc_neighbourhood();
		}
		return true;
	}
	return false;
}


/**
 * Bauhilfsfunktion - einen Weg entfernen
 *
 * @return bool	    true, falls weg vorhanden war
 * @param wegtyp    um welchen wegtyp geht es
 * @param ribi_rem  sollen die ribis der nachbar zurückgesetzt werden?
 *
 * @author V. Meyer
 */
sint32 grund_t::weg_entfernen(weg_t::typ wegtyp, bool ribi_rem)
{
	weg_t   *weg = gib_weg(wegtyp);

	if(weg) {
		if(ribi_rem) {
			ribi_t::ribi ribi = weg->gib_ribi();
			grund_t *to;

			for(int r = 0; r < 4; r++) {
				if((ribi & ribi_t::nsow[r]) && get_neighbour(to, wegtyp, koord::nsow[r])) {
					weg_t *weg2 = to->gib_weg(wegtyp);
					if(weg2  &&  !ist_tunnel()   &&  !ist_bruecke()) {
						weg2->ribi_rem(ribi_t::rueckwaerts(ribi_t::nsow[r]));
						to->calc_bild();
					}
				}
			}
		}

		sint32 costs=0;	// costs for removal are construction costs

		for(sint8 i=0, j=-1; i<MAX_WEGE; i++) {
			if(wege[i]) {
				if(wege[i]->gib_typ() == wegtyp) {
					if(gib_besitzer() && !ist_wasser()) {
						gib_besitzer()->add_maintenance(-wege[i]->gib_besch()->gib_wartung());
					}

					costs += wege[i]->gib_besch()->gib_preis();
					delete wege[i];
					wege[i] = NULL;

					j = i;
				}
				else if(j != -1) {	// shift up!
					wege[j] = wege[i];
					wege[i] = NULL;
					j = i;
				}
			}
		}
		calc_bild();

		return costs;
	}
	return 0;
}

bool grund_t::get_neighbour(grund_t *&to, weg_t::typ type, koord dir) const
{
	if(dir != koord::nord && dir != koord::sued && dir != koord::ost && dir != koord::west) {
		return false;
	}
	grund_t *gr = NULL;

	const planquadrat_t * plan = welt->lookup(pos.gib_2d() + dir);
	if(!plan) {
		return false;
	}

	// find ground in the same height: This work always!
	const sint16 this_height = get_vmove(dir);
	for( unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
		gr = plan->gib_boden_bei(i);
		if(gr->get_vmove(-dir)==this_height) {
			// test, if connected
			if(!is_connected(gr, type, dir)) {
				continue;
			}
			to = gr;
			return true;
		}
	}
//	DBG_MESSAGE("grund_t::get_neighbour()","No ground connection from %i,%i,%i to dir %i,%i",pos.x,pos.y,pos.z,dir.x,dir.y);
	return false;
}

bool grund_t::is_connected(const grund_t *gr, weg_t::typ wegtyp, koord dv) const
{
	if(!gr) {
		return false;
	}
	if(wegtyp==weg_t::invalid) {
		return true;
	}

	const int ribi1 = gib_weg_ribi_unmasked(wegtyp);
	const int ribi2 = gr->gib_weg_ribi_unmasked(wegtyp);
//	DBG_MESSAGE("grund_t::is_connected()","at (%i,%i,%i) ribi1=%i at (%i,%i,%i) ribi2=%i",pos.x,pos.y,pos.z,ribi1,gr->gib_pos().x,gr->gib_pos().y,gr->gib_pos().z,ribi2);
	if(dv == koord::nord) {
		return (ribi1 & ribi_t::nord) && (ribi2 & ribi_t::sued);
	} else if(dv == koord::sued) {
		return (ribi1 & ribi_t::sued) && (ribi2 & ribi_t::nord);
	} else if(dv == koord::west) {
		return (ribi1 & ribi_t::west) && (ribi2 & ribi_t::ost );
	} else if(dv == koord::ost) {
		return (ribi1 & ribi_t::ost ) && (ribi2 & ribi_t::west);
	}
	return false;
}



// now we need a more sophisticated calculations ...
int grund_t::get_vmove(koord dir) const
{
	const sint8 slope=gib_weg_hang();
	sint16 h=gib_hoehe();
	if(ist_bruecke()  &&  gib_grund_hang()!=0  &&  welt->lookup(pos)==this) {
		h += 16;	// end or start of a bridge
	}

	if(dir == koord::ost) {
		h += corner3(slope)*16;
//		h += corner2(slope)*16;
	} else if(dir == koord::west) {
		h += corner1(slope)*16;
//		h += corner4(slope)*16;
	} else if(dir == koord::sued) {
		h += corner1(slope)*16;
//		h += corner2(slope)*16;
	} else if(dir == koord::nord) {
		h += corner3(slope)*16;
//		h += corner4(slope)*16;
	} else {
		trap();	// error: not a direction ...
	}
	return h;   // no way slope
}

int grund_t::get_max_speed()
{
	int max = 0;
	int i = 0;
	int tmp = 0;

	while(wege[i] && i<MAX_WEGE) {
		tmp = wege[i]->gib_max_speed();
	  if (tmp > max) {
	  	max = tmp;
	  }
  	i++;
	}

	return max;
}
