/*
 * simplay.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 *
 * Renovation in dec 2004 for other vehicles, timeline
 * @author prissi
 */

/* number of lines to check (must be =>3 */
#define KI_TEST_LINES 10
/* No lines will be considered below */
#define KI_MIN_PROFIT 5000
// this does not work well together with 128er pak
//#ifdef EARLY_SEARCH_FOR_BUYER
// this make KI2 try passenger transport
#define TRY_PASSENGER


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "simdebug.h"

#include "besch/grund_besch.h"
#include "boden/boden.h"

#include "boden/grund.h"
#include "boden/wege/weg.h"
#include "boden/wege/strasse.h"
#include "boden/wege/schiene.h"
#include "simskin.h"
#include "besch/skin_besch.h"
#include "besch/weg_besch.h"
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
#include "simmesg.h"

#include "simintr.h"

#include "simimg.h"
#include "simcolor.h"

#include "simwin.h"
#include "gui/messagebox.h"

#include "dings/zeiger.h"
#include "dings/gebaeude.h"

#include "dataobj/fahrplan.h"
#include "dataobj/umgebung.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"
#include "dataobj/einstellungen.h"

#include "bauer/vehikelbauer.h"
#include "bauer/warenbauer.h"
#include "bauer/brueckenbauer.h"
#include "bauer/wegbauer.h"

#include "blockmanager.h"	// electrify tracks

#include "utils/simstring.h"

#include "simgraph.h"

#ifndef tpl_slist_tpl_h
#include "tpl/slist_tpl.h"
#endif

#include "gui/money_frame.h"

#include "simcity.h"

