/*
 * Copyright (c) 2008 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 * New configurable OOP tool system
 */

#include <algorithm>

#include "unicode.h"

#include "simevent.h"
#include "simworld.h"
#include "simwin.h"
#include "player/simplay.h"
#include "simmenu.h"
#include "simwerkz.h"
#include "simskin.h"
#include "simsound.h"

#include "bauer/hausbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/brueckenbauer.h"
#include "bauer/tunnelbauer.h"

#include "besch/haus_besch.h"

#include "boden/grund.h"
#include "boden/wege/strasse.h"

#include "dataobj/translator.h"
#include "dataobj/umgebung.h"
#include "dataobj/tabfile.h"

#include "dings/roadsign.h"
#include "dings/wayobj.h"
#include "dings/zeiger.h"

#include "gui/werkzeug_waehler.h"

#include "utils/simstring.h"


// for key loockup; is always sorted during the game
vector_tpl<werkzeug_t *>werkzeug_t::char_to_tool(0);

// here are the default values, icons, cursor, sound definitions ...
vector_tpl<werkzeug_t *>werkzeug_t::general_tool(GENERAL_TOOL_COUNT);
vector_tpl<werkzeug_t *>werkzeug_t::simple_tool(SIMPLE_TOOL_COUNT);
vector_tpl<werkzeug_t *>werkzeug_t::dialog_tool(DIALOGE_TOOL_COUNT);

// the number of toolbars is not know yet
vector_tpl<toolbar_t *>werkzeug_t::toolbar_tool(0);

char werkzeug_t::toolstr[1024];


werkzeug_t *create_general_tool(int toolnr)
{
	switch(toolnr) {
		case WKZ_ABFRAGE:          return new wkz_abfrage_t();
		case WKZ_REMOVER:          return new wkz_remover_t();
		case WKZ_RAISE_LAND:       return new wkz_raise_t();
		case WKZ_LOWER_LAND:       return new wkz_lower_t();
		case WKZ_SETSLOPE:         return new wkz_setslope_t();
		case WKZ_RESTORESLOPE:     return new wkz_restoreslope_t();
		case WKZ_MARKER:           return new wkz_marker_t();
		case WKZ_CLEAR_RESERVATION:return new wkz_clear_reservation_t();
		case WKZ_TRANSFORMER:      return new wkz_transformer_t();
		case WKZ_ADD_CITY:         return new wkz_add_city_t();
		case WKZ_CHANGE_CITY_SIZE: return new wkz_change_city_size_t();
		case WKZ_PLANT_TREE:       return new wkz_plant_tree_t();
		case WKZ_FAHRPLAN_ADD:     return new wkz_fahrplan_add_t();
		case WKZ_FAHRPLAN_INS:     return new wkz_fahrplan_ins_t();
		case WKZ_WEGEBAU:          return new wkz_wegebau_t();
		case WKZ_BRUECKENBAU:      return new wkz_brueckenbau_t();
		case WKZ_TUNNELBAU:        return new wkz_tunnelbau_t();
		case WKZ_WAYREMOVER:       return new wkz_wayremover_t();
		case WKZ_WAYOBJ:           return new wkz_wayobj_t();
		case WKZ_STATION:          return new wkz_station_t();
		case WKZ_ROADSIGN:         return new wkz_roadsign_t();
		case WKZ_DEPOT:            return new wkz_depot_t();
		case WKZ_BUILD_HAUS:       return new wkz_build_haus_t();
		case WKZ_LAND_CHAIN:       return new wkz_build_industries_land_t();
		case WKZ_CITY_CHAIN:       return new wkz_build_industries_city_t();
		case WKZ_BUILD_FACTORY:    return new wkz_build_factory_t();
		case WKZ_LINK_FACTORY:     return new wkz_link_factory_t();
		case WKZ_HEADQUARTER:      return new wkz_headquarter_t();
		case WKZ_LOCK_GAME:        return new wkz_lock_game_t();
		case WKZ_ADD_CITYCAR:      return new wkz_add_citycar_t();
		case WKZ_FOREST:           return new wkz_forest_t();
		case WKZ_STOP_MOVER:       return new wkz_stop_moving_t();
		case WKZ_MAKE_STOP_PUBLIC: return new wkz_make_stop_public_t();
		case WKZ_REMOVE_WAYOBJ:    return new wkz_wayobj_remover_t();
		case WKZ_SLICED_AND_UNDERGROUND_VIEW: return new wkz_show_underground_t();
		case WKZ_BUY_HOUSE:        return new wkz_buy_house_t();
	}
	dbg->fatal("create_general_tool()","cannot satisfy request for general_tool[%i]!",toolnr);
	return NULL;
}

