/*
 * dialog to enter values ??for map generation
 *
 * Hj. Malthaner
 *
 * April 2000
 *
 * Max Kielland 2013
 * Added theme support
 */

/*
 * Dialog to configure the generation of a new map
 */

#include "welt.h"
#include "karte.h"

#include "../simdebug.h"
#include "../simworld.h"
#include "../gui/simwin.h"
#include "../display/simimg.h"
#include "../simmesg.h"
#include "../simskin.h"
#include "../simtools.h"
#include "../simversion.h"

#include "../bauer/hausbauer.h"
#include "../bauer/wegbauer.h"

#include "../besch/haus_besch.h"

#include "../dataobj/settings.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"

// just for their structure size ...
#include "../boden/wege/schiene.h"
#include "../obj/baum.h"
#include "../simcity.h"
#include "../vehicle/simvehikel.h"
#include "../player/simplay.h"

#include "../simcolor.h"

#include "../display/simgraph.h"

#include "../simsys.h"
#include "../utils/simstring.h"

#include "sprachen.h"
#include "climates.h"
#include "settings_frame.h"
#include "loadsave_frame.h"
#include "load_relief_frame.h"
#include "messagebox.h"
#include "scenario_frame.h"


// Local adjustment
#define L_PREVIEW_SIZE_MIN (16)
#define L_DIALOG_WIDTH     (260)
#define L_EDIT_WIDTH       (edit_Width)
#define L_COLUMN1_X (L_DIALOG_WIDTH - L_EDIT_WIDTH - D_H_SPACE - MAP_PREVIEW_SIZE_X - D_MARGIN_RIGHT)
#define L_COLUMN2_X (L_DIALOG_WIDTH - L_EDIT_WIDTH - D_MARGIN_RIGHT)
#define L_BUTTON_EXTRA_WIDE ( (L_DIALOG_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT - D_V_SPACE) / 2 )
#define L_BUTTON_COLUMN_2   (L_DIALOG_WIDTH - L_BUTTON_EXTRA_WIDE - D_MARGIN_RIGHT)


