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

#include "tpl/slist_tpl.h"

class spieler_t;
class koord3d;
class koord;
class toolbar_t;
class werkzeug_waehler_t;
class wegbauer_t;
class weg_besch_t;

/****************************** helper functions: *****************************/

char *tooltip_with_price(const char * tip, sint64 price);

/************************ general tool *******************************/

// query tile info: default tool
class wkz_abfrage_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Abfrage"); }
	const char *work( karte_t *, spieler_t *, koord3d );
};


// remove uppermost object from tile
class wkz_remover_t : public werkzeug_t {
private:
	static bool wkz_remover_intern(spieler_t *sp, karte_t *welt, koord3d pos, const char *&msg);
public:
	const char *get_tooltip(spieler_t *) { return translator::translate("Abriss"); }
	const char *work( karte_t *, spieler_t *, koord3d );
};

// alter land height tools
class wkz_raise_t : public werkzeug_t {
private:
	bool is_dragging;
	sint16 drag_height;
public:
	wkz_raise_t() : werkzeug_t() { offset = Z_GRID; }
	const char *get_tooltip(spieler_t *sp) { return tooltip_with_price("Anheben", sp->get_welt()->get_einstellungen()->cst_alter_land); }
	virtual image_id get_icon(spieler_t *sp) const { return grund_t::underground_mode==grund_t::ugm_all ? IMG_LEER : icon; }
	bool init( karte_t *, spieler_t * ) { is_dragging = false; return true; }
	bool exit( karte_t *, spieler_t * ) { is_dragging = false; return true; }
	const char *work( karte_t *, spieler_t *, koord3d );
	const char *move( karte_t *, spieler_t *, uint16 /* buttonstate */, koord3d );
};

class wkz_lower_t : public werkzeug_t {
private:
	bool is_dragging;
	sint16 drag_height;
public:
	wkz_lower_t() : werkzeug_t() { offset = Z_GRID; }
	const char *get_tooltip(spieler_t *sp) { return tooltip_with_price("Absenken", sp->get_welt()->get_einstellungen()->cst_alter_land); }
	virtual image_id get_icon(spieler_t *sp) const { return grund_t::underground_mode==grund_t::ugm_all ? IMG_LEER : icon; }
	bool init( karte_t *, spieler_t * ) { is_dragging = false; return true; }
	bool exit( karte_t *, spieler_t * ) { is_dragging = false; return true; }
	virtual const char *work( karte_t *, spieler_t *, koord3d);
	virtual const char *move( karte_t *, spieler_t *, uint16 /* buttonstate */, koord3d );
};

/* slope tool definitions */
class wkz_setslope_t : public werkzeug_t {
public:
	static const char *wkz_set_slope_work( karte_t *welt, spieler_t *sp, koord3d pos, int slope );
	const char *get_tooltip(spieler_t *sp) { return tooltip_with_price("Built artifical slopes", sp->get_welt()->get_einstellungen()->cst_set_slope); }
	virtual const char *work( karte_t *welt, spieler_t *sp, koord3d k ) { return wkz_set_slope_work( welt, sp, k, atoi(default_param) ); }
};

class wkz_restoreslope_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *sp) { return tooltip_with_price("Restore natural slope", sp->get_welt()->get_einstellungen()->cst_set_slope); }
	virtual const char *work( karte_t *welt, spieler_t *sp, koord3d k ) { return wkz_setslope_t::wkz_set_slope_work( welt, sp, k, RESTORE_SLOPE ); }
};

class wkz_marker_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *sp) { return tooltip_with_price("Marker", sp->get_welt()->get_einstellungen()->cst_buy_land); }
	virtual const char *work( karte_t *welt, spieler_t *sp, koord3d k );
};

class wkz_clear_reservation_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Clear block reservation"); }
	bool init( karte_t *, spieler_t * );
	bool exit( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

class wkz_transformer_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *);
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

class wkz_add_city_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *sp) { return tooltip_with_price( "Found new city", sp->get_welt()->get_einstellungen()->cst_found_city ); }
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

// buy a house to protext it from renovating
class wkz_buy_house_t : public werkzeug_t {
public:
	const char *get_tooltip(spieler_t *) { return translator::translate("Haus kaufen"); }
	const char *work( karte_t *, spieler_t *, koord3d );
};
/************** the following tools need a valid default_param ************************/

