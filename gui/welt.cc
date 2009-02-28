/*
 * dialog zur Eingabe der Werte fuer die Kartenerzeugung
 *
 * Hj. Malthaner
 *
 * April 2000
 */

#include <string.h>

#include "welt.h"
#include "karte.h"

#include "../simdebug.h"
#include "../simio.h"
#include "../simworld.h"
#include "../simwin.h"
#include "../simimg.h"
#include "../simtools.h"

#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"

// just for their structure size ...
#include "../boden/wege/schiene.h"
#include "../dings/baum.h"
#include "../simcity.h"
#include "../vehicle/simvehikel.h"

#include "../simcolor.h"

#include "../simgraph.h"

#include "load_relief_frame.h"

#include "../utils/simstring.h"
#include "components/list_button.h"

#define START_HEIGHT (28)

#define LEFT_ARROW (110)
#define RIGHT_ARROW (160)
#define TEXT_RIGHT (145) // ten are offset in routine ..

#define LEFT_WIDE_ARROW (185)
#define RIGHT_WIDE_ARROW (235)
#define TEXT_WIDE_RIGHT (220)

#define RIGHT_COLUMN (185)
#define RIGHT_COLUMN_WIDTH (60)

#include <sys/stat.h>
#include <time.h>

welt_gui_t::welt_gui_t(karte_t *welt, einstellungen_t *sets) : gui_frame_t("Neue Welt")
{
DBG_MESSAGE("","sizeof(stat)=%d, sizeof(tm)=%d",sizeof(struct stat),sizeof(struct tm) );
	this->welt = welt;
	this->sets = sets;
	this->old_lang = -1;
	this->sets->set_beginner_mode(umgebung_t::default_einstellungen.get_beginner_mode());
	loaded_heightfield = load_heightfield = false;
	load = start = close = scenario = quit = false;
	int intTopOfButton=START_HEIGHT;
	sets->heightfield = "";

	// select map stuff ..
	inp_map_number.set_pos(koord(LEFT_ARROW, intTopOfButton));
	inp_map_number.set_groesse(koord(RIGHT_ARROW-LEFT_ARROW+10, 12));
	inp_map_number.set_limits(0,0x7FFFFFFF);
	inp_map_number.set_value(abs(sets->get_karte_nummer())%9999);
	inp_map_number.add_listener( this );
	add_komponente( &inp_map_number );

	intTopOfButton += 12;
	intTopOfButton += 12;

	inp_x_size.set_pos(koord(LEFT_ARROW,intTopOfButton) );
	inp_x_size.set_groesse(koord(RIGHT_ARROW-LEFT_ARROW+10, 12));
	inp_x_size.add_listener(this);
	inp_x_size.set_value( sets->get_groesse_x() );
	inp_x_size.set_limits( 64, min(32766,4194304/sets->get_groesse_y()) );
	inp_x_size.set_increment_mode( sets->get_groesse_x()>=512 ? 128 : 64 );
	inp_x_size.wrap_mode( false );
	add_komponente( &inp_x_size );
	intTopOfButton += 12;

	inp_y_size.set_pos(koord(LEFT_ARROW,intTopOfButton) );
	inp_y_size.set_groesse(koord(RIGHT_ARROW-LEFT_ARROW+10, 12));
	inp_y_size.add_listener(this);
	inp_y_size.set_limits( 64, min(32766,4194304/sets->get_groesse_x()) );
	inp_y_size.set_value( sets->get_groesse_y() );
	inp_y_size.set_increment_mode( sets->get_groesse_y()>=512 ? 128 : 64 );
	inp_y_size.wrap_mode( false );
	add_komponente( &inp_y_size );
	intTopOfButton += 12;

	// maps etc.
	intTopOfButton += 5;
	random_map.set_pos( koord(11, intTopOfButton) );
	random_map.set_groesse( koord(104, BUTTON_HEIGHT) );
	random_map.set_typ(button_t::roundbox);
	random_map.add_listener( this );
	add_komponente( &random_map );
	load_map.set_pos( koord(104+11+30, intTopOfButton) );
	load_map.set_groesse( koord(104, BUTTON_HEIGHT) );
	load_map.set_typ(button_t::roundbox);
	load_map.add_listener( this );
	add_komponente( &load_map );
	intTopOfButton += BUTTON_HEIGHT;

	// city stuff
	intTopOfButton += 5;
	inp_number_of_towns.set_pos(koord(RIGHT_COLUMN,intTopOfButton) );
	inp_number_of_towns.set_groesse(koord(RIGHT_COLUMN_WIDTH, 12));
	inp_number_of_towns.add_listener(this);
	inp_number_of_towns.set_limits(0,999);
	inp_number_of_towns.set_value(abs(sets->get_anzahl_staedte()) );
	add_komponente( &inp_number_of_towns );
	intTopOfButton += 12;

	inp_town_size.set_pos(koord(RIGHT_COLUMN,intTopOfButton) );
	inp_town_size.set_groesse(koord(RIGHT_COLUMN_WIDTH, 12));
	inp_town_size.add_listener(this);
	inp_town_size.set_limits(0,999999);
	inp_town_size.set_increment_mode(50);
	inp_town_size.set_value( sets->get_mittlere_einwohnerzahl() );
	add_komponente( &inp_town_size );
	intTopOfButton += 12;


	inp_intercity_road_len.set_pos(koord(RIGHT_COLUMN,intTopOfButton) );
	inp_intercity_road_len.set_groesse(koord(RIGHT_COLUMN_WIDTH, 12));
	inp_intercity_road_len.add_listener(this);
	inp_intercity_road_len.set_limits(0,9999);
	inp_intercity_road_len.set_value( umgebung_t::intercity_road_length );
	inp_intercity_road_len.set_increment_mode( umgebung_t::intercity_road_length>=1000 ? 100 : 20 );
	add_komponente( &inp_intercity_road_len );
	intTopOfButton += 12;

	inp_traffic_density.set_pos(koord(RIGHT_COLUMN,intTopOfButton) );
	inp_traffic_density.set_groesse(koord(RIGHT_COLUMN_WIDTH, 12));
	inp_traffic_density.add_listener(this);
	inp_traffic_density.set_limits(0,16);
	inp_traffic_density.set_value(abs(sets->get_verkehr_level()) );
	add_komponente( &inp_traffic_density );
	intTopOfButton += 12;

	// industry stuff
	intTopOfButton += 5;
	inp_other_industries.set_pos(koord(RIGHT_COLUMN,intTopOfButton) );
	inp_other_industries.set_groesse(koord(RIGHT_COLUMN_WIDTH, 12));
	inp_other_industries.add_listener(this);
	inp_other_industries.set_limits(0,999);
	inp_other_industries.set_value(abs(sets->get_land_industry_chains()) );
	add_komponente( &inp_other_industries );
	intTopOfButton += 12;

	inp_electric_producer.set_pos(koord(RIGHT_COLUMN,intTopOfButton) );
	inp_electric_producer.set_groesse(koord(RIGHT_COLUMN_WIDTH, 12));
	inp_electric_producer.add_listener(this);
	inp_electric_producer.set_limits(0,100);
	inp_electric_producer.set_value(abs(sets->get_electric_promille()/10) );
	add_komponente( &inp_electric_producer );
	intTopOfButton += 12;

	inp_tourist_attractions.set_pos(koord(RIGHT_COLUMN,intTopOfButton) );
	inp_tourist_attractions.set_groesse(koord(RIGHT_COLUMN_WIDTH, 12));
	inp_tourist_attractions.add_listener(this);
	inp_tourist_attractions.set_limits(0,999);
	inp_tourist_attractions.set_value(abs(sets->get_tourist_attractions()) );
	add_komponente( &inp_tourist_attractions );
	intTopOfButton += 12;

	// other settings
	intTopOfButton += 5;
	use_intro_dates.set_pos( koord(10,intTopOfButton) );
	use_intro_dates.set_typ( button_t::square_state );
	use_intro_dates.pressed = sets->get_use_timeline()&1;
	use_intro_dates.add_listener( this );
	add_komponente( &use_intro_dates );

	inp_intro_date.set_pos(koord(RIGHT_COLUMN,intTopOfButton) );
	inp_intro_date.set_groesse(koord(RIGHT_COLUMN_WIDTH, 12));
	inp_intro_date.add_listener(this);
	inp_intro_date.set_limits(1400,2050);
	inp_intro_date.set_increment_mode(10);
	inp_intro_date.set_value(abs(sets->get_starting_year()) );
	add_komponente( &inp_intro_date );
	intTopOfButton += 12;

	intTopOfButton += 5;
	allow_player_change.set_pos( koord(10,intTopOfButton) );
	allow_player_change.set_typ( button_t::square_state );
	allow_player_change.add_listener( this );
	allow_player_change.pressed = sets->get_allow_player_change();
	add_komponente( &allow_player_change );
	intTopOfButton += 12;

	intTopOfButton += 5;
	use_beginner_mode.set_pos( koord(10,intTopOfButton) );
	use_beginner_mode.set_typ( button_t::square_state );
	use_beginner_mode.add_listener( this );
	use_beginner_mode.pressed = sets->get_beginner_mode();
	add_komponente( &use_beginner_mode );
	intTopOfButton += 12;

	// load game
	intTopOfButton += 10;
	load_game.set_pos( koord(10, intTopOfButton) );
	load_game.set_groesse( koord(104, 14) );
	load_game.set_typ(button_t::roundbox);
	load_game.add_listener( this );
	add_komponente( &load_game );

	// load scenario
	load_scenario.set_pos( koord(104+11+30, intTopOfButton) );
	load_scenario.set_groesse( koord(104, 14) );
	load_scenario.set_typ(button_t::roundbox);
	load_scenario.add_listener( this );
	add_komponente( &load_scenario );

	// start game
	intTopOfButton += 5+BUTTON_HEIGHT;
	start_game.set_pos( koord(10, intTopOfButton) );
	start_game.set_groesse( koord(104, 14) );
	start_game.set_typ(button_t::roundbox);
	start_game.add_listener( this );
	add_komponente( &start_game );

	// quit game
	quit_game.set_pos( koord(104+11+30, intTopOfButton) );
	quit_game.set_groesse( koord(104, 14) );
	quit_game.set_typ(button_t::roundbox);
	quit_game.add_listener( this );
	add_komponente( &quit_game );

	set_fenstergroesse( koord(260, intTopOfButton+14+8+16) );

	update_preview();
}



