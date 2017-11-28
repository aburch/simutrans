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
#include "gui/simwin.h"
#include "player/simplay.h"
#include "simmenu.h"
#include "simtool.h"
#include "simtool-dialogs.h"
#include "simskin.h"
#include "simsound.h"

#include "bauer/hausbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/brueckenbauer.h"
#include "bauer/tunnelbauer.h"

#include "descriptor/building_desc.h"

#include "boden/grund.h"
#include "boden/wege/strasse.h"

#include "dataobj/environment.h"
#include "dataobj/tabfile.h"
#include "dataobj/scenario.h"

#include "obj/roadsign.h"
#include "obj/wayobj.h"
#include "obj/zeiger.h"

#include "gui/tool_selector.h"

#include "utils/simstring.h"
#include "network/memory_rw.h"

karte_ptr_t tool_t::welt;

// for key lookup; is always sorted during the game
vector_tpl<tool_t *>tool_t::char_to_tool(0);

// here are the default values, icons, cursor, sound definitions ...
vector_tpl<tool_t *>tool_t::general_tool(GENERAL_TOOL_COUNT);
vector_tpl<tool_t *>tool_t::simple_tool(SIMPLE_TOOL_COUNT);
vector_tpl<tool_t *>tool_t::dialog_tool(DIALOGE_TOOL_COUNT);

// the number of toolbars is not know yet
vector_tpl<toolbar_t *>tool_t::toolbar_tool(0);

char tool_t::toolstr[1024];

toolbar_last_used_t *toolbar_last_used_t::last_used_tools = NULL;

// separator in toolbars
class tool_dummy_t : public tool_t {
public:
	tool_dummy_t() : tool_t(dummy_id) {}

	bool init(player_t*) OVERRIDE { return false; }
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
	bool is_move_network_save(player_t*) const OVERRIDE { return true; }
};
tool_t *tool_t::dummy = new tool_dummy_t();

tool_t *create_general_tool(int toolnr)
{
	tool_t* tool = NULL;
	switch(toolnr) {
		case TOOL_QUERY:            tool = new tool_query_t(); break;
		case TOOL_REMOVER:          tool = new tool_remover_t(); break;
		case TOOL_RAISE_LAND:       tool = new tool_raise_t(); break;
		case TOOL_LOWER_LAND:       tool = new tool_lower_t(); break;
		case TOOL_SETSLOPE:         tool = new tool_setslope_t(); break;
		case TOOL_RESTORESLOPE:     tool = new tool_restoreslope_t(); break;
		case TOOL_MARKER:           tool = new tool_marker_t(); break;
		case TOOL_CLEAR_RESERVATION:tool = new tool_clear_reservation_t(); break;
		case TOOL_TRANSFORMER:      tool = new tool_transformer_t(); break;
		case TOOL_ADD_CITY:         tool = new tool_add_city_t(); break;
		case TOOL_CHANGE_CITY_SIZE: tool = new tool_change_city_size_t(); break;
		case TOOL_PLANT_TREE:       tool = new tool_plant_tree_t(); break;
		case TOOL_SCHEDULE_ADD:     tool = new tool_schedule_add_t(); break;
		case TOOL_SCHEDULE_INS:     tool = new tool_schedule_ins_t(); break;
		case TOOL_BUILD_WAY:        tool = new tool_build_way_t(); break;
		case TOOL_BUILD_BRIDGE:     tool = new tool_build_bridge_t(); break;
		case TOOL_BUILD_TUNNEL:     tool = new tool_build_tunnel_t(); break;
		case TOOL_WAYREMOVER:       tool = new tool_wayremover_t(); break;
		case TOOL_BUILD_WAYOBJ:     tool = new tool_build_wayobj_t(); break;
		case TOOL_BUILD_STATION:    tool = new tool_build_station_t(); break;
		case TOOL_BUILD_ROADSIGN:   tool = new tool_build_roadsign_t(); break;
		case TOOL_BUILD_DEPOT:      tool = new tool_build_depot_t(); break;
		case TOOL_BUILD_HOUSE:      tool = new tool_build_house_t(); break;
		case TOOL_BUILD_LAND_CHAIN: tool = new tool_build_land_chain_t(); break;
		case TOOL_CITY_CHAIN:       tool = new tool_city_chain_t(); break;
		case TOOL_BUILD_FACTORY:    tool = new tool_build_factory_t(); break;
		case TOOL_LINK_FACTORY:     tool = new tool_link_factory_t(); break;
		case TOOL_HEADQUARTER:      tool = new tool_headquarter_t(); break;
		case TOOL_LOCK_GAME:        tool = new tool_lock_game_t(); break;
		case TOOL_ADD_CITYCAR:      tool = new tool_add_citycar_t(); break;
		case TOOL_FOREST:           tool = new tool_forest_t(); break;
		case TOOL_STOP_MOVER:       tool = new tool_stop_mover_t(); break;
		case TOOL_MAKE_STOP_PUBLIC: tool = new tool_make_stop_public_t(); break;
		case TOOL_REMOVE_WAYOBJ:    tool = new tool_remove_wayobj_t(); break;
		case TOOL_SLICED_AND_UNDERGROUND_VIEW: tool = new tool_show_underground_t(); break;
		case TOOL_BUY_HOUSE:        tool = new tool_buy_house_t(); break;
		case TOOL_BUILD_CITYROAD:   tool = new tool_build_cityroad(); break;
		case TOOL_ERROR_MESSAGE: tool = new tool_error_message_t(); break;
		case TOOL_CHANGE_WATER_HEIGHT: tool = new tool_change_water_height_t(); break;
		case TOOL_SET_CLIMATE:      tool = new tool_set_climate_t(); break;
		default:                   dbg->error("create_general_tool()","cannot satisfy request for general_tool[%i]!",toolnr);
		                           return NULL;
	}
	// check for right id (exception: TOOL_SLICED_AND_UNDERGROUND_VIEW)
	assert(tool->get_id()  ==  (toolnr | GENERAL_TOOL)  ||  toolnr==TOOL_SLICED_AND_UNDERGROUND_VIEW);
	return tool;
}

