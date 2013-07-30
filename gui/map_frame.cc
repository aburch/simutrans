/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#include <string>
#include <stdio.h>
#include <cmath>

#include "karte.h"
#include "map_frame.h"


#include "../simworld.h"
#include "../simwin.h"
#include "../simgraph.h"
#include "../simcolor.h"
#include "../bauer/fabrikbauer.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../dataobj/koord.h"
#include "../dataobj/loadsave.h"
#include "../besch/fabrik_besch.h"
#include "../simfab.h"
#include "../simtools.h"

static koord old_ij=koord::invalid;

karte_t *map_frame_t::welt;

koord map_frame_t::window_size;
bool  map_frame_t::legend_visible=false;
bool  map_frame_t::scale_visible=false;
bool  map_frame_t::directory_visible=false;
bool  map_frame_t::is_cursor_hidden=false;
bool  map_frame_t::filter_factory_list=true;

// Caches list of factories in current game world
stringhashtable_tpl<const fabrik_besch_t *> map_frame_t::factory_list;

// Hajo: we track our position onscreen
koord map_frame_t::screenpos;


class legend_entry_t
{
public:
	legend_entry_t() {}
	legend_entry_t(const std::string &text_, int colour_) : text(text_), colour(colour_) {}
	bool operator==(const legend_entry_t& rhs) 	{
		return ( (text == rhs.text)  &&  (colour == rhs.colour) );
	}

	std::string text;
	int         colour;
};

static vector_tpl<legend_entry_t> legend(16);


typedef struct {
	COLOR_VAL color;
	COLOR_VAL select_color;
	const char *button_text;
	const char *tooltip_text;
	reliefkarte_t::MAP_MODES mode;
} map_button_t;


map_button_t button_init[MAP_MAX_BUTTONS] = {
	{ COL_LIGHT_GREEN,  COL_DARK_GREEN,  "Towns", "Overlay town names", reliefkarte_t::MAP_TOWN },
	{ COL_LIGHT_GREEN,  COL_DARK_GREEN,  "CityLimit", "Overlay city limits", reliefkarte_t::MAP_CITYLIMIT },
	{ COL_WHITE,        COL_BLACK,       "Buildings", "Show level of city buildings", reliefkarte_t::MAP_LEVEL },
	{ COL_LIGHT_GREEN,  COL_DARK_GREEN,  "PaxDest", "Overlay passenger destinations when a town window is open", reliefkarte_t::MAP_PAX_DEST },
	{ COL_LIGHT_GREEN,  COL_DARK_GREEN,  "Tourists", "Highlite tourist attraction", reliefkarte_t::MAP_TOURIST },
	{ COL_LIGHT_GREEN,  COL_DARK_GREEN,  "Factories", "Highlite factories", reliefkarte_t::MAP_FACTORIES },
	{ COL_LIGHT_YELLOW, COL_BLACK,       "Passagiere", "Show passenger coverage/passenger network", reliefkarte_t::MAP_PASSENGER },
	{ COL_LIGHT_YELLOW, COL_BLACK,       "Post", "Show mail service coverage/mail network", reliefkarte_t::MAP_MAIL },
	{ COL_LIGHT_YELLOW, COL_BLACK,       "Fracht", "Show transported freight/freight network", reliefkarte_t::MAP_FREIGHT },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE, "Status", "Show capacity and if halt is overcrowded", reliefkarte_t::MAP_STATUS },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE, "hl_btn_sort_waiting", "Show how many people/much is waiting at halts", reliefkarte_t::MAP_WAITING },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE, "Queueing", "Show the change of waiting at halts", reliefkarte_t::MAP_WAITCHANGE },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE, "Service", "Show how many convoi reach a station", reliefkarte_t::MAP_SERVICE },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE, "Transfers", "Sum of departure/arrivals at halts", reliefkarte_t::MAP_TRANSFER },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE, "Origin", "Show initial passenger departure", reliefkarte_t::MAP_ORIGIN },
	{ COL_WHITE,        COL_BLACK,       "Traffic", "Show usage of network", reliefkarte_t::MAP_TRAFFIC },
	{ COL_WHITE,        COL_BLACK,       "Speedlimit", "Show speedlimit of ways", reliefkarte_t::MAX_SPEEDLIMIT },
	{ COL_WHITE,        COL_BLACK,       "Tracks", "Highlight railroad tracks", reliefkarte_t::MAP_TRACKS },
	{ COL_LIGHT_GREEN,  COL_DARK_GREEN,  "Depots", "Highlite depots", reliefkarte_t::MAP_DEPOT },
	{ COL_WHITE,        COL_BLACK,       "Powerlines", "Highlite electrical transmission lines", reliefkarte_t::MAP_POWERLINES },
	{ COL_WHITE,        COL_BLACK,       "Forest", "Highlite forests", reliefkarte_t::MAP_FOREST },
	{ COL_WHITE,        COL_BLACK,       "Ownership", "Show the owenership of infrastructure", reliefkarte_t::MAP_OWNER }
};


