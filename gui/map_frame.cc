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

#include "simwin.h"

#include "../simsys.h"
#include "../simworld.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../simcolor.h"
#include "../bauer/fabrikbauer.h"
#include "../bauer/goods_manager.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../dataobj/koord.h"
#include "../dataobj/loadsave.h"
#include "../descriptor/factory_desc.h"
#include "../simfab.h"
#include "../utils/simrandom.h"
#include "../tpl/minivec_tpl.h"
#include "money_frame.h"


static koord old_ij=koord::invalid;

karte_ptr_t map_frame_t::welt;

scr_size map_frame_t::window_size;
bool  map_frame_t::legend_visible=false;
bool  map_frame_t::scale_visible=false;
bool  map_frame_t::directory_visible=false;
bool  map_frame_t::is_cursor_hidden=false;
bool  map_frame_t::filter_factory_list=true;

// Caches list of factories in current game world
stringhashtable_tpl<const factory_desc_t *> map_frame_t::factory_list;

// Hajo: we track our position onscreen
scr_coord map_frame_t::screenpos;

#define L_BUTTON_WIDTH (button_size.w)

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
	{ COL_WHITE,        COL_GREY5,       "Buildings", "Show level of city buildings", reliefkarte_t::MAP_LEVEL },
	{ COL_LIGHT_GREEN,  COL_DARK_GREEN,  "PaxDest", "Overlay passenger destinations when a town window is open", reliefkarte_t::MAP_PAX_DEST },
	{ COL_LIGHT_GREEN,  COL_DARK_GREEN,  "Tourists", "Highlite tourist attraction", reliefkarte_t::MAP_TOURIST },
	{ COL_LIGHT_GREEN,  COL_DARK_GREEN,  "Factories", "Highlite factories", reliefkarte_t::MAP_FACTORIES },
	{ COL_LIGHT_YELLOW, COL_DARK_YELLOW,       "Passagiere", "Show passenger coverage/passenger network", reliefkarte_t::MAP_PASSENGER },
	{ COL_LIGHT_YELLOW, COL_DARK_YELLOW,       "Post", "Show mail service coverage/mail network", reliefkarte_t::MAP_MAIL },
	{ COL_LIGHT_YELLOW, COL_DARK_YELLOW,       "Fracht", "Show transported freight/freight network", reliefkarte_t::MAP_FREIGHT },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE, "Status", "Show capacity and if halt is overcrowded", reliefkarte_t::MAP_STATUS },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE, "hl_btn_sort_waiting", "Show how many people/much is waiting at halts", reliefkarte_t::MAP_WAITING },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE, "Queueing", "Show the change of waiting at halts", reliefkarte_t::MAP_WAITCHANGE },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE, "Service", "Show how many convoi reach a station", reliefkarte_t::MAP_SERVICE },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE, "Transfers", "Sum of departure/arrivals at halts", reliefkarte_t::MAP_TRANSFER },
	{ COL_LIGHT_PURPLE, COL_DARK_PURPLE, "Origin", "Show initial passenger departure", reliefkarte_t::MAP_ORIGIN },
	{ COL_WHITE,        COL_GREY5,       "Traffic", "Show usage of network", reliefkarte_t::MAP_TRAFFIC },
	{ COL_WHITE,        COL_GREY5,       "Wear", "Show the condition of ways", reliefkarte_t::MAP_CONDITION },
	{ COL_WHITE,        COL_GREY5,       "Speedlimit", "Show speedlimit of ways", reliefkarte_t::MAX_SPEEDLIMIT },
	{ COL_WHITE,        COL_GREY5,       "Weight limit", "Show the weight limit of ways", reliefkarte_t::MAP_WEIGHTLIMIT },
	{ COL_WHITE,        COL_GREY5,       "Tracks", "Highlight railroad tracks", reliefkarte_t::MAP_TRACKS },
	{ COL_LIGHT_GREEN,  COL_DARK_GREEN,  "Depots", "Highlite depots", reliefkarte_t::MAP_DEPOT },
	{ COL_WHITE,        COL_GREY5,       "Powerlines", "Highlite electrical transmission lines", reliefkarte_t::MAP_POWERLINES },
	{ COL_WHITE,        COL_GREY5,       "Forest", "Highlite forests", reliefkarte_t::MAP_FOREST },
	{ COL_WHITE,        COL_GREY5,       "Ownership", "Show the owenership of infrastructure", reliefkarte_t::MAP_OWNER }
};

#define MAP_TRANSPORT_TYPE_ITEMS (9)
typedef struct {
	const char * name;
	simline_t::linetype line_type;
} transport_type_item_t;

