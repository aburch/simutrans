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
#include "../dataobj/environment.h"
#include "../display/viewport.h"
#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"


#include "city_info.h"
#include "karte.h"

#include "../display/simgraph.h"

#define PAX_DEST_X (D_MARGIN_LEFT+2*D_BUTTON_WIDTH+D_V_SPACE)
#define PAX_DEST_Y (D_TITLEBAR_HEIGHT+D_MARGIN_TOP)
#define PAX_DEST_MIN_SIZE (16)		///< minimum width/height of the minimap
#define PAX_DEST_VERTICAL (4.0/3.0)	///< aspect factor where minimaps change to over/under instead of left/right
#define MIN_CHART_HEIGHT (166)		///< minimum size of the city charts
//#define CHART_OFFSET (40)			///< Chart offset from left margin

// @author hsiegeln
const char *hist_type[MAX_CITY_HISTORY] =
{
	"citicens", "Growth", "Buildings", "Verkehrsteilnehmer",
	"Transported", "walking", "Passagiere", "sended", "directmail", "Post",
	"Arrived", "Goods", "Electricity"
};


const uint8 hist_type_color[MAX_CITY_HISTORY] =
{
	COL_WHITE, COL_DARK_GREEN, COL_LIGHT_PURPLE, COL_CONSTRUCTION,
	COL_LIGHT_BLUE, COL_DARK_BLUE, COL_SOFT_BLUE, COL_LIGHT_YELLOW, COL_DARK_YELLOW, COL_YELLOW,
	COL_LIGHT_BROWN, COL_BROWN, COL_OPS_PROFIT
};


city_info_t::city_info_t(stadt_t* city) :
	gui_frame_t( name, NULL ),
	city(city),
	pax_dest_old(0,0),
	pax_dest_new(0,0)
{
	reset_city_name();

	minimaps_size = scr_size(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE); // default minimaps size
	minimap2_offset = scr_coord(minimaps_size.w + D_H_SPACE, 0);

	// add city name input field
	name_input.set_pos(scr_coord(D_MARGIN_LEFT, D_MARGIN_TOP));
	name_input.set_size(scr_size(2*D_BUTTON_WIDTH, D_EDIT_HEIGHT));
	name_input.add_listener( this );
	add_component(&name_input);

	// add "allow city growth" button below city info
	allow_growth.init( button_t::square_state, "Allow city growth",
		scr_coord(D_MARGIN_LEFT, D_MARGIN_TOP + D_BUTTON_HEIGHT + D_V_SPACE+ 8*LINESPACE + D_V_SPACE) );
	allow_growth.pressed = city->get_citygrowth();
	allow_growth.add_listener( this );
	add_component(&allow_growth);

	// add city charts
	// year chart
	chart.set_pos(scr_coord(D_MARGIN_LEFT, 1));
	chart.set_size(scr_size(340,120));
	chart.set_dimension(MAX_CITY_HISTORY_YEARS, 10000);
	chart.set_seed(welt->get_last_year());
	chart.set_background(SYSCOL_CHART_BACKGROUND);
	for(  uint32 i = 0;  i<MAX_CITY_HISTORY;  i++  ) {
		chart.add_curve( color_idx_to_rgb(hist_type_color[i]), city->get_city_history_year(),
			MAX_CITY_HISTORY, i, 12, STANDARD, (city->stadtinfo_options & (1<<i))!=0, true, 0 );
	}

	// month chart
	mchart.set_pos(scr_coord(D_MARGIN_LEFT,1));
	mchart.set_size(scr_size(340,120));
	mchart.set_dimension(MAX_CITY_HISTORY_MONTHS, 10000);
	mchart.set_seed(0);
	mchart.set_background(SYSCOL_CHART_BACKGROUND);
	for(  uint32 i = 0;  i<MAX_CITY_HISTORY;  i++  ) {
		mchart.add_curve( color_idx_to_rgb(hist_type_color[i]), city->get_city_history_month(),
			MAX_CITY_HISTORY, i, 12, STANDARD, (city->stadtinfo_options & (1<<i))!=0, true, 0 );
	}
	mchart.set_visible(false);

	// tab (month/year)
	year_month_tabs.add_tab(&chart, translator::translate("Years"));
	year_month_tabs.add_tab(&mchart, translator::translate("Months"));
	add_component(&year_month_tabs);

	// add filter buttons; skip electricity
	for(  int hist=0;  hist<MAX_CITY_HISTORY-1;  hist++  ) {
		filterButtons[hist].init(button_t::box_state, hist_type[hist], scr_coord(0,0), scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
		filterButtons[hist].background_color = color_idx_to_rgb(hist_type_color[hist]);
		filterButtons[hist].pressed = (city->stadtinfo_options & (1<<hist))!=0;
		filterButtons[hist].add_listener(this);
		add_component(filterButtons + hist);
	}

	pax_destinations_last_change = city->get_pax_destinations_new_change();

	const int window_height = D_TITLEBAR_HEIGHT + D_MARGIN_TOP + D_BUTTON_HEIGHT + D_V_SPACE +
		8*LINESPACE + D_V_SPACE + D_CHECKBOX_HEIGHT + D_V_SPACE + MIN_CHART_HEIGHT + 2*(D_BUTTON_HEIGHT + D_V_SPACE);

	set_windowsize(scr_size(D_DEFAULT_WIDTH, window_height));
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, window_height));

	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));
}