/**
 * Calculates preview from height map
 * @param filename name of heightfield file
 * @author Hajo/prissi
 */
bool
welt_gui_t::update_from_heightfield(const char *filename)
{
	DBG_MESSAGE("welt_gui_t::update_from_heightfield()",filename);

	sint16 w, h;
	sint8 *h_field=NULL;
	if(karte_t::get_height_data_from_file(filename, sets->get_grundwasser(), h_field, w, h, false )) {
		sets->set_groesse_x(w);
		sets->set_groesse_y(h);

		// ensure correct display under all circumstances
		const float skip_x = (float)preview_size/(float)w;
		const long skip_y = h>preview_size  ? h/preview_size :  1;
		for(int karte_y=0, y=0; y<h  &&  karte_y<preview_size; y++) {
			// new line?
			if(y%skip_y==0) {
				int karte_x=0;
				float rest_x=0.0;

				for(int x=0; x<w; x++) {
					while(karte_x<=rest_x  &&  karte_x<preview_size) {
						karte[(karte_y*preview_size)+karte_x] = reliefkarte_t::calc_hoehe_farbe( h_field[x+y*w], sets->get_grundwasser()-1 );
						karte_x ++;
					}
					rest_x += skip_x;
				}
				karte_y++;
			}
		}

		// blow up y direction
		if(h<preview_size) {
			const sint16 repeat_y = ((sint16)preview_size)/h;
			for(sint16 oy=h-1, y=preview_size-1;  oy>0;  oy--  ) {
				for( sint16 i=0;  i<=repeat_y  &&  y>0;  i++  ) {
					memcpy( karte+y, karte+oy, 3*preview_size );
					y --;
				}
			}
		}

		strcpy(map_number_s,translator::translate("file"));

		return true;
	}
	return false;
}



