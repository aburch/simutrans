/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef simwerkz_h
#define simwerkz_h

#include "simtypes.h"

class haus_besch_t;

typedef struct {
	const haus_besch_t *besch;
	bool ignore_climates;
	bool add_to_next_city;
	uint8 rotation; //255 for any
} build_haus_struct;


/* internal functions: Only for AI (gives no error messages) */
int wkz_remover_intern(spieler_t *sp, karte_t *welt, koord pos, const char *&msg);
bool wkz_halt_aux(spieler_t *sp, karte_t *welt, koord pos, const char *&msg, const haus_besch_t *besch, waytype_t wegtype, sint64 cost, const char *type_name);


#define INIT  koord(-1,-1)
#define EXIT  koord(-2,-2)
#define DRAGGING  koord(-3,-3)

int wkz_abfrage(spieler_t *, karte_t *welt, koord pos);
int wkz_raise(spieler_t *sp, karte_t *welt, koord pos);
int wkz_lower(spieler_t *sp, karte_t *welt, koord pos);
int wkz_remover(spieler_t *sp, karte_t *welt, koord pos);

int wkz_wegebau(spieler_t *sp, karte_t *welt, koord pos, value_t lParam);

/* removes a way like a driving car ... */
int wkz_wayremover(spieler_t *sp, karte_t *welt,  koord pos, value_t lParam);

/* add catenary during construction */
int wkz_wayobj(spieler_t *sp, karte_t *welt, koord pos, value_t lParam);

int wkz_clear_reservation(spieler_t *sp, karte_t *welt, koord pos);

int wkz_marker(spieler_t *sp, karte_t *welt, koord pos);

int wkz_senke(spieler_t *sp, karte_t *welt, koord pos);

int wkz_roadsign(spieler_t *sp, karte_t *welt, koord pos, value_t value);

int wkz_station_building(spieler_t *sp, karte_t *welt, koord pos, value_t value);

// building all kind of stops be sea harbours
int wkz_halt(spieler_t *sp, karte_t *welt, koord pos,value_t f);

// built sea harbours
int wkz_dockbau(spieler_t *sp, karte_t *welt, koord pos,value_t value);

int wkz_depot(spieler_t *sp, karte_t *welt, koord pos,value_t f);

// sonderwerkzeuge
int wkz_fahrplan_add(spieler_t *sp, karte_t *welt, koord pos,value_t f);
int wkz_fahrplan_ins(spieler_t *sp, karte_t *welt, koord pos,value_t f);

/**
 * found a new city
 * @author Hj. Malthaner
 */
int wkz_add_city(spieler_t *sp, karte_t *welt, koord pos);


/**
 * Create an articial slope
 * @param param the slope type
 * @author Hj. Malthaner
 */
int wkz_set_slope(spieler_t *sp, karte_t *welt, koord pos, value_t param);


/**
 * Plant a tree
 * @author Hj. Malthaner
 */
int wkz_pflanze_baum(spieler_t *, karte_t *welt, koord pos, value_t param);

int wkz_build_industries_land(spieler_t *sp, karte_t *welt, koord pos, value_t param);
int wkz_build_industries_city(spieler_t *sp, karte_t *welt, koord pos, value_t param);

// build a single factory
int wkz_build_fab(spieler_t *, karte_t *welt, koord pos, value_t param);

// links production/consumtion between two factories
int wkz_factory_link(spieler_t *sp, karte_t *welt, koord pos);

/* open the list of halt */
int wkz_list_halt_tool(spieler_t *sp, karte_t *welt,koord k);

/* open the list of vehicle */
int wkz_list_vehicle_tool(spieler_t *sp, karte_t *welt, koord v);

/* open the list of towns */
int wkz_list_town_tool(spieler_t *sp, karte_t *welt, koord v);

/* open the list of goods */
int wkz_list_good_tool(spieler_t *sp, karte_t *welt, koord v);

/* open the list of factories */
int wkz_list_factory_tool(spieler_t *, karte_t *welt,koord k);

/* open the list of attraction */
int wkz_list_curiosity_tool(spieler_t *, karte_t *welt,koord k);

/* undo building */
int wkz_undo(spieler_t* sp);

int wkz_headquarter(spieler_t *sp, karte_t *welt, koord pos);

/* switch to next player
 * @author prissi
 */
int wkz_switch_player(spieler_t *, karte_t *welt, koord pos);

/* change city size
 * @author prissi
 */
int wkz_grow_city(spieler_t *, karte_t *welt, koord pos, value_t lParam);

/* built random tourist attraction
 * @author prissi
 */
 int wkz_add_haus(spieler_t *, karte_t *welt, koord pos, value_t lParam);

// protects map from further change
int wkz_lock( spieler_t *, karte_t *welt, koord pos);

int wkz_step_year( spieler_t *, karte_t *welt, koord pos);

#endif