// step size by default_param
class wkz_change_city_size_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate( atoi(default_param)>=0 ? "Grow city" : "Shrink city" ); }
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

class wkz_plant_tree_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate( "Plant tree" ); }
	virtual const char *move( karte_t *welt, spieler_t *sp, uint16 b, koord3d k ) { return b==1 ? work(welt,sp,k) : NULL; }
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

/* only called directly from schedule => no tooltip!
 * default_param must point to a schedule!
 */
class wkz_fahrplan_add_t : public werkzeug_t {
	virtual const char *work( karte_t *welt, spieler_t *sp, koord3d k );
};

class wkz_fahrplan_ins_t : public werkzeug_t {
	virtual const char *work( karte_t *welt, spieler_t *sp, koord3d k );
};

class wkz_wegebau_t : public two_click_werkzeug_t {
private:
	static const weg_besch_t *defaults[17];	// default ways for all types
	const weg_besch_t *besch;
	const weg_besch_t *get_besch(karte_t *,bool) const;
public:
	virtual image_id get_icon(spieler_t *) const { return grund_t::underground_mode==grund_t::ugm_all ? IMG_LEER : icon; }
	virtual const char *get_tooltip(spieler_t *);
	virtual bool is_selected( karte_t *welt ) const;
	virtual bool init( karte_t *, spieler_t * );

private:
	void calc_route( wegbauer_t &bauigel, const koord3d &, const koord3d & );

	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual const char *valid_pos( karte_t *, spieler_t *, const koord3d & );
};

class wkz_brueckenbau_t : public werkzeug_t {
	virtual image_id get_icon(spieler_t *) const { return grund_t::underground_mode==grund_t::ugm_all ? IMG_LEER : icon; }
	const char *get_tooltip(spieler_t *);
	const char *work( karte_t *welt, spieler_t *sp, koord3d k );
};

class wkz_tunnelbau_t : public werkzeug_t {
private:
	koord3d start;
	zeiger_t *wkz_tunnelbau_bauer;
public:
	wkz_tunnelbau_t() : werkzeug_t() { wkz_tunnelbau_bauer=NULL; }
	const char *get_tooltip(spieler_t *);
	bool init( karte_t *, spieler_t * );
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	const char *work( karte_t *welt, spieler_t *sp, koord3d k );
};

class wkz_wayremover_t : public two_click_werkzeug_t {
private:
	bool calc_route( route_t &, spieler_t *, const koord3d& start, const koord3d &to );
public:
	const char *get_tooltip(spieler_t *);
private:
	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual const char *valid_pos( karte_t *, spieler_t *, const koord3d & );
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
	virtual const char *valid_pos( karte_t *, spieler_t *, const koord3d & );

public:
	wkz_wayobj_t(bool b=true) : two_click_werkzeug_t(), build(b) {};
	virtual const char *get_tooltip(spieler_t *);
	virtual bool is_selected(karte_t *welt) const;
	virtual bool init( karte_t *, spieler_t * );
};

class wkz_wayobj_remover_t : public wkz_wayobj_t {
public:
	wkz_wayobj_remover_t() : wkz_wayobj_t(false) {}
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
	virtual image_id get_icon(spieler_t *) const;
	const char *get_tooltip(spieler_t *);
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

class wkz_roadsign_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *);
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

