/*
 * simplay.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "simdebug.h"
#include "boden/grund.h"
#include "boden/wege/strasse.h"
#include "boden/wege/schiene.h"
#include "simskin.h"
#include "besch/skin_besch.h"
#include "simworld.h"
#include "simplay.h"
#include "simcosts.h"
#include "simsfx.h"
#include "railblocks.h"
#include "simwerkz.h"
#include "simfab.h"
#include "simhalt.h"
#include "simvehikel.h"
#include "simconvoi.h"
#include "simtools.h"
#include "simticker.h"

#include "simintr.h"

#include "simimg.h"
#include "simcolor.h"

#include "simwin.h"
#include "gui/messagebox.h"

#include "dings/zeiger.h"
#include "dataobj/fahrplan.h"
#include "dataobj/umgebung.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"

#include "bauer/vehikelbauer.h"
#include "bauer/warenbauer.h"
#include "bauer/brueckenbauer.h"
#include "bauer/wegbauer.h"

#include "utils/simstring.h"

#include "simgraph.h"

#ifndef tpl_slist_tpl_h
#include "tpl/slist_tpl.h"
#endif


/**
 * Konstruktor
 * @param welt Die Welt (Karte) des Spiels
 * @param color Kennfarbe des Spielers
 * @author Hj. Malthaner
 */
spieler_t::spieler_t(karte_t *wl, int color)
{
    halt_list = new slist_tpl <halthandle_t>;

    welt = wl;
    kennfarbe = color;
    // konto = 15000000;

    konto = umgebung_t::starting_money;

    konto_ueberzogen = 0;
    automat = false;		// Start nicht als automatischer Spieler

    /**
     * initialize finance history array
     * @author hsiegeln
     */

    for (int year=0; year<MAX_HISTORY_YEARS; year++) {
      for (int cost_type=0; cost_type<MAX_COST; cost_type++) {
		finance_history_year[year][cost_type] = 0;
      }
    }

    for (int month=0; month<MAX_HISTORY_MONTHS; month++) {
      for (int cost_type=0; cost_type<MAX_COST; cost_type++) {
		finance_history_month[month][cost_type] = 0;
      }
    }

    haltcount = 0;

    maintenance = 0;

    state = NEUE_ROUTE;
    substate = NR_INIT;

    gewinn = 0;
    count = 0;

    start = NULL;
    ziel = NULL;

    steps = simrand(16);

    init_texte();
}


/**
 * Destruktor
 * @author Hj. Malthaner
 */
spieler_t::~spieler_t()
{
  delete halt_list;
  halt_list = 0;
}



void
spieler_t::init_texte()
{
    for(int n=0; n<50; n++) {
	text_alter[n] = -80;
	texte[n][0] = 0;
    }

    last_message_index = 0;
}


/**
 * Zeigt Meldungen aus der Queue des Spielers auf dem Bildschirm an
 * @author Hj. Malthaner
 */
void
spieler_t::display_messages(const int xoff, const int yoff, const int width)
{
    const int raster = get_tile_raster_width();

    int last_displayed_message = -1;

    for(int n=0; n<=last_message_index; n++) {
	const int i=text_pos[n].x-welt->gib_ij_off().x;
	const int j=text_pos[n].y-welt->gib_ij_off().y;
	const int x = (i-j)*(raster/2) + (width/2);
	const int y = (i+j)*(raster/4) +
	              (text_alter[n] >> 3) -
		      welt->lookup_hgt(text_pos[n]);

	if(text_alter[n] >= -80) {

	    display_text(1, x+xoff+1, y+yoff+1, texte[n], SCHWARZ, true);
	    display_text(1, x+xoff, y+yoff, texte[n], kennfarbe+3, true);

	    last_displayed_message = n;
	}
    }

    last_message_index = last_displayed_message;
}


/**
 * Age messages (move them upwards)
 * @author Hj. Malthaner
 */
void
spieler_t::age_messages(long delta_t)
{
  delta_t >>= 3;

  for(int n=0; n<=last_message_index; n++) {
    if(text_alter[n] >= -80) {
      text_alter[n] -= delta_t;
    }
  }
}


void
spieler_t::add_message(koord k, int betrag)
{

    for(int n=0; n<50; n++) {
	if(text_alter[n] <= -80) {
	    text_pos[n] = k;

	    // sprintf(texte[n],"%10.2f",betrag/100.0);
	    money_to_string(texte[n], betrag/100.0);

	    text_alter[n] = 127;

	    if(n > last_message_index) {
		last_message_index = n;
	    }

	    break;
	}
    }
}


/**
 * Wird von welt in kurzen abständen aufgerufen
 * @author Hj. Malthaner
 */
void
spieler_t::step()
{
    steps ++;

    if(automat) {
	do_ki();
    }

    INT_CHECK("simplay 141");

    // die haltestellen müssen die Fahrpläne rgelmaessig pruefen

    int i = steps & 15;
    slist_iterator_tpl <halthandle_t> iter( halt_list );

    while(iter.next()) {

        if( (i & 15) == 0 ) {
	    INT_CHECK("simplay 154");
            iter.get_current()->step();
	    INT_CHECK("simplay 156");
        }
        i++;

    }
    INT_CHECK("simplay 157");
}


/**
 * wird von welt nach jedem monat aufgerufen
 *
 * @author Hj. Malthaner
 */

void
spieler_t::neuer_monat()
{
    static char buf[256];

    // Wartungskosten abziehen
    buche(-maintenance, COST_MAINTENANCE);

    // Bankrott ?
    if(!umgebung_t::freeplay) {
	if(konto < 0) {
	    konto_ueberzogen++;

	    if(this == welt->gib_spieler(0)) {

		if(konto_ueberzogen == 1) {
		    sprintf(buf,
                            translator::translate("Verschuldet:\n\nDu hast %d Monate Zeit,\ndie Schulden zurueckzuzahlen.\n"),
		            MAX_KONTO_VERZUG-konto_ueberzogen+1);

		    create_win(-1, -1,
                               new nachrichtenfenster_t(welt,
                                                        buf),
                               w_autodelete);

		} else if(konto_ueberzogen <= MAX_KONTO_VERZUG) {
		    sprintf(buf,
                            translator::translate("Verschuldet:\n\nDu hast %d Monate Zeit,\ndie Schulden zurueckzuzahlen.\n"),
		            MAX_KONTO_VERZUG-konto_ueberzogen+1);

//			create_win(-1, -1, new nachrichtenfenster_t(welt, "Verschuldet:\n\nDenk daran, rechtzeitig\ndie Schulden zurueckzuzahlen.\n"), w_autodelete);


		    create_win(-1, -1,
                               new nachrichtenfenster_t(welt,
                                                        buf),
                               w_autodelete);


                } else {
		    destroy_all_win();
		    create_win(280, 40, new nachrichtenfenster_t(welt, "Bankrott:\n\nDu bist bankrott.\n"), w_autodelete);
		    //welt->beenden();          // 02-Nov-2001  Markus Weber    beenden has a new parameter
		    welt->beenden(false);
		}
	    }
	} else {
	    konto_ueberzogen = 0;
	}
    }
    roll_finance_history_month();
    calc_finance_history();
}


