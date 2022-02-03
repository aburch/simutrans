/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simdebug.h"
#include "../world/simcity.h"
#include "../tool/simmenu.h"
#include "../world/simworld.h"
#include "../simcolor.h"
#include "../dataobj/translator.h"
#include "../display/viewport.h"
#include "../utils/cbuffer.h"
#include "../utils/simstring.h"
#include "../tpl/array2d_tpl.h"

#include "city_info.h"
#include "minimap.h"
#include "components/gui_button_to_chart.h"

#include "../display/simgraph.h"

#define PAX_DEST_MIN_SIZE (16)      ///< minimum width/height of the minimap
#define PAX_DEST_VERTICAL (4.0/3.0) ///< aspect factor where minimaps change to over/under instead of left/right


/**
 * Component to show both passenger destination minimaps
 */
class gui_city_minimap_t : public gui_world_component_t
{
	stadt_t* city;
	scr_size minimaps_size;        ///< size of minimaps
	scr_coord minimap2_offset;     ///< position offset of second minimap
	array2d_tpl<PIXVAL> pax_dest_old, pax_dest_new;
	uint32 pax_destinations_last_change;

	void init_pax_dest( array2d_tpl<PIXVAL> &pax_dest );
	void add_pax_dest( array2d_tpl<PIXVAL> &pax_dest, const sparse_tpl<PIXVAL>* city_pax_dest );

public:
	gui_city_minimap_t(stadt_t* city) : city(city),
		pax_dest_old(0,0),
		pax_dest_new(0,0)
	{
		minimaps_size = scr_size(PAX_DESTINATIONS_SIZE, PAX_DESTINATIONS_SIZE); // default minimaps size
		minimap2_offset = scr_coord(minimaps_size.w + D_H_SPACE, 0);
	}
	scr_size get_min_size() const OVERRIDE { return scr_size(PAX_DEST_MIN_SIZE*2 + D_H_SPACE, PAX_DEST_MIN_SIZE); }

	// set size of minimap, decide for horizontal or vertical arrangement
	void set_size(scr_size size) OVERRIDE
	{
		gui_world_component_t::set_size(size);
		// calculate new minimaps size : expand horizontally or vertically ?
		const float world_aspect = (float)welt->get_size().x / (float)welt->get_size().y;
		const scr_coord space(size.w, size.h);
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
		pax_destinations_last_change = city->get_pax_destinations_new_change();
	}
	// handle clicks into minimaps
	bool infowin_event(const event_t *ev) OVERRIDE
	{
		int my = ev->my;
		if(  my > minimaps_size.h  &&  minimap2_offset.y > 0  ) {
			// Little trick to handle both maps with the same code: Just remap the y-values of the bottom map.
			my -= minimaps_size.h + D_V_SPACE;
		}

		if(  ev->ev_class!=EVENT_KEYBOARD  &&  ev->ev_code==MOUSE_LEFTBUTTON  &&  0<=my  &&  my<minimaps_size.h  ) {
			int mx = ev->mx;
			if(  mx > minimaps_size.w  &&  minimap2_offset.x > 0  ) {
				// Little trick to handle both maps with the same code: Just remap the x-values of the right map.
				mx -= minimaps_size.w + D_H_SPACE;
			}

			if(  0 <= mx && mx < minimaps_size.w  ) {
				// Clicked in a minimap.
				const koord p = koord(
					(mx * welt->get_size().x) / (minimaps_size.w),
									  (my * welt->get_size().y) / (minimaps_size.h));
				welt->get_viewport()->change_world_position( p );
				return true;
			}
		}
		return false;
	}
	// draw both minimaps
	void draw(scr_coord offset) OVERRIDE
	{
		const uint32 current_pax_destinations = city->get_pax_destinations_new_change();
		if(  pax_destinations_last_change > current_pax_destinations  ) {
			// new month started
			pax_dest_old = pax_dest_new;
			init_pax_dest( pax_dest_new );
			add_pax_dest( pax_dest_new, city->get_pax_destinations_new());
		}
		else if(  pax_destinations_last_change != current_pax_destinations  ) {
			// Since there are only new colors, this is enough:
			add_pax_dest( pax_dest_new, city->get_pax_destinations_new() );
		}
		pax_destinations_last_change = current_pax_destinations;

		display_array_wh(pos.x + offset.x, pos.y + offset.y, minimaps_size.w, minimaps_size.h, pax_dest_old.to_array() );
		display_array_wh(pos.x + offset.x + minimap2_offset.x, pos.y + offset.y + minimap2_offset.y, minimaps_size.w, minimaps_size.h, pax_dest_new.to_array() );
	}
};


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
	city(city)
{
	if (city) {
		init();
	}
}

