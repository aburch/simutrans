/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simdebug.h"
#include "../simcity.h"
#include "../simmenu.h"
#include "../simworld.h"
#include "../simcolor.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"
#include "components/list_button.h"

#include "stadt_info.h"
#include "karte.h"

#include "../simgraph.h"

#define PAX_DEST_X (140)
#define PAX_DEST_Y (24)
#define PAX_DEST_MARGIN (4)

// minimal window size
#define MIN_WIN_WIDTH 410
#define MIN_WIN_HEIGHT 365

// @author hsiegeln
const char *hist_type[MAX_CITY_HISTORY] =
{
	"citicens", "Growth", "Buildings", "Verkehrsteilnehmer",
	"Transported", "Passagiere", "sended", "Post",
	"Arrived", "Goods", "Electricity"
};


const int hist_type_color[MAX_CITY_HISTORY] =
{
	COL_WHITE, COL_DARK_GREEN, COL_LIGHT_PURPLE, 110,
	COL_LIGHT_BLUE, COL_BLUE, COL_LIGHT_YELLOW, COL_YELLOW,
	COL_LIGHT_BROWN, COL_BROWN, 87
};


stadt_info_t::stadt_info_t(stadt_t* stadt_) :
	gui_frame_t( name, NULL ),
	stadt(stadt_),
	pax_dest_old(0,0),
	pax_dest_new(0,0)
{
	reset_city_name();

	minimapSize = PAX_DESTINATIONS_SIZE;	// default minimaps size

	name_input.set_groesse(koord(124, 14));
	name_input.set_pos(koord(8, 8));
	name_input.add_listener( this );

	add_komponente(&name_input);

	allow_growth.init( button_t::square_state, "Allow city growth", koord(8,104) );
	allow_growth.pressed = stadt->get_citygrowth();
	allow_growth.add_listener( this );
	add_komponente(&allow_growth);

	//CHART YEAR
	chart.set_pos(koord(21,1));
	chart.set_groesse(koord(340,120));
	chart.set_dimension(MAX_CITY_HISTORY_YEARS, 10000);
	chart.set_seed(stadt->get_welt()->get_last_year());
	chart.set_background(MN_GREY1);
	for(  uint32 i = 0;  i<MAX_CITY_HISTORY;  i++  ) {
		chart.add_curve( hist_type_color[i], stadt->get_city_history_year(), MAX_CITY_HISTORY, i, 12, STANDARD, (stadt->stadtinfo_options & (1<<i))!=0, true, 0 );
	}
	//CHART YEAR END

	//CHART MONTH
	mchart.set_pos(koord(21,1));
	mchart.set_groesse(koord(340,120));
	mchart.set_dimension(MAX_CITY_HISTORY_MONTHS, 10000);
	mchart.set_seed(0);
	mchart.set_background(MN_GREY1);
	for(  uint32 i = 0;  i<MAX_CITY_HISTORY;  i++  ) {
		mchart.add_curve( hist_type_color[i], stadt->get_city_history_month(), MAX_CITY_HISTORY, i, 12, STANDARD, (stadt->stadtinfo_options & (1<<i))!=0, true, 0 );
	}
	mchart.set_visible(false);
	//CHART MONTH END

	// tab (month/year)
	year_month_tabs.add_tab(&chart, translator::translate("Years"));
	year_month_tabs.add_tab(&mchart, translator::translate("Months"));
	add_komponente(&year_month_tabs);

	// add filter buttons
	for(  int hist=0;  hist<MAX_CITY_HISTORY-1;  hist++  ) {
		filterButtons[hist].init(button_t::box_state, translator::translate(hist_type[hist]), koord(4+(hist%4)*100,290+(hist/4)*(BUTTON_HEIGHT+4)), koord(96, BUTTON_HEIGHT));
		filterButtons[hist].background = hist_type_color[hist];
		filterButtons[hist].pressed = (stadt->stadtinfo_options & (1<<hist))!=0;
		// skip electricity
		filterButtons[hist].add_listener(this);
		add_komponente(filterButtons + hist);
	}

	pax_destinations_last_change = stadt->get_pax_destinations_new_change();

	// size window and set resizable
	set_resizemode(diagonal_resize);
	set_min_windowsize(koord(MIN_WIN_WIDTH, MIN_WIN_HEIGHT));
	set_fenstergroesse(koord(MIN_WIN_WIDTH, MIN_WIN_HEIGHT));
}


/**
 * Set window size and adjust component sizes and/or positions accordingly
 */