welt_gui_t::welt_gui_t(settings_t* const sets_par) :
	gui_frame_t( translator::translate("Neue Welt" ) ),
	map(0,0)
{
	// Coordinates are relative to parent (TITLEHEIGHT already subtracted)
	scr_coord cursor(D_MARGIN_LEFT,D_MARGIN_TOP);
	scr_coord_val edit_Width = display_get_char_max_width("0123456789")*5 + D_ARROW_LEFT_WIDTH + D_ARROW_RIGHT_WIDTH;
	scr_coord_val label_width = L_COLUMN1_X - D_MARGIN_LEFT - D_H_SPACE;

	sets = sets_par;
	sets->beginner_mode = env_t::default_settings.get_beginner_mode();

	city_density       = ( sets->get_anzahl_staedte()      ) ? sqrt((double)sets->get_groesse_x()*sets->get_groesse_y()) / sets->get_anzahl_staedte()      : 0.0;
	industry_density   = ( sets->get_factory_count()       ) ? sqrt((double)sets->get_groesse_x()*sets->get_groesse_y()) / sets->get_factory_count()       : 0.0;
	attraction_density = ( sets->get_tourist_attractions() ) ? sqrt((double)sets->get_groesse_x()*sets->get_groesse_y()) / sets->get_tourist_attractions() : 0.0;
	river_density      = ( sets->get_river_number()        ) ? sqrt((double)sets->get_groesse_x()*sets->get_groesse_y()) / sets->get_river_number()        : 0.0;

	// find earliest start and end date ...
	uint16 game_start = 4999;
	uint16 game_ends = 0;

	// first check town halls
	FOR(vector_tpl<haus_besch_t const*>, const besch, *hausbauer_t::get_list(haus_besch_t::rathaus)) {
		uint16 intro_year = (besch->get_intro_year_month()+11)/12;
		if(  intro_year<game_start  ) {
			game_start = intro_year;
		}
		uint16 retire_year = (besch->get_retire_year_month()+11)/12;
		if(  retire_year>game_ends  ) {
			game_ends = retire_year;
		}
	}

	// then streets
	game_start = max( game_start, (wegbauer_t::get_earliest_way(road_wt)->get_intro_year_month()+11)/12 );
	game_ends  = min( game_ends,  (wegbauer_t::get_latest_way(road_wt)->get_retire_year_month()+11)/12 );

	loaded_heightfield = load_heightfield = false;
	load = start = close = scenario = quit = false;
	sets->heightfield = "";

	//******************************************************************
	// Component creation
	// Map preview
	map_preview.init(scr_coord(L_COLUMN1_X+edit_Width+D_H_SPACE, cursor.y));
	add_komponente( &map_preview );

	// World setting label
	world_title_label.init("1WORLD_CHOOSE",cursor);
	world_title_label.set_width(L_COLUMN1_X+edit_Width - cursor.x);
	add_komponente( &world_title_label );
	cursor.y += LINESPACE; // label

	// divider 1
	divider_1.init(cursor,L_DIALOG_WIDTH - D_MARGIN_LEFT -  D_H_SPACE - MAP_PREVIEW_SIZE_X - D_MARGIN_RIGHT);
	add_komponente( &divider_1 );
	cursor.y += D_DIVIDER_HEIGHT;

	// Map number edit
	inp_map_number.init( abs(sets->get_karte_nummer()), 0, 0x7FFFFFFF, 1, true );
	inp_map_number.set_pos(scr_coord(L_COLUMN1_X, cursor.y));
	inp_map_number.set_size(scr_size(edit_Width, D_EDIT_HEIGHT));
	inp_map_number.add_listener( this );
	add_komponente( &inp_map_number );

	// Map number label
	map_number_label.init("2WORLD_CHOOSE",cursor);
	map_number_label.set_width( label_width );
	map_number_label.align_to(&inp_map_number,ALIGN_CENTER_V);
	add_komponente( &map_number_label );
	cursor.y += D_EDIT_HEIGHT;

	// Map size label
	size_label.init("Size (%d MB):",cursor);
	size_label.set_width( label_width );
	add_komponente( &size_label );
	cursor.y += LINESPACE;

	// Map X size edit
	inp_x_size.init( sets->get_groesse_x(), 8, min(32000,min(32000,16777216/sets->get_groesse_y())), sets->get_groesse_x()>=512 ? 128 : 64, false );
	inp_x_size.set_pos( scr_coord(L_COLUMN1_X,cursor.y) );
	inp_x_size.set_size(scr_size(edit_Width, D_EDIT_HEIGHT));
	inp_x_size.add_listener(this);
	add_komponente( &inp_x_size );
	cursor.y += D_EDIT_HEIGHT;

	// Map size Y edit
	inp_y_size.init( sets->get_groesse_y(), 8, min(32000,16777216/sets->get_groesse_x()), sets->get_groesse_y()>=512 ? 128 : 64, false );
	inp_y_size.set_pos(scr_coord(L_COLUMN1_X,cursor.y) );
	inp_y_size.set_size(scr_size(edit_Width, D_EDIT_HEIGHT));
	inp_y_size.add_listener(this);
	add_komponente( &inp_y_size );
	cursor.y += D_EDIT_HEIGHT;
	cursor.y += D_V_SPACE;

	// Random map button
	random_map.init(button_t::roundbox,"Random map",cursor,scr_size(L_BUTTON_EXTRA_WIDE, D_BUTTON_HEIGHT));
	random_map.set_tooltip("chooses a random map");
	random_map.add_listener( this );
	add_komponente( &random_map );

	// Load height map button
	load_map.init(button_t::roundbox,"Lade Relief",scr_coord(L_BUTTON_COLUMN_2, cursor.y),scr_size(L_BUTTON_EXTRA_WIDE, D_BUTTON_HEIGHT));
	load_map.set_tooltip("load height data from file");
	load_map.add_listener( this );
	add_komponente( &load_map );
	cursor.y += D_BUTTON_HEIGHT + D_V_SPACE;

	label_width = L_COLUMN2_X - D_MARGIN_LEFT - D_H_SPACE;

	// Number of towns edit
	inp_number_of_towns.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_number_of_towns.set_size(scr_size(edit_Width, D_EDIT_HEIGHT));
	inp_number_of_towns.add_listener(this);
	inp_number_of_towns.set_limits(0,999);
	inp_number_of_towns.set_value(abs(sets->get_anzahl_staedte()) );
	add_komponente( &inp_number_of_towns );

	// Number of towns label
	cities_label.init("5WORLD_CHOOSE",cursor);
	cities_label.set_width( label_width );
	cities_label.align_to(&inp_number_of_towns,ALIGN_CENTER_V);
	add_komponente( &cities_label );
	cursor.y += D_EDIT_HEIGHT;

	// Town size edit
	inp_town_size.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_town_size.set_size(scr_size(edit_Width, D_EDIT_HEIGHT));
	inp_town_size.add_listener(this);
	inp_town_size.set_limits(0,999999);
	inp_town_size.set_increment_mode(50);
	inp_town_size.set_value( sets->get_mittlere_einwohnerzahl() );
	add_komponente( &inp_town_size );

	// Town size label
	median_label.init("Median Citizen per town",cursor);
	median_label.set_width( label_width );
	median_label.align_to(&inp_town_size,ALIGN_CENTER_V);
	add_komponente( &median_label );
	cursor.y += D_EDIT_HEIGHT;

	// Intercity road length edit
	inp_intercity_road_len.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_intercity_road_len.set_size(scr_size(edit_Width, D_EDIT_HEIGHT));
	inp_intercity_road_len.add_listener(this);
	inp_intercity_road_len.set_limits(0,9999);
	inp_intercity_road_len.set_value( env_t::intercity_road_length );
	inp_intercity_road_len.set_increment_mode( env_t::intercity_road_length>=1000 ? 100 : 20 );
	add_komponente( &inp_intercity_road_len );

	// Intercity road length label
	intercity_label.init("Intercity road len:",cursor);
	intercity_label.set_width( label_width );
	intercity_label.align_to(&inp_intercity_road_len,ALIGN_CENTER_V);
	add_komponente( &intercity_label );
	cursor.y += D_EDIT_HEIGHT;

	// Factories edit
	inp_other_industries.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_other_industries.set_size(scr_size(edit_Width, D_EDIT_HEIGHT));
	inp_other_industries.add_listener(this);
	inp_other_industries.set_limits(0,999);
	inp_other_industries.set_value(abs(sets->get_factory_count()) );
	add_komponente( &inp_other_industries );

	// Factories label
	factories_label.init("No. of Factories",cursor);
	factories_label.set_width( label_width );
	factories_label.align_to(&inp_other_industries,ALIGN_CENTER_V);
	add_komponente( &factories_label );
	cursor.y += D_EDIT_HEIGHT;

	// Tourist attr. edit
	inp_tourist_attractions.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_tourist_attractions.set_size(scr_size(edit_Width, D_EDIT_HEIGHT));
	inp_tourist_attractions.add_listener(this);
	inp_tourist_attractions.set_limits(0,999);
	inp_tourist_attractions.set_value(abs(sets->get_tourist_attractions()) );
	add_komponente( &inp_tourist_attractions );

	// Tourist attr. label
	tourist_label.init("Tourist attractions",cursor);
	tourist_label.set_width( label_width );
	tourist_label.align_to(&inp_tourist_attractions,ALIGN_CENTER_V);
	add_komponente( &tourist_label );
	cursor.y += D_EDIT_HEIGHT;
	cursor.y += D_V_SPACE;

	// Timeline year edit
	inp_intro_date.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_intro_date.set_size(scr_size(edit_Width, D_EDIT_HEIGHT));
	inp_intro_date.add_listener(this);
	inp_intro_date.set_limits(game_start,game_ends);
	inp_intro_date.set_increment_mode(10);
	inp_intro_date.set_value(abs(sets->get_starting_year()) );
	add_komponente( &inp_intro_date );

	// Use timeline checkbox
	use_intro_dates.init(button_t::square_state,"Use timeline start year", cursor);
	use_intro_dates.set_width( label_width );
	use_intro_dates.align_to(&inp_intro_date,ALIGN_CENTER_V);
	use_intro_dates.pressed = sets->get_use_timeline()&1;
	use_intro_dates.add_listener( this );
	add_komponente( &use_intro_dates );
	cursor.y += D_EDIT_HEIGHT;

	// Use beginner mode checkbox
	use_beginner_mode.init(button_t::square_state, "Use beginner mode", cursor);
	use_beginner_mode.set_width( label_width );
	use_beginner_mode.set_tooltip("Higher transport fees, crossconnect all factories");
	use_beginner_mode.pressed = sets->get_beginner_mode();
	use_beginner_mode.add_listener( this );
	add_komponente( &use_beginner_mode );
	cursor.y += D_CHECKBOX_HEIGHT;

	// divider 2
	divider_2.init(cursor,L_DIALOG_WIDTH-D_MARGIN_LEFT-D_MARGIN_RIGHT);
	add_komponente( &divider_2 );
	cursor.y += D_DIVIDER_HEIGHT;

	// Map settings button
	open_setting_gui.init(button_t::roundbox, "Setting", cursor, scr_size(L_BUTTON_EXTRA_WIDE, D_BUTTON_HEIGHT));
	open_setting_gui.pressed = win_get_magic( magic_settings_frame_t );
	open_setting_gui.add_listener( this );
	add_komponente( &open_setting_gui );

	// Landscape settings button
	open_climate_gui.init(button_t::roundbox,"Climate Control", scr_coord(L_BUTTON_COLUMN_2,cursor.y), scr_size(L_BUTTON_EXTRA_WIDE, D_BUTTON_HEIGHT));
	open_climate_gui.pressed = win_get_magic( magic_climate );
	open_climate_gui.add_listener( this );
	add_komponente( &open_climate_gui );
	cursor.y += D_BUTTON_HEIGHT;

	// divider 3
	divider_3.init(cursor,L_DIALOG_WIDTH-D_MARGIN_LEFT-D_MARGIN_RIGHT);
	add_komponente( &divider_3 );
	cursor.y += D_DIVIDER_HEIGHT;

	// load game
	load_game.init(button_t::roundbox, "Load game", cursor, scr_size(L_BUTTON_EXTRA_WIDE, D_BUTTON_HEIGHT));
	load_game.add_listener( this );
	add_komponente( &load_game );

	// load scenario
	load_scenario.init(button_t::roundbox,"Load scenario", scr_coord(L_BUTTON_COLUMN_2,cursor.y), scr_size(L_BUTTON_EXTRA_WIDE, D_BUTTON_HEIGHT));
	load_scenario.add_listener( this );
	add_komponente( &load_scenario );
	cursor.y += D_BUTTON_HEIGHT;
	cursor.y += D_V_SPACE;

	// start game
	start_game.init(button_t::roundbox, "Starte Spiel", cursor, scr_size(L_BUTTON_EXTRA_WIDE, D_BUTTON_HEIGHT));
	start_game.add_listener( this );
	add_komponente( &start_game );

	// quit game
	quit_game.init(button_t::roundbox,"Beenden", scr_coord(L_BUTTON_COLUMN_2,cursor.y), scr_size(L_BUTTON_EXTRA_WIDE, D_BUTTON_HEIGHT));
	quit_game.add_listener( this );
	add_komponente( &quit_game );
	cursor.y += D_BUTTON_HEIGHT;

	set_windowsize( scr_size(L_DIALOG_WIDTH, D_TITLEBAR_HEIGHT + cursor.y + D_MARGIN_BOTTOM) );
	resize(scr_coord(0,0));

	update_preview();
}