werkzeug_t *create_simple_tool(int toolnr)
{
	switch(toolnr) {
		case WKZ_PAUSE:             return new wkz_pause_t();
		case WKZ_FASTFORWARD:       return new wkz_fastforward_t();
		case WKZ_SCREENSHOT:        return new wkz_screenshot_t();
		case WKZ_INCREASE_INDUSTRY: return new wkz_increase_industry_t();
		case WKZ_UNDO:              return new wkz_undo_t();
		case WKZ_SWITCH_PLAYER:     return new wkz_switch_player_t();
		case WKZ_STEP_YEAR:         return new wkz_step_year_t();
		case WKZ_CHANGE_GAME_SPEED: return new wkz_change_game_speed_t();
		case WKZ_ZOOM_IN:           return new wkz_zoom_in_t();
		case WKZ_ZOOM_OUT:          return new wkz_zoom_out_t();
		case WKZ_SHOW_COVERAGE:     return new wkz_show_coverage_t();
		case WKZ_SHOW_NAMES:        return new wkz_show_name_t();
		case WKZ_SHOW_GRID:         return new wkz_show_grid_t();
		case WKZ_SHOW_TREES:        return new wkz_show_trees_t();
		case WKZ_SHOW_HOUSES:       return new wkz_show_houses_t();
		case WKZ_SHOW_UNDERGROUND:  return new wkz_show_underground_t();
		case WKZ_ROTATE90:          return new wkz_rotate90_t();
		case WKZ_QUIT:              return new wkz_quit_t();
		case WKZ_FILL_TREES:        return new wkz_fill_trees_t();
		case WKZ_DAYNIGHT_LEVEL:    return new wkz_daynight_level_t();
		case WKZ_VEHICLE_TOOLTIPS:  return new wkz_vehicle_tooltips_t();
	}
	dbg->fatal("create_simple_tool()","cannot satisfy request for simple_tool[%i]!",toolnr);
	return NULL;
}


werkzeug_t *create_dialog_tool(int toolnr)
{
	switch(toolnr) {
		case WKZ_HELP:           return new wkz_help_t();
		case WKZ_OPTIONEN:       return new wkz_optionen_t();
		case WKZ_MINIMAP:        return new wkz_minimap_t();
		case WKZ_LINEOVERVIEW:   return new wkz_lines_t();
		case WKZ_MESSAGES:       return new wkz_messages_t();
		case WKZ_FINANCES:       return new wkz_finances_t();
		case WKZ_PLAYERS:        return new wkz_players_t();
		case WKZ_DISPLAYOPTIONS: return new wkz_displayoptions_t();
		case WKZ_SOUND:          return new wkz_sound_t();
		case WKZ_LANGUAGE:       return new wkz_language_t();
		case WKZ_PLAYERCOLOR:    return new wkz_playercolor_t();
		case WKZ_JUMP:           return new wkz_jump_t();
		case WKZ_LOAD:           return new wkz_load_t();
		case WKZ_SAVE:           return new wkz_save_t();
		case WKZ_LIST_HALT:      return new wkz_list_halt_t();
		case WKZ_LIST_CONVOI:    return new wkz_list_convoi_t();
		case WKZ_LIST_TOWN:      return new wkz_list_town_t();
		case WKZ_LIST_GOODS:     return new wkz_list_goods_t();
		case WKZ_LIST_FACTORY:   return new wkz_list_factory_t();
		case WKZ_LIST_CURIOSITY: return new wkz_list_curiosity_t();
		case WKZ_EDIT_FACTORY:   return new wkz_factorybuilder_t();
		case WKZ_EDIT_ATTRACTION:return new wkz_attractionbuilder_t();
		case WKZ_EDIT_HOUSE:     return new wkz_housebuilder_t();
		case WKZ_EDIT_TREE:      return new wkz_treebuilder_t();
		case WKZ_ENLARGE_MAP:    return new wkz_enlarge_map_t();
		case WKZ_LIST_LABEL:     return new wkz_list_label_t();
		case WKZ_CLIMATES:       return new wkz_climates_t();
	}
	dbg->fatal("create_dialog_tool()","cannot satisfy request for dialog_tool[%i]!",toolnr);
	return NULL;
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
		if(strncmp("COMMA",str,5)==0) {
			return ',';
		}
		// HOME
		if(strncmp("HOME",str,4)==0) {
			return SIM_KEY_HOME;
		}
		// END
		if(strncmp("END",str,3)==0) {
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
		w->id = i | GENERAL_TOOL;
		general_tool.append(w);
	}
	for(  uint16 i=0;  i<SIMPLE_TOOL_COUNT;  i++  ) {
		werkzeug_t *w = create_simple_tool( i );
		w->id = i | SIMPLE_TOOL;
		simple_tool.append(w);
	}
	for(  uint16 i=0;  i<DIALOGE_TOOL_COUNT;  i++  ) {
		werkzeug_t *w = create_dialog_tool( i );
		w->id = i | DIALOGE_TOOL;
		dialog_tool.append(w);
	}
}




