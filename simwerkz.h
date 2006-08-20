/*
 * simwerkz.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef simwerkz_h
#define simwerkz_h

#include "simtypes.h"

#define INIT  koord(-1,-1)
#define EXIT  koord(-2,-2)

int wkz_abfrage(spieler_t *, karte_t *welt, koord pos);
int wkz_raise(spieler_t *sp, karte_t *welt, koord pos);
int wkz_lower(spieler_t *sp, karte_t *welt, koord pos);
int wkz_remover(spieler_t *sp, karte_t *welt, koord pos);


int wkz_wegebau(spieler_t *sp, karte_t *welt, koord pos, value_t lParam);

int wkz_frachthof(spieler_t *sp, karte_t *welt, koord pos);
int wkz_post(spieler_t *sp, karte_t *welt, koord pos, value_t value);
int wkz_station_building(spieler_t *sp, karte_t *welt, koord pos, value_t value);
//int wkz_lagerhaus(spieler_t *sp, karte_t *welt, koord pos);
int wkz_bahnhof(spieler_t *sp, karte_t *welt, koord pos, value_t lParam);
int wkz_bushalt(spieler_t *sp, karte_t *welt, koord pos,value_t value);
int wkz_dockbau(spieler_t *sp, karte_t *welt, koord pos,value_t value);

int wkz_marker(spieler_t *sp, karte_t *welt, koord pos);

int wkz_senke(spieler_t *sp, karte_t *welt, koord pos);

int wkz_signale(spieler_t *sp, karte_t *welt, koord pos);
int wkz_presignals(spieler_t *sp, karte_t *welt, koord pos);
int wkz_roadsign(spieler_t *sp, karte_t *welt, koord pos, value_t value);

int wkz_bahndepot(spieler_t *sp, karte_t *welt, koord pos);
int wkz_tramdepot(spieler_t *sp, karte_t *welt, koord pos);
int wkz_monoraildepot(spieler_t *sp, karte_t *welt, koord pos);

int wkz_strassendepot(spieler_t *sp, karte_t *welt, koord pos);
int wkz_schiffdepot(spieler_t *sp, karte_t *welt, koord pos, value_t value);



// sonderwerkzeuge

int wkz_fahrplan_add(spieler_t *sp, karte_t *welt, koord pos,value_t f);
int wkz_fahrplan_ins(spieler_t *sp, karte_t *welt, koord pos,value_t f);

// das testwerkzeug

int wkz_blocktest(spieler_t *sp, karte_t *welt, koord pos);
int wkz_test(spieler_t *sp, karte_t *welt, koord pos);

int wkz_electrify_block(spieler_t *, karte_t *welt, koord pos);

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
int wkz_pflanze_baum(spieler_t *, karte_t *welt, koord pos);

#ifdef USE_DRIVABLES
int wkz_test_new_cars(spieler_t *, karte_t *welt, koord pos);
#endif

int wkz_build_industries_land(spieler_t *sp, karte_t *welt, koord pos);
int wkz_build_industries_city(spieler_t *sp, karte_t *welt, koord pos);

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

/* undo building */
int wkz_undo(spieler_t *sp, karte_t *welt);

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
 int wkz_add_attraction(spieler_t *, karte_t *welt, koord pos);

#endif