tool_t *create_simple_tool(int toolnr)
{
	tool_t* tool = NULL;
	switch(toolnr) {
		case TOOL_PAUSE:             tool = new tool_pause_t(); break;
		case TOOL_FASTFORWARD:       tool = new tool_fastforward_t(); break;
		case TOOL_SCREENSHOT:        tool = new tool_screenshot_t(); break;
		case TOOL_INCREASE_INDUSTRY: tool = new tool_increase_industry_t(); break;
		case TOOL_UNDO:              tool = new tool_undo_t(); break;
		case TOOL_SWITCH_PLAYER:     tool = new tool_switch_player_t(); break;
		case TOOL_STEP_YEAR:         tool = new tool_step_year_t(); break;
		case TOOL_CHANGE_GAME_SPEED: tool = new tool_change_game_speed_t(); break;
		case TOOL_ZOOM_IN:           tool = new tool_zoom_in_t(); break;
		case TOOL_ZOOM_OUT:          tool = new tool_zoom_out_t(); break;
		case TOOL_SHOW_COVERAGE:     tool = new tool_show_coverage_t(); break;
		case TOOL_SHOW_NAME:        tool = new tool_show_name_t(); break;
		case TOOL_SHOW_GRID:         tool = new tool_show_grid_t(); break;
		case TOOL_SHOW_TREES:        tool = new tool_show_trees_t(); break;
		case TOOL_SHOW_HOUSES:       tool = new tool_show_houses_t(); break;
		case TOOL_SHOW_UNDERGROUND:  tool = new tool_show_underground_t(); break;
		case TOOL_ROTATE90:          tool = new tool_rotate90_t(); break;
		case TOOL_QUIT:              tool = new tool_quit_t(); break;
		case TOOL_FILL_TREES:        tool = new tool_fill_trees_t(); break;
		case TOOL_DAYNIGHT_LEVEL:    tool = new tool_daynight_level_t(); break;
		case TOOL_VEHICLE_TOOLTIPS:  tool = new tool_vehicle_tooltips_t(); break;
		case TOOL_TOOGLE_PAX:        tool = new tool_toggle_pax_station_t(); break;
		case TOOL_TOOGLE_PEDESTRIANS:tool = new tool_toggle_pedestrians_t(); break;
		case TOOL_TRAFFIC_LEVEL:     tool = new tool_traffic_level_t(); break;
		case TOOL_CHANGE_CONVOI:       tool = new tool_change_convoi_t(); break;
		case TOOL_CHANGE_LINE:         tool = new tool_change_line_t(); break;
		case TOOL_CHANGE_DEPOT:        tool = new tool_change_depot_t(); break;
		case UNUSED_WKZ_PWDHASH_TOOL: dbg->warning("create_simple_tool()","deprecated tool [%i] requested", toolnr); return NULL;
		case TOOL_CHANGE_PLAYER:   tool = new tool_change_player_t(); break;
		case TOOL_CHANGE_TRAFFIC_LIGHT:tool = new tool_change_traffic_light_t(); break;
		case TOOL_CHANGE_CITY:  tool = new tool_change_city_t(); break;
		case TOOL_RENAME:       tool = new tool_rename_t(); break;
		case TOOL_ADD_MESSAGE:  tool = new tool_add_message_t(); break;
		case TOOL_TOGGLE_RESERVATION:tool = new tool_toggle_reservation_t(); break;
		case TOOL_VIEW_OWNER:        tool = new tool_view_owner_t(); break;
		case TOOL_HIDE_UNDER_CURSOR: tool = new tool_hide_under_cursor_t(); break;
		default:                    dbg->error("create_simple_tool()","cannot satisfy request for simple_tool[%i]!",toolnr);
		                            return NULL;
	}
	assert(tool->get_id()  ==  (toolnr | SIMPLE_TOOL));
	return tool;
}