class wkz_depot_t : public werkzeug_t {
private:
	static char toolstring[256];
	const char *wkz_depot_aux(karte_t *welt, spieler_t *sp, koord3d pos, const haus_besch_t *besch, waytype_t wegtype, sint64 cost);
public:
	virtual image_id get_icon(spieler_t *sp) const { return sp->get_player_nr()==1 ? IMG_LEER : icon; }
	const char *get_tooltip(spieler_t *);
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

/* builds (random) tourist attraction (deafult_param==NULL) and maybe adds it to the next city
 * the parameter string is a follow (or NULL):
 * 1#theater
 * first letter: ignore climates
 * second letter: rotation (0,1,2,3,#=random)
 * finally building name
 * @author prissi
 */
class wkz_build_haus_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Built random attraction"); }
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
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
class wkz_build_industries_land_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Build land consumer"); }
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

class wkz_build_industries_city_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Build city market"); }
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

class wkz_build_factory_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Build city market"); }
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

class wkz_link_factory_t : public werkzeug_t {
private:
	fabrik_t* last_fab;
	zeiger_t *wkz_linkzeiger;
public:
	wkz_link_factory_t() : werkzeug_t() { wkz_linkzeiger=NULL; last_fab=NULL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("Connect factory"); }
	bool init( karte_t *, spieler_t * );
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

class wkz_headquarter_t : public werkzeug_t {
private:
	const haus_besch_t *next_level( spieler_t *sp );
public:
	const char *get_tooltip(spieler_t *);
	bool init( karte_t *, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

/* protects map from further change (here because two clicks to confirm it!) */
class wkz_lock_game_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Lock game"); }
	bool init( karte_t *welt, spieler_t * ) { return welt->get_einstellungen()->get_allow_player_change(); }
	const char *work( karte_t *welt, spieler_t *, koord3d ) {
		welt->get_einstellungen()->set_allow_player_change( false );
		destroy_all_win();
		welt->switch_active_player( 0 );
		welt->set_werkzeug( general_tool[WKZ_ABFRAGE] );
		return NULL;
	}
};

/* add random citycar if no default is set; else add a certain city car */
class wkz_add_citycar_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Add random citycar"); }
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

/* make forest */
class wkz_forest_t : public two_click_werkzeug_t {
public:
	const char *get_tooltip(spieler_t *) { return translator::translate("Add forest"); }
private:
	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &, const koord3d & );
	virtual const char *valid_pos( karte_t *, spieler_t *, const koord3d & );
};

/* stop moving tool */
class wkz_stop_moving_t : public werkzeug_t {
private:
	koord3d last_pos;
	zeiger_t *wkz_linkzeiger;
	waytype_t waytype[2];
	halthandle_t last_halt;
public:
	wkz_stop_moving_t() : werkzeug_t() { wkz_linkzeiger=NULL; }
	const char *get_tooltip(spieler_t *) { return translator::translate("replace stop"); }
	bool init( karte_t *, spieler_t * );
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

/* make all tiles of this player a public stop
 * if this player is public, make all connected tiles a public stop */
class wkz_make_stop_public_t : public werkzeug_t {
public:
	wkz_make_stop_public_t() : werkzeug_t() {}
	bool init( karte_t *, spieler_t * );
	bool exit( karte_t *w, spieler_t *s ) { return init(w,s); }
	const char *get_tooltip(spieler_t *);
	virtual const char *move( karte_t *, spieler_t *, uint16 /* buttonstate */, koord3d );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

/********************* one click tools ****************************/

class wkz_pause_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Pause"); }
	bool is_selected(karte_t *welt) const { return welt->is_paused(); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->set_fast_forward(0);
		welt->set_pause( welt->is_paused()^1 );
		return false;
	}
};

class wkz_fastforward_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Fast forward"); }
	bool is_selected(karte_t *welt) const { return welt->is_fast_forward(); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->set_pause(0);
		welt->set_fast_forward( welt->is_fast_forward()^1 );
		return false;
	}
};

class wkz_screenshot_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Screenshot"); }
	bool init( karte_t *, spieler_t * ) {
		display_snapshot();
		create_win( new news_img("Screenshot\ngespeichert.\n"), w_time_delete, magic_none);
		return false;
	}
};

// builts next chain
class wkz_increase_industry_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Increase Industry density"); }
	bool init( karte_t *welt, spieler_t * ) {
		fabrikbauer_t::increase_industry_density( welt, false );
		return false;
	}
};

/* prissi: undo building */
class wkz_undo_t : public werkzeug_t {
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
	const char *get_tooltip(spieler_t *) { return translator::translate("Change player"); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->switch_active_player( welt->get_active_player_nr()+1 );
		return false;
	}
};

// setp one year forward
class wkz_step_year_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Step timeline one year"); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->step_year();
		return false;
	}
};

class wkz_change_game_speed_t : public werkzeug_t {
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
	const char *get_tooltip(spieler_t *) { return translator::translate("zooming in"); }
	bool init( karte_t *welt, spieler_t * ) {
		win_change_zoom_factor(true);
		welt->set_dirty();
		return false;
	}
};

class wkz_zoom_out_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("zooming out"); }
	bool init( karte_t *welt, spieler_t * ) {
		win_change_zoom_factor(false);
		welt->set_dirty();
		return false;
	}
};

class wkz_show_coverage_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("show station coverage"); }
	bool is_selected(karte_t *) const { return umgebung_t::station_coverage_show; }
	bool init( karte_t *welt, spieler_t * ) {
		umgebung_t::station_coverage_show = !umgebung_t::station_coverage_show;
		welt->set_dirty();
		return false;
	}
};