map_frame_t::map_frame_t(karte_t *world) :
	gui_frame_t( translator::translate("Reliefkarte") ),
	scrolly(reliefkarte_t::get_karte()),
	zoom_label("map zoom"),
	min_label("min"),
	max_label("max")
{
	koord cursor( D_MARGIN_LEFT,D_MARGIN_TOP );
	scr_coord_val zoom_label_width = display_get_char_max_width("0123456789") * 4 + display_get_char_width(':');

	welt = world;
	old_ij = koord::invalid;
	is_dragging = false;
	zoomed = false;

	// init map
	reliefkarte_t *karte = reliefkarte_t::get_karte();
	karte->set_welt( welt );

	const koord gr = karte->get_groesse();
	const koord s_gr=scrolly.get_groesse();
	const koord ij = welt->get_world_position();
	const koord win_size = gr-s_gr; // this is the visible area
	karte->set_mode( (reliefkarte_t::MAP_MODES)umgebung_t::default_mapmode );
	scrolly.set_scroll_position(  max(0,min(ij.x-win_size.x/2,gr.x)), max(0, min(ij.y-win_size.y/2,gr.y)) );

	// first row of coontrols
	// selections button
	b_show_legend.init(button_t::roundbox_state, "Show legend", cursor);
	b_show_legend.set_tooltip("Shows buttons on special topics.");
	b_show_legend.add_listener(this);
	add_komponente(&b_show_legend);
	cursor.x += D_BUTTON_WIDTH + D_H_SPACE;

	// industry list button
	b_show_directory.init(button_t::roundbox_state, "Show industry", cursor);
	b_show_directory.set_tooltip("Shows a listing with all industries on the map.");
	b_show_directory.add_listener(this);
	add_komponente(&b_show_directory);
	cursor.x += D_BUTTON_WIDTH + D_H_SPACE;

	// scale button
	b_show_scale.init(button_t::roundbox_state, "Show map scale", cursor);
	b_show_scale.set_tooltip("Shows the color code for several selections.");
	b_show_scale.add_listener(this);
	add_komponente(&b_show_scale);
	cursor = koord(D_MARGIN_LEFT, cursor.y + D_BUTTON_HEIGHT + D_V_SPACE);

	// second row of controls
	// zoom levels label
	zoom_label.set_pos(cursor);
	zoom_label.set_color(SYSCOL_TEXT);
	add_komponente( &zoom_label );
	cursor.x += zoom_label.get_groesse().x + D_H_SPACE;

	// zoom levels arrow left
	zoom_buttons[0].init(button_t::repeatarrowleft, NULL,cursor);
	zoom_buttons[0].add_listener( this );
	add_komponente( zoom_buttons+0 );
	cursor.x += zoom_buttons[0].get_groesse().x;

	// zoom level value label
	zoom_value_label.set_pos(cursor);
	zoom_value_label.set_groesse( koord(zoom_label_width,LINESPACE) );
	zoom_value_label.set_align(gui_label_t::centered);
	zoom_value_label.set_color(SYSCOL_TEXT);
	add_komponente( &zoom_value_label );
	cursor.x += zoom_label_width;

	// zoom levels arrow right
	zoom_buttons[1].init(button_t::repeatarrowright, NULL, cursor);
	zoom_buttons[1].add_listener( this );
	add_komponente( zoom_buttons+1 );
	cursor.x += zoom_buttons[1].get_groesse().x + D_H_SPACE;

	// rotate map 45°
	b_rotate45.init( button_t::square_state, "isometric map", cursor);
	b_rotate45.set_tooltip("Similar view as the main window");
	b_rotate45.add_listener(this);
	b_rotate45.pressed = karte->isometric;
	add_komponente(&b_rotate45);

	// align second row
	// Max Kielland: This will be done automatically (and properly) by the new gui_layout_t control in the near future.
	zoom_value_label.align_to(&zoom_buttons[0],ALIGN_CENTER_V);
	zoom_buttons[1].align_to(&zoom_buttons[0],ALIGN_CENTER_V);
	zoom_label.align_to(&zoom_buttons[0],ALIGN_CENTER_V);
	b_rotate45.align_to(&zoom_buttons[0],ALIGN_CENTER_V);
	cursor = koord(D_MARGIN_LEFT,zoom_buttons[0].get_pos().y+zoom_buttons[0].get_groesse().y+D_V_SPACE);

	// legend container
	filter_container.set_pos(cursor);
	filter_container.set_visible(false);
	add_komponente(&filter_container);

	// insert selections: show networks, in legend container
	b_overlay_networks.init(button_t::square_state, "Networks");
	b_overlay_networks.set_tooltip("Overlay schedules/network");
	b_overlay_networks.add_listener(this);
	b_overlay_networks.pressed = (umgebung_t::default_mapmode & reliefkarte_t::MAP_LINES)!=0;
	filter_container.add_komponente( &b_overlay_networks );

	// insert filter buttons in legend container
	for (int type=0; type<MAP_MAX_BUTTONS; type++) {
		filter_buttons[type].init( button_t::box_state, button_init[type].button_text);
		filter_buttons[type].set_tooltip( button_init[type].tooltip_text );
		filter_buttons[type].pressed = button_init[type].mode&umgebung_t::default_mapmode;
		filter_buttons[type].background = filter_buttons[type].pressed ? button_init[type].select_color : button_init[type].color;
		filter_buttons[type].foreground = filter_buttons[type].pressed ? COL_WHITE : COL_BLACK;
		filter_buttons[type].add_listener(this);
		filter_container.add_komponente(filter_buttons + type);
	}

	// directory container
	directory_container.set_pos(cursor);
	directory_container.set_visible(false);
	add_komponente(&directory_container);

	// factory list: show used button
	b_filter_factory_list.init(button_t::square_state, "Show only used");
	b_filter_factory_list.set_tooltip("In the industry legend show only currently existing factories");
	b_filter_factory_list.add_listener(this);
	directory_container.add_komponente( &b_filter_factory_list );
	update_factory_legend();

	// scale container
	scale_container.set_pos(cursor);
	scale_container.set_visible(false);
	add_komponente(&scale_container);
	scale_container.add_komponente(&min_label);
	scale_container.add_komponente(&max_label);

	// map scrolly
	scrolly.set_show_scroll_x(true);
	scrolly.set_scroll_discrete_y(false);
	add_komponente(&scrolly);

	// resore window size and options
	set_fenstergroesse( window_size );
	show_hide_legend( legend_visible );
	show_hide_scale( scale_visible );
	show_hide_directory( directory_visible );
	set_resizemode(diagonal_resize);
}