transport_type_item_t transport_type_items[MAP_TRANSPORT_TYPE_ITEMS] = {
	{"All", simline_t::line},
	{"Maglev", simline_t::maglevline},
	{"Monorail", simline_t::monorailline},
	{"Train", simline_t::trainline},
	{"Narrowgauge", simline_t::narrowgaugeline},
	{"Tram", simline_t::tramline},
	{"Truck", simline_t::truckline},
	{"Ship", simline_t::shipline},
	{"Air", simline_t::airline}
};

map_frame_t::map_frame_t() :
	gui_frame_t( translator::translate("Reliefkarte") ),
	scrolly(reliefkarte_t::get_karte()),
	zoom_label("map zoom"),
	min_label("min"),
	max_label("max")
{
	scr_coord cursor( D_MARGIN_LEFT,D_MARGIN_TOP );
	const scr_coord_val zoom_label_width = display_get_char_max_width("0123456789") * 4 + display_get_char_width(':');
	const scr_size button_size(max(D_BUTTON_WIDTH, 100), D_BUTTON_HEIGHT);

	old_ij = koord::invalid;
	is_dragging = false;
	zoomed = false;

	// init map
	reliefkarte_t *karte = reliefkarte_t::get_karte();
	karte->init();

	const scr_size size = karte->get_size();
	const scr_size s_size=scrolly.get_size();
	const koord ij = welt->get_viewport()->get_world_position();
	const scr_size win_size = size-s_size; // this is the visible area
	karte->set_mode( (reliefkarte_t::MAP_MODES)env_t::default_mapmode );
	//show all players by default
	karte->player_showed_on_map = -1;
	scrolly.set_scroll_position(  max(0,min(ij.x-win_size.w/2,size.w)), max(0, min(ij.y-win_size.h/2,size.h)) );
	scrolly.set_focusable( true );
	scrolly.set_scrollbar_mode(scrollbar_t::show_always);

	// first row of controls
	// selections button
	b_show_legend.init(button_t::roundbox_state, "Show legend", cursor, button_size );
	b_show_legend.set_tooltip("Shows buttons on special topics.");
	b_show_legend.add_listener(this);
	add_component(&b_show_legend);
	cursor.x += L_BUTTON_WIDTH + D_H_SPACE;

	// industry list button
	b_show_directory.init(button_t::roundbox_state, "Show industry", cursor, button_size);
	b_show_directory.set_tooltip("Shows a listing with all industries on the map.");
	b_show_directory.add_listener(this);
	add_component(&b_show_directory);
	cursor.x += L_BUTTON_WIDTH + D_H_SPACE;

	// scale button
	b_show_scale.init(button_t::roundbox_state, "Show map scale", cursor, button_size);
	b_show_scale.set_tooltip("Shows the color code for several selections.");
	b_show_scale.add_listener(this);
	add_component(&b_show_scale);
	cursor = scr_coord(D_MARGIN_LEFT, cursor.y + D_BUTTON_HEIGHT + D_V_SPACE);

	// second row of controls
	// zoom levels label
	zoom_label.set_pos(cursor);
	zoom_label.set_color(SYSCOL_TEXT);
	add_component( &zoom_label );
	cursor.x += zoom_label.get_size().w + D_H_SPACE;

	// zoom levels arrow left
	zoom_buttons[0].init(button_t::repeatarrowleft, NULL,cursor);
	zoom_buttons[0].add_listener( this );
	add_component( zoom_buttons+0 );
	cursor.x += zoom_buttons[0].get_size().w;

	// zoom level value label
	zoom_value_label.set_pos(cursor);
	zoom_value_label.set_size( scr_size(zoom_label_width,LINESPACE) );
	zoom_value_label.set_align(gui_label_t::centered);
	zoom_value_label.set_color(SYSCOL_TEXT);
	add_component( &zoom_value_label );
	cursor.x += zoom_label_width;

	// zoom levels arrow right
	zoom_buttons[1].init(button_t::repeatarrowright, NULL, cursor);
	zoom_buttons[1].add_listener( this );
	add_component( zoom_buttons+1 );
	cursor.x += zoom_buttons[1].get_size().w + D_H_SPACE;

	// rotate map 45° (isometric view)
	b_rotate45.init( button_t::square_state, "isometric map", cursor);
	b_rotate45.set_tooltip("Similar view as the main window");
	b_rotate45.add_listener(this);
	b_rotate45.pressed = karte->isometric;
	add_component(&b_rotate45);

	// align second row
	// Max Kielland: This will be done automatically (and properly) by the new gui_layout_t control in the near future.
	zoom_value_label.align_to(&zoom_buttons[0],ALIGN_CENTER_V);
	zoom_buttons[1].align_to(&zoom_buttons[0],ALIGN_CENTER_V);
	zoom_label.align_to(&zoom_buttons[0],ALIGN_CENTER_V);
	b_rotate45.align_to(&zoom_buttons[0],ALIGN_CENTER_V);
	cursor = scr_coord(D_MARGIN_LEFT,zoom_buttons[0].get_pos().y+zoom_buttons[0].get_size().h+D_V_SPACE);

	// filter container
	filter_container.set_pos(cursor);
	filter_container.set_visible(false);
	add_component(&filter_container);

	// insert selections: show networks, in filter container
	b_overlay_networks.init(button_t::square_state, "Networks");
	b_overlay_networks.set_tooltip("Overlay schedules/network");
	b_overlay_networks.add_listener(this);
	b_overlay_networks.pressed = (env_t::default_mapmode & reliefkarte_t::MAP_LINES)!=0;
	filter_container.add_component( &b_overlay_networks );

	// player combo for network overlay
	viewed_player_c.set_pos( scr_coord(D_BUTTON_WIDTH + D_H_SPACE, 0) );
	viewed_player_c.set_size( scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT) );
	viewed_player_c.set_max_size(scr_size(116, 10 * D_BUTTON_HEIGHT));

	viewed_player_c.append_element( new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate("All"), COL_BLACK));
	viewable_players[ 0 ] = -1;
	for(  int np = 0, count = 1;  np < MAX_PLAYER_COUNT;  np++  ) {
		if(  welt->get_player( np )  &&  welt->get_player( np )->get_finance()->has_convoi()  ) {
			viewed_player_c.append_element( new gui_scrolled_list_t::const_text_scrollitem_t(welt->get_player( np )->get_name(), welt->get_player( np )->get_player_color1()+4));
			viewable_players[ count++ ] = np;
		}
	}

	viewed_player_c.set_selection(0);
	reliefkarte_t::get_karte()->player_showed_on_map = -1;
	viewed_player_c.set_focusable( true );
	viewed_player_c.add_listener( this );
	filter_container.add_component(&viewed_player_c);

	// freight combo for network overlay
	freight_type_c.set_pos( scr_coord(2*D_BUTTON_WIDTH+3*D_H_SPACE, 0) );
	freight_type_c.set_size( scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT) );
	freight_type_c.set_max_size( scr_size( 116, 5 * D_BUTTON_HEIGHT) );
	{
		int count = 0;
		viewable_freight_types[count++] = NULL;
		freight_type_c.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("All"), SYSCOL_TEXT) );
		viewable_freight_types[count++] = goods_manager_t::passengers;
		freight_type_c.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Passagiere"), SYSCOL_TEXT) );
		viewable_freight_types[count++] = goods_manager_t::mail;
		freight_type_c.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Post"), SYSCOL_TEXT) );
		viewable_freight_types[count++] = goods_manager_t::none; // for all freight ...
		freight_type_c.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Fracht"), SYSCOL_TEXT) );
		for(  int i = 0;  i < goods_manager_t::get_max_catg_index();  i++  ) {
			const goods_desc_t *freight_type = goods_manager_t::get_info_catg(i);
			const int index = freight_type->get_catg_index();
			if(  index == goods_manager_t::INDEX_NONE  ||  freight_type->get_catg()==0  ) {
				continue;
			}
			freight_type_c.append_element( new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(freight_type->get_catg_name()), SYSCOL_TEXT));
			viewable_freight_types[count++] = freight_type;
		}
		for(  int i=0;  i < goods_manager_t::get_count();  i++  ) {
			const goods_desc_t *ware = goods_manager_t::get_info(i);
			if(  ware->get_catg() == 0  &&  ware->get_index() > 2  ) {
				// Special freight: Each good is special
				viewable_freight_types[count++] = ware;
				freight_type_c.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate(ware->get_name()), SYSCOL_TEXT) );
			}
		}
	}
	freight_type_c.set_selection(0);
	reliefkarte_t::get_karte()->freight_type_group_index_showed_on_map = NULL;
	freight_type_c.set_focusable( true );
	freight_type_c.add_listener( this );
	filter_container.add_component(&freight_type_c);

	// mode of transpost combo for network overlay
	transport_type_c.set_pos(scr_coord(3 * (D_BUTTON_WIDTH + D_H_SPACE), 0));
	transport_type_c.set_size( scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT) );
	transport_type_c.set_max_size( scr_size( 116, 10 * D_BUTTON_HEIGHT) );

	for (int i = 0; i < MAP_TRANSPORT_TYPE_ITEMS; i++) {
		transport_type_c.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(transport_type_items[i].name), SYSCOL_TEXT));
		viewable_transport_types[ i ] = transport_type_items[i].line_type;
	}

	transport_type_c.set_selection(0);
	reliefkarte_t::get_karte()->transport_type_showed_on_map = simline_t::line;
	transport_type_c.set_focusable( true );
	transport_type_c.add_listener( this );
	filter_container.add_component(&transport_type_c);

	b_overlay_networks_load_factor.init(button_t::square_state, "Free Capacity");
	b_overlay_networks_load_factor.set_pos(scr_coord(4*(D_BUTTON_WIDTH+D_H_SPACE), 0));
	b_overlay_networks_load_factor.set_tooltip("Color according to transport capacity left");
	b_overlay_networks_load_factor.add_listener(this);
	b_overlay_networks_load_factor.pressed = 0;
	reliefkarte_t::get_karte()->show_network_load_factor = 0;
	filter_container.add_component( &b_overlay_networks_load_factor );

	// insert filter buttons in legend container
	for (int index=0; index<MAP_MAX_BUTTONS; index++) {
		filter_buttons[index].init( button_t::box_state, button_init[index].button_text, scr_coord(0,0), button_size);
		filter_buttons[index].set_tooltip( button_init[index].tooltip_text );
		filter_buttons[index].pressed = button_init[index].mode&env_t::default_mapmode;
		filter_buttons[index].background_color = filter_buttons[index].pressed ? button_init[index].select_color : button_init[index].color;
		filter_buttons[index].text_color = filter_buttons[index].pressed ? SYSCOL_TEXT_HIGHLIGHT : SYSCOL_TEXT;
		filter_buttons[index].add_listener(this);
		filter_container.add_component(filter_buttons + index);
	}

	// directory container
	directory_container.set_pos(cursor);
	directory_container.set_visible(false);
	add_component(&directory_container);

	// factory list: show used button
	b_filter_factory_list.init(button_t::square_state, "Show only used");
	b_filter_factory_list.set_tooltip("In the industry legend show only currently existing factories");
	b_filter_factory_list.add_listener(this);
	directory_container.add_component( &b_filter_factory_list );
	update_factory_legend();

	// scale container
	scale_container.set_pos(cursor);
	scale_container.set_visible(false);
	add_component(&scale_container);
	scale_container.add_component(&min_label);
	scale_container.add_component(&max_label);
	scale_container.add_component(&tile_scale_label);

	// map scrolly
	scrolly.set_show_scroll_x(true);
	scrolly.set_scroll_discrete_y(false);
	add_component(&scrolly);

	// restore window size and options
	set_windowsize( window_size );
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
			minivec_tpl<uint8> colours;
			FOR(vector_tpl<fabrik_t*>, const f, welt->get_fab_list()) {
				factory_desc_t const& d = *f->get_desc();
				if(  d.get_distribution_weight() > 0  ) {
					if( colours.append_unique(d.get_kennfarbe()) ) {
						std::string const label( translator::translate(d.get_name()) );
						legend.append_unique( legend_entry_t(label, d.get_kennfarbe()) );
					}
				}
			}
		}
		else {
			FOR(stringhashtable_tpl<factory_desc_t const*>, const& i, factory_builder_t::get_factory_table()) {
				factory_desc_t const& d = *i.value;
				if ( d.get_distribution_weight() > 0 ) {
					std::string const label(translator::translate(d.get_name()));
					legend.append_unique( legend_entry_t(label, d.get_kennfarbe()) );
				}
			}
		}

		// If any names are too long for current windows size, shorten them. Can't be done in above loop in case shortening some names cause
		// then to become non unique and thus get clumped together
		scr_coord_val client_width = get_windowsize().w - D_MARGIN_LEFT - D_MARGIN_RIGHT;
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


