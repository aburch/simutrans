/*
 * New OO tool system
 *
 * contains tools that will open windows
 * (split from simwerkz.h)
 *
 * This file is part of the Simutrans project under the artistic license.
 */

#ifndef simwerkz_dialogs_h
#define simwerkz_dialogs_h

#include "simmenu.h"
#include "simwin.h"

#include "dataobj/translator.h"

#include "gui/factory_edit.h"
#include "gui/curiosity_edit.h"
#include "gui/citybuilding_edit.h"
#include "gui/baum_edit.h"
#include "gui/jump_frame.h"
#include "gui/optionen.h"
#include "gui/map_frame.h"
#include "gui/display_settings.h"
#include "gui/player_frame_t.h"
#include "gui/loadsave_frame.h"
#include "gui/money_frame.h"
#include "gui/schedule_list.h"
#include "gui/sound_frame.h"
#include "gui/sprachen.h"
#include "gui/kennfarbe.h"
#include "gui/help_frame.h"
#include "gui/message_frame_t.h"
#include "gui/messagebox.h"
#include "gui/convoi_frame.h"
#include "gui/halt_list_frame.h"
#include "gui/citylist_frame_t.h"
#include "gui/goods_frame_t.h"
#include "gui/factorylist_frame_t.h"
#include "gui/curiositylist_frame_t.h"
#include "gui/enlarge_map_frame_t.h"
#include "gui/labellist_frame_t.h"
#include "gui/climates.h"
#include "gui/settings_frame.h"
#include "gui/server_frame.h"
#include "gui/schedule_list.h"

class spieler_t;

/********************** dialoge tools *****************************/

