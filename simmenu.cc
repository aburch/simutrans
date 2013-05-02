/*
 * Copyright (c) 2008 prissi
 *
 * This file is part of the Simutrans project under the artistic license.
 *
 * New configurable OOP tool system
 */

#include <string>
#include <algorithm>

#include "unicode.h"

#include "simevent.h"
#include "simworld.h"
#include "simwin.h"
#include "player/simplay.h"
#include "simmenu.h"
#include "simwerkz.h"
#include "simwerkz-dialogs.h"
#include "simskin.h"
#include "simsound.h"

#include "bauer/hausbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/brueckenbauer.h"
#include "bauer/tunnelbauer.h"

#include "besch/haus_besch.h"

#include "boden/grund.h"
#include "boden/wege/strasse.h"

#include "dataobj/umgebung.h"
#include "dataobj/tabfile.h"
#include "dataobj/scenario.h"

#include "dings/roadsign.h"
#include "dings/wayobj.h"
#include "dings/zeiger.h"

#include "gui/werkzeug_waehler.h"

#include "utils/simstring.h"
#include "utils/memory_rw.h"


// for key loockup; is always sorted during the game
vector_tpl<werkzeug_t *>werkzeug_t::char_to_tool(0);

// here are the default values, icons, cursor, sound definitions ...
vector_tpl<werkzeug_t *>werkzeug_t::general_tool(GENERAL_TOOL_COUNT);
vector_tpl<werkzeug_t *>werkzeug_t::simple_tool(SIMPLE_TOOL_COUNT);
vector_tpl<werkzeug_t *>werkzeug_t::dialog_tool(DIALOGE_TOOL_COUNT);

// the number of toolbars is not know yet
vector_tpl<toolbar_t *>werkzeug_t::toolbar_tool(0);

char werkzeug_t::toolstr[1024];

// separator in toolbars
class wkz_dummy_t : public werkzeug_t {
public:
	wkz_dummy_t() : werkzeug_t(dummy_id) {}

