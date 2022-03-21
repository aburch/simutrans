/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TOOL_SIMTOOL_DIALOGS_H
#define TOOL_SIMTOOL_DIALOGS_H


#include "../tool/simmenu.h"
#include "../gui/simwin.h"

#include "../dataobj/translator.h"
#include "../dataobj/scenario.h"

#include "../gui/factory_edit.h"
#include "../gui/curiosity_edit.h"
#include "../gui/citybuilding_edit.h"
#include "../gui/baum_edit.h"
#include "../gui/groundobj_edit.h"
#include "../gui/jump_frame.h"
#include "../gui/optionen.h"
#include "../gui/map_frame.h"
#include "../gui/display_settings.h"
#include "../gui/player_frame.h"
#include "../gui/loadsave_frame.h"
#include "../gui/money_frame.h"
#include "../gui/schedule_list.h"
#include "../gui/sound_frame.h"
#include "../gui/sprachen.h"
#include "../gui/kennfarbe.h"
#include "../gui/help_frame.h"
#include "../gui/message_frame.h"
#include "../gui/messagebox.h"
#include "../gui/convoi_frame.h"
#include "../gui/halt_list_frame.h"
#include "../gui/citylist_frame.h"
#include "../gui/goods_frame.h"
#include "../gui/factorylist_frame.h"
#include "../gui/curiositylist_frame.h"
#include "../gui/enlarge_map_frame.h"
#include "../gui/labellist_frame.h"
#include "../gui/climates.h"
#include "../gui/settings_frame.h"
#include "../gui/server_frame.h"
#include "../gui/schedule_list.h"
#include "../gui/themeselector.h"
#include "../gui/scenario_frame.h"
#include "../gui/scenario_info.h"
#include "../gui/depotlist_frame.h"
#include "../gui/vehiclelist_frame.h"
#include "../gui/script_tool_frame.h"

#include "../obj/baum.h"
#include "../obj/groundobj.h"


class player_t;

/********************** dialog tools *****************************/