void map_frame_t::update_factory_legend()
{
	// When dialog is opened, update the list of industries currently in the world
	legend.clear();
	if(  directory_visible  ) {
		if(  filter_factory_list  ) {
			FOR(slist_tpl<fabrik_t*>, const f, welt->get_fab_list()) {
				if(  f->get_besch()->get_gewichtung() > 0  ) {
					std::string const label( translator::translate(f->get_besch()->get_name()) );
					legend.append_unique( legend_entry_t(label, f->get_besch()->get_kennfarbe()) );
				}
			}
		}
		else {
			FOR(stringhashtable_tpl<fabrik_besch_t const*>, const& i, fabrikbauer_t::get_fabesch()) {
				fabrik_besch_t const& d = *i.value;
				if (d.get_gewichtung() > 0) {
					std::string const label(translator::translate(d.get_name()));
					legend.append_unique(legend_entry_t(label, d.get_kennfarbe()));
				}
			}
		}

		// If any names are too long for current windows size, shorten them. Can't be done in above loop in case shortening some names cause
		// then to become non unique and thus get clumped together
		scr_coord_val client_width = get_fenstergroesse().x - D_MARGIN_LEFT - D_MARGIN_RIGHT;
		const int dot_len = proportional_string_width("..");
		const int fac_cols = clamp(legend.get_count(), 1, client_width / ((D_DEFAULT_WIDTH-D_MARGINS_X)/3));

		FOR(vector_tpl<legend_entry_t>, & l, legend) {
			std::string label = l.text;
			size_t i;
			for(  i=12;  i < label.size()  &&  display_calc_proportional_string_len_width(label.c_str(), i) < (client_width / fac_cols) - dot_len - D_INDICATOR_BOX_WIDTH - 2 - D_H_SPACE;  i++  ) {}
			if(  i < label.size()  ) {
				label = label.substr(0, i);
				label.append("..");
				l.text = label;
			}
		}
	}
}