/**
 * Calculates preview from height map
 * @param filename name of heightfield file
 * @author Hajo/prissi
 */
bool welt_gui_t::update_from_heightfield(const char *filename)
{
	DBG_MESSAGE("welt_gui_t::update_from_heightfield()", "%s", filename);

	sint16 w, h;
	sint8 *h_field=NULL;
	if(karte_t::get_height_data_from_file(filename, (sint8)sets->get_grundwasser(), h_field, w, h, false )) {
		sets->set_groesse_x(w);
		sets->set_groesse_y(h);
		update_densities();

		inp_x_size.set_value(sets->get_groesse_x());
		inp_y_size.set_value(sets->get_groesse_y());

		resize_preview();

		const int mx = sets->get_groesse_x()/map_size.w;
		const int my = sets->get_groesse_y()/map_size.h;
		for(  int y=0;  y<map_size.h;  y++  ) {
			for(  int x=0;  x<map_size.w;  x++  ) {
				map.at(x,y) = reliefkarte_t::calc_hoehe_farbe( h_field[x*mx+y*my*w], sets->get_grundwasser()-1 );
			}
		}
		map_preview.set_map_data(&map,map_size);
		return true;
	}
	return false;
}


// sets the new values for the number input filed for the densities
void welt_gui_t::update_densities()
{
	if(  city_density!=0.0  ) {
		inp_number_of_towns.set_value( max( 1, (sint32)(0.5+sqrt((double)sets->get_groesse_x()*sets->get_groesse_y())/city_density) ) );
		sets->set_anzahl_staedte( inp_number_of_towns.get_value() );
	}
	if(  industry_density!=0.0  ) {
		inp_other_industries.set_value( max( 1, (sint32)(0.5+sqrt((double)sets->get_groesse_x()*sets->get_groesse_y())/industry_density) ) );
		sets->set_factory_count( inp_other_industries.get_value() );
	}
	if(  attraction_density!=0.0  ) {
		inp_tourist_attractions.set_value( max( 1, (sint32)(0.5+sqrt((double)sets->get_groesse_x()*sets->get_groesse_y())/attraction_density) ) );
		sets->set_tourist_attractions( inp_tourist_attractions.get_value() );
	}
	if(  river_density!=0.0  ) {
		sets->river_number = max( 1, (sint32)(0.5+sqrt((double)sets->get_groesse_x()*sets->get_groesse_y())/river_density) );
		if(  climate_gui_t *climate_gui = (climate_gui_t *)win_get_magic( magic_climate )  ) {
			climate_gui->update_river_number( sets->get_river_number() );
		}
	}
}


