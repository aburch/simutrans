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
#include "wege/schiene.h"
#include "wege/strasse.h"
#include "wege/dock.h"

#include "../gui/karte.h"
#include "../gui/ground_info.h"

#include "../simworld.h"
#include "../simvehikel.h"
#include "../simplay.h"
#include "../simwin.h"
#include "../simimg.h"
#include "../simgraph.h"
#include "../simdepot.h"
#include "../simhalt.h"
#include "../blockmanager.h"

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




// klassenlose funktionen und daten


/**
 * Table of ground texts
 * @author Hj. Malthaner
 */
inthashtable_tpl <unsigned int, const char*> ground_texts;


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

  const unsigned int n =
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
    const unsigned int n =
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
    memset(wege, 0, sizeof(wege));
    welt = wl;
    flags = 0;
    back_bild_nr = -1;
}


grund_t::grund_t(karte_t *wl, loadsave_t *file)
{
    memset(wege, 0, sizeof(wege));
    welt = wl;
    flags = 0;
    back_bild_nr = -1;

    rdwr(file);
}

void grund_t::rdwr(loadsave_t *file)
{
    file->wr_obj_id(gib_typ());
    pos.rdwr(file);

    // dbg->debug("grund_t::rdwr()", "pos=%d,%d,%d", pos.x, pos.y, pos.z);


    if(file->is_saving()) {
      const char *text = gib_text();
      file->rdwr_str(text, "+");
    } else {
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
    file->rdwr_signed_char(besitzer_n, "\n");

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
		  dbg->debug("grund_t::rdwr()", "road");
		  wege[i] = new strasse_t (welt, file);
		  break;
		case weg_t::schiene:
		  dbg->debug("grund_t::rdwr()", "railroad");
		  wege[i] = new schiene_t (welt, file);
		  break;
		case weg_t::wasser:
		  dbg->debug("grund_t::rdwr()", "dock");
		  wege[i] = new dock_t (welt, file);
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
    } else {
        for(int i = 0; i < MAX_WEGE; i++) {
	    if(wege[i]) {
	        wege[i]->rdwr(file);
	    }
        }
        file->wr_obj_id(-1);   // Ende der Wege
    }
    dinge.rdwr(welt, file);
    if(file->is_loading()) {
        flags |= dirty;
    }
}


grund_t::grund_t(karte_t *wl, koord3d pos)
{
    this->pos = pos;
    flags = 0;
    besitzer_n = -1;

    memset(wege, 0, sizeof(wege));
    welt = wl;
    back_bild_nr = -1;

    setze_bild( 0 );    // setzt   flags = dirty;
    weg_bild_nr = -1;
}