void map_frame_t::show_hide_legend(const bool show)
{
	filter_container.set_visible(show);
	b_show_legend.pressed = show;
	legend_visible = show;
	resize();
}


void map_frame_t::show_hide_scale(const bool show)
{
	scale_container.set_visible(show);
	b_show_scale.pressed = show;
	scale_visible = show;
	resize();
}


void map_frame_t::show_hide_directory(const bool show)
{
	directory_container.set_visible(show);
	b_show_directory.pressed = show;
	b_filter_factory_list.pressed = filter_factory_list;
	directory_visible = show;
	resize();
}


bool map_frame_t::action_triggered( gui_action_creator_t *komp, value_t)
{
	if(komp==&b_show_legend) {
		show_hide_legend( !b_show_legend.pressed );
	}
	else if(komp==&b_show_scale) {
		show_hide_scale( !b_show_scale.pressed );
	}
	else if(komp==&b_show_directory) {
		show_hide_directory( !b_show_directory.pressed );
	}
	else if (komp==&b_filter_factory_list) {
		filter_factory_list = !filter_factory_list;
		show_hide_directory( b_show_directory.pressed );
	}
	else if(komp==zoom_buttons+1) {
		// zoom out
		zoom(true);
	}
	else if(komp==zoom_buttons+0) {
		// zoom in
		zoom(false);
	}
	else if(komp==&b_rotate45) {
		// rotated/straight map
		reliefkarte_t::get_karte()->isometric ^= 1;
		b_rotate45.pressed = reliefkarte_t::get_karte()->isometric;
		reliefkarte_t::get_karte()->calc_map_groesse();
		scrolly.set_groesse( scrolly.get_groesse() );
	}
	else if(komp==&b_overlay_networks) {
		b_overlay_networks.pressed ^= 1;
		if(  b_overlay_networks.pressed  ) {
			umgebung_t::default_mapmode |= reliefkarte_t::MAP_LINES;
		}
		else {
			umgebung_t::default_mapmode &= ~reliefkarte_t::MAP_LINES;
		}
		reliefkarte_t::get_karte()->set_mode(  (reliefkarte_t::MAP_MODES)umgebung_t::default_mapmode  );
	}
	else {
		for(  int i=0;  i<MAP_MAX_BUTTONS;  i++  ) {
			if(  komp == filter_buttons+i  ) {
				if(  filter_buttons[i].pressed  ) {
					umgebung_t::default_mapmode &= ~button_init[i].mode;
				}
				else {
					if(  (button_init[i].mode & reliefkarte_t::MAP_MODE_FLAGS) == 0  ) {
						// clear all persistent states
						umgebung_t::default_mapmode &= reliefkarte_t::MAP_MODE_FLAGS;
					}
					else if(  button_init[i].mode & reliefkarte_t::MAP_MODE_HALT_FLAGS  ) {
						// clear all other halt states
						umgebung_t::default_mapmode &= ~reliefkarte_t::MAP_MODE_HALT_FLAGS;
					}
					umgebung_t::default_mapmode |= button_init[i].mode;
				}
				filter_buttons[i].pressed ^= 1;
				break;
			}
		}
		reliefkarte_t::get_karte()->set_mode(  (reliefkarte_t::MAP_MODES)umgebung_t::default_mapmode  );
		for(  int i=0;  i<MAP_MAX_BUTTONS;  i++  ) {
			filter_buttons[i].pressed = (button_init[i].mode&umgebung_t::default_mapmode)!=0;
			filter_buttons[i].background = filter_buttons[i].pressed ? button_init[i].select_color : button_init[i].color;
			filter_buttons[i].foreground = filter_buttons[i].pressed ? COL_WHITE : COL_BLACK;
		}
	}
	return true;
}


