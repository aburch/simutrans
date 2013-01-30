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

koord map_frame_t::size=koord(0,0);
bool map_frame_t::legend_visible=false;
bool map_frame_t::scale_visible=false;
bool map_frame_t::directory_visible=false;
bool map_frame_t::is_cursor_hidden=false;
bool map_frame_t::filter_factory_list=true;

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
		return text == rhs.text && colour == rhs.colour;
	}

	std::string text;
	int       colour;
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
	{ COL_LIGHT_YELLOW, COL_BLACK,        "Passagiere", "Show passenger coverage/passenger network", reliefkarte_t::MAP_PASSENGER },
	{ COL_LIGHT_YELLOW, COL_BLACK,        "Post", "Show mail service coverage/mail network", reliefkarte_t::MAP_MAIL },
	{ COL_LIGHT_YELLOW, COL_BLACK,        "Fracht", "Show transported freight/freight network", reliefkarte_t::MAP_FREIGHT },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE,  "Status", "Show capacity and if halt is overcrowded", reliefkarte_t::MAP_STATUS },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE,  "hl_btn_sort_waiting", "Show how many people/much is waiting at halts", reliefkarte_t::MAP_WAITING },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE,  "Queueing", "Show the change of waiting at halts", reliefkarte_t::MAP_WAITCHANGE },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE,  "Service", "Show how many convoi reach a station", reliefkarte_t::MAP_SERVICE },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE,  "Transfers", "Sum of departure/arrivals at halts", reliefkarte_t::MAP_TRANSFER },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE,  "Origin", "Show initial passenger departure", reliefkarte_t::MAP_ORIGIN },
	{ COL_WHITE,        COL_BLACK,        "Traffic", "Show usage of network", reliefkarte_t::MAP_TRAFFIC },
	{ COL_WHITE,        COL_BLACK,        "Speedlimit", "Show speedlimit of ways", reliefkarte_t::MAX_SPEEDLIMIT },
	{ COL_WHITE,        COL_BLACK,        "Tracks", "Highlight railroad tracks", reliefkarte_t::MAP_TRACKS },
	{ COL_LIGHT_GREEN,  COL_DARK_GREEN,   "Depots", "Highlite depots", reliefkarte_t::MAP_DEPOT },
	{ COL_WHITE,        COL_BLACK,        "Powerlines", "Highlite electrical transmission lines", reliefkarte_t::MAP_POWERLINES },
	{ COL_WHITE,        COL_BLACK,        "Forest", "Highlite forests", reliefkarte_t::MAP_FOREST },
	{ COL_WHITE,        COL_BLACK,        "Ownership", "Show the owenership of infrastructure", reliefkarte_t::MAP_OWNER }
};


