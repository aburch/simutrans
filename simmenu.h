/*
 * simmenu.h
 *
 * Copyright (c) 2006 prissi
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef simmenu_h
#define simmenu_h

class karte_t;
class spieler_t;

enum menu_entries { MENU_SLOPE=0, MENU_RAIL, MENU_MONORAIL, MENU_TRAM, MENU_ROAD, MENU_SHIP, MENU_AIRPORT, MENU_SPECIAL, MENU_LISTS, MENU_EDIT };

char * tool_tip_with_price(const char * tip, sint64 price);

// open a menu tool window
void menu_open(karte_t *welt, menu_entries menu_nr, spieler_t *sp );



#endif