/**
 * Calculate the new Map-Preview. Initialize the new RNG!
 * @author Hj. Malthaner
 */
void welt_gui_t::update_preview()
{
	if(  loaded_heightfield  ) {
		update_from_heightfield(sets->heightfield.c_str());
	}
	else {
		resize_preview();

		setsimrand( 0xFFFFFFFF, sets->get_karte_nummer() );

		const int mx = sets->get_groesse_x()/map_size.w;
		const int my = sets->get_groesse_y()/map_size.h;
		for(  int y=0;  y<map_size.h;  y++  ) {
			for(  int x=0;  x<map_size.w;  x++  ) {
				map.at(x,y) = reliefkarte_t::calc_hoehe_farbe(karte_t::perlin_hoehe( sets, koord(x*mx,y*my), koord::invalid ), sets->get_grundwasser());
			}
		}
		sets->heightfield = "";
	}
	map_preview.set_map_data(&map,map_size);
}


void welt_gui_t::resize_preview()
{
	const float world_aspect = (float)sets->get_groesse_x() / (float)sets->get_groesse_y();

	if(  world_aspect > 1.0  ) {
		map_size.w = MAP_PREVIEW_SIZE_X-2;
		map_size.h = (sint16) max( (const int)((float)map_size.w / world_aspect), L_PREVIEW_SIZE_MIN-2);
	}
	else {
		map_size.h = MAP_PREVIEW_SIZE_Y-2;
		map_size.w = (sint16) max( (const int)((float)map_size.h * world_aspect), L_PREVIEW_SIZE_MIN-2);
	}
	map.resize( map_size.w, map_size.h );
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool welt_gui_t::action_triggered( gui_action_creator_t *komp,value_t v)
{
	// check for changed map (update preview for any event)
	int knr = inp_map_number.get_value(); //

	if(komp==&inp_map_number) {
		sets->heightfield = "";
		loaded_heightfield = false;
	}
	else if(komp==&inp_x_size) {
		if(  !loaded_heightfield  ) {
			sets->set_groesse_x( v.i );
			inp_x_size.set_increment_mode( v.i>=64 ? (v.i>=512 ? 128 : 64) : 8 );
			inp_y_size.set_limits( 8, min(32000,16777216/sets->get_groesse_x()) );
			update_densities();
		}
		else {
			inp_x_size.set_value(sets->get_groesse_x()); // can't change size with heightfield loaded
		}
	}
	else if(komp==&inp_y_size) {
		if(  !loaded_heightfield  ) {
			sets->set_groesse_y( v.i );
			inp_y_size.set_increment_mode( v.i>=64 ? (v.i>=512 ? 128 : 64) : 8 );
			inp_x_size.set_limits( 8, min(32000,16777216/sets->get_groesse_y()) );
			update_densities();
		}
		else {
			inp_y_size.set_value(sets->get_groesse_y()); // can't change size with heightfield loaded
		}
	}
	else if(komp==&inp_number_of_towns) {
		sets->set_anzahl_staedte( v.i );
		city_density = sets->get_anzahl_staedte() ? sqrt((double)sets->get_groesse_x()*sets->get_groesse_y()) / sets->get_anzahl_staedte() : 0.0;
	}
	else if(komp==&inp_town_size) {
		sets->set_mittlere_einwohnerzahl( v.i );
	}
	else if(komp==&inp_intercity_road_len) {
		env_t::intercity_road_length = v.i;
		inp_intercity_road_len.set_increment_mode( v.i>=1000 ? 100 : 20 );
	}
	else if(komp==&inp_other_industries) {
		sets->set_factory_count( v.i );
		industry_density = sets->get_factory_count() ? sqrt((double)sets->get_groesse_x()*sets->get_groesse_y()) / sets->get_factory_count() : 0.0;
	}
	else if(komp==&inp_tourist_attractions) {
		sets->set_tourist_attractions( v.i );
		attraction_density = sets->get_tourist_attractions() ? sqrt((double)sets->get_groesse_x()*sets->get_groesse_y()) / sets->get_tourist_attractions() : 0.0;
	}
	else if(komp==&inp_intro_date) {
		sets->set_starting_year( (sint16)(v.i) );
	}
	else if(komp==&random_map) {
		knr = simrand(9999);
		inp_map_number.set_value(knr);
		sets->heightfield = "";
		loaded_heightfield = false;
	}
	else if(komp==&load_map) {
		// load relief
		loaded_heightfield = false;
		sets->heightfield = "";
		sets->grundwasser = -2*env_t::pak_height_conversion_factor;
		create_win(new load_relief_frame_t(sets), w_info, magic_load_t);
		knr = sets->get_karte_nummer();	// otherwise using cancel would not show the normal generated map again
	}
	else if(komp==&use_intro_dates) {
		// 0,1 should force setting to new game as well. don't allow to change
		// 2,3 allow to change
		if(sets->get_use_timeline()&2) {
			// don't change bit1. bit1 affects loading saved game
			sets->set_use_timeline( sets->get_use_timeline()^1 );
			use_intro_dates.pressed = sets->get_use_timeline()&1;
		}
	}
	else if(komp==&use_beginner_mode) {
		sets->beginner_mode = sets->get_beginner_mode()^1;
		use_beginner_mode.pressed = sets->get_beginner_mode();
	}
	else if(komp==&open_setting_gui) {
		gui_frame_t *sg = win_get_magic( magic_settings_frame_t );
		if(  sg  ) {
			destroy_win( sg );
			open_setting_gui.pressed = false;
		}
		else {
			create_win(10, 40, new settings_frame_t(sets), w_info, magic_settings_frame_t );
			open_setting_gui.pressed = true;
		}
	}
	else if(komp==&open_climate_gui) {
		gui_frame_t *climate_gui = win_get_magic( magic_climate );
		if(  climate_gui  ) {
			destroy_win( climate_gui );
			open_climate_gui.pressed = false;
		}
		else {
			climate_gui_t *cg = new climate_gui_t(sets);
			create_win((display_get_width() - cg->get_windowsize().w-10), 40, cg, w_info, magic_climate );
			open_climate_gui.pressed = true;
		}
	}
	else if(komp==&load_game) {
		welt->get_message()->clear();
		create_win( new loadsave_frame_t(true), w_info, magic_load_t);
	}
	else if(komp==&load_scenario) {
		destroy_all_win(true);
		welt->get_message()->clear();
		create_win( new scenario_frame_t(), w_info, magic_load_t );
	}
	else if(komp==&start_game) {
		destroy_all_win(true);
		welt->get_message()->clear();
		create_win(200, 100, new news_img("Erzeuge neue Karte.\n", skinverwaltung_t::neueweltsymbol->get_bild_nr(0)), w_info, magic_none);
		if(loaded_heightfield) {
			welt->load_heightfield(&env_t::default_settings);
		}
		else {
			env_t::default_settings.heightfield = "";
			welt->init( &env_t::default_settings, 0 );
		}
		destroy_all_win(true);
		welt->step_month( env_t::default_settings.get_starting_month() );
		welt->set_pause(false);
		// save setting ...
		loadsave_t file;
		if(file.wr_open("default.sve",loadsave_t::binary,"settings only",SAVEGAME_VER_NR)) {
			// save default setting
			env_t::default_settings.rdwr(&file);
			file.close();
		}
	}
	else if(komp==&quit_game) {
		destroy_all_win(true);
		env_t::quit_simutrans = true;
	}

	if(knr>=0) {
		sets->nummer = knr;
		if(!loaded_heightfield) {
			update_preview();
		}
	}
	return true;
}


bool  welt_gui_t::infowin_event(const event_t *ev)
{
	gui_frame_t::infowin_event(ev);

	if(ev->ev_class==INFOWIN  &&  ev->ev_code==WIN_CLOSE) {
		close = true;
	}

	return true;
}


void welt_gui_t::draw(scr_coord pos, scr_size size)
{
	// Coordinates are relative to parent (TITLEHEIGHT **NOT** subtracted)
	cbuffer_t buf;

	// Update child controls before redraw
	if(!loaded_heightfield  && sets->heightfield.size()!=0) {
		if(update_from_heightfield(sets->heightfield.c_str())) {
			loaded_heightfield = true;
		}
		else {
			loaded_heightfield = false;
			sets->heightfield = "";
		}
	}

	// simulate toggle buttons
	open_setting_gui.pressed = win_get_magic( magic_settings_frame_t );
	open_climate_gui.pressed = false;

	if(  win_get_magic( magic_climate )  ) {
		open_climate_gui.pressed = true;
		// check if number was directly changed
		sint16 new_river_number = max( 1, (sint32)(0.5+sqrt((double)sets->get_groesse_x()*sets->get_groesse_y())/river_density) );
		if(  sets->get_river_number() != new_river_number  ) {
			river_density = sets->get_river_number() ? sqrt((double)sets->get_groesse_x()*sets->get_groesse_y()) / sets->get_river_number() : 0.0;
		}
	}

	use_intro_dates.pressed = sets->get_use_timeline()&1;
	use_beginner_mode.pressed = sets->get_beginner_mode();

	// Calculate map memory
	const uint sx = sets->get_groesse_x();
	const uint sy = sets->get_groesse_y();
	const long memory = (
		sizeof(karte_t) +
		sizeof(spieler_t) * 8 +
		sizeof(convoi_t) * 1000 +
		(sizeof(schiene_t) + sizeof(vehikel_t)) * 10 * (sx + sy) +
		sizeof(stadt_t) * sets->get_anzahl_staedte() +
		(
			sizeof(grund_t) +
			sizeof(planquadrat_t) +
			sizeof(baum_t) * 2 +
			sizeof(void*) * 4
		) * sx * sy
	) / (1024 * 1024);
	buf.printf( translator::translate("Size (%d MB):"), memory );
	size_label.set_text(buf);

	// draw child controls
	gui_frame_t::draw(pos, size);
}
