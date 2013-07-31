/*
 * all color related stuff
 */

#ifndef simcolor_h
#define simcolor_h

#define LIGHT_COUNT (15)

// this is a player color => use different table for conversion
#define PLAYER_FLAG        (0x8000)
#define TRANSPARENT_FLAGS  (0x7800)
#define TRANSPARENT25_FLAG (0x2000)
#define TRANSPARENT50_FLAG (0x4000)
#define TRANSPARENT75_FLAG (0x6000)
#define OUTLINE_FLAG       (0x0800)

typedef unsigned short PLAYER_COLOR_VAL;
typedef unsigned char COLOR_VAL;

// Menu colours (they don't change beetween day and night)
#define MN_GREY0  (229)
#define MN_GREY1  (230)
#define MN_GREY2  (231)
#define MN_GREY3  (232)
#define MN_GREY4  (233)


// fixed colors
#define COL_BLACK           (240)
#define COL_WHITE           (215)
#define COL_RED             (131)
#define COL_DARK_RED        (128)
#define COL_LIGHT_RED       (134)
#define COL_YELLOW          (171)
#define COL_DARK_YELLOW     (168)
#define COL_LIGHT_YELLOW    (175)
#define COL_LEMON_YELLOW    (31)
#define COL_BLUE            (147)
#define COL_DARK_BLUE       (144)
#define COL_LIGHT_BLUE      (103)
#define COL_GREEN           (140)
#define COL_DARK_GREEN      (136)
#define COL_LIGHT_GREEN     (143)
#define COL_ORANGE          (155)
#define COL_DARK_ORANGE     (153)
#define COL_LIGHT_ORANGE    (158)
#define COL_BRIGHT_ORANGE   (133)
#define COL_LILAC           (221)
#define COL_MAGENTA         (63)
#define COL_PURPLE          (76)
#define COL_DARK_PURPLE     (73)
#define COL_LIGHT_PURPLE    (79)
#define COL_TURQUOISE       (53)
#define COL_LIGHT_TURQUOISE (55)
#define COL_DARK_TURQUOISE  (50)
#define COL_LIGHT_BROWN     (191)
#define COL_BROWN           (189)
#define COL_DARK_BROWN      (178)

// message colors
#define CITY_KI      (209)
#define NEW_VEHICLE  COL_PURPLE

// by niels
#define COL_GREY1 (208)
#define COL_GREY2 (210)
#define COL_GREY3 (212)
#define COL_GREY4 (11)
#define COL_GREY5 (213)
#define COL_GREY6 (15)

#define VEHIKEL_KENN  COL_YELLOW

#define WIN_TITEL     (154)

#define MONEY_PLUS   COL_BLACK
#define MONEY_MINUS  COL_RED

// used in many dialogues graphs
#define COL_REVENUE         (142)
#define COL_OPERATION       (132)
#define COL_MAINTENANCE     COL_LIGHT_RED
#define COL_TOLL            (157)
#define COL_POWERLINES      (46)
#define COL_OPS_PROFIT      (87)
#define COL_NEW_VEHICLES    COL_LIGHT_PURPLE
#define COL_CONSTRUCTION    (110)
#define COL_PROFIT          (6)
#define COL_TRANSPORTED     COL_YELLOW
#define COL_MAXSPEED        COL_TURQUOISE

#define COL_CASH            (52)
#define COL_VEHICLE_ASSETS  COL_MAGENTA
#define COL_MARGIN          COL_LIGHT_YELLOW
#define COL_WEALTH          (95)

#define COL_COUNVOI_COUNT   (55)
#define COL_FREE_CAPACITY   COL_TOLL
#define COL_DISTANCE        COL_OPS_PROFIT

#define COL_CITICENS        COL_WHITE
#define COL_GROWTH          (122)
#define COL_HAPPY           COL_WHITE
#define COL_UNHAPPY         COL_RED
#define COL_NO_ROUTE        COL_BLUE
#define COL_WAITING         COL_YELLOW
#define COL_ARRIVED         COL_DARK_ORANGE
#define COL_DEPARTED        COL_DARK_YELLOW

#define SYSCOL_HIGHLIGHT          skinverwaltung_t::theme_color_highlight
#define SYSCOL_SHADOW             skinverwaltung_t::theme_color_shadow
#define SYSCOL_FACE               skinverwaltung_t::theme_color_face
#define SYSCOL_BUTTON_TEXT        button_t::button_color_text
#define SYSCOL_DISABLED_BUTTON_TEXT        button_t::button_color_disabled_text
#define SYSCOL_TEXT               skinverwaltung_t::theme_color_text
#define SYSCOL_SELECTED_TEXT      skinverwaltung_t::theme_color_selected_text
#define SYSCOL_SELECTED_BACGROUND skinverwaltung_t::theme_color_selected_background
#define SYSCOL_STATIC_TEXT        skinverwaltung_t::theme_color_static_text
#define SYSCOL_DISABLED_TEXT      skinverwaltung_t::theme_color_disabled_text

#endif