// general help
class wkz_help_t : public werkzeug_t {
public:
	wkz_help_t() : werkzeug_t(WKZ_HELP | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Help"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_mainhelp); }
	bool init(karte_t*, spieler_t*) OVERRIDE {
		help_frame_t::open_help_on( "general.txt" );
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_mainhelp); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// open info/quit dialoge
class wkz_optionen_t : public werkzeug_t {
public:
	wkz_optionen_t() : werkzeug_t(WKZ_OPTIONEN | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Einstellungen aendern"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_optionen_gui_t); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		create_win(240, 120, new optionen_gui_t(welt), w_info, magic_optionen_gui_t);
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_optionen_gui_t); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// open minimap
class wkz_minimap_t : public werkzeug_t {
public:
	wkz_minimap_t() : werkzeug_t(WKZ_MINIMAP | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Reliefkarte"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_reliefmap); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		create_win( new map_frame_t(welt), w_info, magic_reliefmap);
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_reliefmap); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// open line management
class wkz_lines_t : public werkzeug_t {
public:
	wkz_lines_t() : werkzeug_t(WKZ_LINEOVERVIEW | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Line Management"); }
	image_id get_icon(spieler_t* sp) const OVERRIDE { return sp->get_player_nr() == 1 ? IMG_LEER : icon; }
	bool is_selected(karte_t const* welt) const OVERRIDE { return win_get_magic(magic_line_management_t + welt->get_active_player_nr()); }
	bool init(karte_t*, spieler_t* sp) OVERRIDE {
		if(sp->get_player_nr()!=1) {
			sp->simlinemgmt.line_management_window( sp );
		}
		return false;
	}
	bool exit(karte_t*, spieler_t* const sp) OVERRIDE { destroy_win(win_get_magic(magic_line_management_t + sp->get_player_nr())); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// open messages
class wkz_messages_t : public werkzeug_t {
public:
	wkz_messages_t() : werkzeug_t(WKZ_MESSAGES | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Mailbox"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_messageframe); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		create_win( new message_frame_t(welt), w_info, magic_messageframe );
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_messageframe); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// open finance window
class wkz_finances_t : public werkzeug_t {
public:
	wkz_finances_t() : werkzeug_t(WKZ_FINANCES | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Finanzen"); }
	bool is_selected(karte_t const* welt) const OVERRIDE { return win_get_magic(magic_finances_t+welt->get_active_player_nr()); }
	bool init(karte_t*, spieler_t* sp) OVERRIDE {
		create_win( new money_frame_t(sp), w_info, magic_finances_t+sp->get_player_nr() );
		return false;
	}
	bool exit(karte_t*, spieler_t* const sp) OVERRIDE { destroy_win(magic_finances_t + sp->get_player_nr()); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// open player dialoge
class wkz_players_t : public werkzeug_t {
public:
	wkz_players_t() : werkzeug_t(WKZ_PLAYERS | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Spielerliste"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_ki_kontroll_t); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		create_win( 272, 160, new ki_kontroll_t(welt), w_info, magic_ki_kontroll_t);
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_ki_kontroll_t); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// open display options
class wkz_displayoptions_t : public werkzeug_t {
public:
	wkz_displayoptions_t() : werkzeug_t(WKZ_DISPLAYOPTIONS | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Helligk."); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_color_gui_t); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		create_win(new color_gui_t(welt), w_info, magic_color_gui_t);
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_color_gui_t); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// open sound dialoge
class wkz_sound_t : public werkzeug_t {
public:
	wkz_sound_t() : werkzeug_t(WKZ_SOUND | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Sound"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_sound_kontroll_t); }
	bool init(karte_t*, spieler_t*) OVERRIDE {
		create_win(new sound_frame_t(), w_info, magic_sound_kontroll_t);
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_sound_kontroll_t); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// open language dialoge
class wkz_language_t : public werkzeug_t {
public:
	wkz_language_t() : werkzeug_t(WKZ_LANGUAGE | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Sprache"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_sprachengui_t); }
	bool init(karte_t*, spieler_t*) OVERRIDE {
		create_win(new sprachengui_t(), w_info, magic_sprachengui_t);
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_sprachengui_t); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// open player color dialoge
class wkz_playercolor_t : public werkzeug_t {
public:
	wkz_playercolor_t() : werkzeug_t(WKZ_PLAYERCOLOR | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Farbe"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_farbengui_t); }
	bool init(karte_t*, spieler_t* sp) OVERRIDE {
		create_win(new farbengui_t(sp), w_info, magic_farbengui_t);
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_farbengui_t); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// jump to position dialoge
class wkz_jump_t : public werkzeug_t {
public:
	wkz_jump_t() : werkzeug_t(WKZ_JUMP | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Jump to"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_jump); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		create_win( new jump_frame_t(welt), w_info, magic_jump);
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_jump); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// load game dialoge
class wkz_load_t : public werkzeug_t {
public:
	wkz_load_t() : werkzeug_t(WKZ_LOAD | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Laden"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_load_t); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		destroy_all_win( true );
		create_win(new loadsave_frame_t(welt, true), w_info, magic_load_t);
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_load_t); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
};

// save game dialoge
class wkz_save_t : public werkzeug_t {
public:
	wkz_save_t() : werkzeug_t(WKZ_SAVE | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Speichern"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_save_t); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		create_win(new loadsave_frame_t(welt, false), w_info, magic_save_t);
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_save_t); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
};

/* open the list of halt */
class wkz_list_halt_t : public werkzeug_t {
public:
	wkz_list_halt_t() : werkzeug_t(WKZ_LIST_HALT | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("hl_title"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_halt_list_t); }
	bool init(karte_t*, spieler_t* sp) OVERRIDE {
		create_win( new halt_list_frame_t(sp), w_info, magic_halt_list_t );
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_halt_list_t); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

/* open the list of vehicle */
class wkz_list_convoi_t : public werkzeug_t {
public:
	wkz_list_convoi_t() : werkzeug_t(WKZ_LIST_CONVOI | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("cl_title"); }
	bool is_selected(karte_t const* const welt) const OVERRIDE { return win_get_magic(magic_convoi_list + welt->get_active_player_nr()); }
	bool init(karte_t*, spieler_t* sp) OVERRIDE {
		create_win( new convoi_frame_t(sp), w_info, magic_convoi_list+sp->get_player_nr() );
		return false;
	}
	bool exit(karte_t*, spieler_t* const sp) OVERRIDE { destroy_win(magic_convoi_list + sp->get_player_nr()); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

/* open the list of towns */
class wkz_list_town_t : public werkzeug_t {
public:
	wkz_list_town_t() : werkzeug_t(WKZ_LIST_TOWN | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("tl_title"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_citylist_frame_t); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		create_win( new citylist_frame_t(welt), w_info, magic_citylist_frame_t );
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_citylist_frame_t); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

/* open the list of goods */
class wkz_list_goods_t : public werkzeug_t {
public:
	wkz_list_goods_t() : werkzeug_t(WKZ_LIST_GOODS | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("gl_title"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_goodslist); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		create_win( new goods_frame_t(welt), w_info, magic_goodslist );
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_goodslist); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

/* open the list of factories */
class wkz_list_factory_t : public werkzeug_t {
public:
	wkz_list_factory_t() : werkzeug_t(WKZ_LIST_FACTORY | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("fl_title"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_factorylist); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		create_win( new factorylist_frame_t(welt), w_info, magic_factorylist );
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_factorylist); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

/* open the list of attraction */
class wkz_list_curiosity_t : public werkzeug_t {
public:
	wkz_list_curiosity_t() : werkzeug_t(WKZ_LIST_CURIOSITY | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("curlist_title"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_curiositylist); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		create_win( new curiositylist_frame_t(welt), w_info, magic_curiositylist );
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_curiositylist); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

/* factory building dialog */
class wkz_factorybuilder_t : public werkzeug_t {
public:
	wkz_factorybuilder_t() : werkzeug_t(WKZ_EDIT_FACTORY | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("factorybuilder"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_edit_factory); }
	bool init(karte_t* welt, spieler_t* sp) OVERRIDE {
		if (!is_selected(welt)) {
			create_win( new factory_edit_frame_t(sp,welt), w_info, magic_edit_factory );
		}
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_edit_factory); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

/* attraction building dialog */
class wkz_attractionbuilder_t : public werkzeug_t {
public:
	wkz_attractionbuilder_t() : werkzeug_t(WKZ_EDIT_ATTRACTION | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("curiosity builder"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_edit_attraction); }
	bool init(karte_t* welt, spieler_t* sp) OVERRIDE {
		if (!is_selected(welt)) {
			create_win( new curiosity_edit_frame_t(sp,welt), w_info, magic_edit_attraction );
		}
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_edit_attraction); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

/* house building dialog */
class wkz_housebuilder_t : public werkzeug_t {
public:
	wkz_housebuilder_t() : werkzeug_t(WKZ_EDIT_HOUSE | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("citybuilding builder"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_edit_house); }
	bool init(karte_t* welt, spieler_t* sp) OVERRIDE {
		if (!is_selected(welt)) {
			create_win( new citybuilding_edit_frame_t(sp,welt), w_info, magic_edit_house );
		}
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_edit_house); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

/* house building dialog */
class wkz_treebuilder_t : public werkzeug_t {
public:
	wkz_treebuilder_t() : werkzeug_t(WKZ_EDIT_TREE | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("baum builder"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_edit_tree); }
	bool init(karte_t* welt, spieler_t* sp) OVERRIDE {
		if (!is_selected(welt)) {
			create_win( new baum_edit_frame_t(sp,welt), w_info, magic_edit_tree );
		}
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_edit_tree); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// to increase map-size
class wkz_enlarge_map_t : public werkzeug_t{
public:
	wkz_enlarge_map_t() : werkzeug_t(WKZ_ENLARGE_MAP | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return umgebung_t::networkmode ? translator::translate("deactivated in online mode") : translator::translate("enlarge map"); }
	image_id get_icon(spieler_t *) const { return umgebung_t::networkmode ? IMG_LEER : icon; }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_bigger_map); }
	bool init(karte_t* welt, spieler_t* sp) OVERRIDE {
		if(  !umgebung_t::networkmode  ) {
			destroy_all_win( true );
			create_win( new enlarge_map_frame_t(sp,welt), w_info, magic_bigger_map );
		}
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_bigger_map); return false; }
};

/* open the list of label */
class wkz_list_label_t : public werkzeug_t {
public:
	wkz_list_label_t() : werkzeug_t(WKZ_LIST_LABEL | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("labellist_title"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_labellist); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		create_win( new labellist_frame_t(welt), w_info, magic_labellist );
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_labellist); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

/* open climate settings */
class wkz_climates_t : public werkzeug_t {
public:
	wkz_climates_t() : werkzeug_t(WKZ_CLIMATES | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return (!umgebung_t::networkmode  ||  umgebung_t::server) ? translator::translate("Climate Control") : translator::translate("deactivated in online mode"); }
	image_id get_icon(spieler_t *) const { return (!umgebung_t::networkmode  ||  umgebung_t::server) ? icon : IMG_LEER; }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_climate); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		if(  !umgebung_t::networkmode  ||  umgebung_t::server  ) {
			create_win(new climate_gui_t(&welt->get_settings()), w_info, magic_climate);
		}
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_climate); return false; }
};

/* open all game settings */
class wkz_settings_t : public werkzeug_t {
public:
	wkz_settings_t() : werkzeug_t(WKZ_SETTINGS | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return (!umgebung_t::networkmode  ||  umgebung_t::server) ? translator::translate("Setting") : translator::translate("deactivated in online mode"); }
	image_id get_icon(spieler_t *) const { return (!umgebung_t::networkmode  ||  umgebung_t::server) ? icon : IMG_LEER; }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_settings_frame_t); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		if(  !umgebung_t::networkmode  ||  umgebung_t::server  ) {
			create_win(new settings_frame_t(&welt->get_settings()), w_info, magic_settings_frame_t);
		}
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_settings_frame_t); return false; }
};

/* server info and join dialoge */
class wkz_server_t : public werkzeug_t {
public:
	wkz_server_t() : werkzeug_t(WKZ_GAMEINFO | DIALOGE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Game info"); }
	bool is_selected(karte_t const*) const OVERRIDE { return win_get_magic(magic_server_frame_t); }
	bool init(karte_t* welt, spieler_t*) OVERRIDE {
		create_win( new server_frame_t(welt), w_info, magic_server_frame_t );
		return false;
	}
	bool exit(karte_t*, spieler_t*) OVERRIDE { destroy_win(magic_server_frame_t); return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};
#endif