/**
 * Methode fuer jaehrliche Aktionen
 * @author Hj. Malthaner, Owen Rudge, hsiegeln
 */
void spieler_t::neues_jahr()
{

    roll_finance_history_year();

}

/**
* we need to roll the finance history every year, so that
* the most recent year is at position 0, etc
* @author hsiegeln
*/
void spieler_t::roll_finance_history_month()
{
	for (int i=MAX_HISTORY_MONTHS-1; i>0; i--)
	{
		for (int cost_type = 0; cost_type<MAX_COST; cost_type++)
		{
			finance_history_month[i][cost_type] = finance_history_month[i-1][cost_type];
		}
	}

	for (int i=0;i<MAX_COST;i++)
	{
		finance_history_month[0][i] = 0;
	}
}

void spieler_t::roll_finance_history_year()
{
	for (int i=MAX_HISTORY_YEARS-1; i>0; i--)
	{
		for (int cost_type = 0; cost_type<MAX_COST; cost_type++)
		{
			finance_history_year[i][cost_type] = finance_history_year[i-1][cost_type];
		}
	}

	for (int i=0;i<MAX_COST;i++)
	{
		finance_history_year[0][i] = 0;
	}
}

void spieler_t::calc_finance_history()
{
    /**
    * copy finance data into historical finance data array
    * @author hsiegeln
    */
    sint64 profit, assets, mprofit;
    profit = assets = mprofit = 0;
    for (int i=0; i<MAX_COST; i++)
    {
    // all costs < COST_ASSETS influence profit, so we must sum them up
	if (i < COST_ASSETS)
	{
		profit += finance_history_year[0][i];
		mprofit += finance_history_month[0][i];
	}
    }
    finance_history_year[0][COST_PROFIT] = profit;
    finance_history_month[0][COST_PROFIT] = mprofit;

    slist_iterator_tpl<convoihandle_t> iter (welt->gib_convoi_list());
    while(iter.next()) {
    	convoihandle_t cnv = iter.get_current();
    	if (cnv->gib_besitzer() == this) {
    		assets += cnv->calc_restwert();
    	}
    }
    finance_history_year[0][COST_ASSETS] = assets;
    finance_history_year[0][COST_NETWEALTH] = assets + konto;
    finance_history_year[0][COST_CASH] = konto;
    finance_history_month[0][COST_ASSETS] = assets;
    finance_history_month[0][COST_NETWEALTH] = assets + konto;
    finance_history_month[0][COST_CASH] = konto;

    /*
    char x1[128], y1[128], x2[128], y2[128], y3[128], buffer[128];
    sprintf(x1, "%d", assets);
    sprintf(y1, "%d", finance_history_year[0][COST_ASSETS]);
    sprintf(x2, "%d", konto);
    sprintf(y2, "%d", finance_history_year[0][COST_NETWEALTH]);
    sprintf(y3, "%f", assets + konto);
    sprintf(buffer, "%s, %s, %s, %s, %s", x1, y1, x2, y2, y3);
    dbg->message("draw chart: ", buffer);
    */
}


sint64
spieler_t::buche(const long betrag, const koord pos, const int type)
{
    konto += (sint64)betrag;

    if (type < MAX_COST) {
	// fill year history
	finance_history_year[0][type] += (sint64)betrag;
	finance_history_year[0][COST_PROFIT] += (sint64)betrag;
	finance_history_year[0][COST_CASH] = konto;
	finance_history_year[0][COST_OPERATING_PROFIT] = finance_history_year[0][COST_INCOME] + finance_history_year[0][COST_VEHICLE_RUN] + finance_history_year[0][COST_MAINTENANCE];
	// fill month history
	finance_history_month[0][type] += (sint64)betrag;
	finance_history_month[0][COST_PROFIT] += (sint64)betrag;
	finance_history_month[0][COST_CASH] = konto;
	finance_history_month[0][COST_OPERATING_PROFIT] = finance_history_month[0][COST_INCOME] + finance_history_month[0][COST_VEHICLE_RUN] + finance_history_month[0][COST_MAINTENANCE];
    }

    if(betrag != 0) {
	add_message(pos, betrag);

	if(ABS(betrag) > 10000) {
	    struct sound_info info;

	    info.index = SFX_CASH;
	    info.volume = 255;
	    info.pri = 0;

	    welt->play_sound_area_clipped(pos, info);
	}
    }

    return konto;
}


sint64
spieler_t::buche(long betrag, int type)
{
    konto += (sint64)betrag;

    if (type < MAX_COST) {
	// fill year history
	finance_history_year[0][type] += (sint64)betrag;
	finance_history_year[0][COST_PROFIT] += (sint64)betrag;
	finance_history_year[0][COST_CASH] = konto;
	finance_history_year[0][COST_OPERATING_PROFIT] = finance_history_year[0][COST_INCOME] + finance_history_year[0][COST_VEHICLE_RUN] + finance_history_year[0][COST_MAINTENANCE];
	// fill month history
	finance_history_month[0][type] += (sint64)betrag;
	finance_history_month[0][COST_PROFIT] += (sint64)betrag;
	finance_history_month[0][COST_CASH] = konto;
	finance_history_month[0][COST_OPERATING_PROFIT] = finance_history_month[0][COST_INCOME] + finance_history_month[0][COST_VEHICLE_RUN] + finance_history_month[0][COST_MAINTENANCE];
    }

    return konto;
}


/**
 * Erzeugt eine neue Haltestelle des Spielers an Position pos
 * @author Hj. Malthaner
 */
halthandle_t
spieler_t::halt_add(koord pos)
{
    halthandle_t halt = haltestelle_t::create(welt, pos, this);

    halt_list->insert( halt );

    haltcount ++;
    return halt;
}


/**
 * Entfernt eine Haltestelle des Spielers aus der Liste
 * @author Hj. Malthaner
 */
void
spieler_t::halt_remove(halthandle_t halt)
{
    halt_list->remove( halt );
}


