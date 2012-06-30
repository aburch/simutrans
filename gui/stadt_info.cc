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


#include "stadt_info.h"
#include "karte.h"

#include "../simgraph.h"

#define PAX_DEST_X (138)
#define PAX_DEST_Y (20)
#define PAX_DEST_MARGIN (4)
#define PAX_DEST_MIN_SIZE (16)		// minimum width/height of the minimap
#define PAX_DEST_VERTICAL (4.0/3.0) // aspect factor where minimaps change to over/under instead of left/right

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
	COL_LIGHT_BLUE, 100, COL_LIGHT_YELLOW, COL_YELLOW,
	COL_LIGHT_BROWN, COL_BROWN, 87
};


stadt_info_t::stadt_info_t(stadt_t* stadt_) :
	gui_frame_t( name, NULL ),
	stadt(stadt_),
	pax_dest_old(0,0),
	pax_dest_new(0,0)
{
	reset_city_name();

	minimaps_size = koord(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE); // default minimaps size
	minimap2_offset = koord(minimaps_size.x + PAX_DEST_MARGIN, 0);

	name_input.set_pos(koord(8, 4));
	name_input.set_groesse(koord(126, 13));
	name_input.add_listener( this );

	add_komponente(&name_input);

	allow_growth.init( button_t::square_state, "Allow city growth", koord(8, 4 + (D_BUTTON_HEIGHT+2) + 8*LINESPACE) );
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

	// tab (month/year)
	year_month_tabs.add_tab(&chart, translator::translate("Years"));
	year_month_tabs.add_tab(&mchart, translator::translate("Months"));
	add_komponente(&year_month_tabs);

	// add filter buttons          skip electricity
	for(  int hist=0;  hist<MAX_CITY_HISTORY-1;  hist++  ) {
		filterButtons[hist].init(button_t::box_state, hist_type[hist], koord(0,0), koord(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
		filterButtons[hist].background = hist_type_color[hist];
		filterButtons[hist].pressed = (stadt->stadtinfo_options & (1<<hist))!=0;
		filterButtons[hist].add_listener(this);
		add_komponente(filterButtons + hist);
	}

	pax_destinations_last_change = stadt->get_pax_destinations_new_change();

	set_fenstergroesse(koord(PAX_DEST_X + PAX_DESTINATIONS_SIZE + PAX_DEST_MARGIN*2 + 1, 342));
	set_min_windowsize(koord(D_DEFAULT_WIDTH, 256));

	set_resizemode(diagonal_resize);
	resize(koord(0,0));
}


void stadt_info_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);

	// calculate layout of filter buttons
	const int col = max( 1, min( (get_fenstergroesse().x-2)/(D_BUTTON_WIDTH+D_H_SPACE), MAX_CITY_HISTORY-1 ) );
	const int row = ((MAX_CITY_HISTORY-2)/col)+1;

	// calculate new minimaps size : expand horizontally or vertically ?
	const karte_t* const welt = stadt_t::get_welt();
	const float world_aspect = (float)welt->get_groesse_x() / (float)welt->get_groesse_y();

	const koord space = koord(get_fenstergroesse().x - PAX_DEST_X - PAX_DEST_MARGIN - 1, max( allow_growth.get_pos().y + LINESPACE+1 - 5, get_fenstergroesse().y - 166 - (D_BUTTON_HEIGHT+2)*row ));
	const float space_aspect = (float)space.x / (float)space.y;

	if(  world_aspect / space_aspect > PAX_DEST_VERTICAL  ) { // world wider than space, use vertical minimap layout
		minimaps_size.y = (space.y - PAX_DEST_MARGIN) / 2;
		minimaps_size.x = (sint16) ((float)minimaps_size.y * world_aspect);
		if(  minimaps_size.x  > space.x  ) {
			minimaps_size.x = space.x;
			minimaps_size.y = max((int)((float)minimaps_size.x / world_aspect), PAX_DEST_MIN_SIZE);
		}
		minimap2_offset = koord( 0, minimaps_size.y + PAX_DEST_MARGIN );
	}
	else { // horizontal minimap layout
		minimaps_size.x = (space.x - PAX_DEST_MARGIN) / 2;
		minimaps_size.y = (sint16) ((float)minimaps_size.x / world_aspect);
		if(  minimaps_size.y > space.y  ) {
			minimaps_size.y = space.y;
			minimaps_size.x = max((int)((float)minimaps_size.y * world_aspect), PAX_DEST_MIN_SIZE);
		}
		minimap2_offset = koord( minimaps_size.x + PAX_DEST_MARGIN, 0 );
	}

	// resize minimaps
	pax_dest_old.resize( minimaps_size.x, minimaps_size.y );
	pax_dest_new.resize( minimaps_size.x, minimaps_size.y );

	// reinit minimaps data
	init_pax_dest( pax_dest_old );
	pax_dest_new = pax_dest_old;
	add_pax_dest( pax_dest_old, stadt->get_pax_destinations_old() );
	add_pax_dest( pax_dest_new, stadt->get_pax_destinations_new() );

	// move and resize charts
	year_month_tabs.set_pos(koord(60, max( allow_growth.get_pos().y + LINESPACE, (world_aspect / space_aspect > PAX_DEST_VERTICAL ? minimaps_size.y*2 + PAX_DEST_MARGIN : minimaps_size.y) + PAX_DEST_MARGIN )) );
	year_month_tabs.set_groesse(koord(get_fenstergroesse().x - 80, get_fenstergroesse().y - D_TITLEBAR_HEIGHT - year_month_tabs.get_pos().y - 4 - (D_BUTTON_HEIGHT+2)*(row+1) - 1 ));

	// move and resize filter buttons
	for(  int hist=0;  hist<MAX_CITY_HISTORY-1;  hist++  ) {
		const koord pos = koord(2 + (D_BUTTON_WIDTH+D_H_SPACE)*(hist%col), get_fenstergroesse().y - (D_BUTTON_HEIGHT+2)*(row+1) - 1 + (D_BUTTON_HEIGHT+2)*((int)hist/col) );
		filterButtons[hist].set_pos( pos );
	}
}