map_frame_t::map_frame_t(karte_t *w) :
	gui_frame_t( translator::translate("Reliefkarte") ),
	scrolly(reliefkarte_t::get_karte()),
	zoom_label("map zoom")
{
	welt = w;

	// show the various objects
	b_show_legend.init(button_t::roundbox_state, "Show legend", koord(BUTTON1_X,0), koord(D_BUTTON_WIDTH-1,D_BUTTON_HEIGHT));
	b_show_legend.set_tooltip("Shows buttons on special topics.");
	b_show_legend.add_listener(this);
	add_komponente(&b_show_legend);

	b_show_scale.init(button_t::roundbox_state, "Show map scale", koord(BUTTON2_X-1,0), koord(D_BUTTON_WIDTH+2,D_BUTTON_HEIGHT));
	b_show_scale.set_tooltip("Shows the color code for several selections.");
	b_show_scale.add_listener(this);
	add_komponente(&b_show_scale);

	b_show_directory.init(button_t::roundbox_state, "Show industry", koord(BUTTON3_X+1,0), koord(D_BUTTON_WIDTH-1,D_BUTTON_HEIGHT));
	b_show_directory.set_tooltip("Shows a listing with all industries on the map.");
	b_show_directory.add_listener(this);
	add_komponente(&b_show_directory);

	// zoom levels
	zoom_buttons[0].init(button_t::repeatarrowleft, NULL, koord(BUTTON1_X,D_BUTTON_HEIGHT+D_V_SPACE));
	zoom_buttons[0].add_listener( this );
	add_komponente( zoom_buttons+0 );

	zoom_buttons[1].init(button_t::repeatarrowright, NULL, koord(BUTTON1_X+40,D_BUTTON_HEIGHT+D_V_SPACE));
	zoom_buttons[1].add_listener( this );
	add_komponente( zoom_buttons+1 );

	zoom_label.set_pos( koord(BUTTON1_X+54,D_BUTTON_HEIGHT+D_V_SPACE) );
	add_komponente( &zoom_label );

	// rotate map 45°
	b_rotate45.init( button_t::square_state, "isometric map", koord(BUTTON1_X+54+proportional_string_width(zoom_label.get_text_pointer())+D_H_SPACE+10,D_BUTTON_HEIGHT+D_V_SPACE));
	b_rotate45.set_tooltip("Similar view as the main window");
	b_rotate45.add_listener(this);
	add_komponente(&b_rotate45);

	b_overlay_networks.init(button_t::square_state, "Networks", koord(BUTTON1_X,D_BUTTON_HEIGHT*2+2*D_V_SPACE));
	b_overlay_networks.set_tooltip("Overlay schedules/network");
	b_overlay_networks.add_listener(this);
	b_overlay_networks.set_visible( legend_visible );
	b_overlay_networks.pressed = (umgebung_t::default_mapmode & reliefkarte_t::MAP_LINES)!=0;
	add_komponente( &b_overlay_networks );

	// filter factory list
	b_filter_factory_list.init(button_t::square_state, "Show only used", koord(BUTTON1_X,D_BUTTON_HEIGHT*3+4));
	b_filter_factory_list.set_tooltip("In the industry legend show only currently existing factories");
	b_filter_factory_list.add_listener(this);
	b_filter_factory_list.set_visible( directory_visible );
	add_komponente( &b_filter_factory_list );

	// init factory name legend
	update_factory_legend();

	// init the rest
	scrolly.set_pos( koord(0, D_BUTTON_HEIGHT*4 + 2) );
	scrolly.set_show_scroll_x(true);
	scrolly.set_scroll_discrete_y(false);
	add_komponente(&scrolly);

	// and now the buttons
	for (int type=0; type<MAP_MAX_BUTTONS; type++) {
		filter_buttons[type].init( button_t::box_state, button_init[type].button_text, koord(0,0), koord(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
		filter_buttons[type].set_tooltip( button_init[type].tooltip_text );
		filter_buttons[type].set_visible(false);
		filter_buttons[type].pressed = button_init[type].mode&umgebung_t::default_mapmode;
		filter_buttons[type].background = filter_buttons[type].pressed ? button_init[type].select_color : button_init[type].color;
		filter_buttons[type].foreground = filter_buttons[type].pressed ? COL_WHITE : COL_BLACK;
		filter_buttons[type].add_listener(this);
		add_komponente(filter_buttons + type);
	}

 	// Hajo: Hack: use static size if set by a former object
	const koord size2 = size;

	set_fenstergroesse(koord(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+256+6));
	set_min_windowsize(koord(BUTTON4_X, D_TITLEBAR_HEIGHT+2*D_BUTTON_HEIGHT+4+64));

	if(  size2 != koord(0,0)  ) {
		if(  legend_visible  ) {
			show_hide_legend( legend_visible );
		}
		if(  scale_visible  ) {
			show_hide_scale( scale_visible );
		}
		if(  directory_visible  ) {
			show_hide_directory( directory_visible );
		}

		resize( size2 - get_fenstergroesse() );
	}

	set_resizemode(diagonal_resize);
	resize(koord(0,0));

	reliefkarte_t *karte = reliefkarte_t::get_karte();
	karte->set_welt( welt );

	const koord gr = karte->get_groesse();
	const koord s_gr=scrolly.get_groesse();
	const koord ij = welt->get_world_position();
	const koord win_size = gr-s_gr;	// this is the visible area
	scrolly.set_scroll_position(  max(0,min(ij.x-win_size.x/2,gr.x)), max(0, min(ij.y-win_size.y/2,gr.y)) );

	old_ij = koord::invalid;

	// Hajo: Trigger layouting
	is_dragging = false;
	zoomed = false;

	karte->set_mode( (reliefkarte_t::MAP_MODES)umgebung_t::default_mapmode );

	b_rotate45.pressed = karte->isometric;
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
		const int dot_len = proportional_string_width("..");
		const int fac_cols = clamp(legend.get_count(), 1, get_fenstergroesse().x / (D_DEFAULT_WIDTH/3));

		FOR(vector_tpl<legend_entry_t>, & l, legend) {
			std::string label = l.text;
			size_t i;
			for(  i=12;  i < label.size()  &&  display_calc_proportional_string_len_width(label.c_str(), i) < get_fenstergroesse().x / fac_cols - dot_len - 13;  i++  ) {}
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
	b_show_legend.pressed = show;
	legend_visible = show;

	b_overlay_networks.set_visible( show );

	const int col = max( 1, min( (get_fenstergroesse().x-2)/(D_BUTTON_WIDTH+D_H_SPACE), MAP_MAX_BUTTONS ) );
	const int row = ((MAP_MAX_BUTTONS-1)/col)+1;
	const int offset_y = (D_BUTTON_HEIGHT+2)*row;
	const koord offset = show ? koord(0, offset_y) : koord(0, -offset_y);

	for(  int type=0;  type<MAP_MAX_BUTTONS;  type++  ) {
		filter_buttons[type].set_visible(show);
	}
	scrolly.set_pos(scrolly.get_pos() + offset);

	set_min_windowsize(get_min_windowsize() + offset);
	set_fenstergroesse(get_fenstergroesse() + offset);
	resize(koord(0,0));
}


void map_frame_t::show_hide_scale(const bool show)
{
	b_show_scale.pressed = show;
	scale_visible = show;

	const koord offset = show ? koord(0, (LINESPACE+4)) : koord(0, -(LINESPACE+4));

	scrolly.set_pos(scrolly.get_pos() + offset);

	set_min_windowsize(get_min_windowsize() + offset);
	set_fenstergroesse(get_fenstergroesse() + offset);
	resize(koord(0,0));
}


void map_frame_t::show_hide_directory(const bool show)
{
	bool directory_view_toggeled = (b_show_directory.pressed != show);
	b_show_directory.pressed = show;
	b_filter_factory_list.pressed = filter_factory_list;
	directory_visible = show;

	b_filter_factory_list.set_visible( directory_visible );

	const int fac_cols = clamp( legend.get_count(), 1, get_fenstergroesse().x / (D_DEFAULT_WIDTH/3) );
	const int fac_rows = (legend.get_count() - 1) / fac_cols + 1 + D_V_SPACE*2 + D_BUTTON_HEIGHT;
	//No need to resize when only factory filter button state changes
	const koord offset = directory_view_toggeled ? (show ? koord(0, (fac_rows*LINESPACE)) : koord(0, -(fac_rows*LINESPACE))) : koord(0, 0);

	scrolly.set_pos(scrolly.get_pos() + offset);

	set_min_windowsize(get_min_windowsize() + offset);
	set_fenstergroesse(get_fenstergroesse() + offset);
	resize(koord(0,0));
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
		resize(koord(0,0));
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
			resize( koord(0,0) );
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
		const koord s_gr = scrolly.get_groesse() - koord(scrollbar_t::BAR_SIZE, scrollbar_t::BAR_SIZE);
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

	scrolly.set_groesse(get_client_windowsize()-scrolly.get_pos());
	map_frame_t::size = get_fenstergroesse();
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   01-Jun-2002
 */
void map_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);

	const KOORD_VAL old_offset_y = scrolly.get_pos().y;
	int offset_y = D_BUTTON_HEIGHT*2 + D_V_SPACE*2;

	if(legend_visible) {
		// MOVE NETWORK OVERLAY BUTTON
		b_overlay_networks.set_pos( koord( D_MARGIN_LEFT, offset_y ) );
		offset_y += D_BUTTON_HEIGHT;

		// calculate space with legend
		const int col = max( 1, min( (get_fenstergroesse().x-D_MARGIN_LEFT-D_MARGIN_RIGHT+D_H_SPACE)/(D_BUTTON_WIDTH+D_H_SPACE), MAP_MAX_BUTTONS ) );
		const int row = ((MAP_MAX_BUTTONS-1)/col)+1;

		// set button pos
		for (int type=0; type<MAP_MAX_BUTTONS; type++) {
			koord pos = koord( D_MARGIN_LEFT+(D_BUTTON_WIDTH+D_H_SPACE)*(type%col), offset_y+(D_BUTTON_HEIGHT+2)*((int)type/col) );
			filter_buttons[type].set_pos( pos );
		}
		offset_y += (D_BUTTON_HEIGHT+2)*row;
		offset_y += D_V_SPACE;
	}

	if(scale_visible) {
		// plus scale bar
		offset_y += LINESPACE;
		offset_y += D_V_SPACE;
	}

	if(directory_visible) {
		b_filter_factory_list.set_pos( koord( D_MARGIN_LEFT, offset_y ) );
		offset_y += D_BUTTON_HEIGHT;
		// full program including factory texts
		update_factory_legend();
		const int fac_cols = clamp( legend.get_count(), 1, get_fenstergroesse().x / (D_DEFAULT_WIDTH/3));
		const int fac_rows = (legend.get_count() - 1) / fac_cols + 1;
		offset_y += fac_rows*LINESPACE;
		offset_y += D_V_SPACE;
	}

	// offset of map
	scrolly.set_pos( koord(0,offset_y) );
	set_dirty();

	if(  offset_y != old_offset_y  ) {
		set_min_windowsize(get_min_windowsize() + koord(0,offset_y - old_offset_y));
		resize(koord(0,0));
	}
}


/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void map_frame_t::zeichnen(koord pos, koord gr)
{
	// update our stored screen position
	screenpos = pos;

	reliefkarte_t::get_karte()->set_xy_offset_size( koord(scrolly.get_scroll_x(), scrolly.get_scroll_y()), koord(scrolly.get_groesse()-koord(12,11)) );

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

	gui_frame_t::zeichnen(pos, gr);

	char buf[16];
	sint16 zoom_in, zoom_out;
	reliefkarte_t::get_karte()->get_zoom_factors(zoom_out, zoom_in);
	sprintf( buf, "%i:%i", zoom_in, zoom_out );
	int zoomextwidth = display_proportional( pos.x+BUTTON1_X+D_BUTTON_HEIGHT+D_H_SPACE, pos.y+D_TITLEBAR_HEIGHT+D_BUTTON_HEIGHT+D_V_SPACE, buf, ALIGN_LEFT, COL_WHITE, true);
	// move zoom arrow position and label accordingly
	zoom_buttons[1].set_pos( koord( BUTTON1_X+D_BUTTON_HEIGHT+2*D_H_SPACE+zoomextwidth, zoom_buttons[1].get_pos().y ) );
	zoom_label.set_pos( koord( BUTTON1_X+2*D_BUTTON_HEIGHT+3*D_H_SPACE+zoomextwidth, zoom_label.get_pos().y ) );
	b_rotate45.set_pos( koord( zoom_label.get_pos().x+proportional_string_width(zoom_label.get_text_pointer())+D_H_SPACE+10, b_rotate45.get_pos().y ) );

	int offset_y = D_BUTTON_HEIGHT*2 + D_V_SPACE*2 + D_TITLEBAR_HEIGHT;
	if(legend_visible) {
		offset_y = D_TITLEBAR_HEIGHT+filter_buttons[MAP_MAX_BUTTONS-1].get_pos().y+D_BUTTON_HEIGHT+2+D_V_SPACE;
	}

	// draw scale
	if(scale_visible) {
		koord bar_pos = pos + koord( 0, offset_y+2 );
		// color bar
		for(  int i=0;  i<MAX_SEVERITY_COLORS;  i++  ) {
			display_fillbox_wh(bar_pos.x + 30 + i*(gr.x-60)/MAX_SEVERITY_COLORS, bar_pos.y+2,  (gr.x-60)/(MAX_SEVERITY_COLORS-1), 7, reliefkarte_t::calc_severity_color(i,MAX_SEVERITY_COLORS), false);
		}
		display_proportional(bar_pos.x + 26, bar_pos.y, translator::translate("min"), ALIGN_RIGHT, COL_BLACK, false);
		display_proportional(bar_pos.x + size.x - 26, bar_pos.y, translator::translate("max"), ALIGN_LEFT, COL_BLACK, false);
		offset_y += LINESPACE;
		offset_y += D_V_SPACE;
	}

	// draw factory descriptions
	if(directory_visible) {
		offset_y += D_BUTTON_HEIGHT;

		const int fac_cols = clamp(legend.get_count(), 1, gr.x / (D_DEFAULT_WIDTH/3));
		uint u = 0;
		FORX(vector_tpl<legend_entry_t>, const& i, legend, ++u) {
			const int xpos = pos.x + (u % fac_cols) * (gr.x-fac_cols/2-1)/fac_cols + 3;
			const int ypos = pos.y + (u / fac_cols) * LINESPACE + offset_y+2;

			if(  ypos+LINESPACE > pos.y+gr.y  ) {
				break;
			}
			display_fillbox_wh(xpos, ypos + 1 , 7, 7, i.colour, false);
			display_proportional(xpos + 9, ypos, i.text.c_str(), ALIGN_LEFT, COL_BLACK, false);
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
		resize( koord(0,0) );
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