bool map_frame_t::action_triggered( gui_action_creator_t *comp, value_t)
{
	if(comp==&b_show_legend) {
		show_hide_legend( !b_show_legend.pressed );
	}
	else if(comp==&b_show_scale) {
		show_hide_scale( !b_show_scale.pressed );
	}
	else if(comp==&b_show_directory) {
		show_hide_directory( !b_show_directory.pressed );
	}
	else if (comp==&b_filter_factory_list) {
		filter_factory_list = !filter_factory_list;
		show_hide_directory( b_show_directory.pressed );
	}
	else if(comp==zoom_buttons+1) {
		// zoom out
		zoom(true);
	}
	else if(comp==zoom_buttons+0) {
		// zoom in
		zoom(false);
	}
	else if(comp==&b_rotate45) {
		// rotated/straight map
		reliefkarte_t::get_karte()->isometric ^= 1;
		b_rotate45.pressed = reliefkarte_t::get_karte()->isometric;
		reliefkarte_t::get_karte()->calc_map_size();
		scrolly.set_size( scrolly.get_size() );
	}
	else if(comp==&b_overlay_networks) {
		b_overlay_networks.pressed ^= 1;
		if(  b_overlay_networks.pressed  ) {
			env_t::default_mapmode |= reliefkarte_t::MAP_LINES;
		}
		else {
			env_t::default_mapmode &= ~reliefkarte_t::MAP_LINES;
		}
		reliefkarte_t::get_karte()->set_mode(  (reliefkarte_t::MAP_MODES)env_t::default_mapmode  );
	}
	else if (comp == &viewed_player_c) {
		reliefkarte_t::get_karte()->player_showed_on_map = viewable_players[viewed_player_c.get_selection()];
		reliefkarte_t::get_karte()->invalidate_map_lines_cache();
	}
	else if (comp == &transport_type_c) {
		reliefkarte_t::get_karte()->transport_type_showed_on_map = viewable_transport_types[transport_type_c.get_selection()];
		reliefkarte_t::get_karte()->invalidate_map_lines_cache();
	}
	else if (comp == &freight_type_c) {
		reliefkarte_t::get_karte()->freight_type_group_index_showed_on_map = viewable_freight_types[freight_type_c.get_selection()];
		reliefkarte_t::get_karte()->invalidate_map_lines_cache();
	}
	else if (comp == &b_overlay_networks_load_factor) {
		reliefkarte_t::get_karte()->show_network_load_factor = !reliefkarte_t::get_karte()->show_network_load_factor;
		b_overlay_networks_load_factor.pressed = !b_overlay_networks_load_factor.pressed;
		reliefkarte_t::get_karte()->invalidate_map_lines_cache();
	}
	else {
		for(  int i=0;  i<MAP_MAX_BUTTONS;  i++  ) {
			if(  comp == filter_buttons+i  ) {
				if(  filter_buttons[i].pressed  ) {
					env_t::default_mapmode &= ~button_init[i].mode;
				}
				else {
					if(  (button_init[i].mode & reliefkarte_t::MAP_MODE_FLAGS) == 0  ) {
						// clear all persistent states
						env_t::default_mapmode &= reliefkarte_t::MAP_MODE_FLAGS;
					}
					else if(  button_init[i].mode & reliefkarte_t::MAP_MODE_HALT_FLAGS  ) {
						// clear all other halt states
						env_t::default_mapmode &= ~reliefkarte_t::MAP_MODE_HALT_FLAGS;
					}
					env_t::default_mapmode |= button_init[i].mode;
				}
				filter_buttons[i].pressed ^= 1;
				break;
			}
		}
		reliefkarte_t::get_karte()->set_mode(  (reliefkarte_t::MAP_MODES)env_t::default_mapmode  );
		for(  int i=0;  i<MAP_MAX_BUTTONS;  i++  ) {
			filter_buttons[i].pressed = (button_init[i].mode&env_t::default_mapmode)!=0;
			filter_buttons[i].background_color = filter_buttons[i].pressed ? button_init[i].select_color : button_init[i].color;
			filter_buttons[i].text_color = filter_buttons[i].pressed ? SYSCOL_TEXT_HIGHLIGHT : SYSCOL_TEXT;
		}
	}
	filter_buttons[18].set_tooltip("Passenger destinations");
	return true;
}


