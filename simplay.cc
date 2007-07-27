/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 *
 * Renovation in dec 2004 for other vehicles, timeline
 * @author prissi
 */

/* number of lines to check (must be =>3 */
#define KI_TEST_LINES 50
/* No lines will be considered below */
#define KI_MIN_PROFIT 5000
// this does not work well together with 128er pak
//#ifdef EARLY_SEARCH_FOR_BUYER
// this make KI2 try passenger transport
#define TRY_PASSENGER

// Costs for AI estimate calculations only (not booked in the game)
#define CST_STRASSE -10000
#define CST_MEDIUM_VEHIKEL -250000
#define CST_FRACHTHOF -150000

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
#include "besch/sound_besch.h"
#include "besch/weg_besch.h"
#include "simworld.h"
#include "simplay.h"
#include "simwerkz.h"
#include "simfab.h"
#include "simhalt.h"
#include "simvehikel.h"
#include "simconvoi.h"
#include "simsound.h"
#include "simtools.h"
#include "simmesg.h"
#include "simware.h"

#include "simintr.h"

#include "simimg.h"
#include "simcolor.h"

#include "simwin.h"
#include "gui/messagebox.h"

#include "dings/zeiger.h"
#include "dings/gebaeude.h"
#include "dings/wayobj.h"

#include "dataobj/fahrplan.h"
#include "dataobj/umgebung.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"
#include "dataobj/einstellungen.h"
#include "bauer/hausbauer.h"
#include "bauer/vehikelbauer.h"
#include "bauer/warenbauer.h"
#include "bauer/brueckenbauer.h"
#include "bauer/tunnelbauer.h"
#include "bauer/wegbauer.h"

#include "utils/simstring.h"

#include "simgraph.h"

#include "gui/money_frame.h"
#include "gui/schedule_list.h"

#include "simcity.h"

// true, if line contruction is under way
// our "semaphore"
static bool baue;
static fabrik_t *baue_start=NULL;
static fabrik_t *baue_ziel=NULL;