/**
 * Ermittelt, ob an Position i,j eine Haltestelle des Spielers
 * vorhanden ist.
 *
 * @return die gefundene Haltestelle oder NULL wenn keine Haltestelle
 * an Position i,j
 * @author Hj. Malthaner
 */
halthandle_t
spieler_t::ist_halt(const koord k) const
{
    slist_iterator_tpl<halthandle_t> iter( halt_list );

    while(iter.next()) {
	halthandle_t halt = iter.get_current();

	if( halt->ist_da(k) ) {
	    return halt;
	}
    }

    // return unbound handle
    return halthandle_t();
}

// ki-helfer

static const koord platz_offsets[9] =
{
    koord(0,0),
    koord(-1,0),
    koord(1,0),
    koord(0,-1),
    koord(0,1),
    koord(1,1),
    koord(1,-1),
    koord(-1,1),
    koord(-1,-1),
};


bool
spieler_t::suche_platz(int xpos, int ypos, koord *start)
{
    for(int i=0; i<9; i++) {
	const koord pos = koord(xpos, ypos) + platz_offsets[i];

	if(ist_halt(pos).is_bound()) {
	    return false;
	}

	const planquadrat_t * plan = welt->lookup(pos);
	const grund_t * gr = plan ? plan->gib_kartenboden() : 0;

	if(gr &&
	   welt->get_slope(pos) == 0 &&
	   // (gr->ist_natur() || gr->gib_typ() == grund_t::fundament) &&
	   gr->ist_natur() &&
	   gr->kann_alle_obj_entfernen(this) == NULL) {

	    *start = pos;
	    return true;
	}
    }
    return false;
}

bool
spieler_t::suche_platz(int xpos, int ypos, int dx, int dy,
		       koord off, koord *start)
{
    bool ok = true;

    if(dx >= 0 && dy >= 0) {
	ok &= suche_platz(xpos+off.x+1,ypos+off.y+1, start);
    }
    if(dx < 0 && dy >= 0) {
	ok &= suche_platz(xpos-2,ypos+off.y+1, start);
    }
    if(dx >= 0 && dy < 0) {
	ok &= suche_platz(xpos+off.x+1,ypos-2, start);
    }
    if(dx < 0 && dy < 0) {
	ok &= suche_platz(xpos-2,ypos-2, start);
    }

    if(!ok) {
      // Hajo: try harder :)

      if(dx > 0) {
	ok &= suche_platz(xpos+off.x+1,ypos+off.y/2+1, start);
      }
      if(!ok && dx < 0) {
	ok &= suche_platz(xpos-2,ypos+off.y/2+1, start);
      }
      if(!ok && dy < 0) {
	ok &= suche_platz(xpos+off.x/2+1,ypos-2, start);
      }
      if(!ok && dy > 0) {
	ok &= suche_platz(xpos+off.x/2+1,ypos+off.y+1, start);
      }
    }

    return ok;
}

// ki-haupt

void
spieler_t::do_ki()
{
    if((steps & 3) != 0) {
	return;
    }

    switch(state) {
    case NEUE_ROUTE:
	switch(substate) {

	case NR_INIT:
	    gewinn = -1;
	    count = 0;
	    start = NULL;
	    ziel  = NULL;

	    if((40 * -CST_STRASSE) -
		CST_MEDIUM_VEHIKEL*6 -
		CST_FRACHTHOF*2 < konto) {

		substate = NR_SAMMLE_ROUTEN;
	    }
	    break;

	case NR_SAMMLE_ROUTEN:
	    {
		fabrik_t * start_neu = NULL;
		fabrik_t * ziel_neu = NULL;
		int gewinn_neu = suche_transport_quelle_ziel(&start_neu, &ziel_neu);

		if(gewinn_neu > gewinn) {
		    start = start_neu;
		    ziel = ziel_neu;
		    gewinn = gewinn_neu;
		}

		if(gewinn_neu > 5000) {
		    count ++;
		}

		if(count >= 6 && gewinn > 5000 &&
                   start != NULL && ziel != NULL) {
		    substate = NR_BAUE_ROUTE1;
		}
            }
	    break;

	case NR_BAUE_ROUTE1:
	    {
	        dbg->message("spieler_t::do_ki()",
			     "want to build a route from %s (%d,%d) to %s (%d,%d) with value %d",
			     start->gib_name(),
			     start->gib_pos().x, start->gib_pos().y,
			     ziel->gib_name(),
			     ziel->gib_pos().x, ziel->gib_pos().y,
			     gewinn);

		koord zv = start->gib_pos().gib_2d() - ziel->gib_pos().gib_2d();
		int dist = ABS(zv.x) + ABS(zv.y);

		// Eigentliche Rechnung:
		// produktion pro monat mal reisezeit in monaten geteilt durch frachtmenge
		// Bei der Abgabe rechnen wir einen Sicherheitsfaktor mit ein
		count = (int)(((start->max_produktion() - start->gib_abgabe_letzt(0)) * dist) / 2500);

	        dbg->message("spieler_t::do_ki()",
			     "guess to need %d standard vehicles for route",
			     count);

		// wenn wir nahe dran sind, bevorzugen wir lkw
		if(dist < 8 && count > 2) {
		    count = 2;
		}

		// bis 3 fahrzeuge sind lkw günstiger da wir eine lok einsparen
		if(count == 3) {
		    count = 2;
		}

		if(count > 8) {
		    count = 8;
		} else if(count < 1) {
		    count = 1;
		}

		bool ok = suche_platz1_platz2(start, ziel);

		if(!ok) {
		    substate = NR_INIT;
		} else {
		    substate = NR_BAUE_ROUTE2;
		}
	    }
	    break;
	case NR_BAUE_ROUTE2:
	    {
		koord zv = platz2 - platz1;
		int dist = ABS(zv.x) + ABS(zv.y);

		// kosten haben negatives vorzeichen

		if((dist * -CST_STRASSE) -
		    CST_MEDIUM_VEHIKEL*count -
		    CST_FRACHTHOF*2 > konto) {
//			printf("Spieler: nicht genug Geld fuer Strasse+Fahrzeuge\n");

		} else {

		    // Strategie:
		    // Wenn mehr als 3 Fahrzeuge und falls Schienenfzge
		    // erhältlich dann schiene sonst strasse


		    substate = NR_INIT; // falls gar nichts klappt gehen wir
                                        // zum initialzustand zurueck


		    if(count > 3 &&
		       vehikelbauer_t::gib_info(start->gib_ausgang()->get(0).gib_typ(),
                                                          vehikel_besch_t::schiene,
							  0)) {
			substate = NR_BAUE_SCHIENEN_ROUTE1;
		    } else {

			if(vehikelbauer_t::gib_info(start->gib_ausgang()->get(0).gib_typ(),
                                                              vehikel_besch_t::strasse,
							      1)) {
			    substate = NR_BAUE_STRASSEN_ROUTE;
			}
		    }
		}

	    }
	    break;
	case NR_BAUE_SCHIENEN_ROUTE1:
	    {
		bool ok = create_complex_rail_transport();

		if(ok) {
		    substate = NR_BAUE_BAHNHOF;
		} else {
		    substate = NR_BAUE_SCHIENEN_ROUTE2;
		}
	    }
	    break;

	case NR_BAUE_SCHIENEN_ROUTE2:
	    {
		const koord tmp = platz2;
		platz2 = platz1;
		platz1 = tmp;

		bool ok = create_complex_rail_transport();

		if(ok) {
		    substate = NR_BAUE_BAHNHOF;
		} else {
		    // Test, ob es Strassenfahrzeuge für die Fracht gibt
		    if(vehikelbauer_t::gib_info(start->gib_ausgang()->get(0).gib_typ(),
                                                          vehikel_besch_t::strasse,
							  1)) {
			substate = NR_BAUE_STRASSEN_ROUTE;
		    } else {
			// gibt es nicht, neu anfangen
			substate = NR_INIT;
		    }
		}
	    }
	    break;

	case NR_BAUE_BAHNHOF:
	    {
		int l1 = baue_bahnhof(platz1, count);
		int l2 = baue_bahnhof(platz2, count);

		count = MIN(l1, l2);
		substate = NR_BAUE_SCHIENEN_VEHIKEL;
	    }
	    break;

	case NR_BAUE_SCHIENEN_VEHIKEL:
	    create_rail_transport_vehikel(start, count);
	    substate = NR_INIT;
	    break;

	case NR_BAUE_STRASSEN_ROUTE:
	    {
		bool ok = create_simple_road_transport();

		if(ok) {
		    substate = NR_BAUE_STRASSEN_VEHIKEL;
		} else {
		    substate = NR_BAUE_STRASSEN_ROUTE2;
		}

	    }
	    break;

	case NR_BAUE_STRASSEN_ROUTE2:
	    {
		bool ok = create_complex_road_transport();

		if(ok) {
		    substate = NR_BAUE_STRASSEN_VEHIKEL;
		} else {
		    // schilder aufraeumen
		    if(welt->lookup(platz1)) {
			welt->access(platz1)->gib_kartenboden()->obj_loesche_alle(this);
		    }
		    if(welt->lookup(platz2)) {
			welt->access(platz2)->gib_kartenboden()->obj_loesche_alle(this);
		    }
		}

		substate = NR_INIT;
	    }
	    break;

	case NR_BAUE_STRASSEN_VEHIKEL:
	    create_road_transport_vehikel(start, count);
	    substate = NR_INIT;
            break;

	default:
	  dbg->error("spieler_t::do_ki()",
		     "State %d with illegal substate %d!",
		     state, substate);
	}
	break;

    default:
        dbg->error("spieler_t::do_ki()",
		   "Illegal state %d with substate %d!",
		   state, substate);
    }
}


