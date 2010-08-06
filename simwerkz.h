/*
 * New OO tool system
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef simwerkz2_h
#define simwerkz2_h

#include "simtypes.h"
#include "simskin.h"
#include "simsound.h"
#include "simworld.h"
#include "simmenu.h"
#include "simwin.h"

#include "besch/way_obj_besch.h"

#include "bauer/fabrikbauer.h"

#include "dataobj/umgebung.h"
#include "dataobj/translator.h"

#include "dings/baum.h"

#include "gui/factory_edit.h"
#include "gui/curiosity_edit.h"
#include "gui/citybuilding_edit.h"
#include "gui/baum_edit.h"
#include "gui/jump_frame.h"
#include "gui/optionen.h"
#include "gui/map_frame.h"
#include "gui/colors.h"
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

#include "tpl/slist_tpl.h"

class spieler_t;
class koord3d;
class koord;
class wegbauer_t;
class weg_besch_t;

/****************************** helper functions: *****************************/

char *tooltip_with_price(const char * tip, sint64 price);

/************************ general tool *******************************/

// query tile info: default tool
class wkz_abfrage_t : public werkzeug_t {
public:
	wkz_abfrage_t() : werkzeug_t() { id = WKZ_ABFRAGE | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Abfrage"); }
	const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};


// remove uppermost object from tile
class wkz_remover_t : public werkzeug_t {
private:
	static bool wkz_remover_intern(spieler_t *sp, karte_t *welt, koord3d pos, const char *&msg);
public:
	wkz_remover_t() : werkzeug_t() { id = WKZ_REMOVER | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Abriss"); }
	const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

// alter land height tools
class wkz_raise_t : public werkzeug_t {
private:
	bool is_dragging;
	sint16 drag_height;
public:
	wkz_raise_t() : werkzeug_t() { offset = Z_GRID; id = WKZ_RAISE_LAND | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *sp) { return tooltip_with_price("Anheben", sp->get_welt()->get_einstellungen()->cst_alter_land); }
	virtual image_id get_icon(spieler_t *) const { return grund_t::underground_mode==grund_t::ugm_all ? IMG_LEER : icon; }
	bool init( karte_t *, spieler_t * ) { is_dragging = false; return true; }
	bool exit( karte_t *, spieler_t * ) { is_dragging = false; return true; }
	const char *check( karte_t *, spieler_t *, koord3d );
	const char *work( karte_t *, spieler_t *, koord3d );
	const char *move( karte_t *, spieler_t *, uint16 /* buttonstate */, koord3d );
};

class wkz_lower_t : public werkzeug_t {
private:
	bool is_dragging;
	sint16 drag_height;
public:
	wkz_lower_t() : werkzeug_t() { offset = Z_GRID; id = WKZ_LOWER_LAND | GENERAL_TOOL;  }
	const char *get_tooltip(spieler_t *sp) { return tooltip_with_price("Absenken", sp->get_welt()->get_einstellungen()->cst_alter_land); }
	virtual image_id get_icon(spieler_t *) const { return grund_t::underground_mode==grund_t::ugm_all ? IMG_LEER : icon; }
	bool init( karte_t *, spieler_t * ) { is_dragging = false; return true; }
	bool exit( karte_t *, spieler_t * ) { is_dragging = false; return true; }
	const char *check( karte_t *, spieler_t *, koord3d );
	const char *work( karte_t *, spieler_t *, koord3d);
	const char *move( karte_t *, spieler_t *, uint16 /* buttonstate */, koord3d );
};

/* slope tool definitions */
class wkz_setslope_t : public werkzeug_t {
public:
	wkz_setslope_t() : werkzeug_t() { id = WKZ_SETSLOPE | GENERAL_TOOL; }
	static const char *wkz_set_slope_work( karte_t *welt, spieler_t *sp, koord3d pos, int slope );
	const char *get_tooltip(spieler_t *sp) { return tooltip_with_price("Built artifical slopes", sp->get_welt()->get_einstellungen()->cst_set_slope); }
	virtual bool is_init_network_save() const { return true; }
	const char *check( karte_t *, spieler_t *, koord3d );
	virtual const char *work( karte_t *welt, spieler_t *sp, koord3d k ) { return wkz_set_slope_work( welt, sp, k, atoi(default_param) ); }
};

class wkz_restoreslope_t : public werkzeug_t {
public:
	wkz_restoreslope_t() : werkzeug_t() { id = WKZ_RESTORESLOPE | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *sp) { return tooltip_with_price("Restore natural slope", sp->get_welt()->get_einstellungen()->cst_set_slope); }
	virtual bool is_init_network_save() const { return true; }
	const char *check( karte_t *, spieler_t *, koord3d );
	virtual const char *work( karte_t *welt, spieler_t *sp, koord3d k ) { return wkz_setslope_t::wkz_set_slope_work( welt, sp, k, RESTORE_SLOPE ); }
};

class wkz_marker_t : public kartenboden_werkzeug_t {
public:
	wkz_marker_t() : kartenboden_werkzeug_t() { id = WKZ_MARKER | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *sp) { return tooltip_with_price("Marker", sp->get_welt()->get_einstellungen()->cst_buy_land); }
	virtual const char *work( karte_t *welt, spieler_t *sp, koord3d k );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_clear_reservation_t : public werkzeug_t {
public:
	wkz_clear_reservation_t() : werkzeug_t() { id = WKZ_CLEAR_RESERVATION | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Clear block reservation"); }
	bool init( karte_t *, spieler_t * );
	bool exit( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_transformer_t : public kartenboden_werkzeug_t {
public:
	wkz_transformer_t() : kartenboden_werkzeug_t() { id = WKZ_TRANSFORMER | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *);
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_add_city_t : public kartenboden_werkzeug_t {
public:
	wkz_add_city_t() : kartenboden_werkzeug_t() { id = WKZ_ADD_CITY | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *sp) { return tooltip_with_price( "Found new city", sp->get_welt()->get_einstellungen()->cst_found_city ); }
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

// buy a house to protext it from renovating
class wkz_buy_house_t : public kartenboden_werkzeug_t {
public:
	wkz_buy_house_t() : kartenboden_werkzeug_t() { id = WKZ_BUY_HOUSE | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Haus kaufen"); }
	const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};
/************** the following tools need a valid default_param ************************/

// step size by default_param
class wkz_change_city_size_t : public werkzeug_t {
public:
	wkz_change_city_size_t() : werkzeug_t() { id = WKZ_CHANGE_CITY_SIZE | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate( atoi(default_param)>=0 ? "Grow city" : "Shrink city" ); }
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_plant_tree_t : public kartenboden_werkzeug_t {
public:
	wkz_plant_tree_t() : kartenboden_werkzeug_t() { id = WKZ_PLANT_TREE | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate( "Plant tree" ); }
	virtual const char *move( karte_t *welt, spieler_t *sp, uint16 b, koord3d k ) { return b==1 ? work(welt,sp,k) : NULL; }
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

/* only called directly from schedule => no tooltip!
 * default_param must point to a schedule!
 */
class wkz_fahrplan_add_t : public werkzeug_t {
public:
	wkz_fahrplan_add_t() : werkzeug_t() { id = WKZ_FAHRPLAN_ADD | GENERAL_TOOL; }
	virtual const char *work( karte_t *welt, spieler_t *sp, koord3d k );
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_fahrplan_ins_t : public werkzeug_t {
public:
	wkz_fahrplan_ins_t() : werkzeug_t() { id = WKZ_FAHRPLAN_INS | GENERAL_TOOL; }
	virtual const char *work( karte_t *welt, spieler_t *sp, koord3d k );
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_wegebau_t : public two_click_werkzeug_t {
private:
	static const weg_besch_t *defaults[17];	// default ways for all types
protected:
	const weg_besch_t *besch;

public:
	wkz_wegebau_t() : two_click_werkzeug_t(), besch(NULL) { id = WKZ_WEGEBAU | GENERAL_TOOL; }
	virtual image_id get_icon(spieler_t *) const;
	virtual const char *get_tooltip(spieler_t *);
	virtual const char* get_default_param() const;
	virtual bool is_selected( karte_t *welt ) const;
	virtual bool init( karte_t *, spieler_t * );
	virtual bool is_move_network_save(spieler_t *sp) const { return two_click_werkzeug_t::is_move_network_save(sp) && (besch  &&  besch->get_styp()!=1); }
protected:
	const weg_besch_t *get_besch(uint16,bool) const;
	void calc_route( wegbauer_t &bauigel, const koord3d &, const koord3d & );
private:
	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual uint8 is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & );
};

class wkz_build_cityroad : public wkz_wegebau_t {
public:
	wkz_build_cityroad() : wkz_wegebau_t() { id = WKZ_CITYROAD | GENERAL_TOOL; }
	virtual bool init( karte_t *, spieler_t * );
	virtual image_id get_icon(spieler_t *sp) const { return werkzeug_t::get_icon(sp); }
	virtual bool is_selected( karte_t *welt ) const { return werkzeug_t::is_selected(welt); }
private:
	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
};

class wkz_brueckenbau_t : public two_click_werkzeug_t {
public:
	wkz_brueckenbau_t() : two_click_werkzeug_t() { id = WKZ_BRUECKENBAU | GENERAL_TOOL; }
	virtual image_id get_icon(spieler_t *) const { return grund_t::underground_mode==grund_t::ugm_all ? IMG_LEER : icon; }
	const char *get_tooltip(spieler_t *);
	virtual bool is_move_network_save(spieler_t *) const { return false;}
private:
	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual uint8 is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & );

	ribi_t::ribi ribi;
};

class wkz_tunnelbau_t : public two_click_werkzeug_t {
public:
	wkz_tunnelbau_t() : two_click_werkzeug_t() { id = WKZ_TUNNELBAU | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *);
	virtual bool is_move_network_save(spieler_t *) const { return false;}
	const char *check( karte_t *, spieler_t *, koord3d );
private:
	void calc_route( wegbauer_t &bauigel, const koord3d &, const koord3d &, karte_t* );

	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual uint8 is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & );
};

class wkz_wayremover_t : public two_click_werkzeug_t {
private:
	bool calc_route( route_t &, spieler_t *, const koord3d& start, const koord3d &to );
public:
	wkz_wayremover_t() : two_click_werkzeug_t() { id = WKZ_WAYREMOVER | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *);
private:
	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual uint8 is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & );
};

class wkz_wayobj_t : public two_click_werkzeug_t {
protected:
	const bool build;
private:
	static const way_obj_besch_t *default_electric;
	const way_obj_besch_t *besch;
	const way_obj_besch_t *get_besch( const karte_t *welt ) const;
	waytype_t wt;

	bool calc_route( route_t &, spieler_t *, const koord3d& start, const koord3d &to );

	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual uint8 is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & );

public:
	wkz_wayobj_t(bool b=true) : two_click_werkzeug_t(), build(b) { id = WKZ_WAYOBJ | GENERAL_TOOL; };
	virtual const char *get_tooltip(spieler_t *);
	virtual bool is_selected(karte_t *welt) const;
	virtual bool init( karte_t *, spieler_t * );
};

class wkz_wayobj_remover_t : public wkz_wayobj_t {
public:
	wkz_wayobj_remover_t() : wkz_wayobj_t(false) { id = WKZ_REMOVE_WAYOBJ | GENERAL_TOOL; }
	virtual bool is_selected(karte_t *welt) const { return werkzeug_t::is_selected( welt ); }
};

class wkz_station_t : public werkzeug_t {
private:
	static char toolstring[256];
	const char *wkz_station_building_aux( karte_t *, spieler_t *, koord3d, const haus_besch_t *, sint8 rotation );
	const char *wkz_station_dock_aux( karte_t *, spieler_t *, koord3d, const haus_besch_t * );
	const char *wkz_station_aux( karte_t *, spieler_t *, koord3d, const haus_besch_t *, waytype_t, sint64 cost, const char *halt_suffix );
	const haus_besch_t *get_besch( sint8 &rotation ) const;
public:
	wkz_station_t() : werkzeug_t() { id = WKZ_STATION | GENERAL_TOOL; }
	virtual image_id get_icon(spieler_t *) const;
	const char *get_tooltip(spieler_t *);
	bool init( karte_t *, spieler_t * );
	const char *check( karte_t *, spieler_t *, koord3d );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_roadsign_t : public werkzeug_t {
public:
	wkz_roadsign_t() : werkzeug_t() { id = WKZ_ROADSIGN | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *);
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_depot_t : public werkzeug_t {
private:
	static char toolstring[256];
	const char *wkz_depot_aux(karte_t *welt, spieler_t *sp, koord3d pos, const haus_besch_t *besch, waytype_t wegtype, sint64 cost);
public:
	wkz_depot_t() : werkzeug_t() { id = WKZ_DEPOT | GENERAL_TOOL; }
	virtual image_id get_icon(spieler_t *sp) const { return sp->get_player_nr()==1 ? IMG_LEER : icon; }
	const char *get_tooltip(spieler_t *);
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

/* builds (random) tourist attraction (deafult_param==NULL) and maybe adds it to the next city
 * the parameter string is a follow (or NULL):
 * 1#theater
 * first letter: ignore climates
 * second letter: rotation (0,1,2,3,#=random)
 * finally building name
 * @author prissi
 */
class wkz_build_haus_t : public kartenboden_werkzeug_t {
public:
	wkz_build_haus_t() : kartenboden_werkzeug_t() { id = WKZ_BUILD_HAUS | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Built random attraction"); }
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

/* builts a (if param=NULL random) industry chain starting here *
 * the parameter string is a follow (or NULL):
 * 1#34,oelfeld
 * first letter: ignore climates
 * second letter: rotation (0,1,2,3,#=random)
 * next number is production value
 * finally industry name
 * NULL means random chain!
 */
class wkz_build_industries_land_t : public kartenboden_werkzeug_t {
public:
	wkz_build_industries_land_t() : kartenboden_werkzeug_t() { id = WKZ_LAND_CHAIN | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Build land consumer"); }
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_build_industries_city_t : public kartenboden_werkzeug_t {
public:
	wkz_build_industries_city_t() : kartenboden_werkzeug_t() { id = WKZ_CITY_CHAIN | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Build city market"); }
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_build_factory_t : public kartenboden_werkzeug_t {
public:
	wkz_build_factory_t() : kartenboden_werkzeug_t() { id = WKZ_BUILD_FACTORY | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Build city market"); }
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_link_factory_t : public kartenboden_werkzeug_t {
private:
	fabrik_t* last_fab;
	zeiger_t *wkz_linkzeiger;
public:
	wkz_link_factory_t() : kartenboden_werkzeug_t() { wkz_linkzeiger=NULL; last_fab=NULL; id = WKZ_LINK_FACTORY | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Connect factory"); }
	bool init( karte_t *, spieler_t * );
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_headquarter_t : public kartenboden_werkzeug_t {
private:
	const haus_besch_t *next_level( spieler_t *sp );
public:
	wkz_headquarter_t() : kartenboden_werkzeug_t() { id = WKZ_HEADQUARTER | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *);
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

/* protects map from further change (here because two clicks to confirm it!) */
class wkz_lock_game_t : public werkzeug_t {
public:
	wkz_lock_game_t() : werkzeug_t() { id = WKZ_LOCK_GAME | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Lock game"); }
	bool init( karte_t *welt, spieler_t * ) { return welt->get_einstellungen()->get_allow_player_change(); }
	const char *work( karte_t *welt, spieler_t *, koord3d ) {
		welt->access_einstellungen()->set_allow_player_change( false );
		destroy_all_win( true );
		welt->switch_active_player( 0 );
		welt->set_werkzeug( general_tool[WKZ_ABFRAGE], welt->get_spieler(0) );
		return NULL;
	}
	virtual bool is_init_network_save() const { return true; }
};

/* add random citycar if no default is set; else add a certain city car */
class wkz_add_citycar_t : public werkzeug_t {
public:
	wkz_add_citycar_t() : werkzeug_t() { id = WKZ_ADD_CITYCAR | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Add random citycar"); }
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

/* make forest */
class wkz_forest_t : public two_click_werkzeug_t {
public:
	wkz_forest_t() : two_click_werkzeug_t() { id = WKZ_FOREST | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Add forest"); }
private:
	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual uint8 is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & );
};

/* stop moving tool */
class wkz_stop_moving_t : public werkzeug_t {
private:
	koord3d last_pos;
	zeiger_t *wkz_linkzeiger;
	waytype_t waytype[2];
	halthandle_t last_halt;
public:
	wkz_stop_moving_t() : werkzeug_t() { wkz_linkzeiger=NULL; id = WKZ_STOP_MOVER | GENERAL_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("replace stop"); }
	bool init( karte_t *, spieler_t * );
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

/* make all tiles of this player a public stop
 * if this player is public, make all connected tiles a public stop */
class wkz_make_stop_public_t : public werkzeug_t {
public:
	wkz_make_stop_public_t() : werkzeug_t() { id = WKZ_MAKE_STOP_PUBLIC | GENERAL_TOOL;  }
	bool init( karte_t *, spieler_t * );
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	const char *get_tooltip(spieler_t *);
	virtual const char *move( karte_t *, spieler_t *, uint16 /* buttonstate */, koord3d );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

/********************* one click tools ****************************/

class wkz_pause_t : public werkzeug_t {
public:
	wkz_pause_t() : werkzeug_t() { id = WKZ_PAUSE | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Pause"); }
	bool is_selected(karte_t *welt) const { return welt->is_paused(); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->set_fast_forward(0);
		welt->set_pause( welt->is_paused()^1 );
		return false;
	}
};

class wkz_fastforward_t : public werkzeug_t {
public:
	wkz_fastforward_t() : werkzeug_t() { id = WKZ_FASTFORWARD | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Fast forward"); }
	bool is_selected(karte_t *welt) const { return welt->is_fast_forward(); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->set_pause(0);
		welt->set_fast_forward( welt->is_fast_forward()^1 );
		return false;
	}
};

class wkz_screenshot_t : public werkzeug_t {
public:
	wkz_screenshot_t() : werkzeug_t() { id = WKZ_SCREENSHOT | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Screenshot"); }
	bool init( karte_t *, spieler_t * ) {
		display_snapshot();
		create_win( new news_img("Screenshot\ngespeichert.\n"), w_time_delete, magic_none);
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// builts next chain
class wkz_increase_industry_t : public werkzeug_t {
public:
	wkz_increase_industry_t() : werkzeug_t() { id = WKZ_INCREASE_INDUSTRY | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Increase Industry density"); }
	bool init( karte_t *welt, spieler_t * ) {
		fabrikbauer_t::increase_industry_density( welt, false );
		return false;
	}
};

/* prissi: undo building */
class wkz_undo_t : public werkzeug_t {
public:
	wkz_undo_t() : werkzeug_t() { id = WKZ_UNDO | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Undo last ways construction"); }
	bool init( karte_t *, spieler_t *sp ) {
		if(!sp->undo()) {
			create_win( new news_img("UNDO failed!"), w_time_delete, magic_none);
		}
		return false;
	}
};

/* switch to next player
 * @author prissi
 */
class wkz_switch_player_t : public werkzeug_t {
public:
	wkz_switch_player_t() : werkzeug_t() { id = WKZ_SWITCH_PLAYER | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Change player"); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->switch_active_player( welt->get_active_player_nr()+1 );
		return false;
	}
	// since it should be handled internally
	virtual bool is_init_network_save() const { return true; }
};

// setp one year forward
class wkz_step_year_t : public werkzeug_t {
public:
	wkz_step_year_t() : werkzeug_t() { id = WKZ_STEP_YEAR | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Step timeline one year"); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->step_year();
		return false;
	}
};

class wkz_change_game_speed_t : public werkzeug_t {
public:
	wkz_change_game_speed_t() : werkzeug_t() { id = WKZ_CHANGE_GAME_SPEED | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) {
		int faktor = atoi(default_param);
		return faktor>0 ? translator::translate("Accelerate time") : translator::translate("Deccelerate time");
	}
	bool init( karte_t *welt, spieler_t * ) {
		welt->change_time_multiplier( atoi(default_param) );
		return false;
	}
};

class wkz_zoom_in_t : public werkzeug_t {
public:
	wkz_zoom_in_t() : werkzeug_t() { id = WKZ_ZOOM_IN | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("zooming in"); }
	bool init( karte_t *welt, spieler_t * ) {
		win_change_zoom_factor(true);
		welt->set_dirty();
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
};

class wkz_zoom_out_t : public werkzeug_t {
public:
	wkz_zoom_out_t() : werkzeug_t() { id = WKZ_ZOOM_OUT | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("zooming out"); }
	bool init( karte_t *welt, spieler_t * ) {
		win_change_zoom_factor(false);
		welt->set_dirty();
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
};

class wkz_show_coverage_t : public werkzeug_t {
public:
	wkz_show_coverage_t() : werkzeug_t() { id = WKZ_SHOW_COVERAGE | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("show station coverage"); }
	bool is_selected(karte_t *) const { return umgebung_t::station_coverage_show; }
	bool init( karte_t *welt, spieler_t * ) {
		umgebung_t::station_coverage_show = !umgebung_t::station_coverage_show;
		welt->set_dirty();
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_show_name_t : public werkzeug_t {
public:
	wkz_show_name_t() : werkzeug_t() { id = WKZ_SHOW_NAMES | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) {
		return translator::translate(
			umgebung_t::show_names==3 ? "hide station names" :
			(umgebung_t::show_names&1) ? "show waiting bars" : "show station names");
	}
	bool init( karte_t *welt, spieler_t * ) {
		umgebung_t::show_names = (umgebung_t::show_names+1) & 3;
		welt->set_dirty();
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_show_grid_t : public werkzeug_t {
public:
	wkz_show_grid_t() : werkzeug_t() { id = WKZ_SHOW_GRID | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("show grid"); }
	bool is_selected(karte_t *) const { return grund_t::show_grid; }
	bool init( karte_t *welt, spieler_t * ) {
		grund_t::toggle_grid();
		welt->set_dirty();
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_show_trees_t : public werkzeug_t {
public:
	wkz_show_trees_t() : werkzeug_t() { id = WKZ_SHOW_TREES | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("hide trees"); }
	bool is_selected(karte_t *) const { return umgebung_t::hide_trees; }
	bool init( karte_t *welt, spieler_t * ) {
		umgebung_t::hide_trees = !umgebung_t::hide_trees;
		welt->set_dirty();
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_show_houses_t : public werkzeug_t {
public:
	wkz_show_houses_t() : werkzeug_t() { id = WKZ_SHOW_HOUSES | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) {
		return translator::translate(
			umgebung_t::hide_buildings==0 ? "hide city building" :
			(umgebung_t::hide_buildings==1) ? "hide all building" : "show all building");
	}
	bool init( karte_t *welt, spieler_t * ) {
		umgebung_t::hide_buildings ++;
		if(umgebung_t::hide_buildings>umgebung_t::ALL_HIDDEN_BUIDLING) {
			umgebung_t::hide_buildings = umgebung_t::NOT_HIDE;
		}
		welt->set_dirty();
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_show_underground_t : public werkzeug_t {
public:
	wkz_show_underground_t() : werkzeug_t() { id = WKZ_SHOW_UNDERGROUND | SIMPLE_TOOL; }
	static sint8 save_underground_level;
	const char *get_tooltip(spieler_t *);
	bool is_selected(karte_t *) const;
	void draw_after( karte_t *w, koord pos ) const;
	bool init( karte_t *welt, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_rotate90_t : public werkzeug_t {
public:
	wkz_rotate90_t() : werkzeug_t() { id = WKZ_ROTATE90 | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Rotate map"); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->rotate90();
		welt->update_map();
		return false;
	}
};

class wkz_quit_t : public werkzeug_t {
public:
	wkz_quit_t() : werkzeug_t() { id = WKZ_QUIT | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Beenden"); }
	bool init( karte_t *welt, spieler_t * ) {
		destroy_all_win( true );
		welt->beenden( true );
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
};

// step size by default_param
class wkz_fill_trees_t : public werkzeug_t {
public:
	wkz_fill_trees_t() : werkzeug_t() { id = WKZ_FILL_TREES | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Fill trees"); }
	bool init( karte_t *welt, spieler_t * ) {
		if(  default_param  ) {
			baum_t::fill_trees( welt, atoi(default_param) );
		}
		return false;
	}
};

/* change day/night view manually */
class wkz_daynight_level_t : public werkzeug_t {
public:
	wkz_daynight_level_t() : werkzeug_t() { id = WKZ_DAYNIGHT_LEVEL | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *);
	bool init( karte_t *, spieler_t * );
	virtual bool is_init_network_save() const { return true; }
};


/* change day/night view manually */
class wkz_vehicle_tooltips_t : public werkzeug_t {
public:
	wkz_vehicle_tooltips_t() : werkzeug_t() { id = WKZ_VEHICLE_TOOLTIPS | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Toggle vehicle tooltips"); }
	bool init( karte_t *, spieler_t * ) {
		umgebung_t::show_vehicle_states = (umgebung_t::show_vehicle_states+1)%3;
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
};

class wkz_toggle_pax_station_t : public werkzeug_t {
public:
	wkz_toggle_pax_station_t() : werkzeug_t() { id = WKZ_TOOGLE_PAX | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("5LIGHT_CHOOSE"); }
	bool is_selected(karte_t *welt) const { return welt->get_einstellungen()->get_show_pax(); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->access_einstellungen()->set_show_pax( !welt->get_einstellungen()->get_show_pax() );
		return false;
	}
	virtual bool is_init_network_save() const { return false; }
};

class wkz_toggle_pedestrians_t : public werkzeug_t {
public:
	wkz_toggle_pedestrians_t() : werkzeug_t() { id = WKZ_TOOGLE_PEDESTRIANS | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("6LIGHT_CHOOSE"); }
	bool is_selected(karte_t *welt) const { return welt->get_einstellungen()->get_random_pedestrians(); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->access_einstellungen()->set_random_pedestrians( !welt->get_einstellungen()->get_random_pedestrians() );
		return false;
	}
	virtual bool is_init_network_save() const { return false; }
};

/* internal simple tools needed for networksynchronisation */
class wkz_traffic_level_t : public werkzeug_t {
public:
	wkz_traffic_level_t() : werkzeug_t() { id = WKZ_TRAFFIC_LEVEL | SIMPLE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("6WORLD_CHOOSE"); }
	bool is_selected(karte_t *) const { return false; }
	bool init( karte_t *welt, spieler_t * ) {
		assert(  default_param  );
		welt->access_einstellungen()->set_verkehr_level( atoi(default_param) );
		return false;
	}
	virtual bool is_init_network_save() const { return false; }
};

class wkz_change_convoi_t : public werkzeug_t {
public:
	wkz_change_convoi_t() : werkzeug_t() { id = WKZ_CONVOI_TOOL | SIMPLE_TOOL; }
	virtual bool init( karte_t *, spieler_t * );
	virtual bool is_init_network_save() const { return false; }
};

class wkz_change_line_t : public werkzeug_t {
public:
	wkz_change_line_t() : werkzeug_t() { id = WKZ_LINE_TOOL | SIMPLE_TOOL; }
	virtual bool init( karte_t *, spieler_t * );
	virtual bool is_init_network_save() const { return false; }
};

class wkz_change_depot_t : public werkzeug_t {
public:
	wkz_change_depot_t() : werkzeug_t() { id = WKZ_DEPOT_TOOL | SIMPLE_TOOL; }
	virtual bool init( karte_t *, spieler_t * );
	virtual bool is_init_network_save() const { return false; }
};

class wkz_change_password_hash_t : public werkzeug_t {
public:
	wkz_change_password_hash_t() : werkzeug_t() { id = WKZ_PWDHASH_TOOL | SIMPLE_TOOL; }
	virtual bool init( karte_t *, spieler_t * );
	virtual bool is_init_network_save() const { return false; }
};

// adds a new player of certain type to the map
class wkz_change_player_t : public werkzeug_t {
public:
	wkz_change_player_t() : werkzeug_t() { id = WKZ_SET_PLAYER_TOOL | SIMPLE_TOOL; }
	virtual bool init( karte_t *, spieler_t * );
	virtual bool is_init_network_save() const { return false; }
};

// change timing of traffic light
class wkz_change_traffic_light_t : public werkzeug_t {
public:
	wkz_change_traffic_light_t() : werkzeug_t() { id = WKZ_TRAFFIC_LIGHT_TOOL | SIMPLE_TOOL; }
	virtual bool init( karte_t *, spieler_t * );
	virtual bool is_init_network_save() const { return false; }
};

/********************** dialoge tools *****************************/

// general help
class wkz_help_t : public werkzeug_t {
public:
	wkz_help_t() : werkzeug_t() { id = WKZ_HELP | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Help"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_mainhelp); }
	bool init( karte_t *, spieler_t * ) {
		create_win(new help_frame_t("general.txt"), w_info, magic_mainhelp);
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// open info/quit dialoge
class wkz_optionen_t : public werkzeug_t {
public:
	wkz_optionen_t() : werkzeug_t() { id = WKZ_OPTIONEN | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Einstellungsfenster"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_optionen_gui_t); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win(240, 120, new optionen_gui_t(welt), w_info, magic_optionen_gui_t);
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// open minimap
class wkz_minimap_t : public werkzeug_t {
public:
	wkz_minimap_t() : werkzeug_t() { id = WKZ_MINIMAP | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Reliefkarte"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_reliefmap); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new map_frame_t(welt), w_info, magic_reliefmap);
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// open line management
class wkz_lines_t : public werkzeug_t {
public:
	wkz_lines_t() : werkzeug_t() { id = WKZ_LINEOVERVIEW | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Line Management"); }
	virtual image_id get_icon(spieler_t *sp) const { return sp->get_player_nr()==1 ? IMG_LEER : icon; }
	bool is_selected(karte_t *welt) const { return win_get_magic((long)(&(welt->get_active_player()->simlinemgmt))); }
	bool init( karte_t *, spieler_t *sp ) {
		if(sp->get_player_nr()!=1) {
			sp->simlinemgmt.zeige_info( sp );
		}
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// open messages
class wkz_messages_t : public werkzeug_t {
public:
	wkz_messages_t() : werkzeug_t() { id = WKZ_MESSAGES | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Mailbox"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_messageframe); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new message_frame_t(welt), w_info, magic_messageframe );
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// open messages
class wkz_finances_t : public werkzeug_t {
public:
	wkz_finances_t() : werkzeug_t() { id = WKZ_FINANCES | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Finanzen"); }
	bool is_selected(karte_t *welt) const { return win_get_magic((long)welt->get_active_player()); }
	bool init( karte_t *, spieler_t *sp ) {
		create_win( new money_frame_t(sp), w_info, (long)sp );
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// open player dialoge
class wkz_players_t : public werkzeug_t {
public:
	wkz_players_t() : werkzeug_t() { id = WKZ_PLAYERS | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Spielerliste"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_ki_kontroll_t); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( 272, 160, new ki_kontroll_t(welt), w_info, magic_ki_kontroll_t);
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// open player dialoge
class wkz_displayoptions_t : public werkzeug_t {
public:
	wkz_displayoptions_t() : werkzeug_t() { id = WKZ_DISPLAYOPTIONS | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Helligk."); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_color_gui_t); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win(new color_gui_t(welt), w_info, magic_color_gui_t);
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// open sound dialoge
class wkz_sound_t : public werkzeug_t {
public:
	wkz_sound_t() : werkzeug_t() { id = WKZ_SOUND | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Sound"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_sound_kontroll_t); }
	bool init( karte_t *, spieler_t * ) {
		create_win(new sound_frame_t(), w_info, magic_sound_kontroll_t);
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// open language dialoge
class wkz_language_t : public werkzeug_t {
public:
	wkz_language_t() : werkzeug_t() { id = WKZ_LANGUAGE | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Sprache"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_sprachengui_t); }
	bool init( karte_t *, spieler_t * ) {
		create_win(new sprachengui_t(), w_info, magic_sprachengui_t);
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// open player color dialoge
class wkz_playercolor_t : public werkzeug_t {
public:
	wkz_playercolor_t() : werkzeug_t() { id = WKZ_PLAYERCOLOR | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Farbe"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_farbengui_t); }
	bool init( karte_t *, spieler_t *sp ) {
		create_win(new farbengui_t(sp), w_info, magic_farbengui_t);
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// jump to position dialoge
class wkz_jump_t : public werkzeug_t {
public:
	wkz_jump_t() : werkzeug_t() { id = WKZ_JUMP | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Jump to"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_jump); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new jump_frame_t(welt), w_info, magic_jump);
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// load game dialoge
class wkz_load_t : public werkzeug_t {
public:
	wkz_load_t() : werkzeug_t() { id = WKZ_LOAD | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Laden"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_load_t); }
	bool is_init_network_save() const { return true; }
	bool init( karte_t *welt, spieler_t * ) {
		destroy_all_win( true );
		create_win(new loadsave_frame_t(welt, true), w_info, magic_load_t);
		return false;
	}
};

// save game dialoge
class wkz_save_t : public werkzeug_t {
public:
	wkz_save_t() : werkzeug_t() { id = WKZ_SAVE | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Speichern"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_save_t); }
	bool is_init_network_save() const { return true; }
	bool init( karte_t *welt, spieler_t * ) {
		create_win(new loadsave_frame_t(welt, false), w_info, magic_save_t);
		return false;
	}
};

/* open the list of halt */
class wkz_list_halt_t : public werkzeug_t {
public:
	wkz_list_halt_t() : werkzeug_t() { id = WKZ_LIST_HALT | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("hl_title"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_halt_list_t); }
	bool init( karte_t *, spieler_t *sp ) {
		create_win( new halt_list_frame_t(sp), w_info, magic_halt_list_t );
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

/* open the list of vehicle */
class wkz_list_convoi_t : public werkzeug_t {
public:
	wkz_list_convoi_t() : werkzeug_t() { id = WKZ_LIST_CONVOI | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("cl_title"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_convoi_t); }
	bool init( karte_t *, spieler_t *sp ) {
		create_win( new convoi_frame_t(sp), w_info, magic_convoi_t );
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

/* open the list of towns */
class wkz_list_town_t : public werkzeug_t {
public:
	wkz_list_town_t() : werkzeug_t() { id = WKZ_LIST_TOWN | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("tl_title"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_citylist_frame_t); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new citylist_frame_t(welt), w_info, magic_citylist_frame_t );
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

/* open the list of goods */
class wkz_list_goods_t : public werkzeug_t {
public:
	wkz_list_goods_t() : werkzeug_t() { id = WKZ_LIST_GOODS | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("gl_title"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_goodslist); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new goods_frame_t(welt), w_info, magic_goodslist );
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

/* open the list of factories */
class wkz_list_factory_t : public werkzeug_t {
public:
	wkz_list_factory_t() : werkzeug_t() { id = WKZ_LIST_FACTORY | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("fl_title"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_factorylist); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new factorylist_frame_t(welt), w_info, magic_factorylist );
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

/* open the list of attraction */
class wkz_list_curiosity_t : public werkzeug_t {
public:
	wkz_list_curiosity_t() : werkzeug_t() { id = WKZ_LIST_CURIOSITY | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("curlist_title"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_curiositylist); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new curiositylist_frame_t(welt), w_info, magic_curiositylist );
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

/* factory building dialog */
class wkz_factorybuilder_t : public werkzeug_t {
public:
	wkz_factorybuilder_t() : werkzeug_t() { id = WKZ_EDIT_FACTORY | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("factorybuilder"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_edit_factory); }
	bool init( karte_t *welt, spieler_t *sp ) {
		if (!is_selected(welt)) {
			create_win( new factory_edit_frame_t(sp,welt), w_info, magic_edit_factory );
		}
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

/* attraction building dialog */
class wkz_attractionbuilder_t : public werkzeug_t {
public:
	wkz_attractionbuilder_t() : werkzeug_t() { id = WKZ_EDIT_ATTRACTION | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("curiosity builder"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_edit_attraction); }
	bool init( karte_t *welt, spieler_t *sp ) {
		if (!is_selected(welt)) {
			create_win( new curiosity_edit_frame_t(sp,welt), w_info, magic_edit_attraction );
		}
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

/* house building dialog */
class wkz_housebuilder_t : public werkzeug_t {
public:
	wkz_housebuilder_t() : werkzeug_t() { id = WKZ_EDIT_HOUSE | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("citybuilding builder"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_edit_house); }
	bool init( karte_t *welt, spieler_t *sp ) {
		if (!is_selected(welt)) {
			create_win( new citybuilding_edit_frame_t(sp,welt), w_info, magic_edit_house );
		}
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

/* house building dialog */
class wkz_treebuilder_t : public werkzeug_t {
public:
	wkz_treebuilder_t() : werkzeug_t() { id = WKZ_EDIT_TREE | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("baum builder"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_edit_tree); }
	bool init( karte_t *welt, spieler_t *sp ) {
		if (!is_selected(welt)) {
			create_win( new baum_edit_frame_t(sp,welt), w_info, magic_edit_tree );
		}
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// to increase map-size
class wkz_enlarge_map_t : public werkzeug_t{
public:
	wkz_enlarge_map_t() : werkzeug_t() { id = WKZ_ENLARGE_MAP | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("enlarge map"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_bigger_map); }
	bool init( karte_t *welt, spieler_t *sp ) {
		destroy_all_win( true );
		create_win( new enlarge_map_frame_t(sp,welt), w_info, magic_bigger_map );
		return false;
	}
};

/* open the list of label */
class wkz_list_label_t : public werkzeug_t {
public:
	wkz_list_label_t() : werkzeug_t() { id = WKZ_LIST_LABEL | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("labellist_title"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_labellist); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new labellist_frame_t(welt), w_info, magic_labellist );
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

/* open climate settings */
class wkz_climates_t : public werkzeug_t {
public:
	wkz_climates_t() : werkzeug_t() { id = WKZ_CLIMATES | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Climate Control"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_climate); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new climate_gui_t(welt->access_einstellungen()), w_info, magic_climate );
		return false;
	}
};

/* open climate settings */
class wkz_settings_t : public werkzeug_t {
public:
	wkz_settings_t() : werkzeug_t() { id = WKZ_SETTINGS | DIALOGE_TOOL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Setting"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_settings_frame_t); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new settings_frame_t(welt->access_einstellungen()), w_info, magic_settings_frame_t );
		return false;
	}
};
#endif