spieler_t::spieler_t(karte_t *wl, uint8 nr) :
	simlinemgmt(wl)
{
	welt = wl;
	player_nr = nr;
	set_player_color( nr*8, nr*8+24 );

	konto = umgebung_t::starting_money;

	konto_ueberzogen = 0;
	automat = false;		// Start nicht als automatischer Spieler

	headquarter_pos = koord::invalid;
	headquarter_level = 0;

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

	state = NR_INIT;

	maintenance = 0;
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
	ship_vehicle = NULL;
	road_weg = NULL;
	baue = false;

	steps = simrand(16);

	init_texte();

	money_frame = NULL;
	line_frame = NULL;

	passenger_transport = true;

	// we have different AI, try to find out our type:
	sprintf(spieler_name_buf,"player %i",player_nr-1);
	if(player_nr>=2) {
		// @author prissi
		// first set default
		passenger_transport = false;
		road_transport = true;
		rail_transport = true;
		ship_transport = false;
		// set the different AI
		switch(player_nr) {
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



spieler_t::~spieler_t()
{
	// maybe free money frame
	if(money_frame!=NULL) {
		delete money_frame;
	}
	money_frame = NULL;
	// maybe free line frame
	if(line_frame!=NULL) {
		delete line_frame;
	}
	line_frame = NULL;
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



/* returns the line dialoge of a player
 * @author prissi
 */
schedule_list_gui_t *
spieler_t::get_line_frame(void)
{
	if(line_frame==NULL) {
		line_frame = new schedule_list_gui_t(this);
	}
	return line_frame;
}



/* returns the name of the player; "player -1" sits in front of the screen
 * @author prissi
 */
const char* spieler_t::gib_name(void) const
{
	return translator::translate(spieler_name_buf);
}



void spieler_t::init_texte()
{
	// these are the numbers => maximum 50 number displayed per player
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
void spieler_t::display_messages()
{
	const sint16 raster = get_tile_raster_width();
	int last_displayed_message = -1;
	const sint16 yoffset = welt->gib_y_off()+((display_get_width()/raster)&1)*(raster/4);

	for(int n=0; n<=last_message_index; n++) {
		if(text_alter[n] >= -80) {
			const koord ij = text_pos[n]-welt->get_world_position()-welt->gib_ansicht_ij_offset();
			const sint16 x = (ij.x-ij.y)*(raster/2) + welt->gib_x_off();
			const sint16 y = (ij.x+ij.y)*(raster/4) + (text_alter[n] >> 4) - tile_raster_scale_y( welt->lookup_hgt(text_pos[n])*TILE_HEIGHT_STEP, raster) + yoffset;

			display_proportional_clip( x+1, y+1, texte[n], ALIGN_LEFT, COL_BLACK, true);
			display_proportional_clip( x, y, texte[n], ALIGN_LEFT, PLAYER_FLAG|(kennfarbe1+3), true);
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
spieler_t::age_messages(long /*delta_t*/)
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



void spieler_t::set_player_color(uint8 col1, uint8 col2)
{
	kennfarbe1 = col1;
	kennfarbe2 = col2;
	display_set_player_color_scheme( player_nr, col1, col2 );
}



/**
 * Wird von welt in kurzen abständen aufgerufen
 * @author Hj. Malthaner
 */
void spieler_t::step()
{
	steps ++;

	if(automat  &&  (steps%MAX_PLAYER_COUNT)==player_nr) {
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
void spieler_t::neuer_monat()
{
	static char buf[256];

	// Wartungskosten abziehen
	buche( -((sint64)maintenance) <<((sint64)welt->ticks_bits_per_tag-18ll), COST_MAINTENANCE);
	calc_finance_history();
	roll_finance_history_month();
	simlinemgmt.new_month();

	// Bankrott ?
	if(!umgebung_t::freeplay) {
		if(konto < 0) {
			konto_ueberzogen++;

			if(this == welt->gib_spieler(0)) {
				if(konto_ueberzogen == 1) {
					// tell the player
					char buf[256];
					sprintf(buf,translator::translate("Verschuldet:\n\nDu hast %d Monate Zeit,\ndie Schulden zurueckzuzahlen.\n"), MAX_KONTO_VERZUG-konto_ueberzogen+1 );
					message_t::get_instance()->add_message(buf,koord::invalid,message_t::problems,player_nr,IMG_LEER);
				} else if(konto_ueberzogen <= MAX_KONTO_VERZUG) {
					sprintf(buf,translator::translate("Verschuldet:\n\nDu hast %d Monate Zeit,\ndie Schulden zurueckzuzahlen.\n"), MAX_KONTO_VERZUG-konto_ueberzogen+1);
					message_t::get_instance()->add_message(buf,koord::invalid,message_t::problems,player_nr,IMG_LEER);
				} else {
					destroy_all_win();
					create_win(280, 40, new news_img("Bankrott:\n\nDu bist bankrott.\n"), w_autodelete);
					welt->beenden(false);
				}
			}
		} else {
			konto_ueberzogen = 0;
		}
	}
}



/**
 * Methode fuer jaehrliche Aktionen
 * @author Hj. Malthaner, Owen Rudge, hsiegeln
 */
void spieler_t::neues_jahr()
{
	calc_finance_history();
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
	for (i=MAX_HISTORY_MONTHS-1; i>0; i--) {
		for (int cost_type = 0; cost_type<MAX_COST; cost_type++) {
			finance_history_month[i][cost_type] = finance_history_month[i-1][cost_type];
		}
	}
	for (int i=0;i<MAX_COST;i++) {
		finance_history_month[0][i] = 0;
	}
	finance_history_year[0][COST_ASSETS] = 0;
}



void spieler_t::roll_finance_history_year()
{
	int i;
	for (i=MAX_HISTORY_YEARS-1; i>0; i--) {
		for (int cost_type = 0; cost_type<MAX_COST; cost_type++) {
			finance_history_year[i][cost_type] = finance_history_year[i-1][cost_type];
		}
	}
	for (int i=0;i<MAX_COST;i++) {
		finance_history_year[0][i] = 0;
	}
	finance_history_month[0][COST_ASSETS] = 0;
}



void spieler_t::calc_finance_history()
{
	/**
	* copy finance data into historical finance data array
	* @author hsiegeln
	*/
	sint64 profit, assets, mprofit;
	profit = assets = mprofit = 0;
	for (int i=0; i<MAX_COST; i++) {
		// all costs < COST_ASSETS influence profit, so we must sum them up
		if(i<COST_ASSETS) {
			profit += finance_history_year[0][i];
			mprofit += finance_history_month[0][i];
		}
	}

	if(finance_history_month[0][COST_ASSETS]==0) {
		// new month has started
		for (vector_tpl<convoihandle_t>::const_iterator i = welt->convois_begin(), end = welt->convois_end(); i != end; ++i) {
			convoihandle_t cnv = *i;
			if(cnv->gib_besitzer()==this) {
				assets += cnv->calc_restwert();
			}
		}
		finance_history_year[0][COST_ASSETS] = finance_history_month[0][COST_ASSETS] = assets;
	}

	finance_history_year[0][COST_PROFIT] = profit;
	finance_history_month[0][COST_PROFIT] = mprofit;

	finance_history_year[0][COST_NETWEALTH] = finance_history_year[0][COST_ASSETS] + konto;
	finance_history_year[0][COST_CASH] = konto;
	finance_history_year[0][COST_OPERATING_PROFIT] = finance_history_year[0][COST_INCOME] + finance_history_year[0][COST_VEHICLE_RUN] + finance_history_year[0][COST_MAINTENANCE];
	sint64 margin_div = (finance_history_year[0][COST_VEHICLE_RUN] + finance_history_year[0][COST_MAINTENANCE]);
	if(margin_div<0) { margin_div = -margin_div; }
	finance_history_year[0][COST_MARGIN] = margin_div!= 0 ? (100*finance_history_year[0][COST_OPERATING_PROFIT]) / margin_div : 0;

	finance_history_month[0][COST_NETWEALTH] = finance_history_month[0][COST_ASSETS] + konto;
	finance_history_month[0][COST_CASH] = konto;
	finance_history_month[0][COST_OPERATING_PROFIT] = finance_history_month[0][COST_INCOME] + finance_history_month[0][COST_VEHICLE_RUN] + finance_history_month[0][COST_MAINTENANCE];
	margin_div = (finance_history_month[0][COST_VEHICLE_RUN] + finance_history_month[0][COST_MAINTENANCE]);
	if(margin_div<0) { margin_div = -margin_div; }
	finance_history_month[0][COST_MARGIN] = margin_div!= 0 ? (100*finance_history_month[0][COST_OPERATING_PROFIT]) / margin_div : 0;
}



// add and amount, including the display of the message and some other things ...
sint64 spieler_t::buche(const sint64 betrag, const koord pos, const int type)
{
	buche(betrag, type);

	if(betrag != 0) {
		add_message(pos, betrag);

		if(!(labs((sint32)betrag)<=10000)) {
			struct sound_info info;

			info.index = SFX_CASH;
			info.volume = 255;
			info.pri = 0;

			welt->play_sound_area_clipped(pos, info);
		}
	}
	return konto;
}



// add an amout to a subcategory
sint64 spieler_t::buche(const sint64 betrag, int type)
{
	konto += betrag;

	if(type < MAX_COST) {
		// fill year history
		finance_history_year[0][type] += betrag;
		finance_history_year[0][COST_PROFIT] += betrag;
		finance_history_year[0][COST_CASH] = konto;
		// fill month history
		finance_history_month[0][type] += betrag;
		finance_history_month[0][COST_PROFIT] += betrag;
		finance_history_month[0][COST_CASH] = konto;
		// the other will be updated only monthly or whe a finance window is shown
	}
	return konto;
}



/* return true, if the owner is none, myself or player(1)
 * @author prissi
 */
bool spieler_t::check_owner( const spieler_t *sp ) const
{
	return (this==sp)  ||  (sp==NULL)  ||  (sp==welt->gib_spieler(1));
}



/**
 * Erzeugt eine neue Haltestelle des Spielers an Position pos
 * @author Hj. Malthaner
 */
halthandle_t spieler_t::halt_add(koord pos)
{
	halthandle_t halt = haltestelle_t::create(welt, pos, this);
	halt_add(halt);
	return halt;
}



/**
 * Erzeugt eine neue Haltestelle des Spielers an Position pos
 * @author Hj. Malthaner
 */
void
spieler_t::halt_add(halthandle_t halt)
{
	if (!halt_list.contains(halt)) {
		halt_list.append(halt);
		haltcount ++;
	}
}



/**
 * Entfernt eine Haltestelle des Spielers aus der Liste
 * @author Hj. Malthaner
 */
void
spieler_t::halt_remove(halthandle_t halt)
{
	halt_list.remove(halt);
}



/* return true, if my bahnhof is here
 * @author prissi
 */
bool spieler_t::is_my_halt(koord pos) const
{
	halthandle_t halt = haltestelle_t::gib_halt(welt,pos);
	if(halt.is_bound()  &&  check_owner(halt->gib_besitzer())) {
		return true;
	}
	// nothing here
	return false;
}



/* return true, if my bahnhof is here
 * @author prissi
 */
unsigned spieler_t::is_my_halt(koord3d pos) const
{
DBG_MESSAGE("spieler_t::is_my_halt()","called on (%i,%i)",pos.x,pos.y);
	const planquadrat_t *plan = welt->lookup(pos.gib_2d());
	if(plan) {

		for(unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
			grund_t *gr=plan->gib_boden_bei(i);
			if(gr) {
DBG_MESSAGE("spieler_t::is_my_halt()","grund %i exists",i);
				halthandle_t halt = gr->gib_halt();
//DBG_MESSAGE("spieler_t::is_my_halt()","check halt id %i",halt.get_id());
				if(halt.is_bound()  &&  check_owner(halt->gib_besitzer())) {
					return i+1;
				}
			}
		}
	}
	// nothing here
	return false;
}



/**
 * Speichert Zustand des Spielers
 * @param file Datei, in die gespeichert wird
 * @author Hj. Malthaner
 */
void spieler_t::rdwr(loadsave_t *file)
{
	int halt_count=0;
	int start_index=-1;
	int ziel_index=-1;

	// prepare for saving by default variables
	if(file->is_saving()) {
DBG_DEBUG("spieler_t::rdwr()","%i has %i halts.",welt->sp2num( this ),halt_count);
		start_index = welt->gib_fab_index(start);
		ziel_index = welt->gib_fab_index(ziel);
	}
	file->rdwr_delim("Sp ");
	file->rdwr_longlong(konto, " ");
	file->rdwr_long(konto_ueberzogen, " ");
	file->rdwr_long(steps, " ");
	if(file->get_version()<99009) {
		sint32 farbe;
		file->rdwr_long(farbe, " ");
		kennfarbe1 = (uint8)farbe*2;
		kennfarbe2 = kennfarbe1+24;
	}
	else {
		file->rdwr_byte(kennfarbe1, " ");
		file->rdwr_byte(kennfarbe2, " ");
	}
	if(file->get_version()<99008) {
		file->rdwr_long(halt_count, " ");
	}
	file->rdwr_long(haltcount, " ");

	if (file->get_version() < 84008) {
		// not so old save game
		for (int year = 0;year<MAX_HISTORY_YEARS;year++) {
			for (int cost_type = 0; cost_type<MAX_COST; cost_type++) {
				if (file->get_version() < 84007) {
					// a cost_type has has been added. For old savegames we only have 9 cost_types, now we have 10.
					// for old savegames only load 9 types and calculate the 10th; for new savegames load all 10 values
					if (cost_type < 9) {
						file->rdwr_longlong(finance_history_year[year][cost_type], " ");
					} else {
						sint64 tmp = finance_history_year[year][COST_VEHICLE_RUN] + finance_history_year[year][COST_MAINTENANCE];
						if(tmp<0) { tmp = -tmp; }
						finance_history_year[year][COST_MARGIN] = (tmp== 0) ? 0 : (finance_history_year[year][COST_OPERATING_PROFIT] * 100) / tmp;
						finance_history_year[year][COST_OPERATING_PROFIT] = 0;
						finance_history_year[year][COST_TRANSPORTED_GOODS] = 0;
					}
				} else {
					if (cost_type < 10) {
						file->rdwr_longlong(finance_history_year[year][cost_type], " ");
					} else {
						sint64 tmp = finance_history_year[year][COST_VEHICLE_RUN] + finance_history_year[year][COST_MAINTENANCE];
						if(tmp<0) { tmp = -tmp; }
						finance_history_year[year][COST_MARGIN] = (tmp==0) ? 0 : (finance_history_year[year][COST_OPERATING_PROFIT] * 100) / tmp;
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
			sint64 tmp = finance_history_year[year][COST_VEHICLE_RUN] + finance_history_year[year][COST_MAINTENANCE];
			if(tmp<0) { tmp = -tmp; }
			finance_history_year[year][COST_MARGIN] = (tmp== 0) ? 0 : (finance_history_year[year][COST_OPERATING_PROFIT] * 100) / tmp;
			finance_history_year[year][COST_TRANSPORTED_GOODS] = 0;
		}
		// in 84008 monthly finance history was introduced
		for (int month = 0;month<MAX_HISTORY_MONTHS;month++) {
			for (int cost_type = 0; cost_type<10; cost_type++) {
				file->rdwr_longlong(finance_history_month[month][cost_type], " ");
			}
			sint64 tmp = finance_history_month[month][COST_VEHICLE_RUN] + finance_history_month[month][COST_MAINTENANCE];
			if(tmp<0) { tmp = -tmp; }
			finance_history_month[month][COST_MARGIN] = (tmp==0) ? 0 : (finance_history_month[month][COST_OPERATING_PROFIT] * 100) / tmp;
			finance_history_month[month][COST_TRANSPORTED_GOODS] = 0;
		}
	}
	else if (file->get_version() < 99011) {
		// powerline category missing
		for (int year = 0;year<MAX_HISTORY_YEARS;year++) {
			for (int cost_type = 0; cost_type<12; cost_type++) {
				file->rdwr_longlong(finance_history_year[year][cost_type], " ");
			}
			finance_history_year[year][COST_POWERLINES] = 0;
		}
		for (int month = 0;month<MAX_HISTORY_MONTHS;month++) {
			for (int cost_type = 0; cost_type<12; cost_type++) {
				file->rdwr_longlong(finance_history_month[month][cost_type], " ");
			}
			finance_history_month[month][COST_POWERLINES] = 0;
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

	// state is not saved anymore
	if(file->get_version()<99014) {
		zustand dummy=NR_INIT;
		file->rdwr_enum(dummy, " ");
		file->rdwr_enum(dummy, "\n");
	}

	file->rdwr_long(start_index, " ");
	file->rdwr_long(ziel_index, "\n");
	file->rdwr_long(gewinn, " ");
	file->rdwr_long(count, "\n");
	platz1.rdwr( file );
	platz2.rdwr( file );

	// Hajo: sanity checks
	if(halt_count < 0) {
		dbg->fatal("spieler_t::rdwr()", "Halt count is out of bounds: %d -> corrupt savegame?", halt_count);
	}
	if(haltcount < 0) {
		dbg->fatal("spieler_t::rdwr()", "Haltcount is out of bounds: %d -> corrupt savegame?", haltcount);
	}
	if(start_index < -1) {
		dbg->fatal("spieler_t::rdwr()", "Start index is out of bounds: %d -> corrupt savegame?", start_index);
	}
	if(ziel_index < -1) {
		dbg->fatal("spieler_t::rdwr()", "Ziel index is out of bounds: %d -> corrupt savegame?", ziel_index);
	}

	if(file->is_loading()) {
		// first: financial sanity check
		for (int year = 0;year<MAX_HISTORY_YEARS;year++) {
			sint64 value=0;
			for (int cost_type = 0; cost_type<MAX_COST; cost_type++) {
				value += finance_history_year[year][cost_type];
			}
		}

DBG_MESSAGE("spieler_t::rdwr","loading ...");
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
		state = NR_INIT;
		count_rail = count_road = 255;
		baue = false;
		baue_start = baue_ziel = NULL;

		// last destinations unknown
		last_ziel = NULL;
		rail_engine = NULL;
		rail_vehicle = NULL;
		road_vehicle = NULL;
		ship_vehicle = NULL;
		rail_weg = NULL;
		road_weg = NULL;

		// halt_count will be zero for newer savegames
DBG_DEBUG("spieler_t::rdwr()","player %i: loading %i halts.",welt->sp2num( this ),halt_count);
		for(int i=0; i<halt_count; i++) {
			halthandle_t halt = haltestelle_t::create( welt, file );
			// it was possible to have stops without ground: do not load them
			if(halt.is_bound()) {
				halt_list.insert(halt);
				if(!halt->existiert_in_welt()) {
					dbg->warning("spieler_t::rdwr()","empty halt id %i qill be ignored", halt.get_id() );
				}
			}
		}
		init_texte();

		// empty undo buffer
		init_undo(road_wt,0);
	}

	// headquarter stuff
	if (file->get_version() < 86004)
	{
		headquarter_level = 0;
		headquarter_pos = koord::invalid;
	}
	else {
		file->rdwr_long(headquarter_level, " ");
		headquarter_pos.rdwr( file );
		if(file->is_loading()) {
			if(headquarter_level>=(sint32)hausbauer_t::headquarter.get_count()) {
				headquarter_level = (sint32)hausbauer_t::headquarter.get_count()-1;
			}
			if(headquarter_level<0) {
				headquarter_pos = koord::invalid;
				headquarter_level = 0;
			}
		}
	}

	// linemanagement
	if(file->get_version()>=88003) {
		simlinemgmt.rdwr(welt,file);
	}
}



/*
 * called after game is fully loaded;
 */
void spieler_t::laden_abschliessen()
{
	simlinemgmt.laden_abschliessen();
	display_set_player_color_scheme( player_nr, kennfarbe1, kennfarbe2 );
}



/**
 * Returns the amount of money for a certain finance section
 *
 * @author Owen Rudge
 */
sint64 spieler_t::get_finance_info(int type)
{
	if (type == COST_CASH) {
		return konto;
	}
	else {
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
sint64 spieler_t::get_finance_info_old(int type)
{
	return finance_history_year[1][type];
}



/**
 * Rückruf, um uns zu informieren, dass eine Station voll ist
 * @author Hansjörg Malthaner
 * @date 25-Nov-2001
 */
void spieler_t::bescheid_station_voll(halthandle_t halt)
{
	if(welt->get_active_player()==this) {
		char buf[256];

		sprintf(buf, translator::translate("!0_STATION_CROWDED"), halt->gib_name());
		message_t::get_instance()->add_message(buf, halt->gib_basis_pos(),message_t::full, player_nr,IMG_LEER);
	}
}



/**
 * Rückruf, um uns zu informieren, dass ein Vehikel ein Problem hat
 * @author Hansjörg Malthaner
 * @date 26-Nov-2001
 */
void spieler_t::bescheid_vehikel_problem(convoihandle_t cnv,const koord3d ziel)
{
	switch(cnv->get_state()) {

		case convoi_t::ROUTING_2:
DBG_MESSAGE("spieler_t::bescheid_vehikel_problem","Vehicle %s can't find a route to (%i,%i)!", cnv->gib_name(),ziel.x,ziel.y);
			if(this==welt->get_active_player()) {
				char buf[256];
				sprintf(buf,translator::translate("Vehicle %s can't find a route!"), cnv->gib_name());
				message_t::get_instance()->add_message(buf, cnv->gib_pos().gib_2d(),message_t::convoi,player_nr,cnv->gib_vehikel(0)->gib_basis_bild());
			}
			else if(this != welt->gib_spieler(0)) {
				cnv->self_destruct();
			}
			break;

		case convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH:
		case convoi_t::CAN_START_ONE_MONTH:
DBG_MESSAGE("spieler_t::bescheid_vehikel_problem","Vehicle %s stucked!", cnv->gib_name(),ziel.x,ziel.y);
			if(this==welt->get_active_player()) {
				char buf[256];
				sprintf(buf,translator::translate("Vehicle %s is stucked!"), cnv->gib_name());
				message_t::get_instance()->add_message(buf, cnv->gib_pos().gib_2d(),message_t::convoi,player_nr,cnv->gib_vehikel(0)->gib_basis_bild());
			}
			break;

		default:
DBG_MESSAGE("spieler_t::bescheid_vehikel_problem","Vehicle %s, state %i!", cnv->gib_name(), cnv->get_state());
	}
}



/* Here functions for UNDO
 * @date 7-Feb-2005
 * @author prissi
 */
void
spieler_t::init_undo( waytype_t wtype, unsigned short max )
{
	// only human player
	// prissi: allow for UNDO for real player
DBG_MESSAGE("spieler_t::int_undo()","undo tiles %i",max);
	last_built.clear();
	last_built.resize(max+1);
	if(max>0) {
		undo_type = wtype;
	}

}


void
spieler_t::add_undo(koord3d k)
{
	if(last_built.get_size()>0) {
//DBG_DEBUG("spieler_t::add_undo()","tile at (%i,%i)",k.x,k.y);
		last_built.append(k);
	}
}



bool
spieler_t::undo()
{
	if (last_built.empty()) {
		// nothing to UNDO
		return false;
	}
	// check, if we can still do undo
	for(unsigned short i=0;  i<last_built.get_count();  i++  ) {
		grund_t* gr = welt->lookup(last_built[i]);
		if(gr==NULL  ||  gr->gib_typ()!=grund_t::boden) {
			// well, something was built here ... so no undo
			last_built.clear();
			return false;
		}
		// we allow only leitung
		if(gr->obj_count()>0) {
			for( unsigned i=1;  i<gr->gib_top();  i++  ) {
				switch(gr->obj_bei(i)->gib_typ()) {
					// these are allowed
					case ding_t::way:
					case ding_t::verkehr:
					case ding_t::fussgaenger:
					case ding_t::leitung:
						break;
					// special case airplane
					// they can be everywhere, so we allow for everythign but runway undo
					case ding_t::aircraft:
						if(undo_type!=air_wt) {
							break;
						}
					// all other are forbidden => no undo any more
					default:
						last_built.clear();
						return false;
				}
			}
		}
	}

	// ok, now remove everything last built
	uint32 cost=0;
	for(unsigned short i=0;  i<last_built.get_count();  i++  ) {
		grund_t* gr = welt->lookup(last_built[i]);
		cost += gr->weg_entfernen(undo_type,true);
//DBG_DEBUG("spieler_t::add_undo()","undo tile %i at (%i,%i)",i,last_built.at(i).x,last_built.at(i).y);
	}
	last_built.clear();
	return cost!=0;
}



/******************************* from here on freight AI **************************************/


/* Activates/deactivates a player
 * @author prissi
 */
bool spieler_t::set_active(bool new_state)
{
	// something to change?
	if(automat!=new_state) {

		if(!new_state) {
			// deactivate AI
			automat = false;
			if(baue  &&  state>NR_BAUE_ROUTE1) {
				// deactivate semaphore, so other could do construction
				baue = false;
		    state = NR_INIT;
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



/* returns ok, of there is a suitable space found
 * only check into two directions, the ones given by dir
 */
bool spieler_t::suche_platz(koord pos, koord &size, koord *dirs) const
{
	sint16 length = abs( size.x + size.y );

	const grund_t *gr = welt->lookup_kartenboden(pos);
	if(gr==NULL) {
		return false;
	}

	sint16 start_z = gr->gib_hoehe();
	int max_dir = length==0 ? 1 : 2;
	// two rotations
	for(  int dir=0;  dir<max_dir;  dir++  ) {
		for( sint16 i=0;  i<=length;  i++  ) {
			grund_t *gr = welt->lookup_kartenboden(  pos + (dirs[dir]*i)  );
			if(gr==NULL  ||  gr->gib_halt().is_bound()  ||  !welt->can_ebne_planquadrat(pos,start_z)  ||  !gr->ist_natur()  ||  gr->kann_alle_obj_entfernen(this)!=NULL  ||  gr->gib_hoehe()<welt->gib_grundwasser()) {
				return false;
			}
		}
		// aparently we can built this rotation here
		size = dirs[dir]*length;
		return true;
	}
	return false;
}



/**
 * Find the last water tile using line algorithm von Hajo
 * start MUST be on land!
 **/
koord spieler_t::find_shore(koord start, koord end) const
{
	int x = start.x;
	int y = start.y;
	int xx = end.x;
	int yy = end.y;

	int i, steps;
	int xp, yp;
	int xs, ys;

	const int dx = xx - x;
	const int dy = yy - y;

	steps = (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
	if (steps == 0) steps = 1;

	xs = (dx << 16) / steps;
	ys = (dy << 16) / steps;

	xp = x << 16;
	yp = y << 16;

	koord last = start;
	for (i = 0; i <= steps; i++) {
		koord next(xp>>16,yp>>16);
		if(next!=last) {
			if(!welt->lookup_kartenboden(next)->ist_wasser()) {
				last = next;
			}
		}
		xp += xs;
		yp += ys;
	}
	// should always find something, since it ends in water ...
	return last;
}



bool spieler_t::find_harbour(koord &start, koord &size, koord target)
{
	koord shore = find_shore(target,start);
	// distance of last found point
	int dist=0x7FFFFFFF;
	koord k;
	// now find a nice shore next to here
	for(  k.y=shore.y-5;  k.y<shore.y+6; k.y++  ) {
		for(  k.x=shore.x-5;  k.x<shore.x+6; k.x++  ) {
			grund_t *gr = welt->lookup_kartenboden(k);
			if(gr  &&  gr->gib_grund_hang()!=0  &&  hang_t::ist_wegbar(gr->gib_grund_hang())  &&  gr->ist_natur()  &&  gr->gib_hoehe()==welt->gib_grundwasser()) {
				koord zv = koord(gr->gib_grund_hang());
				koord dir[2] = { zv, koord(zv.y,zv.x) };
				koord platz = k+zv;
				int current_dist = abs_distance(k,target);
				if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
					// we will take the shortest route found
					start = k;
					dist = current_dist;
				}
			}
		}
	}
	return (dist<0x7FFFFFFF);
}




/* needed renovation due to different sized factories
 * also try "nicest" place first
 * @author HJ & prissi
 */
bool spieler_t::suche_platz(koord &start, koord &size, koord target, koord off)
{
	// distance of last found point
	int dist=0x7FFFFFFF;
	koord	platz;
	int cov = welt->gib_einstellungen()->gib_station_coverage();
	int xpos = start.x;
	int ypos = start.y;

	koord dir[2];
	if(  abs(start.x-target.x)<abs(start.y-target.y)  ) {
		dir[0] = koord( 0, sgn(target.y-start.y) );
		dir[1] = koord( sgn(target.x-start.x), 0 );
	}
	else {
		dir[0] = koord( sgn(target.x-start.x), 0 );
		dir[1] = koord( 0, sgn(target.y-start.y) );
	}

	DBG_MESSAGE("spieler_t::suche_platz()","at (%i,%i) for size (%i,%i)",xpos,ypos,off.x,off.y);
	for (int y = ypos - cov; y < ypos + off.y + cov; y++) {
		for (int x = xpos - cov; x < xpos + off.x + cov; x++) {
			platz = koord(x,y);
			int current_dist = abs_distance(platz,target);
			if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
				// we will take the shortest route found
				start = platz;
				dist = abs_distance(platz,target);
			}
			else {
				koord test(x,y);
				if(is_my_halt(test)) {
DBG_MESSAGE("spieler_t::suche_platz()","Search around stop at (%i,%i)",x,y);

					// we are on a station that belongs to us
					int xneu=x-1, yneu=y-1;
					platz = koord(xneu,y);
					current_dist = abs_distance(platz,target);
					if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
						// we will take the shortest route found
						start = platz;
						dist = current_dist;
					}

					platz = koord(x,yneu);
					current_dist = abs_distance(platz,target);
					if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
						// we will take the shortest route found
						start = platz;
						dist = current_dist;
					}

					// search on the other side of the station
					xneu = x+1;
					yneu = y+1;
					platz = koord(xneu,y);
					current_dist = abs_distance(platz,target);
					if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
						// we will take the shortest route found
						start = platz;
						dist = current_dist;
					}

					platz = koord(x,yneu);
					current_dist = abs_distance(platz,target);
					if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
						// we will take the shortest route found
						start = platz;
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



bool spieler_t::suche_platz1_platz2(fabrik_t *qfab, fabrik_t *zfab, int length )
{
	koord start( qfab->gib_pos().gib_2d() );
	koord start_size( length, 0 );
	koord ziel( zfab->gib_pos().gib_2d() );
	koord ziel_size( length, 0 );

	bool ok = false;

	if(qfab->gib_besch()->gib_platzierung()!=fabrik_besch_t::Wasser) {
		ok = suche_platz(start, start_size, ziel, qfab->gib_besch()->gib_haus()->gib_groesse() );
	}
	else {
		// water factory => find harbour location
		ok = find_harbour(start, start_size, ziel );
	}
	if(ok) {
		// found a place, search for target
		ok = suche_platz(ziel, ziel_size, start, zfab->gib_besch()->gib_haus()->gib_groesse() );
	}

	INT_CHECK("simplay 1729");

	if( !ok ) {
		// keine Bauplaetze gefunden
		DBG_MESSAGE( "spieler_t::suche_platz1_platz2()", "no suitable locations found" );
	}
	else {
		// save places
		platz1 = start;
		size1 = start_size;
		platz2 = ziel;
		size2 = ziel_size;

		// reserve space with marker
		zeiger_t *z;
		grund_t *gr;

		DBG_MESSAGE( "spieler_t::suche_platz1_platz2()", "platz1=%d,%d platz2=%d,%d", platz1.x, platz1.y, platz2.x, platz2.y );

		gr = welt->lookup_kartenboden(platz1);
		z = new zeiger_t(welt, gr->gib_pos(), this);
		z->setze_bild( skinverwaltung_t::belegtzeiger->gib_bild_nr(0) );
		gr->obj_add( z );

		gr = welt->lookup_kartenboden(platz2);
		z = new zeiger_t(welt, gr->gib_pos(), this);
		z->setze_bild( skinverwaltung_t::belegtzeiger->gib_bild_nr(0) );
		gr->obj_add( z );
	}
	return ok;
}



/* return a rating factor; i.e. how much this connecting would benefit game play
 * @author prissi
 */
int spieler_t::rating_transport_quelle_ziel(fabrik_t *qfab,const ware_production_t *ware,fabrik_t *zfab)
{
	const vector_tpl<ware_production_t>& eingang = zfab->gib_eingang();
	// we may have more than one input:
	unsigned missing_input_ware=0;
	unsigned missing_our_ware=0;

	// does the source also produce => then we should try to connect it
	// otherwise production will stop too early
	bool is_manufacturer = !qfab->gib_eingang().empty();

	// hat noch mehr als halbvolle Lager  and more then 35 (otherwise there might be also another transporter) ?
	if(ware->menge < ware->max>>1  ||  ware->menge<(34<<fabrik_t::precision_bits)  ) {
		// zu wenig vorrat für profitable strecke
		return 0;
	}
	// if not really full output storage:
	if(  ware->menge < ware->max-(34<<fabrik_t::precision_bits)  ) {
		// well consider, but others are surely better
		return is_manufacturer ? 4 : 1;
	}

	// so we do a loop
	for (unsigned i = 0; i < eingang.get_count(); i++) {
		if (eingang[i].menge < 10 << fabrik_t::precision_bits) {
			// so more would be helpful
			missing_input_ware ++;
			if (eingang[i].gib_typ() == ware->gib_typ()) {
				missing_our_ware = true;
			}
		}
	}

DBG_MESSAGE("spieler_t::rating_transport_quelle_ziel","Missing our %i, total  missing %i",missing_our_ware,missing_input_ware);
	// so our good is missing
	if(  missing_our_ware  ) {
		if(  missing_input_ware==1  ) {
			if (eingang.get_count() - missing_input_ware == 1) {
				// only our is missing of multiple produkts
				return is_manufacturer ? 64 : 16;
			}
		}
		if (missing_input_ware < eingang.get_count()) {
			// factory is already supplied with mutiple sources, but our is missing
			return is_manufacturer ? 32 : 8;
		}
		return is_manufacturer ? 16 : 4;
	}

	// else there is no high priority
	return is_manufacturer ? 8 : 2;
}



/* guesses the income; should be improved someday ...
 * @author prissi
 */
int spieler_t::guess_gewinn_transport_quelle_ziel(fabrik_t *qfab,const ware_production_t *ware,int ware_nr, fabrik_t *zfab)
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
		const int grundwert = ware->gib_typ()->gib_preis();	// assume 3 cent per square mantenance
		if( dist > 6) {                          // sollte vernuenftige Entfernung sein

			// wie viel koennen wir tatsaechlich transportieren
			// Bei der abgabe rechnen wir einen sicherheitsfaktor 2 mit ein
			// wieviel kann am ziel verarbeitet werden, und wieviel gibt die quelle her?

			const int prod = min(zfab->get_base_production(),
			                 ( qfab->get_base_production() * qfab->gib_besch()->gib_produkt(ware_nr)->gib_faktor() )/256u - qfab->gib_abgabe_letzt(ware_nr) * 2u );

			gewinn = (grundwert *prod-5)+simrand(15000);
			if(dist>100) {
				gewinn /= 2;
			}

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

//DBG_MESSAGE("spieler_t::suche_transport_quelle","Search other %i supplier for: %s",lieferquelle_anzahl,zfab->gib_name());

	// now iterate over all suppliers
	for(  int i=0;  i<lieferquelle_anzahl;  i++  ) {
		// find source
		fabrik_t *start_neu = fabrik_t::gib_fab( welt, lieferquellen[i] );
		// get all ware
		const vector_tpl<ware_production_t>& ausgang = zfab->gib_ausgang();
		const int waren_anzahl = ausgang.get_count();
		// for all products
		for(int ware_nr=0;  ware_nr<waren_anzahl ;  ware_nr++  ) {
			int dieser_gewinn = guess_gewinn_transport_quelle_ziel( start_neu, &ausgang[ware_nr], ware_nr, zfab );
			// more income on this line
			if(  dieser_gewinn>gewinn  ) {
				*qfab = start_neu;
				*quelle_ware = ware_nr;
				gewinn = dieser_gewinn;
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
	if (qfab == NULL || qfab->gib_lieferziele().empty()) {
		return -1;
	}

	const vector_tpl<ware_production_t>& ausgang = qfab->gib_ausgang();

	// ist es ein erzeuger ?
	const int waren_anzahl = ausgang.get_count();

	// for all products
	for(int ware_nr=0;  ware_nr<waren_anzahl ;  ware_nr++  ) {
		const ware_production_t& ware = ausgang[ware_nr];

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

			const koord lieferziel = lieferziele[lieferziel_nr];
			if(is_connected(qfab->gib_pos().gib_2d(), lieferziel, ware.gib_typ())) {
				// already a line for this good ...
				// thius we check the next consumer
				continue;
			}

			int	dieser_gewinn=-1;
			fabrik_t *zfab = fabrik_t::gib_fab( welt, lieferziel );
			if(zfab) {
				dieser_gewinn = guess_gewinn_transport_quelle_ziel( qfab, &ware, ware_nr, zfab );
				if(dieser_gewinn > KI_MIN_PROFIT  &&  gewinn<dieser_gewinn  ) {
					// ok, this seems good or even better ...
					*quelle_ware = ware_nr;
					*ziel = zfab;
					gewinn = dieser_gewinn;
				}
			}
		}
	}

	// no loops: will be -1!
	return gewinn;
}



/* builts dock and ships
 * @author prissi
 */
bool spieler_t::create_ship_transport_vehikel(fabrik_t *qfab, int anz_vehikel)
{
	// must remove marker
	grund_t* gr = welt->lookup_kartenboden(platz1);
	if (gr) gr->obj_loesche_alle(this);
	// try to built dock
	const haus_besch_t* h = hausbauer_t::gib_random_station(haus_besch_t::hafen, welt->get_timeline_year_month(), haltestelle_t::WARE);
	if(h==NULL  ||  !wkz_dockbau(this, welt, platz1, h)) {
		return false;
	}
	// sea pos (and not on harbour ... )
	koord pos1 = platz1 - koord(gr->gib_grund_hang())*h->gib_groesse().y;

	// since 86.01 we use lines for road vehicles ...
	fahrplan_t *fpl=new schifffahrplan_t();
	fpl->append( welt->lookup_kartenboden(pos1), 0 );
	fpl->append( welt->lookup(qfab->gib_pos()), 100 );
	fpl->aktuell = 1;
	linehandle_t line=simlinemgmt.create_line(simline_t::shipline,fpl);
	delete fpl;

	// now create all vehicles as convois
	for(int i=0;  i<anz_vehikel;  i++) {
		vehikel_t* v = vehikelbauer_t::baue( qfab->gib_pos(), this, NULL, ship_vehicle);
		convoi_t* cnv = new convoi_t(this);
		// V.Meyer: give the new convoi name from first vehicle
		cnv->setze_name(v->gib_besch()->gib_name());
		cnv->add_vehikel( v );

		welt->sync_add( cnv );
		cnv->set_line(line);
		cnv->start();
	}
	platz1 += koord(welt->lookup_kartenboden(platz1)->gib_grund_hang());
	return true;
}



/* returns true,
 * if there is already a connection
 * @author prissi
 */
bool spieler_t::is_connected( const koord start_pos, const koord dest_pos, const ware_besch_t *wtyp )
{
	// Dario: Check if there's a stop near destination
	const planquadrat_t* start_plan = welt->lookup(start_pos);
	const halthandle_t* start_list = start_plan->get_haltlist();

	// Dario: Check if there's a stop near destination
	const planquadrat_t* dest_plan = welt->lookup(dest_pos);
	const halthandle_t* dest_list = dest_plan->get_haltlist();

	// suitable end search
	unsigned dest_count = 0;
	for (uint16 h = 0; h<dest_plan->get_haltlist_count(); h++) {
		halthandle_t halt = dest_list[h];
		if (halt->is_enabled(wtyp)) {
			for (uint16 hh = 0; hh<start_plan->get_haltlist_count(); hh++) {
				if (halt == start_list[hh]) {
					// connected with the start (i.e. too close)
					return true;
				}
			}
			dest_count ++;
		}
	}

	if(dest_count==0) {
		return false;
	}

	// now try to find a route
	// ok, they are not in walking distance
	ware_t ware(wtyp);
	ware.setze_zielpos(dest_pos);
	ware.menge = 1;
	for (uint16 hh = 0; hh<start_plan->get_haltlist_count(); hh++) {
		start_list[hh]->suche_route(ware, NULL);
		if (ware.gib_ziel().is_bound()) {
			// ok, already connected
			return true;
		}
	}

	// no connection possible between those
	return false;
}



/* changed to use vehicles searched before
 * @author prissi
 */
void spieler_t::create_road_transport_vehikel(fabrik_t *qfab, int anz_vehikel)
{
	const haus_besch_t* fh = hausbauer_t::gib_random_station(haus_besch_t::ladebucht, welt->get_timeline_year_month(), haltestelle_t::WARE);
	// succeed in frachthof creation
	if(fh  &&  wkz_halt(this, welt, platz1, fh)  &&  wkz_halt(this, welt, platz2, fh)  ) {
		koord3d pos1 = welt->lookup(platz1)->gib_kartenboden()->gib_pos();
		koord3d pos2 = welt->lookup(platz2)->gib_kartenboden()->gib_pos();

		int	start_location=0;
		// sometimes, when factories are very close, we need exakt calculation
		const koord3d& qpos = qfab->gib_pos();
		if ((qpos.x - platz1.x) * (qpos.x - platz1.x) + (qpos.y - platz1.y) * (qpos.y - platz1.y) >
				(qpos.x - platz2.x) * (qpos.x - platz2.x) + (qpos.y - platz2.y) * (qpos.y - platz2.y)) {
			start_location = 1;
		}

		// calculate vehicle start position
		koord3d startpos=(start_location==0)?pos1:pos2;
		ribi_t::ribi w_ribi = welt->lookup(startpos)->gib_weg_ribi_unmasked(road_wt);
		// now start all vehicle one field before, so they load immediately
		startpos = welt->lookup(koord(startpos.gib_2d())+koord(w_ribi))->gib_kartenboden()->gib_pos();

		// since 86.01 we use lines for road vehicles ...
		fahrplan_t *fpl=new autofahrplan_t();
		fpl->append(welt->lookup(pos1), start_location == 0 ? 100 : 0);
		fpl->append(welt->lookup(pos2), start_location == 1 ? 100 : 0);
		fpl->aktuell = start_location;
		linehandle_t line=simlinemgmt.create_line(simline_t::truckline,fpl);
		delete fpl;

		// now create all vehicles as convois
		for(int i=0;  i<anz_vehikel;  i++) {
			vehikel_t* v = vehikelbauer_t::baue(startpos, this, NULL, road_vehicle);
			convoi_t* cnv = new convoi_t(this);
			// V.Meyer: give the new convoi name from first vehicle
			cnv->setze_name(v->gib_besch()->gib_name());
			cnv->add_vehikel( v );

			welt->sync_add( cnv );
			cnv->set_line(line);
			cnv->start();
		}
	}
}



/* now obeys timeline and use "more clever" scheme for vehicle selection *
 * @author prissi
 */
void spieler_t::create_rail_transport_vehikel(const koord platz1, const koord platz2, int anz_vehikel, int ladegrad)
{
	fahrplan_t *fpl;
	convoi_t* cnv = new convoi_t(this);
	koord3d pos1= welt->lookup(platz1)->gib_kartenboden()->gib_pos();
	koord3d pos2 = welt->lookup(platz2)->gib_kartenboden()->gib_pos();

	// probably need to electrify the track?
	if(  rail_engine->get_engine_type()==vehikel_besch_t::electric  ) {
		// we need overhead wires
		value_t v;
		v.p = (const void *)(wayobj_t::wayobj_search(track_wt,overheadlines_wt,welt->get_timeline_year_month()));
		wkz_wayobj(this,welt,INIT,v);
		wkz_wayobj(this,welt,platz1,v);
		wkz_wayobj(this,welt,platz2,v);
	}
	vehikel_t* v = vehikelbauer_t::baue(pos2, this, NULL, rail_engine);

	// V.Meyer: give the new convoi name from first vehicle
	cnv->setze_name(rail_engine->gib_name());
	cnv->add_vehikel( v );

	DBG_MESSAGE( "spieler_t::create_rail_transport_vehikel","for %i cars",anz_vehikel);

	/* now we add cars:
	 * check here also for introduction years
	 */
	for(int i = 0; i < anz_vehikel; i++) {
		// use the vehicle we searched before
		vehikel_t* v = vehikelbauer_t::baue(pos2, this, NULL, rail_vehicle);
		cnv->add_vehikel( v );
	}

	fpl = cnv->gib_vehikel(0)->erzeuge_neuen_fahrplan();

	fpl->aktuell = 0;
	fpl->append(welt->lookup(pos1), ladegrad);
	fpl->append(welt->lookup(pos2), 0);

	cnv->setze_fahrplan(fpl);
	welt->sync_add( cnv );
	cnv->start();
}



bool spieler_t::create_simple_road_transport()
{
	INT_CHECK("simplay 837");

	welt->ebne_planquadrat( platz1, welt->lookup_kartenboden(platz1)->gib_hoehe() );
	welt->ebne_planquadrat( platz2, welt->lookup_kartenboden(platz2)->gib_hoehe() );

	// is there already a connection?
	if(road_vehicle) {
		vehikel_t* test_driver = new automobil_t(koord3d(platz1, 0), road_vehicle, this, NULL);
		route_t verbindung;
		if (verbindung.calc_route(welt, welt->lookup(platz1)->gib_kartenboden()->gib_pos(), welt->lookup(platz2)->gib_kartenboden()->gib_pos(), test_driver, 0) &&
			verbindung.gib_max_n()<2*(sint32)abs_distance(platz1,platz2))  {
DBG_MESSAGE("spieler_t::create_simple_road_transport()","Already connection between %d,%d to %d,%d is only %i",platz1.x, platz1.y, platz2.x, platz2.y, verbindung.gib_max_n() );
			// found something with the nearly same lenght
			delete test_driver;
			return true;
		}
		delete test_driver;
	}
	else {
		return false;
	}

	// no connection => built one!
	wegbauer_t bauigel(welt, this);

	bauigel.route_fuer( wegbauer_t::strasse, road_weg, tunnelbauer_t::find_tunnel(road_wt,road_vehicle->gib_geschw(),welt->get_timeline_year_month()), brueckenbauer_t::find_bridge(road_wt,road_vehicle->gib_geschw(),welt->get_timeline_year_month()) );

	// we won't destroy cities (and save the money)
	bauigel.set_keep_existing_faster_ways(true);
	bauigel.set_keep_city_roads(true);
	bauigel.set_maximum(10000);

	INT_CHECK("simplay 846");

	bauigel.calc_route(welt->lookup(platz1)->gib_kartenboden()->gib_pos(),welt->lookup(platz2)->gib_kartenboden()->gib_pos());

	// Strasse muss min. 3 Felder lang sein, sonst kann man keine
	// zwei verschiedene stops bauen

	if(bauigel.max_n > 1) {
DBG_MESSAGE("spieler_t::create_simple_road_transport()","building simple road from %d,%d to %d,%d",platz1.x, platz1.y, platz2.x, platz2.y);
		bauigel.baue();
		return true;
	}
DBG_MESSAGE("spieler_t::create_simple_road_transport()","building simple road from %d,%d to %d,%d failed",platz1.x, platz1.y, platz2.x, platz2.y);
	return false;
}






/* try harder to built a station:
 * check also sidewards
 * @author prissi
 */
int spieler_t::baue_bahnhof(koord3d quelle, koord *p, int anz_vehikel, fabrik_t *fab)
{
	int laenge = max(((rail_vehicle->get_length()*anz_vehikel)+rail_engine->get_length()+15)/16,1);

	int baulaenge = 0;
	ribi_t::ribi ribi = welt->lookup(*p)->gib_kartenboden()->gib_weg_ribi(track_wt);
	int i;

	koord zv ( ribi );

#if 1
	koord t = *p;
	bool ok = true;

	for(i=0; i<laenge; i++) {
		grund_t *gr = welt->lookup_kartenboden(t);
		ok &= (gr != NULL) &&  !gr->has_two_ways()  &&  gr->gib_weg_hang()==hang_t::flach;
		if(!ok) {
			break;
		}
		baulaenge ++;
		t += zv;
	}

	// too short
	if(baulaenge==1) {
		return 0;
	}

	// to avoid broken stations, they will be always built next to an existing
	bool make_all_bahnhof=false;

	// find a freight train station
	const haus_besch_t* besch = hausbauer_t::gib_random_station(haus_besch_t::bahnhof, welt->get_timeline_year_month(), haltestelle_t::WARE);
	koord pos;
	for(  pos=t-zv;  pos!=*p;  pos-=zv ) {
		if(  make_all_bahnhof  ||  is_my_halt(pos+koord(-1,-1))  ||  is_my_halt(pos+koord(-1,1))  ||  is_my_halt(pos+koord(1,-1))  ||  is_my_halt(pos+koord(1,1))  ) {
			// start building, if next to an existing station
			make_all_bahnhof = true;
			wkz_halt_aux(this, welt, pos, besch, track_wt, umgebung_t::cst_multiply_station, "BF");
		}
		INT_CHECK("simplay 753");
	}
	// now add the other squares (going backwards)
	for(  pos=*p;  pos!=t;  pos+=zv ) {
		if(  !is_my_halt(pos)  ) {
			wkz_halt_aux(this, welt, pos, besch, track_wt, umgebung_t::cst_multiply_station, "BF");
		}
	}
#else
	zv.x = -zv.x;
	zv.y = -zv.y;

DBG_MESSAGE("spieler_t::baue_bahnhof()","try to build a train station of length %d (%d vehicles) at (%i,%i) (ribi %i)", laenge, anz_vehikel, p->x, p->y, ribi);

	if(abs(zv.x)+abs(zv.y)!=1) {
		dbg->error("spieler_t::baue_bahnhof()","Not allowed here!");
		return 0;
	}

	INT_CHECK("simplay 548");

	const int hoehe = welt->lookup(t)->gib_kartenboden()->gib_hoehe();
	t = *p;
	for(i=0; i<laenge-1; i++) {
		t += zv;
		welt->ebne_planquadrat(t, hoehe);
		// update image
		for(int j=-1; j<=1; j++) {
			for(int i=-1; i<=1; i++) {
				if(welt->ist_in_kartengrenzen(t.x+i,t.y+j)) {
					welt->lookup(t+koord(i,j))->gib_kartenboden()->calc_bild();
				}
			}
		}
	}

	t = *p;
	bool ok = true;

	for(i=0; i<laenge-1; i++) {
		t += zv;

		const planquadrat_t *plan = welt->lookup(t);
		grund_t *gr = plan ? plan->gib_kartenboden() : 0;

		ok &= (gr != NULL) &&
				(gr->ist_natur() || gr->gib_typ() == grund_t::fundament) &&
				gr->kann_alle_obj_entfernen(this)==NULL &&  !gr->has_two_ways()  &&
				gr->gib_weg_hang() == hang_t::flach  &&  !gr->is_halt();

		if(ok) {
			// remove city building here
			const gebaeude_t* gb = gr->find<gebaeude_t>();
			if(gb  &&  gb->gib_besitzer()==NULL) {
				const char *msg=NULL;
				wkz_remover_intern(this,welt,t,msg);
			}
		}
		else {
			// not ok? then try to extend station into the other direction
			const planquadrat_t *plan = welt->lookup(*p-zv);
			gr = plan ? plan->gib_kartenboden(): 0;

			// try to extend station into the other direction
			ok = (gr != NULL) &&
					gr->hat_weg(track_wt) &&
					gr->gib_weg_ribi(track_wt) == ribi_t::doppelt(ribi) &&
					gr->kann_alle_obj_entfernen(this) == NULL &&  !gr->has_two_ways()  &&
					gr->gib_weg_hang()== hang_t::flach  &&  !gr->is_halt();
			if(ok) {
DBG_MESSAGE("spieler_t::baue_bahnhof","go back one segment");
				// remove city building here
				const gebaeude_t* gb = gr->find<gebaeude_t>();
				if(gb  &&  gb->gib_besitzer()==NULL) {
					const char *msg=NULL;
					wkz_remover_intern(this,welt,*p-zv,msg);
				}
				// change begin of station
				(*p) -= zv;
			}
			t -= zv;
		}
	}

	baulaenge = abs_distance(t+zv,*p);
DBG_MESSAGE("spieler_t::baue_bahnhof","achieved length %i",baulaenge);
	if(baulaenge==1  &&  laenge>1  &&  fab) {
		// ok, maximum station length in one tile, better try something else
		int cov = welt->gib_einstellungen()->gib_station_coverage();
		const koord coverage_koord(cov, cov);
		koord pos=*p;
		sint32 cost = 0;
		koord last_dir = koord((ribi_t::ribi)welt->lookup(*p)->gib_kartenboden()->gib_weg_ribi(track_wt));
		while(1) {
			grund_t *gr=welt->lookup(pos)->gib_kartenboden();
			if(gr==NULL  ||  gr->gib_weg_hang()!=gr->gib_grund_hang()) {
				break;
			}
			koord dir((ribi_t::ribi)gr->gib_weg_ribi(track_wt));
			if(!fabrik_t::sind_da_welche(welt,pos-coverage_koord,pos+coverage_koord).is_contained(fab)) {
				*p = pos;
				break;
			}
			cost = gr->weg_entfernen(track_wt, true);
			pos += dir;
			buche(-cost, pos, COST_CONSTRUCTION);
			if(dir!=last_dir  ||  welt->lookup(pos)->gib_kartenboden()->gib_weg_hang()!=hang_t::flach) {
				// was a curve or a slope: try again
				*p = pos;
				return baue_bahnhof( quelle, p, anz_vehikel, fab );
			}
		}
DBG_MESSAGE("spieler_t::baue_bahnhof","failed");
		return 0;
	}

	INT_CHECK("simplay 593");

	koord pos;

	wegbauer_t bauigel(welt, this);
	bauigel.route_fuer(wegbauer_t::schiene, rail_weg);
	bauigel.calc_straight_route(welt->lookup((*p)-zv)->gib_kartenboden()->gib_pos(), welt->lookup(t)->gib_kartenboden()->gib_pos());
	bauigel.baue();

	// to avoid broken stations, they will be always built next to an existing
	bool make_all_bahnhof=false;

	// find a freight train station
	const haus_besch_t* besch = hausbauer_t::gib_random_station(haus_besch_t::bahnhof, welt->get_timeline_year_month(), haltestelle_t::WARE);
	for(  pos=*p;  pos!=t+zv;  pos+=zv ) {
		if(  make_all_bahnhof  ||  is_my_halt(pos+koord(-1,-1))  ||  is_my_halt(pos+koord(-1,1))  ||  is_my_halt(pos+koord(1,-1))  ||  is_my_halt(pos+koord(1,1))  ) {
			// start building, if next to an existing station
			make_all_bahnhof = true;
			wkz_halt(this, welt, pos, besch);
		}
		INT_CHECK("simplay 753");
	}
	// now add the other squares (going backwards)
	for(  pos=t;  pos!=*p-zv;  pos-=zv ) {
		if(  !is_my_halt(pos)  ) {
			wkz_halt(this, welt, pos, besch);
		}
	}

DBG_MESSAGE("spieler_t::baue_bahnhof","set pos *p %i,%i to %i,%i",p->x,p->y,t.x,t.y);
	// return endpos
	*p = t;

#endif
	laenge = min( anz_vehikel, (baulaenge*16 - rail_engine->get_length())/rail_vehicle->get_length() );
//DBG_MESSAGE("spieler_t::baue_bahnhof","Final station at (%i,%i) with %i tiles for %i cars",p->x,p->y,baulaenge,laenge);
	return laenge;
}



/* built a very simple track with just the minimum effort
 * usually good enough, since it can use road crossings
 * @author prissi
 */
bool
spieler_t::create_simple_rail_transport()
{
	// first: make plain stations tiles as intended
	sint16 z1 = welt->lookup_kartenboden(platz1)->gib_hoehe();
	koord k=platz1;
	koord diff1( sgn(size1.x), sgn(size1.y) );
	koord perpend( sgn(size1.y), sgn(size1.x) );
	ribi_t::ribi ribi1 = ribi_typ( diff1 );
	while(k!=size1+platz1) {
		welt->ebne_planquadrat( k, z1 );
		welt->lookup_kartenboden( k-diff1 )->calc_bild();
		welt->lookup_kartenboden( k-diff1+perpend )->calc_bild();
		welt->lookup_kartenboden( k-diff1-perpend )->calc_bild();
		weg_t *sch = weg_t::alloc(track_wt);
		sch->setze_besch( rail_weg );
		int cost = -welt->lookup_kartenboden(k)->neuen_weg_bauen(sch, ribi1, this) - rail_weg->gib_preis();
		buche(cost, k, COST_CONSTRUCTION);
		ribi1 = ribi_t::doppelt( ribi1 );
		k += diff1;
	}
	welt->lookup_kartenboden( k )->calc_bild();
	welt->lookup_kartenboden( k+perpend )->calc_bild();
	welt->lookup_kartenboden( k-perpend )->calc_bild();

	// make the second ones flat ...
	sint16 z2 = welt->lookup_kartenboden(platz2)->gib_hoehe();
	k = platz2;
	koord diff2 = koord( sgn(size2.x), sgn(size2.y) );
	perpend = koord( sgn(size2.y), sgn(size2.x) );
	ribi_t::ribi ribi2 = ribi_typ( diff2 );
	while(k!=size2+platz2) {
		welt->ebne_planquadrat(k,z2);
		welt->lookup_kartenboden( k-diff2 )->calc_bild();
		welt->lookup_kartenboden( k-diff2+perpend )->calc_bild();
		welt->lookup_kartenboden( k-diff2-perpend )->calc_bild();
		weg_t *sch = weg_t::alloc(track_wt);
		sch->setze_besch( rail_weg );
		int cost = -welt->lookup_kartenboden(k)->neuen_weg_bauen(sch, ribi2, this) - rail_weg->gib_preis();
		buche(cost, k, COST_CONSTRUCTION);
		ribi2 = ribi_t::doppelt( ribi2 );
		k += diff2;
	}
	welt->lookup_kartenboden( k )->calc_bild();
	welt->lookup_kartenboden( k+perpend )->calc_bild();
	welt->lookup_kartenboden( k-perpend )->calc_bild();

	// now calc the route
	wegbauer_t bauigel(welt, this);
	bauigel.route_fuer( (wegbauer_t::bautyp_t)(wegbauer_t::schiene|wegbauer_t::bot_flag), rail_weg, tunnelbauer_t::find_tunnel(track_wt,rail_engine->gib_geschw(),welt->get_timeline_year_month()), brueckenbauer_t::find_bridge(track_wt,rail_engine->gib_geschw(),welt->get_timeline_year_month()) );
	bauigel.set_keep_existing_ways(false);
	bauigel.calc_route( koord3d(platz2+size2,z2), koord3d(platz1+size1,z1) );
	INT_CHECK("simplay 2478");

	if(bauigel.max_n > 3) {
		//just check, if I could not start at the other end of the station ...
		int start=0, end=bauigel.max_n;
		for( int j=1;  j<bauigel.max_n-1;  j++  ) {
			if(bauigel.gib_route_bei(j)==platz2-diff2) {
				start = j;
				platz2 += size2-diff2;
				size2 = size2*(-1);
				diff2 = diff2*(-1);
			}
			if(bauigel.gib_route_bei(j)==platz1-diff1) {
				end = j;
				platz1 += size1-diff1;
				size1 = size1*(-1);
				diff1 = diff1*(-1);
			}
		}
		// so found shorter route?
		if(start!=0  ||  end!=bauigel.max_n) {
			bauigel.calc_route( koord3d(platz2+size2,z2), koord3d(platz1+size1,z1) );
		}

DBG_MESSAGE("spieler_t::create_simple_rail_transport()","building simple track from %d,%d to %d,%d",platz1.x, platz1.y, platz2.x, platz2.y);
		bauigel.baue();
		// connect to track
		ribi1 = ribi_typ(diff1);
		assert( welt->lookup_kartenboden(platz1+size1-diff1)->weg_erweitern(track_wt, ribi1) );
		ribi1 = ribi_t::rueckwaerts(ribi1);
		assert( welt->lookup_kartenboden(platz1+size1)->weg_erweitern(track_wt, ribi1) );
		ribi2 = ribi_typ(diff2);
		assert( welt->lookup_kartenboden(platz2+size2-diff2)->weg_erweitern(track_wt, ribi2) );
		ribi2 = ribi_t::rueckwaerts(ribi2);
		assert( welt->lookup_kartenboden(platz2+size2)->weg_erweitern(track_wt, ribi2) );
		return true;
	}
	else {
		// remove station ...
		koord k=platz1;
		while(k!=size1+platz1) {
			int cost = -welt->lookup_kartenboden(k)->weg_entfernen( track_wt, true );
			buche(cost, k, COST_CONSTRUCTION);
			k += diff1;
		}
		k=platz2;
		while(k!=size2+platz2) {
			int cost = -welt->lookup_kartenboden(k)->weg_entfernen( track_wt, true );
			buche(cost, k, COST_CONSTRUCTION);
			k += diff2;
		}
	}
	return false;
}



// the normal length procedure for freigth AI
void spieler_t::do_ki()
{
	if(passenger_transport) {
		if (!welt->gib_staedte().empty()) {
			// passenger are special ...
			do_passenger_ki();
		}
		return;
	}

	if(konto_ueberzogen>0) {
		// nothing to do but to remove unneeded convois to gain some money
		state = CHECK_CONVOI;
	}

	switch(state) {

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
				state = NR_SAMMLE_ROUTEN;
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
			fabrik_t *start_neu = NULL;
			fabrik_t *ziel_neu = NULL;
			int	start_ware_neu;
			int gewinn_neu=-1;

			if(  count==KI_TEST_LINES-2  &&  last_ziel!=NULL) {
				// we built a line: do we need another supplier?
				gewinn_neu = suche_transport_quelle(&start_neu, &start_ware_neu, last_ziel);
				if(  gewinn_neu>KI_MIN_PROFIT  &&  start_neu!=last_start  &&  start_neu!=NULL  ) {
					// marginally profitable -> then built it!
					DBG_MESSAGE("spieler_t::do_ki","Select quelle from %s (%i,%i) to %s (%i,%i) (income %i)", start_neu->gib_name(), start_neu->gib_pos().x, start_neu->gib_pos().y, ziel_neu->gib_name(), ziel_neu->gib_pos().x, ziel_neu->gib_pos().y, gewinn_neu);
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
				if(count==KI_TEST_LINES-1  &&  last_ziel!=NULL) {
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
					if(start_neu  &&  start_neu->gib_besch()->gib_produkte()>0  &&  (start_neu->gib_besch()->gib_platzierung()!=fabrik_besch_t::Wasser  ||  vehikelbauer_t::vehikel_search( water_wt, welt->get_timeline_year_month(), 10, 10, start_neu->gib_besch()->gib_produkt(0)->gib_ware(), false ))  ) {
						// ship available or not needed
						if (start_neu!=NULL  &&  start_neu!=last_start) {
							gewinn_neu = suche_transport_ziel(start_neu, &start_ware_neu, &ziel_neu);
							if(ziel_neu!=NULL  &&  gewinn_neu>KI_MIN_PROFIT) {
								DBG_MESSAGE("spieler_t::do_ki", "Check route from %s (%i,%i) to %s (%i,%i) (income %i)", start_neu->gib_name(), start_neu->gib_pos().x, start_neu->gib_pos().y, ziel_neu->gib_name(), ziel_neu->gib_pos().x, ziel_neu->gib_pos().y, gewinn_neu);
							}
							else {
								gewinn_neu = -1;
							}
						}
					}
				}
			}

			// better than last one?
			// and not identical to last one
			if(gewinn_neu>gewinn  &&  start_neu!=NULL  &&  !(start_neu==last_start  &&  last_ziel==ziel_neu)  &&  !(start_neu==baue_start  &&  ziel_neu==baue_ziel)  ) {
				// more income and not the same than before ...
				start = start_neu;
				start_ware = start_ware_neu;
				ziel = ziel_neu;
				gewinn = gewinn_neu;
				DBG_MESSAGE("spieler_t::do_ki", "Consider route from %s (%i,%i) to %s (%i,%i) (income %i)", start_neu->gib_name(), start_neu->gib_pos().x, start_neu->gib_pos().y, ziel_neu->gib_name(), ziel_neu->gib_pos().x, ziel_neu->gib_pos().y, gewinn_neu);
			}
			count ++;

			if(count >= KI_TEST_LINES) {
				if(start!=NULL  &&  ziel!=NULL  &&  !(start==baue_start  &&  ziel==baue_ziel)  ) {
DBG_MESSAGE("spieler_t::do_ki","Decicion %s to %s (income %i)",start->gib_name(),ziel->gib_name(), gewinn);
					state = NR_BAUE_ROUTE1;
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

			// needed for search
			// first we have to find a suitable car
			const ware_besch_t* freight = start->gib_ausgang()[start_ware].gib_typ();

			// last check, if the route is not taken yet
			if(is_connected(start->gib_pos().gib_2d(), ziel->gib_pos().gib_2d(), freight)) {
				// falls gar nichts klappt gehen wir zum initialzustand zurueck
				baue = false;
				state = CHECK_CONVOI;
				return;
			}

			/* if we reached here, we decide to built a route;
			 * the KI just chooses the way to run the operation at maximum profit (minimum loss).
			 * The KI will built also a loosing route; this might be required by future versions to
			 * be able to built a network!
			 * @author prissi
			 */
			DBG_MESSAGE("spieler_t::do_ki()","%s want to build a route from %s (%d,%d) to %s (%d,%d) with value %d", gib_name(), start->gib_name(), start->gib_pos().x, start->gib_pos().y, ziel->gib_name(), ziel->gib_pos().x, ziel->gib_pos().y, gewinn );

			/* for the calculation we need:
			 * a suitable car (and engine)
			 * a suitable weg
			 */
			koord zv = start->gib_pos().gib_2d() - ziel->gib_pos().gib_2d();
			int dist = abs(zv.x) + abs(zv.y);

			// guess the "optimum" speed (usually a little too low)
			uint32 best_rail_speed = 80;// is ok enough for goods, was: min(60+freight->gib_speed_bonus()*5, 140 );
			uint32 best_road_speed = min(60+freight->gib_speed_bonus()*5, 130 );

			// obey timeline
			uint month_now = (welt->use_timeline() ? welt->get_current_month() : 0);

			INT_CHECK("simplay 1265");

			// is rail transport allowed?
			if(rail_transport) {
				// any rail car that transport this good (actually this weg_t the largest)
				rail_vehicle = vehikelbauer_t::vehikel_search( track_wt, month_now, 0, best_rail_speed,  freight );
			}
			rail_engine = NULL;
			rail_weg = NULL;
DBG_MESSAGE("do_ki()","rail vehicle %p",rail_vehicle);

			// is road transport allowed?
			if(road_transport) {
				// any road car that transport this good (actually this returns the largest)
				road_vehicle = vehikelbauer_t::vehikel_search( road_wt, month_now, 10, best_road_speed, freight );
			}
			road_weg = NULL;
DBG_MESSAGE("do_ki()","road vehicle %p",road_vehicle);

			ship_vehicle = NULL;
			if(start->gib_besch()->gib_platzierung()==fabrik_besch_t::Wasser) {
				// largest ship available
				ship_vehicle = vehikelbauer_t::vehikel_search( water_wt, month_now, 10, 20, freight );
			}

			INT_CHECK("simplay 1265");

			// properly calculate production
			const int prod = min(ziel->get_base_production(),
			                 ( start->get_base_production() * start->gib_besch()->gib_produkt(start_ware)->gib_faktor() )/256u - start->gib_abgabe_letzt(start_ware) );

DBG_MESSAGE("do_ki()","check railway");
			/* calculate number of cars for railroad */
			count_rail=255;	// no cars yet
			if(  rail_vehicle!=NULL  ) {
				// if our car is faster: well use slower speed to save money
			 	best_rail_speed = min(51,rail_vehicle->gib_geschw());
				// for engine: gues number of cars
				count_rail = (prod*dist) / (rail_vehicle->gib_zuladung()*best_rail_speed)+1;
				// assume the engine weight 100 tons for power needed calcualtion
				int total_weight = count_rail*( (rail_vehicle->gib_zuladung()*freight->gib_weight_per_unit())/1000 + rail_vehicle->gib_gewicht());
//				long power_needed = (long)(((best_rail_speed*best_rail_speed)/2500.0+1.0)*(100.0+count_rail*(rail_vehicle->gib_gewicht()+rail_vehicle->gib_zuladung()*freight->gib_weight_per_unit()*0.001)));
				rail_engine = vehikelbauer_t::vehikel_search( track_wt, month_now, total_weight, best_rail_speed, NULL, true );
				if(  rail_engine!=NULL  ) {
				 	best_rail_speed = min(rail_engine->gib_geschw(),rail_vehicle->gib_geschw());
				  // find cheapest track with that speed (and no monorail/elevated/tram tracks, please)
				 rail_weg = wegbauer_t::weg_search( track_wt, best_rail_speed, welt->get_timeline_year_month(),weg_t::type_flat );
				 if(  rail_weg!=NULL  ) {
					  if(  best_rail_speed>rail_weg->gib_topspeed()  ) {
					  	best_rail_speed = rail_weg->gib_topspeed();
					  }
					  // no train can have more than 15 cars
					  count_rail = min( 22, (3*prod*dist) / (rail_vehicle->gib_zuladung()*best_rail_speed*2) );
					  // if engine too week, reduce number of cars
					  if(  count_rail*80*64>(int)(rail_engine->gib_leistung()*rail_engine->get_gear())  ) {
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

DBG_MESSAGE("do_ki()","check railway");
			/* calculate number of cars for road; much easier */
			count_road=255;	// no cars yet
			if(  road_vehicle!=NULL  ) {
				best_road_speed = road_vehicle->gib_geschw();
				// find cheapest road
				road_weg = wegbauer_t::weg_search( road_wt, best_road_speed, welt->get_timeline_year_month(),weg_t::type_flat );
				if(  road_weg!=NULL  ) {
					if(  best_road_speed>road_weg->gib_topspeed()  ) {
						best_road_speed = road_weg->gib_topspeed();
					}
					// minimum vehicle is 1, maximum vehicle is 48, more just result in congestion
					count_road = min( 254, (prod*dist) / (road_vehicle->gib_zuladung()*best_road_speed*2)+2 );
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
				count_rail = max( 3, count_rail );
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
			if(  min(count_road,count_rail)!=255  ) {
				// road or rail?
				int length = 1;
				if(  cost_rail<cost_road  ) {
					length = (rail_engine->get_length() + count_rail*rail_vehicle->get_length()+15)/16;
					if(suche_platz1_platz2(start, ziel, length)) {
						state = ship_vehicle ? NR_BAUE_WATER_ROUTE : NR_BAUE_SIMPLE_SCHIENEN_ROUTE;
					}
				}
				if(state==NR_BAUE_ROUTE1  &&  suche_platz1_platz2(start, ziel, 1)) {
					// rail was too expensive or not successfull
					count_rail = 255;
					state = ship_vehicle ? NR_BAUE_WATER_ROUTE : NR_BAUE_STRASSEN_ROUTE;
				}
			}
			// no success at all?
			if(state==NR_BAUE_ROUTE1) {
				baue = false;
				state = CHECK_CONVOI;
			}
		}
		break;

		// built a simple ship route
		case NR_BAUE_WATER_ROUTE:
			{
				int ships_needed = 1+(rail_vehicle ? count_rail*rail_vehicle->gib_zuladung() : count_road*road_vehicle->gib_zuladung())/ship_vehicle->gib_zuladung();
				koord harbour=platz1;
				if(create_ship_transport_vehikel(start,ships_needed)) {
					if(welt->lookup(harbour)->gib_halt()->gib_fab_list().contains(ziel)) {
						// so close, so we are already connected
						grund_t *gr = welt->lookup_kartenboden(platz2);
						if (gr) gr->obj_loesche_alle(this);
						state = NR_ROAD_SUCCESS;
					}
					else {
						// else we need to built the second part of the route
						state = rail_vehicle ? NR_BAUE_SIMPLE_SCHIENEN_ROUTE : NR_BAUE_STRASSEN_ROUTE;
					}
				}
				else {
					ship_vehicle = NULL;
					state = CHECK_CONVOI;
				}
			}
			break;

		// built a simple railroad
		case NR_BAUE_SIMPLE_SCHIENEN_ROUTE:
			if(create_simple_rail_transport()) {
				sint16 org_count_rail = count_rail;
				count_rail = baue_bahnhof(start->gib_pos(), &platz1, count_rail, ziel);
				if(count_rail>0) {
					count_rail = baue_bahnhof(ziel->gib_pos(), &platz2, count_rail, start);
				}
				if(count_rail>0) {
					if(count_rail<org_count_rail) {
						// rethink engine
					 	int best_rail_speed = min(51,rail_vehicle->gib_geschw());
						// needed for search
						// first we have to find a suitable car
						const ware_besch_t* freight = start->gib_ausgang()[start_ware].gib_typ();
					  	// obey timeline
						uint month_now = (welt->use_timeline() ? welt->get_current_month() : 0);
						// for engine: gues number of cars
						long power_needed=(long)(((best_rail_speed*best_rail_speed)/2500.0+1.0)*(100.0+count_rail*(rail_vehicle->gib_gewicht()+rail_vehicle->gib_zuladung()*freight->gib_weight_per_unit()*0.001)));
						const vehikel_besch_t *v=vehikelbauer_t::vehikel_search( track_wt, month_now, power_needed, best_rail_speed, NULL );
						if(v->gib_betriebskosten()<rail_engine->gib_betriebskosten()) {
							rail_engine = v;
						}
					}
					create_rail_transport_vehikel( platz1, platz2, count_rail, 100 );
					state = NR_RAIL_SUCCESS;
				}
				else {
DBG_MESSAGE("spieler_t::step()","remove already constructed rail between %i,%i and %i,%i and try road",platz1.x,platz1.y,platz2.x,platz2.y);
					value_t v;
					// no sucess: clean route
					v.i = (int)track_wt;
					wkz_wayremover(this,welt,INIT,v);
					wkz_wayremover(this,welt,platz1,v);
					wkz_wayremover(this,welt,platz2,v);
					state = NR_BAUE_STRASSEN_ROUTE;
				}
			}
			else {
				state = NR_BAUE_STRASSEN_ROUTE;
			}
		break;

		// built a simple road (no bridges, no tunnels)
		case NR_BAUE_STRASSEN_ROUTE:
			if(create_simple_road_transport()) {
				create_road_transport_vehikel(start, count_road );
				state = NR_ROAD_SUCCESS;
			}
			else {
				state = NR_BAUE_CLEAN_UP;
			}
		break;

		// remove marker etc.
		case NR_BAUE_CLEAN_UP:
		{
			if(ship_vehicle) {
				// only here, if we could built ships but no connection
				halthandle_t start_halt;
				for( int r=0;  r<4;  r++  ) {
					start_halt = haltestelle_t::gib_halt(welt,platz1+koord::nsow[r]);
					if(start_halt.is_bound()  &&  (start_halt->get_station_type()&haltestelle_t::dock)!=0) {
						// delete all ships on this line
						vector_tpl<linehandle_t> lines;
						simlinemgmt.get_lines( simline_t::shipline, &lines );
						if(!lines.empty()) {
							linehandle_t line = lines.back();
							fahrplan_t *fpl=line->get_fahrplan();
							if(fpl->maxi()>1  &&  haltestelle_t::gib_halt(welt,fpl->eintrag[0].pos)==start_halt) {
								while(line->count_convoys()>0) {
									line->get_convoy(0)->self_destruct();
									line->get_convoy(0)->step();
								}
							}
							simlinemgmt.delete_line( line );
						}
						// delete harbour
						const char *msg;
						wkz_remover_intern(this,welt,platz1+koord::nsow[r],msg);
						break;
					}
				}
			}
			// otherwise it may always try to built the same route!
			ziel = NULL;
			// schilder aufraeumen
			grund_t* gr = welt->lookup_kartenboden(platz1);
			if (gr) gr->obj_loesche_alle(this);
			gr = welt->lookup_kartenboden(platz2);
			if (gr) gr->obj_loesche_alle(this);
			baue = false;
			state = CHECK_CONVOI;
			break;
		}

		// successful rail construction
		case NR_RAIL_SUCCESS:
		{
			// tell the player
			char buf[256];
			const koord3d& spos = start->gib_pos();
			const koord3d& zpos = ziel->gib_pos();
			sprintf(buf, translator::translate("%s\nopened a new railway\nbetween %s\nat (%i,%i) and\n%s at (%i,%i)."), gib_name(), translator::translate(start->gib_name()), spos.x, spos.y, translator::translate(ziel->gib_name()), zpos.x, zpos.y);
			message_t::get_instance()->add_message(buf, spos.gib_2d(), message_t::ai,player_nr,rail_engine->gib_basis_bild());

			baue = false;
			state = CHECK_CONVOI;
		}
		break;

		// successful rail construction
		case NR_ROAD_SUCCESS:
		{
			// tell the player
			char buf[256];
			const koord3d& spos = start->gib_pos();
			const koord3d& zpos = ziel->gib_pos();
			sprintf(buf, translator::translate("%s\nnow operates\n%i trucks between\n%s at (%i,%i)\nand %s at (%i,%i)."), gib_name(), count_road, translator::translate(start->gib_name()), spos.x, spos.y, translator::translate(ziel->gib_name()), zpos.x, zpos.y);
			message_t::get_instance()->add_message(buf, spos.gib_2d(), message_t::ai, player_nr, road_vehicle->gib_basis_bild());

			baue = false;
			state = CHECK_CONVOI;
		}
		break;

		// remove stucked vehicles (only from roads!)
		case CHECK_CONVOI:
		{
			for (vector_tpl<convoihandle_t>::const_iterator i = welt->convois_begin(), end = welt->convois_end(); i != end; ++i) {
				const convoihandle_t cnv = *i;
				if(cnv->gib_besitzer()!=this) {
					continue;
				}

				if(cnv->gib_vehikel(0)->gib_waytype()==water_wt) {
					// ships will be only deleted together with the connecting convoi
					continue;
				}

				long gewinn = 0;
				for( int j=0;  j<12;  j++  ) {
					gewinn += cnv->get_finance_history( j, CONVOI_PROFIT );
				}

				// apparently we got the toatlly wrong vehicle here ...
				// (but we will delete it only, if we need, because it may be needed for a chain)
				bool delete_this = (konto_ueberzogen>0)  &&  (gewinn < -(sint32)cnv->calc_restwert());

				// check for empty vehicles (likely stucked) that are making no plus and remove them ...
				// take care, that the vehicle is old enough ...
				if(!delete_this  &&  (welt->get_current_month()-cnv->gib_vehikel(0)->gib_insta_zeit())>6  &&  gewinn<=0  ){
					sint64 goods=0;
					// no goods for six months?
					for( int i=0;  i<6;  i ++) {
						goods += cnv->get_finance_history(i,CONVOI_TRANSPORTED_GOODS);
					}
					delete_this = (goods==0);
				}

				// well, then delete this (likely stucked somewhere) or insanely unneeded
				if(delete_this) {
					waytype_t wt = cnv->gib_vehikel(0)->gib_besch()->get_waytype();
					linehandle_t line = cnv->get_line();
					DBG_MESSAGE("spieler_t::do_ki()","%s retires convoi %s!", gib_name(), cnv->gib_name());

					koord3d start_pos, end_pos;
					fahrplan_t *fpl = cnv->gib_fahrplan();
					if(fpl  &&  fpl->maxi()>1) {
						start_pos = fpl->eintrag[0].pos;
						end_pos = fpl->eintrag[1].pos;
					}

					cnv->self_destruct();
					cnv->step();	// to really get rid of it

					// last vehicle on that connection (no line => railroad)
					if(  !line.is_bound()  ||  line->count_convoys()==0  ) {
						// check if a conncetion boat must be removed
						halthandle_t start_halt = haltestelle_t::gib_halt(welt,start_pos);
						if(start_halt.is_bound()  &&  (start_halt->get_station_type()&haltestelle_t::dock)!=0) {
							// delete all ships on this line
							vector_tpl<linehandle_t> lines;
							koord water_stop = koord::invalid;
							simlinemgmt.get_lines( simline_t::shipline, &lines );
							for (vector_tpl<linehandle_t>::const_iterator iter2 = lines.begin(), end = lines.end(); iter2 != end; iter2++) {
								linehandle_t line = *iter2;
								fahrplan_t *fpl=line->get_fahrplan();
								if(fpl->maxi()>1  &&  haltestelle_t::gib_halt(welt,fpl->eintrag[0].pos)==start_halt) {
									water_stop = koord( (start_pos.x+fpl->eintrag[0].pos.x)/2, (start_pos.y+fpl->eintrag[0].pos.y)/2 );
									while(line->count_convoys()>0) {
										line->get_convoy(0)->self_destruct();
										line->get_convoy(0)->step();
									}
								}
								simlinemgmt.delete_line( line );
							}
							// delete harbour
							const char *msg;
							wkz_remover_intern(this,welt,water_stop,msg);
						}
					}

					value_t v;
					v.i = (int)wt;
					if(wt==track_wt) {
						wkz_wayremover(this,welt,INIT,v);
						wkz_wayremover(this,welt,start_pos.gib_2d(),v);
						wkz_wayremover(this,welt,end_pos.gib_2d(),v);
					}
					else {
						// last convoi => remove completely<
						if(line.is_bound()  &&  line->count_convoys()==0) {
							simlinemgmt.delete_line( line );

							// cannot remove all => likely some other convois there too
							wkz_wayremover(this,welt,start_pos.gib_2d(),v);
							if(!wkz_wayremover(this,welt,end_pos.gib_2d(),v)) {
								const char *msg;
								// remove loading bays and road
								wkz_remover_intern(this,welt,start_pos.gib_2d(),msg);
								wkz_remover_intern(this,welt,start_pos.gib_2d(),msg);
								// remove loading bays and road
								wkz_remover_intern(this,welt,end_pos.gib_2d(),msg);
								wkz_remover_intern(this,welt,end_pos.gib_2d(),msg);
							}
						}
					}
					break;
				}
			}
			state = NR_INIT;
		}
		break;

		default:
			DBG_MESSAGE("spieler_t::do_ki()",	"Illegal state!", state );
			state = NR_INIT;
	}
}



/************************************** from here on passenger AI ******************************/



/* return the hub of a city (always the very first stop) or zero
 * @author prissi
 */
halthandle_t spieler_t::get_our_hub( const stadt_t *s )
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
koord spieler_t::built_hub( const koord pos, int radius )
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

				// flat, solid, and ours
				if (!gr->ist_wasser() && gr->gib_grund_hang() == hang_t::flach) {
					const ding_t* thing = gr->obj_bei(0);
					if (thing == NULL || thing->gib_besitzer() == NULL || thing->gib_besitzer() == this) {
						if(gr->is_halt()  &&  gr->gib_halt()->gib_besitzer()==this) {
							// ok, one halt belongs already to us ...
							return try_pos;
						} else if(gr->hat_weg(road_wt)) {
							ribi_t::ribi  ribi = gr->gib_weg_ribi_unmasked(road_wt);
							if( abs(x)+abs(y)<=dist  &&  (ribi_t::ist_gerade(ribi)  ||  ribi_t::ist_einfach(ribi))    ) {
								best_pos = try_pos;
								dist = abs(x)+abs(y);
							}
						} else if(gr->ist_natur()  &&  abs(x)+abs(y)<=dist-2) {
							// also ok for a stop, but second choice
							// so wie gave it a malus of 2
							best_pos = try_pos;
							dist = abs(x)+abs(y)+2;
						}
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
void spieler_t::create_bus_transport_vehikel(koord startpos2d,int anz_vehikel,koord *stops,int anzahl,bool do_wait)
{
DBG_MESSAGE("spieler_t::create_bus_transport_vehikel()","bus at (%i,%i)",startpos2d.x,startpos2d.y);
	// now start all vehicle one field before, so they load immediately
	koord3d startpos = welt->lookup(startpos2d)->gib_kartenboden()->gib_pos();

	// since 86.01 we use lines for road vehicles ...
	fahrplan_t *fpl=new autofahrplan_t();
	// do not start at current stop => wont work ...
	for(int j=0;  j<anzahl;  j++) {
		fpl->append(welt->lookup(stops[j])->gib_kartenboden(), j == 0 || !do_wait ? 0 : 10);
	}
	fpl->aktuell = (stops[0]==startpos2d);
	linehandle_t line=simlinemgmt.create_line(simline_t::truckline,fpl);
	delete fpl;

	// now create all vehicles as convois
	for(int i=0;  i<anz_vehikel;  i++) {
		vehikel_t* v = vehikelbauer_t::baue(startpos, this, NULL, road_vehicle);
		convoi_t* cnv = new convoi_t(this);
		// V.Meyer: give the new convoi name from first vehicle
		cnv->setze_name(v->gib_besch()->gib_name());
		cnv->add_vehikel( v );

		welt->sync_add( cnv );
		cnv->set_line(line);
		cnv->start();
	}
}



// BUS AI
void spieler_t::do_passenger_ki()
{
	if((steps/MAX_PLAYER_COUNT)&7) {
		return;
	}

	switch(state) {
		case NR_INIT:
		{
			// assume fail
			state = CHECK_CONVOI;

			// if we have this little money, we do nothing
			if(konto<2000000) {
				return;
			}

			const weighted_vector_tpl<stadt_t*>& staedte = welt->gib_staedte();
			int anzahl = staedte.get_count();
			int offset = (anzahl>1)?simrand(anzahl-1):0;
			// start with previous target
			const stadt_t* last_start_stadt=start_stadt;
			start_stadt = end_stadt;
			end_stadt = NULL;
			end_ausflugsziel = NULL;
			ziel = NULL;
			platz2 = koord::invalid;
			// if no previous town => find one
			if(start_stadt==NULL) {
				start_stadt = staedte[offset];
			}
DBG_MESSAGE("spieler_t::do_passenger_ki()","using city %s for start",start_stadt->gib_name());
			const halthandle_t start_halt = get_our_hub(start_stadt);
			// find starting place

if(!start_halt.is_bound()) {
	DBG_MESSAGE("spieler_t::do_passenger_ki()","new_hub");
}
			platz1 = start_halt.is_bound()?start_halt->gib_basis_pos():built_hub(start_stadt->gib_pos(),9);
			if(platz1==koord::invalid) {
				return;
			}
DBG_MESSAGE("spieler_t::do_passenger_ki()","using place (%i,%i) for start",platz1.x,platz1.y);

			if(anzahl==1  ||  simrand(3)==0) {
DBG_MESSAGE("spieler_t::do_passenger_ki()","searching attraction");
				// 25 % of all connections are tourist attractions
				const weighted_vector_tpl<gebaeude_t*> &ausflugsziele = welt->gib_ausflugsziele();
				// this way, we are sure, our factory is connected to this town ...
				const weighted_vector_tpl<fabrik_t *> &fabriken = start_stadt->gib_arbeiterziele();
				unsigned	last_dist = 0xFFFFFFFF;
				bool ausflug=simrand(2)!=0;	// holidays first ...
				int ziel_count=ausflug?ausflugsziele.get_count():fabriken.get_count();
				count = 1;	// one vehicle
#if 0
				// might work better, if finished ...
				if(ausflug) {
					const weighted_vector_tpl<gebaeude_t*> &ausflugsziele = welt->gib_ausflugsziele();
					for(  int k=0;  k<4;  k++  ) {
						gebaeude_t *gb = welt->gib_random_ausflugsziel();
						if(gb->gib_post_level()<=2) {
							// not a good object to go to ...
							continue;
						}
						pos=gb->gib_pos().gib_2d();
						size=gb->gib_tile()->gib_besch()->gib_groesse(bg->gib_tile()->gib_layout());
				}
#endif
				for( int i=0;  i<ziel_count;  i++  ) {
					unsigned	dist;
					koord pos, size;
					if(ausflug) {
						const gebaeude_t* a = ausflugsziele[i];
						if (a->gib_post_level() <= 25) {
							// not a good object to go to ...
							continue;
						}
						pos  = a->gib_pos().gib_2d();
						size = a->gib_tile()->gib_besch()->gib_groesse(a->gib_tile()->gib_layout());
					}
					else {
						const fabrik_t* f = fabriken[i];
						if (f->gib_besch()->gib_pax_level() <= 10) {
							// not a good object to go to ... we want more action ...
							continue;
						}
						pos  = f->gib_pos().gib_2d();
						size = f->gib_besch()->gib_haus()->gib_groesse();
					}
					const stadt_t *next_town = welt->suche_naechste_stadt(pos);
					if(next_town==NULL  ||  start_stadt==next_town) {
						// this is either a town already served (so we do not create a new hub)
						// or a lonely point somewhere
						// in any case we do not want to serve this location already
						koord test_platz=built_hub(pos,size.x);
						if(!is_my_halt(test_platz)) {
							// not served
							dist = abs_distance(platz1,test_platz);
							if(dist+simrand(50)<last_dist  &&   dist>3) {
								// but closer than the others
								if(ausflug) {
									end_ausflugsziel = ausflugsziele[i];
									count = 1;
//									count = 1 + end_ausflugsziel->gib_passagier_level()/128;
//DBG_MESSAGE("spieler_t::do_passenger_ki()","testing attraction %s with %i busses",end_ausflugsziel->gib_tile()->gib_besch()->gib_name(), count);
								}
								else {
									ziel = fabriken[i];
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
					state = NR_SAMMLE_ROUTEN;
DBG_MESSAGE("spieler_t::do_passenger_ki()","decision: %s wants to built network between %s and %s",gib_name(),start_stadt->gib_name(),ausflug?end_ausflugsziel->gib_tile()->gib_besch()->gib_name():ziel->gib_name());
				}
			}
			else {
DBG_MESSAGE("spieler_t::do_passenger_ki()","searching town");
				count = 1;
				// find a good route
				for( int i=0;  i<anzahl;  i++  ) {
					const int nr = (i+offset)%anzahl;
					const stadt_t* cur = staedte[nr];
assert(cur!=NULL);
					if(cur!=last_start_stadt  &&  cur!=start_stadt) {
						int	dist=-1;
						if(end_stadt!=NULL) {
							halthandle_t end_halt = get_our_hub(cur);
DBG_MESSAGE("spieler_t::do_passenger_ki()","found end hub");
							int dist1 = abs_distance(platz1,cur->gib_pos());

							// if there is a not too long route (less than 4 times changes), then do not built a line between
							if(start_halt.is_bound()  &&  end_halt.is_bound()) {
								ware_t pax(warenbauer_t::passagiere);
								pax.setze_zielpos(end_halt->gib_basis_pos());
								INT_CHECK("simplay 838");
								start_halt->suche_route(pax);
								if(!pax.gib_ziel().is_bound()  &&  dist1>welt->gib_groesse_max()/3) {
									// already connected
									continue;
								}
							}
							int dist2 = abs_distance(platz1,end_stadt->gib_pos());
							dist = dist1-dist2;
						}
						// check if more close or NULL
						if(end_stadt==NULL  ||  dist<0  ) {
							end_stadt = cur;
						}
					}
				}
				// ok, found two cities
				if(start_stadt!=NULL  &&  end_stadt!=NULL) {
					state = NR_SAMMLE_ROUTEN;
DBG_MESSAGE("spieler_t::do_passenger_ki()","%s wants to built network between %s and %s",gib_name(),start_stadt->gib_name(),end_stadt->gib_name());
				}
			}
		}
		break;

		// so far only busses
		case NR_SAMMLE_ROUTEN:
		{
			//
			koord end_hub_pos = koord::invalid;
DBG_MESSAGE("spieler_t::do_passenger_ki()","Find hub");
			// also for target (if not tourist attraction!)
			if(end_stadt!=NULL) {
DBG_MESSAGE("spieler_t::do_passenger_ki()","try to built connect to city %p", end_stadt );
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
				state = NR_BAUE_ROUTE1;
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
			uint month_now = (welt->use_timeline() ? welt->get_current_month() : 0);

			// we want the fastest we can get!
			road_vehicle = vehikelbauer_t::vehikel_search( road_wt, month_now, 10, 80, warenbauer_t::passagiere );
			if(road_vehicle!=NULL) {
//						road_weg = wegbauer_t::weg_search( road_wt, road_vehicle->gib_geschw(), welt->get_timeline_year_month(),weg_t::type_flat );
				// find the really cheapest road
				road_weg = wegbauer_t::weg_search( road_wt, 10, welt->get_timeline_year_month(), weg_t::type_flat );
				state = NR_BAUE_STRASSEN_ROUTE;
DBG_MESSAGE("spieler_t::do_passenger_ki()","using %s on %s",road_vehicle->gib_name(),road_weg->gib_name());
			}
			else {
				// no success
				end_stadt = NULL;
				state = CHECK_CONVOI;
			}
			// ok, now I do
			baue = true;
			state = NR_BAUE_STRASSEN_ROUTE;
		}
		break;

		// built a simple road (no bridges, no tunnels)
		case NR_BAUE_STRASSEN_ROUTE:
		{
			const haus_besch_t* bs = hausbauer_t::gib_random_station(haus_besch_t::bushalt, welt->get_timeline_year_month(), haltestelle_t::PAX);
			if(bs  &&  create_simple_road_transport()  &&
				(is_my_halt(platz1)  ||  wkz_halt(this, welt, platz1, bs ))  &&
				(is_my_halt(platz2)  ||  wkz_halt(this, welt, platz2, bs ))
			  ) {
				koord list[2]={ platz1, platz2 };
				// wait only, if traget is not a hub but an attraction/factory
				create_bus_transport_vehikel(platz1,count,list,2,end_stadt==NULL);
//			create_bus_transport_vehikel(platz2,1,list,2);
				state = NR_ROAD_SUCCESS;
			}
			else {
				state = NR_BAUE_CLEAN_UP;
			}
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
			message_t::get_instance()->add_message(buf,platz1,message_t::ai,player_nr,road_vehicle->gib_basis_bild());
		}
		break;


		// add vehicles to crowded lines
		case CHECK_CONVOI:
		{
			for (vector_tpl<convoihandle_t>::const_iterator i = welt->convois_begin(), end = welt->convois_end(); i != end; ++i) {
				const convoihandle_t cnv = *i;
				if(cnv->gib_besitzer()==this) {
					// check for empty vehicles (likely stucked) that are making no plus and remove them ...
					// take care, that the vehicle is old enough ...
					if((welt->get_current_month()-cnv->gib_vehikel(0)->gib_insta_zeit())>2  &&  cnv->gib_jahresgewinn()==0  ){
						sint64 passenger=0;
						for( int i=0;  i<MAX_MONTHS;  i ++) {
							passenger += cnv->get_finance_history(i,CONVOI_TRANSPORTED_GOODS);
						}
						if(passenger==0) {
							// no passengers for two months?
							// well, then delete this (likely stucked somewhere)
DBG_MESSAGE("spieler_t::do_passenger_ki()","convoi %s not needed!",cnv->gib_name());
							cnv->self_destruct();
							break;
						}
					}
					// then check for overcrowded lines
					if(cnv->gib_vehikel(0)->gib_fracht_menge()==cnv->gib_vehikel(0)->gib_fracht_max()  &&  cnv->get_line().is_bound()) {
						INT_CHECK("simplay 889");
						// this is our vehicle, and is 100% loaded
						fahrplan_t  *f = cnv->gib_fahrplan();
						koord3d startpos= cnv->gib_pos();
						// next checkpoint also crowed with things for us?
						halthandle_t h0 = welt->lookup(f->eintrag[0].pos)->gib_halt();
						halthandle_t h1 = welt->lookup(f->eintrag[1].pos)->gib_halt();
						if(!h1.is_bound() ||  !h0.is_bound()) {
							// somebody deleted our stops or messed with the schedules ...
							continue;
						}
DBG_MESSAGE("spieler_t::do_passenger_ki()","checking our convoi %s between %s and %s",cnv->gib_name(),h0->gib_name(),h1->gib_name());
						DBG_MESSAGE(
							"spieler_t::do_passenger_ki()", "waiting: %s (%i) and %s (%i)",
							h0->gib_name(), h0->gib_ware_fuer_zwischenziel(warenbauer_t::passagiere, haltestelle_t::gib_halt(welt,f->eintrag[1].pos)),
							h1->gib_name(), h1->gib_ware_fuer_zwischenziel(warenbauer_t::passagiere, haltestelle_t::gib_halt(welt,f->eintrag[0].pos))
						);

						// we assume crowed for more than 129 waiting passengers
						if (h0->gib_ware_fuer_zwischenziel(warenbauer_t::passagiere, haltestelle_t::gib_halt(welt,f->eintrag[1].pos)) > h0->get_capacity() ||
								h1->gib_ware_fuer_zwischenziel(warenbauer_t::passagiere, haltestelle_t::gib_halt(welt,f->eintrag[0].pos)) > h1->get_capacity()) {
DBG_MESSAGE("spieler_t::do_passenger_ki()","copy convoi %s on route %s to %s",cnv->gib_name(),h0->gib_name(),h1->gib_name());
							vehikel_t* v = vehikelbauer_t::baue(startpos, this, NULL, cnv->gib_vehikel(0)->gib_besch());
							convoi_t* new_cnv = new convoi_t(this);
							// V.Meyer: give the new convoi name from first vehicle
							new_cnv->setze_name( v->gib_besch()->gib_name() );
							new_cnv->add_vehikel( v );
							// and start new convoi
							welt->sync_add( new_cnv );
							new_cnv->set_line(cnv->get_line());
							new_cnv->start();
							// we do not want too many copies, just copy once!
							break;
						}
					}
				}
			}
			state = NR_INIT;
		}
		break;

		default:
			DBG_MESSAGE("spieler_t::do_passenger_ki()",	"Illegal state %d!", state );
			end_stadt = NULL;
			state = NR_INIT;
	}
}



