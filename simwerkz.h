/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef simwerkz_h
#define simwerkz_h

#include "simtypes.h"

class karte_t;
class spieler_t;
class haus_besch_t;

typedef struct {
	const haus_besch_t *besch;
	bool ignore_climates;
	bool add_to_next_city;
	uint8 rotation; //255 for any
} build_haus_struct;

typedef enum wkz_mode_t { WKZ_INIT=1, WKZ_EXIT, WKZ_DO, WKZ_DRAG };
typedef int (*tool_func_param)(enum wkz_mode_t, spieler_t*, karte_t*, koord, value_t);
typedef int (*tool_func)(enum wkz_mode_t, spieler_t*, karte_t*, koord);


/* internal functions: Only for AI (gives no error messages) */
int wkz_remover_intern(spieler_t *sp, karte_t *welt, koord pos, const char *&msg);
bool wkz_halt_aux(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos, const char *&msg, const haus_besch_t *besch, waytype_t wegtype, sint64 cost, const char *type_name);


int wkz_abfrage(enum wkz_mode_t, spieler_t *, karte_t *welt, koord pos);
int wkz_raise(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos);
int wkz_lower(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos);
int wkz_remover(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos);

int wkz_wegebau(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos, value_t lParam);

/* removes a way like a driving car ... */
int wkz_wayremover(enum wkz_mode_t, spieler_t *sp, karte_t *welt,  koord pos, value_t lParam);

/* add catenary during construction */
int wkz_wayobj(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos, value_t lParam);

int wkz_clear_reservation(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos);

int wkz_marker(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos);

int wkz_senke(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos);

int wkz_roadsign(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos, value_t value);

int wkz_station_building(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos, value_t value);

// building all kind of stops be sea harbours
int wkz_halt(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos,value_t f);

// built sea harbours
int wkz_dockbau(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos,value_t value);

int wkz_depot(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos,value_t f);

// sonderwerkzeuge
int wkz_fahrplan_add(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos,value_t f);
int wkz_fahrplan_ins(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos,value_t f);

/**
 * found a new city
 * @author Hj. Malthaner
 */
int wkz_add_city(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos);


/**
 * Create an articial slope
 * @param param the slope type
 * @author Hj. Malthaner
 */
int wkz_set_slope(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos, value_t param);


/**
 * Plant a tree
 * @author Hj. Malthaner
 */
int wkz_pflanze_baum(enum wkz_mode_t, spieler_t *, karte_t *welt, koord pos, value_t param);

int wkz_build_industries_land(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos, value_t param);
int wkz_build_industries_city(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos, value_t param);

// build a single factory
int wkz_build_fab(enum wkz_mode_t, spieler_t *, karte_t *welt, koord pos, value_t param);

// links production/consumtion between two factories
int wkz_factory_link(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos);

/* open the list of halt */
int wkz_increase_chain(enum wkz_mode_t, spieler_t *sp, karte_t *welt,koord k);

/* open the list of halt */
int wkz_list_halt_tool(enum wkz_mode_t, spieler_t *sp, karte_t *welt,koord k);

/* open the list of vehicle */
int wkz_list_vehicle_tool(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord v);

/* open the list of towns */
int wkz_list_town_tool(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord v);

/* open the list of goods */
int wkz_list_good_tool(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord v);

/* open the list of factories */
int wkz_list_factory_tool(enum wkz_mode_t, spieler_t *, karte_t *welt,koord k);

/* open the list of attraction */
int wkz_list_curiosity_tool(enum wkz_mode_t, spieler_t *, karte_t *welt,koord k);

/* undo building */
int wkz_undo(spieler_t* sp);

int wkz_headquarter(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos);

/* switch to next player
 * @author prissi
 */
int wkz_switch_player(enum wkz_mode_t, spieler_t *, karte_t *welt, koord pos);

/* change city size
 * @author prissi
 */
int wkz_grow_city(enum wkz_mode_t, spieler_t *, karte_t *welt, koord pos, value_t lParam);

/* built random tourist attraction
 * @author prissi
 */
 int wkz_add_haus(enum wkz_mode_t, spieler_t *, karte_t *welt, koord pos, value_t lParam);

// protects map from further change
int wkz_lock(enum wkz_mode_t, spieler_t *, karte_t *welt, koord pos);

int wkz_step_year(enum wkz_mode_t, spieler_t *, karte_t *welt, koord pos);

#endif