bool
spieler_t::checke_bahnhof_bau(koord)
{
    return true;
}

int
spieler_t::baue_bahnhof(koord &p, int anz_vehikel)
{
    int laenge = MAX((anz_vehikel+1)/2, 2);       // aufrunden!
    koord t = p;

    int baulaenge = 0;
    ribi_t::ribi ribi = welt->lookup(p)->gib_kartenboden()->gib_weg_ribi(weg_t::schiene);
    int i;

    koord zv ( ribi );

    zv.x = -zv.x;
    zv.y = -zv.y;

    dbg->message("spieler_t::baue_bahnhof()",
		 "building a train station of length %d (%d vehicles)",
		 laenge, anz_vehikel);

    INT_CHECK("simplay 548");

    const int hoehe = welt->lookup(t)->gib_kartenboden()->gib_hoehe();

    t = p;
    for(i=0; i<laenge-1; i++) {
	t += zv;
        welt->ebne_planquadrat(t, hoehe);
    }

    t = p;
    bool ok = true;

    for(i=0; i<laenge-1; i++) {
	t += zv;

	const planquadrat_t *plan = welt->lookup(t);
	grund_t *gr = plan ? plan->gib_kartenboden() : 0;

	ok &= gr != NULL &&
              (gr->ist_natur() || gr->gib_typ() == grund_t::fundament) &&
	      gr->kann_alle_obj_entfernen(this) == NULL &&
	      gr->gib_grund_hang() == hang_t::flach;


        if(ok) {
	  // gr->obj_loesche_alle(this);
	  wkz_remover(this, welt, t);

	} else {

	    const planquadrat_t *plan = welt->lookup(p-zv);
	    gr = plan ? plan->gib_kartenboden(): 0;

	    // wenns nicht mehr vorwaerts geht, dann vielleicht zurueck ?

	    ok = gr != NULL &&
	         gr->gib_weg(weg_t::schiene) != NULL &&
		 gr->gib_weg_ribi(weg_t::schiene) == ribi_t::doppelt(ribi) &&
		 gr->kann_alle_obj_entfernen(this) == NULL &&
		 welt->get_slope(p-zv) == 0;
	    if(ok) {
	      gr->obj_loesche_alle(this);
	      p -= zv;
	    }

	    t -= zv;
	}
    }

    INT_CHECK("simplay 593");

    wegbauer_t bauigel(welt, this);
    bauigel.baubaer = false;

    INT_CHECK("simplay 599");

    const weg_besch_t * besch = wegbauer_t::gib_besch("wooden_sleeper_track");
    bauigel.route_fuer(wegbauer_t::schiene, besch);

    bauigel.calc_route(p, t);
    bauigel.baue();

    // ziel auf das ende des Bahnhofs setzen
    // p wurde als referenz übergeben !!!

    t += zv;

    do {
	wkz_bahnhof(this, welt, p, hausbauer_t::gueterbahnhof_besch);

	p += zv;
	baulaenge ++;

	INT_CHECK("simplay 753");
    } while(p != t);

    p -= zv;

    return baulaenge*2;
}