	bool init(karte_t*, spieler_t*) OVERRIDE { return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
	bool is_move_network_save(spieler_t*) const OVERRIDE { return true; }
};
werkzeug_t *werkzeug_t::dummy = new wkz_dummy_t();




werkzeug_t *create_general_tool(int toolnr)
{
	werkzeug_t* tool = NULL;
	switch(toolnr) {
		case WKZ_ABFRAGE:          tool = new wkz_abfrage_t(); break;
		case WKZ_REMOVER:          tool = new wkz_remover_t(); break;
		case WKZ_RAISE_LAND:       tool = new wkz_raise_t(); break;
		case WKZ_LOWER_LAND:       tool = new wkz_lower_t(); break;
		case WKZ_SETSLOPE:         tool = new wkz_setslope_t(); break;
		case WKZ_RESTORESLOPE:     tool = new wkz_restoreslope_t(); break;
		case WKZ_MARKER:           tool = new wkz_marker_t(); break;
		case WKZ_CLEAR_RESERVATION:tool = new wkz_clear_reservation_t(); break;
		case WKZ_TRANSFORMER:      tool = new wkz_transformer_t(); break;
		case WKZ_ADD_CITY:         tool = new wkz_add_city_t(); break;
		case WKZ_CHANGE_CITY_SIZE: tool = new wkz_change_city_size_t(); break;
		case WKZ_PLANT_TREE:       tool = new wkz_plant_tree_t(); break;
		case WKZ_FAHRPLAN_ADD:     tool = new wkz_fahrplan_add_t(); break;
		case WKZ_FAHRPLAN_INS:     tool = new wkz_fahrplan_ins_t(); break;
		case WKZ_WEGEBAU:          tool = new wkz_wegebau_t(); break;
		case WKZ_BRUECKENBAU:      tool = new wkz_brueckenbau_t(); break;
		case WKZ_TUNNELBAU:        tool = new wkz_tunnelbau_t(); break;
		case WKZ_WAYREMOVER:       tool = new wkz_wayremover_t(); break;
		case WKZ_WAYOBJ:           tool = new wkz_wayobj_t(); break;
		case WKZ_STATION:          tool = new wkz_station_t(); break;
		case WKZ_ROADSIGN:         tool = new wkz_roadsign_t(); break;
		case WKZ_DEPOT:            tool = new wkz_depot_t(); break;
		case WKZ_BUILD_HAUS:       tool = new wkz_build_haus_t(); break;
		case WKZ_LAND_CHAIN:       tool = new wkz_build_industries_land_t(); break;
		case WKZ_CITY_CHAIN:       tool = new wkz_build_industries_city_t(); break;
		case WKZ_BUILD_FACTORY:    tool = new wkz_build_factory_t(); break;
		case WKZ_LINK_FACTORY:     tool = new wkz_link_factory_t(); break;
		case WKZ_HEADQUARTER:      tool = new wkz_headquarter_t(); break;
		case WKZ_LOCK_GAME:        tool = new wkz_lock_game_t(); break;
		case WKZ_ADD_CITYCAR:      tool = new wkz_add_citycar_t(); break;
		case WKZ_FOREST:           tool = new wkz_forest_t(); break;
		case WKZ_STOP_MOVER:       tool = new wkz_stop_moving_t(); break;
		case WKZ_MAKE_STOP_PUBLIC: tool = new wkz_make_stop_public_t(); break;
		case WKZ_REMOVE_WAYOBJ:    tool = new wkz_wayobj_remover_t(); break;
		case WKZ_SLICED_AND_UNDERGROUND_VIEW: tool = new wkz_show_underground_t(); break;
		case WKZ_BUY_HOUSE:        tool = new wkz_buy_house_t(); break;
		case WKZ_CITYROAD:         tool = new wkz_build_cityroad(); break;
		case WKZ_ERR_MESSAGE_TOOL: tool = new wkz_error_message_t(); break;
		default:                   dbg->error("create_general_tool()","cannot satisfy request for general_tool[%i]!",toolnr);
		                           return NULL;
	}
	// check for right id (exception: WKZ_SLICED_AND_UNDERGROUND_VIEW)
	assert(tool->get_id()  ==  (toolnr | GENERAL_TOOL)  ||  toolnr==WKZ_SLICED_AND_UNDERGROUND_VIEW);
	return tool;
}

werkzeug_t *create_simple_tool(int toolnr)
{
	werkzeug_t* tool = NULL;
	switch(toolnr) {
		case WKZ_PAUSE:             tool = new wkz_pause_t(); break;
		case WKZ_FASTFORWARD:       tool = new wkz_fastforward_t(); break;
		case WKZ_SCREENSHOT:        tool = new wkz_screenshot_t(); break;
		case WKZ_INCREASE_INDUSTRY: tool = new wkz_increase_industry_t(); break;
		case WKZ_UNDO:              tool = new wkz_undo_t(); break;
		case WKZ_SWITCH_PLAYER:     tool = new wkz_switch_player_t(); break;
		case WKZ_STEP_YEAR:         tool = new wkz_step_year_t(); break;
		case WKZ_CHANGE_GAME_SPEED: tool = new wkz_change_game_speed_t(); break;
		case WKZ_ZOOM_IN:           tool = new wkz_zoom_in_t(); break;
		case WKZ_ZOOM_OUT:          tool = new wkz_zoom_out_t(); break;
		case WKZ_SHOW_COVERAGE:     tool = new wkz_show_coverage_t(); break;
		case WKZ_SHOW_NAMES:        tool = new wkz_show_name_t(); break;
		case WKZ_SHOW_GRID:         tool = new wkz_show_grid_t(); break;
		case WKZ_SHOW_TREES:        tool = new wkz_show_trees_t(); break;
		case WKZ_SHOW_HOUSES:       tool = new wkz_show_houses_t(); break;
		case WKZ_SHOW_UNDERGROUND:  tool = new wkz_show_underground_t(); break;
		case WKZ_ROTATE90:          tool = new wkz_rotate90_t(); break;
		case WKZ_QUIT:              tool = new wkz_quit_t(); break;
		case WKZ_FILL_TREES:        tool = new wkz_fill_trees_t(); break;
		case WKZ_DAYNIGHT_LEVEL:    tool = new wkz_daynight_level_t(); break;
		case WKZ_VEHICLE_TOOLTIPS:  tool = new wkz_vehicle_tooltips_t(); break;
		case WKZ_TOOGLE_PAX:        tool = new wkz_toggle_pax_station_t(); break;
		case WKZ_TOOGLE_PEDESTRIANS:tool = new wkz_toggle_pedestrians_t(); break;
		case WKZ_TRAFFIC_LEVEL:     tool = new wkz_traffic_level_t(); break;
		case WKZ_CONVOI_TOOL:       tool = new wkz_change_convoi_t(); break;
		case WKZ_LINE_TOOL:         tool = new wkz_change_line_t(); break;
		case WKZ_DEPOT_TOOL:        tool = new wkz_change_depot_t(); break;
		case UNUSED_WKZ_PWDHASH_TOOL: dbg->warning("create_simple_tool()","deprecated tool [%i] requested", toolnr); return NULL;
		case WKZ_SET_PLAYER_TOOL:   tool = new wkz_change_player_t(); break;
		case WKZ_TRAFFIC_LIGHT_TOOL:tool = new wkz_change_traffic_light_t(); break;
		case WKZ_CHANGE_CITY_TOOL:  tool = new wkz_change_city_t(); break;
		case WKZ_RENAME_TOOL:       tool = new wkz_rename_t(); break;
		case WKZ_ADD_MESSAGE_TOOL:  tool = new wkz_add_message_t(); break;
		case WKZ_TOGGLE_RESERVATION:tool = new wkz_toggle_reservation_t(); break;
		case WKZ_VIEW_OWNER:        tool = new wkz_view_owner_t(); break;
		case WKZ_HIDE_UNDER_CURSOR: tool = new wkz_hide_under_cursor_t(); break;
		default:                    dbg->error("create_simple_tool()","cannot satisfy request for simple_tool[%i]!",toolnr);
		                            return NULL;
	}
	assert(tool->get_id()  ==  (toolnr | SIMPLE_TOOL));
	return tool;
}


werkzeug_t *create_dialog_tool(int toolnr)
{
	werkzeug_t* tool = NULL;
	switch(toolnr) {
		case WKZ_HELP:           tool = new wkz_help_t(); break;
		case WKZ_OPTIONEN:       tool = new wkz_optionen_t(); break;
		case WKZ_MINIMAP:        tool = new wkz_minimap_t(); break;
		case WKZ_LINEOVERVIEW:   tool = new wkz_lines_t(); break;
		case WKZ_MESSAGES:       tool = new wkz_messages_t(); break;
		case WKZ_FINANCES:       tool = new wkz_finances_t(); break;
		case WKZ_PLAYERS:        tool = new wkz_players_t(); break;
		case WKZ_DISPLAYOPTIONS: tool = new wkz_displayoptions_t(); break;
		case WKZ_SOUND:          tool = new wkz_sound_t(); break;
		case WKZ_LANGUAGE:       tool = new wkz_language_t(); break;
		case WKZ_PLAYERCOLOR:    tool = new wkz_playercolor_t(); break;
		case WKZ_JUMP:           tool = new wkz_jump_t(); break;
		case WKZ_LOAD:           tool = new wkz_load_t(); break;
		case WKZ_SAVE:           tool = new wkz_save_t(); break;
		case WKZ_LIST_HALT:      tool = new wkz_list_halt_t(); break;
		case WKZ_LIST_CONVOI:    tool = new wkz_list_convoi_t(); break;
		case WKZ_LIST_TOWN:      tool = new wkz_list_town_t(); break;
		case WKZ_LIST_GOODS:     tool = new wkz_list_goods_t(); break;
		case WKZ_LIST_FACTORY:   tool = new wkz_list_factory_t(); break;
		case WKZ_LIST_CURIOSITY: tool = new wkz_list_curiosity_t(); break;
		case WKZ_EDIT_FACTORY:   tool = new wkz_factorybuilder_t(); break;
		case WKZ_EDIT_ATTRACTION:tool = new wkz_attractionbuilder_t(); break;
		case WKZ_EDIT_HOUSE:     tool = new wkz_housebuilder_t(); break;
		case WKZ_EDIT_TREE:      tool = new wkz_treebuilder_t(); break;
		case WKZ_ENLARGE_MAP:    tool = new wkz_enlarge_map_t(); break;
		case WKZ_LIST_LABEL:     tool = new wkz_list_label_t(); break;
		case WKZ_CLIMATES:       tool = new wkz_climates_t(); break;
		case WKZ_SETTINGS:       tool = new wkz_settings_t(); break;
		case WKZ_GAMEINFO:       tool = new wkz_server_t(); break;
		default:                 dbg->error("create_dialog_tool()","cannot satisfy request for dialog_tool[%i]!",toolnr);
		                         return NULL;
	}
	assert(tool->get_id() == (toolnr | DIALOGE_TOOL));
	return tool;
}

werkzeug_t *create_tool(int toolnr)
{
	werkzeug_t *wkz = NULL;
	if(  toolnr & GENERAL_TOOL  ) {
		wkz = create_general_tool(toolnr & 0xFFF);
	}
	else if(  toolnr & SIMPLE_TOOL  ) {
		wkz = create_simple_tool(toolnr & 0xFFF);
	}
	else if(  toolnr & DIALOGE_TOOL  ) {
		wkz = create_dialog_tool(toolnr & 0xFFF);
	}
	if (wkz == NULL) {
		dbg->error("create_tool()","cannot satisfy request for tool with id %i!",toolnr);
	}
	return wkz;
}


static uint16 str_to_key( const char *str )
{
	if(  str[1]==','  ||  str[1]<=' ') {
		return (uint8)*str;
	}
	else {
		// check for utf8
		if(  127<(uint8)*str  ) {
			size_t len=0;
			uint16 c = utf8_to_utf16( (const utf8 *)str, &len );
			if(str[len]==',') {
				return c;
			}
		}
		// control char
		if(str[0]=='^') {
			return (str[1]&(~32))-64;
		}
		// direct value (decimal)
		if(str[0]=='#') {
			return atoi(str+1);
		}
		// Function key?
		if(str[0]=='F') {
			uint8 function = atoi(str+1);
			if(function>0) {
				return SIM_KEY_F1+function-1;
			}
		}
		// COMMA
		if (strstart(str, "COMMA")) {
			return ',';
		}
		// HOME
		if (strstart(str, "HOME")) {
			return SIM_KEY_HOME;
		}
		// END
		if (strstart(str, "END")) {
			return SIM_KEY_END;
		}
	}
	// invalid key
	return 0xFFFF;
}



// just fills the default tables before other tools are added
void werkzeug_t::init_menu()
{
	for(  uint16 i=0;  i<GENERAL_TOOL_COUNT;  i++  ) {
		werkzeug_t *w = create_general_tool( i );
		general_tool.append(w);
	}
	for(  uint16 i=0;  i<SIMPLE_TOOL_COUNT;  i++  ) {
		werkzeug_t *w = create_simple_tool( i );
		simple_tool.append(w);
	}
	for(  uint16 i=0;  i<DIALOGE_TOOL_COUNT;  i++  ) {
		werkzeug_t *w = create_dialog_tool( i );
		dialog_tool.append(w);
	}
}

void werkzeug_t::exit_menu()
{
	clear_ptr_vector( general_tool );
	clear_ptr_vector( simple_tool );
	clear_ptr_vector( dialog_tool );
}


// for sorting: compare tool key
static bool compare_werkzeug(werkzeug_t const* const a, werkzeug_t const* const b)
{
	uint16 const ac = a->command_key & ~32;
	uint16 const bc = b->command_key & ~32;
	return ac != bc ? ac < bc : a->command_key < b->command_key;
}


// read a tab file to add images, cursors and sound to the tools
void werkzeug_t::read_menu(const std::string &objfilename)
{
	char_to_tool.clear();
	tabfile_t menuconf;
	// only use pak sepcific menues, since otherwise images may missing
	if (!menuconf.open((objfilename+"config/menuconf.tab").c_str())) {
		dbg->fatal("werkzeug_t::init_menu()", "Can't read %sconfig/menuconf.tab", objfilename.c_str() );
	}

	tabfileobj_t contents;
	menuconf.read(contents);

	// structure to hold information for iterating through different tool types
	struct tool_class_info_t {
		const char* type;
		uint16 count;
		vector_tpl<werkzeug_t *> &tools;
		const skin_besch_t *icons;
		const skin_besch_t *cursor;
		bool with_sound;

	};
	tool_class_info_t info[] = {
		{ "general_tool", GENERAL_TOOL_COUNT, general_tool, skinverwaltung_t::werkzeuge_general, skinverwaltung_t::cursor_general, true },
		{ "simple_tool",  SIMPLE_TOOL_COUNT,  simple_tool,  skinverwaltung_t::werkzeuge_simple,  NULL, false},
		{ "dialog_tool",  DIALOGE_TOOL_COUNT, dialog_tool,  skinverwaltung_t::werkzeuge_dialoge, NULL, false }
	};

	// first init all tools
	DBG_MESSAGE( "werkzeug_t::init_menu()", "Reading general menu" );
	for(  uint16 t=0; t<3; t++) {
		for(  uint16 i=0;  i<info[t].count;  i++  ) {
			char id[256];
			sprintf( id, "%s[%i]", info[t].type, i );
			const char *str = contents.get( id );
			/* Format of str:
			 * for general tools: icon,cursor,sound,key
			 *     icon is image number in menu.GeneralTools, cursor image number in cursor.GeneralTools
			 * for simple and dialog tools: icon,key
			 *     icon is image number in menu.SimpleTools and menu.DialogeTools
			 * -1 will disable any of them
			 */
			werkzeug_t *w = info[t].tools[i];
			if(*str  &&  *str!=',') {
				// ok, first comes icon
				while(*str==' ') {
					str++;
				}
				uint16 icon = (uint16)atoi(str);
				if(  icon==0  &&  *str!='0'  ) {
					// check, if file name ...
					int i=0;
					while(  str[i]!=0  &&  str[i]!=','  ) {
						i++;
					}
					const skin_besch_t *s=skinverwaltung_t::get_extra(str,i-1);
					w->icon = s ? s->get_bild_nr(0) : IMG_LEER;
				}
				else {
					if(  icon>=info[t].icons->get_bild_anzahl()  ) {
						dbg->warning( "werkzeug_t::init_menu()", "wrong icon (%i) given for %s[%i]", icon, info[t].type, i );
					}
					w->icon = info[t].icons->get_bild_nr(icon);
				}
				do {
					str++;
				} while(*str  &&  *str!=',');
			}
			if(info[t].cursor) {
				if(*str==',') {
					// next comes cursor
					str++;
					if(*str!=',') {
						uint16 cursor = (uint16)atoi(str);
						if(  cursor>=info[t].cursor->get_bild_anzahl()  ) {
							dbg->warning( "werkzeug_t::init_menu()", "wrong cursor (%i) given for %s[%i]", cursor, info[t].type, i );
						}
						w->cursor = info[t].cursor->get_bild_nr(cursor);
						do {
							str++;
						} while(*str  &&  *str!=',');
					}
				}
			}
			if(info[t].with_sound) {
				if(*str==',') {
					// ok_sound
					str++;
					if(*str!=',') {
						int sound = atoi(str);
						if(  sound>0  ) {
							w->ok_sound = sound_besch_t::get_compatible_sound_id(sound);
						}
						do {
							str++;
						} while(*str  &&  *str!=',');
					}
				}
			}
			if(*str==',') {
				// key
				str++;
				while(*str==' ') {
					str++;
				}
				if(*str>=' ') {
					w->command_key = str_to_key(str);
					char_to_tool.append(w);
				}
			}
		}
	}
	// now the toolbar tools
	DBG_MESSAGE( "werkzeug_t::read_menu()", "Reading toolbars" );
	// default size
	umgebung_t::iconsize = koord( contents.get_int("icon_width",32), contents.get_int("icon_height",32) );
	// first: add main menu
	toolbar_tool.resize( skinverwaltung_t::werkzeuge_toolbars->get_bild_anzahl() );
	toolbar_tool.append(new toolbar_t(TOOLBAR_TOOL, "", "", umgebung_t::iconsize));
	// now for the rest
	for(  uint16 i=0;  i<toolbar_tool.get_count();  i++  ) {
		char id[256];
		for(  int j=0;  ;  j++  ) {
			/* str should now contain something like 1,2,-1
			 * first parameter is the image number in "GeneralTools"
			 * next is the cursor in "GeneralTools"
			 * final is the sound
			 * -1 will disable any of them
			 */
			sprintf( id, "toolbar[%i][%i]", i, j );
			const char *str = contents.get( id );
			if(*str==0) {
				// empty entry => toolbar finished ...
				break;
			}

			werkzeug_t *addtool = NULL;

			/* first, parse the string; we could have up to four parameters */
			const char *toolname = str;
			image_id icon = IMG_LEER;
			const char *key_str = NULL;
			const char *param_str = NULL;	// in case of toolbars, it will also contain the tooltip
			// parse until next zero-level comma
			uint level = 0;
			while(*str) {
				if (*str == ')') {
					level++;
				}
				else if (*str == '(') {
					level--;
				}
				else if (*str == ','  &&  level == 0) {
					break;
				}
				str++;
			}
			// icon
			if(*str==',') {
				str++;
				if(*str!=',') {
					// ok, first come icon
					while(*str==' ') {
						str++;
					}
					icon = (uint16)atoi(str);
					if(  icon==0  &&  *str!='0'  ) {
						// check, if file name ...
						int i=0;
						while(  str[i]!=0  &&  str[i]!=','  ) {
							i++;
						}
						const skin_besch_t *s=skinverwaltung_t::get_extra(str,i-1);
						icon = s ? s->get_bild_nr(0) : IMG_LEER;
					}
					else {
						if(  icon>=skinverwaltung_t::werkzeuge_toolbars->get_bild_anzahl()  ) {
							dbg->warning( "werkzeug_t::read_menu()", "wrong icon (%i) given for toolbar_tool[%i][%i]", icon, i, j );
							icon = 0;
						}
						icon = skinverwaltung_t::werkzeuge_toolbars->get_bild_nr(icon);
					}
					while(*str  &&  *str!=',') {
						str++;
					}
				}
			}
			// key
			if(*str==',') {
				str++;
				while(*str==' '  &&  *str) {
					str ++;
				}
				if(*str!=',' &&  *str) {
					key_str = str;
				}
				while(*str!=','  &&  *str) {
					str ++;
				}
			}
			// parameter
			if(*str==',') {
				str++;
				if(*str>=' ') {
					param_str = str;
				}
			}
			bool create_tool = icon!=IMG_LEER  ||  key_str  ||  param_str;

			if (char const* const c = strstart(toolname, "general_tool[")) {
				uint8 toolnr = atoi(c);
				if(  toolnr<GENERAL_TOOL_COUNT  ) {
					if(create_tool) {
						// compatibility mode: wkz_cityroad is used for wkz_wegebau with defaultparam 'cityroad'
						if(  toolnr==WKZ_WEGEBAU  &&  param_str  &&  strcmp(param_str,"city_road")==0) {
							toolnr = WKZ_CITYROAD;
							dbg->warning("werkzeug_t::read_menu()", "toolbar[%i][%i]: replaced way-builder(id=14) with default param=cityroad by cityroad builder(id=36)", i,j);
						}
						// now create tool
						addtool = create_general_tool( toolnr );
						// copy defaults
						*addtool = *(general_tool[toolnr]);

						general_tool.append( addtool );
					}
					else {
						addtool = general_tool[toolnr];
					}
				}
				else {
					dbg->error( "werkzeug_t::read_menu()", "When parsing menuconf.tab: No general tool %i defined (max %i)!", toolnr, GENERAL_TOOL_COUNT );
				}
			} else if (char const* const c = strstart(toolname, "simple_tool[")) {
				uint8 const toolnr = atoi(c);
				if(  toolnr<SIMPLE_TOOL_COUNT  ) {
					if(create_tool) {
						addtool = create_simple_tool( toolnr );
						*addtool = *(simple_tool[toolnr]);
						simple_tool.append( addtool );
					}
					else {
						addtool = simple_tool[toolnr];
					}
				}
				else {
					dbg->error( "werkzeug_t::read_menu()", "When parsing menuconf.tab: No simple tool %i defined (max %i)!", toolnr, SIMPLE_TOOL_COUNT );
				}
			} else if (char const* const c = strstart(toolname, "dialog_tool[")) {
				uint8 const toolnr = atoi(c);
				if(  toolnr<DIALOGE_TOOL_COUNT  ) {
					if(create_tool) {
						addtool = create_dialog_tool( toolnr );
						*addtool = *(dialog_tool[toolnr]);
						dialog_tool.append( addtool );
					}
					else {
						addtool = dialog_tool[toolnr];
					}
				}
				else {
					dbg->error( "werkzeug_t::read_menu()", "When parsing menuconf.tab: No dialog tool %i defined (max %i)!", toolnr, DIALOGE_TOOL_COUNT );
				}
			} else if (char const* const c = strstart(toolname, "toolbar[")) {
				uint8 const toolnr = atoi(c);
				assert(toolnr>0);
				if(toolbar_tool.get_count()==toolnr) {
					if(param_str==NULL) {
						param_str = "Unnamed toolbar";
						dbg->warning( "werkzeug_t::read_menu()", "Missing title for toolbar[%d]", toolnr);
					}
					char *c = strdup(param_str);
					const char *title = c;
					c += strcspn(c, ",");
					if (*c != '\0') *c++ = '\0';
					toolbar_t* const tb = new toolbar_t(toolbar_tool.get_count() | TOOLBAR_TOOL, title, c, umgebung_t::iconsize);
					toolbar_tool.append(tb);
					addtool = tb;
				}
			}
			else {
				// make a default tool to add the parameter here
				addtool = new wkz_dummy_t();
				addtool->default_param = strdup(toolname);
				addtool->command_key = 1;
			}
			if(addtool) {
				if(icon!=IMG_LEER) {
					addtool->icon = icon;
				}
				if(key_str!=NULL) {
					addtool->command_key = str_to_key(key_str);
					char_to_tool.append(addtool);
				}
				if(param_str!=NULL  &&  ((addtool->get_id() & TOOLBAR_TOOL) == 0)) {
					addtool->default_param = strdup(param_str);
				}
				toolbar_tool[i]->append(addtool);
			}
		}
	}
	// sort characters
	std::sort(char_to_tool.begin(), char_to_tool.end(), compare_werkzeug);
}


void werkzeug_t::update_toolbars(karte_t *welt)
{
	// renew toolbar
	// iterate twice, to get correct icons if a toolbar changes between empty and non-empty
	for(uint j=0; j<2; j++) {
		bool change = false;
		FOR(vector_tpl<toolbar_t*>, const i, toolbar_tool) {
			bool old_icon_empty = i->get_icon(welt->get_active_player()) == IMG_LEER;
			i->update(welt, welt->get_active_player());
			change |= old_icon_empty ^ (i->get_icon(welt->get_active_player()) == IMG_LEER);
		}
		if (!change) {
			// no toolbar changes between empty and non-empty, no need to loop again
			break;
		}
	}
}


void werkzeug_t::draw_after(karte_t *welt, koord pos, bool dirty) const
{
	// default action: grey corner if selected
	image_id id = get_icon( welt->get_active_player() );
	if(  id!=IMG_LEER  &&  is_selected(welt)  ) {
		display_img_blend( id, pos.x, pos.y, TRANSPARENT50_FLAG|OUTLINE_FLAG|COL_BLACK, false, dirty );
	}
}

bool werkzeug_t::is_selected(const karte_t *welt) const
{
	return welt->get_werkzeug(welt->get_active_player_nr())==this;
}

const char *werkzeug_t::check_pos( karte_t *welt, spieler_t *, koord3d pos )
{
	grund_t *gr = welt->lookup(pos);
	return (gr  &&  !gr->is_visible()) ? "" : NULL;
}

bool werkzeug_t::check_valid_pos( karte_t *w, koord k ) const
{
	if(is_grid_tool()) {
		return w->is_within_grid_limits(k);
	}
	return w->is_within_limits(k);
};

/**
 * Initializes cursor object: image, y-offset, size of marked area,
 * has to be called after init().
 * @param zeiger cursor object
 */
void werkzeug_t::init_cursor( zeiger_t *zeiger) const
{
	zeiger->set_bild( cursor );
	zeiger->set_yoff( offset );
	zeiger->set_area( cursor_area, cursor_centered);
}

const char *kartenboden_werkzeug_t::check_pos( karte_t *welt, spieler_t *, koord3d pos )
{
	grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
	return (gr  &&  !gr->is_visible()) ? "" : NULL;
}



image_id toolbar_t::get_icon(spieler_t *sp) const
{
	// no image for edit tools => do not open
	if(  icon==IMG_LEER  ||  (sp!=NULL  &&  strcmp(default_param,"EDITTOOLS")==0  &&  sp->get_player_nr()!=1)  ) {
		return IMG_LEER;
	}
	// now have we a least one visible tool?
	if (wzw  &&  !wzw->empty(sp)) {
		return icon;
	}
	return IMG_LEER;
}



// simply true, if visible
bool toolbar_t::is_selected(const karte_t *) const
{
	return win_get_magic(magic_toolbar + toolbar_tool.index_of(const_cast<toolbar_t*>(this)));
}


// just returns sound info after bracket
static sint16 get_sound( const char *c )
{
	while(  *c  &&  *c!=')'  ) {
		c++;
	}
	while(  *c  &&  *c!=','  ) {
		c++;
	}
	return (*c ? atoi( c+1 )-2 : NO_SOUND);
}



// fills and displays a toolbar
void toolbar_t::update(karte_t *welt, spieler_t *sp)
{
	const bool create = (wzw == NULL);
	if(create) {
		DBG_MESSAGE("toolbar_t::update()","create toolbar %s",default_param);
		wzw = new werkzeug_waehler_t( welt, default_param, helpfile, toolbar_tool.index_of(this), iconsize, this!=werkzeug_t::toolbar_tool[0] );
	}
	else {
		DBG_MESSAGE("toolbar_t::update()","update toolbar %s",default_param);
	}

	wzw->reset_tools();
	// now (re)fill it
	FOR(slist_tpl<werkzeug_t*>, const w, tools) {
		// no way to call this tool? => then it is most likely a metatool
		if(w->command_key==1  &&  w->get_icon(welt->get_active_player())==IMG_LEER) {
			if (char const* const param = w->get_default_param()) {
				if(  create  ) {
					DBG_DEBUG("toolbar_t::update()", "add metatool (param=%s)", param);
				}
				if (char const* c = strstart(param, "ways(")) {
					waytype_t way = (waytype_t)atoi(c);
					while(*c  &&  *c!=','  &&  *c!=')') {
						c++;
					}
					weg_t::system_type subtype = (weg_t::system_type)(*c!=0 ? atoi(++c) : 0);
					wegbauer_t::fill_menu( wzw, way, subtype, get_sound(c), welt );
				} else if (char const* const c = strstart(param, "bridges(")) {
					waytype_t const way = (waytype_t)atoi(c);
					brueckenbauer_t::fill_menu(wzw, way, get_sound(c), welt);
				} else if (char const* const c = strstart(param, "tunnels(")) {
					waytype_t const way = (waytype_t)atoi(c);
					tunnelbauer_t::fill_menu(wzw, way, get_sound(c), welt);
				} else if (char const* const c = strstart(param, "signs(")) {
					waytype_t const way = (waytype_t)atoi(c);
					roadsign_t::fill_menu(wzw, way, get_sound(c), welt);
				} else if (char const* const c = strstart(param, "wayobjs(")) {
					waytype_t const way = (waytype_t)atoi(c);
					wayobj_t::fill_menu(wzw, way, get_sound(c), welt);
				} else if (char const* c = strstart(param, "buildings(")) {
					haus_besch_t::utyp const utype = (haus_besch_t::utyp)atoi(c);
					while(*c  &&  *c!=','  &&  *c!=')') {
						c++;
					}
					waytype_t way = (waytype_t)(*c!=0 ? atoi(++c) : 0);
					hausbauer_t::fill_menu( wzw, utype, way, get_sound(c), welt );
				} else if (param[0] == '-') {
					// add dummy werkzeug as seperator
					wzw->add_werkzeug( dummy );
				}
			}
		}
		else if(w->get_icon(welt->get_active_player())!=IMG_LEER) {
			// get the right city_road
			if(w->get_id() == (WKZ_CITYROAD | GENERAL_TOOL)) {
				w->flags = 0;
				w->init(welt,sp);
			}
			if(  create  ) {
				DBG_DEBUG( "toolbar_t::update()", "add tool %i (param=%s)", w->get_id(), w->get_default_param() );
			}
			scenario_t *scen = welt->get_scenario();
			if(  scen->is_scripted()  &&  !scen->is_tool_allowed(sp, w->get_id(), w->get_waytype())) {
				continue;
			}
			// now add it to the toolbar gui
			wzw->add_werkzeug( w );
		}
	}

	if(  (strcmp(this->default_param,"EDITTOOLS")==0  &&  sp!=welt->get_spieler(1))  ) {
		destroy_win(wzw);
		return;
	}
}



// fills and displays a toolbar
bool toolbar_t::init(karte_t *welt, spieler_t *sp)
{
	update( welt, sp );
	bool close = (strcmp(this->default_param,"EDITTOOLS")==0  &&  sp!=welt->get_spieler(1));

	// show/create window
	if(  win_get_magic(magic_toolbar+toolbar_tool.index_of(this))  ) {
		if(close) {
			destroy_win(wzw);
		}
		else {
			top_win(wzw);
		}

	}
	else if(!close  &&  this!=werkzeug_t::toolbar_tool[0]) {
		// not open and not main menu
		create_win( wzw, w_info|w_do_not_delete|w_no_overlap, magic_toolbar+toolbar_tool.index_of(this) );
		DBG_MESSAGE("toolbar_t::init()", "ID=%id", get_id());
	}
	return false;
}


bool toolbar_t::exit( karte_t *, spieler_t *)
{
	if(  win_get_magic(magic_toolbar+toolbar_tool.index_of(this))  ) {
		destroy_win(wzw);
	}
	return false;
}


bool two_click_werkzeug_t::init( karte_t *welt, spieler_t *)
{
	first_click_var = true;
	start = koord3d::invalid;
	if (is_local_execution()) {
		welt->show_distance = koord3d::invalid;
	}
	cleanup( true );
	return true;
}


void two_click_werkzeug_t::rdwr_custom_data(uint8, memory_rw_t *packet)
{
	packet->rdwr_bool(first_click_var);
	sint16 posx = start.x; packet->rdwr_short(posx); start.x = posx;
	sint16 posy = start.y; packet->rdwr_short(posy); start.y = posy;
	sint8  posz = start.z; packet->rdwr_byte(posz);  start.z = posz;
}


bool two_click_werkzeug_t::is_first_click() const
{
	return first_click_var;
}


bool two_click_werkzeug_t::is_work_here_network_save( karte_t *welt, spieler_t *sp, koord3d pos )
{
	if(  !is_first_click()  ) {
		return false;
	}
	const char *error = "";	//default: nosound
	uint8 value = is_valid_pos( welt, sp, pos, error, koord3d::invalid );
	DBG_MESSAGE("two_click_werkzeug_t::is_work_here_network_save", "Position %s valid=%d", pos.get_str(), value );
	if(  value == 0  ) {
		return false;
	}

	// work directly if possible and ctrl is NOT pressed
	if( (value & 1)  &&  !( (value & 2)  &&  is_ctrl_pressed())) {
		// would work here directly.
		return false;
	}
	else {
		// set starting position only
		return true;
	}
}


const char *two_click_werkzeug_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	if(  !is_first_click()  &&  start_marker  ) {
		start = start_marker->get_pos(); // if map was rotated.
	}