// true, if line contruction is under way
// our "semaphore"
static bool baue;
static fabrik_t *baue_start=NULL;
static fabrik_t *baue_ziel=NULL;




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
				if ((cost_type == COST_CASH) || (cost_type == COST_NETWEALTH)) {
					finance_history_year[year][cost_type] = umgebung_t::starting_money;
				}
      }
    }

    for (int month=0; month<MAX_HISTORY_MONTHS; month++) {
      for (int cost_type=0; cost_type<MAX_COST; cost_type++) {
				finance_history_month[month][cost_type] = 0;
				if ((cost_type == COST_CASH) || (cost_type == COST_NETWEALTH)) {
					finance_history_month[month][cost_type] = umgebung_t::starting_money;
				}
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
    last_start = NULL;
    last_ziel = NULL;

	rail_engine = NULL;
	rail_vehicle = NULL;
	rail_weg = NULL;
	road_vehicle = NULL;
	road_weg = NULL;
	baue = false;

    steps = simrand(16);

    init_texte();

	// UNDO array empty
	last_built = NULL;

	money_frame = NULL;

	// we have different AI, try to find out our type:
	int spieler_num=kennfarbe/4;
	sprintf(spieler_name_buf,"player %i",spieler_num-1);
	if(spieler_num>=2) {
		// @author prissi
		// first set default
		passenger_transport = false;
		road_transport = true;
		rail_transport = true;
		ship_transport = false;
		// set the different AI
		switch(spieler_num) {
			case 2:
				rail_transport = false;
				break;
			case 7:
				road_transport = false;
				break;
#ifdef TRY_PASSENGER
			case 3:
				passenger_transport = true;
				start_stadt = end_stadt = NULL;
				end_ausflugsziel = NULL;
			break;
#endif
		}
		// the other behave normal
	}
}


/**
 * Destruktor
 * @author Hj. Malthaner
 */
spieler_t::~spieler_t()
{
	// maybe free money frame
	if(money_frame!=NULL) {
		delete money_frame;
	}
	money_frame = NULL;
	// maybe free undo list
	if(last_built!=NULL) {
		delete last_built;
	}
	last_built = NULL;
	// always there
	delete halt_list;
	halt_list = 0;
}




/* returns the money dialoge of a player
 * @author prissi
 */
money_frame_t *
spieler_t::gib_money_frame(void)
{
	if(money_frame==NULL) {
		money_frame = new money_frame_t(this);
	}
	return money_frame;
}



/* returns the name of the player; "player -1" sits in front of the screen
 * @author prissi
 */
const char *
spieler_t::gib_name(void)
{
	return translator::translate(spieler_name_buf);
}



/* Activates/deactivates a player
 * @author prissi
 */
bool
spieler_t::set_active(bool new_state)
{
	// something to change?
	if(automat!=new_state) {

		if(!new_state) {
			// deactivate AI
			automat = false;
			if(baue  &&  state>=NEUE_ROUTE  &&  state<CHECK_CONVOI  &&  substate>=NR_BAUE_ROUTE1) {
				// deactivate semaphore, so other could do construction
				baue = false;
			    state = NEUE_ROUTE;
			    substate = NR_INIT;
				start_stadt = end_stadt = NULL;
				start = ziel = NULL;
			}
		}
		else {
			// aktivate AI
			automat = true;
		}
	}
	return automat;
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
	              (text_alter[n] >> 4) -
		      welt->lookup_hgt(text_pos[n]);

	if(text_alter[n] >= -80) {

	    display_proportional_clip( x+xoff+1, y+yoff+1, texte[n], ALIGN_LEFT, SCHWARZ, true);
	    display_proportional_clip( x+xoff, y+yoff, texte[n], ALIGN_LEFT, kennfarbe+3, true);

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
  for(int n=0; n<=last_message_index; n++) {
    if(text_alter[n] >= -80) {
      text_alter[n] -= 5;//delta_t>>2;
    }
  }
}


void
spieler_t::add_message(koord k, int betrag)
{

    for(int n=0; n<50; n++) {
	if(text_alter[n] <= -80) {
	    text_pos[n] = k;

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
			// tell the player
			char buf[256];
		    sprintf(buf,translator::translate("Verschuldet:\n\nDu hast %d Monate Zeit,\ndie Schulden zurueckzuzahlen.\n"),
		    	MAX_KONTO_VERZUG-konto_ueberzogen+1);
		    message_t::get_instance()->add_message(buf,koord::invalid,message_t::problems,kennfarbe,IMG_LEER);
		} else if(konto_ueberzogen <= MAX_KONTO_VERZUG) {
		    sprintf(buf,translator::translate("Verschuldet:\n\nDu hast %d Monate Zeit,\ndie Schulden zurueckzuzahlen.\n"),
		            MAX_KONTO_VERZUG-konto_ueberzogen+1);
		    message_t::get_instance()->add_message(buf,koord::invalid,message_t::problems,kennfarbe,IMG_LEER);
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
        int i;
	for (i=MAX_HISTORY_MONTHS-1; i>0; i--)
	{
		for (int cost_type = 0; cost_type<MAX_COST; cost_type++)
		{
			finance_history_month[i][cost_type] = finance_history_month[i-1][cost_type];
		}
	}

	for (int i=0;i<MAX_COST;i++)
	{
		finance_history_month[0][i] = 0;
		if ((i == COST_ASSETS) || (i == COST_NETWEALTH)) {
			finance_history_month[0][i] = finance_history_month[1][i];
		}
	}
}

void spieler_t::roll_finance_history_year()
{
        int i;
	for (i=MAX_HISTORY_YEARS-1; i>0; i--)
	{
		for (int cost_type = 0; cost_type<MAX_COST; cost_type++)
		{
			finance_history_year[i][cost_type] = finance_history_year[i-1][cost_type];
		}
	}

	for (int i=0;i<MAX_COST;i++)
	{
		finance_history_year[0][i] = 0;
		if ((i == COST_ASSETS) || (i == COST_NETWEALTH)) {
			finance_history_year[0][i] = finance_history_year[1][i];
		}
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
		finance_history_year[0][COST_MARGIN] = (finance_history_year[0][COST_VEHICLE_RUN] + finance_history_year[0][COST_MAINTENANCE]) != 0 ? finance_history_year[0][COST_OPERATING_PROFIT] / abs((finance_history_year[0][COST_VEHICLE_RUN] + finance_history_year[0][COST_MAINTENANCE])) : 0;
    finance_history_month[0][COST_ASSETS] = assets;
    finance_history_month[0][COST_NETWEALTH] = assets + konto;
    finance_history_month[0][COST_CASH] = konto;
		finance_history_month[0][COST_MARGIN] = (finance_history_month[0][COST_VEHICLE_RUN] + finance_history_month[0][COST_MAINTENANCE]) != 0 ? finance_history_month[0][COST_OPERATING_PROFIT] / abs((finance_history_month[0][COST_VEHICLE_RUN] + finance_history_month[0][COST_MAINTENANCE])) : 0;

    /*
    char x1[128], y1[128], x2[128], y2[128], y3[128], buffer[128];
    sprintf(x1, "%d", assets);
    sprintf(y1, "%d", finance_history_year[0][COST_ASSETS]);
    sprintf(x2, "%d", konto);
    sprintf(y2, "%d", finance_history_year[0][COST_NETWEALTH]);
    sprintf(y3, "%f", assets + konto);
    sprintf(buffer, "%s, %s, %s, %s, %s", x1, y1, x2, y2, y3);
    DBG_MESSAGE("draw chart: ", buffer);
    */
}


sint64
spieler_t::buche(const long betrag, const koord pos, const int type)
{
    buche(betrag, type);

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
	finance_history_year[0][COST_MARGIN] = (finance_history_year[0][COST_VEHICLE_RUN] + finance_history_year[0][COST_MAINTENANCE]) != 0 ? finance_history_year[0][COST_OPERATING_PROFIT] * 100/ abs((finance_history_year[0][COST_VEHICLE_RUN] + finance_history_year[0][COST_MAINTENANCE])) : 0;
	// fill month history
	finance_history_month[0][type] += (sint64)betrag;
	finance_history_month[0][COST_PROFIT] += (sint64)betrag;
	finance_history_month[0][COST_CASH] = konto;
	finance_history_month[0][COST_OPERATING_PROFIT] = finance_history_month[0][COST_INCOME] + finance_history_month[0][COST_VEHICLE_RUN] + finance_history_month[0][COST_MAINTENANCE];
	finance_history_month[0][COST_MARGIN] = (finance_history_month[0][COST_VEHICLE_RUN] + finance_history_month[0][COST_MAINTENANCE]) != 0 ? finance_history_month[0][COST_OPERATING_PROFIT] * 100/ abs((finance_history_month[0][COST_VEHICLE_RUN] + finance_history_month[0][COST_MAINTENANCE])) : 0;
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
	if(!halt_list->contains(halt)) {
		halt_list->append( halt );
		haltcount ++;
	}
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


// ki-helfer

bool
spieler_t::suche_platz(int xpos, int ypos, koord *start)
{
	const koord pos = koord(xpos, ypos);

	// not on the map => illegal of course ...
	if(!welt->ist_in_kartengrenzen(pos)) {
	    return false;
	}
	// or a station?
	if(is_my_halt(pos).is_bound()) {
	    return false;
	}
	const planquadrat_t * plan = welt->lookup(pos);
	const grund_t * gr = plan ? plan->gib_kartenboden() : NULL;

	if(gr!=NULL &&
	   (welt->get_slope(pos)==0)  &&
	   // (gr->ist_natur() || gr->gib_typ() == grund_t::fundament) &&
	   gr->ist_natur() &&
	   (gr->kann_alle_obj_entfernen(this)==NULL)  ) {

	    *start = pos;
	    return true;
	}
    return false;
}


/* needed renovation due to different sized factories
 * also try "nicest" place first
 * @author HJ & prissi
 */
bool
spieler_t::suche_platz(int xpos, int ypos, int xpos_target, int ypos_target, koord off, koord *start)
{
DBG_MESSAGE("spieler_t::suche_platz()","at (%i,%i) for size (%i,%i)",xpos,ypos,off.x,off.y);
	// distance of last found point
	int dist=0x7FFFFFFF;
	koord	platz;
	for(  int y=ypos-welt->gib_einstellungen()->gib_station_coverage();  y<ypos+off.y+welt->gib_einstellungen()->gib_station_coverage();  y++  ) {
		for(  int x=xpos-welt->gib_einstellungen()->gib_station_coverage();  x<xpos+off.x+welt->gib_einstellungen()->gib_station_coverage();  x++  ) {
			int	current_dist = abs(xpos_target-x) + abs(ypos_target-y);
			if(   suche_platz(x,y, &platz)  &&  current_dist<dist  ) {
				// we will take the shortest route found
				*start = platz;
				dist = current_dist;
			}
			else {
				koord test(x,y);
				if(is_my_halt(test).is_bound()) {
DBG_MESSAGE("spieler_t::suche_platz()","Search around stop at (%i,%i)",x,y);
					// we are on a station that belongs to us
					int xneu=x-1, yneu=y-1;
					current_dist = abs(xpos_target-xneu) + abs(ypos_target-y);
					if(   suche_platz(xneu,y, &platz)  &&  current_dist<dist  ) {
						// we will take the shortest route found
						*start = platz;
						dist = current_dist;
					}
					current_dist = abs(xpos_target-x) + abs(ypos_target-yneu);
					if(   suche_platz(x,yneu, &platz)  &&  current_dist<dist  ) {
						// we will take the shortest route found
						*start = platz;
						dist = current_dist;
					}
					xneu=x+1;
					yneu=y+1;
					current_dist = abs(xpos_target-xneu) + abs(ypos_target-y);
					if(   suche_platz(xneu,y, &platz)  &&  current_dist<dist  ) {
						// we will take the shortest route found
						*start = platz;
						dist = current_dist;
					}
					current_dist = abs(xpos_target-x) + abs(ypos_target-yneu);
					if(   suche_platz(x,yneu, &platz)  &&  current_dist<dist  ) {
						// we will take the shortest route found
						*start = platz;
						dist = current_dist;
					}
				}
			}
		}
	}
	// of, success of short than maximum
	if(  dist<0x7FFFFFFF  ) {
		return true;
	}
	return false;
}



void
spieler_t::do_passenger_ki()
{
	static int wait = 0;

	wait ++;
	if(wait&7) {
		return;
	}

	switch(state) {
		case NEUE_ROUTE:
		{
			// if we have this little money, we do nothing
			if(konto<2000000) {
				return;
			}
			// assume fail
			state = CHECK_CONVOI;

			const vector_tpl<stadt_t *> *staedte = welt->gib_staedte();
			int anzahl = staedte->get_count();
			int offset = (anzahl>1)?simrand(anzahl-1):0;
			// start with previous target
			const stadt_t* last_star_stadt=start_stadt;
			start_stadt = end_stadt;
			end_stadt = NULL;
			end_ausflugsziel = NULL;
			ziel = NULL;
			platz2 = koord::invalid;
			// if no previous town => find one
			if(start_stadt==NULL) {
				start_stadt = staedte->get(offset);
			}
DBG_MESSAGE("spieler_t::do_passenger_ki()","using city %s for start",start_stadt->gib_name());
			const halthandle_t start_halt = get_our_hub(start_stadt);
			// find starting place
			platz1 = start_halt.is_bound()?start_halt->gib_basis_pos():built_hub(start_stadt->gib_pos(),9);
			if(platz1==koord::invalid) {
				return;
			}

			if(anzahl==1  ||  simrand(3)==0) {
//DBG_MESSAGE("spieler_t::do_passenger_ki()","searching attraction");
				// 25 % of all connections are tourist attractions
				const slist_tpl<gebaeude_t*> &ausflugsziele = welt->gib_ausflugsziele();
				// this way, we are sure, our factory is connected to this town ...
				const slist_tpl<fabrik_t *> &fabriken = start_stadt->gib_arbeiterziele();
				int	last_dist = 0xFFFF;
				bool ausflug=simrand(2)!=0;	// holidays first ...
				int ziel_count=ausflug?ausflugsziele.count():fabriken.count();
				count = 1;	// one vehicle
				for( int i=0;  i<ziel_count;  i++  ) {
					int	dist;
					koord pos, size;
					if(ausflug) {
						if(ausflugsziele.at(i)->gib_post_level()<=2) {
							// not a good object to go to ...
							continue;
						}
						pos=ausflugsziele.at(i)->gib_pos().gib_2d();
						size=ausflugsziele.at(i)->gib_tile()->gib_besch()->gib_groesse(ausflugsziele.at(i)->gib_tile()->gib_layout());
					}
					else {
						pos=fabriken.at(i)->gib_pos().gib_2d();
						size=fabriken.at(i)->gib_besch()->gib_haus()->gib_groesse();
					}
					const stadt_t *next_town = welt->suche_naechste_stadt(pos);
					if(next_town==NULL  ||  start_stadt==next_town) {
						// this is either a town already served (so we do not create a new hub)
						// or a lonely point somewhere
						// in any case we do not want to serve this location already
						koord test_platz=built_hub(pos,size.x);
						if(!is_my_halt(test_platz).is_bound()) {
							// not served
							dist = abs_distance(platz1,test_platz);
							if(dist+simrand(50)<last_dist  &&   dist>3) {
								// but closer than the others
								if(ausflug) {
									end_ausflugsziel = ausflugsziele.at(i);
									count = 1;
//									count = 1 + end_ausflugsziel->gib_passagier_level()/128;
//DBG_MESSAGE("spieler_t::do_passenger_ki()","testing attraction %s with %i busses",end_ausflugsziel->gib_tile()->gib_besch()->gib_name(), count);
								}
								else {
									ziel = fabriken.at(i);
									count = 1;// + (ziel->gib_besch()->gib_pax_level()*ziel->gib_groesse().x*ziel->gib_groesse().y)/128;
								}
								last_dist = dist;
								platz2 = test_platz;
							}
						}
					}
				}
				// test for success
				if(platz2!=koord::invalid) {
					// found something
					state = BAUE_VERBINDUNG;
					substate = NR_INIT;
DBG_MESSAGE("spieler_t::do_passenger_ki()","decision: %s wants to built network between %s and %s",gib_name(),start_stadt->gib_name(),ausflug?end_ausflugsziel->gib_tile()->gib_besch()->gib_name():ziel->gib_name());
				}
			}
			else {
//DBG_MESSAGE("spieler_t::do_passenger_ki()","searching town");
				count = 1;
				// find a good route
				for( int i=0;  i<anzahl;  i++  ) {
					const int nr = (i+offset)%anzahl;
					stadt_t *cur = staedte->get(nr);
					if(cur!=last_star_stadt  &&  cur!=start_stadt  &&  !is_connected(start_halt,cur->get_linksoben(),cur->get_rechtsunten())  ) {
						int	dist;
						if(end_stadt!=NULL) {
							halthandle_t end_halt = get_our_hub(end_stadt);
							// if there is a route, then do not built a line between
							if(end_halt.is_bound()) {
								ware_t pax(warenbauer_t::passagiere);
								pax.setze_zielpos(end_halt->gib_basis_pos());
								INT_CHECK("simplay 838");
								start_halt->suche_route(pax);
								if(pax.gib_ziel() != koord::invalid) {
									// already connected
									continue;
								}
							}
							koord dist1, dist2;
							dist1 = platz1-cur->gib_pos();
							dist2 = platz1-end_stadt->gib_pos();
							dist = abs(dist1.x)+abs(dist1.y) - (abs(dist2.x)+abs(dist2.y));
						}
						// check if more close or NULL
						if(end_stadt==NULL  ||  dist<0  ) {
							end_stadt = cur;
						}
					}
				}
				// ok, found two cities
				if(start_stadt!=NULL  &&  end_stadt!=NULL) {
					state = BAUE_VERBINDUNG;
					substate = NR_INIT;
DBG_MESSAGE("spieler_t::do_passenger_ki()","%s wants to built network between %s and %s",gib_name(),start_stadt->gib_name(),end_stadt->gib_name());
				}
			}
		}
		break;

		// so far only busses
		case BAUE_VERBINDUNG:
		{
			switch(substate) {
				case NR_INIT:
				{
					//
					koord end_hub_pos = koord::invalid;
DBG_MESSAGE("spieler_t::do_passenger_ki()","Find hub");
					// also for target (if not tourist attraction!)
					if(end_stadt!=NULL) {
DBG_MESSAGE("spieler_t::do_passenger_ki()","town %p", end_stadt );
						// target is town
						halthandle_t h = get_our_hub(end_stadt);
						if(h.is_bound()) {
							end_hub_pos = h->gib_basis_pos();
						}
						else {
							end_hub_pos = built_hub(end_stadt->gib_pos(),9);
						}
					}
					else {
						// already found place
						end_hub_pos = platz2;
					}
					// was successful?
					if(end_hub_pos!=koord::invalid) {
						// ok, now we check the vehicle
						platz2 = end_hub_pos;
DBG_MESSAGE("spieler_t::do_passenger_ki()","hub found -> NR_BAUE_ROUTE1");
						substate = NR_BAUE_ROUTE1;
					}
					else {
						// no success
DBG_MESSAGE("spieler_t::do_passenger_ki()","no suitable hub found");
						end_stadt = NULL;
						state = CHECK_CONVOI;
					}
				}
				break;

				// get a suitable vehicle
				case NR_BAUE_ROUTE1:
				// wait for construction semaphore
				if(!baue)
				{
				  	// obey timeline
					unsigned month_now = (welt->gib_zeit_ms() >> welt->ticks_bits_per_tag) + (umgebung_t::starting_year * 12);
					if(  umgebung_t::use_timeline == false   ) {
						month_now = 0xFFFFFFFFu;
					}

					// we want the fastest we can get!
					road_vehicle = vehikelbauer_t::vehikel_search( vehikel_besch_t::strasse, month_now, 10, 80, warenbauer_t::passagiere );
					if(road_vehicle!=NULL) {
						// find cheapest road
						road_weg = wegbauer_t::weg_search( weg_t::strasse, road_vehicle->gib_geschw() );
						substate = NR_BAUE_STRASSEN_ROUTE;
DBG_MESSAGE("spieler_t::do_passenger_ki()","using %s on %s",road_vehicle->gib_name(),road_weg->gib_name());
					}
					else {
						// no success
						end_stadt = NULL;
						state = CHECK_CONVOI;
					}
					// ok, now I do
					baue = true;
					substate = NR_BAUE_STRASSEN_ROUTE;
				}
				break;

				// built a simple road (no bridges, no tunnels)
				case NR_BAUE_STRASSEN_ROUTE:
					if(create_simple_road_transport()  &&
						(is_my_halt(platz1).is_bound()  ||  wkz_bushalt(this, welt, platz1,hausbauer_t::car_stops.at(0)))  &&
						(is_my_halt(platz2).is_bound()  ||  wkz_bushalt(this, welt, platz2,hausbauer_t::car_stops.at(0)))
					  ) {
						koord list[2]={ platz1, platz2 };
						// wait only, if traget is not a hub but an attraction/factory
						create_bus_transport_vehikel(platz1,count,list,2,end_stadt==NULL);
//						create_bus_transport_vehikel(platz2,1,list,2);
						substate = NR_ROAD_SUCCESS;
					}
					else {
						substate = NR_BAUE_STRASSEN_ROUTE2;
					}
				break;

				// built a road, try with bridges or tunnels
				case NR_BAUE_STRASSEN_ROUTE2:
					if(create_complex_road_transport()  &&
						(is_my_halt(platz1).is_bound()  ||  wkz_bushalt(this, welt, platz1,hausbauer_t::car_stops.at(0)))  &&
						(is_my_halt(platz2).is_bound()  ||  wkz_bushalt(this, welt, platz2,hausbauer_t::car_stops.at(0)))
					  ) {
						koord list[2]={ platz1, platz2 };
						// wait only, if traget is not a hub but an attraction/factory
						create_bus_transport_vehikel(platz1,count,list,2,end_stadt==NULL);
						substate = NR_ROAD_SUCCESS;
					}
					else {
						substate = NR_BAUE_CLEAN_UP;
					}
				break;

				// remove marker etc.
				case NR_BAUE_CLEAN_UP:
					baue = false;
					state = CHECK_CONVOI;
					// otherwise it may always try to built the same route!
					end_stadt = NULL;
				break;

				// successful rail construction
				case NR_ROAD_SUCCESS:
				{
					state = CHECK_CONVOI;
					baue = false;
					// tell the player
					char buf[256];
					if(end_ausflugsziel!=NULL) {
						platz1 = end_ausflugsziel->gib_pos().gib_2d();
						sprintf(buf, translator::translate("%s now\noffers bus services\nbetween %s\nand attraction\n%s\nat (%i,%i).\n"), gib_name(), start_stadt->gib_name(), make_single_line_string(translator::translate(end_ausflugsziel->gib_tile()->gib_besch()->gib_name()),2), platz1.x, platz1.y );
DBG_DEBUG("do_passenger_ki()","attraction success");
						end_stadt = start_stadt;
					}
					else if(ziel!=NULL) {
						platz1 = ziel->gib_pos().gib_2d();
DBG_DEBUG("do_passenger_ki()","%s",translator::translate("%s now\noffers bus services\nbetween %s\nand factory\n%s\nat (%i,%i).\n") );
						sprintf(buf, translator::translate("%s now\noffers bus services\nbetween %s\nand factory\n%s\nat (%i,%i).\n"), gib_name(), start_stadt->gib_name(), make_single_line_string(translator::translate(ziel->gib_name()),2), platz1.x, platz1.y );
DBG_DEBUG("do_passenger_ki()","factory success1");
						end_stadt = start_stadt;
					}
					else {
						sprintf(buf, translator::translate("Travellers now\nuse %s's\nbusses between\n%s \nand %s.\n"), gib_name(), start_stadt->gib_name(), end_stadt->gib_name() );
					}
DBG_DEBUG("do_passenger_ki()","calling message_t()");
					message_t::get_instance()->add_message(buf,platz1,message_t::ai,kennfarbe,road_vehicle->gib_basis_bild());
				}
				break;
			}
		}
		break;

		// add vehicles to crowded lines
		case CHECK_CONVOI:
		{
#if 0
			slist_iterator_tpl<halthandle_t> iter( halt_list );

			while(iter.next()  &&  road_vehicle!=NULL) {
				halthandle_t halt = iter.get_current();
				if(halt.is_bound()  &&  (halt->get_pax_unhappy()>200  ||  halt->gib_ware_summe(warenbauer_t::passagiere)>1000)) {
//					slist_tpl<ware_t> * wliste = waren.get(warenbauer_t::passagiere);
					slist_iterator_tpl<ware_t> iter (halt->gib_warenliste(warenbauer_t::passagiere));
					// now we check if there are many many people waiting ...
					while(iter.next()) {
						ware_t ware = iter.get_current();
						// since a single stop do not hold more than 128 people
						if(ware.menge>129) {
							// we want the fastest we can get
						  	// obey timeline
							unsigned month_now = 0xFFFFFFFFu;
							if(  umgebung_t::use_timeline == true   ) {
								month_now = (welt->gib_zeit_ms() >> welt->ticks_bits_per_tag) + (umgebung_t::starting_year * 12);
							}
							road_vehicle = vehikelbauer_t::vehikel_search( vehikel_besch_t::strasse, month_now, 10, 80, warenbauer_t::passagiere );
							koord list[2] = {halt->gib_basis_pos(),ware.gib_zwischenziel()};
							create_bus_transport_vehikel( list[0], 1, list, 2, false );	// overcrowded line, so waiting does not make sense
DBG_MESSAGE("spieler_t::do_passenger_ki()","add new convoi to crowded line from %s to %s",halt->gib_name(),is_my_halt(list[1])->gib_name());
							break;
						}
					}
				}
			}
#else
			slist_iterator_tpl<convoihandle_t> iter (welt->gib_convoi_list());
			while(iter.next()) {
				const convoihandle_t cnv = iter.get_current();
				if(cnv->gib_besitzer()==this) {
					// check for empty vehicles (likely stucked) that are making no plus and remove them ...
					// take care, that the vehicle is old enough ...
					if((welt->gib_zeit_ms()-cnv->gib_vehikel(0)->gib_insta_zeit())>>(karte_t::ticks_bits_per_tag+1)>2  &&  cnv->gib_jahresgewinn()<=0  ){
						sint64 passenger=0;
						for( int i=0;  i<MAX_MONTHS;  i ++) {
							passenger += cnv->get_finance_history(i,CONVOI_TRANSPORTED_GOODS);
						}
						if(passenger==0) {
							// now passengers for two months?
							// well, then delete this (likely stucked somewhere)
DBG_MESSAGE("spieler_t::do_passenger_ki()","convoi %s not needed!",cnv->gib_name());
							buche( -cnv->calc_restwert(), cnv->gib_pos().gib_2d(), COST_NEW_VEHICLE );
							cnv->self_destruct();
							break;
						}
					}
					// then check for overcrowded lines
					if(cnv->gib_vehikel(0)->gib_fracht_menge()==cnv->gib_vehikel(0)->gib_fracht_max()) {
						INT_CHECK("simplay 889");
						// this is our vehicle, and is 100% loaded
						fahrplan_t  *f = cnv->gib_fahrplan();
						koord3d startpos= cnv->gib_pos();
						// next checkpoint also crowed with things for us?
						halthandle_t h0=welt->lookup( f->eintrag.at(0).pos )->gib_halt();
						halthandle_t h1=welt->lookup( f->eintrag.at(1).pos )->gib_halt();
DBG_MESSAGE("spieler_t::do_passenger_ki()","checking our convoi %s between %s and %s",cnv->gib_name(),h0->gib_name(),h1->gib_name());
DBG_MESSAGE("spieler_t::do_passenger_ki()","waiting: %s (%i) and %s (%i)",h0->gib_name(),h0->gib_ware_fuer_zwischenziel(warenbauer_t::passagiere,f->eintrag.at(1).pos.gib_2d()),h1->gib_name(),h1->gib_ware_fuer_zwischenziel(warenbauer_t::passagiere,f->eintrag.at(0).pos.gib_2d()));

						// we assume crowed for more than 129 waiting passengers
						if(	h0->gib_ware_fuer_zwischenziel(warenbauer_t::passagiere,f->eintrag.at(1).pos.gib_2d())>250   ||
							h1->gib_ware_fuer_zwischenziel(warenbauer_t::passagiere,f->eintrag.at(0).pos.gib_2d())>250   ) {
	DBG_MESSAGE("spieler_t::do_passenger_ki()","copy convoi %s on route %s to %s",cnv->gib_name(),h0->gib_name(),h1->gib_name());
							vehikel_t * v = vehikelbauer_t::baue(welt, startpos, this,NULL, cnv->gib_vehikel(0)->gib_besch());
							convoi_t *new_cnv = new convoi_t(welt, this);
							// V.Meyer: give the new convoi name from first vehicle
							new_cnv->setze_name( v->gib_besch()->gib_name() );
							new_cnv->add_vehikel( v );
							fahrplan_t *fpl = new_cnv->gib_vehikel(0)->erzeuge_neuen_fahrplan();
							// do not start at current stop => wont work ...
							fpl->aktuell = (f->eintrag.at(0).pos==startpos)?1:0;
							// now copy scedule
							fpl->maxi = f->maxi;
							for(int j=0;  j<=f->maxi;  j++) {
								fpl->eintrag.at(j).pos = f->eintrag.at(j).pos;
								// waiting on a overcrowded line does not make sense!
								// fpl->eintrag.at(j).ladegrad = f->eintrag.at(j).ladegrad;
								fpl->eintrag.at(j).ladegrad = 0;
							}
							// and start new convoi
							welt->sync_add( new_cnv );
							new_cnv->setze_fahrplan(fpl);
							new_cnv->start();
							// we do not want too many copies, just copy once!
							break;
						}
					}
				}
			}
#endif
			state = NEUE_ROUTE;
		}
		break;

		default:
			DBG_MESSAGE("spieler_t::do_passenger_ki()",	"Illegal state %d with substate %d!", state, substate);
			state = NEUE_ROUTE;
			end_stadt = NULL;
			substate = NR_INIT;
	}
}




void
spieler_t::do_ki()
{
	if((steps & 7) != 0) {
		return;
	}

	if(passenger_transport) {
		// passenger are special ...
		do_passenger_ki();
		return;
	}

	switch(state) {
		case NEUE_ROUTE:
			switch(substate) {

				case NR_INIT:
					gewinn = -1;
					count = 0;
					// calculate routes only for account > 10000
					if(  (40 * -CST_STRASSE) -
						CST_MEDIUM_VEHIKEL*6 -
						CST_FRACHTHOF*2 < konto) {
						last_start = start;
						last_ziel = ziel;
						start = ziel = NULL;
						substate = NR_SAMMLE_ROUTEN;
						// small advantage in front of contructing AI
						if(baue) {
							count = KI_TEST_LINES/2 - 1;
						}
					}
				break;

				/* try to built a network:
				* last target also new target ...
				*/
				case NR_SAMMLE_ROUTEN:
				{
					fabrik_t * start_neu = NULL;
					fabrik_t * ziel_neu = NULL;
					int	start_ware_neu;
					int gewinn_neu=-1;

					if(  count==KI_TEST_LINES-2  &&  last_ziel!=NULL  ) {
						// we built a line: do we need another supplier?
						gewinn_neu = suche_transport_quelle(&start_neu, &start_ware_neu, last_ziel);
						if(  gewinn_neu>KI_MIN_PROFIT  &&  start_neu!=last_start  &&  start_neu!=NULL  ) {
							// marginally profitable -> then built it!
DBG_MESSAGE("spieler_t::do_ki","Select quelle from %s (%i,%i) to %s (%i,%i) (income %i)",start_neu->gib_name(),start_neu->pos.x,start_neu->pos.y,ziel_neu->gib_name(),ziel_neu->pos.x,ziel_neu->pos.y, gewinn_neu);
							start = start_neu;
							start_ware = start_ware_neu;
							gewinn = gewinn_neu;
							ziel = last_ziel;
							count = KI_TEST_LINES;
						}
						count++;
					}
					else
					{
#ifdef EARLY_SEARCH_FOR_BUYER
						// does not work well together with 128er pak
						if(  count==KI_TEST_LINES-1  &&  last_ziel!=NULL  ) {
							// found somebody, who wants our stuff?
							start_neu = last_ziel;
							gewinn_neu = suche_transport_ziel(start_neu, &start_ware_neu, &ziel_neu);
							if(  gewinn_neu>gewinn>>1  ) {
								// marginally profitable -> then built it!
								start = start_neu;
								start_ware = start_ware_neu;
								ziel = ziel_neu;
								gewinn = gewinn_neu;
							}
							count++;
						}
						else
#endif
						{
							// just normal random search ...
							start_neu = welt->get_random_fab();
							if( start_neu!=NULL  &&  !welt->lookup(start_neu->pos)->ist_wasser()    &&  start_neu!=last_start  ) {
								gewinn_neu = suche_transport_ziel(start_neu, &start_ware_neu, &ziel_neu);
								if(gewinn_neu > -1   ) {
DBG_MESSAGE("spieler_t::do_ki","Check route from %s (%i,%i) to %s (%i,%i) (income %i)",start_neu->gib_name(),start_neu->pos.x,start_neu->pos.y,ziel_neu->gib_name(),ziel_neu->pos.x,ziel_neu->pos.y, gewinn_neu);
								}
							}
						}
					}

					// better than last one?
					// and not identical to last one
					if(gewinn_neu > gewinn  &&  !(start_neu==last_start  &&  last_ziel==ziel_neu)  &&  !(start_neu==baue_start  &&  ziel_neu==baue_ziel)  ) {
						// more income and not the same than before ...
						start = start_neu;
						start_ware = start_ware_neu;
						ziel = ziel_neu;
						gewinn = gewinn_neu;
DBG_MESSAGE("spieler_t::do_ki","Consider route from %s (%i,%i) to %s (%i,%i) (income %i)",start_neu->gib_name(),start_neu->pos.x,start_neu->pos.y,ziel_neu->gib_name(),ziel_neu->pos.x,ziel_neu->pos.y, gewinn_neu);
					}

					if(gewinn_neu > KI_MIN_PROFIT) {
						count ++;
DBG_MESSAGE("spieler_t::do_ki","Tried already %i routes",count);
					}

					if(count >= KI_TEST_LINES && gewinn > KI_MIN_PROFIT ) {
						if(  start !=NULL && ziel!=NULL  &&  !(start==baue_start  &&  ziel==baue_ziel)  ) {
DBG_MESSAGE("spieler_t::do_ki","Decicion %s to %s (income %i)",start->gib_name(),ziel->gib_name(), gewinn);
							substate = NR_BAUE_ROUTE1;
						}
						else {
							// use another line
							state = CHECK_CONVOI;
						}
					}
				}
				break;

				// now we need so select the cheapest mean to get maximum profit
				case NR_BAUE_ROUTE1:
				{
					if(baue) {
DBG_MESSAGE("do_ki()","Player %s could not built: baue=%1",gib_name(),baue);
						state = CHECK_CONVOI;
						break;
					}
					// just mark for other AI: this is off limits ...
					baue = true;
					baue_start = start;
					baue_ziel = ziel;
					/* if we reached here, we decide to built a route;
					 * the KI just chooses the way to run the operation at maximum profit (minimum loss).
					 * The KI will built also a loosing route; this might be required by future versions to
					 * be able to built a network!
					 * @author prissi
					 */
DBG_MESSAGE("spieler_t::do_ki()","%s want to build a route from %s (%d,%d) to %s (%d,%d) with value %d",
		gib_name(),
	     start->gib_name(), start->gib_pos().x, start->gib_pos().y,
	     ziel->gib_name(), ziel->gib_pos().x, ziel->gib_pos().y,
	     gewinn);

					koord zv = start->gib_pos().gib_2d() - ziel->gib_pos().gib_2d();
					int dist = ABS(zv.x) + ABS(zv.y);

					/* for the calculation we need:
					 * a suitable car (and engine)
					 * a suitable weg
					 */

					// needed for search
					// first we have to find a suitable car
					const ware_besch_t *freight = start->gib_ausgang()->get(start_ware).gib_typ();

					// guess the "optimum" speed (usually a little too low)
				  	int best_rail_speed = MIN(60+freight->gib_speed_bonus()*5, 140 );
				  	int best_road_speed = MIN(60+freight->gib_speed_bonus()*5, 130 );

				  	// obey timeline
					unsigned month_now = (welt->gib_zeit_ms() >> welt->ticks_bits_per_tag) + (umgebung_t::starting_year * 12);
					if(  umgebung_t::use_timeline == false   ) {
						month_now = 0xFFFFFFFFu;
					}

					INT_CHECK("simplay 1265");

					// is rail transport allowed?
					if(rail_transport) {
						// any rail car that transport this good (actually this returns the largest)
						rail_vehicle = vehikelbauer_t::vehikel_search( vehikel_besch_t::schiene, month_now, 0, best_rail_speed,  freight );
					}
					rail_engine = NULL;
					rail_weg = NULL;
DBG_MESSAGE("do_ki()","rail vehicle %p",rail_vehicle);

					// is road transport allowed?
					if(road_transport) {
						// any road car that transport this good (actually this returns the largest)
						road_vehicle = vehikelbauer_t::vehikel_search( vehikel_besch_t::strasse, month_now, 10, best_road_speed, freight );
					}
					road_weg = NULL;
DBG_MESSAGE("do_ki()","road vehicle %p",road_vehicle);

					INT_CHECK("simplay 1265");

					// properly calculate production
					const int prod = MIN(ziel->max_produktion(),
					                 ( start->max_produktion() * start->gib_besch()->gib_produkt(start_ware)->gib_faktor() )/256 - start->gib_abgabe_letzt(start_ware) );

					/* calculate number of cars for railroad */
					count_rail=255;	// no cars yet
					if(  rail_vehicle!=NULL  ) {
						// if our car is faster: well use faster speed
					 	best_rail_speed = MIN(80,rail_vehicle->gib_geschw());
						// for engine: gues number of cars
						count_rail = (prod*dist) / (rail_vehicle->gib_zuladung()*best_rail_speed)+1;
						rail_engine = vehikelbauer_t::vehikel_search( vehikel_besch_t::schiene, month_now, 80*count_rail, best_rail_speed, NULL );
						if(  rail_engine!=NULL  ) {
						  if(  best_rail_speed>rail_engine->gib_geschw()  ) {
						    best_rail_speed = rail_engine->gib_geschw();
						  }
						  // find cheapest track with that speed
						 rail_weg = wegbauer_t::weg_search( weg_t::schiene, best_rail_speed );
						 if(  rail_weg!=NULL  ) {
							  if(  best_rail_speed>rail_weg->gib_topspeed()  ) {
							  	best_rail_speed = rail_weg->gib_topspeed();
							  }
							  // no train can have more than 15 cars
							  count_rail = MIN( 15, (prod*dist) / (rail_vehicle->gib_zuladung()*best_rail_speed) );
							  // if engine too week, reduce number of cars
							  if(  count_rail*80*64>(rail_engine->gib_leistung()*rail_engine->get_gear())  ) {
							  	count_rail = rail_engine->gib_leistung()*rail_engine->get_gear()/(80*64);
							  }
							count_rail = ((count_rail+1)&0x0FE)+1;
DBG_MESSAGE("spieler_t::do_ki()","Engine %s guess to need %d rail cars %s for route (%s)", rail_engine->gib_name(), count_rail, rail_vehicle->gib_name(), rail_weg->gib_name() );
							}
						}
						if(  rail_engine==NULL  ||  rail_weg==NULL  ) {
							// no rail transport possible
DBG_MESSAGE("spieler_t::do_ki()","No railway possible.");
							rail_vehicle = NULL;
							count_rail = 255;
						}
					}

					INT_CHECK("simplay 1265");

					/* calculate number of cars for road; much easier */
					count_road=255;	// no cars yet
					if(  road_vehicle!=NULL  ) {
						best_road_speed = road_vehicle->gib_geschw();
						// find cheapest road
						road_weg = wegbauer_t::weg_search( weg_t::strasse, best_road_speed );
						if(  road_weg!=NULL  ) {
							if(  best_road_speed>road_weg->gib_topspeed()  ) {
								best_road_speed = road_weg->gib_topspeed();
							}
							// minimum vehicle is 1, maximum vehicle is 48, more just result in congestion
							count_road = MIN( 254, (prod*dist) / (road_vehicle->gib_zuladung()*best_road_speed*2)+2 );
DBG_MESSAGE("spieler_t::do_ki()","guess to need %d road cars %s for route %s", count_road, road_vehicle->gib_name(), road_weg->gib_name() );
						}
						else {
							// no roads there !?!
DBG_MESSAGE("spieler_t::do_ki()","No roadway possible.");
						}
					}

					// find the cheapest transport ...
					// assume maximum cost
					int	cost_rail=0x7FFFFFFF, cost_road=0x7FFFFFFF;
					int	income_rail=0, income_road=0;

					// calculate cost for rail
					if(  count_rail<255  ) {
						int freight_price = (freight->gib_preis()*rail_vehicle->gib_zuladung()*count_rail)/24*((8000+(best_rail_speed-80)*freight->gib_speed_bonus())/1000);
						// calculated here, since the above number was based on production
						// only uneven number of cars bigger than 3 makes sense ...
						count_rail = MAX( 3, count_rail );
						income_rail = (freight_price*best_rail_speed)/(2*dist+count_rail);
						cost_rail = rail_weg->gib_wartung() + (((count_rail+1)/2)*300)/dist + ((count_rail*rail_vehicle->gib_betriebskosten()+rail_engine->gib_betriebskosten())*best_rail_speed)/(2*dist+count_rail);
						DBG_MESSAGE("spieler_t::do_ki()","Netto credits per day for rail transport %.2f (income %.2f)",cost_rail/100.0, income_rail/100.0 );
						cost_rail -= income_rail;
					}

					// and calculate cost for road
					if(  count_road<255  ) {
						// for short distance: reduce number of cars
						// calculated here, since the above number was based on production
						count_road = CLIP( (dist*15)/best_road_speed, 2, count_road );
						int freight_price = (freight->gib_preis()*road_vehicle->gib_zuladung()*count_road)/24*((8000+(best_road_speed-80)*freight->gib_speed_bonus())/1000);
						cost_road = road_weg->gib_wartung() + 300/dist + (count_road*road_vehicle->gib_betriebskosten()*best_road_speed)/(2*dist+5);
						income_road = (freight_price*best_road_speed)/(2*dist+5);
						DBG_MESSAGE("spieler_t::do_ki()","Netto credits per day and km for road transport %.2f (income %.2f)",cost_road/100.0, income_road/100.0 );
						cost_road -= income_road;
					}

					// check location, if vehicles found
					if(  MIN(count_road,count_rail)!=255  &&  suche_platz1_platz2(start, ziel)  ) {
						// road or rail?
						if(  cost_rail<cost_road  ) {
							substate = NR_BAUE_SIMPLE_SCHIENEN_ROUTE;
						}
						else {
							substate = NR_BAUE_STRASSEN_ROUTE;
						}
					}
					else {
						// falls gar nichts klappt gehen wir zum initialzustand zurueck
						baue = false;
						state = CHECK_CONVOI;
					}
				}
				break;

				// built a simple railroad (no bridges, no tunnels)
				case NR_BAUE_SIMPLE_SCHIENEN_ROUTE:
					if(create_simple_rail_transport()) {
						count_rail = baue_bahnhof(start->pos,&platz1, count_rail)-1;
						count_rail = baue_bahnhof(ziel->pos,&platz2, count_rail)-1;
						create_rail_transport_vehikel(platz1,platz2, count_rail, 100 );
						substate = NR_RAIL_SUCCESS;
					}
					else {
						substate = NR_BAUE_SCHIENEN_ROUTE1;
					}
				break;

				// built a railroad, try with bridges or tunnels
				case NR_BAUE_SCHIENEN_ROUTE1:
					if(create_complex_rail_transport()) {
						count_rail = baue_bahnhof(start->pos,&platz1, count_rail)-1;
						count_rail = baue_bahnhof(ziel->pos,&platz2, count_rail)-1;
						create_rail_transport_vehikel(platz1,platz2, count_rail, 100 );
						substate = NR_RAIL_SUCCESS;
					}
					else {
						substate = NR_BAUE_SCHIENEN_ROUTE2;
					}
				break;

				// built a railroad, try with bridges or tunnels, only backwards
				// CHECK!
				// prissi: since the waybuilder tries also backwards, I am not sure this is successful
				case NR_BAUE_SCHIENEN_ROUTE2:
				{
					const koord tmp = platz2;
					platz2 = platz1;
					platz1 = tmp;

					if(create_complex_rail_transport()) {
						count_rail = baue_bahnhof(start->pos,&platz2, count_rail)-1;
						count_rail = baue_bahnhof(ziel->pos,&platz1, count_rail)-1;
						create_rail_transport_vehikel(platz2,platz1,count_rail, 100 );
						substate = NR_RAIL_SUCCESS;
					}
					else {
						substate = NR_BAUE_CLEAN_UP;
						// could not lay track
						if(  count_road<255  ) {
							substate = NR_BAUE_STRASSEN_ROUTE;
						}
					}
				}
				break;

				// built a simple road (no bridges, no tunnels)
				case NR_BAUE_STRASSEN_ROUTE:
					if(create_simple_road_transport()) {
						create_road_transport_vehikel(start, count_road  );
						substate = NR_ROAD_SUCCESS;
					}
					else {
						substate = NR_BAUE_STRASSEN_ROUTE2;
					}
				break;

				// built a road, try with bridges or tunnels
				case NR_BAUE_STRASSEN_ROUTE2:
					if(create_complex_road_transport()) {
						create_road_transport_vehikel(start, count_road  );
						substate = NR_ROAD_SUCCESS;
					}
					else {
						substate = NR_BAUE_CLEAN_UP;
					}
				break;

				// remove marker etc.
				case NR_BAUE_CLEAN_UP:
					// otherwise it may always try to built the same route!
					ziel = NULL;
					// schilder aufraeumen
					if(welt->lookup(platz1)) {
						welt->access(platz1)->gib_kartenboden()->obj_loesche_alle(this);
					}
					if(welt->lookup(platz2)) {
						welt->access(platz2)->gib_kartenboden()->obj_loesche_alle(this);
					}
					baue = false;
					state = CHECK_CONVOI;
				break;

				// successful rail construction
				case NR_RAIL_SUCCESS:
				{
					// tell the player
					char buf[256];
					sprintf(buf, translator::translate("%s\nopened a new railway\nbetween %s\nat (%i,%i) and\n%s at (%i,%i)."), gib_name(), translator::translate(start->gib_name()), start->pos.x, start->pos.y, translator::translate(ziel->gib_name()), ziel->pos.x, ziel->pos.y );
					message_t::get_instance()->add_message(buf,start->pos.gib_2d(),message_t::ai,kennfarbe,rail_engine->gib_basis_bild());

					baue = false;
					state = CHECK_CONVOI;
				}
				break;

				// successful rail construction
				case NR_ROAD_SUCCESS:
				{
					// tell the player
					char buf[256];
					sprintf(buf, translator::translate("%s\nnow operates\n%i trucks between\n%s at (%i,%i)\nand %s at (%i,%i)."), gib_name(), count_road, translator::translate(start->gib_name()), start->pos.x, start->pos.y, translator::translate(ziel->gib_name()), ziel->pos.x, ziel->pos.y );
					message_t::get_instance()->add_message(buf,start->pos.gib_2d(),message_t::ai,kennfarbe,road_vehicle->gib_basis_bild());

					baue = false;
					state = CHECK_CONVOI;
				}
				break;

				default:
					dbg->error("spieler_t::do_ki()","State %d with illegal substate %d!", state, substate);
			}
		break;

		// remove stucked vehicles
		case CHECK_CONVOI:
		{
			slist_iterator_tpl<convoihandle_t> iter (welt->gib_convoi_list());
			while(iter.next()) {
				const convoihandle_t cnv = iter.get_current();
				if(cnv->gib_besitzer()==this) {
					// check for empty vehicles (likely stucked) that are making no plus and remove them ...
					// take care, that the vehicle is old enough ...
					if((welt->gib_zeit_ms()-cnv->gib_vehikel(0)->gib_insta_zeit())>>(karte_t::ticks_bits_per_tag+1)>2  &&  cnv->gib_jahresgewinn()==0  ){
						sint64 passenger=0;
						for( int i=0;  i<MAX_MONTHS;  i ++) {
							passenger += cnv->get_finance_history(i,CONVOI_TRANSPORTED_GOODS);
						}
						if(passenger==0) {
							// now passengers for two months?
							// well, then delete this (likely stucked somewhere)
DBG_MESSAGE("spieler_t::do_ki()","convoi %s not needed!",cnv->gib_name());
							buche( -cnv->calc_restwert(), cnv->gib_pos().gib_2d(), COST_NEW_VEHICLE );
							cnv->self_destruct();
							break;
						}
					}
#if 0
					// then check for overcrowded lines
					if(cnv->gib_vehikel(0)->gib_fracht_menge()==cnv->gib_vehikel(0)->gib_fracht_max()) {
						INT_CHECK("simplay 889");
						// this is our vehicle, and is 100% loaded
						fahrplan_t  *f = cnv->gib_fahrplan();
						koord3d startpos= cnv->gib_pos();
						// next checkpoint also crowed with things for us?
						halthandle_t h0=welt->lookup( f->eintrag.at(0).pos )->gib_halt();
						halthandle_t h1=welt->lookup( f->eintrag.at(1).pos )->gib_halt();
DBG_MESSAGE("spieler_t::do_passenger_ki()","checking our convoi %s between %s and %s",cnv->gib_name(),h0->gib_name(),h1->gib_name());
DBG_MESSAGE("spieler_t::do_passenger_ki()","waiting: %s (%i) and %s (%i)",h0->gib_name(),h0->gib_ware_fuer_zwischenziel(warenbauer_t::passagiere,f->eintrag.at(1).pos.gib_2d()),h1->gib_name(),h1->gib_ware_fuer_zwischenziel(warenbauer_t::passagiere,f->eintrag.at(0).pos.gib_2d()));

						// we assume crowed for more than 129 waiting passengers
						if(	h0->gib_ware_fuer_zwischenziel(warenbauer_t::passagiere,f->eintrag.at(1).pos.gib_2d())>250   ||
							h1->gib_ware_fuer_zwischenziel(warenbauer_t::passagiere,f->eintrag.at(0).pos.gib_2d())>250   ) {
	DBG_MESSAGE("spieler_t::do_passenger_ki()","copy convoi %s on route %s to %s",cnv->gib_name(),h0->gib_name(),h1->gib_name());
							vehikel_t * v = vehikelbauer_t::baue(welt, startpos, this,NULL, cnv->gib_vehikel(0)->gib_besch());
							convoi_t *new_cnv = new convoi_t(welt, this);
							// V.Meyer: give the new convoi name from first vehicle
							new_cnv->setze_name( v->gib_besch()->gib_name() );
							new_cnv->add_vehikel( v );
							fahrplan_t *fpl = new_cnv->gib_vehikel(0)->erzeuge_neuen_fahrplan();
							// do not start at current stop => wont work ...
							fpl->aktuell = (f->eintrag.at(0).pos==startpos)?1:0;
							// now copy scedule
							fpl->maxi = f->maxi;
							for(int j=0;  j<=f->maxi;  j++) {
								fpl->eintrag.at(j).pos = f->eintrag.at(j).pos;
								// waiting on a overcrowded line does not make sense!
								// fpl->eintrag.at(j).ladegrad = f->eintrag.at(j).ladegrad;
								fpl->eintrag.at(j).ladegrad = 0;
							}
							// and start new convoi
							welt->sync_add( new_cnv );
							new_cnv->setze_fahrplan(fpl);
							new_cnv->start();
							// we do not want too many copies, just copy once!
							break;
						}
					}
#endif
				}
			}
			state = NEUE_ROUTE;
			substate = NR_INIT;
		}
		break;

		default:
			DBG_MESSAGE("spieler_t::do_ki()",	"Illegal state %d with substate %d!", state, substate);
			state = NEUE_ROUTE;
			substate = NR_INIT;
	}
}




/* return true, if my bahnhof is here
 * @author prissi
 */
halthandle_t
spieler_t::is_my_halt(koord pos) const
{
	halthandle_t halt;
	if(welt->ist_in_kartengrenzen(pos)) {
		halt = welt->lookup(pos)->gib_kartenboden()->gib_halt();
		if(  halt.is_bound()  &&  halt->gib_besitzer()==this  ) {
			return halt;
		}
	}
	// nothing here
	halthandle_t unbound;
	return unbound;
}


/* try farder to built a station:
 * check also sidewards
 * @author prissi
 */
int
spieler_t::baue_bahnhof(koord3d quelle,koord *p, int anz_vehikel)
{
	int laenge = MAX((anz_vehikel+1)/2,1);       // aufrunden!
	koord t = *p;

	int baulaenge = 0;
	ribi_t::ribi ribi = welt->lookup(*p)->gib_kartenboden()->gib_weg_ribi(weg_t::schiene);
	int i;

	koord zv ( ribi );

	zv.x = -zv.x;
	zv.y = -zv.y;

	DBG_MESSAGE("spieler_t::baue_bahnhof()",
	 "try to build a train station of length %d (%d vehicles) at (%i,%i)",
	 laenge, anz_vehikel, p->x, p->y);

	INT_CHECK("simplay 548");

	const int hoehe = welt->lookup(t)->gib_kartenboden()->gib_hoehe();

	t = *p;
	for(i=0; i<laenge-1; i++) {
		t += zv;
		welt->ebne_planquadrat(t, hoehe);
	}

	t = *p;
	bool ok = true;

	for(i=0; i<laenge-1; i++) {
		t += zv;

		const planquadrat_t *plan = welt->lookup(t);
		grund_t *gr = plan ? plan->gib_kartenboden() : 0;

		ok &= gr != NULL &&
		(gr->ist_natur() || gr->gib_typ() == grund_t::fundament) &&
		gr->kann_alle_obj_entfernen(this) == NULL &&
		gr->gib_grund_hang() == hang_t::flach  &&  !gr->gib_halt().is_bound();


		if(ok) {
			// gr->obj_loesche_alle(this);
			wkz_remover(this, welt, t);

		} else {

			const planquadrat_t *plan = welt->lookup(*p-zv);
			gr = plan ? plan->gib_kartenboden(): 0;

			// wenns nicht mehr vorwaerts geht, dann vielleicht zurueck ?
			ok = gr != NULL &&
						gr->gib_weg(weg_t::schiene) != NULL &&
						gr->gib_weg_ribi(weg_t::schiene) == ribi_t::doppelt(ribi) &&
						gr->kann_alle_obj_entfernen(this) == NULL &&
						welt->get_slope(*p-zv) == 0  &&  !gr->gib_halt().is_bound();
			if(ok) {
				gr->obj_loesche_alle(this);
				// ziel auf das ende des Bahnhofs setzen
DBG_MESSAGE("spieler_t::baue_bahnhof","go back one segment");
				(*p) -= zv;
			}
			else {
				// try to extend to fabrik, if not already done
				koord test = koord(p->x-quelle.x,p->y-quelle.y);
				if(  gr!=NULL  &&  gr->gib_weg_ribi(weg_t::schiene) == ribi_t::ist_kurve(ribi)  &&  ribi_typ(test)!=ribi_typ(zv)  ) {
DBG_MESSAGE("spieler_t::baue_bahnhof","try to remove last segment");
					wkz_remover(this, welt, *p);
					// one back
					// ziel auf das ende des Bahnhofs setzen
					(*p) -= zv;
					t = (*p);
					i = -1;
					ribi = welt->lookup(*p)->gib_kartenboden()->gib_weg_ribi(weg_t::schiene);
					zv = ( ribi );
					continue;
				}
			}

			t -= zv;
		}
	}

	INT_CHECK("simplay 593");

	wegbauer_t bauigel(welt, this);
	bauigel.baubaer = false;

	INT_CHECK("simplay 599");

	bauigel.route_fuer(wegbauer_t::schiene, rail_weg);
	bauigel.calc_route(*p, t);
	bauigel.baue();

	// to avoid broken stations, they will be always built next to an existing
	bool make_all_bahnhof=false;

	// find a freight train station
	const haus_besch_t * besch=hausbauer_t::train_stops.at(0);
	for( int i=1;  i<hausbauer_t::train_stops.count();  i++  ) {
		if(strstr(hausbauer_t::train_stops.at(i)->gib_name(),"Freight")) {
			besch = hausbauer_t::train_stops.at(i);
			if(simrand(20)<7) {
				break;
			}
		}
	}

	// first one direction
     koord pos;
	for(  pos=*p;  pos!=t+zv;  pos+=zv ) {
		if(  make_all_bahnhof  ||  is_my_halt(pos+koord(-1,-1)).is_bound()  ||  is_my_halt(pos+koord(-1,1)).is_bound()  ||  is_my_halt(pos+koord(1,-1)).is_bound()  ||  is_my_halt(pos+koord(1,1)).is_bound()  ) {
			// start building, if next to an existing station
			make_all_bahnhof = true;
			wkz_bahnhof(this, welt, pos, besch);
		}
		INT_CHECK("simplay 753");
	}
	// now add the other squares (going backwards)
	for(  pos=t;  pos!=*p-zv;  pos-=zv ) {
		if(  !is_my_halt(pos).is_bound()  ) {
			wkz_bahnhof(this, welt, pos, besch);
		}
		baulaenge ++;
	}

/*
	if(  p->x>t.x    ||  p->y>t.y  ) {
		// built other way round
		start = t-zv;
		end = *p-zv;
		zv.x = -zv.x;
		zv.y = -zv.y;
	}
	do {
		wkz_bahnhof(this, welt, start, hausbauer_t::gueterbahnhof_besch);

		start += zv;
		baulaenge ++;

		INT_CHECK("simplay 753");
	} while(start != end);
*/

//	p -= zv;
DBG_MESSAGE("spieler_t::baue_bahnhof","Final station at (%i,%i)",p->x,p->y);
	return baulaenge*2;
}


/* return a rating factor; i.e. how much this connecting would benefit game play
 * @author prissi
 */
int
spieler_t::rating_transport_quelle_ziel(fabrik_t *qfab,const ware_t *ware,fabrik_t *zfab)
{
	const vector_tpl<ware_t> *eingang = zfab->gib_eingang();
	// we may have more than one input:
	unsigned missing_input_ware=0;
	unsigned missing_our_ware=0;

	// hat noch mehr als halbvolle Lager  and more then 35 (otherwise there might be also another transporter) ?
	if(ware->menge < ware->max>>1  ||  ware->menge<(34<<fabrik_t::precision_bits)  ) {
		// zu wenig vorrat für profitable strecke
		return 0;
	}
	// if not really full output storage:
	if(  ware->menge < ware->max-(34<<fabrik_t::precision_bits)  ) {
		// well consider, but others are surely better
		return 1;
	}

	// so we do a loop
	for(  unsigned i=0;  i<eingang->get_count();  i++  ) {
		if( eingang->get(i).menge<(10<<fabrik_t::precision_bits)  ) {
			// so more would be helpful
			missing_input_ware ++;
			if(  eingang->get(i).gib_typ()==ware->gib_typ()  ) {
				missing_our_ware = 1;
			}
		}
	}

DBG_MESSAGE("spieler_t::rating_transport_quelle_ziel","Missing our %i, total  missing %i",missing_our_ware,missing_input_ware);
	// so our good is missing
	if(  missing_our_ware  ) {
		if(  missing_input_ware==1  ) {
			if(  eingang->get_count()-missing_input_ware==1  ) {
				// only our is missing of multiple produkts
				return 16;
			}
		}
		if(  missing_input_ware<eingang->get_count()  ) {
			// factory is already supplied with mutiple sources, but our is missing
			return 8;
		}
		return 4;
	}

	// else there is no high priority
	return 2;
}




/* guesses the income; should be improved someday ...
 * @author prissi
 */
int
spieler_t::guess_gewinn_transport_quelle_ziel(fabrik_t *qfab,const ware_t *ware,int ware_nr, fabrik_t *zfab)
{
	int gewinn=-1;
	// more checks
	if(ware->gib_typ() != warenbauer_t::nichts &&
		zfab != NULL && zfab->verbraucht(ware->gib_typ()) == 0) {

		// hat noch mehr als halbvolle Lager  and more then 35 (otherwise there might be also another transporter) ?
		if(ware->menge < ware->max*0.90  ||  ware->menge<(34<<fabrik_t::precision_bits)  ) {
			// zu wenig vorrat für profitable strecke
			return -1;
		}

		// wenn andere fahrzeuge genutzt werden muss man die werte anpassen
		// da aber später genau berechnet wird, welche Fahrzeuge am günstigsten sind => grobe schätzung ok!
		const int dist = abs(zfab->gib_pos().x - qfab->gib_pos().x) + abs(zfab->gib_pos().y - qfab->gib_pos().y);
		const int speed = 512;                  // LKW
		const int grundwert = ware->gib_preis();	// assume 3 cent per square mantenance
		if( dist > 6) {                          // sollte vernuenftige Entfernung sein

			// wie viel koennen wir tatsaechlich transportieren
			// Bei der abgabe rechnen wir einen sicherheitsfaktor 2 mit ein
			// wieviel kann am ziel verarbeitet werden, und wieviel gibt die quelle her?

			const int prod = MIN(zfab->max_produktion(),
			                 ( qfab->max_produktion() * qfab->gib_besch()->gib_produkt(ware_nr)->gib_faktor() )/256 - qfab->gib_abgabe_letzt(ware_nr) * 2 );

			gewinn = (grundwert *prod-5)*dist/128;

//			// verlust durch fahrtkosten, geschätze anzhal vehikel ist fracht/16
//			gewinn -= (int)(dist * 16 * (abs(prod)/16));   // dist * steps/planquad * kosten
			// use our importance factor too ...
			gewinn *= rating_transport_quelle_ziel(qfab,ware,zfab);
		}
	}
	return gewinn;
}



/* searches for a factory, that can be supply the target zfab
 * @author prissi
 */
int spieler_t::suche_transport_quelle(fabrik_t **qfab, int *quelle_ware, fabrik_t *zfab)
{
	int gewinn = -1;

	// keine geeignete Quelle gefunden ?
	if(zfab == NULL) {
		return -1;
	}

	// prepare iterate over all suppliers
	const vector_tpl <koord> & lieferquellen = zfab->get_suppliers();
	const int lieferquelle_anzahl=lieferquellen.get_count();

DBG_MESSAGE("spieler_t::suche_transport_quelle","Search other %i supplier for: %s",lieferquelle_anzahl,zfab->gib_name());

	// now iterate over all suppliers
	for(  int i=0;  i<lieferquelle_anzahl;  i++  ) {
		// find source
		const koord lieferquelle = lieferquellen.get(i);

		if(welt->lookup(lieferquelle)) {
			// valid koordinate?
			ding_t * dt = welt->lookup(lieferquelle)->gib_kartenboden()->obj_bei(1);
			if(dt==NULL) {
				// is already served ...
				continue;
			}
			fabrik_t * start_neu = dt->fabrik();
			// get all ware
			const vector_tpl<ware_t> *ausgang = zfab->gib_ausgang();
			const int waren_anzahl = ausgang->get_count();
			// for all products
			for(int ware_nr=0;  ware_nr<waren_anzahl ;  ware_nr++  ) {
				const ware_t ware = ausgang->get(ware_nr);
				int dieser_gewinn = guess_gewinn_transport_quelle_ziel( start_neu, &ware, ware_nr, zfab );
				// more income on this line
				if(  dieser_gewinn>gewinn  ) {
					*qfab = start_neu;
					*quelle_ware = ware_nr;
					gewinn = dieser_gewinn;
				}
			}

		}
	}

	if(gewinn>-1) {
DBG_MESSAGE("spieler_t::suche_transport_quelle","Found supplier %s (revenue %d)",(*qfab)->gib_name(),gewinn);
	}
	// no loops: will be -1!
	return gewinn;
}




/* searches for a factory, that can be supplied by the source qfab
 * @author prissi
 */
int spieler_t::suche_transport_ziel(fabrik_t *qfab, int *quelle_ware, fabrik_t **ziel)
{
	int gewinn = -1;

	// keine geeignete Quelle gefunden ?
	if(qfab == NULL) {
		return -1;
	}

	const vector_tpl<ware_t> *ausgang = qfab->gib_ausgang();

	// ist es ein erzeuger ?
	const int waren_anzahl = ausgang->get_count();

	// for all products
	for(int ware_nr=0;  ware_nr<waren_anzahl ;  ware_nr++  ) {
		const ware_t ware = ausgang->get(ware_nr);

		// hat es schon etwas produziert ?
		if(ware.menge < ware.max/4) {
			// zu wenig vorrat für profitable strecke
			continue;
		}

		// search consumer
		const vector_tpl <koord> & lieferziele = qfab->gib_lieferziele();
		const int lieferziel_anzahl=lieferziele.get_count();
DBG_MESSAGE("spieler_t::suche_transport_ziel","Lieferziele %d",lieferziel_anzahl);

		// loop for all targets
		for(  int lieferziel_nr=0;  lieferziel_nr<lieferziel_anzahl;  lieferziel_nr++  ) {
			// XXX KI sollte auch andere Lieferziele prüfen!
			const koord lieferziel = lieferziele.get(lieferziel_nr);
			int	dieser_gewinn=-1;
			fabrik_t *zfab = NULL;

			if(welt->lookup(lieferziel)) {
				ding_t * dt = welt->lookup(lieferziel)->gib_kartenboden()->obj_bei(1);
				if(dt) {
					zfab = dt->fabrik();
					dieser_gewinn = guess_gewinn_transport_quelle_ziel( qfab, &ware, ware_nr, zfab );
				}
			}
			// Is it better?
			if(dieser_gewinn > KI_MIN_PROFIT  &&  gewinn<dieser_gewinn  ) {
				// ok, this seems good or even better ...
				*quelle_ware = ware_nr;
				*ziel = zfab;
				gewinn = dieser_gewinn;
			}
		}
	}
	if(gewinn>-1) {
DBG_MESSAGE("spieler_t::suche_transport_ziel","Found factory %s (revenue: %d)",(*ziel)->gib_name(),gewinn);
	}
	// no loops: will be -1!
	return gewinn;
}






/* returns, if there is already a connection from this halt to this coordinates (upperleft/lowerright)
 * @author prissi
 */
bool
spieler_t::is_connected(halthandle_t halt, koord upperleft, koord lowerright)
{
	// check for valid handle
	if(!halt.is_bound()) {
		return false;
	}
//DBG_MESSAGE("spieler_t::is_connected()","Iteration");
	// ok, no we can proceed
	const slist_tpl<warenziel_t> *ziele = halt->gib_warenziele();
	slist_iterator_tpl<warenziel_t> iter (ziele);
	while(iter.next()) {
		warenziel_t wz = iter.get_current();
		halthandle_t a_halt = halt->gib_halt(wz.gib_ziel());
		if(a_halt.is_bound()) {
			koord pos = a_halt->gib_basis_pos();
//DBG_MESSAGE("spieler_t::is_connected()","connection to (%i,%i), compare with (%i,%i)(%i,%i)",pos.x,pos.y,upperleft.x,upperleft.y,lowerright.x,lowerright.y);
			if(upperleft.x<pos.x  &&  upperleft.y<pos.y  &&  lowerright.x>pos.x  &&  lowerright.y>pos.y) {
				// ok, we have a cennection
DBG_MESSAGE("spieler_t::is_connected()","%s is already connected to area %i,%i|%i,%i).",halt->gib_name(),upperleft.x,upperleft.y,lowerright.x,lowerright.y);
				return true;
			}
		}
	}
	return false;
}


/* return the hub of a city (always the very first stop) or zero
 * @author prissi
 */
halthandle_t
spieler_t::get_our_hub( const stadt_t *s )
{
	slist_iterator_tpl <halthandle_t> iter( halt_list );
	while(iter.next()) {
		koord h=iter.get_current()->gib_basis_pos();
		if(h.x>s->get_linksoben().x  &&  h.y>s->get_linksoben().y  &&  h.x<s->get_rechtsunten().x  &&  h.y<s->get_rechtsunten().y) {
DBG_MESSAGE("spieler_t::get_our_hub()","found %s at (%i,%i)",s->gib_name(),h.x,h.y);
			return iter.get_current();
		}
	}
	return halthandle_t();
}



/* tries to built a hub near the koordinate
 * @author prissi
 */
koord
spieler_t::built_hub( const koord pos, int radius )
{
	// no found found
	koord best_pos = koord::invalid;
	// "shortest" distance
	int dist = 999;

	for( int x=-radius;  x<=radius;  x++ ) {
		for( int y=-radius;  y<=radius;  y++ ) {
			koord try_pos=pos+koord(x,y);
			if(welt->ist_in_kartengrenzen(try_pos)) {
				const grund_t * gr = welt->lookup(try_pos)->gib_kartenboden();
				// flat, road and no other bus stop here ...
				if(gr->gib_grund_hang()==hang_t::flach  &&  (gr->gib_besitzer()==NULL || gr->gib_besitzer()==this)  ) {
					if(gr->gib_halt().is_bound()) {
						// ok, the halt is our ...
      					return try_pos;
					}
					else if(gr->gib_weg(weg_t::strasse)) {
						ribi_t::ribi  ribi = gr->gib_weg_ribi_unmasked(weg_t::strasse);
	      				if( abs(x)+abs(y)<=dist  &&  (ribi_t::nordsued==ribi  || ribi_t::ostwest==ribi  ||  ribi_t::ist_einfach(ribi))    ) {
	      					best_pos = try_pos;
	      					dist = abs(x)+abs(y);
	      				}
	      			}
	      			else if(gr->ist_natur()  &&  !gr->ist_wasser()  &&  abs(x)+abs(y)<=dist-2) {
	      				// also ok for a stop, but second choice
	      				// so wie gave it a malus of 2
      					best_pos = try_pos;
      					dist = abs(x)+abs(y)+2;
	      			}
				}
			}
		}
	}
DBG_MESSAGE("spieler_t::built_hub()","suggest hub at (%i,%i)",best_pos.x,best_pos.y);
	return best_pos;
}



/* creates a more general road transport
 * @author prissi
 */
void
spieler_t::create_bus_transport_vehikel(koord startpos2d,int anz_vehikel,koord *stops,int anzahl,bool do_wait)
{
DBG_MESSAGE("spieler_t::create_bus_transport_vehikel()","bus at (%i,%i)",startpos2d.x,startpos2d.y);
	// now start all vehicle one field before, so they load immediately
	koord3d startpos = welt->lookup(startpos2d)->gib_kartenboden()->gib_pos();

	fahrplan_t *fpl;
	// now create all vehicles as convois
	for(int i=0;  i<anz_vehikel;  i++) {

		vehikel_t * v = vehikelbauer_t::baue(welt, startpos, this,NULL, road_vehicle);

		convoi_t *cnv = new convoi_t(welt, this);
		// V.Meyer: give the new convoi name from first vehicle
		cnv->setze_name(v->gib_besch()->gib_name());
		cnv->add_vehikel( v );

		fpl = cnv->gib_vehikel(0)->erzeuge_neuen_fahrplan();

		// do not start at current stop => wont work ...
		fpl->aktuell = (stops[0]==startpos2d);
		for(int j=0;  j<anzahl;  j++) {
			fpl->eintrag.at(j).pos = welt->lookup(stops[j])->gib_kartenboden()->gib_pos();
			fpl->eintrag.at(j).ladegrad = (j==0  ||  !do_wait)?0:10;	// do not wait at hubs ...
		}
		fpl->maxi = anzahl-1;

		welt->sync_add( cnv );
		cnv->setze_fahrplan(fpl);
		cnv->start();
	}
}



/* changed to use vehicles searched before
 * @author prissi
 */
void
spieler_t::create_road_transport_vehikel(fabrik_t *qfab, int anz_vehikel)
{
	// succeed in frachthof creation
    if(hausbauer_t::frachthof_besch  &&  wkz_frachthof(this, welt, platz1)  &&  wkz_frachthof(this, welt, platz2)  ) {

     const koord3d pos1 ( welt->lookup(platz1)->gib_kartenboden()->gib_pos() );
     const koord3d pos2 ( welt->lookup(platz2)->gib_kartenboden()->gib_pos() );

	int	start_location=0;
    	// sometimes, when factories are very close, we need exakt calculation
	if(  (qfab->pos.x - platz1.x)* (qfab->pos.x - platz1.x) + (qfab->pos.y - platz1.y)*(qfab->pos.y - platz1.y)  >
	  	(qfab->pos.x - platz2.x)*(qfab->pos.x - platz2.x) + (qfab->pos.y - platz2.y)*(qfab->pos.y - platz2.y)  ) {
	  	start_location = 1;
	 }

	// calculate vehicle start position
     koord3d startpos=(start_location==0)?pos1:pos2;
	ribi_t::ribi w_ribi = welt->lookup(startpos)->gib_weg_ribi_unmasked(weg_t::strasse);
	// now start all vehicle one field before, so they load immediately
	startpos = welt->lookup(koord(startpos.gib_2d())+koord(w_ribi))->gib_kartenboden()->gib_pos();

	fahrplan_t *fpl;
	// now create all vehicles as convois
	for(int i=0;  i<anz_vehikel;  i++) {

	    vehikel_t * v = vehikelbauer_t::baue(welt, startpos, this,NULL, road_vehicle);

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
	    fpl->eintrag.at(start_location).ladegrad = 100;

	    welt->sync_add( cnv );
	    cnv->setze_fahrplan(fpl);
	    cnv->start();
	    }
	}
}



/* now obeys timeline and use "more clever" scheme for vehicle selection *
 * @author prissi
 */
void
spieler_t::create_rail_transport_vehikel(const koord platz1, const koord platz2, int anz_vehikel, int ladegrad)
{

    fahrplan_t *fpl;
    convoi_t *cnv = new convoi_t(welt, this);
   const koord3d pos1 ( welt->lookup(platz1)->gib_kartenboden()->gib_pos() );
   const koord3d pos2 ( welt->lookup(platz2)->gib_kartenboden()->gib_pos() );

    // lokomotive bauen
    unsigned month_now = (welt->gib_zeit_ms() >> welt->ticks_bits_per_tag) + (umgebung_t::starting_year * 12);
    if(  umgebung_t::use_timeline == false   ) {
    		month_now = 0xFFFFFFFFu;
    	}

    	// probably need to electrify the track?
	if(  rail_engine->get_engine_type()==vehikel_besch_t::electric  ) {
		// we need overhead wires
		DBG_MESSAGE( "spieler_t::create_rail_transport_vehikel","Spieler needs to eletrify track.");
		blockmanager * bm = blockmanager::gib_manager();
		bm->setze_tracktyp( welt, pos1, 1 );
	}
   vehikel_t * v = vehikelbauer_t::baue(welt, pos2, this, NULL, rail_engine);
    // V.Meyer: give the new convoi name from first vehicle
    cnv->setze_name(rail_engine->gib_name());
    cnv->add_vehikel( v );

	/* now we add cars:
	 * check here also for introduction years
	 */
    for(int i = 0; i < anz_vehikel; i++) {
    	// use the vehicle we searched before
	vehikel_t * v = vehikelbauer_t::baue(welt, pos2, this,NULL,rail_vehicle);
	cnv->add_vehikel( v );
    }

    fpl = cnv->gib_vehikel(0)->erzeuge_neuen_fahrplan();

    fpl->aktuell = 0;
    fpl->maxi = 1;
    fpl->eintrag.at(0).pos = pos1;
    fpl->eintrag.at(0).ladegrad = ladegrad;
    fpl->eintrag.at(1).pos = pos2;
    fpl->eintrag.at(1).ladegrad = 0;

    cnv->setze_fahrplan(fpl);
    welt->sync_add( cnv );
    cnv->start();
}

bool
spieler_t::suche_platz1_platz2(fabrik_t *qfab, fabrik_t *zfab)
{
    DBG_MESSAGE("spieler_t::suche_platz1_platz2()",
		 "searching locations for stops/stations");

    const koord3d start ( qfab->gib_pos() );
    const koord3d ziel ( zfab->gib_pos() );

    bool ok = false;

    if(  suche_platz(start.x, start.y, ziel.x, ziel.y, qfab->gib_besch()->gib_haus()->gib_groesse(), &platz1) ) {
    	// found a place, search for target
	    ok = suche_platz(ziel.x, ziel.y, platz1.x, platz1.y, zfab->gib_besch()->gib_haus()->gib_groesse(), &platz2);
	}

    INT_CHECK("simplay 969");

    if( !ok ) {   // keine Bauplaetze gefunden
        DBG_MESSAGE("spieler_t::suche_platz1_platz2()",
	             "no suitable locations found");
    } else {
	// plaetz reservieren
	zeiger_t *z;
	grund_t *gr;

        DBG_MESSAGE("spieler_t::suche_platz1_platz2()",
	             "platz1=%d,%d platz2=%d,%d",
		     platz1.x, platz1.y, platz2.x, platz2.y);

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

	bauigel.route_fuer( wegbauer_t::strasse, road_weg );
	bauigel.baubaer = true;
	// we won't destroy cities (and save the money)
	bauigel.set_keep_existing_faster_ways(true);
	bauigel.set_maximum(10000);

	INT_CHECK("simplay 846");

	bauigel.calc_route(platz1,platz2);

	// Strasse muss min. 3 Felder lang sein, sonst kann man keine
	// zwei verschiedene stops bauen

	if(bauigel.max_n > 1) {
DBG_MESSAGE("spieler_t::create_simple_road_transport()","building simple road from %d,%d to %d,%d",platz1.x, platz1.y, platz2.x, platz2.y);
		bauigel.baue();
		return true;
	}
	else {
		// is there already a connection
		{
      		vehikel_t *test_driver=new automobil_t(welt,koord3d(platz1,0),road_vehicle,this,NULL);;
			route_t verbindung;
			if(	verbindung.calc_route(welt,welt->lookup(platz1)->gib_kartenboden()->gib_pos(),welt->lookup(platz2)->gib_kartenboden()->gib_pos(),(fahrer_t *)test_driver)  &&
				verbindung.gib_max_n()*2<3*abs_distance(platz1,platz2))  {
DBG_MESSAGE("spieler_t::create_simple_road_transport()","But connection between %d,%d to %d,%d is only %i",platz1.x, platz1.y, platz2.x, platz2.y, verbindung.gib_max_n() );
				// found something with the nearly same lenght
				delete test_driver;
				return true;
			}
			delete test_driver;
		}
		// built a route with this place in between (usually sucessful, but not straightest
		koord mitte=(platz1+platz2)/2+koord(simrand(4)-2,simrand(4)-2);
		INT_CHECK("simplay 2098");
		bauigel.baubaer = false;
		bauigel.calc_route(platz1,mitte);
		// found half way
		if(bauigel.max_n > 1) {
			wegbauer_t bauhase(welt, this);

			bauhase.route_fuer( wegbauer_t::strasse, road_weg );
			bauhase.baubaer = false;
			bauhase.set_keep_existing_faster_ways(true);
			bauhase.set_maximum(10000);

			INT_CHECK("simplay 2109");
			bauhase.calc_route(platz2,mitte);
			INT_CHECK("simplay 2111");
			// found something back from here
			if(bauhase.max_n > 1) {
				// just remove doubles
				while(bauigel.max_n>0  &&   bauhase.max_n>0  &&   bauigel.gib_route_bei(bauigel.max_n)==bauhase.gib_route_bei(bauhase.max_n)) {
					mitte = bauigel.gib_route_bei(bauigel.max_n);
					bauigel.max_n --;
					bauhase.max_n --;
				}
				// in priciple, now we need to make the route simpler ...
			    slist_tpl <koord> list;
				// now add everything to road
				for(  int i=0;  i<=bauigel.max_n;  i++  ) {
					list.insert( bauigel.gib_route_bei(i) );
				}
				list.insert( mitte );
				// now add everything to road
				for(  int i=bauhase.max_n;  i>=0;  i--  ) {
					list.insert( bauhase.gib_route_bei(i) );
				}
				// then built this road ...
				bauigel.baue_strecke(list);
DBG_MESSAGE("spieler_t::create_simple_road_transport()","building simple road from %d,%d to %d,%d by half waypoint",platz1.x, platz1.y, platz2.x, platz2.y);
				return true;
			}
		}
DBG_MESSAGE("spieler_t::create_simple_road_transport()","building simple road from %d,%d to %d,%d failed",platz1.x, platz1.y, platz2.x, platz2.y);
		return false;
	}
}

bool
spieler_t::create_complex_road_transport()
{
    INT_CHECK("simplay 865");

    wegbauer_t bauigel(welt, this);

    INT_CHECK("simplay 867");

    bauigel.baubaer = true;
    bauigel.set_keep_existing_faster_ways(true);
    bauigel.set_maximum(32000);
    bauigel.route_fuer( wegbauer_t::strasse_bot, 0 );

    bauigel.calc_route(platz1,platz2);

    INT_CHECK("simplay 768");

    // Strasse muss min. 3 Felder lang sein, sonst kann man keine
    // zwei verschiedene stops bauen

    if(bauigel.max_n <= 3) {
        DBG_MESSAGE("spieler_t::create_complex_road_transport()",
		     "no suitable route was found, aborting roadbuilding");
	return false;
    }


    slist_tpl <koord> list;

    list.insert( platz1 );

    for(int i=1; i<bauigel.max_n-1; i++) {
	koord p = bauigel.gib_route_bei(i);
        grund_t *bd = welt->lookup(p)->gib_kartenboden();


        if(bd->gib_weg(weg_t::schiene) != NULL) {
            DBG_MESSAGE("spieler_t::create_complex_road_transport()",
	                 "try to cross railroad track");

	    ribi_t::ribi ribi = bd->gib_weg_ribi(weg_t::schiene);

		INT_CHECK("simplay 2415");
	    const bool ok = versuche_brueckenbau(p, &i, ribi, bauigel, list);

	    if(!ok) {
	      DBG_MESSAGE("spieler_t::create_complex_road_transport()",
			   "bridge failed, aborting roadbuilding");
	      return false;
	    } else {
	      DBG_MESSAGE("spieler_t::create_complex_road_transport()",
			   "using bridge");
	    }
	}
    }

    INT_CHECK("simplay 895");

    bauigel.route_fuer( wegbauer_t::strasse, road_weg );

    list.insert( platz2 );

    bauigel.baubaer = true;

    if(!checke_streckenbau(bauigel, list)) {
        DBG_MESSAGE("spieler_t::create_complex_road_transport()",
		     "checke_streckenbau() returned false, aborting roadbuilding");
	return false;
    }

    INT_CHECK("simplay 2443");
    bauigel.baue_strecke(list);

    slist_iterator_tpl <koord> iter (list);

    if(iter.next()) {

      while(iter.next()) {
	const koord k = iter.get_current();

	if(!iter.next()) {
	  break;
	}

//	was brueckenbauer_t::baue(this, welt, k, weg_t::sschiene);
	brueckenbauer_t::baue(this, welt, k, weg_t::strasse,road_weg->gib_topspeed());
      }
    }
    return true;
}



/* built a very simple track with just the minimum effort
 * usually good enough, since it can use road crossings
 * @author prissi
 */
bool
spieler_t::create_simple_rail_transport()
{
	wegbauer_t bauigel(welt, this);
	bauigel.route_fuer( wegbauer_t::schiene_bot_bau, rail_weg );
	bauigel.set_keep_existing_ways(false);
	bauigel.baubaer = false;	// no tunnels, no bridges
	bauigel.calc_route(platz1,platz2);
	INT_CHECK("simplay 2478");

	if(bauigel.max_n > 3) {
DBG_MESSAGE("spieler_t::create_simple_rail_transport()","building simple track from %d,%d to %d,%d",platz1.x, platz1.y, platz2.x, platz2.y);
		bauigel.baue();
		INT_CHECK("simplay 2480");
		return true;
	}
	else {
		bauigel.route_fuer( wegbauer_t::schiene_bot_bau, rail_weg );
		bauigel.set_keep_existing_ways(false);
		bauigel.baubaer = true;	// ok, try tunnels and bridges
		bauigel.calc_route(platz1,platz2);
		INT_CHECK("simplay 2478");
		if(bauigel.max_n > 3) {
DBG_MESSAGE("spieler_t::create_simple_rail_transport()","building simple track (baubaer) from %d,%d to %d,%d",platz1.x, platz1.y, platz2.x, platz2.y);
			bauigel.baue();
			INT_CHECK("simplay 2493");
			return true;
		}
		else {
DBG_MESSAGE("spieler_t::create_simple_rail_transport()","building simple track from %d,%d to %d,%d failed", platz1.x, platz1.y, platz2.x, platz2.y);
			return false;
		}
	}
}



bool
spieler_t::create_complex_rail_transport()
{
    DBG_MESSAGE("spieler_t::create_complex_rail_transport()",
		 "try to build complex railroads");

    INT_CHECK("simplay 957");

    wegbauer_t bauigel(welt, this);

    INT_CHECK("simplay 961");

    bauigel.baubaer = false;
    // was schiene_bot
    bauigel.route_fuer( wegbauer_t::schiene_bot, 0 );

    bauigel.calc_route(platz1, platz2);

    INT_CHECK("simplay 968");

    // Strecke muss min. 3 Felder lang sein, sonst kann man keine
    // zwei verschiedene stops bauen

    if(bauigel.max_n < 3) {
        DBG_MESSAGE("spieler_t::create_complex_rail_transport()",
		     "no suitable route was found, aborting trackbuilding");

	return false;
    }

    slist_tpl <koord> list;

    list.insert( platz1 );

    for(int i=1; i<bauigel.max_n-1; i++) {
	koord p = bauigel.gib_route_bei(i);
        grund_t *bd = welt->lookup(p)->gib_kartenboden();


	if(  bd->gib_weg(weg_t::strasse) != NULL  ||  bd->gib_weg(weg_t::schiene) != NULL) {
            DBG_MESSAGE("spieler_t::create_complex_rail_transport()","try to cross way");

	    ribi_t::ribi ribi = 0;

	    bool ok = false;

	    if(bd->gib_weg(weg_t::schiene)) {
	      ribi = bd->gib_weg_ribi(weg_t::schiene);
	    } else {
	      ribi = bd->gib_weg_ribi(weg_t::strasse);
	      // perhaps we can built a crossing?
		if(  i>1  &&  (bd->gib_besitzer()==0  ||  bd->gib_besitzer()==this)  &&  bd->gib_halt()==NULL) {
		      ribi_t::ribi ribi_route = ribi_typ(bauigel.gib_route_bei(i+1),bauigel.gib_route_bei(i-1));
DBG_MESSAGE("spieler_t::create_complex_rail_transport()","crossing ribi $%02x mit $%02x",ribi_route,ribi);
			ok = 	ribi_t::ist_gerade(ribi)
					&&  !ribi_t::ist_einfach(ribi)
					&&  ribi_t::ist_gerade(ribi_route)
					&&  (ribi & ribi_route)==0;
DBG_MESSAGE("spieler_t::create_complex_rail_transport()","railroad crossing is%s possible",ok==true?" ":" not");
	    	}
	    }

	    if(!ok) {
	    	// we must built a bridge
		ok = versuche_brueckenbau(p, &i, ribi, bauigel, list);
		}

	    if(!ok) {
	      DBG_MESSAGE("spieler_t::create_complex_rail_transport()","bridge failed, aborting trackbuilding");
	      return false;
	    } else {
	      DBG_MESSAGE("spieler_t::create_complex_rail_transport()","using bridge/crossing");
	    }
	}
    }

    INT_CHECK("simplay 980");

    list.insert( koord(platz2) );

    bauigel.route_fuer( wegbauer_t::schiene_bot_bau, rail_weg );
    bauigel.baubaer = true;

    if(!checke_streckenbau(bauigel, list)) {
        DBG_MESSAGE("spieler_t::create_complex_rail_transport()",
		     "checke_streckenbau() returned false, aborting trackbuilding");
	return false;
    }

    bauigel.baue_strecke(list);

    slist_iterator_tpl <koord> iter (list);

    if(iter.next()) {

      while(iter.next()) {
	const koord k = iter.get_current();

	if(!iter.next()) {
	  break;
	}

	brueckenbauer_t::baue(this, welt, k, weg_t::schiene,rail_weg->gib_topspeed());
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
spieler_t::versuche_brueckenbau(koord p, int *index, ribi_t::ribi ribi,
                                wegbauer_t &bauigel,
				slist_tpl <koord> &list)
{
	bool ok = true;

	const koord p1 = bauigel.gib_route_bei(*index-1);
	if(  welt->get_slope(p1)!=0 ) {	// only on flat ground
		return false;
	}

	// find an end; must be straight and on flat unowned ground!
	for(  int i=*index+1;  i<bauigel.max_n-2;  i++  )
	{
		const koord p2 = bauigel.gib_route_bei(i+1);
		const grund_t *bd = welt->lookup(p2)->gib_kartenboden();
		koord zv = p2 - p1;

		if(  (zv.x != 0 && zv.y != 0) ||     // Hajo: only straight bridges possible
			welt->get_slope(p2)!=0  ||
			bd==NULL  ) {
			return false;    // Hajo: only straight bridges possible with no height difference at the moment
		}
		if(  !bd->ist_natur()  ) {
			// not empty, try next!
			continue;
		}

		// Hajo: Bridge might be possible, check if
		// way ends are straight.

		zv.x = sgn(zv.x);
		zv.y = sgn(zv.y);

		ok &= bauigel.gib_route_bei(*index-2) == p1 - zv;
		ok &= bauigel.gib_route_bei(i+1) == p2 + zv;

		if(ok) {
			list.insert( p1 );
			list.insert( p2 );
			*index = i;
			return true;
		}
	}
	// gets here, if no bridge possible
	return false;
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
	int start_index=-1;
	int ziel_index=-1;

	// prepare for saving by default variables
	if(file->is_saving()) {
		halt_count = halt_list->count();
DBG_DEBUG("spieler_t::rdwr()","%i has %i halts.",welt->sp2num( this ),halt_count);
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
	}
	else {
		file->rdwr_int(haltcount, " ");
	}

	// from here on loading financial stuff
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
		for (int year = 0;year<MAX_HISTORY_YEARS;year++) {
			for (int cost_type = 0; cost_type<MAX_COST; cost_type++) {
				finance_history_year[year][cost_type] = 0;
			}
		}
		for (int month = 0;month<MAX_HISTORY_MONTHS;month++) {
			for (int cost_type = 0; cost_type<MAX_COST; cost_type++) {
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
			}
			else {
				finance_history_year[0][cost_type] = 0;
				finance_history_year[1][cost_type] = 0;
			}
		}
	}
	else 	if (file->get_version() < 84008) {
		// not so old save game
		for (int year = 0;year<MAX_HISTORY_YEARS;year++) {
			for (int cost_type = 0; cost_type<MAX_COST; cost_type++) {
				if (file->get_version() < 84007) {
					// a cost_type has has been added. For old savegames we only have 9 cost_types, now we have 10.
					// for old savegames only load 9 types and calculate the 10th; for new savegames load all 10 values
					if (cost_type < 9) {
						file->rdwr_longlong(finance_history_year[year][cost_type], " ");
					} else {
						finance_history_year[year][COST_OPERATING_PROFIT] = finance_history_year[year][COST_INCOME] + finance_history_year[year][COST_VEHICLE_RUN] + finance_history_year[year][COST_MAINTENANCE];
						finance_history_year[year][COST_MARGIN] = (finance_history_year[year][COST_VEHICLE_RUN] + finance_history_year[year][COST_MAINTENANCE]) != 0 ? finance_history_year[year][COST_OPERATING_PROFIT] * 100 / abs((finance_history_year[year][COST_VEHICLE_RUN] + finance_history_year[year][COST_MAINTENANCE])) : 0;
						finance_history_year[year][COST_TRANSPORTED_GOODS] = 0;
					}
				} else {
					if (cost_type < 10) {
						file->rdwr_longlong(finance_history_year[year][cost_type], " ");
					} else {
						finance_history_year[year][COST_MARGIN] = (finance_history_year[year][COST_VEHICLE_RUN] + finance_history_year[year][COST_MAINTENANCE]) != 0 ? finance_history_year[year][COST_OPERATING_PROFIT] * 100 / abs((finance_history_year[year][COST_VEHICLE_RUN] + finance_history_year[year][COST_MAINTENANCE])) : 0;
						finance_history_year[year][COST_TRANSPORTED_GOODS] = 0;
					}
				}
			}
//DBG_MESSAGE("player_t::rdwr()", "finance_history[year=%d][cost_type=%d]=%ld", year, cost_type,finance_history_year[year][cost_type]);
		}
	}
	else if (file->get_version() < 86000) {
		for (int year = 0;year<MAX_HISTORY_YEARS;year++) {
			for (int cost_type = 0; cost_type<10; cost_type++) {
				file->rdwr_longlong(finance_history_year[year][cost_type], " ");
			}
			finance_history_year[year][COST_MARGIN] = (finance_history_year[year][COST_VEHICLE_RUN] + finance_history_year[year][COST_MAINTENANCE]) != 0 ? finance_history_year[year][COST_OPERATING_PROFIT] * 100 / abs((finance_history_year[year][COST_VEHICLE_RUN] + finance_history_year[year][COST_MAINTENANCE])) : 0;
			finance_history_year[year][COST_TRANSPORTED_GOODS] = 0;
		}
		// in 84008 monthly finance history was introduced
		for (int month = 0;month<MAX_HISTORY_MONTHS;month++) {
			for (int cost_type = 0; cost_type<10; cost_type++) {
				file->rdwr_longlong(finance_history_month[month][cost_type], " ");
			}
			finance_history_month[month][COST_MARGIN] = (finance_history_month[month][COST_VEHICLE_RUN] + finance_history_month[month][COST_MAINTENANCE]) != 0 ? finance_history_month[month][COST_OPERATING_PROFIT] * 100 / abs((finance_history_month[month][COST_VEHICLE_RUN] + finance_history_month[month][COST_MAINTENANCE])) : 0;
			finance_history_month[month][COST_TRANSPORTED_GOODS] = 0;
		}
	}
	else {
		// most recent savegame version
		for (int year = 0;year<MAX_HISTORY_YEARS;year++) {
			for (int cost_type = 0; cost_type<MAX_COST; cost_type++) {
				file->rdwr_longlong(finance_history_year[year][cost_type], " ");
			}
		}
		for (int month = 0;month<MAX_HISTORY_MONTHS;month++) {
			for (int cost_type = 0; cost_type<MAX_COST; cost_type++) {
				file->rdwr_longlong(finance_history_month[month][cost_type], " ");
			}
		}
	}

	file->rdwr_bool(automat, "\n");

	zustand dummy=NEUE_ROUTE;
	file->rdwr_enum(dummy, " ");
	subzustand subdummy=NR_INIT;
	file->rdwr_enum(subdummy, "\n");
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
	if(dummy != NEUE_ROUTE ) {
		//  may happens, when loading old games
		dbg->warning("spieler_t::rdwr()", "State is out of bounds: %d -> corrupt savegame?", state);
	}
	if(subdummy < NR_INIT || subdummy > NR_ROAD_SUCCESS) {
		// may happen, when loading old games
		dbg->warning("spieler_t::rdwr()", "Substate is out of bounds: %d -> corrupt savegame?", substate);
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
DBG_MESSAGE("spieler_t::rdwr","Save ok");
	}
	else {
		// loading
		start = NULL;
		if(start_index != -1) {
			start = welt->gib_fab(start_index);
		}
		last_start = NULL;
		ziel = NULL;
		if(ziel_index != -1) {
			ziel = welt->gib_fab(ziel_index);
		}
		start_stadt = end_stadt = NULL;
		end_ausflugsziel = NULL;

		// @author prissi
		// go back to initial search state
		substate = NR_INIT;
		count_rail = count_road = 255;
		baue = false;
		baue_start = baue_ziel = NULL;

		// last destinations unknown
		last_ziel = NULL;
		rail_engine = NULL;
		rail_vehicle = NULL;
		road_vehicle = NULL;
		rail_weg = NULL;
		road_weg = NULL;

DBG_DEBUG("spieler_t::rdwr()","player %i: loading %i halts.",welt->sp2num( this ),halt_count);
		for(int i=0; i<halt_count; i++) {
			halthandle_t halt = haltestelle_t::create( welt, file );
			halt->laden_abschliessen();
			halt_list->insert( halt );
		}
		init_texte();

		// empty undo buffer
		init_undo(weg_t::strasse,0);
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
	if(welt->gib_spieler(0)==this) {
		char buf[128];

		sprintf(buf, translator::translate("!0_STATION_CROWDED"), halt->gib_name());
//        ticker_t::get_instance()->add_msg(buf, halt->gib_basis_pos(), DUNKELROT);
		message_t::get_instance()->add_message(buf, halt->gib_basis_pos(),message_t::full, kennfarbe,IMG_LEER);
	}
}



/**
 * Rückruf, um uns zu informieren, dass ein Vehikel ein Problem hat
 * @author Hansjörg Malthaner
 * @date 26-Nov-2001
 */
void spieler_t::bescheid_vehikel_problem(convoihandle_t cnv,const koord3d ziel)
{
DBG_MESSAGE("spieler_t::bescheid_vehikel_problem","Vehicle %s can't find a route to (%i,%i)!", cnv->gib_name(),ziel.x,ziel.y);
	if(welt->gib_spieler(0)==this) {
		char buf[256];
		sprintf(buf,"Vehicle %s can't find a route!", cnv->gib_name());
//		ticker_t::get_instance()->add_msg(buf, cnv->gib_pos().gib_2d(),ROT);
		message_t::get_instance()->add_message(buf, cnv->gib_pos().gib_2d(),message_t::convoi,kennfarbe,cnv->gib_vehikel(0)->gib_basis_bild());
	}
}



/* Here functions for UNDO
 * @date 7-Feb-2005
 * @author prissi
 */

void
spieler_t::init_undo( weg_t::typ wtype, int max )
{
	if(kennfarbe>=4) {
		// this is an KI
		last_built = NULL;
		return;
	}
	// only human player
	// prissi: allow for UNDO for real player
	if(last_built) {
		delete last_built;
		last_built = NULL;
	}
DBG_MESSAGE("spieler_t::int_undo()","undo tiles %i",max);
	last_built_count = 0;
	if(max>0) {
		last_built = new array_tpl<koord> (max+1);
		undo_type = wtype;
	}

}


void
spieler_t::add_undo(koord k)
{
	if(last_built!=NULL) {
DBG_DEBUG("spieler_t::add_undo()","tile %i at (%i,%i)",last_built_count,k.x,k.y);
		assert(last_built_count<last_built->get_size());
		last_built->at(last_built_count++) = k;
	}
}



bool
spieler_t::undo()
{
	if(last_built_count==0  ||  last_built==NULL) {
		// nothing to UNDO
		return false;
	}
	// try to remove everything last built
	bool ok = false;
	for(int i=0;  i<last_built_count;  i++  ) {
		grund_t *gr = welt->lookup(last_built->at(i))->gib_kartenboden();
		ok |= gr->weg_entfernen(undo_type,true);
DBG_DEBUG("spieler_t::add_undo()","undo tile %i at (%i,%i)",i,last_built->at(i).x,last_built->at(i).y);
	}
	delete last_built;
	last_built = NULL;
	return ok;
}