int
spieler_t::suche_transport_quelle_ziel(fabrik_t **quelle, fabrik_t **ziel)
{
    fabrik_t *qfab = welt->get_random_fab();
    int gewinn = -1;

    // geeignete Quelle gefunden ?

    if(qfab != NULL) {

	const vector_tpl<ware_t> *ausgang = qfab->gib_ausgang();

	// ist es ein erzeuger ?
	if(ausgang->get_count() == 0) {
	    // nein
	    return -1;
	}

	const ware_t ware = ausgang->get(0);

	// hat es schon etwas produziert ?
	if(ware.menge < ware.max/8) {
	    // zu wenig vorrat für profitable strecke
	    return -1;
	}

	const vector_tpl <koord> & lieferziele = qfab->gib_lieferziele();

	if(lieferziele.get_count() == 0) {
	    // kein lieferziel, kein transport
	    return -1;
	}


	// XXX KI sollte auch andere Lieferziele prüfen!
	const koord lieferziel = lieferziele.get(0);


	fabrik_t *zfab = NULL;

	if(welt->lookup(lieferziel)) {
	    ding_t * dt = welt->lookup(lieferziel)->gib_kartenboden()->obj_bei(1);
	    if(dt) {
		zfab = dt->fabrik();
	    }
	}

	// more checks
	if(ware.gib_typ() != warenbauer_t::nichts &&
	   zfab != NULL && zfab->verbraucht(ware.gib_typ()) > 0) {

            // schaetze gewinn
	    // wenn andere fahrzeuge genutzt werden muss man die werte anpassen

	    const int dist = abs(zfab->gib_pos().x - qfab->gib_pos().x) + abs(zfab->gib_pos().y - qfab->gib_pos().y);
	    const int speed = 512;                  // LKW
	    const int grundwert = ware.gib_preis();

	    if( dist > 6) {                          // sollte vernuenftige Entfernung sein

		// wie viel koennen wir tatsaechlich transportieren
		// Bei der abgabe rechnen wir einen sicherheitsfaktor 2 mit ein
		// wieviel kann am ziel verarbeitet werden, und wieviel gibt die quelle her?

		const int prod = MIN(zfab->max_produktion(),
	                             qfab->max_produktion() - qfab->gib_abgabe_letzt(0));

		// pow(0.95, dist) statt pow(0.97, dist)
		// weil auf kuerzeren Strecken
		// die Taktraten hoeher sind.
		gewinn = (int)(grundwert * dist * (speed/1024.0) * pow(0.95, dist) * prod);

		// verlust durch fahrtkosten, geschätze anzhal vehikel ist fracht/16
		gewinn -= (int)(dist * 16 * (abs(prod)/16));   // dist * steps/planquad * kosten
	    }

	    if(gewinn > 5000) {
//		    printf("Spieler: passendes Fabrikpaar fuer transport gefunden\n");
//		    printf("Spieler: Entfernung %d\n", dist);
//		    printf("Spieler: Schaetze Gewinn auf %d\n", gewinn);

	        *quelle = qfab;
		*ziel = zfab;
	    }
	}
    }

    return gewinn;
}

void
spieler_t::create_road_transport_vehikel(fabrik_t *qfab, int anz_vehikel)
{
    if(hausbauer_t::frachthof_besch) {
	wkz_frachthof(this, welt, platz1);
	wkz_frachthof(this, welt, platz2);

	fahrplan_t *fpl;

	if(anz_vehikel > 2) {
	    anz_vehikel = 2;
	}


	int i = 0;

	while(i < anz_vehikel) {
	    const koord3d pos1 ( welt->lookup(platz1)->gib_kartenboden()->gib_pos() );
	    const koord3d pos2 ( welt->lookup(platz2)->gib_kartenboden()->gib_pos() );


	    const vehikel_besch_t * besch = vehikelbauer_t::gib_info(qfab->gib_ausgang()->get(0).gib_typ(),
								vehikel_besch_t::strasse,
								1);

	    vehikel_t * v = vehikelbauer_t::baue(welt, pos2, this,
					       NULL, besch);

	    convoi_t *cnv = new convoi_t(welt, this);
	    // V.Meyer: give the new convoi name from first vehicle
	    cnv->setze_name(v->gib_besch()->gib_name());
	    cnv->add_vehikel( v );

	    fpl = cnv->gib_vehikel(0)->erzeuge_neuen_fahrplan();

	    fpl->aktuell = 0;
	    fpl->maxi = 1;
	    fpl->eintrag.at(0).pos = pos1;
	    fpl->eintrag.at(0).ladegrad = 0;
	    fpl->eintrag.at(1).pos = pos2;
	    fpl->eintrag.at(1).ladegrad = 0;

	    welt->sync_add( cnv );
	    cnv->start();
	    cnv->setze_fahrplan(fpl);

	    const koord tmp = platz2;
	    platz2 = platz1;
	    platz1 = tmp;

	    i++;
	}
    }
}

static const int lok_laenge_tab[8] =	// power values
{
    240,
    240,

    990,
    990,

    2060,
    2060,
    2060,

    1670   // Faster but less power. Good decision?
};

void
spieler_t::create_rail_transport_vehikel(fabrik_t *qfab, int anz_vehikel)
{
    fahrplan_t *fpl;
    convoi_t *cnv = new convoi_t(welt, this);
    const koord3d pos1 ( welt->lookup(platz1)->gib_kartenboden()->gib_pos() );
    const koord3d pos2 ( welt->lookup(platz2)->gib_kartenboden()->gib_pos() );

    // lokomotive bauen
    const vehikel_besch_t *besch = vehikelbauer_t::vehikel_fuer_leistung(lok_laenge_tab[MIN(7, anz_vehikel-2)], vehikel_besch_t::schiene);
    vehikel_t * v = vehikelbauer_t::baue(welt, pos2, this, NULL, besch);
    // V.Meyer: give the new convoi name from first vehicle
    cnv->setze_name(besch->gib_name());
    cnv->add_vehikel( v );

    for(int i = 0; i < anz_vehikel-1; i++) {
	const vehikel_besch_t * besch = vehikelbauer_t::gib_info(qfab->gib_ausgang()->get(0).gib_typ(),
                                                            vehikel_besch_t::schiene,
							    0);

	vehikel_t * v = vehikelbauer_t::baue(welt, pos2, this,
	                                   NULL,
					   besch);

	cnv->add_vehikel( v );
    }

    fpl = cnv->gib_vehikel(0)->erzeuge_neuen_fahrplan();

    fpl->aktuell = 0;
    fpl->maxi = 1;
    fpl->eintrag.at(0).pos = pos1;
    fpl->eintrag.at(0).ladegrad = 0;
    fpl->eintrag.at(1).pos = pos2;
    fpl->eintrag.at(1).ladegrad = 0;


    const int d1 = abs(qfab->pos.x - pos1.x) + abs(qfab->pos.y - pos1.y);
    const int d2 = abs(qfab->pos.x - pos2.x) + abs(qfab->pos.y - pos2.y);

    if(d1 < d2) {
      fpl->eintrag.at(0).ladegrad = 100;
    }
    if(d2 < d1) {
      fpl->eintrag.at(1).ladegrad = 100;
    }

    cnv->setze_fahrplan(fpl);

    welt->sync_add( cnv );
    cnv->start();
}