class wkz_show_name_t : public werkzeug_t {
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
};

class wkz_show_grid_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("show grid"); }
	bool is_selected(karte_t *) const { return grund_t::show_grid; }
	bool init( karte_t *welt, spieler_t * ) {
		grund_t::toggle_grid();
		welt->set_dirty();
		return false;
	}
};

class wkz_show_trees_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("hide trees"); }
	bool is_selected(karte_t *) const { return umgebung_t::hide_trees; }
	bool init( karte_t *welt, spieler_t * ) {
		umgebung_t::hide_trees = !umgebung_t::hide_trees;
		welt->set_dirty();
		return false;
	}
};

class wkz_show_houses_t : public werkzeug_t {
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
};

class wkz_show_underground_t : public werkzeug_t {
	static sint8 save_underground_level;
	const char *get_tooltip(spieler_t *);
	bool is_selected(karte_t *) const;
	void draw_after( karte_t *w, koord pos ) const;
	bool init( karte_t *welt, spieler_t * );
	virtual const char *work( karte_t *, spieler_t *, koord3d );
};

class wkz_rotate90_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Rotate map"); }
	bool init( karte_t *welt, spieler_t * ) {
		welt->rotate90();
		welt->update_map();
		return false;
	}
};

class wkz_quit_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Beenden"); }
	bool init( karte_t *welt, spieler_t * ) {
		destroy_all_win();
		welt->beenden( true );
		return false;
	}
};

// step size by default_param
class wkz_fill_trees_t : public werkzeug_t {
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
	const char *get_tooltip(spieler_t *);
	bool init( karte_t *, spieler_t * );
};


/* change day/night view manually */
class wkz_vehicle_tooltips_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return "Toggle vehicle tooltips"; }
	bool init( karte_t *, spieler_t * ) {
		umgebung_t::show_vehicle_states = (umgebung_t::show_vehicle_states+1)%3;
		return false;
	}
};

/********************** dialoge tools *****************************/

// general help
class wkz_help_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Help"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_mainhelp); }
	bool init( karte_t *, spieler_t * ) {
		create_win(new help_frame_t("general.txt"), w_info, magic_mainhelp);
		return false;
	}
};

// open info/quit dialoge
class wkz_optionen_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Einstellungsfenster"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_optionen_gui_t); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win(240, 120, new optionen_gui_t(welt), w_info, magic_optionen_gui_t);
		return false;
	}
};

// open minimap
class wkz_minimap_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Reliefkarte"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_reliefmap); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new map_frame_t(welt), w_info, magic_reliefmap);
		return false;
	}
};

// open line management
class wkz_lines_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Line Management"); }
	virtual image_id get_icon(spieler_t *sp) const { return sp->get_player_nr()==1 ? IMG_LEER : icon; }
	bool is_selected(karte_t *welt) const { return win_get_magic((long)(&(welt->get_active_player()->simlinemgmt))); }
	bool init( karte_t *, spieler_t *sp ) {
		if(sp->get_player_nr()!=1) {
			sp->simlinemgmt.zeige_info( sp );
		}
		return false;
	}
};

// open messages
class wkz_messages_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Mailbox"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_messageframe); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new message_frame_t(welt), w_info, magic_messageframe );
		return false;
	}
};

// open messages
class wkz_finances_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Finanzen"); }
	bool is_selected(karte_t *welt) const { return win_get_magic((long)welt->get_active_player()); }
	bool init( karte_t *, spieler_t *sp ) {
		create_win( new money_frame_t(sp), w_info, (long)sp );
		return false;
	}
};

// open player dialoge
class wkz_players_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Spielerliste"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_ki_kontroll_t); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( 272, 160, new ki_kontroll_t(welt), w_info, magic_ki_kontroll_t);
		return false;
	}
};

// open player dialoge
class wkz_displayoptions_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Helligk."); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_color_gui_t); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win(new color_gui_t(welt), w_info, magic_color_gui_t);
		return false;
	}
};

// open sound dialoge
class wkz_sound_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Sound"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_sound_kontroll_t); }
	bool init( karte_t *, spieler_t * ) {
		create_win(new sound_frame_t(), w_info, magic_sound_kontroll_t);
		return false;
	}
};