void city_info_t::init()
{
	reset_city_name();

	set_table_layout(1,0);

	// add city name input field
	name_input.add_listener( this );
	add_component(&name_input);

	add_table(2,0)->set_alignment(ALIGN_TOP);
	{
		add_table(1,0);
		gui_label_buf_t *lb = new_component<gui_label_buf_t>();
		lb->buf().printf("%s:", translator::translate("City size") );
		lb->update();

		add_component(&lb_size);
		add_component(&lb_buildings);
		add_component(&lb_border);
		add_component(&lb_unemployed);
		add_component(&lb_homeless);

		// add "allow city growth" button below city info
		allow_growth.init( button_t::square_state, "Allow city growth");
		allow_growth.pressed = city->get_citygrowth();
		allow_growth.add_listener( this );
		add_component(&allow_growth);
		end_table();

		pax_map = new_component<gui_city_minimap_t>(city);
	}
	end_table();

	// tab (month/year)
	year_month_tabs.add_tab(&container_year, translator::translate("Years"));
	year_month_tabs.add_tab(&container_month, translator::translate("Months"));
	add_component(&year_month_tabs);
	// .. put the same buttons in both containers
	button_t* buttons[MAX_CITY_HISTORY-1];
	// add city charts
	// year chart
	container_year.set_table_layout(1,0);
	container_year.add_component(&chart);
	chart.set_min_size(scr_size(0 ,8*LINESPACE));
	chart.set_dimension(MAX_CITY_HISTORY_YEARS, 10000);
	chart.set_seed(welt->get_last_year());
	chart.set_background(SYSCOL_CHART_BACKGROUND);

	container_year.add_table(4,3)->set_force_equal_columns(true);
	//   skip electricity
	for(  uint32 i = 0;  i<MAX_CITY_HISTORY-1;  i++  ) {
		sint16 curve = chart.add_curve( color_idx_to_rgb(hist_type_color[i]), city->get_city_history_year(),
			MAX_CITY_HISTORY, i, 12, STANDARD, (city->stadtinfo_options & (1<<i))!=0, true, 0 );
		// add button
		buttons[i] = container_year.new_component<button_t>();
		buttons[i]->init(button_t::box_state_automatic | button_t::flexible, hist_type[i]);
		buttons[i]->background_color = color_idx_to_rgb(hist_type_color[i]);
		buttons[i]->pressed = (city->stadtinfo_options & (1<<i))!=0;

		button_to_chart.append(buttons[i], &chart, curve);
	}
	container_year.end_table();

	// month chart
	container_month.set_table_layout(1,0);
	container_month.add_component(&mchart);
	mchart.set_pos(scr_coord(D_MARGIN_LEFT,1));
	mchart.set_min_size(scr_size(0 ,8*LINESPACE));
	mchart.set_dimension(MAX_CITY_HISTORY_MONTHS, 10000);
	mchart.set_seed(0);
	mchart.set_background(SYSCOL_CHART_BACKGROUND);

	container_month.add_table(4,3)->set_force_equal_columns(true);
	for(  uint32 i = 0;  i<MAX_CITY_HISTORY-1;  i++  ) {
		sint16 curve = mchart.add_curve( color_idx_to_rgb(hist_type_color[i]), city->get_city_history_month(),
			MAX_CITY_HISTORY, i, 12, STANDARD, (city->stadtinfo_options & (1<<i))!=0, true, 0 );

		// add button
		container_month.add_component(buttons[i]);
		button_to_chart.append(buttons[i], &mchart, curve);
	}
	container_month.end_table();

	update_labels();
	set_resizemode(diagonal_resize);
	reset_min_windowsize();
}


