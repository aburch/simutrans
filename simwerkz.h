/*
 * New OO tool system
 *
 * This file is part of the Simutrans project under the artistic license.
 */

#ifndef simwerkz2_h
#define simwerkz2_h

#include "simtypes.h"
#include "simworld.h"
#include "simmenu.h"
#include "simdings.h"

#include "boden/wege/schiene.h"

#include "dataobj/umgebung.h"
#include "dataobj/translator.h"

#include "dings/baum.h"

#include "player/simplay.h"

#include "tpl/slist_tpl.h"

class koord3d;
class koord;
class wegbauer_t;
class haus_besch_t;
class roadsign_besch_t;
class weg_besch_t;
class route_t;
class way_obj_besch_t;

/****************************** helper functions: *****************************/

char *tooltip_with_price(const char * tip, sint64 price);

/************************ general tool *******************************/

// query tile info: default tool
class wkz_abfrage_t : public werkzeug_t {
public:
	wkz_abfrage_t() : werkzeug_t(WKZ_ABFRAGE | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Abfrage"); }
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};


// remove uppermost object from tile
class wkz_remover_t : public werkzeug_t {
private:
	static bool wkz_remover_intern(spieler_t *sp, karte_t *welt, koord3d pos, const char *&msg);
public:
	wkz_remover_t() : werkzeug_t(WKZ_REMOVER | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Abriss"); }
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
};

// alter land height tools
class wkz_raise_lower_base_t : public werkzeug_t {
protected:
	bool is_dragging;
	sint16 drag_height;

	bool drag(karte_t *welt, koord pos, sint16 h, int &n);
	virtual sint16 get_drag_height(karte_t *welt, koord pos) = 0;
	bool check_dragging();
public:
	wkz_raise_lower_base_t(uint16 id) : werkzeug_t(id | GENERAL_TOOL) { offset = Z_GRID; }
	image_id get_icon(spieler_t*) const OVERRIDE { return grund_t::underground_mode==grund_t::ugm_all ? IMG_LEER : icon; }
	bool init(karte_t*, spieler_t*) OVERRIDE { is_dragging = false; return true; }
	bool exit(karte_t*, spieler_t*) OVERRIDE { is_dragging = false; return true; }
	/**
	 * technically move is not network safe, however its implementation is:
	 * it sends work commands over network itself
	 */
	char const* move(karte_t*, spieler_t*, uint16 /* buttonstate */, koord3d) OVERRIDE;

	bool is_init_network_save() const OVERRIDE { return true; }
	/**
	 * work() is only called when not dragging
	 * if work() is called with is_dragging==true then is_dragging is reseted
	 */
	bool is_work_network_save() const OVERRIDE { return is_dragging;}

