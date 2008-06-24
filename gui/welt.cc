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
#include "../simplay.h"
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

#include <sys/stat.h>
#include <time.h>

welt_gui_t::welt_gui_t(karte_t *welt, einstellungen_t *sets) : gui_frame_t("Neue Welt")
{
DBG_MESSAGE("","sizeof(stat)=%d, sizeof(tm)=%d",sizeof(struct stat),sizeof(struct tm) );
	this->welt = welt;
	this->sets = sets;
	this->old_lang = -1;
	this->sets->setze_beginner_mode(umgebung_t::beginner_mode_first);
	loaded_heightfield = load_heightfield = false;
	load = start = close = scenario = quit = false;
	int intTopOfButton=START_HEIGHT;
	sets->heightfield = "";

	// select map stuff ..
	map_number[0].setze_pos( koord(LEFT_ARROW,intTopOfButton) );
	map_number[0].setze_typ( button_t::repeatarrowleft );
	map_number[0].add_listener( this );
	add_komponente( map_number+0 );
	sprintf(map_number_s,"%d",abs(sets->gib_karte_nummer())%9999);
	inp_map_number.setze_pos( koord(RIGHT_ARROW-36,intTopOfButton-2) );
	inp_map_number.setze_groesse( koord(32,12) );
	inp_map_number.setze_text(map_number_s, 5);
	inp_map_number.add_listener( this );
	add_komponente( &inp_map_number );
	map_number[1].setze_pos( koord(RIGHT_ARROW,intTopOfButton) );
	map_number[1].setze_typ( button_t::repeatarrowright );
	map_number[1].add_listener( this );
	add_komponente( map_number+1 );
	intTopOfButton += 12;

	x_size[0].setze_pos( koord(LEFT_ARROW,intTopOfButton) );
	x_size[0].setze_typ( button_t::repeatarrowleft );
	x_size[0].add_listener( this );
	add_komponente( x_size+0 );
	x_size[1].setze_pos( koord(RIGHT_ARROW,intTopOfButton) );
	x_size[1].setze_typ( button_t::repeatarrowright );
	x_size[1].add_listener( this );
	add_komponente( x_size+1 );
	intTopOfButton += 12;

	y_size[0].setze_pos( koord(LEFT_ARROW,intTopOfButton) );
	y_size[0].setze_typ( button_t::repeatarrowleft );
	y_size[0].add_listener( this );
	add_komponente( y_size+0 );
	y_size[1].setze_pos( koord(RIGHT_ARROW,intTopOfButton) );
	y_size[1].setze_typ( button_t::repeatarrowright );
	y_size[1].add_listener( this );
	add_komponente( y_size+1 );
	intTopOfButton += 12;
	intTopOfButton += 12;

	// towns etc.
	intTopOfButton += 5;
	random_map.setze_pos( koord(11, intTopOfButton) );
	random_map.setze_groesse( koord(104, BUTTON_HEIGHT) );
	random_map.setze_typ(button_t::roundbox);
	random_map.add_listener( this );
	add_komponente( &random_map );
	load_map.setze_pos( koord(104+11+30, intTopOfButton) );
	load_map.setze_groesse( koord(104, BUTTON_HEIGHT) );
	load_map.setze_typ(button_t::roundbox);
	load_map.add_listener( this );
	add_komponente( &load_map );
	intTopOfButton += BUTTON_HEIGHT;

	// city stuff
	intTopOfButton += 5;
	number_of_towns[0].setze_pos( koord(LEFT_WIDE_ARROW,intTopOfButton) );
	number_of_towns[0].setze_typ( button_t::repeatarrowleft );
	number_of_towns[0].add_listener( this );
	add_komponente( number_of_towns+0 );
	number_of_towns[1].setze_pos( koord(RIGHT_WIDE_ARROW,intTopOfButton) );
	number_of_towns[1].setze_typ( button_t::repeatarrowright );
	number_of_towns[1].add_listener( this );
	add_komponente( number_of_towns+1 );
	intTopOfButton += 12;

	town_size[0].setze_pos( koord(LEFT_WIDE_ARROW,intTopOfButton) );
	town_size[0].setze_typ( button_t::repeatarrowleft );
	town_size[0].add_listener( this );
	add_komponente( town_size+0 );
	town_size[1].setze_pos( koord(RIGHT_WIDE_ARROW,intTopOfButton) );
	town_size[1].setze_typ( button_t::repeatarrowright );
	town_size[1].add_listener( this );
	add_komponente( town_size+1 );
	intTopOfButton += 12;

	intercity_road_len[0].setze_pos( koord(LEFT_WIDE_ARROW,intTopOfButton) );
	intercity_road_len[0].setze_typ( button_t::repeatarrowleft );
	intercity_road_len[0].add_listener( this );
	add_komponente( intercity_road_len+0 );
	intercity_road_len[1].setze_pos( koord(RIGHT_WIDE_ARROW,intTopOfButton) );
	intercity_road_len[1].setze_typ( button_t::repeatarrowright );
	intercity_road_len[1].add_listener( this );
	add_komponente( intercity_road_len+1 );
	intTopOfButton += 12;

	traffic_desity[0].setze_pos( koord(LEFT_WIDE_ARROW,intTopOfButton) );
	traffic_desity[0].setze_typ( button_t::repeatarrowleft );
	traffic_desity[0].add_listener( this );
	add_komponente( traffic_desity+0 );
	traffic_desity[1].setze_pos( koord(RIGHT_WIDE_ARROW,intTopOfButton) );
	traffic_desity[1].setze_typ( button_t::repeatarrowright );
	traffic_desity[1].add_listener( this );
	add_komponente( traffic_desity+1 );
	intTopOfButton += 12;

	// industry stuff
	intTopOfButton += 5;
	other_industries[0].setze_pos( koord(LEFT_WIDE_ARROW,intTopOfButton) );
	other_industries[0].setze_typ( button_t::repeatarrowleft );
	other_industries[0].add_listener( this );
	add_komponente( other_industries+0 );
	other_industries[1].setze_pos( koord(RIGHT_WIDE_ARROW,intTopOfButton) );
	other_industries[1].setze_typ( button_t::repeatarrowright );
	other_industries[1].add_listener( this );
	add_komponente( other_industries+1 );
	intTopOfButton += 12;

	electric_producer[0].setze_pos( koord(LEFT_WIDE_ARROW,intTopOfButton) );
	electric_producer[0].setze_typ( button_t::repeatarrowleft );
	electric_producer[0].add_listener( this );
	add_komponente( electric_producer+0 );
	electric_producer[1].setze_pos( koord(RIGHT_WIDE_ARROW,intTopOfButton) );
	electric_producer[1].setze_typ( button_t::repeatarrowright );
	electric_producer[1].add_listener( this );
	add_komponente( electric_producer+1 );
	intTopOfButton += 12;

	tourist_attractions[0].setze_pos( koord(LEFT_WIDE_ARROW,intTopOfButton) );
	tourist_attractions[0].setze_typ( button_t::repeatarrowleft );
	tourist_attractions[0].add_listener( this );
	add_komponente( tourist_attractions+0 );
	tourist_attractions[1].setze_pos( koord(RIGHT_WIDE_ARROW,intTopOfButton) );
	tourist_attractions[1].setze_typ( button_t::repeatarrowright );
	tourist_attractions[1].add_listener( this );
	add_komponente( tourist_attractions+1 );
	intTopOfButton += 12;

	// other settings
	intTopOfButton += 5;
	use_intro_dates.setze_pos( koord(10,intTopOfButton) );
	use_intro_dates.setze_typ( button_t::square );
	use_intro_dates.add_listener( this );
	add_komponente( &use_intro_dates );
	intro_date[0].setze_pos( koord(LEFT_WIDE_ARROW,intTopOfButton) );
	intro_date[0].setze_typ( button_t::repeatarrowleft );
	intro_date[0].add_listener( this );
	add_komponente( intro_date+0 );
	intro_date[1].setze_pos( koord(RIGHT_WIDE_ARROW,intTopOfButton) );
	intro_date[1].setze_typ( button_t::repeatarrowright );
	intro_date[1].add_listener( this );
	add_komponente( intro_date+1 );
	intTopOfButton += 12;

	intTopOfButton += 5;
	allow_player_change.setze_pos( koord(10,intTopOfButton) );
	allow_player_change.setze_typ( button_t::square );
	allow_player_change.add_listener( this );
	add_komponente( &allow_player_change );
	intTopOfButton += 12;

	intTopOfButton += 5;
	use_beginner_mode.setze_pos( koord(10,intTopOfButton) );
	use_beginner_mode.setze_typ( button_t::square );
	use_beginner_mode.add_listener( this );
	add_komponente( &use_beginner_mode );
	intTopOfButton += 12;

	// load game
	intTopOfButton += 10;
	load_game.setze_pos( koord(10, intTopOfButton) );
	load_game.setze_groesse( koord(104, 14) );
	load_game.setze_typ(button_t::roundbox);
	load_game.add_listener( this );
	add_komponente( &load_game );

	// load scenario
	load_scenario.setze_pos( koord(104+11+30, intTopOfButton) );
	load_scenario.setze_groesse( koord(104, 14) );
	load_scenario.setze_typ(button_t::roundbox);
	load_scenario.add_listener( this );
	add_komponente( &load_scenario );

	// start game
	intTopOfButton += 5+BUTTON_HEIGHT;
	start_game.setze_pos( koord(10, intTopOfButton) );
	start_game.setze_groesse( koord(104, 14) );
	start_game.setze_typ(button_t::roundbox);
	start_game.add_listener( this );
	add_komponente( &start_game );

	// quit game
	quit_game.setze_pos( koord(104+11+30, intTopOfButton) );
	quit_game.setze_groesse( koord(104, 14) );
	quit_game.setze_typ(button_t::roundbox);
	quit_game.add_listener( this );
	add_komponente( &quit_game );

	setze_fenstergroesse( koord(260, intTopOfButton+14+8+16) );

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
	FILE *file = fopen(filename, "rb");
	if(file) {
		char buf [256];
		int param[3], index=0;
		char *c=buf+2;

		// parsing the header of this mixed file format is nottrivial ...
		fread(buf, 1, 3, file);
		buf[2] = 0;
		if(strncmp(buf, "P6", 2)) {
			dbg->fatal("karte_t::load_heightfield()","Heightfield has wrong image type %s instead P6", buf);
		}

		while(index<3) {
			// the format is "P6[whitespace]width[whitespace]height[[whitespace bitdepth]]newline]
			// however, Photoshop is the first program, that uses space for the first whitespace ...
			// so we cater for Photoshop too
			while(*c  &&  *c<=32) {
				c++;
			}
			// usually, after P6 there comes a comment with the maker
			// but comments can be anywhere
			if(*c==0) {
				read_line(buf, 255, file);
				c = buf;
				continue;
			}
			param[index++] = atoi(c);
			while(*c>='0'  &&  *c<='9') {
				c++;
			}
		}

		if(param[2]!=255) {
			dbg->fatal("karte_t::load_heightfield()","Heightfield has wrong color depth %d", param[2] );
		}

		// after the header only binary data will follow
		int w=param[0], h=param[1];
		sets->setze_groesse_x(w);
		sets->setze_groesse_y(h);

		// ensure correct display under all circumstances
		const float skip_x = (float)preview_size/(float)w;
		const long skip_y = h>preview_size  ? h/preview_size :  1;
		for(int karte_y=0, y=0; y<h  &&  karte_y<preview_size; y++) {
			// new line?
			if(y%skip_y==0) {
				int karte_x=0;
				float rest_x=0.0;

				for(int x=0; x<w; x++) {
					sint16 R = (unsigned char)fgetc(file);
					sint16 G = (unsigned char)fgetc(file);
					sint16 B = (unsigned char)fgetc(file);

					while(karte_x<=rest_x  &&  karte_x<preview_size) {
						karte[(karte_y*preview_size)+karte_x] = reliefkarte_t::calc_hoehe_farbe( (((R*2+G*3+B)/4 - 224+16)/16)*Z_TILE_STEP, sets->gib_grundwasser() );
						karte_x ++;
					}
					rest_x += skip_x;
				}
				karte_y++;
			}
			else {
				// just skip it
				fseek(file,3*w, SEEK_CUR);
			}
		}

		// blow up y direction
		if(h<preview_size) {
			const long repeat_y = preview_size/h;
			for(int oy=h-1, y=preview_size-1;  oy>0;  oy--  ) {
				for( int i=0;  i<=repeat_y  &&  y>0;  i++  ) {
					memcpy( karte+y, karte+oy, 3*preview_size );
					y --;
				}
			}
		}

DBG_MESSAGE("welt_gui_t::update_from_heightfield()","success (%5f,%5f)",skip_x,skip_y);
		fclose(file);

		x_size[0].disable();
		x_size[1].disable();
		y_size[0].disable();
		y_size[1].disable();

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
	const int mx = sets->gib_groesse_x()/preview_size;
	const int my = sets->gib_groesse_y()/preview_size;

	if(loaded_heightfield) {
		update_from_heightfield(sets->heightfield);
	}
	else {
		setsimrand( 0xFFFFFFFF, sets->gib_karte_nummer() );
		for(int j=0; j<preview_size; j++) {
			for(int i=0; i<preview_size; i++) {
				karte[j*preview_size+i] = reliefkarte_t::calc_hoehe_farbe((karte_t::perlin_hoehe(i*mx, j*my, sets->gib_map_roughness(), sets->gib_max_mountain_height())/16)+1, sets->gib_grundwasser()/Z_TILE_STEP);
			}
		}
		sets->heightfield = "";
		loaded_heightfield = false;
	}

	x_size[0].enable();
	x_size[1].enable();
	y_size[0].enable();
	y_size[1].enable();
}




/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool
welt_gui_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
	// check for changed map (update preview for any event)
	int knr = atoi(map_number_s)%9999;
	if(knr==0  &&  (map_number_s[0]<'0'  ||  map_number_s[0]>'9')) {
		knr = -1;
	}

	if(komp==map_number+0) {
		knr --;
	}
	else if(komp==map_number+1) {
		if(knr>=0) {
			knr ++;
		}
	}
	else if(komp==x_size+0) {
		if(sets->gib_groesse_x() > 512 ) {
			sets->setze_groesse_x( (sets->gib_groesse_x()-1)&0x1F80 );
			update_preview();
		} else if(sets->gib_groesse_x() > 64 ) {
			sets->setze_groesse_x( (sets->gib_groesse_x()-1)&0x1FC0 );
			update_preview();
		}
	}
	else if(komp==x_size+1) {
		if(sets->gib_groesse_x() < 512 ) {
			sets->setze_groesse_x( (sets->gib_groesse_x()+64)&0x1FC0 );
			update_preview();
		} else if(sets->gib_groesse_x() < 4096 ) {
			sets->setze_groesse_x( (sets->gib_groesse_x()+128)&0x1F80 );
			update_preview();
		}
	}
	else if(komp==y_size+0) {
		if(sets->gib_groesse_y() > 512 ) {
			sets->setze_groesse_y( (sets->gib_groesse_y()-1)&0x1F80 );
			update_preview();
		} else if(sets->gib_groesse_y() > 64 ) {
			sets->setze_groesse_y( (sets->gib_groesse_y()-1)&0x1FC0 );
			update_preview();
		}
	}
	else if(komp==y_size+1) {
		if(sets->gib_groesse_y() < 512 ) {
			sets->setze_groesse_y( (sets->gib_groesse_y()+64)&0x1FC0 );
			update_preview();
		} else if(sets->gib_groesse_y() < 4096 ) {
			sets->setze_groesse_y( (sets->gib_groesse_y()+128)&0x1F80 );
			update_preview();
		}
	}
	else if(komp==number_of_towns+0) {
		if(sets->gib_anzahl_staedte()>0 ) {
			sets->setze_anzahl_staedte( sets->gib_anzahl_staedte() - 1 );
		}
	}
	else if(komp==number_of_towns+1) {
		sets->setze_anzahl_staedte( sets->gib_anzahl_staedte() + 1 );
	}
	else if(komp==town_size+0) {
		if(sets->gib_mittlere_einwohnerzahl()>100 ) {
			sets->setze_mittlere_einwohnerzahl( sets->gib_mittlere_einwohnerzahl() - 100 );
		}
	}
	else if(komp==town_size+1) {
		sets->setze_mittlere_einwohnerzahl( sets->gib_mittlere_einwohnerzahl() + 100 );
	}
	else if(komp==intercity_road_len+0) {
		umgebung_t::intercity_road_length -= 100;
		if(umgebung_t::intercity_road_length<0) {
			umgebung_t::intercity_road_length = 0;
		}
	}
	else if(komp==intercity_road_len+1) {
		umgebung_t::intercity_road_length += 100;
	}
	else if(komp==traffic_desity+0) {
		if(sets->gib_verkehr_level() > 0 ) {
			sets->setze_verkehr_level( sets->gib_verkehr_level() - 1 );
		}
	}
	else if(komp==traffic_desity+1) {
		if(sets->gib_verkehr_level() < 16 ) {
			sets->setze_verkehr_level( sets->gib_verkehr_level() + 1 );
		}
	}
	else if(komp==other_industries+0) {
		if(sets->gib_land_industry_chains() > 0) {
			sets->setze_land_industry_chains( sets->gib_land_industry_chains() -1 );
		}
	}
	else if(komp==other_industries+1) {
		sets->setze_land_industry_chains( sets->gib_land_industry_chains() + 1 );
	}
	else if(komp==electric_producer+0) {
		if(sets->gib_electric_promille() > 0) {
			sets->setze_electric_promille( sets->gib_electric_promille() - 1 );
		}
	}
	else if(komp==electric_producer+1) {
		sets->setze_electric_promille( sets->gib_electric_promille() + 1 );
	}
	else if(komp==tourist_attractions+0) {
		if(sets->gib_tourist_attractions() > 0) {
			sets->setze_tourist_attractions( sets->gib_tourist_attractions() - 1 );
		}
	}
	else if(komp==tourist_attractions+1) {
		sets->setze_tourist_attractions( sets->gib_tourist_attractions() + 1 );
	}
	else if(komp==intro_date+0) {
		sets->setze_starting_year( sets->gib_starting_year() - 1 );
	}
	else if(komp==intro_date+1) {
		sets->setze_starting_year( sets->gib_starting_year() + 1 );
	}
	else if(komp==&random_map) {
		knr = simrand(9999);
	}
	else if(komp==&load_map) {
		// load relief
		loaded_heightfield = false;
		sets->heightfield = "";
		sets->setze_grundwasser(-2);
		create_win(new load_relief_frame_t(sets), w_info, magic_load_t);
		knr = sets->gib_karte_nummer();	// otherwise using cancel would not show the normal generated map again
	}
	else if(komp==&use_intro_dates) {
		sets->setze_use_timeline( !sets->gib_use_timeline() );
	}
	else if(komp==&allow_player_change) {
		sets->setze_allow_player_change( !sets->gib_allow_player_change() );
	}
	else if(komp==&use_beginner_mode) {
		sets->setze_beginner_mode( !sets->gib_beginner_mode() );
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
		sets->setze_karte_nummer( knr );
		if(!loaded_heightfield) {
			update_preview();
			sprintf(map_number_s,"%d",knr);
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
			sprintf(map_number_s,"%d",abs(sets->gib_karte_nummer())%9999);
		}
	}
	use_intro_dates.pressed = sets->gib_use_timeline();
	allow_player_change.pressed = sets->gib_allow_player_change();
	use_beginner_mode.pressed = sets->gib_beginner_mode();

	if(old_lang!=translator::get_language()) {
		// update button texts!
		random_map.setze_text("Random map");
		random_map.set_tooltip("chooses a random map");
		load_map.setze_text("Lade Relief");
		load_map.set_tooltip("load height data from file");
		use_intro_dates.setze_text("Use timeline start year");
		allow_player_change.setze_text("Allow player change");
		use_beginner_mode.setze_text("Beginner mode");
		load_game.setze_text("Load game");
		load_scenario.setze_text("Load scenario");
		start_game.setze_text("Starte Spiel");
		quit_game.setze_text("Beenden");
		old_lang = translator::get_language();
		welt->setze_dirty();
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

	const uint sx = sets->gib_groesse_x();
	const uint sy = sets->gib_groesse_y();
	const long memory = (
		sizeof(karte_t) +
		sizeof(spieler_t) * 8 +
		sizeof(convoi_t) * 1000 +
		(sizeof(schiene_t) + sizeof(vehikel_t)) * 10 * (sx + sy) +
		sizeof(stadt_t) * sets->gib_anzahl_staedte() +
		(
			sizeof(grund_t) +
			sizeof(planquadrat_t) +
			sizeof(baum_t) * 2 +
			sizeof(void*) * 4
		) * sx * sy
	) / (1024 * 1024);
	sprintf(buf, translator::translate("3WORLD_CHOOSE"), memory);
	display_proportional_clip(x, y, buf, ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(sets->gib_groesse_x(), 0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(sets->gib_groesse_y(), 0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12+5;

	y += BUTTON_HEIGHT+12+5;	// buttons

	display_proportional_clip(x, y, translator::translate("5WORLD_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(sets->gib_anzahl_staedte(), 0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Median Citizen per town"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(sets->gib_mittlere_einwohnerzahl(),0),ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Intercity road len:"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(umgebung_t::intercity_road_length,0),ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("6WORLD_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(sets->gib_verkehr_level(),0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12+5;

	display_proportional_clip(x, y, translator::translate("Land industries"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(sets->gib_land_industry_chains(),0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Percent Electricity"), ALIGN_LEFT, COL_BLACK, true);
	sprintf( buf, "%1.1lf", sets->gib_electric_promille()/10.0 );
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, buf, ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Tourist attractions"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(sets->gib_tourist_attractions(),0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12+5;

	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(sets->gib_starting_year(), 0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12+5;
	y += 12+5;
	y += 12+5;

	display_ddd_box_clip(x, y, 240, 0, MN_GREY0, MN_GREY4);
}
