/* simconst.h
 *
 * all color related stuff
 */

#ifndef simcolor_h
#define simcolor_h

// this is a player color => use different table for conversion
#define PLAYER_FLAG (0x8000)

#define TRANSPARENT_FLAGS (0x7000)
#define TRANSPARENT25_FLAG (0x2000)
#define TRANSPARENT50_FLAG (0x4000)
#define TRANSPARENT75_FLAG (0x6000)

typedef unsigned short PLAYER_COLOR_VAL;
typedef unsigned char COLOR_VAL;

// Menuefarben (aendern sich nicht von Tag zu Nacht)
#define MN_GREY0  (37)
#define MN_GREY1  (38)
#define MN_GREY2  (39)
#define MN_GREY3  (40)
#define MN_GREY4  (41)


// these change
#define COL_BLACK    (88)
#define COL_WHITE   (102)
#define COL_RED       (122)
#define COL_DARK_RED (104)
#define COL_YELLOW      (121)
#define COL_BLUE      (221)
#define COL_GREEN     (169)
#define COL_DARK_ORANGE (131)
#define COL_ORANGE    (132)
#define COL_PURPLE (80)

// message colors
#define CITY_KI (1)
#define NEW_VEHICLE (251)

// by niels
#define COL_GREY1  (90)
#define COL_GREY2  (92)
#define COL_GREY3  (94)
#define COL_GREY4  (96)
#define COL_GREY5  (98)
#define COL_GREY6  (100)

// Kenfarben fuer die Karte
#define KOHLE_KENN        COL_BLACK
#define STRASSE_KENN      COL_BLACK
#define SCHIENE_KENN      (230)
#define CHANNEL_KENN      COL_PURPLE
#define MONORAIL_KENN      (131)
#define RUNWAY_KENN      (96)
#define POWERLINE_KENN      (124)
#define HALT_KENN         COL_RED
#define VEHIKEL_KENN      COL_YELLOW

#define WIN_TITEL       (129)
#define MESG_WIN        (120)
#define MESG_WIN_HELL   (148)
#define MESG_WIN_DUNKEL (136)

#define MONEY_PLUS COL_BLACK
#define MONEY_MINUS COL_RED

// special chart colors
#define COL_CONSTRUCTION (63)
#define COL_OPERATION (11)
#define COL_NEW_VEHICLES (15)
#define COL_REVENUE (132)
#define COL_MAINTENANCE (23)
#define COL_VEHICLE_ASSETS (27)
#define COL_CASH (31)
#define COL_WEALTH (35)
#define COL_PROFIT (241)
#define COL_OPS_PROFIT (61)
#define COL_MARGIN (62)
#define COL_TRANSPORTED (169)
#define COL_FREE_CAPACITY COL_DARK_ORANGE
#define COL_CITICENS (7)
#define COL_GROWTH (62)
#define COL_HAPPY COL_WHITE
#define COL_UNHAPPY COL_RED
#define COL_NO_ROUTE COL_BLUE
#define COL_WAITING COL_YELLOW
#define COL_ARRIVED COL_DARK_ORANGE
#define COL_DEPARTED (23)

#endif