	/**
	 * @return true if this tool operates over the grid, not the map tiles.
	 */
	bool is_grid_tool() const {return true;}
};

class wkz_raise_t : public wkz_raise_lower_base_t {
public:
	wkz_raise_t() : wkz_raise_lower_base_t(WKZ_RAISE_LAND) {}
	char const* get_tooltip(spieler_t const* const sp) const OVERRIDE { return tooltip_with_price("Anheben", sp->get_welt()->get_settings().cst_alter_land); }
	char const* check_pos(karte_t*, spieler_t*, koord3d) OVERRIDE;
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	sint16 get_drag_height(karte_t *welt, koord pos) OVERRIDE;
};

class wkz_lower_t : public wkz_raise_lower_base_t {
public:
	wkz_lower_t() : wkz_raise_lower_base_t(WKZ_LOWER_LAND) {}
	char const* get_tooltip(spieler_t const* const sp) const OVERRIDE { return tooltip_with_price("Absenken", sp->get_welt()->get_settings().cst_alter_land); }
	char const* check_pos(karte_t*, spieler_t*, koord3d) OVERRIDE;
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	sint16 get_drag_height(karte_t *welt, koord pos) OVERRIDE;
};

/* slope tool definitions */
class wkz_setslope_t : public werkzeug_t {
public:
	wkz_setslope_t() : werkzeug_t(WKZ_SETSLOPE | GENERAL_TOOL) {}
	static const char *wkz_set_slope_work( karte_t *welt, spieler_t *sp, koord3d pos, int slope );
	char const* get_tooltip(spieler_t const* const sp) const OVERRIDE { return tooltip_with_price("Built artifical slopes", sp->get_welt()->get_settings().cst_set_slope); }
	bool is_init_network_save() const OVERRIDE { return true; }
	char const* check_pos(karte_t*, spieler_t*, koord3d) OVERRIDE;
	char const* work(karte_t* const welt, spieler_t* const sp, koord3d const k) OVERRIDE { return wkz_set_slope_work(welt, sp, k, atoi(default_param)); }
};

class wkz_restoreslope_t : public werkzeug_t {
public:
	wkz_restoreslope_t() : werkzeug_t(WKZ_RESTORESLOPE | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const* const sp) const OVERRIDE { return tooltip_with_price("Restore natural slope", sp->get_welt()->get_settings().cst_set_slope); }
	bool is_init_network_save() const OVERRIDE { return true; }
	char const* check_pos(karte_t*, spieler_t*, koord3d) OVERRIDE;
	char const* work(karte_t* const welt, spieler_t* const sp, koord3d const k) OVERRIDE { return wkz_setslope_t::wkz_set_slope_work(welt, sp, k, RESTORE_SLOPE); }
};

class wkz_marker_t : public kartenboden_werkzeug_t {
public:
	wkz_marker_t() : kartenboden_werkzeug_t(WKZ_MARKER | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const* const sp) const OVERRIDE { return tooltip_with_price("Marker", sp->get_welt()->get_settings().cst_buy_land); }
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
};

class wkz_clear_reservation_t : public werkzeug_t {
public:
	wkz_clear_reservation_t() : werkzeug_t(WKZ_CLEAR_RESERVATION | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Clear block reservation"); }
	bool init(karte_t*, spieler_t*) OVERRIDE;
	bool exit(karte_t*, spieler_t*) OVERRIDE;
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
};

class wkz_transformer_t : public kartenboden_werkzeug_t {
private:
	bool is_powerline_available( const karte_t * ) const;
public:
	wkz_transformer_t() : kartenboden_werkzeug_t(WKZ_TRANSFORMER | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE;
	image_id get_icon(spieler_t*) const OVERRIDE;
	bool init(karte_t*, spieler_t*) OVERRIDE;
	char const* check_pos(karte_t*, spieler_t*, koord3d) OVERRIDE;
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	waytype_t get_waytype() const OVERRIDE { return powerline_wt; }
};

class wkz_add_city_t : public kartenboden_werkzeug_t {
public:
	wkz_add_city_t() : kartenboden_werkzeug_t(WKZ_ADD_CITY | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const* const sp) const OVERRIDE { return tooltip_with_price("Found new city", sp->get_welt()->get_settings().cst_found_city); }
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
};

// buy a house to protect it from renovating
class wkz_buy_house_t : public kartenboden_werkzeug_t {
public:
	wkz_buy_house_t() : kartenboden_werkzeug_t(WKZ_BUY_HOUSE | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Haus kaufen"); }
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
};
/************** the following tools need a valid default_param ************************/

// step size by default_param
class wkz_change_city_size_t : public werkzeug_t {
public:
	wkz_change_city_size_t() : werkzeug_t(WKZ_CHANGE_CITY_SIZE | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate( atoi(default_param)>=0 ? "Grow city" : "Shrink city" ); }
	bool init(karte_t*, spieler_t*) OVERRIDE;
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
};

class wkz_plant_tree_t : public kartenboden_werkzeug_t {
public:
	wkz_plant_tree_t() : kartenboden_werkzeug_t(WKZ_PLANT_TREE | GENERAL_TOOL) {}
	image_id get_icon(spieler_t *) const { return baum_t::get_anzahl_besch() > 0 ? icon : IMG_LEER; }
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate( "Plant tree" ); }
	bool init(karte_t*, spieler_t*) { return baum_t::get_anzahl_besch() > 0; }
	char const* move(karte_t* const welt, spieler_t* const sp, uint16 const b, koord3d const k) OVERRIDE;
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
};

/* only called directly from schedule => no tooltip!
 * default_param must point to a schedule!
 */
class wkz_fahrplan_add_t : public werkzeug_t {
public:
	wkz_fahrplan_add_t() : werkzeug_t(WKZ_FAHRPLAN_ADD | GENERAL_TOOL) {}
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

class wkz_fahrplan_ins_t : public werkzeug_t {
public:
	wkz_fahrplan_ins_t() : werkzeug_t(WKZ_FAHRPLAN_INS | GENERAL_TOOL) {}
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

class wkz_wegebau_t : public two_click_werkzeug_t {
private:
	static const weg_besch_t *defaults[17];	// default ways for all types