/**
 * Berechnet Preview-Karte neu. Inititialisiert RNG neu!
 * @author Hj. Malthaner
 */
void
welt_gui_t::update_preview()
{
	const int mx = sets->get_groesse_x()/preview_size;
	const int my = sets->get_groesse_y()/preview_size;

	if(loaded_heightfield) {
		update_from_heightfield(sets->heightfield);
	}
	else {
		setsimrand( 0xFFFFFFFF, sets->get_karte_nummer() );
		for(int j=0; j<preview_size; j++) {
			for(int i=0; i<preview_size; i++) {
				karte[j*preview_size+i] = reliefkarte_t::calc_hoehe_farbe(karte_t::perlin_hoehe( sets, koord(i*mx,j*my), koord::invalid ), sets->get_grundwasser()/Z_TILE_STEP);
			}
		}
		sets->heightfield = "";
		loaded_heightfield = false;
	}
}




/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool
welt_gui_t::action_triggered( gui_action_creator_t *komp,value_t v)
{
	// check for changed map (update preview for any event)
	int knr = inp_map_number.get_value(); //

	if(komp==&inp_x_size) {
		sets->set_groesse_x( v.i );
		inp_x_size.set_increment_mode( v.i>=512 ? 128 : 64 );
		inp_y_size.set_limits( 64, min(32766,16777216/sets->get_groesse_x()) );
		update_preview();
	}
	else if(komp==&inp_y_size) {
		sets->set_groesse_y( v.i );
		inp_y_size.set_increment_mode( v.i>=512 ? 128 : 64 );
		inp_x_size.set_limits( 64, min(32766,16777216/sets->get_groesse_y()) );
		update_preview();
	}
	else if(komp==&inp_number_of_towns) {
		sets->set_anzahl_staedte( v.i );
	}
	else if(komp==&inp_town_size) {
		sets->set_mittlere_einwohnerzahl( v.i );
	}
	else if(komp==&inp_intercity_road_len) {
		umgebung_t::intercity_road_length = v.i;
		inp_intercity_road_len.set_increment_mode( v.i>=1000 ? 100 : 20 );
	}
	else if(komp==&inp_traffic_density) {
		sets->set_verkehr_level( v.i );
	}
	else if(komp==&inp_other_industries) {
		sets->set_land_industry_chains( v.i );
	}
	else if(komp==&inp_electric_producer) {
		sets->set_electric_promille( v.i*10 );
	}
	else if(komp==&inp_tourist_attractions) {
		sets->set_tourist_attractions( v.i );
	}
	else if(komp==&inp_intro_date) {
		sets->set_starting_year( v.i );
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
		sets->set_grundwasser(-2);
		create_win(new load_relief_frame_t(sets), w_info, magic_load_t);
		knr = sets->get_karte_nummer();	// otherwise using cancel would not show the normal generated map again
	}
	else if(komp==&use_intro_dates) {
		sets->set_use_timeline( use_intro_dates.pressed^1 );
		use_intro_dates.pressed = sets->get_use_timeline();
	}
	else if(komp==&allow_player_change) {
		sets->set_allow_player_change( allow_player_change.pressed^1 );
		allow_player_change.pressed = sets->get_allow_player_change();
	}
	else if(komp==&use_beginner_mode) {
		sets->set_beginner_mode( use_beginner_mode.pressed^1 );
		use_beginner_mode.pressed = sets->get_beginner_mode();
	}
	else if(komp==&load_game) {
		load = true;
	}
	else if(komp==&load_scenario) {
		scenario = true;
	}
	else if(komp==&start_game) {
		if(loaded_heightfield) {
			load_heightfield = true;
		}
		else {
			start = true;
		}
	}
	else if(komp==&quit_game) {
		quit = true;
	}

	if(knr>=0) {
		sets->set_karte_nummer( knr );
		if(!loaded_heightfield) {
			update_preview();
		}
	}
	return true;
}



