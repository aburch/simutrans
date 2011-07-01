/*
 * New OO tool system
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef simwerkz2_h
#define simwerkz2_h

#include "simtypes.h"
#include "simskin.h"
#include "simworld.h"
#include "simmenu.h"
#include "simwin.h"

#include "besch/way_obj_besch.h"

#include "bauer/fabrikbauer.h"

#include "dataobj/umgebung.h"
#include "dataobj/translator.h"

#include "dings/baum.h"

#include "gui/messagebox.h"

#include "player/simplay.h"

#include "tpl/slist_tpl.h"

class koord3d;
class koord;
class wegbauer_t;
class roadsign_besch_t;
class weg_besch_t;
class route_t;

/****************************** helper functions: *****************************/

char *tooltip_with_price(const char * tip, sint64 price);

/************************ general tool *******************************/

// query tile info: default tool
class wkz_abfrage_t : public werkzeug_t {
public:
	wkz_abfrage_t() : werkzeug_t() { id = WKZ_ABFRAGE | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Abfrage"); }
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
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Abriss"); }
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
	const char *get_tooltip(const spieler_t *sp) const { return tooltip_with_price("Anheben", sp->get_welt()->get_settings().cst_alter_land); }
	virtual image_id get_icon(spieler_t *) const { return grund_t::underground_mode==grund_t::ugm_all ? IMG_LEER : icon; }
	bool init( karte_t *, spieler_t * ) { is_dragging = false; return true; }
	bool exit( karte_t *, spieler_t * ) { is_dragging = false; return true; }
	const char *check( karte_t *, spieler_t *, koord3d );
	const char *work( karte_t *, spieler_t *, koord3d );
	const char *move( karte_t *, spieler_t *, uint16 /* buttonstate */, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_lower_t : public werkzeug_t {
private:
	bool is_dragging;
	sint16 drag_height;
public:
	wkz_lower_t() : werkzeug_t() { offset = Z_GRID; id = WKZ_LOWER_LAND | GENERAL_TOOL;  }
	const char *get_tooltip(const spieler_t *sp) const { return tooltip_with_price("Absenken", sp->get_welt()->get_settings().cst_alter_land); }
	virtual image_id get_icon(spieler_t *) const { return grund_t::underground_mode==grund_t::ugm_all ? IMG_LEER : icon; }
	bool init( karte_t *, spieler_t * ) { is_dragging = false; return true; }
	bool exit( karte_t *, spieler_t * ) { is_dragging = false; return true; }
	const char *check( karte_t *, spieler_t *, koord3d );
	const char *work( karte_t *, spieler_t *, koord3d);
	const char *move( karte_t *, spieler_t *, uint16 /* buttonstate */, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

/* slope tool definitions */
class wkz_setslope_t : public werkzeug_t {
public:
	wkz_setslope_t() : werkzeug_t() { id = WKZ_SETSLOPE | GENERAL_TOOL; }
	static const char *wkz_set_slope_work( karte_t *welt, spieler_t *sp, koord3d pos, int slope );
	const char *get_tooltip(const spieler_t *sp) const { return tooltip_with_price("Built artifical slopes", sp->get_welt()->get_settings().cst_set_slope); }
	virtual bool is_init_network_save() const { return true; }
	const char *check( karte_t *, spieler_t *, koord3d );
	virtual const char *work( karte_t *welt, spieler_t *sp, koord3d k ) { return wkz_set_slope_work( welt, sp, k, atoi(default_param) ); }
};

class wkz_restoreslope_t : public werkzeug_t {
public:
	wkz_restoreslope_t() : werkzeug_t() { id = WKZ_RESTORESLOPE | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *sp) const { return tooltip_with_price("Restore natural slope", sp->get_welt()->get_settings().cst_set_slope); }
	virtual bool is_init_network_save() const { return true; }
	const char *check( karte_t *, spieler_t *, koord3d );
	virtual const char *work( karte_t *welt, spieler_t *sp, koord3d k ) { return wkz_setslope_t::wkz_set_slope_work( welt, sp, k, RESTORE_SLOPE ); }
};

class wkz_marker_t : public kartenboden_werkzeug_t {
public:
	wkz_marker_t() : kartenboden_werkzeug_t() { id = WKZ_MARKER | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *sp) const { return tooltip_with_price("Marker", sp->get_welt()->get_settings().cst_buy_land); }
	virtual const char *work( karte_t *welt, spieler_t *sp, koord3d k );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_clear_reservation_t : public werkzeug_t {
public:
	wkz_clear_reservation_t() : werkzeug_t() { id = WKZ_CLEAR_RESERVATION | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Clear block reservation"); }
	bool init( karte_t *, spieler_t * );
	bool exit( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_transformer_t : public kartenboden_werkzeug_t {
public:
	wkz_transformer_t() : kartenboden_werkzeug_t() { id = WKZ_TRANSFORMER | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const;
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_add_city_t : public kartenboden_werkzeug_t {
public:
	wkz_add_city_t() : kartenboden_werkzeug_t() { id = WKZ_ADD_CITY | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *sp) const { return tooltip_with_price("Found new city", sp->get_welt()->get_settings().cst_found_city); }
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

// buy a house to protext it from renovating
class wkz_buy_house_t : public kartenboden_werkzeug_t {
public:
	wkz_buy_house_t() : kartenboden_werkzeug_t() { id = WKZ_BUY_HOUSE | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Haus kaufen"); }
	const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};
/************** the following tools need a valid default_param ************************/

// step size by default_param
class wkz_change_city_size_t : public werkzeug_t {
public:
	wkz_change_city_size_t() : werkzeug_t() { id = WKZ_CHANGE_CITY_SIZE | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate( atoi(default_param)>=0 ? "Grow city" : "Shrink city" ); }
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_plant_tree_t : public kartenboden_werkzeug_t {
public:
	wkz_plant_tree_t() : kartenboden_werkzeug_t() { id = WKZ_PLANT_TREE | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate( "Plant tree" ); }
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

	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual uint8 is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & );

protected:
	static karte_t *welt;
	const weg_besch_t *besch;

	virtual const weg_besch_t *get_besch(uint16,bool) const;
	void calc_route( wegbauer_t &bauigel, const koord3d &, const koord3d & );

public:
	wkz_wegebau_t() : two_click_werkzeug_t(), besch(NULL) { id = WKZ_WEGEBAU | GENERAL_TOOL; }
	virtual image_id get_icon(spieler_t *) const;
	virtual const char *get_tooltip(const spieler_t *) const;
	virtual const char* get_default_param(spieler_t *) const;
	virtual bool is_selected(const karte_t *welt ) const;
	virtual bool init( karte_t *, spieler_t * );
	virtual bool is_move_network_save(spieler_t *sp) const { return two_click_werkzeug_t::is_move_network_save(sp) && (besch  &&  (besch->get_styp()!=1  ||  besch->get_wtyp()==air_wt)); }
	virtual bool is_init_network_save() const { return true; }
};

class wkz_build_cityroad : public wkz_wegebau_t {
private:
	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
public:
	wkz_build_cityroad() : wkz_wegebau_t() { id = WKZ_CITYROAD | GENERAL_TOOL; }
	virtual const weg_besch_t *get_besch(uint16,bool) const;
	virtual image_id get_icon(spieler_t *sp) const { return werkzeug_t::get_icon(sp); }
	virtual bool is_selected(const karte_t *welt ) const { return werkzeug_t::is_selected(welt); }
	virtual bool is_init_network_save() const { return true; }
};

class wkz_brueckenbau_t : public two_click_werkzeug_t {
private:
	ribi_t::ribi ribi;

	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual uint8 is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & );
public:
	wkz_brueckenbau_t() : two_click_werkzeug_t() { id = WKZ_BRUECKENBAU | GENERAL_TOOL; }
	virtual image_id get_icon(spieler_t *) const { return grund_t::underground_mode==grund_t::ugm_all ? IMG_LEER : icon; }
	const char *get_tooltip(const spieler_t *) const;
	virtual bool is_move_network_save(spieler_t *) const { return false;}
	virtual bool is_init_network_save() const { return true; }
};

class wkz_tunnelbau_t : public two_click_werkzeug_t {
private:
	void calc_route( wegbauer_t &bauigel, const koord3d &, const koord3d &, karte_t* );
	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual uint8 is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & );
public:
	wkz_tunnelbau_t() : two_click_werkzeug_t() { id = WKZ_TUNNELBAU | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const;
	virtual bool is_move_network_save(spieler_t *) const { return false;}
	const char *check( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_wayremover_t : public two_click_werkzeug_t {
private:
	bool calc_route( route_t &, spieler_t *, const koord3d& start, const koord3d &to );
	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual uint8 is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & );
	// error message to be returned by calc_route
	const char *calc_route_error;
public:
	wkz_wayremover_t() : two_click_werkzeug_t() { id = WKZ_WAYREMOVER | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const;
	virtual bool is_init_network_save() const { return true; }
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
	virtual const char *get_tooltip(const spieler_t *) const;
	virtual bool is_selected(const karte_t *welt) const;
	virtual bool init( karte_t *, spieler_t * );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_wayobj_remover_t : public wkz_wayobj_t {
public:
	wkz_wayobj_remover_t() : wkz_wayobj_t(false) { id = WKZ_REMOVE_WAYOBJ | GENERAL_TOOL; }
	virtual bool is_selected(const karte_t *welt) const { return werkzeug_t::is_selected( welt ); }
	virtual bool is_init_network_save() const { return true; }
};

class wkz_station_t : public werkzeug_t {
private:
	static char toolstring[256];
	const char *wkz_station_building_aux( karte_t *, spieler_t *, bool, koord3d, const haus_besch_t *, sint8 rotation );
	const char *wkz_station_dock_aux( karte_t *, spieler_t *, koord3d, const haus_besch_t * );
	const char *wkz_station_aux( karte_t *, spieler_t *, koord3d, const haus_besch_t *, waytype_t, sint64 cost, const char *halt_suffix );
	const haus_besch_t *get_besch( sint8 &rotation ) const;
public:
	wkz_station_t() : werkzeug_t() { id = WKZ_STATION | GENERAL_TOOL; }
	virtual image_id get_icon(spieler_t *) const;
	const char *get_tooltip(const spieler_t *) const;
	bool init( karte_t *, spieler_t * );
	const char *check( karte_t *, spieler_t *, koord3d );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_roadsign_t : public two_click_werkzeug_t {
public:
	wkz_roadsign_t();
	const char *get_tooltip(const spieler_t *) const;
	bool init( karte_t *, spieler_t * );
	bool exit( karte_t *welt, spieler_t *sp );

	void set_values(spieler_t *sp, uint8 spacing, bool remove, bool replace );
	void get_values(spieler_t *sp, uint8 &spacing, bool &remove, bool &replace );
	bool is_init_network_save() const { return true; }
	void draw_after( karte_t *welt, koord pos ) const;
	const char* get_default_param(spieler_t *sp) const;

private:
	const roadsign_besch_t* besch;
	const char *place_sign_intern( karte_t *, spieler_t *, grund_t*, const roadsign_besch_t* b = NULL);
	// place signals every n tiles:
	uint8 signal_spacing[MAX_PLAYER_COUNT];
	bool remove_intermediate_signals[MAX_PLAYER_COUNT];
	bool replace_other_signals[MAX_PLAYER_COUNT];
	static char toolstring[256];
	// read the variables from the default_param
	void read_default_param(spieler_t *sp);

	const char* check_pos_intern(karte_t *, spieler_t *, koord3d);
	bool calc_route( route_t &, spieler_t *, const koord3d& start, const koord3d &to );

	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual uint8 is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & );
};

class wkz_depot_t : public werkzeug_t {
private:
	static char toolstring[256];
	const char *wkz_depot_aux(karte_t *welt, spieler_t *sp, koord3d pos, const haus_besch_t *besch, waytype_t wegtype, sint64 cost);
public:
	wkz_depot_t() : werkzeug_t() { id = WKZ_DEPOT | GENERAL_TOOL; }
	virtual image_id get_icon(spieler_t *sp) const { return sp->get_player_nr()==1 ? IMG_LEER : icon; }
	const char *get_tooltip(const spieler_t *) const;
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
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Built random attraction"); }
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
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Build land consumer"); }
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_build_industries_city_t : public kartenboden_werkzeug_t {
public:
	wkz_build_industries_city_t() : kartenboden_werkzeug_t() { id = WKZ_CITY_CHAIN | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Build city market"); }
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_build_factory_t : public kartenboden_werkzeug_t {
public:
	wkz_build_factory_t() : kartenboden_werkzeug_t() { id = WKZ_BUILD_FACTORY | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Build city market"); }
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

class wkz_link_factory_t : public two_click_werkzeug_t {
public:
	wkz_link_factory_t() : two_click_werkzeug_t() { id = WKZ_LINK_FACTORY | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Connect factory"); }
	virtual bool is_init_network_save() const { return true; }
private:
	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & ) { }
	virtual uint8 is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & );
	virtual image_id get_marker_image();
};

class wkz_headquarter_t : public kartenboden_werkzeug_t {
private:
	const haus_besch_t *next_level( const spieler_t *sp ) const;
public:
	wkz_headquarter_t() : kartenboden_werkzeug_t() { id = WKZ_HEADQUARTER | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const;
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

/* protects map from further change (here because two clicks to confirm it!) */
class wkz_lock_game_t : public werkzeug_t {
public:
	wkz_lock_game_t() : werkzeug_t() { id = WKZ_LOCK_GAME | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Lock game"); }
	const char *work( karte_t *welt, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

/* add random citycar if no default is set; else add a certain city car */
class wkz_add_citycar_t : public werkzeug_t {
public:
	wkz_add_citycar_t() : werkzeug_t() { id = WKZ_ADD_CITYCAR | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Add random citycar"); }
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

/* make forest */
class wkz_forest_t : public two_click_werkzeug_t {
public:
	wkz_forest_t() : two_click_werkzeug_t() { id = WKZ_FOREST | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Add forest"); }
private:
	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual uint8 is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & );
	virtual bool is_init_network_save() const { return true; }
};

/* stop moving tool */
class wkz_stop_moving_t : public two_click_werkzeug_t {
private:
	waytype_t waytype[2];
	halthandle_t last_halt;
public:
	wkz_stop_moving_t() : two_click_werkzeug_t() { id = WKZ_STOP_MOVER | GENERAL_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("replace stop"); }
	virtual bool is_init_network_save() const { return true; }

private:
	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & ) { }
	virtual uint8 is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & );
	virtual image_id get_marker_image();

	void read_start_position(karte_t *welt, spieler_t *sp, const koord3d &pos);
};

/* make all tiles of this player a public stop
 * if this player is public, make all connected tiles a public stop */
class wkz_make_stop_public_t : public werkzeug_t {
public:
	wkz_make_stop_public_t() : werkzeug_t() { id = WKZ_MAKE_STOP_PUBLIC | GENERAL_TOOL;  }
	bool init( karte_t *, spieler_t * );
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	const char *get_tooltip(const spieler_t *) const;
	virtual const char *move( karte_t *, spieler_t *, uint16 /* buttonstate */, koord3d );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual bool is_init_network_save() const { return true; }
};

/********************* one click tools ****************************/

class wkz_pause_t : public werkzeug_t {
public:
	wkz_pause_t() : werkzeug_t() { id = WKZ_PAUSE | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return umgebung_t::networkmode ? translator::translate("deactivated in online mode") : translator::translate("Pause"); }
	image_id get_icon(spieler_t *) const { return umgebung_t::networkmode ? IMG_LEER : icon; }
	bool is_selected(const karte_t *welt) const { return welt->is_paused(); }
	bool init( karte_t *welt, spieler_t * ) {
		if(  !umgebung_t::networkmode  ) {
			welt->set_fast_forward(0);
			welt->set_pause( welt->is_paused()^1 );
		}
		return false;
	}
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	virtual bool is_init_network_save() const { return !umgebung_t::networkmode; }
	virtual bool is_work_network_save() const { return !umgebung_t::networkmode; }
};

class wkz_fastforward_t : public werkzeug_t {
public:
	wkz_fastforward_t() : werkzeug_t() { id = WKZ_FASTFORWARD | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return umgebung_t::networkmode ? translator::translate("deactivated in online mode") : translator::translate("Fast forward"); }
	image_id get_icon(spieler_t *) const { return umgebung_t::networkmode ? IMG_LEER : icon; }
	bool is_selected(const karte_t *welt) const { return welt->is_fast_forward(); }
	bool init( karte_t *welt, spieler_t * ) {
		if(  !umgebung_t::networkmode  ) {
			welt->set_pause(0);
			welt->set_fast_forward( welt->is_fast_forward()^1 );
		}
		return false;
	}
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	virtual bool is_init_network_save() const { return !umgebung_t::networkmode; }
	virtual bool is_work_network_save() const { return !umgebung_t::networkmode; }
};

class wkz_screenshot_t : public werkzeug_t {
public:
	wkz_screenshot_t() : werkzeug_t() { id = WKZ_SCREENSHOT | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Screenshot"); }
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
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Increase Industry density"); }
	bool init( karte_t *welt, spieler_t * ) {
		fabrikbauer_t::increase_industry_density( welt, true, false );
		return false;
	}
};

/* prissi: undo building */
class wkz_undo_t : public werkzeug_t {
public:
	wkz_undo_t() : werkzeug_t() { id = WKZ_UNDO | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Undo last ways construction"); }
	bool init( karte_t *, spieler_t *sp ) {
		if(!sp->undo()  &&  is_local_execution()) {
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
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Change player"); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->switch_active_player( welt->get_active_player_nr()+1, true );
		return false;
	}
	// since it is handled internally
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// setp one year forward
class wkz_step_year_t : public werkzeug_t {
public:
	wkz_step_year_t() : werkzeug_t() { id = WKZ_STEP_YEAR | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Step timeline one year"); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->step_year();
		return false;
	}
};

class wkz_change_game_speed_t : public werkzeug_t {
public:
	wkz_change_game_speed_t() : werkzeug_t() { id = WKZ_CHANGE_GAME_SPEED | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const {
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
	const char *get_tooltip(const spieler_t *) const { return translator::translate("zooming in"); }
	bool init( karte_t *welt, spieler_t * ) {
		win_change_zoom_factor(true);
		welt->set_dirty();
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_zoom_out_t : public werkzeug_t {
public:
	wkz_zoom_out_t() : werkzeug_t() { id = WKZ_ZOOM_OUT | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("zooming out"); }
	bool init( karte_t *welt, spieler_t * ) {
		win_change_zoom_factor(false);
		welt->set_dirty();
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_show_coverage_t : public werkzeug_t {
public:
	wkz_show_coverage_t() : werkzeug_t() { id = WKZ_SHOW_COVERAGE | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("show station coverage"); }
	bool is_selected(const karte_t *) const { return umgebung_t::station_coverage_show; }
	bool init( karte_t *welt, spieler_t * ) {
		umgebung_t::station_coverage_show = !umgebung_t::station_coverage_show;
		welt->set_dirty();
		return false;
	}
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_show_name_t : public werkzeug_t {
public:
	wkz_show_name_t() : werkzeug_t() { id = WKZ_SHOW_NAMES | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const {
		return translator::translate(
			(umgebung_t::show_names>>2)==2 ? "hide station names" :
			(umgebung_t::show_names&1) ? "show waiting bars" : "show station names");
	}
	bool init( karte_t *welt, spieler_t * ) {
		if(  umgebung_t::show_names>=11  ) {
			if(  (umgebung_t::show_names&3)==3  ) {
				umgebung_t::show_names = 0;
			}
			else {
				umgebung_t::show_names = 2;
			}
		}
		else if(  umgebung_t::show_names==2  ) {
				umgebung_t::show_names = 3;
		}
		else if(  umgebung_t::show_names==0  ) {
			umgebung_t::show_names = 1;
		}
		else {
			umgebung_t::show_names += 4;
		}
		welt->set_dirty();
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_show_grid_t : public werkzeug_t {
public:
	wkz_show_grid_t() : werkzeug_t() { id = WKZ_SHOW_GRID | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("show grid"); }
	bool is_selected(const karte_t *) const { return grund_t::show_grid; }
	bool init( karte_t *welt, spieler_t * ) {
		grund_t::toggle_grid();
		welt->set_dirty();
		return false;
	}
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_show_trees_t : public werkzeug_t {
public:
	wkz_show_trees_t() : werkzeug_t() { id = WKZ_SHOW_TREES | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("hide trees"); }
	bool is_selected(const karte_t *) const { return umgebung_t::hide_trees; }
	bool init( karte_t *welt, spieler_t * ) {
		umgebung_t::hide_trees = !umgebung_t::hide_trees;
		welt->set_dirty();
		return false;
	}
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_show_houses_t : public werkzeug_t {
public:
	wkz_show_houses_t() : werkzeug_t() { id = WKZ_SHOW_HOUSES | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const {
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
	const char *get_tooltip(const spieler_t *) const;
	bool is_selected(const karte_t *) const;
	void draw_after( karte_t *w, koord pos ) const;
	bool init( karte_t *welt, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
	bool exit( karte_t *, spieler_t * ) { return false; }
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_rotate90_t : public werkzeug_t {
public:
	wkz_rotate90_t() : werkzeug_t() { id = WKZ_ROTATE90 | SIMPLE_TOOL; }
	image_id get_icon(spieler_t *) const { return umgebung_t::networkmode ? IMG_LEER : icon; }
	const char *get_tooltip(const spieler_t *) const { return umgebung_t::networkmode ? translator::translate("deactivated in online mode") : translator::translate("Rotate map"); }
	bool init( karte_t *welt, spieler_t * ) {
		if(  !umgebung_t::networkmode  ) {
			welt->rotate90();
			welt->update_map();
		}
		return false;
	}
	virtual bool is_init_network_save() const { return !umgebung_t::networkmode; }
	virtual bool is_work_network_save() const { return !umgebung_t::networkmode; }
};

class wkz_quit_t : public werkzeug_t {
public:
	wkz_quit_t() : werkzeug_t() { id = WKZ_QUIT | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Beenden"); }
	bool init( karte_t *welt, spieler_t * ) {
		destroy_all_win( true );
		welt->beenden( true );
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

// step size by default_param
class wkz_fill_trees_t : public werkzeug_t {
public:
	wkz_fill_trees_t() : werkzeug_t() { id = WKZ_FILL_TREES | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Fill trees"); }
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
	const char *get_tooltip(const spieler_t *) const;
	bool init( karte_t *, spieler_t * );
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};


/* change day/night view manually */
class wkz_vehicle_tooltips_t : public werkzeug_t {
public:
	wkz_vehicle_tooltips_t() : werkzeug_t() { id = WKZ_VEHICLE_TOOLTIPS | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("Toggle vehicle tooltips"); }
	bool init( karte_t *, spieler_t * ) {
		umgebung_t::show_vehicle_states = (umgebung_t::show_vehicle_states+1)%3;
		return false;
	}
	virtual bool is_init_network_save() const { return true; }
	virtual bool is_work_network_save() const { return true; }
};

class wkz_toggle_pax_station_t : public werkzeug_t {
public:
	wkz_toggle_pax_station_t() : werkzeug_t() { id = WKZ_TOOGLE_PAX | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("5LIGHT_CHOOSE"); }
	bool is_selected(const karte_t *welt) const { return welt->get_settings().get_show_pax(); }
	bool init( karte_t *welt, spieler_t * ) {
		settings_t& s = welt->get_settings();
		s.set_show_pax(!s.get_show_pax());
		return false;
	}
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	virtual bool is_init_network_save() const { return false; }
};

class wkz_toggle_pedestrians_t : public werkzeug_t {
public:
	wkz_toggle_pedestrians_t() : werkzeug_t() { id = WKZ_TOOGLE_PEDESTRIANS | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("6LIGHT_CHOOSE"); }
	bool is_selected(const karte_t *welt) const { return welt->get_settings().get_random_pedestrians(); }
	bool init( karte_t *welt, spieler_t * ) {
		settings_t& s = welt->get_settings();
		s.set_random_pedestrians(!s.get_random_pedestrians());
		return false;
	}
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	virtual bool is_init_network_save() const { return false; }
};

/* internal simple tools needed for networksynchronisation */
class wkz_traffic_level_t : public werkzeug_t {
public:
	wkz_traffic_level_t() : werkzeug_t() { id = WKZ_TRAFFIC_LEVEL | SIMPLE_TOOL; }
	const char *get_tooltip(const spieler_t *) const { return translator::translate("6WORLD_CHOOSE"); }
	bool is_selected(const karte_t *) const { return false; }
	bool init( karte_t *welt, spieler_t * ) {
		assert(  default_param  );
		welt->get_settings().set_verkehr_level(atoi(default_param));
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
};

// change timing of traffic light
class wkz_change_traffic_light_t : public werkzeug_t {
public:
	wkz_change_traffic_light_t() : werkzeug_t() { id = WKZ_TRAFFIC_LIGHT_TOOL | SIMPLE_TOOL; }
	virtual bool init( karte_t *, spieler_t * );
	virtual bool is_init_network_save() const { return false; }
};

// change city: (dis)allow growth
class wkz_change_city_t : public werkzeug_t {
public:
	wkz_change_city_t() : werkzeug_t() { id = WKZ_CHANGE_CITY_TOOL | SIMPLE_TOOL; }
	virtual bool init( karte_t *, spieler_t * );
	virtual bool is_init_network_save() const { return false; }
};

// internal tool: rename stuff
class wkz_rename_t : public werkzeug_t {
public:
	wkz_rename_t() : werkzeug_t() { id = WKZ_RENAME_TOOL | SIMPLE_TOOL; }
	virtual bool init( karte_t *, spieler_t * );
	virtual bool is_init_network_save() const { return false; }
};

// internal tool: send message (could be used for chats)
class wkz_add_message_t : public werkzeug_t {
public:
	wkz_add_message_t() : werkzeug_t() { id = WKZ_ADD_MESSAGE_TOOL | SIMPLE_TOOL; }
	virtual bool init( karte_t *, spieler_t * );
	virtual bool is_init_network_save() const { return false; }
};


#endif