void city_info_t::resize(const scr_coord delta)
{
	gui_frame_t::resize(delta);

	// calculate layout of filter buttons
	const int cols = max( 1, min( (get_windowsize().w-D_MARGIN_LEFT-D_MARGIN_RIGHT+D_H_SPACE)/(D_BUTTON_WIDTH+D_H_SPACE), MAX_CITY_HISTORY-1 ) );
	const int rows = ((MAX_CITY_HISTORY-2)/cols)+1;

	// calculate new minimaps size : expand horizontally or vertically ?
	const float world_aspect = (float)welt->get_size().x / (float)welt->get_size().y;

	const scr_coord space = scr_coord(get_windowsize().w - PAX_DEST_X - D_MARGIN_RIGHT,
		max( allow_growth.get_pos().y + D_BUTTON_HEIGHT, get_windowsize().h-D_MARGINS_Y-D_V_SPACE-MIN_CHART_HEIGHT-(rows+1)*D_BUTTON_HEIGHT-rows*D_V_SPACE));
	const float space_aspect = (float)space.x / (float)space.y;

	if(  world_aspect/space_aspect > PAX_DEST_VERTICAL  ) { // world wider than space, use vertical minimap layout
		minimaps_size.h = (space.y - D_V_SPACE) / 2;
		if(  minimaps_size.h * world_aspect <= space.x) {
			// minimap fits
			minimaps_size.w = (sint16) (minimaps_size.h * world_aspect);
		}
		else {
			// too large, truncate
			minimaps_size.w = space.x;
			minimaps_size.h = max((int)(minimaps_size.w / world_aspect), PAX_DEST_MIN_SIZE);
		}
		minimap2_offset = scr_coord( 0, minimaps_size.h + D_V_SPACE );
	}
	else { // horizontal minimap layout
		minimaps_size.w = (space.x - D_H_SPACE) / 2;
		if (minimaps_size.w / world_aspect <= space.y) {
			// minimap fits
			minimaps_size.h = (sint16) (minimaps_size.w / world_aspect);
		}
		else {
			// too large, truncate
			minimaps_size.h = space.y;
			minimaps_size.w = max((int)(minimaps_size.h * world_aspect), PAX_DEST_MIN_SIZE);
		}
		minimap2_offset = scr_coord( minimaps_size.w + D_H_SPACE, 0 );
	}

	// resize minimaps
	pax_dest_old.resize( minimaps_size.w, minimaps_size.h );
	pax_dest_new.resize( minimaps_size.w, minimaps_size.h );

	// reinit minimaps data
	init_pax_dest( pax_dest_old );
	pax_dest_new = pax_dest_old;
	add_pax_dest( pax_dest_old, city->get_pax_destinations_old() );
	add_pax_dest( pax_dest_new, city->get_pax_destinations_new() );

	// move and resize charts
	year_month_tabs.set_pos(scr_coord(D_MARGIN_LEFT,
		D_MARGIN_TOP + max(D_BUTTON_HEIGHT+D_V_SPACE+8*LINESPACE+D_V_SPACE+D_CHECKBOX_HEIGHT+D_V_SPACE,
		(world_aspect / space_aspect > PAX_DEST_VERTICAL ? 2*minimaps_size.h+D_V_SPACE : minimaps_size.h) + D_V_SPACE )) );

	year_month_tabs.set_size(scr_size(get_windowsize().w - D_MARGIN_RIGHT-D_MARGIN_LEFT,
		get_windowsize().h-year_month_tabs.get_pos().y - D_MARGIN_BOTTOM - (D_BUTTON_HEIGHT+D_V_SPACE)*(rows+1) - 1 ));

	// move and resize filter buttons
	for(  int i=0;  i<MAX_CITY_HISTORY-1;  i++  ) {
		const scr_coord pos = scr_coord(D_MARGIN_LEFT + (D_BUTTON_WIDTH+D_H_SPACE)*(i%cols),
			get_windowsize().h-D_MARGIN_BOTTOM -(rows+1)*D_BUTTON_HEIGHT-rows*D_V_SPACE + (D_BUTTON_HEIGHT+D_V_SPACE)*(i/cols) );
		filterButtons[i].set_pos( pos );
	}
}


