/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMCOLOR_H
#define SIMCOLOR_H


#define LIGHT_COUNT         (15)
#define SPECIAL_COLOR_COUNT (224)

// this is a player color => use different table for conversion
#define PLAYER_FLAG        (0x800000)
#define TRANSPARENT_FLAGS  (0x780000)
#define TRANSPARENT25_FLAG (0x200000)
#define TRANSPARENT50_FLAG (0x400000)
#define TRANSPARENT75_FLAG (0x600000)
#define OUTLINE_FLAG       (0x080000)

// pixels stored in system type (uint16)
typedef unsigned short PIXVAL;
// PIXVAL with above flags (eg. transparent) (uint32)
typedef unsigned int FLAGGED_PIXVAL;

// Menu colours (they don't change between day and night)
#define MN_GREY0            229
#define MN_GREY1            230
#define MN_GREY2            231
#define MN_GREY3            232
#define MN_GREY4            233


// fixed colors
#define COL_BLACK           240
#define COL_WHITE           215
#define COL_RED             131
#define COL_DARK_RED        128
#define COL_LIGHT_RED       134
#define COL_YELLOW          171
#define COL_DARK_YELLOW     168
#define COL_LIGHT_YELLOW    175
#define COL_LEMON_YELLOW    31
#define COL_BLUE            147
#define COL_DARK_BLUE       144
#define COL_SOFT_BLUE       100
#define COL_LIGHT_BLUE      103
#define COL_GREEN           140
#define COL_DARK_GREEN      136
#define COL_LIGHT_GREEN     143
#define COL_ORANGE          155
#define COL_DARK_ORANGE     153
#define COL_LIGHT_ORANGE    158
#define COL_BRIGHT_ORANGE   133
#define COL_LILAC           221
#define COL_MAGENTA         63
#define COL_PURPLE          76
#define COL_DARK_PURPLE     73
#define COL_LIGHT_PURPLE    79
#define COL_TURQUOISE       53
#define COL_LIGHT_TURQUOISE 55
#define COL_DARK_TURQUOISE  50
#define COL_LIGHT_BROWN     191
#define COL_BROWN           189
#define COL_DARK_BROWN      178

// message colors
#define CITY_KI             color_idx_to_rgb(209)
#define NEW_VEHICLE         color_idx_to_rgb(COL_PURPLE)

// by niels
#define COL_GREY1           208
#define COL_GREY2           210
#define COL_GREY3           212
#define COL_GREY4           11
#define COL_GREY5           213
#define COL_GREY6           15

// Way colours for the map
#define COL_ROAD            color_idx_to_rgb(COL_GREY1)
#define COL_RAIL            color_idx_to_rgb(185)
#define COL_CANAL           color_idx_to_rgb(23)
#define COL_MONORAIL        color_idx_to_rgb(COL_ORANGE)
#define COL_RUNWAY          color_idx_to_rgb(28)
#define COL_POWERLINE       color_idx_to_rgb(COL_LIGHT_TURQUOISE)
#define COL_HALT            color_idx_to_rgb(COL_RED)
#define COL_BUILDING        color_idx_to_rgb(COL_GREY3)
#define COL_VEHICLE         color_idx_to_rgb(COL_YELLOW)

// used in many dialogues graphs
#define COL_REVENUE         142
#define COL_OPERATION       132
#define COL_MAINTENANCE     COL_LIGHT_RED
#define COL_TOLL            157
#define COL_POWERLINES      46
#define COL_OPS_PROFIT      87
#define COL_NEW_VEHICLES    COL_LIGHT_PURPLE
#define COL_CONSTRUCTION    110
#define COL_PROFIT          6
#define COL_TRANSPORTED     COL_YELLOW
#define COL_MAXSPEED        COL_TURQUOISE

#define COL_CASH            52
#define COL_VEHICLE_ASSETS  COL_MAGENTA
#define COL_MARGIN          COL_LIGHT_YELLOW
#define COL_WEALTH          95

#define COL_CONVOI_COUNT    COL_LIGHT_TURQUOISE
#define COL_FREE_CAPACITY   COL_TOLL
#define COL_DISTANCE        COL_OPS_PROFIT

#define COL_CITIZENS        COL_WHITE
#define COL_GROWTH          122
#define COL_HAPPY           COL_WHITE
#define COL_UNHAPPY         COL_RED
#define COL_NO_ROUTE        COL_BLUE
#define COL_WAITING         COL_YELLOW
#define COL_ARRIVED         COL_DARK_ORANGE
#define COL_DEPARTED        COL_DARK_YELLOW