	// remove marker
	cleanup( true );

	const char *error = "";	//default: nosound
	uint8 value = is_valid_pos( welt, sp, pos, error, !is_first_click() ? start : koord3d::invalid );
	DBG_MESSAGE("two_click_werkzeug_t::work", "Position %s valid=%d", pos.get_str(), value );
	if(  value == 0  ) {
		flags &= ~(WFL_SHIFT | WFL_CTRL);
		init( welt, sp );
		return error;
	}

	if(  is_first_click()  ) {
		// work directly if possible and ctrl is NOT pressed
		if( (value & 1)  &&  !( (value & 2)  &&  is_ctrl_pressed())) {
			// Work here directly.
			DBG_MESSAGE("two_click_werkzeug_t::work", "Call tool at %s", pos.get_str() );
			error = do_work( welt, sp, pos, koord3d::invalid );
		}
		else {
			// set starting position.
			DBG_MESSAGE("two_click_werkzeug_t::work", "Setting start to %s", pos.get_str() );
			start_at( welt, pos );
		}
	}
	else {
		if( value & 2 ) {
			DBG_MESSAGE("two_click_werkzeug_t::work", "Setting end to %s", pos.get_str() );
			error = do_work( welt, sp, start, pos );
		}
		flags &= ~(WFL_SHIFT | WFL_CTRL);
		init( welt, sp ); // Do the cleanup stuff after(!) do_work (otherwise start==koord3d::invalid).
	}
	return error;
}