city_info_t::~city_info_t()
{
	// send rename command if necessary
	rename_city();
}


// returns position of the city center on the map
koord3d city_info_t::get_weltpos(bool)
{
	return welt->lookup_kartenboden( city->get_center() )->get_pos();
}


bool city_info_t::is_weltpos()
{
	return (welt->get_viewport()->is_on_center( get_weltpos(false)));
}


/**
 * send rename command if necessary
 */
void city_info_t::rename_city()
{
	if(  welt->get_cities().is_contained(city)  ) {
		const char *t = name_input.get_text();
		// only change if old name and current name are the same
		// otherwise some unintended undo if renaming would occur
		if(  t  &&  t[0]  &&  strcmp(t, city->get_name())!=0  &&  strcmp(old_name, city->get_name())==0  ) {
			// text changed => call tool
			cbuffer_t buf;
			buf.printf( "t%u,%s", welt->get_cities().index_of(city), name );
			tool_t *tmp_tool = create_tool( TOOL_RENAME | SIMPLE_TOOL );
			tmp_tool->set_default_param( buf );
			welt->set_tool( tmp_tool, welt->get_public_player());
			// since init always returns false, it is safe to delete immediately
			delete tmp_tool;
			// do not trigger this command again
			tstrncpy(old_name, t, sizeof(old_name));
		}
	}
}


void city_info_t::reset_city_name()
{
	// change text input
	if(  welt->get_cities().is_contained(city)  ) {
		tstrncpy(old_name, city->get_name(), sizeof(old_name));
		tstrncpy(name, city->get_name(), sizeof(name));
		name_input.set_text(name, sizeof(name));
	}
}


void city_info_t::init_pax_dest( array2d_tpl<PIXVAL> &pax_dest )
{
	const int size_x = welt->get_size().x;
	const int size_y = welt->get_size().y;
	for(  sint16 y = 0;  y < minimaps_size.h;  y++  ) {
		for(  sint16 x = 0;  x < minimaps_size.w;  x++  ) {
			const grund_t *gr = welt->lookup_kartenboden( koord( (x * size_x) / minimaps_size.w, (y * size_y) / minimaps_size.h ) );
			pax_dest.at(x,y) = reliefkarte_t::calc_relief_farbe(gr);
		}
	}
}


void city_info_t::add_pax_dest( array2d_tpl<PIXVAL> &pax_dest, const sparse_tpl<PIXVAL>* city_pax_dest )
{
	PIXVAL color;
	koord pos;
	// how large the box in the world?
	const sint16 dd_x = 1+(minimaps_size.w-1)/PAX_DESTINATIONS_SIZE;
	const sint16 dd_y = 1+(minimaps_size.h-1)/PAX_DESTINATIONS_SIZE;

	for(  uint16 i = 0;  i < city_pax_dest->get_data_count();  i++  ) {
		city_pax_dest->get_nonzero(i, pos, color);

		// calculate display position according to minimap size
		const sint16 x0 = (pos.x*minimaps_size.w)/PAX_DESTINATIONS_SIZE;
		const sint16 y0 = (pos.y*minimaps_size.h)/PAX_DESTINATIONS_SIZE;

		for(  sint32 y=0;  y<dd_y  &&  y+y0<minimaps_size.h;  y++  ) {
			for(  sint32 x=0;  x<dd_x  &&  x+x0<minimaps_size.w;  x++  ) {
				pax_dest.at( x+x0, y+y0 ) = color;
			}
		}
	}
}