// open language dialoge
class wkz_language_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Sprache"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_sprachengui_t); }
	bool init( karte_t *, spieler_t * ) {
		create_win(new sprachengui_t(), w_info, magic_sprachengui_t);
		return false;
	}
};

// open player color dialoge
class wkz_playercolor_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Farbe"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_farbengui_t); }
	bool init( karte_t *, spieler_t *sp ) {
		create_win(new farbengui_t(sp), w_info, magic_farbengui_t);
		return false;
	}
};

// jump to position dialoge
class wkz_jump_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Jump to"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_jump); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new jump_frame_t(welt), w_info, magic_jump);
		return false;
	}
};

// load game dialoge
class wkz_load_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Laden"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_load_t); }
	bool init( karte_t *welt, spieler_t * ) {
		destroy_all_win();
		create_win(new loadsave_frame_t(welt, true), w_info, magic_load_t);
		return false;
	}
};

// save game dialoge
class wkz_save_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Speichern"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_save_t); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win(new loadsave_frame_t(welt, false), w_info, magic_save_t);
		return false;
	}
};

/* open the list of halt */
class wkz_list_halt_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("hl_title"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_halt_list_t); }
	bool init( karte_t *, spieler_t *sp ) {
		create_win( new halt_list_frame_t(sp), w_info, magic_halt_list_t );
		return false;
	}
};

/* open the list of vehicle */
class wkz_list_convoi_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("cl_title"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_convoi_t); }
	bool init( karte_t *, spieler_t *sp ) {
		create_win( new convoi_frame_t(sp), w_info, magic_convoi_t );
		return false;
	}
};

/* open the list of towns */
class wkz_list_town_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("tl_title"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_citylist_frame_t); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new citylist_frame_t(welt), w_info, magic_citylist_frame_t );
		return false;
	}
};

/* open the list of goods */
class wkz_list_goods_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("gl_title"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_goodslist); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new goods_frame_t(welt), w_info, magic_goodslist );
		return false;
	}
};

/* open the list of factories */
class wkz_list_factory_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("fl_title"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_factorylist); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new factorylist_frame_t(welt), w_info, magic_factorylist );
		return false;
	}
};

/* open the list of attraction */
class wkz_list_curiosity_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("curlist_title"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_curiositylist); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new curiositylist_frame_t(welt), w_info, magic_curiositylist );
		return false;
	}
};

/* factory building dialog */
class wkz_factorybuilder_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("factorybuilder"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_edit_factory); }
	bool init( karte_t *welt, spieler_t *sp ) {
		create_win( new factory_edit_frame_t(sp,welt), w_info, magic_edit_factory );
		return false;
	}
};

/* attraction building dialog */
class wkz_attractionbuilder_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("curiosity builder"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_edit_attraction); }
	bool init( karte_t *welt, spieler_t *sp ) {
		create_win( new curiosity_edit_frame_t(sp,welt), w_info, magic_edit_attraction );
		return false;
	}
};

/* house building dialog */
class wkz_housebuilder_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("citybuilding builder"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_edit_house); }
	bool init( karte_t *welt, spieler_t *sp ) {
		create_win( new citybuilding_edit_frame_t(sp,welt), w_info, magic_edit_house );
		return false;
	}
};

/* house building dialog */
class wkz_treebuilder_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("baum builder"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_edit_tree); }
	bool init( karte_t *welt, spieler_t *sp ) {
		create_win( new baum_edit_frame_t(sp,welt), w_info, magic_edit_tree );
		return false;
	}
};

// to increase map-size
class wkz_enlarge_map_t : public werkzeug_t{
	const char *get_tooltip(spieler_t *) { return translator::translate("enlarge map"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_bigger_map); }
	bool init( karte_t *welt, spieler_t *sp ) {
		create_win( new enlarge_map_frame_t(sp,welt), w_info, magic_bigger_map );
		return false;
	}
};

/* open the list of label */
class wkz_list_label_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("labellist_title"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_labellist); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new labellist_frame_t(welt), w_info, magic_labellist );
		return false;
	}
};

/* open the list of label */
class wkz_climates_t : public werkzeug_t {
	const char *get_tooltip(spieler_t *) { return translator::translate("Climate Control"); }
	bool is_selected(karte_t *) const { return win_get_magic(magic_climate); }
	bool init( karte_t *welt, spieler_t * ) {
		create_win( new climate_gui_t(welt->get_einstellungen()), w_info, magic_climate );
		return false;
	}
};
#endif