const char *two_click_werkzeug_t::move( karte_t *welt, spieler_t *sp, uint16 buttonstate, koord3d pos )
{
	DBG_MESSAGE("two_click_werkzeug_t::move", "Button: %d, Pos: %s", buttonstate, pos.get_str());
	if(  buttonstate == 0  ) {
		return "";
	}

	if(  start == pos  ) {
		init( welt, sp );
	}

	const char *error = NULL;

	if(  start == koord3d::invalid  ) {
		// start dragging.
		cleanup( true );

		uint8 value = is_valid_pos( welt, sp, pos, error, koord3d::invalid );
		if( error || value == 0 ) {
			return error;
		}
		if( value & 2 ) {
			start_at( welt, pos );
		}
	}
	else {
		// continue dragging.
		cleanup( false );

		if( start_marker ) {
			start = start_marker->get_pos(); // if map was rotated.
		}
		uint8 value = is_valid_pos( welt, sp, pos, error, start );
		if( error || value == 0 ) {
			return error;
		}
		if( value & 2 ) {
			display_show_load_pointer( true );
			mark_tiles( welt, sp, start, pos );
			display_show_load_pointer( false );
		}
	}
	return "";
}


void two_click_werkzeug_t::start_at( karte_t *welt, koord3d &new_start )
{
	first_click_var = false;
	start = new_start;
	if (is_local_execution()) {
		welt->show_distance = new_start;
		start_marker = new zeiger_t(welt, start, NULL);
		start_marker->set_bild( get_marker_image() );
		grund_t *gr = welt->lookup( start );
		if( gr ) {
			gr->obj_add(start_marker);
		}
	}
	DBG_MESSAGE("two_click_werkzeug_t::start_at", "Setting start to %s", start.get_str());
}