void city_info_t::draw(scr_coord pos, scr_size size)
{
	stadt_t* const c = city;

	// Hajo: update chart seed
	chart.set_seed(welt->get_last_year());

	gui_frame_t::draw(pos, size);
	set_dirty();

	// display city stats
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

	display_multiline_text_rgb(pos.x + D_MARGIN_LEFT,
		pos.y+D_TITLEBAR_HEIGHT + D_MARGIN_TOP+D_EDIT_HEIGHT+D_V_SPACE, buf, SYSCOL_TEXT );

	const uint32 current_pax_destinations = c->get_pax_destinations_new_change();
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
	pax_destinations_last_change = current_pax_destinations;

	display_array_wh(pos.x + PAX_DEST_X, pos.y + PAX_DEST_Y, minimaps_size.w, minimaps_size.h, pax_dest_old.to_array() );
	display_array_wh(pos.x + PAX_DEST_X + minimap2_offset.x, pos.y + PAX_DEST_Y + minimap2_offset.y, minimaps_size.w, minimaps_size.h, pax_dest_new.to_array() );
}


bool city_info_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	static char param[16];
	if(  komp==&allow_growth  ) {
		sprintf(param,"g%hi,%hi,%hi", city->get_pos().x, city->get_pos().y, (short)(!city->get_citygrowth()) );
		tool_t::simple_tool[TOOL_CHANGE_CITY]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_CITY], welt->get_public_player());
		return true;
	}
	if(  komp==&name_input  ) {
		// send rename command if necessary
		rename_city();
	}
	else {
		for(  int i=0;  i<MAX_CITY_HISTORY;  i++  ) {
			if(  komp == &filterButtons[i]  ) {
				filterButtons[i].pressed ^= 1;
				if(  filterButtons[i].pressed  ) {
					city->stadtinfo_options |= (1<<i);
					chart.show_curve(i);
					mchart.show_curve(i);
				}
				else {
					city->stadtinfo_options &= ~(1<<i);
					chart.hide_curve(i);
					mchart.hide_curve(i);
				}
				return true;
			}
		}
	}
	return false;
}


void city_info_t::map_rotate90( sint16 )
{
	resize(scr_coord(0,0));
	pax_destinations_last_change = city->get_pax_destinations_new_change();
}


// current task: just update the city pointer ...
bool city_info_t::infowin_event(const event_t *ev)
{
	if(  IS_WINDOW_TOP(ev)  ) {
		reliefkarte_t::get_karte()->set_city( city );
	}

	uint16 my = ev->my;
	if(  my > PAX_DEST_Y + minimaps_size.h  &&  minimap2_offset.y > 0  ) {
		// Little trick to handle both maps with the same code: Just remap the y-values of the bottom map.
		my -= minimaps_size.h + D_V_SPACE;
	}

	if(  ev->ev_class!=EVENT_KEYBOARD  &&  ev->ev_code==MOUSE_LEFTBUTTON  &&  PAX_DEST_Y<=my  &&  my<PAX_DEST_Y+minimaps_size.h  ) {
		uint16 mx = ev->mx;
		if(  mx > PAX_DEST_X + minimaps_size.w  &&  minimap2_offset.x > 0  ) {
			// Little trick to handle both maps with the same code: Just remap the x-values of the right map.
			mx -= minimaps_size.w + D_H_SPACE;
		}

		if(  PAX_DEST_X <= mx && mx < PAX_DEST_X + minimaps_size.w  ) {
			// Clicked in a minimap.
			mx -= PAX_DEST_X;
			my -= PAX_DEST_Y;
			const koord p = koord(
				(mx * welt->get_size().x) / (minimaps_size.w),
				(my * welt->get_size().y) / (minimaps_size.h));
			welt->get_viewport()->change_world_position( p );
		}
	}

	return gui_frame_t::infowin_event(ev);
}


void city_info_t::update_data()
{
	allow_growth.pressed = city->get_citygrowth();
	if(  strcmp(old_name, city->get_name())!=0  ) {
		reset_city_name();
	}
	set_dirty();
}


/********** dialog restoring after saving stuff **********/


