/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_HALT_DETAIL_H
#define GUI_HALT_DETAIL_H


#include "components/gui_textarea.h"

#include "gui_frame.h"
#include "components/gui_scrollpane.h"
#include "components/gui_label.h"
#include "components/gui_tab_panel.h"
#include "components/gui_container.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_halthandled_lines.h"

#include "../halthandle_t.h"
#include "../utils/cbuffer_t.h"
#include "../gui/simwin.h"
#include "../simfab.h"

class player_t;


// tab1 - pax and mail
class halt_detail_pas_t : public gui_container_t
{
private:
	halthandle_t halt;

	cbuffer_t pas_info;

public:
	halt_detail_pas_t(halthandle_t halt);

	void set_halt(halthandle_t h) { halt = h; }

	void draw(scr_coord offset);

	// draw pax or mail classes waiting amount information table
	void draw_class_table(scr_coord offset, const uint8 class_name_cell_width, const goods_desc_t *warentyp);
};

// tab2 - freight
class halt_detail_goods_t : public gui_container_t
{
private:
	halthandle_t halt;
	karte_t *welt;

	cbuffer_t goods_info;

public:
	halt_detail_goods_t(halthandle_t halt);
	void set_halt(halthandle_t h) { halt = h; }

	uint32 active_freight_catg = 0;

	void recalc_size();

	void draw(scr_coord offset);
};

// A display of nearby industries info for station detail
class gui_halt_nearby_factory_info_t : public gui_world_component_t
{
private:
	halthandle_t halt;

	slist_tpl<const goods_desc_t *> required_material;
	slist_tpl<const goods_desc_t *> active_product;
	slist_tpl<const goods_desc_t *> inactive_product;

	uint32 line_selected;

public:
	gui_halt_nearby_factory_info_t(const halthandle_t& halt);

	bool infowin_event(event_t const *ev) OVERRIDE;

	void recalc_size();

	void draw(scr_coord offset) OVERRIDE;
};


class gui_halt_route_info_t : public gui_world_component_t
{
private:
	halthandle_t halt;

	vector_tpl<halthandle_t> halt_list;
	uint32 line_selected;

	uint8 selected_route_catg_index = goods_manager_t::INDEX_PAS;
	uint8 selected_class = 255;
	bool station_display_mode;

	void draw_list_by_catg(scr_coord offset);
	void draw_list_by_dest(scr_coord offset);

public:
	gui_halt_route_info_t(const halthandle_t& halt, uint8 catg_index, bool station_mode = false);

	void build_halt_list(uint8 catg_index, uint8 g_class = 255, bool station_mode = false);
	bool infowin_event(event_t const *ev) OVERRIDE;

	void recalc_size();

	void draw(scr_coord offset) OVERRIDE;
};

class halt_detail_t : public gui_frame_t, action_listener_t
{
private:
	halthandle_t halt;
	player_t *cached_active_player; // So that, if different from current, change line links
	uint32 cached_line_count;
	uint32 cached_convoy_count;
	uint32 old_factory_count, old_catg_count;
	uint32 update_time;
	scr_coord_val cashed_size_y;
	static sint16 tabstate;
	bool show_pas_info, show_freight_info;

	cbuffer_t buf;

	gui_halthandled_lines_t line_number;
	halt_detail_pas_t pas;
	halt_detail_goods_t goods;
	gui_container_t cont, cont_goods, cont_route;
	gui_scrollpane_t scrolly_pas, scrolly_goods, scrolly_route;
	gui_label_t lb_selected_route_catg;
	gui_heading_t lb_nearby_factory, lb_routes, lb_serve_catg, lb_serve_lines, lb_serve_convoys;

	gui_halt_nearby_factory_info_t nearby_factory;
	gui_tab_panel_t tabs;
	gui_scrollpane_t scrolly;


	// service tab stuffs
	gui_textarea_t txt_info;
	slist_tpl<gui_label_t *>linelabels;
	slist_tpl<button_t *>linebuttons;
	slist_tpl<gui_label_t *> convoylabels;
	slist_tpl<button_t *> convoybuttons;
	slist_tpl<char*> label_names;

	// route tab stuffs
	gui_halt_route_info_t route;
	bool list_by_station = false;
	button_t bt_by_category, bt_by_station;
	slist_tpl<button_t *>catg_buttons, pas_class_buttons, mail_class_buttons;
	char *pass_class_name_untranslated[32];
	char *mail_class_name_untranslated[32];
	uint8 selected_route_catg_index = goods_manager_t::INDEX_NONE;
	uint8 selected_class = 0;
	// Opening and closing the button panel on the route tab
	void open_close_catg_buttons();

	bool has_min_sizer() const OVERRIDE { return true; }

	void update_components();

	void set_tab_opened();

public:
	halt_detail_t(halthandle_t halt);

	~halt_detail_t();

	void halt_detail_info();

	void init();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char * get_help_filename() const OVERRIDE { return "station_details.txt"; }

	// Set window size and adjust component sizes and/or positions accordingly
	virtual void set_windowsize(scr_size size) OVERRIDE;

	virtual koord3d get_weltpos(bool set) OVERRIDE;

	virtual bool is_weltpos() OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	bool infowin_event(const event_t *ev) OVERRIDE;

	// only defined to update schedule, if changed
	void draw( scr_coord pos, scr_size size ) OVERRIDE;

	// this constructor is only used during loading
	halt_detail_t();

	void rdwr( loadsave_t *file ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_halt_detail; }
};

#endif