city_info_t::~city_info_t()
{
	// send rename command if necessary
	rename_city();
	// save button state
	uint32 flags = 0;
	for(gui_button_to_chart_t* b2c : button_to_chart.list()) {
		if (b2c->get_button()->pressed) {
			flags |= 1 << b2c->get_curve();
		}
	}
	city->stadtinfo_options = flags;
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


void gui_city_minimap_t::init_pax_dest( array2d_tpl<PIXVAL> &pax_dest )
{
	const int size_x = welt->get_size().x;
	const int size_y = welt->get_size().y;
	for(  sint16 y = 0;  y < minimaps_size.h;  y++  ) {
		for(  sint16 x = 0;  x < minimaps_size.w;  x++  ) {
			const grund_t *gr = welt->lookup_kartenboden( koord( (x * size_x) / minimaps_size.w, (y * size_y) / minimaps_size.h ) );
			pax_dest.at(x,y) = minimap_t::calc_ground_color(gr);
		}
	}
}


void gui_city_minimap_t::add_pax_dest( array2d_tpl<PIXVAL> &pax_dest, const sparse_tpl<PIXVAL>* city_pax_dest )
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


void city_info_t::update_labels()
{
	stadt_t* const c = city;

	// display city stats
	lb_size.buf().printf( "%d (%.1f)", c->get_einwohner(), c->get_wachstum() / 10.0);         lb_size.update();
	lb_buildings.buf().printf( translator::translate("%d buildings\n"), c->get_buildings() ); lb_buildings.update();

	const koord ul = c->get_linksoben();
	const koord lr = c->get_rechtsunten();
	lb_border.buf().printf( "%d,%d - %d,%d", ul.x, ul.y, lr.x , lr.y); lb_border.update();

	lb_unemployed.buf().printf("%s: %d", translator::translate("Unemployed"), c->get_unemployed()); lb_unemployed.update();
	lb_homeless.buf().printf("%s: %d", translator::translate("Homeless"), c->get_homeless());       lb_homeless.update();
}


void city_info_t::draw(scr_coord pos, scr_size size)
{
	// update chart seed
	chart.set_seed(welt->get_last_year());
	update_labels();
	gui_frame_t::draw(pos, size);
}


bool city_info_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	static char param[16];
	if(  comp==&allow_growth  ) {
		sprintf(param,"g%hi,%hi,%hi", city->get_pos().x, city->get_pos().y, (short)(!city->get_citygrowth()) );
		tool_t::simple_tool[TOOL_CHANGE_CITY]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_CITY], welt->get_public_player());
		return true;
	}
	if(  comp==&name_input  ) {
		// send rename command if necessary
		rename_city();
	}
	return false;
}


void city_info_t::map_rotate90( sint16 )
{
	pax_map->set_size( pax_map->get_size() );
}


// current task: just update the city pointer ...
bool city_info_t::infowin_event(const event_t *ev)
{
	if(  IS_WINDOW_TOP(ev)  ) {
		minimap_t::get_instance()->set_selected_city( city );
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
void city_info_t::rdwr(loadsave_t *file)
{
	// window size
	scr_size size = get_windowsize();
	size.rdwr( file );

	uint32 townindex;
	if (file->is_saving()) {
		townindex = welt->get_cities().index_of(city);
	}
	file->rdwr_long( townindex );

	if(  file->is_loading()  ) {
		city = welt->get_cities()[townindex];

		init();
		win_set_magic( this, (ptrdiff_t)city );
		reset_min_windowsize();
		set_windowsize(size);
	}

	// button-to-chart array
	button_to_chart.rdwr(file);

	year_month_tabs.rdwr(file);

	if (city == NULL) {
		destroy_win(this);
	}
}