stadt_info_t::~stadt_info_t()
{
	// send rename command if necessary
	rename_city();
}


// returns position of depot on the map
koord3d stadt_info_t::get_weltpos(bool)
{
	return stadt->get_welt()->lookup_kartenboden( stadt->get_pos() )->get_pos();
}


bool stadt_info_t::is_weltpos()
{
	karte_t *welt = stadt->get_welt();
	return ( welt->get_x_off() | welt->get_y_off()) == 0  &&
		welt->get_world_position() == welt->calculate_world_position( get_weltpos(false) );
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
	const int gr_x = welt->get_groesse_x();
	const int gr_y = welt->get_groesse_y();
	for(  sint16 y = 0;  y < minimaps_size.y;  y++  ) {
		for(  sint16 x = 0;  x < minimaps_size.x;  x++  ) {
			const grund_t *gr = welt->lookup_kartenboden( koord( (x * gr_x) / minimaps_size.x, (y * gr_y) / minimaps_size.y ) );
			pax_dest.at(x,y) = reliefkarte_t::calc_relief_farbe(gr);
		}
	}
}


void stadt_info_t::add_pax_dest( array2d_tpl<uint8> &pax_dest, const sparse_tpl< uint8 >* city_pax_dest )
{
	uint8 color;
	koord pos;
	// how large the box in the world?
	const sint16 dd_x = 1+(minimaps_size.x-1)/PAX_DESTINATIONS_SIZE;
	const sint16 dd_y = 1+(minimaps_size.y-1)/PAX_DESTINATIONS_SIZE;

	for( uint16 i = 0;  i < city_pax_dest->get_data_count();  i++  ) {
		city_pax_dest->get_nonzero(i, pos, color);

		// calculate display position according to minimap size
		const sint16 x0 = (pos.x*minimaps_size.x)/PAX_DESTINATIONS_SIZE;
		const sint16 y0 = (pos.y*minimaps_size.y)/PAX_DESTINATIONS_SIZE;

		for(  sint32 y=0;  y<dd_y  &&  y+y0<minimaps_size.y;  y++  ) {
			for(  sint32 x=0;  x<dd_x  &&  x+x0<minimaps_size.x;  x++  ) {
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
	buf.append( ": \n" );
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

	display_multiline_text(pos.x + 8, pos.y + D_TITLEBAR_HEIGHT + 4 + (D_BUTTON_HEIGHT+2), buf, COL_BLACK);

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

	display_array_wh(pos.x + PAX_DEST_X, pos.y + PAX_DEST_Y, minimaps_size.x, minimaps_size.y, pax_dest_old.to_array() );
	display_array_wh(pos.x + PAX_DEST_X + minimap2_offset.x, pos.y + PAX_DEST_Y + minimap2_offset.y, minimaps_size.x, minimaps_size.y, pax_dest_new.to_array() );
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
	resize(koord(0,0));
	pax_destinations_last_change = stadt->get_pax_destinations_new_change();
}


// current task: just update the city pointer ...
bool stadt_info_t::infowin_event(const event_t *ev)
{
	if(  IS_WINDOW_TOP(ev)  ) {
		reliefkarte_t::get_karte()->set_city( stadt );
	}

	uint16 my = ev->my;
	if(  my > PAX_DEST_Y + minimaps_size.y  &&  minimap2_offset.y > 0  ) {
		// Little trick to handle both maps with the same code: Just remap the y-values of the bottom map.
		my -= minimaps_size.y + PAX_DEST_MARGIN;
	}

	if(  ev->ev_class!=EVENT_KEYBOARD  &&  ev->ev_code==MOUSE_LEFTBUTTON  &&  PAX_DEST_Y<=my  &&  my<PAX_DEST_Y+minimaps_size.y  ) {
		uint16 mx = ev->mx;
		if(  mx > PAX_DEST_X + minimaps_size.x  &&  minimap2_offset.x > 0  ) {
			// Little trick to handle both maps with the same code: Just remap the x-values of the right map.
			mx -= minimaps_size.x + PAX_DEST_MARGIN;
		}

		if(  PAX_DEST_X <= mx && mx < PAX_DEST_X + minimaps_size.x  ) {
			// Clicked in a minimap.
			mx -= PAX_DEST_X;
			my -= PAX_DEST_Y;
			const koord p = koord(
				(mx * stadt->get_welt()->get_groesse_x()) / (minimaps_size.x),
				(my * stadt->get_welt()->get_groesse_y()) / (minimaps_size.y));
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