void stadt_info_t::set_fenstergroesse(koord groesse)
{
	// resize window
	gui_frame_t::set_fenstergroesse(groesse);

	// move and resize filter buttons
	const sint16 button_per_row = (groesse.x-8)/100;
	const sint16 button_rows = 1+(MAX_CITY_HISTORY-2)/button_per_row;

	// calculate new minimaps size : expand horizontally or vertically ?
	int spaceY = groesse.y - 176 - (BUTTON_HEIGHT+4)*button_rows;
	int spaceX = (int)((groesse.x - PAX_DEST_X - PAX_DEST_MARGIN*2 )/2);
	minimapSize = min( spaceX, spaceY );

	// resize minimaps
	pax_dest_old.resize( minimapSize, minimapSize );
	pax_dest_new.resize( minimapSize, minimapSize );

	// reinit minimaps data
	init_pax_dest( pax_dest_old );
	pax_dest_new = pax_dest_old;
	add_pax_dest( pax_dest_old, stadt->get_pax_destinations_old() );
	add_pax_dest( pax_dest_new, stadt->get_pax_destinations_new() );

	// move and resize charts
	year_month_tabs.set_pos(koord(60, minimapSize + PAX_DEST_MARGIN*3));
	year_month_tabs.set_groesse(koord(groesse.x-80, groesse.y - year_month_tabs.get_pos().y - 46 - (BUTTON_HEIGHT+4)*button_rows ));

	// move and resize filter buttons
	for(  int hist=0;  hist<MAX_CITY_HISTORY-1;  hist++  ) {
		filterButtons[hist].set_pos( koord( 4+(hist%button_per_row)*100, groesse.y-4+(hist/button_per_row-button_rows-1)*(BUTTON_HEIGHT+4) ) );
	}
}


stadt_info_t::~stadt_info_t()
{
	// send rename command if necessary
	rename_city();
}


// returns position of depot on the map
koord3d stadt_info_t::get_weltpos()
{
	return koord3d( stadt->get_pos(), 0 );
}


/**
 * send rename command if necessary
 */
void stadt_info_t::rename_city()
{
	if (stadt->get_welt()->get_staedte().is_contained(stadt)) {
		const char *t = name_input.get_text();
		// only change if old name and current name are the same
		// otherwise some unintended undo if renaming would occur
		if(  t  &&  t[0]  &&  strcmp(t, stadt->get_name())  &&  strcmp(old_name, stadt->get_name())==0) {
			// text changed => call tool
			cbuffer_t buf;
			buf.printf( "t%u,%s", stadt->get_welt()->get_staedte().index_of(stadt), name );
			werkzeug_t *w = create_tool( WKZ_RENAME_TOOL | SIMPLE_TOOL );
			w->set_default_param( buf );
			karte_t* const welt = stadt->get_welt();
			welt->set_werkzeug( w, welt->get_spieler(1));
			// since init always returns false, it is save to delete immediately
			delete w;
			// do not trigger this command again
			tstrncpy(old_name, t, sizeof(old_name));
		}
	}
}


void stadt_info_t::reset_city_name()
{
	// change text input
	if (stadt->get_welt()->get_staedte().is_contained(stadt)) {
		tstrncpy(old_name, stadt->get_name(), sizeof(old_name));
		tstrncpy(name, stadt->get_name(), sizeof(name));
		name_input.set_text(name, sizeof(name));
	}
}


void stadt_info_t::init_pax_dest( array2d_tpl<uint8> &pax_dest )
{
	karte_t *welt = stadt_t::get_welt();
	const uint32 gr_x = welt->get_groesse_x();
	const uint32 gr_y = welt->get_groesse_y();
	for(  uint16 y = 0;  y < minimapSize;  y++  ) {
		for(  uint16 x = 0;  x < minimapSize;  x++  ) {
			const grund_t *gr = welt->lookup_kartenboden( koord( (x * gr_x) / minimapSize, (y * gr_y) / minimapSize ) );
			pax_dest.at(x,y) = reliefkarte_t::calc_relief_farbe(gr);
		}
	}
}


void stadt_info_t::add_pax_dest( array2d_tpl<uint8> &pax_dest, const sparse_tpl< uint8 >* city_pax_dest )
{
	uint8 color;
	koord pos;
	// how large the box in the world?
	const uint16 dd = 1+(minimapSize-1)/PAX_DESTINATIONS_SIZE;

	for( uint16 i = 0;  i < city_pax_dest->get_data_count();  i++  ) {
		city_pax_dest->get_nonzero(i, pos, color);

		// calculate display position according to minimap size
		uint32 x0 = (pos.x*minimapSize)/PAX_DESTINATIONS_SIZE;
		uint32 y0 = (pos.y*minimapSize)/PAX_DESTINATIONS_SIZE;

		for(  uint32 y=0;  y<dd  &&  y+y0<minimapSize;  y++  ) {
			for(  uint32 x=0;  x<dd  &&  x+x0<minimapSize;  x++  ) {
				pax_dest.at( x+x0, y+y0 ) = color;
			}
		}
	}
}