tool_t *create_dialog_tool(int toolnr)
{
	tool_t* tool = NULL;
	switch(toolnr) {
		case DIALOG_HELP:           tool = new dialog_help_t(); break;
		case DIALOG_OPTIONS:        tool = new dialog_options_t(); break;
		case DIALOG_MINIMAP:        tool = new dialog_minimap_t(); break;
		case DIALOG_LINEOVERVIEW:   tool = new dialog_lines_t(); break;
		case DIALOG_MESSAGES:       tool = new dialog_messages_t(); break;
		case DIALOG_FINANCES:       tool = new dialog_finances_t(); break;
		case DIALOG_PLAYERS:        tool = new dialog_players_t(); break;
		case DIALOG_DISPLAYOPTIONS: tool = new dialog_displayoptions_t(); break;
		case DIALOG_SOUND:          tool = new dialog_sound_t(); break;
		case DIALOG_LANGUAGE:       tool = new dialog_language_t(); break;
		case DIALOG_PLAYERCOLOR:    tool = new dialog_playercolor_t(); break;
		case DIALOG_JUMP:           tool = new dialog_jump_t(); break;
		case DIALOG_LOAD:           tool = new dialog_load_t(); break;
		case DIALOG_SAVE:           tool = new dialog_save_t(); break;
		case DIALOG_LIST_HALT:      tool = new dialog_list_halt_t(); break;
		case DIALOG_LIST_CONVOI:    tool = new dialog_list_convoi_t(); break;
		case DIALOG_LIST_TOWN:      tool = new dialog_list_town_t(); break;
		case DIALOG_LIST_GOODS:     tool = new dialog_list_goods_t(); break;
		case DIALOG_LIST_FACTORY:   tool = new dialog_list_factory_t(); break;
		case DIALOG_LIST_CURIOSITY: tool = new dialog_list_curiosity_t(); break;
		case DIALOG_EDIT_FACTORY:   tool = new dialog_edit_factory_t(); break;
		case DIALOG_EDIT_ATTRACTION:tool = new dialog_edit_attraction_t(); break;
		case DIALOG_EDIT_HOUSE:     tool = new dialog_edit_house_t(); break;
		case DIALOG_EDIT_TREE:      tool = new dialog_edit_tree_t(); break;
		case DIALOG_ENLARGE_MAP:    tool = new dialog_enlarge_map_t(); break;
		case DIALOG_LIST_LABEL:     tool = new dialog_list_label_t(); break;
		case DIALOG_CLIMATES:       tool = new dialog_climates_t(); break;
		case DIALOG_SETTINGS:       tool = new dialog_settings_t(); break;
		case DIALOG_GAMEINFO:       tool = new dialog_gameinfo_t(); break;
		case DIALOG_THEMES:         tool = new dialog_themes_t(); break;
		default:                 dbg->error("create_dialog_tool()","cannot satisfy request for dialog_tool[%i]!",toolnr);
		                         return NULL;
	}
	assert(tool->get_id() == (toolnr | DIALOGE_TOOL));
	return tool;
}

tool_t *create_tool(int toolnr)
{
	tool_t *tool = NULL;
	if(  toolnr & GENERAL_TOOL  ) {
		tool = create_general_tool(toolnr & 0xFFF);
	}
	else if(  toolnr & SIMPLE_TOOL  ) {
		tool = create_simple_tool(toolnr & 0xFFF);
	}
	else if(  toolnr & DIALOGE_TOOL  ) {
		tool = create_dialog_tool(toolnr & 0xFFF);
	}
	if (tool == NULL) {
		dbg->error("create_tool()","cannot satisfy request for tool with id %i!",toolnr);
	}
	return tool;
}