bool
spieler_t::suche_platz1_platz2(fabrik_t *qfab, fabrik_t *zfab)
{
    dbg->message("spieler_t::suche_platz1_platz2()",
		 "searching locations for stops/stations");

    const koord3d start ( qfab->gib_pos() );
    const koord3d ziel ( zfab->gib_pos() );

    bool ok = true;

    INT_CHECK("simplay 961");

    ok &= suche_platz(start.x, start.y,
		      ziel.x-start.x, ziel.y-start.y,
		      qfab->gib_groesse(),
		      &platz1);

    INT_CHECK("simplay 965");

    ok &= suche_platz(ziel.x, ziel.y,
		      start.x-ziel.x, start.y-ziel.y,
		      zfab->gib_groesse(),
		      &platz2);

    INT_CHECK("simplay 969");

    if( !ok ) {   // keine Bauplaetze gefunden
        dbg->message("spieler_t::suche_platz1_platz2()",
	             "no suitable locations found");
    } else {
	// plaetz reservieren
	zeiger_t *z;
	grund_t *gr;

        dbg->message("spieler_t::suche_platz1_platz2()",
	             "platz1=%d,%d platz2=%d,%d",
		     platz1.x, platz1.y, platz1.x, platz1.y);

	gr = welt->access(platz1)->gib_kartenboden();
	z = new zeiger_t(welt, gr->gib_pos(), this);
	z->setze_bild(0, skinverwaltung_t::belegtzeiger->gib_bild_nr(0));
	gr->obj_add( z );


	gr = welt->access(platz2)->gib_kartenboden();
	z = new zeiger_t(welt, gr->gib_pos(), this);
	z->setze_bild(0, skinverwaltung_t::belegtzeiger->gib_bild_nr(0));
	gr->obj_add( z );

    }
    return ok;
}


bool
spieler_t::create_simple_road_transport()
{
    INT_CHECK("simplay 837");

    wegbauer_t bauigel(welt, this);

    const weg_besch_t * besch = wegbauer_t::gib_besch("asphalt_road");
    bauigel.route_fuer( wegbauer_t::strasse, besch );
    bauigel.baubaer = true;

    INT_CHECK("simplay 846");

    bauigel.calc_route(platz1,platz2);

    // Strasse muss min. 3 Felder lang sein, sonst kann man keine
    // zwei verschiedene stops bauen

    if(bauigel.max_n > 1) {
        dbg->message("spieler_t::create_simple_road_transport()",
		     "building simple road from %d,%d to %d,%d",
		     platz1.x, platz1.y, platz2.x, platz2.y);
	bauigel.baue();
	return true;
    } else {
        dbg->message("spieler_t::create_simple_road_transport()",
		     "building simple road from %d,%d to %d,%d failed",
		     platz1.x, platz1.y, platz2.x, platz2.y);
	return false;
    }
}

bool
spieler_t::create_complex_road_transport()
{
    dbg->message("spieler_t::create_complex_road_transport()",
		 "try to build complex road");

    INT_CHECK("simplay 865");

    wegbauer_t bauigel(welt, this);

    INT_CHECK("simplay 867");

    bauigel.baubaer = true;
    bauigel.route_fuer( wegbauer_t::strasse_bot, 0 );

    bauigel.calc_route(platz1,platz2);

    INT_CHECK("simplay 768");

    // Strasse muss min. 3 Felder lang sein, sonst kann man keine
    // zwei verschiedene stops bauen

    if(bauigel.max_n <= 3) {
        dbg->message("spieler_t::create_complex_road_transport()",
		     "no suitable route was found, aborting roadbuilding");
	return false;
    }


    slist_tpl <koord> list;

    list.insert( platz1 );

    for(int i=1; i<bauigel.max_n-1; i++) {
	koord p = bauigel.gib_route_bei(i);
        grund_t *bd = welt->lookup(p)->gib_kartenboden();


        if(bd->gib_weg(weg_t::schiene) != NULL) {
            dbg->message("spieler_t::create_complex_road_transport()",
	                 "try to cross railroad track");

	    ribi_t::ribi ribi = bd->gib_weg_ribi(weg_t::schiene);

	    const bool ok = versuche_brueckenbau(p, i, ribi, bauigel, list);

	    if(!ok) {
	      dbg->message("spieler_t::create_complex_road_transport()",
			   "bridge failed, aborting roadbuilding");
	      return false;
	    } else {
	      dbg->message("spieler_t::create_complex_road_transport()",
			   "using bridge");
	    }
	}
    }

    INT_CHECK("simplay 895");

    const weg_besch_t * besch = wegbauer_t::gib_besch("asphalt_road");
    bauigel.route_fuer( wegbauer_t::strasse, besch );

    list.insert( platz2 );

    bauigel.baubaer = true;

    if(!checke_streckenbau(bauigel, list)) {
        dbg->message("spieler_t::create_complex_road_transport()",
		     "checke_streckenbau() returned false, aborting roadbuilding");
	return false;
    }

    baue_strecke(bauigel, list);

    slist_iterator_tpl <koord> iter (list);

    if(iter.next()) {

      while(iter.next()) {
	const koord k = iter.get_current();

	if(!iter.next()) {
	  break;
	}

	brueckenbauer_t::baue(this, welt, k, weg_t::schiene);
      }
    }
    return true;
}