void stadt_info_t::zeichnen(koord pos, koord gr)
{
	stadt_t* const c = stadt;

	// Hajo: update chart seed
	chart.set_seed(c->get_welt()->get_last_year());

	gui_frame_t::zeichnen(pos, gr);

	static cbuffer_t buf;
	buf.clear();

	buf.append( translator::translate("City size") );
	buf.append( ": " );
	buf.append( c->get_einwohner(), 0 );
	buf.append( " (" );
	buf.append( c->get_wachstum() / 10.0, 1 );
	buf.append( ") \n" );
	buf.printf( translator::translate("%d buildings\n"), c->get_buildings() );

	const koord ul = c->get_linksoben();
	const koord lr = c->get_rechtsunten();
	buf.printf( "\n%d,%d - %d,%d\n\n", ul.x, ul.y, lr.x , lr.y);

	buf.append( translator::translate("Unemployed") );
	buf.append( ": " );
	buf.append( c->get_unemployed(), 0 );
	buf.append( "\n" );
	buf.append( translator::translate("Homeless") );
	buf.append( ": " );
	buf.append( c->get_homeless(), 0 );

	display_multiline_text(pos.x + 8, pos.y + 48, buf, COL_BLACK);

	const unsigned long current_pax_destinations = c->get_pax_destinations_new_change();
	if(  pax_destinations_last_change > current_pax_destinations  ) {
		// new month started
		pax_dest_old = pax_dest_new;
		init_pax_dest( pax_dest_new );
		add_pax_dest( pax_dest_new, c->get_pax_destinations_new());
	}
	else if(  pax_destinations_last_change != current_pax_destinations  ) {
		// Since there are only new colors, this is enough:
		add_pax_dest( pax_dest_new, c->get_pax_destinations_new() );
	}
	pax_destinations_last_change =  current_pax_destinations;

	display_array_wh(pos.x + PAX_DEST_X, pos.y + PAX_DEST_Y, minimapSize, minimapSize, pax_dest_old.to_array() );
	display_array_wh(pos.x + PAX_DEST_X + minimapSize + PAX_DEST_MARGIN, pos.y + PAX_DEST_Y, minimapSize, minimapSize, pax_dest_new.to_array() );
}


bool stadt_info_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	static char param[16];
	if(  komp==&allow_growth  ) {
		sprintf(param,"g%hi,%hi,%hi", stadt->get_pos().x, stadt->get_pos().y, !stadt->get_citygrowth() );
		karte_t *welt = stadt->get_welt();
		werkzeug_t::simple_tool[WKZ_CHANGE_CITY_TOOL]->set_default_param( param );
		welt->set_werkzeug( werkzeug_t::simple_tool[WKZ_CHANGE_CITY_TOOL],welt->get_active_player());
		return true;
	}
	if(  komp==&name_input  ) {
		// send rename command if necessary
		rename_city();
	}
	else {
		for ( int i = 0; i<MAX_CITY_HISTORY; i++) {
			if (komp == &filterButtons[i]) {
				filterButtons[i].pressed ^= 1;
				if (filterButtons[i].pressed) {
					stadt->stadtinfo_options |= (1<<i);
					chart.show_curve(i);
					mchart.show_curve(i);
				}
				else {
					stadt->stadtinfo_options &= ~(1<<i);
					chart.hide_curve(i);
					mchart.hide_curve(i);
				}
				return true;
			}
		}
	}
	return false;
}


void stadt_info_t::map_rotate90( sint16 )
{
	init_pax_dest( pax_dest_old );
	pax_dest_new = pax_dest_old;
	add_pax_dest( pax_dest_old, stadt->get_pax_destinations_old());
	add_pax_dest( pax_dest_new, stadt->get_pax_destinations_new());
	pax_destinations_last_change = stadt->get_pax_destinations_new_change();
}


// current task: just update the city pointer ...
bool stadt_info_t::infowin_event(const event_t *ev)
{
	if(  IS_WINDOW_TOP(ev)  ) {
		reliefkarte_t::get_karte()->set_city( stadt );
	}

	if(  ev->ev_class!=EVENT_KEYBOARD  &&  ev->ev_code==MOUSE_LEFTBUTTON  &&  PAX_DEST_Y<=ev->my  &&  ev->my<PAX_DEST_Y+minimapSize  ) {
		uint16 mx = ev->mx;
		if( mx > PAX_DEST_X + minimapSize ) {
			// Little trick to handle both maps with the same code: Just remap the x-values of the right map.
			mx -= minimapSize + PAX_DEST_MARGIN;
		}
		if( PAX_DEST_X <= mx && mx < PAX_DEST_X + minimapSize ) {
			// Clicked in a minimap.
			mx -= PAX_DEST_X;
			const uint16 my = ev->my - PAX_DEST_Y;
			const koord p = koord(
				(mx * stadt->get_welt()->get_groesse_x()) / (minimapSize),
				(my * stadt->get_welt()->get_groesse_y()) / (minimapSize));
			stadt->get_welt()->change_world_position( p );
		}
	}

	return gui_frame_t::infowin_event(ev);
}


void stadt_info_t::update_data()
{
	allow_growth.pressed = stadt->get_citygrowth();
	if (strcmp(old_name, stadt->get_name())) {
		reset_city_name();
	}
	set_dirty();
}