// read a tab file to add images, cursors and sound to the tools
void werkzeug_t::read_menu(cstring_t objfilename)
{
	char_to_tool.clear();
	tabfile_t menuconf;
	// only use pak sepcific menues, since otherwise images may missing
	cstring_t user_dir=umgebung_t::user_dir;
	if (!menuconf.open(objfilename+"config/menuconf.tab")) {
		dbg->fatal("werkzeug_t::init_menu()", "Can't read %sconfig/menuconf.tab", (const char *)objfilename );
	}

	tabfileobj_t contents;
	menuconf.read(contents);

	// ok, first init all tools
	DBG_MESSAGE( "werkzeug_t::init_menu()", "Reading general menu" );
	for(  uint16 i=0;  i<GENERAL_TOOL_COUNT;  i++  ) {
		char id[256];
		sprintf( id, "general_tool[%i]", i );
		const char *str = contents.get( id );
		/* str should now contain something like 1,2,-1
		 * first parameter is the image number in "GeneralTools"
		 * next is the cursor in "GeneralTools"
		 * final is the sound
		 * -1 will disable any of them
		 */
		werkzeug_t *w = general_tool[i];
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
				if(  icon>=skinverwaltung_t::werkzeuge_general->get_bild_anzahl()  ) {
					dbg->fatal( "werkzeug_t::init_menu()", "wrong icon (%i) given for general_tool[%i]", icon, i );
				}
				w->icon = skinverwaltung_t::werkzeuge_general->get_bild_nr(icon);
			}
			do {
				*str++;
			} while(*str  &&  *str!=',');
		}
		if(*str==',') {
			// next comes cursor
			str++;
			if(*str!=',') {
				uint16 cursor = (uint16)atoi(str);
				if(  cursor>=skinverwaltung_t::werkzeuge_general->get_bild_anzahl()  ) {
					dbg->fatal( "werkzeug_t::init_menu()", "wrong cursor (%i) given for general_tool[%i]", cursor, i );
				}
				w->cursor = skinverwaltung_t::cursor_general->get_bild_nr(cursor);
				do
					*str++;
				while(*str  &&  *str!=',');
			}
		}
		if(*str==',') {
			// ok_sound
			str++;
			if(*str!=',') {
				int sound = atoi(str);
				if(  sound>0  ) {
					w->ok_sound = sound_besch_t::get_compatible_sound_id(sound);
				}
				do
					*str++;
				while(*str  &&  *str!=',');
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

	// now the simple tools
	DBG_MESSAGE( "werkzeug_t::init_menu()", "Reading simple menu" );
	for(  uint16 i=0;  i<SIMPLE_TOOL_COUNT;  i++  ) {
		char id[256];
		sprintf( id, "simple_tool[%i]", i );
		const char *str = contents.get( id );
		werkzeug_t *w = simple_tool[i];
		/* str should now contain something like 1,2,-1
		 * first parameter is the image number in "GeneralTools"
		 * next is the cursor in "GeneralTools"
		 * final is the sound
		 * -1 will disable any of them
		 */
		if(*str  &&  *str!=',') {
			// ok, first come icon
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
				if(  icon>=skinverwaltung_t::werkzeuge_simple->get_bild_anzahl()  ) {
					dbg->fatal( "werkzeug_t::init_menu()", "wrong icon (%i) given for simple_tool[%i]", icon, i );
				}
				w->icon = skinverwaltung_t::werkzeuge_simple->get_bild_nr(icon);
			}
			do {
				*str++;
			} while(*str  &&  *str!=',');
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

	// now the dialoge tools
	DBG_MESSAGE( "werkzeug_t::init_menu()", "Reading dialoge menu" );
	for(  uint16 i=0;  i<DIALOGE_TOOL_COUNT;  i++  ) {
		char id[256];
		sprintf( id, "dialog_tool[%i]", i );
		const char *str = contents.get( id );
		werkzeug_t *w = dialog_tool[i];
		/* str should now contain something like 1,2,-1
		 * first parameter is the image number in "GeneralTools"
		 * next is the cursor in "GeneralTools"
		 * final is the sound
		 * -1 will disable any of them
		 */
		if(*str  &&  *str!=',') {
			// ok, first come icon
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
				if(  icon>=skinverwaltung_t::werkzeuge_dialoge->get_bild_anzahl()  ) {
					dbg->fatal( "werkzeug_t::init_menu()", "wrong icon (%i) given for dialoge_tool[%i]", icon, i );
				}
				w->icon = skinverwaltung_t::werkzeuge_dialoge->get_bild_nr(icon);
			}
			do {
				*str++;
			} while(*str  &&  *str!=',');
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

	// now the toolbar tools
	DBG_MESSAGE( "werkzeug_t::read_menu()", "Reading toolbars" );
	// default size
	koord size( contents.get_int("icon_width",32), contents.get_int("icon_height",32) );
	// first: add main menu
	toolbar_tool.resize( skinverwaltung_t::werkzeuge_toolbars->get_bild_anzahl() );
	toolbar_tool.append(new toolbar_t("", "", size));
	toolbar_tool[0]->id = TOOLBAR_TOOL;
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
			/* str should now contain something like 1,2,-1
			 * first parameter is the image number in "GeneralTools"
			 * next is the cursor in "GeneralTools"
			 * final is the sound
			 * -1 will disable any of them
			 */
			werkzeug_t *addtool = NULL;

			/* first, parse the string; we could have up to four parameters */
			const char *toolname = str;
			image_id icon = IMG_LEER;
			const char *key_str = NULL;
			const char *param_str = NULL;	// in case of toolbars, it will also contain the tooltip

			while(*str!=']'  &&  *str) {
				str ++;
			}
			while(*str==']'  ||  *str==' ') {
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
							dbg->fatal( "werkzeug_t::read_menu()", "wrong icon (%i) given for toolbar_tool[%i][%i]", icon, i, j );
						}
						icon = skinverwaltung_t::werkzeuge_toolbars->get_bild_nr(icon);
					}
					while(*str  &&  *str!=',') {
						*str++;
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

			if(strstr(toolname,"general_tool[")) {
				uint8 toolnr = atoi(toolname+13);
				if(  toolnr<GENERAL_TOOL_COUNT  ) {
					if(icon!=IMG_LEER  ||  key_str  ||  param_str) {
						addtool = create_general_tool( toolnr );
						// copy defaults
						*addtool = *(general_tool[toolnr]);
						// add specials
						if(icon!=IMG_LEER) {
							addtool->icon = icon;
						}
						if(key_str!=NULL) {
							addtool->command_key = str_to_key(key_str);
							char_to_tool.append(addtool);
						}
						if(param_str!=NULL) {
							addtool->default_param = strdup(param_str);
						}
						addtool->id = general_tool.get_count() | GENERAL_TOOL;
						general_tool.append( addtool );
					}
					else {
						addtool = general_tool[toolnr];
					}
				}
				else {
					dbg->error( "werkzeug_t::read_menu()", "When parsing menuconf.tab: No general tool %i defined (max %i)!", toolnr, GENERAL_TOOL_COUNT );
				}
			}
			else if(strstr(toolname,"simple_tool[")) {
				uint8 toolnr = atoi(toolname+12);
				if(  toolnr<SIMPLE_TOOL_COUNT  ) {
					if(icon!=IMG_LEER  ||  key_str  ||  param_str) {
						addtool = create_simple_tool( toolnr );
						*addtool = *(simple_tool[toolnr]);
						if(icon!=IMG_LEER) {
							addtool->icon = icon;
						}
						if(key_str!=NULL) {
							addtool->command_key = str_to_key(key_str);
							char_to_tool.append(addtool);
						}
						if(param_str!=NULL) {
							addtool->default_param = strdup(param_str);
						}
						addtool->id = simple_tool.get_count() | SIMPLE_TOOL;
						simple_tool.append( addtool );
					}
					else {
						addtool = simple_tool[toolnr];
					}
				}
				else {
					dbg->error( "werkzeug_t::read_menu()", "When parsing menuconf.tab: No simple tool %i defined (max %i)!", toolnr, SIMPLE_TOOL_COUNT );
				}
			}
			else if(strstr(toolname,"dialog_tool[")) {
				uint8 toolnr = atoi(toolname+12);
				if(  toolnr<DIALOGE_TOOL_COUNT  ) {
					if(icon!=IMG_LEER  ||  key_str  ||  param_str) {
						addtool = create_dialog_tool( toolnr );;
						*addtool = *(dialog_tool[toolnr]);
						if(icon!=IMG_LEER) {
							addtool->icon = icon;
						}
						if(key_str!=NULL) {
							addtool->command_key = str_to_key(key_str);
							char_to_tool.append(addtool);
						}
						if(param_str!=NULL) {
							addtool->default_param = strdup(param_str);
						}
						addtool->id = dialog_tool.get_count() | DIALOGE_TOOL;
						dialog_tool.append( addtool );
					}
					else {
						addtool = dialog_tool[toolnr];
					}
				}
				else {
					dbg->error( "werkzeug_t::read_menu()", "When parsing menuconf.tab: No dialog tool %i defined (max %i)!", toolnr, DIALOGE_TOOL_COUNT );
				}
			}
			else if(strstr(toolname,"toolbar[")) {
				uint8 toolnr = atoi(toolname+8);
				assert(toolnr>0);
				if(toolbar_tool.get_count()==toolnr) {
					if(param_str==NULL) {
						dbg->fatal( "werkzeug_t::read_menu()", "Missing parameter for toolbar" );
					}
					char *c = strdup(param_str);
					const char *title = c;
					while(*c  &&  *c++!=',') {
					}
					c[-1] = 0;
					toolbar_t *tb = new toolbar_t( title, c, size );
					if(icon!=IMG_LEER) {
						tb->icon = icon;
					}
					if(key_str!=NULL) {
						tb->command_key = str_to_key(key_str);
						char_to_tool.append(tb);
					}
					tb->id = toolbar_tool.get_count() | TOOLBAR_TOOL;
					toolbar_tool.append(tb);
					addtool = tb;
				}
			}
			else {
				// make a default tool to add the parameter here
				addtool = new werkzeug_t();
				addtool->default_param = strdup(toolname);
				addtool->command_key = 1;
			}
			if(addtool) {
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
	for (vector_tpl<toolbar_t *>::const_iterator i = toolbar_tool.begin(), end = toolbar_tool.end();  i != end;  ++i  ) {
		(*i)->update(welt, welt->get_active_player());
	}
}


void werkzeug_t::draw_after( karte_t *welt, koord pos ) const
{
	// default action: grey corner if selected
	image_id id = get_icon( welt->get_active_player() );
	if(  id!=IMG_LEER  &&  is_selected(welt)  ) {
		display_img_blend( id, pos.x, pos.y, TRANSPARENT50_FLAG|OUTLINE_FLAG|COL_BLACK, false, true );
	}
}

// seperator in toolbars
class wkz_dummy_t : public werkzeug_t {
	bool init( karte_t *, spieler_t * ) { return false; }
};

werkzeug_t *werkzeug_t::dummy = new wkz_dummy_t();



image_id toolbar_t::get_icon(spieler_t *sp) const
{
	// no image for edit tools => do not open
	if(sp!=NULL  &&  strcmp(default_param,"EDITTOOLS")==0  &&  sp!=sp->get_welt()->get_spieler(1)  ) {
		return IMG_LEER;
	}
	return icon;
}



// simply true, if visible
bool toolbar_t::is_selected(karte_t *) const
{
	return win_get_magic((long)this);
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
	if(wzw==NULL) {
		DBG_MESSAGE("toolbar_t::update()","update/create toolbar %s",default_param);
		wzw = new werkzeug_waehler_t( welt, default_param, helpfile, iconsize, this!=werkzeug_t::toolbar_tool[0] );
	}

	if(  (strcmp(this->default_param,"EDITTOOLS")==0  &&  sp!=welt->get_spieler(1))  ) {
		destroy_win(wzw);
		return;
	}

	wzw->reset_tools();
	// now (re)fill it
	for (slist_tpl<werkzeug_t *>::const_iterator iter = tools.begin(), end = tools.end(); iter != end; ++iter) {
		werkzeug_t *w = *iter;
		// no way to call this tool? => then it is most likely a metatool
		if(w->command_key==1  &&  w->get_icon(welt->get_active_player())==IMG_LEER) {
			if(w->default_param!=NULL) {
				if(strstr(w->default_param,"ways(")) {
					const char *c = w->default_param+5;
					waytype_t way = (waytype_t)atoi(c);
					while(*c  &&  *c!=','  &&  *c!=')') {
						c++;
					}
					weg_t::system_type subtype = (weg_t::system_type)(*c!=0 ? atoi(++c) : 0);
					wegbauer_t::fill_menu( wzw, way, subtype, get_sound(c), welt );
				}
				else if(strstr(w->default_param,"bridges(")) {
					waytype_t way = (waytype_t)atoi(w->default_param+8);
					brueckenbauer_t::fill_menu( wzw, way, get_sound(w->default_param+5), welt );
				}
				else if(strstr(w->default_param,"tunnels(")) {
					waytype_t way = (waytype_t)atoi(w->default_param+8);
					tunnelbauer_t::fill_menu( wzw, way, get_sound(w->default_param+8), welt );
				}
				else if(strstr(w->default_param,"signs(")) {
					waytype_t way = (waytype_t)atoi(w->default_param+6);
					roadsign_t::fill_menu( wzw, way, get_sound(w->default_param+6), welt );
				}
				else if(strstr(w->default_param,"wayobjs(")) {
					waytype_t way = (waytype_t)atoi(w->default_param+8);
					wayobj_t::fill_menu( wzw, way, get_sound(w->default_param+8), welt );
				}
				else if(strstr(w->default_param,"buildings(")) {
					const char *c = w->default_param+10;
					haus_besch_t::utyp utype = (haus_besch_t::utyp)atoi(w->default_param+10);
					while(*c  &&  *c!=','  &&  *c!=')') {
						c++;
					}
					waytype_t way = (waytype_t)(*c!=0 ? atoi(++c) : 0);
					hausbauer_t::fill_menu( wzw, utype, way, get_sound(c), welt );
				}
				else if(w->default_param[0]=='-') {
					// add dummy werkzeug as seperator
					wzw->add_werkzeug( dummy );
				}
			}
		}
		else if(w->get_icon(welt->get_active_player())!=IMG_LEER) {
			wzw->add_werkzeug( w );
		}
	}
}



// fills and displays a toolbar
bool toolbar_t::init(karte_t *welt, spieler_t *sp)
{
	update( welt, sp );
	bool close = (strcmp(this->default_param,"EDITTOOLS")==0  &&  sp!=welt->get_spieler(1));

	// show/create window
	if(win_get_magic((long)this)) {
		if(close) {
			destroy_win(wzw);
		}
		else {
			top_win(wzw);
		}

	}
	else if(!close  &&  this!=werkzeug_t::toolbar_tool[0]) {
		// not open and not main menu
		create_win( wzw, w_info|w_do_not_delete|w_no_overlap, (long)this );
		DBG_MESSAGE("toolbar_t::init()", "ID=%id", id);
	}
	return false;
}



bool two_click_werkzeug_t::init( karte_t *welt, spieler_t *sp )
{
	first_click[sp->get_player_nr()] = true;
	welt->show_distance = start[sp->get_player_nr()] = koord3d::invalid;
	cleanup( sp, true );
	return true;
}


const char *two_click_werkzeug_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	if(  !first_click[sp->get_player_nr()]  &&  start_marker[sp->get_player_nr()]  ) {
		start[sp->get_player_nr()]= start_marker[sp->get_player_nr()]->get_pos(); // if map was rotated.
	}

	// remove marker
	cleanup( sp, true );

	const char *error = "";	//default: nosound
	uint8 value = is_valid_pos( welt, sp, pos, error );
	if(  value == 0  ) {
		init( welt, sp );
		return error;
	}

	if(  first_click[sp->get_player_nr()]  ) {
		if( value & 1 ) {
			// Work here directly.
			DBG_MESSAGE("two_click_werkzeug_t::work", "Call tool at %s", pos.get_str() );
			error = do_work( welt, sp, pos, koord3d::invalid );
		}
		else {
			// set starting position.
			start_at( welt, sp, pos );
		}
	}
	else {
		if( value & 2 ) {
			DBG_MESSAGE("two_click_werkzeug_t::work", "Setting end to %s", pos.get_str() );
			error = do_work( welt, sp, start[sp->get_player_nr()], pos );
		}
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

	const uint8 sp_nr = sp->get_player_nr();
	if(  start[sp_nr] == pos  ) {
		init( welt, sp );
	}

	const char *error = "";
	uint8 value = is_valid_pos( welt, sp, pos, error );
	if( error || value == 0 ) {
		return error;
	}

	if(  start[sp_nr] == koord3d::invalid  ) {
		// start dragging.
		cleanup( sp, true );
		if(  value == 0  ) {
			return error;
		}
		if( value & 2 ) {
			start_at( welt, sp, pos );
		}
	}
	else {
		// continue dragging.
		cleanup( sp, false );

		if( start_marker[sp_nr] ) {
			start[sp_nr] = start_marker[sp_nr]->get_pos(); // if map was rotated.
		}

		if( value & 2 ) {
			display_show_load_pointer( true );
			mark_tiles( welt, sp, start[sp_nr], pos );
			display_show_load_pointer( false );
		}
	}
	return "";
}


void two_click_werkzeug_t::start_at( karte_t *welt, spieler_t* sp, koord3d &new_start )
{
	const uint8 sp_nr = sp->get_player_nr();
	first_click[sp_nr] = false;
	welt->show_distance = start[sp_nr] = new_start;
	start_marker[sp_nr] = new zeiger_t(welt, start[sp_nr], NULL);
	start_marker[sp_nr]->set_bild( get_marker_image() );
	grund_t *gr = welt->lookup( start[sp_nr] );
	if( gr ) {
		gr->obj_add(start_marker[sp_nr]);
	}
	DBG_MESSAGE("two_click_werkzeug_t::start_at", "Setting start to %s", start[sp_nr].get_str());
}


void two_click_werkzeug_t::cleanup( spieler_t *sp, bool delete_start_marker )
{
	const uint8 sp_nr = sp->get_player_nr();
	karte_t *welt = spieler_t::get_welt();
	// delete marker.
	if(  start_marker[sp_nr]!=NULL  &&  delete_start_marker) {
		start_marker[sp_nr]->mark_image_dirty( start_marker[sp_nr]->get_bild(), 0 );
		delete start_marker[sp_nr];
		start_marker[sp_nr] = NULL;
	}
	// delete old route.
	while(!marked[sp_nr].empty()) {
		zeiger_t *z = marked[sp_nr].remove_first();
		z->mark_image_dirty( z->get_bild(), 0 );
		z->mark_image_dirty( z->get_after_bild(), 0 );
		koord3d pos = z->get_pos();
		grund_t *gr = welt->lookup( pos );
		delete z;
		// Remove dummy ground (placed by wkz_tunnelbau_t and wkz_wegebau_t):
		if( (gr->get_typ() == grund_t::tunnelboden  ||  gr->get_typ() == grund_t::monorailboden)  &&  gr->get_weg_nr(0) == NULL ) {
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