void map_frame_t::zoom(bool magnify)
{
	if (reliefkarte_t::get_karte()->change_zoom_factor(magnify)) {
		// recalc all the other data incl scrollbars
		resize();
	}
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool map_frame_t::infowin_event(const event_t *ev)
{
	event_t ev2 = *ev;
	translate_event(&ev2, -scrolly.get_pos().x, -scrolly.get_pos().y-D_TITLEBAR_HEIGHT);

	if(ev->ev_class == INFOWIN) {
		if(ev->ev_code == WIN_OPEN) {
			reliefkarte_t::get_karte()->set_xy_offset_size( koord(0,0), koord(0,0) );
		}
		else if(ev->ev_code == WIN_CLOSE) {
			reliefkarte_t::get_karte()->is_visible = false;
		}
	}

	if(  (IS_WHEELUP(ev) || IS_WHEELDOWN(ev))  ) {
		// otherwise these would go to the vertical scroll bar
		zoom(IS_WHEELUP(ev));
		return true;
	}

	// Hajo: hack: relief map can resize upon right click
	// we track this here, and adjust size.
	if(  IS_RIGHTCLICK(ev)  ) {
		is_dragging = false;
		display_show_pointer(false);
		is_cursor_hidden = true;
		reliefkarte_t::get_karte()->get_welt()->set_scroll_lock(false);
		return true;
	}
	else if(  IS_RIGHTRELEASE(ev)  ) {
		if(!is_dragging) {
			resize();
		}
		is_dragging = false;
		display_show_pointer(true);
		is_cursor_hidden = false;
		reliefkarte_t::get_karte()->get_welt()->set_scroll_lock(false);
		return true;
	}
	else if(  IS_RIGHTDRAG(ev)  &&  reliefkarte_t::get_karte()->getroffen(ev2.mx,ev2.my)  &&  reliefkarte_t::get_karte()->getroffen(ev2.cx,ev2.cy)  ) {
		int x = scrolly.get_scroll_x();
		int y = scrolly.get_scroll_y();
		const int scroll_direction = ( umgebung_t::scroll_multi>0 ? 1 : -1 );

		x += (ev->mx - ev->cx)*scroll_direction*2;
		y += (ev->my - ev->cy)*scroll_direction*2;

		is_dragging = true;
		reliefkarte_t::get_karte()->get_welt()->set_scroll_lock(true);

		scrolly.set_scroll_position(  max(0, x),  max(0, y) );

		// Hajo: re-center mouse pointer
		display_move_pointer(screenpos.x+ev->cx, screenpos.y+ev->cy);
		return true;
	}
	else if(  IS_LEFTDBLCLK(ev)  &&  reliefkarte_t::get_karte()->getroffen(ev2.mx,ev2.my)  ) {
		// recenter cursor by scrolling
		koord ij = reliefkarte_t::get_karte()->get_welt()->get_world_position();
		reliefkarte_t::get_karte()->karte_to_screen(ij);
		const koord s_gr = scrolly.get_groesse();

		scrolly.set_scroll_position(max(0,ij.x-(s_gr.x/2)), max(0,ij.y-(s_gr.y/2)));
		zoomed = false;

		// remember world position, we do not want to have surprises when scrolling later on
		old_ij = ij;
		return true;
	}
	else if(  IS_RIGHTDBLCLK(ev)  ) {
		// zoom to fit window
		do { // first, zoom all the way in
			zoom(true);
		} while(  zoomed  );

		// then zoom back out to fit
		const koord s_gr = scrolly.get_groesse() - button_t::gui_scrollbar_size;
		koord gr = reliefkarte_t::get_karte()->get_groesse();
		zoomed = true;
		while(  zoomed  &&  max(gr.x/s_gr.x, gr.y/s_gr.y)  ) {
			zoom(false);
			gr = reliefkarte_t::get_karte()->get_groesse();
		}
		return true;
	}
	else if(  is_cursor_hidden  ) {
		display_show_pointer(true);
		is_cursor_hidden = false;
	}

	return gui_frame_t::infowin_event(ev);
}


/**
 * size window in response and save it in static size
 * @author (Mathew Hounsell)
 * @date   11-Mar-2003
 */
void map_frame_t::set_fenstergroesse(koord groesse)
{
	gui_frame_t::set_fenstergroesse( groesse );
	window_size = get_fenstergroesse();
	scrolly.set_groesse(get_client_windowsize()-scrolly.get_pos()-koord(1,1));
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   01-Jun-2002
 */

void map_frame_t::resize(const koord delta)
{
	scr_coord_val offset_y = filter_container.get_pos().y;
	scr_coord_val client_width = get_fenstergroesse().x-D_MARGIN_LEFT - D_MARGIN_RIGHT;

	// resize legend
	if(legend_visible) {
		scr_coord_val button_y = b_overlay_networks.get_groesse().y+D_V_SPACE;
		koord pos;

		// calculate number of columns and rows for buttons
		const int col = max( 1, min( client_width/(D_BUTTON_WIDTH+D_H_SPACE), MAP_MAX_BUTTONS ) );
		const int row = ((MAP_MAX_BUTTONS-1)/col)+1;

		// set button pos
		for (int type=0; type<MAP_MAX_BUTTONS; type++) {
			pos = koord( (D_BUTTON_WIDTH+D_H_SPACE)*(type%col), button_y+(D_BUTTON_HEIGHT+D_V_SPACE)*((int)type/col) );
			filter_buttons[type].set_pos( pos );
		}

		// calculate client height and set height
		filter_container.set_groesse(koord(client_width, (D_BUTTON_HEIGHT+D_V_SPACE)*(row+1) ));
		offset_y += filter_container.get_groesse().y + D_V_SPACE;
	}

	if(directory_visible) {
		scr_coord_val button_y = b_filter_factory_list.get_groesse().y+D_V_SPACE;
		scr_coord_val line_height = max(D_INDICATOR_BOX_HEIGHT,LINESPACE);

		directory_container.set_pos(koord(D_MARGIN_LEFT,offset_y));


		// full program including factory texts
		update_factory_legend();

		const int fac_cols = clamp( legend.get_count(), 1, client_width / ((D_DEFAULT_WIDTH-D_MARGINS_X)/3));
		const int fac_rows = (legend.get_count() - 1) / fac_cols + 1;

		// calculate client height and set height
		directory_container.set_groesse(koord(client_width,button_y+fac_rows*line_height));
		offset_y += directory_container.get_groesse().y + D_V_SPACE;
	}

	// resize scale
	if(scale_visible) {
		scale_container.set_pos(koord(D_MARGIN_LEFT,offset_y));
		scale_container.set_groesse(koord(client_width,LINESPACE));
		max_label.align_to(&scale_container,ALIGN_RIGHT,koord(scale_container.get_pos().x,0));
		offset_y += scale_container.get_groesse().y + D_V_SPACE;
	}

	// offset of map
	scrolly.set_pos( koord(1,offset_y) );

	scr_coord_val min_width = max(BUTTON4_X+D_MARGIN_RIGHT-D_H_SPACE,D_DEFAULT_WIDTH);
	set_min_windowsize(koord(min_width, D_TITLEBAR_HEIGHT+offset_y+64+button_t::gui_scrollbar_size.y+1));
	set_dirty();

	gui_frame_t::resize(delta);
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 * @author Hj. Malthaner
 */
void map_frame_t::zeichnen(koord pos, koord gr)
{
	char buf[16];
	sint16 zoom_in, zoom_out;
	//scr_rect client(scr_coord(pos.x+D_MARGIN_LEFT,pos.y+D_TITLEBAR_HEIGHT),gr.x-D_MARGIN_LEFT - D_MARGIN_RIGHT,gr.y-D_TITLEBAR_HEIGHT);

	// update our stored screen position
	screenpos = pos;

	reliefkarte_t::get_karte()->set_xy_offset_size( koord(scrolly.get_scroll_x(), scrolly.get_scroll_y()), koord(scrolly.get_groesse()-button_t::gui_scrollbar_size) );

	// first: check if cursor within map screen size
	karte_t *welt=reliefkarte_t::get_karte()->get_welt();
	koord ij = welt->get_world_position();
	if(welt->is_within_limits(ij)) {
		reliefkarte_t::get_karte()->karte_to_screen(ij);
		// only recenter if zoomed or world position has changed and its outside visible area
		const koord groesse = scrolly.get_groesse();
		if(zoomed  ||  ( old_ij != ij  &&
				( scrolly.get_scroll_x()>ij.x  ||  scrolly.get_scroll_x()+groesse.x<=ij.x  ||
				  scrolly.get_scroll_y()>ij.y  ||  scrolly.get_scroll_y()+groesse.y<=ij.y ) ) ) {
				// recenter cursor by scrolling
				scrolly.set_scroll_position( max(0,ij.x-(groesse.x/2)), max(0,ij.y-(groesse.y/2)) );
				zoomed = false;
		}
		// remember world position, we do not want to have surprises when scrolling later on
		old_ij = ij;
	}

	// update zoom factors and zoom label
	reliefkarte_t::get_karte()->get_zoom_factors(zoom_out, zoom_in);
	sprintf( buf, "%i:%i", zoom_in, zoom_out );
	zoom_value_label.set_text_pointer(buf,false);

	// draw all child controls
	gui_frame_t::zeichnen(pos, gr);

	// draw scale
	if(scale_visible) {
		scr_coord bar_pos( pos.x + scale_container.get_pos().x, pos.y + scale_container.get_pos().y + D_TITLEBAR_HEIGHT );
		scr_coord_val bar_client_x = scale_container.get_groesse().x - min_label.get_groesse().x - max_label.get_groesse().x - (D_H_SPACE<<1);
		double bar_width = (double)bar_client_x/(double)MAX_SEVERITY_COLORS;
		// color bar
		for(  int i=0;  i<MAX_SEVERITY_COLORS;  i++  ) {
			display_fillbox_wh(bar_pos.x + min_label.get_groesse().x + D_H_SPACE + (i*bar_width), bar_pos.y+2,  bar_width+1, 7, reliefkarte_t::calc_severity_color(i,MAX_SEVERITY_COLORS), false);
		}
	}

	// draw factory descriptions
	if(directory_visible) {
		scr_coord_val offset_y = directory_container.get_pos().y + b_filter_factory_list.get_groesse().y + D_V_SPACE;
		const int columns = clamp(legend.get_count(), 1, (gr.x-D_MARGIN_RIGHT-D_MARGIN_LEFT) / ((D_DEFAULT_WIDTH-D_MARGINS_X)/3));
		scr_coord_val u = 0;
		scr_coord_val line_height = max(D_INDICATOR_BOX_HEIGHT,LINESPACE);

		FORX(vector_tpl<legend_entry_t>, const& i, legend, ++u) {
			const int xpos = pos.x+D_MARGIN_LEFT + (u % columns) * (gr.x-D_MARGIN_RIGHT-columns/2-1)/columns;
			const int ypos = pos.y+D_TITLEBAR_HEIGHT + offset_y + (u / columns) * line_height;
			display_fillbox_wh(xpos, ypos + D_GET_CENTER_ALIGN_OFFSET(D_INDICATOR_BOX_HEIGHT,LINESPACE), D_INDICATOR_BOX_WIDTH, D_INDICATOR_BOX_HEIGHT, i.colour, false);
			display_ddd_box(xpos, ypos + D_GET_CENTER_ALIGN_OFFSET(D_INDICATOR_BOX_HEIGHT,LINESPACE), D_INDICATOR_BOX_WIDTH, D_INDICATOR_BOX_HEIGHT,SYSCOL_SHADOW,SYSCOL_SHADOW,false);
			display_proportional(xpos + D_INDICATOR_BOX_WIDTH + 2, ypos, i.text.c_str(), ALIGN_LEFT, SYSCOL_STATIC_TEXT, false);
		}
	}
}


void map_frame_t::rdwr( loadsave_t *file )
{
	bool is_show_schedule = (umgebung_t::default_mapmode & reliefkarte_t::MAP_LINES);
	bool is_show_fab = (umgebung_t::default_mapmode & reliefkarte_t::MAP_FACTORIES);

	file->rdwr_bool( reliefkarte_t::get_karte()->isometric );
	if(  file->get_version()<111004  ) {
		file->rdwr_bool( is_show_schedule );
		file->rdwr_bool( is_show_fab );
	}
	reliefkarte_t::get_karte()->rdwr(file);
	bool show_legend_state = legend_visible;
	file->rdwr_bool( show_legend_state );
	file->rdwr_bool( scale_visible );
	file->rdwr_bool( directory_visible );
	if(  file->get_version()<111004  ) {
		sint8 mode = log2(umgebung_t::default_mapmode & ~reliefkarte_t::MAP_MODE_FLAGS);
		file->rdwr_byte( mode );
		umgebung_t::default_mapmode = 1 << mode;
		umgebung_t::default_mapmode = mode>=0 ? 1 << mode : 0;
		umgebung_t::default_mapmode |= is_show_schedule * reliefkarte_t::MAP_LINES;
		umgebung_t::default_mapmode |= is_show_fab * reliefkarte_t::MAP_FACTORIES;
	}
	else {
		file->rdwr_long( umgebung_t::default_mapmode );
	}

	if(  file->is_loading()  ) {
		koord savesize;
		savesize.rdwr(file);
		set_fenstergroesse( savesize );
		resize();
		// notify minimap of new settings
		reliefkarte_t::get_karte()->calc_map_groesse();
		scrolly.set_groesse( scrolly.get_groesse() );

		sint32 xoff;
		file->rdwr_long( xoff );
		sint32 yoff;
		file->rdwr_long( yoff );
		scrolly.set_scroll_position( xoff, yoff );

		reliefkarte_t::get_karte()->set_mode((reliefkarte_t::MAP_MODES)umgebung_t::default_mapmode);
		for (uint i=0;i<MAP_MAX_BUTTONS;i++) {
			filter_buttons[i].pressed = i==umgebung_t::default_mapmode;
		}
		if(  legend_visible!=show_legend_state  ) {
			action_triggered( &b_show_legend, (long)0 );
		}
		b_filter_factory_list.set_visible( directory_visible );

		b_overlay_networks.pressed = (umgebung_t::default_mapmode & reliefkarte_t::MAP_LINES)!=0;
	}
	else {
		koord gr = get_fenstergroesse();
		gr.rdwr(file);
		sint32 xoff = scrolly.get_scroll_x();
		file->rdwr_long( xoff );
		sint32 yoff = scrolly.get_scroll_y();
		file->rdwr_long( yoff );
	}
}