void map_frame_t::zoom(bool magnify)
{
	if (reliefkarte_t::get_karte()->change_zoom_factor(magnify)) {
		zoomed = true;
		// recalc all the other data incl scrollbars
		resize();
	}
}


/**
 * Events werden hiermit an die GUI-components
 * gemeldet
 * @author Hj. Malthaner
 */
bool map_frame_t::infowin_event(const event_t *ev)
{
	event_t ev2 = *ev;
	translate_event(&ev2, -scrolly.get_pos().x, -scrolly.get_pos().y-D_TITLEBAR_HEIGHT);

	if(ev->ev_class == INFOWIN) {
		if(ev->ev_code == WIN_OPEN) {
			reliefkarte_t::get_karte()->set_xy_offset_size( scr_coord(0,0), scr_size(0,0) );
		}
		else if(ev->ev_code == WIN_CLOSE) {
			reliefkarte_t::get_karte()->is_visible = false;
		}
	}

	// comboboxes shoudl loose their focus on close
	gui_component_t *focus = win_get_focus();
	if(  focus == &viewed_player_c  ) {
		if(  !viewed_player_c.is_dropped()  ) {
			set_focus( NULL );
		}
	}
	else if(  focus == &transport_type_c  ) {
		if(  !transport_type_c.is_dropped()  ) {
			set_focus( NULL );
		}
	}
	else if(  focus == &viewed_player_c  ) {
		if(  !freight_type_c.is_dropped()  ) {
			set_focus( NULL );
		}
	}

	if(  reliefkarte_t::get_karte()->getroffen(ev2.mx,ev2.my)  ) {
		set_focus( reliefkarte_t::get_karte() );
	}

	if(  (IS_WHEELUP(ev) || IS_WHEELDOWN(ev))  &&  reliefkarte_t::get_karte()->getroffen(ev2.mx,ev2.my)  ) {
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
		return true;
	}
	else if(  IS_RIGHTRELEASE(ev)  ) {
		if(!is_dragging) {
			resize();
		}
		is_dragging = false;
		display_show_pointer(true);
		is_cursor_hidden = false;
		return true;
	}
	else if(  IS_RIGHTDRAG(ev)  &&  (reliefkarte_t::get_karte()->getroffen(ev2.mx,ev2.my)  ||  reliefkarte_t::get_karte()->getroffen(ev2.cx,ev2.cy))  ) {
		int x = scrolly.get_scroll_x();
		int y = scrolly.get_scroll_y();
		const int scroll_direction = ( env_t::scroll_multi>0 ? 1 : -1 );

		x += (ev->mx - ev->cx)*scroll_direction*2;
		y += (ev->my - ev->cy)*scroll_direction*2;

		is_dragging = true;

		scrolly.set_scroll_position(  max(0, x),  max(0, y) );

		// Move the mouse pointer back to starting location
		// To prevent a infinite mouse event loop, we just do it when needed.
		if ((ev->mx - ev->cx)!=0  ||  (ev->my-ev->cy)!=0) {
			move_pointer(screenpos.x + ev->cx, screenpos.y+ev->cy);
		}

		return true;
	}
	else if(  IS_LEFTDBLCLK(ev)  &&  reliefkarte_t::get_karte()->getroffen(ev2.mx,ev2.my)  ) {
		// re-center cursor by scrolling
		koord ij = welt->get_viewport()->get_world_position();
		scr_coord center = reliefkarte_t::get_karte()->karte_to_screen(ij);
		const scr_size s_size = scrolly.get_size();

		scrolly.set_scroll_position(max(0,center.x-(s_size.w/2)), max(0,center.y-(s_size.h/2)));
		zoomed = false;

		// remember world position, we do not want to have surprises when scrolling later on
		old_ij = ij;
		return true;
	}
	else if(  IS_RIGHTDBLCLK(ev)  ) {
		// zoom to fit window
		do { // first, zoom all the way in
			zoomed = false;
			zoom(true);
		} while(  zoomed  );

		// then zoom back out to fit
		const scr_size s_size = scrolly.get_size() - D_SCROLLBAR_SIZE;
		scr_size size = reliefkarte_t::get_karte()->get_size();
		zoomed = true;
		while(  zoomed  &&  max(size.w/s_size.w, size.h/s_size.h)  ) {
			zoom(false);
			size = reliefkarte_t::get_karte()->get_size();
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
void map_frame_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize( size );
	window_size = get_windowsize();
	scrolly.set_size( get_client_windowsize()-scrolly.get_pos()-scr_size(1,1) );
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   01-Jun-2002
 */

void map_frame_t::resize(const scr_coord delta)
{
	scr_coord_val offset_y = filter_container.get_pos().y;
	scr_coord_val client_width = get_windowsize().w-D_MARGIN_LEFT - D_MARGIN_RIGHT;

	// resize legend
	if(legend_visible) {
		const scr_coord_val left = 0;
		const scr_coord_val right = left + client_width;
		scr_coord cursor(left, b_overlay_networks.get_size().h + D_V_SPACE);
		scr_size button_size;
		for (int type=0; type<MAP_MAX_BUTTONS; type++) {
			button_size = filter_buttons[type].get_size();
			if (cursor.x + button_size.w > right)
			{
				// start a new line
				cursor.x = left;
				cursor.y += button_size.h + D_V_SPACE;
			}
			filter_buttons[type].set_pos( cursor );
			cursor.x += button_size.w + D_H_SPACE;
		}
		filter_container.set_size(scr_size(client_width, cursor.y + button_size.h + D_V_SPACE));
		offset_y += filter_container.get_size().h + D_V_SPACE;

		// resize combo boxes
		viewed_player_c.set_max_size( scr_size(116, filter_container.get_size().h ) );
		freight_type_c.set_max_size( scr_size(116, filter_container.get_size().h ) );
		transport_type_c.set_max_size( scr_size(116, filter_container.get_size().h ) );
	}

	if(directory_visible) {
		scr_coord_val button_y = b_filter_factory_list.get_size().h+D_V_SPACE;
		scr_coord_val line_height = max(D_INDICATOR_BOX_HEIGHT,LINESPACE);

		directory_container.set_pos(scr_coord(D_MARGIN_LEFT,offset_y));


		// full program including factory texts
		update_factory_legend();

		const int fac_cols = clamp( legend.get_count(), 1, client_width / ((D_DEFAULT_WIDTH-D_MARGINS_X)/3));
		const int fac_rows = (legend.get_count() - 1) / fac_cols + 1;

		// calculate client height and set height
		directory_container.set_size(scr_size(client_width,button_y+fac_rows*line_height));
		offset_y += directory_container.get_size().h + D_V_SPACE;
	}

	// resize scale
	if(scale_visible) {
		scale_container.set_pos(scr_coord(D_MARGIN_LEFT,offset_y));
		scale_container.set_size(scr_size(client_width,LINESPACE + D_V_SPACE + D_LABEL_HEIGHT));
		max_label.align_to(&scale_container,ALIGN_RIGHT,scr_coord(scale_container.get_pos().x,0));
		tile_scale_label.set_pos(scr_coord(0, LINESPACE + D_V_SPACE));
		tile_scale_label.set_width(client_width);
		offset_y += scale_container.get_size().h + D_V_SPACE;
	}

	// offset of map
	scrolly.set_pos( scr_coord(1,offset_y) );

	scr_coord_val min_width = max(BUTTON4_X+D_MARGIN_RIGHT-D_H_SPACE,D_DEFAULT_WIDTH);
	set_min_windowsize(scr_size(min_width, D_TITLEBAR_HEIGHT+offset_y+64+D_SCROLLBAR_HEIGHT+1));
	set_dirty();

	gui_frame_t::resize(delta);
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 * @author Hj. Malthaner
 */
void map_frame_t::draw(scr_coord pos, scr_size size)
{
	char buf[16];
	sint16 zoom_in, zoom_out;

	// update our stored screen position
	screenpos = pos;

	reliefkarte_t::get_karte()->set_xy_offset_size( scr_coord(scrolly.get_scroll_x(), scrolly.get_scroll_y()), scrolly.get_client().get_size() );

	// first: check if cursor within map screen size
	koord ij = welt->get_viewport()->get_world_position();
	if(welt->is_within_limits(ij)) {
		scr_coord center = reliefkarte_t::get_karte()->karte_to_screen(ij);
		// only re-center if zoomed or world position has changed and its outside visible area
		const scr_size size = scrolly.get_size();
		if(zoomed  ||  ( old_ij != ij  &&
				( scrolly.get_scroll_x()>center.x  ||  scrolly.get_scroll_x()+size.w<=center.x  ||
				  scrolly.get_scroll_y()>center.y  ||  scrolly.get_scroll_y()+size.h<=center.y ) ) ) {
				// re-center cursor by scrolling
				scrolly.set_scroll_position( max(0,center.x-(size.w/2)), max(0,center.y-(size.h/2)) );
				zoomed = false;
		}
		// remember world position, we do not want to have surprises when scrolling later on
		old_ij = ij;
	}

	// update zoom factors and zoom label
	reliefkarte_t::get_karte()->get_zoom_factors(zoom_out, zoom_in);
	sprintf( buf, "%i:%i", zoom_in, zoom_out );
	zoom_value_label.set_text_pointer(buf,false);

	if(scale_visible) {
		if(!scale_text)
		{
			scale_text = new char[160]; 
		}
		if(1000 % welt->get_settings().get_meters_per_tile() == 0)
		{
			// Can use integer
			sprintf(scale_text, "%i %s %s", (uint16)(1000 / welt->get_settings().get_meters_per_tile()), translator::translate("tiles"), translator::translate("per 1 km"));
		}
		else
		{
			// Otherwise, must use float	
			sprintf(scale_text, "%f %s %s", (1000.0 / welt->get_settings().get_meters_per_tile()), translator::translate("tiles"), translator::translate("per 1 km"));
		}
		tile_scale_label.set_text(scale_text, false);
	}

	// draw all child controls
	gui_frame_t::draw(pos, size);

	// draw scale
	if(scale_visible) {
		scr_coord bar_pos( pos.x + scale_container.get_pos().x, pos.y + scale_container.get_pos().y + D_TITLEBAR_HEIGHT );
		scr_coord_val bar_client_x = scale_container.get_size().w - min_label.get_size().w - max_label.get_size().w - (D_H_SPACE<<1);
		double bar_width = (double)bar_client_x/(double)MAX_SEVERITY_COLORS;
		// color bar
		for(  int i=0;  i<MAX_SEVERITY_COLORS;  i++  ) {
			display_fillbox_wh(bar_pos.x + min_label.get_size().w + D_H_SPACE + (i*bar_width), bar_pos.y+2,  bar_width+1, 7, reliefkarte_t::calc_severity_color(i+1,MAX_SEVERITY_COLORS), false);
		}
	}

	// draw factory descriptions
	if(directory_visible) {
		scr_coord_val offset_y = directory_container.get_pos().y + b_filter_factory_list.get_size().h + D_V_SPACE;
		const int columns = clamp(legend.get_count(), 1, (size.w-D_MARGIN_RIGHT-D_MARGIN_LEFT) / ((D_DEFAULT_WIDTH-D_MARGINS_X)/3));
		scr_coord_val u = 0;
		scr_coord_val line_height = max(D_INDICATOR_BOX_HEIGHT,LINESPACE);

		FORX(vector_tpl<legend_entry_t>, const& i, legend, ++u) {
			const int xpos = pos.x+D_MARGIN_LEFT + (u % columns) * (size.w-D_MARGIN_RIGHT-columns/2-1)/columns;
			const int ypos = pos.y+D_TITLEBAR_HEIGHT + offset_y + (u / columns) * line_height;
			display_fillbox_wh(xpos, ypos + D_GET_CENTER_ALIGN_OFFSET(D_INDICATOR_BOX_HEIGHT,LINESPACE), D_INDICATOR_BOX_WIDTH, D_INDICATOR_BOX_HEIGHT, i.colour, false);
			display_ddd_box(xpos, ypos + D_GET_CENTER_ALIGN_OFFSET(D_INDICATOR_BOX_HEIGHT,LINESPACE), D_INDICATOR_BOX_WIDTH, D_INDICATOR_BOX_HEIGHT,SYSCOL_SHADOW,SYSCOL_SHADOW,false);
			display_proportional(xpos + D_INDICATOR_BOX_WIDTH + 2, ypos, i.text.c_str(), ALIGN_LEFT, SYSCOL_TEXT, false);
		}
	}

	// may add compass
	if(  reliefkarte_t::get_karte()->isometric  &&  skinverwaltung_t::compass_iso  ) {
		display_img_aligned( skinverwaltung_t::compass_iso->get_image_id( welt->get_settings().get_rotation() ), scrolly.get_client()+pos+scr_coord(4,4+D_TITLEBAR_HEIGHT)-scr_size(8,8), ALIGN_RIGHT|ALIGN_TOP, false );
	}
	else if(  !reliefkarte_t::get_karte()->isometric  &&  skinverwaltung_t::compass_rect  ) {
		display_img_aligned( skinverwaltung_t::compass_rect->get_image_id( welt->get_settings().get_rotation() ), scrolly.get_client()+pos+scr_coord(4,4+D_TITLEBAR_HEIGHT)-scr_size(8,8), ALIGN_RIGHT|ALIGN_TOP, false );
	}
}


void map_frame_t::rdwr( loadsave_t *file )
{
	bool is_show_schedule = (env_t::default_mapmode & reliefkarte_t::MAP_LINES);
	bool is_show_fab = (env_t::default_mapmode & reliefkarte_t::MAP_FACTORIES);

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
		sint8 mode = log2(env_t::default_mapmode & ~reliefkarte_t::MAP_MODE_FLAGS);
		file->rdwr_byte( mode );
		env_t::default_mapmode = 1 << mode;
		env_t::default_mapmode = mode>=0 ? 1 << mode : 0;
		env_t::default_mapmode |= is_show_schedule * reliefkarte_t::MAP_LINES;
		env_t::default_mapmode |= is_show_fab * reliefkarte_t::MAP_FACTORIES;
	}
	else {
		file->rdwr_long( env_t::default_mapmode );
	}
	file->rdwr_bool( b_overlay_networks_load_factor.pressed );

	if(  file->is_loading()  ) {
		scr_size savesize;
		savesize.rdwr(file);
		set_windowsize( savesize );
		resize();
		// notify minimap of new settings
		reliefkarte_t::get_karte()->calc_map_size();
		scrolly.set_size( scrolly.get_size() );

		sint32 xoff;
		file->rdwr_long( xoff );
		sint32 yoff;
		file->rdwr_long( yoff );
		scrolly.set_scroll_position( xoff, yoff );

		reliefkarte_t::get_karte()->set_mode((reliefkarte_t::MAP_MODES)env_t::default_mapmode);
		for (uint i=0;i<MAP_MAX_BUTTONS;i++) {
			filter_buttons[i].pressed = i==env_t::default_mapmode;
		}
		if(  legend_visible!=show_legend_state  ) {
			action_triggered( &b_show_legend, (long)0 );
		}
		b_filter_factory_list.set_visible( directory_visible );

		b_overlay_networks.pressed = (env_t::default_mapmode & reliefkarte_t::MAP_LINES)!=0;
		// restore selection
		sint16 idx;
		file->rdwr_short(idx);
		viewed_player_c.set_selection(idx);
		file->rdwr_short(idx);
		transport_type_c.set_selection(idx);
		file->rdwr_short(idx);
		freight_type_c.set_selection(idx);
	}
	else {
		scr_size size = get_windowsize();
		size.rdwr(file);
		sint32 xoff = scrolly.get_scroll_x();
		file->rdwr_long( xoff );
		sint32 yoff = scrolly.get_scroll_y();
		file->rdwr_long( yoff );
		if(file->is_saving()) {
			sint16 idx = viewed_player_c.get_selection();
			file->rdwr_short(idx);
			idx = transport_type_c.get_selection();
			file->rdwr_short(idx);
			idx = freight_type_c.get_selection();
			file->rdwr_short(idx);
		}
	}
}