grund_t::~grund_t()
{
    destroy_win(grund_infos->get(this));

    // remove text from table
    ground_texts.remove((pos.x << 16) + pos.y);


    if(gib_halt().is_bound()) {
//	printf("Enferne boden %p von Haltestelle %p\n", this, halt);

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
  if(gib_halt().is_bound()) {
    gib_halt()->info( buf );
  }

  for(int i = 0; i < MAX_WEGE; i++) {
    if(wege[i]) {

      if(wege[i]->gib_typ() == weg_t::schiene) {
	wege[i]->info(buf);
    }
      if(wege[i]->gib_typ() == weg_t::strasse) {
	wege[i]->info(buf);
    }
    }
  }

  if(buf.len() == 0) {
    buf.append("\n");
    buf.append(translator::translate("Keine Info."));
  }
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


    if(wege[1]) {    // 2 Wege da?
	// eine Schienenkreuzung
	ribi_t::ribi ribi0 = wege[0]->gib_ribi();
	ribi_t::ribi ribi1 = wege[1]->gib_ribi();

	if(ribi_t::ist_gerade_ns(ribi0) || ribi_t::ist_gerade_ow(ribi1)) {
	    setze_weg_bild(wegbauer_t::gib_kreuzung(wege[0]->gib_typ(), wege[1]->gib_typ())->gib_bild_nr());
	} else {
	    setze_weg_bild(wegbauer_t::gib_kreuzung(wege[1]->gib_typ(), wege[0]->gib_typ())->gib_bild_nr());
	}
	// Hier gibt es ein Problem falls eine Kreuzung isoliert wird -
	// sie dreht sich dann eventuell!
    }
    else if(wege[0] && !ist_bruecke()) {
	setze_weg_bild(wege[0]->calc_bild(pos));
    } else {
	setze_weg_bild(-1);
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
 * Wir ghehen davon aus, das pro Feld nur ein Gebauede erlaubt ist!
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
    weg_t   *weg = typ == weg_t::invalid ? wege[0] : gib_weg(typ);

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


void
grund_t::display_boden(const int xpos, int ypos, bool dirty) const
{
  dirty |= get_flag(grund_t::dirty);
  ypos -= (gib_hoehe() * get_tile_raster_width()) >> 6;

  if(back_bild_nr >= 0) {
    display_img(back_bild_nr, xpos, ypos, true, dirty);
  }

  display_img(gib_bild(), xpos, ypos, true, dirty);

  if(gib_weg_bild() >= 0) {
    display_img(gib_weg_bild(), xpos, ypos - gib_weg_yoff(), true, dirty);
  }
}


void
grund_t::display_dinge(const int xpos, int ypos, bool dirty)
{
  int n;
  ypos -= (gib_hoehe() * get_tile_raster_width()) >> 6;

  dirty |= get_flag(grund_t::dirty);
  clear_flag(grund_t::dirty);


  for(n=0; n<gib_top(); n++) {
    ding_t * dt = obj_bei(n);

    if( dt ) {	        // ist dort ein objekt ?
      dt->display(xpos, ypos, dirty);
    }
  }

  for(n=0; n<gib_top(); n++) {
    ding_t * dt = obj_bei(n);

    if( dt ) {	        // ist dort ein objekt ?
      dt->display_after(xpos, ypos, dirty);
      dt->clear_flag(ding_t::dirty);
    }
  }


  const char * text = gib_text();
  if(text && (umgebung_t::show_names & 1)) {
    const int width = proportional_string_width(text)+7;

    display_ddd_proportional(xpos - (width - get_tile_raster_width())/2,
			     ypos,
			     width,
			     0,
			     text_farbe(),
			     SCHWARZ,
			     text,
			     dirty ||
			     get_flag(grund_t::new_text));

    clear_flag(grund_t::new_text);
  }


  if(text && (umgebung_t::show_names & 2) && halt.is_bound()) {
    halt->display_status(xpos, ypos);
  }
}


/**
 * Manche Böden können zu Haltestellen gehören.
 * Der Zeiger auf die Haltestelle wird hiermit gesetzt.
 * @author Hj. Malthaner
 */
void grund_t::setze_halt(halthandle_t halt) {
    this->halt = halt;
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
bool grund_t::weg_entfernen(weg_t::typ wegtyp, bool ribi_rem)
{
    weg_t   *weg = gib_weg(wegtyp);

    if(weg) {
	if(ribi_rem) {
	    ribi_t::ribi ribi = weg->gib_ribi();

            // V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	    grund_t *to;

	    for(int r = 0; r < 4; r++) {
		if((ribi & ribi_t::nsow[r]) && get_neighbour(to, wegtyp, koord::nsow[r])) {
		    weg_t *weg2 = to->gib_weg(wegtyp);
		    if(weg2) {
			weg2->ribi_rem(ribi_t::rueckwaerts(ribi_t::nsow[r]));
			to->calc_bild();
		    }
		}
	    }
	}
        int	i, j;
	for(i = 0, j = -1; i < MAX_WEGE; i++) {
	    if(wege[i]) {
		if(wege[i]->gib_typ() == wegtyp) {
		    if(gib_besitzer() && !ist_wasser()) {
		      gib_besitzer()->add_maintenance(-wege[i]->gib_besch()->gib_wartung());
		    }

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

	return true;
    }
    return false;
}

bool grund_t::get_neighbour(grund_t *&to, weg_t::typ type, koord dir) const
{
    if(dir != koord::nord && dir != koord::sued &&
	dir != koord::ost && dir != koord::west) {
	 return false;
    }
    grund_t *gr = NULL;


    if(ist_bruecke() || ist_tunnel()) {
	int vmove = get_vmove(dir);

	// Kucken ob drüber oder drunter Anschluß ist
	if(vmove) {
	    gr = welt->lookup(pos + koord3d(dir, vmove));
            if(gr && gr->get_vmove(-dir) != -vmove) {
                gr = NULL;
	    }
	}
	// Wenn nicht, auf gleicher Höhe versuchen
	if(!is_connected(gr, type, dir)) {
	    gr = welt->lookup(pos + dir);
	}
    }
    else {
	// Hajo: check if we are on the map
	const planquadrat_t * plan = welt->lookup(pos.gib_2d() + dir);
	if(!plan) {
	    return false;
	}
	gr = plan->gib_kartenboden();
    }
    // Testen ob Wegverbindung existiert
    if(!is_connected(gr, type, dir)) {
	return false;
    }
    to = gr;
    return true;

}

bool grund_t::is_connected(const grund_t *gr, weg_t::typ wegtyp, koord dv) const
{
    if(!gr) {
	return false;
    }
    if(wegtyp == weg_t::invalid) {
	return true;
    }
    weg_t *weg1 = gib_weg(wegtyp);
    weg_t *weg2 = gr->gib_weg(wegtyp);
    int ribi1 = 0;
    int ribi2 = 0;

    if(weg1 != NULL) {
	ribi1 = weg1->gib_ribi_unmasked();
    } else if(wegtyp == weg_t::wasser) {
	ribi1 = ribi_t::alle;
    }
    if(weg2 != NULL) {
	ribi2 = weg2->gib_ribi_unmasked();
    } else if(wegtyp == weg_t::wasser) {
	ribi2 = ribi_t::alle;
    }
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

int grund_t::get_vmove(koord dir) const
{
static char vmoves[15][4] = {	// hangtyp * destination (N O S W)
    { -16, -16, -16, -16 },	// 0:flach
    { -16, -16,   0,   0 },	// 1:spitze SW
    { -16,   0,   0, -16 },	// 2:spitze SO
    { -16,   0,  16,   0 },	// 3:nordhang
    {   0,   0, -16, -16 },	// 4:spitze NO
    {   0,   0,   0,   0 },	// 5:spitzen SW+NO
    {   0,  16,   0, -16 },	// 6:westhang
    {   0,  16,  16,   0 },	// 7:tal NW
    {   0, -16, -16,   0 },	// 8:spitze NW
    {   0, -16,   0,  16 },	// 9:osthang
    {   0,   0,   0,   0 },	// 10:spitzen NW+SO
    {   0,   0,  16,  16 },	// 11:tal NO
    {  16,   0, -16,   0 },	// 12:suedhang
    {  16,   0,   0,  16 },	// 13:tal SO
    {  16,  16,   0,   0 }};	// 14:tal SW

    int	i;
    hang_t::typ weg_hang = gib_weg_hang();

    if(ist_bruecke() && weg_hang == 0 && ist_karten_boden()) {
	// Spezialbehandlung für "alte" Brückenauffahrten, da
	// der Weg hier komplett eine Ebene höher liegt.
	hang_t::typ gr_hang = gib_grund_hang();

	if(dir == koord::ost || dir == koord::west) {
	    if(gr_hang == hang_t::ost || gr_hang == hang_t::west) {
		return 16;
	    }
	}
	else if(dir == koord::sued || dir == koord::nord) {
	    if(gr_hang == hang_t::sued || gr_hang == hang_t::nord) {
		return 16;
	    }
	}
    }
    if(dir == koord::ost) {
	i = 1; // ost
    } else if(dir == koord::west) {
	i = 3; // west
    } else if(dir == koord::sued) {
	i = 2; // sued
    } else if(dir == koord::nord) {
	i = 0; // nord
    } else {
        assert(false);
	return 0;   // ?? Fehler
    }
    return vmoves[weg_hang][i];
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