#define SYSCOL_TEXT                         gui_theme_t::gui_color_text
#define SYSCOL_TEXT_HIGHLIGHT               gui_theme_t::gui_color_text_highlight
#define SYSCOL_TEXT_SHADOW                  gui_theme_t::gui_color_text_shadow
#define SYSCOL_TEXT_TITLE                   gui_theme_t::gui_color_text_title
#define SYSCOL_TEXT_STRONG                  gui_theme_t::gui_color_text_strong
#define MONEY_MINUS                         gui_theme_t::gui_color_text_minus
#define MONEY_PLUS                          gui_theme_t::gui_color_text_plus
#define SYSCOL_TEXT_UNUSED                  gui_theme_t::gui_color_text_unused
#define SYSCOL_EDIT_TEXT                    gui_theme_t::gui_color_edit_text
#define SYSCOL_EDIT_TEXT_SELECTED           gui_theme_t::gui_color_edit_text_selected
#define SYSCOL_EDIT_TEXT_DISABLED           gui_theme_t::gui_color_edit_text_disabled
#define SYSCOL_EDIT_BACKGROUND_SELECTED     gui_theme_t::gui_color_edit_background_selected
#define SYSCOL_CURSOR_BEAM                  gui_theme_t::gui_color_edit_beam
#define SYSCOL_CHART_BACKGROUND             gui_theme_t::gui_color_chart_background
#define SYSCOL_CHART_LINES_ZERO             gui_theme_t::gui_color_chart_lines_zero
#define SYSCOL_CHART_LINES_ODD              gui_theme_t::gui_color_chart_lines_odd
#define SYSCOL_CHART_LINES_EVEN             gui_theme_t::gui_color_chart_lines_even
#define SYSCOL_LIST_TEXT_SELECTED_FOCUS     gui_theme_t::gui_color_list_text_selected_focus
#define SYSCOL_LIST_TEXT_SELECTED_NOFOCUS   gui_theme_t::gui_color_list_text_selected_nofocus
#define SYSCOL_LIST_BACKGROUND_SELECTED_F   gui_theme_t::gui_color_list_background_selected_f
#define SYSCOL_LIST_BACKGROUND_SELECTED_NF  gui_theme_t::gui_color_list_background_selected_nf
#define SYSCOL_BUTTON_TEXT                  gui_theme_t::gui_color_button_text
#define SYSCOL_BUTTON_TEXT_DISABLED         gui_theme_t::gui_color_button_text_disabled
#define SYSCOL_BUTTON_TEXT_SELECTED         gui_theme_t::gui_color_button_text_selected
#define SYSCOL_COLORED_BUTTON_TEXT          gui_theme_t::gui_color_colored_button_text
#define SYSCOL_COLORED_BUTTON_TEXT_SELECTED gui_theme_t::gui_color_colored_button_text_selected
#define SYSCOL_CHECKBOX_TEXT                gui_theme_t::gui_color_checkbox_text
#define SYSCOL_CHECKBOX_TEXT_DISABLED       gui_theme_t::gui_color_checkbox_text_disabled
#define SYSCOL_TICKER_BACKGROUND            gui_theme_t::gui_color_ticker_background
#define SYSCOL_TICKER_DIVIDER               gui_theme_t::gui_color_ticker_divider
#define SYSCOL_STATUSBAR_TEXT               gui_theme_t::gui_color_statusbar_text
#define SYSCOL_STATUSBAR_BACKGROUND         gui_theme_t::gui_color_statusbar_background
#define SYSCOL_STATUSBAR_DIVIDER            gui_theme_t::gui_color_statusbar_divider
#define SYSCOL_HIGHLIGHT                    gui_theme_t::gui_highlight_color
#define SYSCOL_SHADOW                       gui_theme_t::gui_shadow_color
#define SYSCOL_LOADINGBAR_INNER             gui_theme_t::gui_color_loadingbar_inner
#define SYSCOL_LOADINGBAR_PROGRESS          gui_theme_t::gui_color_loadingbar_progress
#define SYSCOL_OBSOLETE                     gui_theme_t::gui_color_obsolete
#define SYSCOL_EMPTY                        gui_theme_t::gui_color_empty

#endif