	char const* do_work(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	void mark_tiles(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	uint8 is_valid_pos(karte_t*, spieler_t*, koord3d const&, char const*&, koord3d const&) OVERRIDE;

protected:
	static karte_t *welt;
	const weg_besch_t *besch;

	virtual weg_besch_t const* get_besch(uint16, bool) const;
	void calc_route( wegbauer_t &bauigel, const koord3d &, const koord3d & );

public:
	wkz_wegebau_t(uint16 const id = WKZ_WEGEBAU | GENERAL_TOOL) : two_click_werkzeug_t(id), besch() {}
	image_id get_icon(spieler_t*) const OVERRIDE;
	char const* get_tooltip(spieler_t const*) const OVERRIDE;
	char const* get_default_param(spieler_t*) const OVERRIDE;
	bool is_selected(karte_t const*) const OVERRIDE;
	bool init(karte_t*, spieler_t*) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	waytype_t get_waytype() const OVERRIDE;
	// remove preview necessary while building elevated ways
	bool remove_preview_necessary() const OVERRIDE { return !is_first_click()  &&  (besch  &&  (besch->get_styp() == 1  &&  besch->get_wtyp() != air_wt)); }
};

class wkz_build_cityroad : public wkz_wegebau_t {
private:
	char const* do_work(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
public:
	wkz_build_cityroad() : wkz_wegebau_t(WKZ_CITYROAD | GENERAL_TOOL) {}
	weg_besch_t const* get_besch(uint16, bool) const OVERRIDE;
	image_id get_icon(spieler_t* const sp) const OVERRIDE { return werkzeug_t::get_icon(sp); }
	bool is_selected(karte_t const*) const OVERRIDE { return werkzeug_t::is_selected(welt); }
	bool is_init_network_save() const OVERRIDE { return true; }
	waytype_t get_waytype() const OVERRIDE { return road_wt; }
};

class wkz_brueckenbau_t : public two_click_werkzeug_t {
private:
	ribi_t::ribi ribi;

	char const* do_work(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	void mark_tiles(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	uint8 is_valid_pos(karte_t*, spieler_t*, koord3d const&, char const*&, koord3d const&) OVERRIDE;
public:
	wkz_brueckenbau_t() : two_click_werkzeug_t(WKZ_BRUECKENBAU | GENERAL_TOOL) {}
	image_id get_icon(spieler_t*) const OVERRIDE { return grund_t::underground_mode==grund_t::ugm_all ? IMG_LEER : icon; }
	char const* get_tooltip(spieler_t const*) const OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	waytype_t get_waytype() const OVERRIDE;
	bool remove_preview_necessary() const OVERRIDE { return !is_first_click(); }
	void rdwr_custom_data(uint8 player_nr, memory_rw_t*) OVERRIDE;
};

class wkz_tunnelbau_t : public two_click_werkzeug_t {
private:
	void calc_route( wegbauer_t &bauigel, const koord3d &, const koord3d &, karte_t* );
	char const* do_work(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	void mark_tiles(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	uint8 is_valid_pos(karte_t*, spieler_t*, koord3d const&, char const*&, koord3d const&) OVERRIDE;
public:
	wkz_tunnelbau_t() : two_click_werkzeug_t(WKZ_TUNNELBAU | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE;
	char const* check_pos(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	waytype_t get_waytype() const OVERRIDE;
	bool remove_preview_necessary() const OVERRIDE { return !is_first_click(); }
};

class wkz_wayremover_t : public two_click_werkzeug_t {
private:
	bool calc_route( route_t &, spieler_t *, const koord3d& start, const koord3d &to );
	char const* do_work(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	void mark_tiles(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	uint8 is_valid_pos(karte_t*, spieler_t*, koord3d const&, char const*&, koord3d const&) OVERRIDE;
	// error message to be returned by calc_route
	const char *calc_route_error;
public:
	wkz_wayremover_t() : two_click_werkzeug_t(WKZ_WAYREMOVER | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE;
	image_id get_icon(spieler_t*) const OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	waytype_t get_waytype() const OVERRIDE;
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

	char const* do_work(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	void mark_tiles(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	uint8 is_valid_pos(karte_t*, spieler_t*, koord3d const&, char const*&, koord3d const&) OVERRIDE;

public:
	wkz_wayobj_t(uint16 const id = WKZ_WAYOBJ | GENERAL_TOOL, bool b = true) : two_click_werkzeug_t(id), build(b) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE;
	bool is_selected(karte_t const*) const OVERRIDE;
	bool init(karte_t*, spieler_t*) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	waytype_t get_waytype() const OVERRIDE;
};

class wkz_wayobj_remover_t : public wkz_wayobj_t {
public:
	wkz_wayobj_remover_t() : wkz_wayobj_t(WKZ_REMOVE_WAYOBJ | GENERAL_TOOL, false) {}
	bool is_selected(karte_t const* const welt) const OVERRIDE { return werkzeug_t::is_selected(welt); }
	bool is_init_network_save() const OVERRIDE { return true; }
};

class wkz_station_t : public werkzeug_t {
private:
	static char toolstring[256];
	const char *wkz_station_building_aux( karte_t *, spieler_t *, bool, koord3d, const haus_besch_t *, sint8 rotation );
	const char *wkz_station_dock_aux( karte_t *, spieler_t *, koord3d, const haus_besch_t * );
	const char *wkz_station_aux( karte_t *, spieler_t *, koord3d, const haus_besch_t *, waytype_t, sint64 cost, const char *halt_suffix );
	const haus_besch_t *get_besch( sint8 &rotation ) const;

public:
	wkz_station_t() : werkzeug_t(WKZ_STATION | GENERAL_TOOL) {}
	image_id get_icon(spieler_t*) const OVERRIDE;
	char const* get_tooltip(spieler_t const*) const OVERRIDE;
	bool init(karte_t*, spieler_t*) OVERRIDE;
	char const* check_pos(karte_t*, spieler_t*, koord3d) OVERRIDE;
	char const* move(karte_t*, spieler_t*, uint16 /* buttonstate */, koord3d) OVERRIDE;
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	waytype_t get_waytype() const OVERRIDE;
};

// builds roadsigns and signals
class wkz_roadsign_t : public two_click_werkzeug_t {
private:
	const roadsign_besch_t* besch;
	const char *place_sign_intern( karte_t *, spieler_t *, grund_t*, const roadsign_besch_t* b = NULL);

	struct signal_info {
		signal_info() : spacing(2), remove_intermediate(true), replace_other(true) {}

		uint8 spacing; // place signals every n tiles
		bool  remove_intermediate;
		bool  replace_other;
	} signal[MAX_PLAYER_COUNT];

	static char toolstring[256];
	// read the variables from the default_param
	void read_default_param(spieler_t *sp);

	const char* check_pos_intern(karte_t *, spieler_t *, koord3d);
	bool calc_route( route_t &, spieler_t *, const koord3d& start, const koord3d &to );

	char const* do_work(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	void mark_tiles(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	uint8 is_valid_pos(karte_t*, spieler_t*, koord3d const&, char const*&, koord3d const&) OVERRIDE;

	/// save direction of new signs
	vector_tpl< ribi_t::ribi > directions;

public:
	wkz_roadsign_t() : two_click_werkzeug_t(WKZ_ROADSIGN | GENERAL_TOOL), besch() {}

	char const* get_tooltip(spieler_t const*) const OVERRIDE;
	bool init(karte_t*, spieler_t*) OVERRIDE;
	bool exit(karte_t*, spieler_t*) OVERRIDE;

	void set_values(spieler_t *sp, uint8 spacing, bool remove, bool replace );
	void get_values(spieler_t *sp, uint8 &spacing, bool &remove, bool &replace );
	bool is_init_network_save() const OVERRIDE { return true; }
	void draw_after(karte_t*, koord) const OVERRIDE;
	char const* get_default_param(spieler_t*) const OVERRIDE;
	waytype_t get_waytype() const OVERRIDE;
};

class wkz_depot_t : public werkzeug_t {
private:
	static char toolstring[256];
	const char *wkz_depot_aux(karte_t *welt, spieler_t *sp, koord3d pos, const haus_besch_t *besch, waytype_t wegtype, sint64 cost);
public:
	wkz_depot_t() : werkzeug_t(WKZ_DEPOT | GENERAL_TOOL) {}
	image_id get_icon(spieler_t*) const OVERRIDE;
	char const* get_tooltip(spieler_t const*) const OVERRIDE;
	bool init(karte_t*, spieler_t*) OVERRIDE;
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	waytype_t get_waytype() const OVERRIDE;
};

/* builds (random) tourist attraction (default_param==NULL) and maybe adds it to the next city
 * the parameter string is a follow (or NULL):
 * 1#theater
 * first letter: ignore climates
 * second letter: rotation (0,1,2,3,#=random)
 * finally building name
 * @author prissi
 */
class wkz_build_haus_t : public kartenboden_werkzeug_t {
public:
	wkz_build_haus_t() : kartenboden_werkzeug_t(WKZ_BUILD_HAUS | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Built random attraction"); }
	bool init(karte_t*, spieler_t*) OVERRIDE;
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
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
	wkz_build_industries_land_t() : kartenboden_werkzeug_t(WKZ_LAND_CHAIN | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Build land consumer"); }
	bool init(karte_t*, spieler_t*) OVERRIDE;
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
};

class wkz_build_industries_city_t : public kartenboden_werkzeug_t {
public:
	wkz_build_industries_city_t() : kartenboden_werkzeug_t(WKZ_CITY_CHAIN | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Build city market"); }
	bool init(karte_t*, spieler_t*) OVERRIDE;
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
};

class wkz_build_factory_t : public kartenboden_werkzeug_t {
public:
	wkz_build_factory_t() : kartenboden_werkzeug_t(WKZ_BUILD_FACTORY | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Build city market"); }
	bool init(karte_t*, spieler_t*) OVERRIDE;
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
};

class wkz_link_factory_t : public two_click_werkzeug_t {
public:
	wkz_link_factory_t() : two_click_werkzeug_t(WKZ_LINK_FACTORY | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Connect factory"); }
	bool is_init_network_save() const OVERRIDE { return true; }
private:
	char const* do_work(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	void mark_tiles(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE {}
	uint8 is_valid_pos(karte_t*, spieler_t*, koord3d const&, char const*&, koord3d const&) OVERRIDE;
	image_id get_marker_image() OVERRIDE;
};

class wkz_headquarter_t : public kartenboden_werkzeug_t {
private:
	const haus_besch_t *next_level( const spieler_t *sp ) const;
public:
	wkz_headquarter_t() : kartenboden_werkzeug_t(WKZ_HEADQUARTER | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE;
	bool init(karte_t*, spieler_t*) OVERRIDE;
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
};

/* protects map from further change (here because two clicks to confirm it!) */
class wkz_lock_game_t : public werkzeug_t {
public:
	wkz_lock_game_t() : werkzeug_t(WKZ_LOCK_GAME | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return umgebung_t::networkmode ? translator::translate("deactivated in online mode") : translator::translate("Lock game"); }
	image_id get_icon(spieler_t*) const OVERRIDE { return umgebung_t::networkmode ? IMG_LEER : icon; }
	// deactivate in network mode
	bool init( karte_t *, spieler_t *) { return !umgebung_t::networkmode; }
	const char *work( karte_t *welt, spieler_t *, koord3d );
	bool is_init_network_save() const OVERRIDE { return true; }
};

/* add random citycar if no default is set; else add a certain city car */
class wkz_add_citycar_t : public werkzeug_t {
public:
	wkz_add_citycar_t() : werkzeug_t(WKZ_ADD_CITYCAR | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Add random citycar"); }
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
};

/* make forest */
class wkz_forest_t : public two_click_werkzeug_t {
public:
	wkz_forest_t() : two_click_werkzeug_t(WKZ_FOREST | GENERAL_TOOL) {}
	image_id get_icon(spieler_t *) const { return baum_t::get_anzahl_besch() > 0 ? icon : IMG_LEER; }
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Add forest"); }
	bool init( karte_t *welt, spieler_t *sp) { return  baum_t::get_anzahl_besch() > 0  &&  two_click_werkzeug_t::init(welt, sp); }
private:
	char const* do_work(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	void mark_tiles(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	uint8 is_valid_pos(karte_t*, spieler_t*, koord3d const&, char const*&, koord3d const&) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
};

/* stop moving tool */
class wkz_stop_moving_t : public two_click_werkzeug_t {
private:
	waytype_t waytype[2];
	halthandle_t last_halt;
public:
	wkz_stop_moving_t() : two_click_werkzeug_t(WKZ_STOP_MOVER | GENERAL_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("replace stop"); }
	bool is_init_network_save() const OVERRIDE { return true; }

private:
	char const* do_work(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE;
	void mark_tiles(karte_t*, spieler_t*, koord3d const&, koord3d const&) OVERRIDE {}
	uint8 is_valid_pos(karte_t*, spieler_t*, koord3d const&, char const*&, koord3d const&) OVERRIDE;
	image_id get_marker_image() OVERRIDE;

	void read_start_position(karte_t *welt, spieler_t *sp, const koord3d &pos);
};

/* make all tiles of this player a public stop
 * if this player is public, make all connected tiles a public stop */
class wkz_make_stop_public_t : public werkzeug_t {
public:
	wkz_make_stop_public_t() : werkzeug_t(WKZ_MAKE_STOP_PUBLIC | GENERAL_TOOL) {}
	bool init( karte_t *, spieler_t * );
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	char const* get_tooltip(spieler_t const*) const OVERRIDE;
	char const* move(karte_t*, spieler_t*, uint16 /* buttonstate */, koord3d) OVERRIDE;
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
};



// internal tool: show error message at specific coordinate
// used for scenario error messages send by server
class wkz_error_message_t : public werkzeug_t {
public:
	wkz_error_message_t() : werkzeug_t(WKZ_ERR_MESSAGE_TOOL | GENERAL_TOOL) {}
	bool init(karte_t*, spieler_t*) OVERRIDE { return true; }
	bool is_init_network_save() const OVERRIDE { return true; }
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE { return default_param ? default_param : ""; }
};

/********************* one click tools ****************************/

class wkz_pause_t : public werkzeug_t {
public:
	wkz_pause_t() : werkzeug_t(WKZ_PAUSE | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return umgebung_t::networkmode ? translator::translate("deactivated in online mode") : translator::translate("Pause"); }
	image_id get_icon(spieler_t*) const OVERRIDE { return umgebung_t::networkmode ? IMG_LEER : icon; }
	bool is_selected(karte_t const* const welt) const OVERRIDE { return welt->is_paused(); }
	bool init( karte_t *welt, spieler_t * ) {
		if(  !umgebung_t::networkmode  ) {
			welt->set_fast_forward(0);
			welt->set_pause( welt->is_paused()^1 );
		}
		return false;
	}
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	bool is_init_network_save() const OVERRIDE { return !umgebung_t::networkmode; }
	bool is_work_network_save() const OVERRIDE { return !umgebung_t::networkmode; }
};

class wkz_fastforward_t : public werkzeug_t {
public:
	wkz_fastforward_t() : werkzeug_t(WKZ_FASTFORWARD | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return umgebung_t::networkmode ? translator::translate("deactivated in online mode") : translator::translate("Fast forward"); }
	image_id get_icon(spieler_t*) const OVERRIDE { return umgebung_t::networkmode ? IMG_LEER : icon; }
	bool is_selected(karte_t const* const welt) const OVERRIDE { return welt->is_fast_forward(); }
	bool init( karte_t *welt, spieler_t * ) {
		if(  !umgebung_t::networkmode  ) {
			welt->set_pause(0);
			welt->set_fast_forward( welt->is_fast_forward()^1 );
		}
		return false;
	}
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	bool is_init_network_save() const OVERRIDE { return !umgebung_t::networkmode; }
	bool is_work_network_save() const OVERRIDE { return !umgebung_t::networkmode; }
};

class wkz_screenshot_t : public werkzeug_t {
public:
	wkz_screenshot_t() : werkzeug_t(WKZ_SCREENSHOT | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Screenshot"); }
	bool init( karte_t *, spieler_t * ) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// builts next chain
class wkz_increase_industry_t : public werkzeug_t {
public:
	wkz_increase_industry_t() : werkzeug_t(WKZ_INCREASE_INDUSTRY | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Increase Industry density"); }
	bool init( karte_t *welt, spieler_t * );
};

/* prissi: undo building */
class wkz_undo_t : public werkzeug_t {
public:
	wkz_undo_t() : werkzeug_t(WKZ_UNDO | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Undo last ways construction"); }
	bool init( karte_t *, spieler_t *sp ) OVERRIDE;
};

/* switch to next player
 * @author prissi
 */
class wkz_switch_player_t : public werkzeug_t {
public:
	wkz_switch_player_t() : werkzeug_t(WKZ_SWITCH_PLAYER | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Change player"); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->switch_active_player( welt->get_active_player_nr()+1, true );
		return false;
	}
	// since it is handled internally
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// setp one year forward
class wkz_step_year_t : public werkzeug_t {
public:
	wkz_step_year_t() : werkzeug_t(WKZ_STEP_YEAR | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Step timeline one year"); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->step_year();
		return false;
	}
};

class wkz_change_game_speed_t : public werkzeug_t {
public:
	wkz_change_game_speed_t() : werkzeug_t(WKZ_CHANGE_GAME_SPEED | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE {
		int faktor = atoi(default_param);
		return faktor>0 ? translator::translate("Accelerate time") : translator::translate("Deccelerate time");
	}
	bool init( karte_t *welt, spieler_t * ) {
		if(  !umgebung_t::networkmode  ) {
			welt->change_time_multiplier( atoi(default_param) );
		}
		return false;
	}
};


class wkz_zoom_in_t : public werkzeug_t {
public:
	wkz_zoom_in_t() : werkzeug_t(WKZ_ZOOM_IN | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("zooming in"); }
	bool init( karte_t *welt, spieler_t * ) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

class wkz_zoom_out_t : public werkzeug_t {
public:
	wkz_zoom_out_t() : werkzeug_t(WKZ_ZOOM_OUT | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("zooming out"); }
	bool init( karte_t *welt, spieler_t * ) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

class wkz_show_coverage_t : public werkzeug_t {
public:
	wkz_show_coverage_t() : werkzeug_t(WKZ_SHOW_COVERAGE | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("show station coverage"); }
	bool is_selected(karte_t const*) const OVERRIDE { return umgebung_t::station_coverage_show; }
	bool init( karte_t *welt, spieler_t * ) {
		umgebung_t::station_coverage_show = !umgebung_t::station_coverage_show;
		welt->set_dirty();
		return false;
	}
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

class wkz_show_name_t : public werkzeug_t {
public:
	wkz_show_name_t() : werkzeug_t(WKZ_SHOW_NAMES | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE {
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
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

class wkz_show_grid_t : public werkzeug_t {
public:
	wkz_show_grid_t() : werkzeug_t(WKZ_SHOW_GRID | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("show grid"); }
	bool is_selected(karte_t const*) const OVERRIDE { return grund_t::show_grid; }
	bool init( karte_t *welt, spieler_t * ) {
		grund_t::toggle_grid();
		welt->set_dirty();
		return false;
	}
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

class wkz_show_trees_t : public werkzeug_t {
public:
	wkz_show_trees_t() : werkzeug_t(WKZ_SHOW_TREES | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("hide trees"); }
	bool is_selected(karte_t const*) const OVERRIDE {return umgebung_t::hide_trees; }
	bool init( karte_t *welt, spieler_t * );
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

class wkz_show_houses_t : public werkzeug_t {
public:
	wkz_show_houses_t() : werkzeug_t(WKZ_SHOW_HOUSES | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE {
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
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

class wkz_show_underground_t : public werkzeug_t {
public:
	wkz_show_underground_t() : werkzeug_t(WKZ_SHOW_UNDERGROUND | SIMPLE_TOOL) {}
	static sint8 save_underground_level;
	char const* get_tooltip(spieler_t const*) const OVERRIDE;
	bool is_selected(karte_t const*) const OVERRIDE;
	void draw_after(karte_t*, koord) const OVERRIDE;
	bool init( karte_t *welt, spieler_t * );
	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	bool exit( karte_t *, spieler_t * ) { return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

class wkz_rotate90_t : public werkzeug_t {
public:
	wkz_rotate90_t() : werkzeug_t(WKZ_ROTATE90 | SIMPLE_TOOL) {}
	image_id get_icon(spieler_t*) const OVERRIDE { return umgebung_t::networkmode ? IMG_LEER : icon; }
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return umgebung_t::networkmode ? translator::translate("deactivated in online mode") : translator::translate("Rotate map"); }
	bool init( karte_t *welt, spieler_t * ) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return !umgebung_t::networkmode; }
	bool is_work_network_save() const OVERRIDE { return !umgebung_t::networkmode; }
};

class wkz_quit_t : public werkzeug_t {
public:
	wkz_quit_t() : werkzeug_t(WKZ_QUIT | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Beenden"); }
	bool init( karte_t *welt, spieler_t * ) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

// step size by default_param
class wkz_fill_trees_t : public werkzeug_t {
public:
	wkz_fill_trees_t() : werkzeug_t(WKZ_FILL_TREES | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Fill trees"); }
	image_id get_icon(spieler_t *) const { return baum_t::get_anzahl_besch() > 0 ? icon : IMG_LEER; }
	bool init( karte_t *welt, spieler_t * ) {
		if(  baum_t::get_anzahl_besch() > 0  &&  default_param  ) {
			baum_t::fill_trees( welt, atoi(default_param) );
		}
		return false;
	}
};

/* change day/night view manually */
class wkz_daynight_level_t : public werkzeug_t {
public:
	wkz_daynight_level_t() : werkzeug_t(WKZ_DAYNIGHT_LEVEL | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE;
	bool init( karte_t *, spieler_t * );
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

class wkz_vehicle_tooltips_t : public werkzeug_t {
public:
	wkz_vehicle_tooltips_t() : werkzeug_t(WKZ_VEHICLE_TOOLTIPS | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("Toggle vehicle tooltips"); }
	bool init( karte_t *welt, spieler_t * ) {
		umgebung_t::show_vehicle_states = (umgebung_t::show_vehicle_states+1)%3;
		welt->set_dirty();
		return false;
	}
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

class wkz_toggle_pax_station_t : public werkzeug_t {
public:
	wkz_toggle_pax_station_t() : werkzeug_t(WKZ_TOOGLE_PAX | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("5LIGHT_CHOOSE"); }
	bool is_selected(karte_t const* const welt) const OVERRIDE { return welt->get_settings().get_show_pax(); }
	bool init( karte_t *welt, spieler_t * ) {
		if( !umgebung_t::networkmode) {
			settings_t& s = welt->get_settings();
			s.set_show_pax(!s.get_show_pax());
		}
		return false;
	}
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	bool is_init_network_save() const OVERRIDE { return false; }
};

class wkz_toggle_pedestrians_t : public werkzeug_t {
public:
	wkz_toggle_pedestrians_t() : werkzeug_t(WKZ_TOOGLE_PEDESTRIANS | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("6LIGHT_CHOOSE"); }
	bool is_selected(karte_t const* const welt) const OVERRIDE { return welt->get_settings().get_random_pedestrians(); }
	bool init( karte_t *welt, spieler_t * ) {
		if( !umgebung_t::networkmode) {
			settings_t& s = welt->get_settings();
			s.set_random_pedestrians(!s.get_random_pedestrians());
		}
		return false;
	}
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	bool is_init_network_save() const OVERRIDE { return false; }
};

class wkz_toggle_reservation_t : public werkzeug_t {
public:
	wkz_toggle_reservation_t() : werkzeug_t(WKZ_TOGGLE_RESERVATION | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("show/hide block reservations"); }
	bool is_selected(karte_t const*) const OVERRIDE { return schiene_t::show_reservations; }
	bool init( karte_t *welt, spieler_t * ) {
		schiene_t::show_reservations ^= 1;
		welt->set_dirty();
		return false;
	}
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

class wkz_view_owner_t : public werkzeug_t {
public:
	wkz_view_owner_t() : werkzeug_t(WKZ_VIEW_OWNER | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("show/hide object owner"); }
	bool is_selected(karte_t const*) const OVERRIDE { return ding_t::show_owner; }
	bool init( karte_t *welt, spieler_t * ) {
		ding_t::show_owner ^= 1;
		welt->set_dirty();
		return false;
	}
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

class wkz_hide_under_cursor_t : public werkzeug_t {
public:
	wkz_hide_under_cursor_t() : werkzeug_t(WKZ_HIDE_UNDER_CURSOR | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("hide objects under cursor"); }
	bool is_selected(karte_t const*) const OVERRIDE { return umgebung_t::hide_under_cursor; }
	bool init( karte_t *welt, spieler_t * ) {
		umgebung_t::hide_under_cursor = !umgebung_t::hide_under_cursor  &&  umgebung_t::cursor_hide_range>0;
		welt->set_dirty();
		return false;
	}
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
};

/******************************** Internal tools ***********/
/* internal simple tools needed for networksynchronisation */
class wkz_traffic_level_t : public werkzeug_t {
public:
	wkz_traffic_level_t() : werkzeug_t(WKZ_TRAFFIC_LEVEL | SIMPLE_TOOL) {}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate("6WORLD_CHOOSE"); }
	bool is_selected(karte_t const*) const OVERRIDE { return false; }
	bool init( karte_t *welt, spieler_t * ) {
		assert(  default_param  );
		sint16 level = min( max( atoi(default_param), 0), 16);
		welt->get_settings().set_verkehr_level(level);
		return false;
	}
	bool is_init_network_save() const OVERRIDE { return false; }
};

class wkz_change_convoi_t : public werkzeug_t {
public:
	wkz_change_convoi_t() : werkzeug_t(WKZ_CONVOI_TOOL | SIMPLE_TOOL) {}
	bool init(karte_t*, spieler_t*) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return false; }
};

class wkz_change_line_t : public werkzeug_t {
public:
	wkz_change_line_t() : werkzeug_t(WKZ_LINE_TOOL | SIMPLE_TOOL) {}
	bool init(karte_t*, spieler_t*) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return false; }
};

class wkz_change_depot_t : public werkzeug_t {
public:
	wkz_change_depot_t() : werkzeug_t(WKZ_DEPOT_TOOL | SIMPLE_TOOL) {}
	bool init(karte_t*, spieler_t*) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return false; }
};

// adds a new player of certain type to the map
class wkz_change_player_t : public werkzeug_t {
public:
	wkz_change_player_t() : werkzeug_t(WKZ_SET_PLAYER_TOOL | SIMPLE_TOOL) {}
	bool init(karte_t*, spieler_t*) OVERRIDE;
};

// change timing of traffic light
class wkz_change_traffic_light_t : public werkzeug_t {
public:
	wkz_change_traffic_light_t() : werkzeug_t(WKZ_TRAFFIC_LIGHT_TOOL | SIMPLE_TOOL) {}
	bool init(karte_t*, spieler_t*) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return false; }
};

// change city: (dis)allow growth
class wkz_change_city_t : public werkzeug_t {
public:
	wkz_change_city_t() : werkzeug_t(WKZ_CHANGE_CITY_TOOL | SIMPLE_TOOL) {}
	bool init(karte_t*, spieler_t*) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return false; }
};

// internal tool: rename stuff
class wkz_rename_t : public werkzeug_t {
public:
	wkz_rename_t() : werkzeug_t(WKZ_RENAME_TOOL | SIMPLE_TOOL) {}
	bool init(karte_t*, spieler_t*) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return false; }
};

// internal tool: send message
class wkz_add_message_t : public werkzeug_t {
public:
	wkz_add_message_t() : werkzeug_t(WKZ_ADD_MESSAGE_TOOL | SIMPLE_TOOL) {}
	bool init(karte_t*, spieler_t*) OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return false; }
};


#endif