// general help
class dialog_help_t : public tool_t {
public:
	dialog_help_t() : tool_t(DIALOG_HELP | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Help"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_mainhelp); }
	bool init(player_t*) OVERRIDE{
		help_frame_t::open_help_on("general.txt");
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_mainhelp); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

// open info/quit dialog
class dialog_options_t : public tool_t {
public:
	dialog_options_t() : tool_t(DIALOG_OPTIONS | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Einstellungen aendern"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_optionen_gui_t); }
	bool init(player_t*) OVERRIDE{
		create_win(-1, -1, new optionen_gui_t(), w_info, magic_optionen_gui_t, true);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_optionen_gui_t); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

// open minimap
class dialog_minimap_t : public tool_t {
public:
	dialog_minimap_t() : tool_t(DIALOG_MINIMAP | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Reliefkarte"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_reliefmap); }
	bool init(player_t*) OVERRIDE{
		create_win(new map_frame_t(), w_info, magic_reliefmap);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_reliefmap); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

// open line management
class dialog_lines_t : public tool_t {
public:
	dialog_lines_t() : tool_t(DIALOG_LINEOVERVIEW | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Line Management"); }
	image_id get_icon(player_t* player) const OVERRIDE{ return !player || player->get_player_nr() == PUBLIC_PLAYER_NR ? IMG_EMPTY : icon; }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_line_management_t + welt->get_active_player_nr()); }
	bool init(player_t* player) OVERRIDE{
		if (!player->is_public_service()) {
			create_win(new schedule_list_gui_t(player), w_info, magic_line_management_t + player->get_player_nr());
		}
		return false;
	}
	bool exit(player_t* const player) OVERRIDE{ destroy_win(win_get_magic(magic_line_management_t + player->get_player_nr())); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

// open messages
class dialog_messages_t : public tool_t {
public:
	dialog_messages_t() : tool_t(DIALOG_MESSAGES | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Mailbox"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_messageframe); }
	bool init(player_t*) OVERRIDE{
		create_win(new message_frame_t(), w_info, magic_messageframe);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_messageframe); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

// open finance window
class dialog_finances_t : public tool_t {
public:
	dialog_finances_t() : tool_t(DIALOG_FINANCES | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Finanzen"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_finances_t + welt->get_active_player_nr()); }
	bool init(player_t* player) OVERRIDE{
		create_win(new money_frame_t(player), w_info, magic_finances_t + player->get_player_nr());
		return false;
	}
	bool exit(player_t* const player) OVERRIDE{ destroy_win(magic_finances_t + player->get_player_nr()); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

// open player dialog
class dialog_players_t : public tool_t {
public:
	dialog_players_t() : tool_t(DIALOG_PLAYERS | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Spielerliste"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_ki_kontroll_t); }
	bool init(player_t*) OVERRIDE{
		create_win(272, 160, new ki_kontroll_t(), w_info, magic_ki_kontroll_t);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_ki_kontroll_t); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

// open display options
class dialog_displayoptions_t : public tool_t {
public:
	dialog_displayoptions_t() : tool_t(DIALOG_DISPLAYOPTIONS | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Display settings"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_color_gui_t); }
	bool init(player_t*) OVERRIDE{
		create_win(new color_gui_t(), w_info, magic_color_gui_t);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_color_gui_t); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

// open sound dialog
class dialog_sound_t : public tool_t {
public:
	dialog_sound_t() : tool_t(DIALOG_SOUND | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Sound"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_sound_kontroll_t); }
	bool init(player_t*) OVERRIDE{
		create_win(new sound_frame_t(), w_info, magic_sound_kontroll_t);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_sound_kontroll_t); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

// open language dialog
class dialog_language_t : public tool_t {
public:
	dialog_language_t() : tool_t(DIALOG_LANGUAGE | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Sprache"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_sprachengui_t); }
	bool init(player_t*) OVERRIDE{
		create_win(new sprachengui_t(), w_info, magic_sprachengui_t);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_sprachengui_t); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

// open player color dialog
class dialog_playercolor_t : public tool_t {
public:
	dialog_playercolor_t() : tool_t(DIALOG_PLAYERCOLOR | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Farbe"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_farbengui_t); }
	bool init(player_t* player) OVERRIDE{
		create_win(new farbengui_t(player), w_info, magic_farbengui_t);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_farbengui_t); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

// jump to position dialog
class dialog_jump_t : public tool_t {
public:
	dialog_jump_t() : tool_t(DIALOG_JUMP | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Jump to"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_jump); }
	bool init(player_t*) OVERRIDE{
		create_win(new jump_frame_t(), w_info, magic_jump);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_jump); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

// load game dialog
class dialog_load_t : public tool_t {
public:
	dialog_load_t() : tool_t(DIALOG_LOAD | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Laden"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_load_t); }
	bool init(player_t*) OVERRIDE{
		if(  !env_t::server  ) {
			destroy_all_win(true);
			create_win(new loadsave_frame_t(true), w_info, magic_load_t);
		}
		else {
			destroy_win(magic_save_t);
			create_win( new loadsave_frame_t(true), w_info, magic_load_t);
			scr_coord pos = win_get_pos( win_get_magic(magic_load_t) );
			create_win( pos.x+20, pos.y+20, new news_img("Loading a new game will end the current server session!"), w_no_overlap, magic_none);
		}
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_load_t); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
};

// save game dialog
class dialog_save_t : public tool_t {
public:
	dialog_save_t() : tool_t(DIALOG_SAVE | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Speichern"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_save_t); }
	bool init(player_t*) OVERRIDE{
		create_win(new loadsave_frame_t(false), w_info, magic_save_t);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_save_t); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
};

// open scripted-tools dialog
class dialog_script_tool_t : public tool_t {
public:
	dialog_script_tool_t() : tool_t(DIALOG_SCRIPT_TOOL | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Load tool script"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_load_t); }
	bool init(player_t*) OVERRIDE{
		destroy_win(magic_save_t);
		create_win( new script_tool_frame_t(), w_info, magic_load_t );
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_load_t); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
};

// open scenario dialog
class dialog_scenario_t : public tool_t {
public:
	dialog_scenario_t() : tool_t(DIALOG_SCENARIO | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Load scenario"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_load_t); }
	bool init(player_t*) OVERRIDE{
		destroy_win(magic_save_t);
		if(  !env_t::server  ) {
			destroy_all_win(true);
			create_win( new scenario_frame_t(), w_info, magic_load_t );
		}
		else {
			create_win( new scenario_frame_t(), w_info, magic_load_t );
			scr_coord pos = win_get_pos( win_get_magic(magic_load_t) );
			create_win( pos.x+20, pos.y+20, new news_img("Loading a new game will end the current server session!"), w_no_overlap, magic_none);
		}
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_load_t); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
};

// open scenario info dialog
class dialog_scenario_info_t : public tool_t {
public:
	dialog_scenario_info_t() : tool_t(DIALOG_SCENARIO_INFO | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Scenario"); }
	image_id get_icon(player_t *) const OVERRIDE { return world()->get_scenario()->is_scripted() ? icon : IMG_EMPTY; }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_scenario_info); }
	bool init(player_t*) OVERRIDE{
		create_win( new scenario_info_t(), w_info, magic_scenario_info );
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_scenario_info); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
};

/* open the list of halt */
class dialog_list_halt_t : public tool_t {
public:
	dialog_list_halt_t() : tool_t(DIALOG_LIST_HALT | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("hl_title"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_halt_list + welt->get_active_player_nr()); }
	bool init(player_t* player) OVERRIDE{
		create_win(new halt_list_frame_t(), w_info, magic_halt_list + player->get_player_nr());
		return false;
	}
	bool exit(player_t* player) OVERRIDE{ destroy_win(magic_halt_list + player->get_player_nr()); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

/* open the list of vehicle */
class dialog_list_convoi_t : public tool_t {
public:
	dialog_list_convoi_t() : tool_t(DIALOG_LIST_CONVOI | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("cl_title"); }
	image_id get_icon(player_t*) const OVERRIDE{ return welt->get_active_player_nr() == PUBLIC_PLAYER_NR ? IMG_EMPTY : icon; }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_convoi_list + welt->get_active_player_nr()); }
	bool init(player_t* player) OVERRIDE{
		create_win(new convoi_frame_t(), w_info, magic_convoi_list + player->get_player_nr());
		return false;
	}
	bool exit(player_t* const player) OVERRIDE{ destroy_win(magic_convoi_list + player->get_player_nr()); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

/* open the list of depots */
class dialog_list_depot_t : public tool_t {
public:
	dialog_list_depot_t() : tool_t(DIALOG_LIST_DEPOT | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("dp_title"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_depotlist + welt->get_active_player_nr()); }
	image_id get_icon(player_t*) const OVERRIDE{ return welt->get_active_player_nr() == PUBLIC_PLAYER_NR ? IMG_EMPTY : icon; }
	bool init(player_t* player) OVERRIDE{
		create_win(new depotlist_frame_t(player), w_info, magic_depotlist + player->get_player_nr());
		return false;
	}
	bool exit(player_t* player) OVERRIDE{ destroy_win(magic_depotlist + player->get_player_nr()); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

/* open the list of vehicles */
class dialog_list_vehicle_t : public tool_t {
public:
	dialog_list_vehicle_t() : tool_t(DIALOG_LIST_VEHICLE | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("vh_title"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_vehiclelist); }
	bool init(player_t*) OVERRIDE{
		create_win(new vehiclelist_frame_t(), w_info, magic_vehiclelist);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_vehiclelist); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

/* open the list of towns */
class dialog_list_town_t : public tool_t {
public:
	dialog_list_town_t() : tool_t(DIALOG_LIST_TOWN | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("tl_title"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_citylist_frame_t); }
	bool init(player_t*) OVERRIDE{
		create_win(new citylist_frame_t(), w_info, magic_citylist_frame_t);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_citylist_frame_t); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

/* open the list of goods */
class dialog_list_goods_t : public tool_t {
public:
	dialog_list_goods_t() : tool_t(DIALOG_LIST_GOODS | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("gl_title"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_goodslist); }
	bool init(player_t*) OVERRIDE{
		create_win(new goods_frame_t(), w_info, magic_goodslist);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_goodslist); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

/* open the list of factories */
class dialog_list_factory_t : public tool_t {
public:
	dialog_list_factory_t() : tool_t(DIALOG_LIST_FACTORY | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("fl_title"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_factorylist); }
	bool init(player_t*) OVERRIDE{
		create_win(new factorylist_frame_t(), w_info, magic_factorylist);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_factorylist); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

/* open the list of attraction */
class dialog_list_curiosity_t : public tool_t {
public:
	dialog_list_curiosity_t() : tool_t(DIALOG_LIST_CURIOSITY | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("curlist_title"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_curiositylist); }
	bool init(player_t*) OVERRIDE{
		create_win(new curiositylist_frame_t(), w_info, magic_curiositylist);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_curiositylist); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

/* factory building dialog */
class dialog_edit_factory_t : public tool_t {
public:
	dialog_edit_factory_t() : tool_t(DIALOG_EDIT_FACTORY | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("factorybuilder"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_edit_factory); }
	bool init(player_t* player) OVERRIDE{
		if (!is_selected()) {
			create_win(new factory_edit_frame_t(player), w_info, magic_edit_factory);
		}
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_edit_factory); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

/* attraction building dialog */
class dialog_edit_attraction_t : public tool_t {
public:
	dialog_edit_attraction_t() : tool_t(DIALOG_EDIT_ATTRACTION | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("curiosity builder"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_edit_attraction); }
	bool init(player_t* player) OVERRIDE{
		if (!is_selected()) {
			create_win(new curiosity_edit_frame_t(player), w_info, magic_edit_attraction);
		}
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_edit_attraction); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

/* house building dialog */
class dialog_edit_house_t : public tool_t {
public:
	dialog_edit_house_t() : tool_t(DIALOG_EDIT_HOUSE | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("citybuilding builder"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_edit_house); }
	bool init(player_t* player) OVERRIDE{
		if (!is_selected()) {
			create_win(new citybuilding_edit_frame_t(player), w_info, magic_edit_house);
		}
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_edit_house); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

/* tree placing dialog */
class dialog_edit_tree_t : public tool_t {
public:
	dialog_edit_tree_t() : tool_t(DIALOG_EDIT_TREE | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("baum builder"); }
	image_id get_icon(player_t *) const OVERRIDE { return tree_builder_t::has_trees() ? icon : IMG_EMPTY; }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_edit_tree); }
	bool init(player_t* player) OVERRIDE{
		if(  tree_builder_t::has_trees()  && !is_selected()  ) {
			create_win(new baum_edit_frame_t(player), w_info, magic_edit_tree);
		}
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_edit_tree); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

/* groundobj placing dialog */
class dialog_edit_groundobj_t : public tool_t {
public:
	dialog_edit_groundobj_t() : tool_t(DIALOG_EDIT_GROUNDOBJ | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("groundobj builder"); }
	image_id get_icon(player_t *) const OVERRIDE { return groundobj_t::get_count() > 0 ? icon : IMG_EMPTY; }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_edit_groundobj); }
	bool init(player_t* player) OVERRIDE{
		if (!is_selected()) {
			create_win(new groundobj_edit_frame_t(player), w_info, magic_edit_groundobj);
		}
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_edit_groundobj); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

// to increase map-size
class dialog_enlarge_map_t : public tool_t{
public:
	dialog_enlarge_map_t() : tool_t(DIALOG_ENLARGE_MAP | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return env_t::networkmode ? translator::translate("deactivated in online mode") : translator::translate("enlarge map"); }
	image_id get_icon(player_t *) const OVERRIDE { return env_t::networkmode ? IMG_EMPTY : icon; }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_bigger_map); }
	bool init(player_t*) OVERRIDE{
		if (!env_t::networkmode) {
			destroy_all_win(true);
			create_win(new enlarge_map_frame_t(), w_info, magic_bigger_map);
		}
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_bigger_map); return false; }
};

/* open the list of label */
class dialog_list_label_t : public tool_t {
public:
	dialog_list_label_t() : tool_t(DIALOG_LIST_LABEL | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("labellist_title"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_labellist); }
	bool init(player_t*) OVERRIDE{
		create_win(new labellist_frame_t(), w_info, magic_labellist);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_labellist); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

/* open climate settings */
class dialog_climates_t : public tool_t {
public:
	dialog_climates_t() : tool_t(DIALOG_CLIMATES | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return (!env_t::networkmode || env_t::server) ? translator::translate("Climate Control") : translator::translate("deactivated in online mode"); }
	image_id get_icon(player_t *) const OVERRIDE { return (!env_t::networkmode || env_t::server) ? icon : IMG_EMPTY; }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_climate); }
	bool init(player_t*) OVERRIDE{
		if (!env_t::networkmode || env_t::server) {
			create_win(new climate_gui_t(&welt->get_settings()), w_info, magic_climate);
		}
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_climate); return false; }
};

/* open all game settings */
class dialog_settings_t : public tool_t {
public:
	dialog_settings_t() : tool_t(DIALOG_SETTINGS | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return (!env_t::networkmode || env_t::server) ? translator::translate("Setting") : translator::translate("deactivated in online mode"); }
	image_id get_icon(player_t *) const OVERRIDE { return (!env_t::networkmode || env_t::server) ? icon : IMG_EMPTY; }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_settings_frame_t); }
	bool init(player_t*) OVERRIDE{
		if (!env_t::networkmode || env_t::server) {
			create_win(new settings_frame_t(&welt->get_settings()), w_info, magic_settings_frame_t);
		}
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_settings_frame_t); return false; }
};

/* server info and join dialog */
class dialog_gameinfo_t : public tool_t {
public:
	dialog_gameinfo_t() : tool_t(DIALOG_GAMEINFO | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Game info"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_server_frame_t); }
	bool init(player_t*) OVERRIDE{
		create_win(new server_frame_t(), w_info, magic_server_frame_t);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_server_frame_t); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};

/* open themes selector settings */
class dialog_themes_t : public tool_t {
public:
	dialog_themes_t() : tool_t(DIALOG_THEMES | DIALOGE_TOOL) {}
	char const* get_tooltip(player_t const*) const OVERRIDE{ return translator::translate("Select a theme for display"); }
	bool is_selected() const OVERRIDE{ return win_get_magic(magic_themes); }
	bool init(player_t*) OVERRIDE{
		create_win(new themeselector_t(), w_info, magic_themes);
		return false;
	}
	bool exit(player_t*) OVERRIDE{ destroy_win(magic_themes); return false; }
	bool is_init_network_safe() const OVERRIDE{ return true; }
	bool is_work_network_safe() const OVERRIDE{ return true; }
};
#endif