bool
spieler_t::create_complex_rail_transport()
{
    dbg->message("spieler_t::create_complex_rail_transport()",
		 "try to build complex railroads");

    INT_CHECK("simplay 957");

    wegbauer_t bauigel(welt, this);

    INT_CHECK("simplay 961");

    bauigel.baubaer = false;
    bauigel.route_fuer( wegbauer_t::schiene_bot, 0 );

    bauigel.calc_route(platz1, platz2);

    INT_CHECK("simplay 968");

    // Strecke muss min. 3 Felder lang sein, sonst kann man keine
    // zwei verschiedene stops bauen

    if(bauigel.max_n < 3) {
        dbg->message("spieler_t::create_complex_rail_transport()",
		     "no suitable route was found, aborting trackbuilding");

	return false;
    }

    slist_tpl <koord> list;

    list.insert( platz1 );

    for(int i=1; i<bauigel.max_n-1; i++) {
	koord p = bauigel.gib_route_bei(i);
        grund_t *bd = welt->lookup(p)->gib_kartenboden();


	if(bd->gib_weg(weg_t::strasse) != NULL || bd->gib_weg(weg_t::schiene) != NULL) {
            dbg->message("spieler_t::create_complex_rail_transport()",
	                 "try to cross way");

	    ribi_t::ribi ribi = 0;

	    if(bd->gib_weg(weg_t::schiene)) {
	      ribi = bd->gib_weg_ribi(weg_t::schiene);
	    } else {
	      ribi = bd->gib_weg_ribi(weg_t::strasse);
	    }

	    const bool ok = versuche_brueckenbau(p, i, ribi, bauigel, list);

	    if(!ok) {
	      dbg->message("spieler_t::create_complex_rail_transport()",
			   "bridge failed, aborting trackbuilding");

	      return false;
	    } else {
	      dbg->message("spieler_t::create_complex_rail_transport()",
			   "using bridge");
	    }
	}
    }

    INT_CHECK("simplay 980");

    list.insert( koord(platz2) );

    const weg_besch_t * besch = wegbauer_t::gib_besch("wooden_sleeper_track");
    bauigel.route_fuer( wegbauer_t::schiene_bot_bau, besch );
    bauigel.baubaer = true;

    if(!checke_streckenbau(bauigel, list)) {
        dbg->message("spieler_t::create_complex_rail_transport()",
		     "checke_streckenbau() returned false, aborting trackbuilding");
	return false;
    }

    baue_strecke(bauigel, list);

    slist_iterator_tpl <koord> iter (list);

    if(iter.next()) {

      while(iter.next()) {
	const koord k = iter.get_current();

	if(!iter.next()) {
	  break;
	}

	brueckenbauer_t::baue(this, welt, k, weg_t::schiene);
      }
    }
    return true;
}


bool
spieler_t::checke_streckenbau(wegbauer_t &bauigel, slist_tpl<koord> &list)
{
    INT_CHECK("simplay 1034");

    bool ok = true;
    slist_iterator_tpl <koord> iter (list);

    while(iter.next()) {
	const koord k1 = iter.get_current();

	iter.next();
	const koord k2 = iter.get_current();

	bauigel.calc_route(k1, k2);

	if(bauigel.max_n <= 0) {
	    ok = false;
            break;
	}
    }

    return ok;
}


void
spieler_t::baue_strecke(wegbauer_t &bauigel,
                        slist_tpl <koord> &list)
{
    INT_CHECK("simplay 1064");

    slist_iterator_tpl <koord> iter (list);

    while(iter.next()) {
	const koord k1 = iter.get_current();

	iter.next();
	const koord k2 = iter.get_current();

	bauigel.calc_route(k1, k2);
	bauigel.baue();

	INT_CHECK("simplay 974");
    }
}



/**
 * Test if a bridge can be build here
 *
 * @param p position of obstacle
 * @param index index in route where obstacle was hit
 * @param ribi ribi of obstacle
 * @param bauigel the waybilder used to build the way
 * @param list list of bridge start/end koordinates
 * @author Hj. Malthaner
 */
bool
spieler_t::versuche_brueckenbau(koord p, int index, ribi_t::ribi ribi,
                                wegbauer_t &bauigel,
				slist_tpl <koord> &list)
{
  bool ok = true;

  const koord p1 = bauigel.gib_route_bei(index-1);
  const koord p2 = bauigel.gib_route_bei(index+1);

  koord zv = p2 - p1;


  if((zv.x != 0 && zv.y != 0) ||     // Hajo: only straight bridges possible
     welt->get_slope(p1) != 0 || // Hajo: ends on flat ground
     welt->get_slope(p2) != 0) {
    ok = false;    // Hajo: only straight bridges possible
  } else {
    // Hajo: Bridge might be possible, check if
    // way ends are straight.

    zv.x = sgn(zv.x);
    zv.y = sgn(zv.y);

    /*
    printf("Problem bei %d %d Zielvektor %d %d\n", p.x, p.y, zv.x, zv.y);
    printf("Brucke von %d %d nach %d %d\n", p1.x, p1.y, p2.x, p2.y);
    */


    if(index < 2 || index > bauigel.max_n-2) {
      ok = false;
    } else {

      ok &= bauigel.gib_route_bei(index-2) == p1 - zv;
      ok &= bauigel.gib_route_bei(index+2) == p2 + zv;

      if(ok) {
	list.insert( p1 );
	list.insert( p2 );
      }
    }
  }


  return ok;
}


void
spieler_t::create_bussing()
{
/*
    int n = simrand( ANZ_STADT );

    koord liob = welt->stadt[n]->get_linksoben();
    koord reun = welt->stadt[n]->get_rechtsunten();

    for(int j=liob.y; j <= reun.y; j++) {
	for(int i=liob.x; i <= reun.x; i++) {

	}
    }
*/
}


/**
 * Speichert Zustand des Spielers
 * @param file Datei, in die gespeichert wird
 * @author Hj. Malthaner
 */