city_info_t::city_info_t() :
	gui_frame_t( name, NULL ),
	city(0),
	pax_dest_old(0,0),
	pax_dest_new(0,0)
{
	name_input.set_pos(scr_coord(D_MARGIN_LEFT, D_MARGIN_TOP));
	name_input.add_listener( this );
	add_component(&name_input);

	allow_growth.init( button_t::square_state, "Allow city growth",
		scr_coord(D_MARGIN_LEFT, D_MARGIN_TOP + D_BUTTON_HEIGHT+D_V_SPACE + 8*LINESPACE+D_V_SPACE) );
	allow_growth.add_listener( this );
	add_component(&allow_growth);

	//CHART YEAR
	chart.set_pos(scr_coord(21,1));
	chart.set_size(scr_size(340,120));
	chart.set_dimension(MAX_CITY_HISTORY_YEARS, 10000);
	chart.set_seed(welt->get_last_year());
	chart.set_background(SYSCOL_CHART_BACKGROUND);

	//CHART MONTH
	mchart.set_pos(scr_coord(21,1));
	mchart.set_size(scr_size(340,120));
	mchart.set_dimension(MAX_CITY_HISTORY_MONTHS, 10000);
	mchart.set_seed(0);
	mchart.set_background(SYSCOL_CHART_BACKGROUND);

	// tab (month/year)
	year_month_tabs.add_tab(&chart, translator::translate("Years"));
	year_month_tabs.add_tab(&mchart, translator::translate("Months"));
	add_component(&year_month_tabs);

	// add filter buttons; skip electricity
	for(  int hist=0;  hist<MAX_CITY_HISTORY-1;  hist++  ) {
		filterButtons[hist].init(button_t::box_state, hist_type[hist], scr_coord(0,0), scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
		filterButtons[hist].background_color = color_idx_to_rgb(hist_type_color[hist]);
		filterButtons[hist].add_listener(this);
		add_component(filterButtons + hist);
	}

	// Reset window to default size
	const int window_height = D_TITLEBAR_HEIGHT + D_MARGIN_TOP + D_BUTTON_HEIGHT + D_V_SPACE +
		8*LINESPACE + D_V_SPACE + D_CHECKBOX_HEIGHT + D_V_SPACE + MIN_CHART_HEIGHT + 2*(D_BUTTON_HEIGHT + D_V_SPACE);

	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, window_height));
	set_resizemode(diagonal_resize);
}


void city_info_t::rdwr(loadsave_t *file)
{
	scr_size size = get_windowsize();
	sint16 tabstate;
	uint32 townindex;
	uint32 flags = 0;
	if(  file->is_saving()  ) {
		for(  int i = 0;  i<MAX_CITY_HISTORY;  i++  ) {
			if(  filterButtons[i].pressed  ) {
				flags |= (1<<i);
			}
		}
		tabstate = year_month_tabs.get_active_tab_index();
		townindex = welt->get_cities().index_of(city);
	}
	size.rdwr( file );
	file->rdwr_long( townindex );
	file->rdwr_long( flags );
	file->rdwr_short( tabstate );
	if(  file->is_loading()  ) {
		city = welt->get_cities()[townindex];
		city->stadtinfo_options = flags;
		for(  int i = 0;  i<MAX_CITY_HISTORY-1;  i++  ) {
			chart.add_curve( color_idx_to_rgb(hist_type_color[i]), city->get_city_history_year(), MAX_CITY_HISTORY, i, 12, STANDARD, (city->stadtinfo_options & (1<<i))!=0, true, 0 );
			mchart.add_curve( color_idx_to_rgb(hist_type_color[i]), city->get_city_history_month(), MAX_CITY_HISTORY, i, 12, STANDARD, (city->stadtinfo_options & (1<<i))!=0, true, 0 );
			if(  city->stadtinfo_options & (1<<i)  ) {
				filterButtons[i].pressed = 1;
				chart.show_curve(i);
				mchart.show_curve(i);
			}
			else {
				city->stadtinfo_options &= ~(1<<i);
				filterButtons[i].pressed = 0;
				chart.hide_curve(i);
				mchart.hide_curve(i);
			}
		}

		year_month_tabs.set_active_tab_index( tabstate );
		allow_growth.pressed = city->get_citygrowth();
		win_set_magic( this, (ptrdiff_t)city );
		set_windowsize( size );
		reset_city_name();
		pax_destinations_last_change = city->get_pax_destinations_new_change();
		resize( scr_coord(0,0) );
	}
}