void two_click_werkzeug_t::cleanup( bool delete_start_marker )
{
	karte_t *welt = spieler_t::get_welt();
	// delete marker.
	if(  start_marker!=NULL  &&  delete_start_marker) {
		start_marker->mark_image_dirty( start_marker->get_bild(), 0 );
		delete start_marker;
		start_marker = NULL;
	}
	// delete old route.
	while(!marked.empty()) {
		zeiger_t *z = marked.remove_first();
		z->mark_image_dirty( z->get_bild(), 0 );
		z->mark_image_dirty( z->get_after_bild(), 0 );
		koord3d pos = z->get_pos();
		grund_t *gr = welt->lookup( pos );
		delete z;
		// Remove dummy ground (placed by wkz_tunnelbau_t and wkz_wegebau_t):
		if( (gr->get_typ() == grund_t::tunnelboden  ||  gr->get_typ() == grund_t::monorailboden)  &&  gr->get_weg_nr(0) == NULL && !gr->get_leitung() ) {
			welt->access(pos.get_2d())->boden_entfernen(gr);
			delete gr;
			assert( !welt->lookup(pos));
		}
	}
	// delete tooltip.
	win_set_static_tooltip( NULL );
}


image_id two_click_werkzeug_t::get_marker_image()
{
	return skinverwaltung_t::bauigelsymbol->get_bild_nr(0);
}