void
spieler_t::rdwr(loadsave_t *file)
{
    int halt_count;
    int start_index;
    int ziel_index;

    if(file->is_saving()) {
        halt_count = halt_list->count();
        start_index = welt->gib_fab_index(start);
        ziel_index = welt->gib_fab_index(ziel);
    }
    file->rdwr_delim("Sp ");
    file->rdwr_longlong(konto, " ");
    file->rdwr_int(konto_ueberzogen, " ");
    file->rdwr_int(steps, " ");
    file->rdwr_int(kennfarbe, " ");
    file->rdwr_int(halt_count, " ");
    // @author hsiegeln
    if (file->get_version() < 82003)
    {
        haltcount = 0;
    } else {
        file->rdwr_int(haltcount, " ");
        if (haltcount < 0) haltcount = 0;
    }
    if (file->get_version() < 82003)
    {
    	file->rdwr_longlong(finances[0], " ");
		file->rdwr_longlong(finances[1], " ");
		file->rdwr_longlong(finances[2], " ");
		file->rdwr_longlong(finances[3], " ");
		file->rdwr_longlong(finances[4], " ");
		file->rdwr_longlong(old_finances[0], " ");
		file->rdwr_longlong(old_finances[1], " ");
		file->rdwr_longlong(old_finances[2], " ");
		file->rdwr_longlong(old_finances[3], "\n");
		file->rdwr_longlong(old_finances[4], " ");
		/**
		* initialize finance data structures
		* @author hsiegeln
		*/
		for (int year = 0;year<MAX_HISTORY_YEARS;year++)
		{
			for (int cost_type = 0; cost_type<MAX_COST; cost_type++)
			{
				finance_history_year[year][cost_type] = 0;
			}
		}

		for (int month = 0;month<MAX_HISTORY_MONTHS;month++)
		{
			for (int cost_type = 0; cost_type<MAX_COST; cost_type++)
			{
				finance_history_month[month][cost_type] = 0;
			}
		}

		/**
		* old datastructures have less entries, so we only migrate values for the first 5 elements!
		* others will be set to zero
		* @author hsiegeln
		*/

		for (int cost_type = 0; cost_type<MAX_COST; cost_type++)
		{
			if (cost_type < 5)
			{
				finance_history_year[0][cost_type] = finances[cost_type];
				finance_history_year[1][cost_type] = old_finances[cost_type];
			} else {
				finance_history_year[0][cost_type] = 0;
				finance_history_year[1][cost_type] = 0;
			}
		}

    } else if (file->get_version() < 84008) {
		for (int year = 0;year<MAX_HISTORY_YEARS;year++)
		{
			for (int cost_type = 0; cost_type<MAX_COST; cost_type++)
			{
				if (file->get_version() < 84007) {
					// a cost_type has has been added. For old savegames we only have 9 cost_types, now we have 10.
					// for old savegames only load 9 types and calculate the 10th; for new savegames load all 10 values
					if (cost_type < 9) {
						file->rdwr_longlong(finance_history_year[year][cost_type], " ");
					} else {
						finance_history_year[year][cost_type] = finance_history_year[year][COST_INCOME] + finance_history_year[year][COST_VEHICLE_RUN] + finance_history_year[year][COST_MAINTENANCE];
					}
				} else {
						file->rdwr_longlong(finance_history_year[year][cost_type], " ");
				}
				//dbg->message("player_t::rdwr()", "finance_history[year=%d][cost_type=%d]=%ld", year, cost_type,finance_history_year[year][cost_type]);
			}
		}

    } else {
		for (int year = 0;year<MAX_HISTORY_YEARS;year++)
		{
			for (int cost_type = 0; cost_type<MAX_COST; cost_type++)
			{
				file->rdwr_longlong(finance_history_year[year][cost_type], " ");
			}
		}

		// in 84008 monthly finance history was introduced
		for (int month = 0;month<MAX_HISTORY_MONTHS;month++)
		{
			for (int cost_type = 0; cost_type<MAX_COST; cost_type++)
			{
				file->rdwr_longlong(finance_history_month[month][cost_type], " ");
			}
		}
    }

    file->rdwr_bool(automat, "\n");
    file->rdwr_enum(state, " ");
    file->rdwr_enum(substate, "\n");
    file->rdwr_int(start_index, " ");
    file->rdwr_int(ziel_index, "\n");
    file->rdwr_int(gewinn, " ");
    file->rdwr_int(count, "\n");
    platz1.rdwr( file );
    platz2.rdwr( file );

    // Hajo: sanity checks
    if(halt_count < 0) {
      dbg->fatal("spieler_t::rdwr()", "Halt count is out of bounds: %d -> corrupt savegame?", halt_count);
    }
    if(haltcount < 0) {
      dbg->fatal("spieler_t::rdwr()", "Haltcount is out of bounds: %d -> corrupt savegame?", haltcount);
    }
    if(state < NEUE_ROUTE || state > NEUE_ROUTE) {
      dbg->fatal("spieler_t::rdwr()", "State is out of bounds: %d -> corrupt savegame?", state);
    }
    if(substate < NR_INIT || substate > NR_BAUE_STRASSEN_VEHIKEL) {
      dbg->fatal("spieler_t::rdwr()", "Substate is out of bounds: %d -> corrupt savegame?", substate);
    }
    if(start_index < -1) {
      dbg->fatal("spieler_t::rdwr()", "Start index is out of bounds: %d -> corrupt savegame?", start_index);
    }
    if(ziel_index < -1) {
      dbg->fatal("spieler_t::rdwr()", "Ziel index is out of bounds: %d -> corrupt savegame?", ziel_index);
    }


    if(file->is_saving()) {
        slist_iterator_tpl<halthandle_t> iter (halt_list);

        while(iter.next()) {
    	    iter.get_current()->rdwr( file );
        }
    }
    else {
        if(start_index != -1) {
	    start = welt->gib_fab(start_index);
        } else {
	    start = NULL;
        }

        if(ziel_index != -1) {
	    ziel = welt->gib_fab(ziel_index);
        } else {
	    ziel = NULL;
        }
        for(int i=0; i<halt_count; i++) {
	    halthandle_t halt = haltestelle_t::create( welt, file );

	    halt->laden_abschliessen();

	    halt_list->insert( halt );
        }
        init_texte();
    }
}

/**
 * Returns the amount of money for a certain finance section
 *
 * @author Owen Rudge
 */
sint64
spieler_t::get_finance_info(int type)
{
    if (type == COST_CASH) {
	return konto;
//    } else if (type < MAX_COST) {
    } else {
	// return finances[type];
	return finance_history_year[0][type];
    }

    return 0;
}

/**
 * Returns the amount of money for a certain finance section from previous year
 *
 * @author Owen Rudge
 */
sint64
spieler_t::get_finance_info_old(int type)
{
        return finance_history_year[1][type];

    if (type == COST_CASH) {
	return konto;
//    } else if (type < MAX_COST) {
    } else {
//	return old_finances[type];
        return finance_history_year[1][type];
    }

    return 0;
}


/**
 * Rückruf, um uns zu informieren, dass eine Station voll ist
 * @author Hansjörg Malthaner
 * @date 25-Nov-2001
 */
void spieler_t::bescheid_station_voll(halthandle_t halt)
{
    if(!automat) {
	char buf[128];

	sprintf(buf, translator::translate("!0_STATION_CROWDED"), halt->gib_name());

        ticker_t::get_instance()->add_msg(buf,
					  halt->gib_basis_pos(),
					  DUNKELROT);
    }
}

/**
 * Rückruf, um uns zu informieren, dass ein Vehikel ein Problem hat
 * @author Hansjörg Malthaner
 * @date 26-Nov-2001
 */
void spieler_t::bescheid_vehikel_problem(convoihandle_t cnv)
{
    if(!automat) {
 	char buf[256];

	sprintf(buf,"Vehicle %s can't find a route!", cnv->gib_name());

        ticker_t::get_instance()->add_msg(buf,
					  cnv->gib_pos().gib_2d(),
					  ROT);
    } else {
      welt->rem_convoi(cnv);
      cnv->destroy();
      cnv = 0;
    }
}

/**
 * Gets haltcount
 * @author hsiegeln
 */
int spieler_t::get_haltcount() const
{
	return haltcount;
}