void welt_gui_t::infowin_event(const event_t *ev)
{
	gui_frame_t::infowin_event(ev);

	if(ev->ev_class==INFOWIN  &&  ev->ev_code==WIN_CLOSE) {
		close = true;
	}
}



void welt_gui_t::zeichnen(koord pos, koord gr)
{
	if(!loaded_heightfield  && sets->heightfield.len()!=0) {
		if(update_from_heightfield(sets->heightfield)) {
			loaded_heightfield = true;
		}
		else {
			loaded_heightfield = false;
			sets->heightfield = "";
		}
	}

	if(old_lang!=translator::get_language()) {
		// update button texts!
		random_map.set_text("Random map");
		random_map.set_tooltip("chooses a random map");
		load_map.set_text("Lade Relief");
		load_map.set_tooltip("load height data from file");
		use_intro_dates.set_text("Use timeline start year");
		allow_player_change.set_text("Allow player change");
		use_beginner_mode.set_text("Beginner mode");
		load_game.set_text("Load game");
		load_scenario.set_text("Load scenario");
		start_game.set_text("Starte Spiel");
		quit_game.set_text("Beenden");
		old_lang = translator::get_language();
		welt->set_dirty();
	}

	gui_frame_t::zeichnen(pos, gr);

	char buf[256];
	const int x = pos.x+10;
	int y = pos.y+16+START_HEIGHT;

	display_proportional_clip(x, y-20, translator::translate("1WORLD_CHOOSE"),ALIGN_LEFT, COL_BLACK, true);
	display_ddd_box_clip(x, y-5, RIGHT_ARROW, 0, MN_GREY0, MN_GREY4);		// seperator

	display_ddd_box_clip(x+173, y-20, preview_size+2, preview_size+2, MN_GREY0,MN_GREY4);
	display_array_wh(x+174, y-19, preview_size, preview_size, karte);

	display_proportional_clip(x, y, translator::translate("2WORLD_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);
	// since the display is done via a textfiled, we have nothing to do
	y += 12;

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
	sprintf(buf, translator::translate("3WORLD_CHOOSE"), memory);
	display_proportional_clip(x, y, buf, ALIGN_LEFT, COL_BLACK, true);
	y += 12;	// x size
	y += 12+5;	// y size
	y += BUTTON_HEIGHT+12+5;	// buttons

	display_proportional_clip(x, y, translator::translate("5WORLD_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Median Citizen per town"), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Intercity road len:"), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("6WORLD_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);
	y += 12+5;

	display_proportional_clip(x, y, translator::translate("Land industries"), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Percent Electricity"), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Tourist attractions"), ALIGN_LEFT, COL_BLACK, true);
	y += 12+5;

	y += 12+5;
	y += 12+5;
	y += 12+5;

	display_ddd_box_clip(x, y, 240, 0, MN_GREY0, MN_GREY4);
}