static utf32 str_to_key( const char *str )
{
	if(  str[1]==','  ||  str[1]<=' ') {
		return (uint8)*str;
	}
	else {
		// check for utf8
		if(  127<(uint8)*str  ) {
			size_t len = 0;
			utf32 const c = utf8_decoder_t::decode((utf8 const *)str, len);
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
void tool_t::init_menu()
{
	for(  uint16 i=0;  i<GENERAL_TOOL_COUNT;  i++  ) {
		tool_t *tool = create_general_tool( i );
		general_tool.append(tool);
	}
	for(  uint16 i=0;  i<SIMPLE_TOOL_COUNT;  i++  ) {
		tool_t *tool = create_simple_tool( i );
		simple_tool.append(tool);
	}
	for(  uint16 i=0;  i<DIALOGE_TOOL_COUNT;  i++  ) {
		tool_t *tool = create_dialog_tool( i );
		dialog_tool.append(tool);
	}
}

void tool_t::exit_menu()
{
	clear_ptr_vector( general_tool );
	clear_ptr_vector( simple_tool );
	clear_ptr_vector( dialog_tool );
}


// for sorting: compare tool key
static bool compare_tool(tool_t const* const a, tool_t const* const b)
{
	uint16 const ac = a->command_key & ~32;
	uint16 const bc = b->command_key & ~32;
	return ac != bc ? ac < bc : a->command_key < b->command_key;
}


// read a tab file to add images, cursors and sound to the tools
void tool_t::read_menu(const std::string &objfilename)
{
	char_to_tool.clear();
	tabfile_t menuconf;
	// only use pak specific menus, since otherwise images may be missing
	if (!menuconf.open((objfilename+"config/menuconf.tab").c_str())) {
		dbg->fatal("tool_t::init_menu()", "Can't read %sconfig/menuconf.tab", objfilename.c_str() );
	}

	tabfileobj_t contents;
	menuconf.read(contents);

	// structure to hold information for iterating through different tool types
	struct tool_class_info_t {
		const char* type;
		uint16 count;
		vector_tpl<tool_t *> &tools;
		const skin_desc_t *icons;
		const skin_desc_t *cursor;
		bool with_sound;

	};
	tool_class_info_t info[] = {
		{ "general_tool", GENERAL_TOOL_COUNT, general_tool, skinverwaltung_t::tool_icons_general, skinverwaltung_t::cursor_general, true },
		{ "simple_tool",  SIMPLE_TOOL_COUNT,  simple_tool,  skinverwaltung_t::tool_icons_simple,  NULL, false},
		{ "dialog_tool",  DIALOGE_TOOL_COUNT, dialog_tool,  skinverwaltung_t::tool_icons_dialoge, NULL, false }
	};

	// first init all tools
	DBG_MESSAGE( "tool_t::init_menu()", "Reading general menu" );
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
			tool_t *tool = info[t].tools[i];
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
					const skin_desc_t *s=skinverwaltung_t::get_extra(str,i-1);
					tool->icon = s ? s->get_image_id(0) : IMG_EMPTY;
				}
				else {
					if(  icon>=info[t].icons->get_count()  ) {
						dbg->warning( "tool_t::init_menu()", "wrong icon (%i) given for %s[%i]", icon, info[t].type, i );
					}
					tool->icon = info[t].icons->get_image_id(icon);
				}
				do {
					str++;
				} while(*str  &&  *str!=',');
			}
			if(info[t].cursor) {
				if(*str==',') {
					// next comes cursor
					str++;
					if(*str  &&  *str!=',') {
						uint16 cursor = (uint16)atoi(str);
						if(  cursor>=info[t].cursor->get_count()  ) {
							dbg->warning( "tool_t::init_menu()", "wrong cursor (%i) given for %s[%i]", cursor, info[t].type, i );
						}
						tool->cursor = info[t].cursor->get_image_id(cursor);
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
					if(*str  &&  *str!=',') {
						int sound = atoi(str);
						if(  sound>0  ) {
							tool->ok_sound = sound_desc_t::get_compatible_sound_id(sound);
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
					tool->command_key = str_to_key(str);
					char_to_tool.append(tool);
				}
			}
		}
	}

	// now the toolbar tools
	DBG_MESSAGE( "tool_t::read_menu()", "Reading toolbars" );
	toolbar_last_used_t::last_used_tools = new toolbar_last_used_t( TOOL_LAST_USED | TOOLBAR_TOOL, "Last used tools", "last_used.txt" );
	// first: add main menu
	toolbar_tool.resize( skinverwaltung_t::tool_icons_toolbars->get_count() );
	toolbar_tool.append(new toolbar_t(TOOLBAR_TOOL, "", ""));
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

			tool_t *addtool = NULL;

			/* first, parse the string; we could have up to four parameters */
			const char *toolname = str;
			image_id icon = IMG_EMPTY;
			const char *key_str = NULL;
			const char *param_str = NULL; // in case of toolbars, it will also contain the tooltip
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
						const skin_desc_t *s=skinverwaltung_t::get_extra(str,i-1);
						icon = s ? s->get_image_id(0) : IMG_EMPTY;
					}
					else {
						if(  icon>=skinverwaltung_t::tool_icons_toolbars->get_count()  ) {
							dbg->warning( "tool_t::read_menu()", "wrong icon (%i) given for toolbar_tool[%i][%i]", icon, i, j );
							icon = 0;
						}
						icon = skinverwaltung_t::tool_icons_toolbars->get_image_id(icon);
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
			bool create_tool = icon!=IMG_EMPTY  ||  key_str  ||  param_str;

			if (char const* const c = strstart(toolname, "general_tool[")) {
				uint8 toolnr = atoi(c);
				if(  toolnr<GENERAL_TOOL_COUNT  ) {
					if(create_tool) {
						// compatibility mode: tool_cityroad is used for tool_wegebau with defaultparam 'cityroad'
						if(  toolnr==TOOL_BUILD_WAY  &&  param_str  &&  strcmp(param_str,"city_road")==0) {
							toolnr = TOOL_BUILD_CITYROAD;
							dbg->warning("tool_t::read_menu()", "toolbar[%i][%i]: replaced way-builder(id=14) with default param=cityroad by cityroad builder(id=36)", i,j);
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
					dbg->error( "tool_t::read_menu()", "When parsing menuconf.tab: No general tool %i defined (max %i)!", toolnr, GENERAL_TOOL_COUNT );
				}
			}
			else if (char const* const c = strstart(toolname, "simple_tool[")) {
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
					dbg->error( "tool_t::read_menu()", "When parsing menuconf.tab: No simple tool %i defined (max %i)!", toolnr, SIMPLE_TOOL_COUNT );
				}
			}
			else if (char const* const c = strstart(toolname, "dialog_tool[")) {
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
					dbg->error( "tool_t::read_menu()", "When parsing menuconf.tab: No dialog tool %i defined (max %i)!", toolnr, DIALOGE_TOOL_COUNT );
				}
			}
			else if (char const* const c = strstart(toolname, "toolbar[")) {
				uint8 const toolnr = atoi(c);
				if(  toolnr==0  ) {
					if(  strstr( c, "LAST_USED" )  ) {
						toolbar_last_used_t::last_used_tools->icon = icon;
						addtool = toolbar_last_used_t::last_used_tools;
					}
					else {
						dbg->fatal( "Error in menuconf: toolbar cannot call main toolbar", "%s", toolname );
					}
				}
				if(toolbar_tool.get_count()==toolnr) {
					if(param_str==NULL) {
						param_str = "Unnamed toolbar";
						dbg->warning( "tool_t::read_menu()", "Missing title for toolbar[%d]", toolnr);
					}
					char *c = strdup(param_str);
					const char *title = c;
					c += strcspn(c, ",");
					if (*c != '\0') {
						*c++ = '\0';
					}
					toolbar_t* const tb = new toolbar_t(toolbar_tool.get_count() | TOOLBAR_TOOL, title, c);
					toolbar_tool.append(tb);
					addtool = tb;
				}
			}
			else {
				// make a default tool to add the parameter here
				addtool = new tool_dummy_t();
				addtool->default_param = strdup(toolname);
				addtool->command_key = 1;
			}
			if(addtool) {
				if(icon!=IMG_EMPTY) {
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
	toolbar_tool.append( toolbar_last_used_t::last_used_tools );

	// sort characters
	std::sort(char_to_tool.begin(), char_to_tool.end(), compare_tool);
}


void tool_t::update_toolbars()
{
	// renew toolbar
	// iterate twice, to get correct icons if a toolbar changes between empty and non-empty
	for(uint j=0; j<2; j++) {
		bool change = false;
		FOR(vector_tpl<toolbar_t*>, const i, toolbar_tool) {
			bool old_icon_empty = i->get_icon(welt->get_active_player()) == IMG_EMPTY;
			i->update(welt->get_active_player());
			change |= old_icon_empty ^ (i->get_icon(welt->get_active_player()) == IMG_EMPTY);
		}
		if (!change) {
			// no toolbar changes between empty and non-empty, no need to loop again
			break;
		}
	}
}


void tool_t::draw_after(scr_coord pos, bool dirty) const
{
	// default action: grey corner if selected
	image_id id = get_icon( welt->get_active_player() );
	if(  id!=IMG_EMPTY  &&  is_selected()  ) {
		display_img_blend( id, pos.x, pos.y, TRANSPARENT50_FLAG|OUTLINE_FLAG|color_idx_to_rgb(COL_BLACK), false, dirty );
	}
}

bool tool_t::is_selected() const
{
	return welt->get_tool(welt->get_active_player_nr())==this;
}

const char *tool_t::check_pos(player_t *, koord3d pos )
{
	grund_t *gr = welt->lookup(pos);
	return (gr  &&  !gr->is_visible()) ? "" : NULL;
}

bool tool_t::check_valid_pos(koord k ) const
{
	if(is_grid_tool()) {
		return welt->is_within_grid_limits(k);
	}
	return welt->is_within_limits(k);
};

/**
 * Initializes cursor object: image, y-offset, size of marked area,
 * has to be called after init().
 * @param zeiger cursor object
 */
void tool_t::init_cursor( zeiger_t *zeiger) const
{
	zeiger->set_image( cursor );
	zeiger->set_yoff( offset );
	zeiger->set_area( cursor_area, cursor_centered, cursor_offset);
}

const char *kartenboden_tool_t::check_pos(player_t *, koord3d pos )
{
	grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
	return (gr  &&  !gr->is_visible()) ? "" : NULL;
}



image_id toolbar_t::get_icon(player_t *player) const
{
	// no image for edit tools => do not open
	if(  icon==IMG_EMPTY  ||  (player!=NULL  &&  strcmp(default_param,"EDITTOOLS")==0  &&  player->get_player_nr()!=welt->get_public_player()->get_player_nr())  ) {
		return IMG_EMPTY;
	}
	// now have we a least one visible tool?
	if (tool_selector  &&  !tool_selector->empty(player)) {
		return icon;
	}
	return IMG_EMPTY;
}



// simply true, if visible
bool toolbar_t::is_selected() const
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
void toolbar_t::update(player_t *player)
{
	const bool create = (tool_selector == NULL);
	if(create) {
		DBG_MESSAGE("toolbar_t::update()","create toolbar %s",default_param);
		tool_selector = new tool_selector_t( default_param, helpfile, toolbar_tool.index_of(this), this!=tool_t::toolbar_tool[0] );
	}
	else {
		DBG_MESSAGE("toolbar_t::update()","update toolbar %s",default_param);
	}

	tool_selector->reset_tools();
	// now (re)fill it
	FOR(slist_tpl<tool_t*>, const w, tools) {
		// no way to call this tool? => then it is most likely a metatool
		if(w->command_key==1  &&  w->get_icon(player)==IMG_EMPTY) {
			if (char const* const param = w->get_default_param()) {
				if(  create  ) {
					DBG_DEBUG("toolbar_t::update()", "add metatool (param=%s)", param);
				}
				if (char const* c = strstart(param, "ways(")) {
					waytype_t way = (waytype_t)atoi(c);
					while(*c  &&  *c!=','  &&  *c!=')') {
						c++;
					}
					systemtype_t subtype = (systemtype_t)(*c!=0 ? atoi(++c) : 0);
					way_builder_t::fill_menu( tool_selector, way, subtype, get_sound(c));
				}
				else if (char const* const c = strstart(param, "bridges(")) {
					waytype_t const way = (waytype_t)atoi(c);
					bridge_builder_t::fill_menu(tool_selector, way, get_sound(c));
				}
				else if (char const* const c = strstart(param, "tunnels(")) {
					waytype_t const way = (waytype_t)atoi(c);
					tunnel_builder_t::fill_menu(tool_selector, way, get_sound(c));
				}
				else if (char const* const c = strstart(param, "signs(")) {
					waytype_t const way = (waytype_t)atoi(c);
					roadsign_t::fill_menu(tool_selector, way, get_sound(c));
				}
				else if (char const* const c = strstart(param, "wayobjs(")) {
					waytype_t const way = (waytype_t)atoi(c);
					wayobj_t::fill_menu(tool_selector, way, get_sound(c));
				}
				else if (char const* c = strstart(param, "buildings(")) {
					building_desc_t::btype const utype = (building_desc_t::btype)atoi(c);
					while(*c  &&  *c!=','  &&  *c!=')') {
						c++;
					}
					waytype_t way = (waytype_t)(*c!=0 ? atoi(++c) : 0);
					hausbauer_t::fill_menu( tool_selector, utype, way, get_sound(c));
				}
				else if (param[0] == '-') {
					// add dummy tool_t as seperator
					tool_selector->add_tool_selector( dummy );
				}
			}
		}
		else if(w->get_icon(player)!=IMG_EMPTY) {
			// get the right city_road
			if(w->get_id() == (TOOL_BUILD_CITYROAD | GENERAL_TOOL)) {
				w->flags = 0;
				w->init(player);
			}
			if(  create  ) {
				DBG_DEBUG( "toolbar_t::update()", "add tool %i (param=%s)", w->get_id(), w->get_default_param() );
			}
			scenario_t *scen = welt->get_scenario();
			if(  scen->is_scripted()  &&  !scen->is_tool_allowed(player, w->get_id(), w->get_waytype())) {
				continue;
			}
			// now add it to the toolbar gui
			tool_selector->add_tool_selector( w );
		}
	}

	if(  (strcmp(this->default_param,"EDITTOOLS")==0  &&  player!=welt->get_public_player())  ) {
		destroy_win(tool_selector);
		return;
	}
}



// fills and displays a toolbar
bool toolbar_t::init(player_t *player)
{
	update( player );
	bool close = (strcmp(this->default_param,"EDITTOOLS")==0  &&  player!=welt->get_public_player());

	// show/create window
	if(  close  ) {
		destroy_win(tool_selector);
		return false;
	}

	if(  this != tool_t::toolbar_tool[0]  ) {
		// not main menu
		create_win( tool_selector, w_info|w_do_not_delete|w_no_overlap, magic_toolbar+toolbar_tool.index_of(this) );
		DBG_MESSAGE("toolbar_t::init()", "ID=%id", get_id());
	}
	return false;
}


bool toolbar_t::exit(player_t *)
{
	if(  win_get_magic(magic_toolbar+toolbar_tool.index_of(this))  ) {
		destroy_win(tool_selector);
	}
	return false;
}


// from here on last used toolbar tools (for each player!)
void toolbar_last_used_t::update(player_t *sp)
{
	tools.clear();
	if(  sp  ) {
		for(  slist_tpl<tool_t *>::iterator iter = all_tools[sp->get_player_nr()].begin();  iter != all_tools[sp->get_player_nr()].end();  ++iter  ) {
			tools.append( *iter );
		}
	}
	toolbar_t::update( sp );
}


void toolbar_last_used_t::clear()
{
	for(  int i=0;  i < MAX_PLAYER_COUNT;  i++  ) {
		all_tools[i].clear();
	}
	tools.clear();
}


// currently only needed for last used tools
void toolbar_last_used_t::append( tool_t *t, player_t *sp )
{
	static int exclude_from_adding[8]={
		TOOL_SCHEDULE_ADD|GENERAL_TOOL,
		TOOL_SCHEDULE_INS|GENERAL_TOOL,
		TOOL_CHANGE_CONVOI|SIMPLE_TOOL,
		TOOL_CHANGE_LINE|SIMPLE_TOOL,
		TOOL_CHANGE_DEPOT|SIMPLE_TOOL,
		UNUSED_WKZ_PWDHASH_TOOL|SIMPLE_TOOL,
		TOOL_CHANGE_PLAYER|SIMPLE_TOOL,
		TOOL_RENAME|SIMPLE_TOOL
	};

	if(  !sp  ||  t->get_icon(sp)==IMG_EMPTY  ) {
		return;
	}

	// do not add certain tools
	for(  uint i=0;  i<lengthof(exclude_from_adding);  i++  ) {
		if(  t->get_id() == exclude_from_adding[i]  ) {
			return;
		}
	}

	slist_tpl<tool_t *> &players_tools = all_tools[sp->get_player_nr()];

	if(  players_tools.is_contained(t)  ) {
		players_tools.remove( t );
	}
	else {
		while(  players_tools.get_count() >= MAX_LAST_TOOLS  ) {
			players_tools.remove( players_tools.back() );
		}
	}

	players_tools.insert( t );
	// if current => update
	if(  sp == world()->get_active_player()  ) {
		update( sp );
	}
}



bool two_click_tool_t::init(player_t *)
{
	first_click_var = true;
	start = koord3d::invalid;
	if (is_local_execution()) {
		welt->show_distance = koord3d::invalid;
	}
	cleanup( true );
	return true;
}


void two_click_tool_t::rdwr_custom_data(memory_rw_t *packet)
{
	packet->rdwr_bool(first_click_var);
	sint16 posx = start.x; packet->rdwr_short(posx); start.x = posx;
	sint16 posy = start.y; packet->rdwr_short(posy); start.y = posy;
	sint8  posz = start.z; packet->rdwr_byte(posz);  start.z = posz;
}


bool two_click_tool_t::is_first_click() const
{
	return first_click_var;
}


bool two_click_tool_t::is_work_here_network_save(player_t *player, koord3d pos )
{
	if(  !is_first_click()  ) {
		return false;
	}
	const char *error = "";	//default: nosound
	uint8 value = is_valid_pos( player, pos, error, koord3d::invalid );
	DBG_MESSAGE("two_click_tool_t::is_work_here_network_save", "Position %s valid=%d", pos.get_str(), value );
	if(  value == 0  ) {
		// cannot work here at all -> safe
		return true;
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


const char *two_click_tool_t::work(player_t *player, koord3d pos )
{
	if(  !is_first_click()  &&  start_marker  ) {
		start = start_marker->get_pos(); // if map was rotated.
	}

	// remove marker
	cleanup( true );

	const char *error = "";	//default: nosound
	uint8 value = is_valid_pos( player, pos, error, !is_first_click() ? start : koord3d::invalid );
	DBG_MESSAGE("two_click_tool_t::work", "Position %s valid=%d", pos.get_str(), value );
	if(  value == 0  ) {
		flags &= ~(WFL_SHIFT | WFL_CTRL);
		init( player );
		return error;
	}

	if(  is_first_click()  ) {
		// work directly if possible and ctrl is NOT pressed
		if( (value & 1)  &&  !( (value & 2)  &&  is_ctrl_pressed())) {
			// Work here directly.
			DBG_MESSAGE("two_click_tool_t::work", "Call tool at %s", pos.get_str() );
			error = do_work( player, pos, koord3d::invalid );
		}
		else {
			// set starting position.
			DBG_MESSAGE("two_click_tool_t::work", "Setting start to %s", pos.get_str() );
			start_at( pos );
		}
	}
	else {
		if( value & 2 ) {
			DBG_MESSAGE("two_click_tool_t::work", "Setting end to %s", pos.get_str() );
			error = do_work( player, start, pos );
		}
		flags &= ~(WFL_SHIFT | WFL_CTRL);
		init( player ); // Do the cleanup stuff after(!) do_work (otherwise start==koord3d::invalid).
	}
	return error;
}


const char *two_click_tool_t::move(player_t *player, uint16 buttonstate, koord3d pos )
{
	DBG_MESSAGE("two_click_tool_t::move", "Button: %d, Pos: %s", buttonstate, pos.get_str());
	if(  buttonstate == 0  ) {
		return "";
	}

	if(  start == pos  ) {
		init( player );
	}

	const char *error = NULL;

	if(  start == koord3d::invalid  ) {
		// start dragging.
		cleanup( true );

		uint8 value = is_valid_pos( player, pos, error, koord3d::invalid );
		if( error || value == 0 ) {
			return error;
		}
		if( value & 2 ) {
			start_at( pos );
		}
	}
	else {
		// continue dragging.
		cleanup( false );

		if( start_marker ) {
			start = start_marker->get_pos(); // if map was rotated.
		}
		uint8 value = is_valid_pos( player, pos, error, start );
		if( error || value == 0 ) {
			return error;
		}
		if( value & 2 ) {
			display_show_load_pointer( true );
			mark_tiles( player, start, pos );
			display_show_load_pointer( false );
		}
	}
	return "";
}


void two_click_tool_t::start_at( koord3d &new_start )
{
	first_click_var = false;
	start = new_start;
	if (is_local_execution()) {
		welt->show_distance = new_start;
		start_marker = new zeiger_t(start, NULL);
		start_marker->set_image( get_marker_image() );
		grund_t *gr = welt->lookup( start );
		if( gr ) {
			gr->obj_add(start_marker);
		}
	}
	DBG_MESSAGE("two_click_tool_t::start_at", "Setting start to %s", start.get_str());
}


void two_click_tool_t::cleanup( bool delete_start_marker )
{
	// delete marker.
	if(  start_marker!=NULL  &&  delete_start_marker) {
		start_marker->mark_image_dirty( start_marker->get_image(), 0 );
		delete start_marker;
		start_marker = NULL;
	}
	// delete old route.
	while(!marked.empty()) {
		zeiger_t *z = marked.remove_first();
		z->mark_image_dirty( z->get_image(), 0 );
		z->mark_image_dirty( z->get_front_image(), 0 );
		koord3d pos = z->get_pos();
		grund_t *gr = welt->lookup( pos );
		delete z;
		// Remove dummy ground (placed by tool_tunnelbau_t and tool_wegebau_t):
		if(gr  &&   (gr->get_typ() == grund_t::tunnelboden  ||  gr->get_typ() == grund_t::monorailboden)  &&  gr->get_weg_nr(0) == NULL && !gr->get_leitung() ) {
			welt->access(pos.get_2d())->boden_entfernen(gr);
			delete gr;
			assert( !welt->lookup(pos));
		}
	}
	// delete tooltip.
	win_set_static_tooltip( NULL );
}


image_id two_click_tool_t::get_marker_image()
{
	return skinverwaltung_t::bauigelsymbol->get_image_id(0);
}
