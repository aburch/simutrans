/*
 * welt.cc
 *
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
#include "../simcity.h"
#include "../simvehikel.h"

#include "../simcolor.h"

#include "../simgraph.h"
#include "../simdisplay.h"

#include "load_relief_frame.h"

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
	loaded_heightfield = load_heightfield = false;
	load = start = close = false;
	int intTopOfButton=START_HEIGHT;
	sets->heightfield = "";

	// select map stuff ..
	map_number[0].setze_pos( koord(LEFT_ARROW,intTopOfButton) );
	map_number[0].setze_typ( button_t::repeatarrowleft );
	map_number[0].add_listener( this );
	add_komponente( map_number+0 );
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

	// mountian/water stuff
	intTopOfButton += 5;
	water_level[0].setze_pos( koord(LEFT_ARROW,intTopOfButton) );
	water_level[0].setze_typ( button_t::repeatarrowleft );
	water_level[0].add_listener( this );
	add_komponente( water_level+0 );
	water_level[1].setze_pos( koord(RIGHT_ARROW,intTopOfButton) );
	water_level[1].setze_typ( button_t::repeatarrowright );
	water_level[1].add_listener( this );
	add_komponente( water_level+1 );
	intTopOfButton += 12;

	mountain_height[0].setze_pos( koord(LEFT_ARROW,intTopOfButton) );
	mountain_height[0].setze_typ( button_t::repeatarrowleft );
	mountain_height[0].add_listener( this );
	add_komponente( mountain_height+0 );
	mountain_height[1].setze_pos( koord(RIGHT_ARROW,intTopOfButton) );
	mountain_height[1].setze_typ( button_t::repeatarrowright );
	mountain_height[1].add_listener( this );
	add_komponente( mountain_height+1 );
	intTopOfButton += 12;

	mountain_roughness[0].setze_pos( koord(LEFT_ARROW,intTopOfButton) );
	mountain_roughness[0].setze_typ( button_t::repeatarrowleft );
	mountain_roughness[0].add_listener( this );
	add_komponente( mountain_roughness+0 );
	mountain_roughness[1].setze_pos( koord(RIGHT_ARROW,intTopOfButton) );
	mountain_roughness[1].setze_typ( button_t::repeatarrowright );
	mountain_roughness[1].add_listener( this );
	add_komponente( mountain_roughness+1 );
	intTopOfButton += 12;

	intTopOfButton += 5;
	random_map.setze_pos( koord(11, intTopOfButton) );
	random_map.setze_groesse( koord(104, BUTTON_HEIGHT) );
	random_map.setze_typ(button_t::roundbox);
	random_map.add_listener( this );
	random_map.set_tooltip(translator::translate("chooses a random map"));
	add_komponente( &random_map );
	load_map.setze_pos( koord(104+11+30, intTopOfButton) );
	load_map.setze_groesse( koord(104, BUTTON_HEIGHT) );
	load_map.setze_typ(button_t::roundbox);
	load_map.set_tooltip(translator::translate("load height data from file"));
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

	town_industries[0].setze_pos( koord(LEFT_WIDE_ARROW,intTopOfButton) );
	town_industries[0].setze_typ( button_t::repeatarrowleft );
	town_industries[0].add_listener( this );
	add_komponente( town_industries+0 );
	town_industries[1].setze_pos( koord(RIGHT_WIDE_ARROW,intTopOfButton) );
	town_industries[1].setze_typ( button_t::repeatarrowright );
	town_industries[1].add_listener( this );
	add_komponente( town_industries+1 );
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

	// final stating buttons
	intTopOfButton += 10;
	load_game.setze_pos( koord(10, intTopOfButton) );
	load_game.setze_groesse( koord(104, 14) );
	load_game.setze_typ(button_t::roundbox);
	load_game.add_listener( this );
	add_komponente( &load_game );

	// Start game
	//----------------------
	start_game.setze_pos( koord(104+11+30, intTopOfButton) );
	start_game.setze_groesse( koord(104, 14) );
	start_game.setze_typ(button_t::roundbox);
	start_game.add_listener( this );
	add_komponente( &start_game );

	update_preview();
	setze_opaque(true);
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
		int w, h;

		read_line(buf, 255, file);
		if(buf[0]!='P'  ||  buf[1]!='6') {
			return false;
		}

		read_line(buf, 255, file);
		sscanf(buf, "%d %d", &w, &h);

		sets->setze_groesse_x(w);
		sets->setze_groesse_y(h);

		read_line(buf, 255, file);
DBG_MESSAGE("welt_gui_t::update_from_heightfield()",buf);
		if(buf[0]!='2'  ||  buf[1]!='5'  ||  buf[2]!='5') {
			return false;
		}


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
						karte[(karte_y*preview_size)+karte_x] = reliefkarte_t::calc_hoehe_farbe( (((R*2+G*3+B)/4 - 224+16)&(sint16)0xFFF0), sets->gib_grundwasser() );
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
		mountain_height[0].disable();
		mountain_height[1].disable();
		mountain_roughness[0].disable();
		mountain_roughness[1].disable();

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

	setsimrand( 0xFFFFFFFF, sets->gib_karte_nummer() );
	for(int j=0; j<preview_size; j++) {
		for(int i=0; i<preview_size; i++) {
			karte[j*preview_size+i] = reliefkarte_t::calc_hoehe_farbe(karte_t::perlin_hoehe(i*mx, j*my, sets->gib_map_roughness(), sets->gib_max_mountain_height())+16, sets->gib_grundwasser());
		}
	}
	sets->heightfield = "";
	loaded_heightfield = false;

	x_size[0].enable();
	x_size[1].enable();
	y_size[0].enable();
	y_size[1].enable();
	mountain_height[0].enable();
	mountain_height[1].enable();
	mountain_roughness[0].enable();
	mountain_roughness[1].enable();

DBG_MESSAGE("sizes","grund_t=%i, planquadrat_t=%d, ding_t=%d",sizeof(grund_t),sizeof(planquadrat_t),sizeof(ding_t));
}



koord welt_gui_t::gib_fenstergroesse() const
{
    return koord(260, 300);
}



/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool
welt_gui_t::action_triggered(gui_komponente_t *komp)
{
	if(komp==map_number+0) {
		if(sets->gib_karte_nummer() > 0 ) {
			if(!loaded_heightfield) {
				sets->setze_karte_nummer( sets->gib_karte_nummer() - 1 );
			}
			update_preview();
		}
	}
	else if(komp==map_number+1) {
		if(sets->gib_karte_nummer() < 9999 ) {
			if(!loaded_heightfield) {
				sets->setze_karte_nummer( sets->gib_karte_nummer() + 1 );
			}
			update_preview();
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
	else if(komp==water_level+0) {
		if(sets->gib_grundwasser() > -160 ) {
			sets->setze_grundwasser( sets->gib_grundwasser() - 32 );
			if(loaded_heightfield) {
				loaded_heightfield = 0;
			}
			else {
				update_preview();
			}
		}
	}
	else if(komp==water_level+1) {
		if(sets->gib_grundwasser() < 0 ) {
			sets->setze_grundwasser( sets->gib_grundwasser() + 32 );
			if(loaded_heightfield) {
				loaded_heightfield = 0;
			}
			else {
				update_preview();
			}
		}
	}
	else if(komp==mountain_height+0) {
		if(sets->gib_max_mountain_height() > 0.0 ) {
			sets->setze_max_mountain_height( sets->gib_max_mountain_height() - 10 );
			update_preview();
		}
	}
	else if(komp==mountain_height+1) {
		if(sets->gib_max_mountain_height() < 320.0 ) {
			sets->setze_max_mountain_height( sets->gib_max_mountain_height() + 10 );
			update_preview();
		}
	}
	else if(komp==mountain_roughness+0) {
		if(sets->gib_map_roughness() > 0.4 ) {
			sets->setze_map_roughness( sets->gib_map_roughness() - 0.05 );
			update_preview();
		}
	}
	else if(komp==mountain_roughness+1) {
#ifndef DOUBLE_GROUNDS
		if(sets->gib_map_roughness() < 0.7 ) {
#else
		if(sets->gib_map_roughness() < 1.00) {
#endif
			sets->setze_map_roughness( sets->gib_map_roughness() + 0.05 );
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
	else if(komp==town_industries+0) {
		if(sets->gib_city_industry_chains() > 0) {
			sets->setze_city_industry_chains( sets->gib_city_industry_chains() - 1 );
		}
	}
	else if(komp==town_industries+1) {
		sets->setze_city_industry_chains( sets->gib_city_industry_chains() + 1 );
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
		sets->setze_karte_nummer(simrand(9999));
		update_preview();
	}
	else if(komp==&load_map) {
		// load relief
		loaded_heightfield = false;
		sets->heightfield = "";
		create_win(new load_relief_frame_t(welt, sets), w_info, magic_load_t);
	}
	else if(komp==&use_intro_dates) {
		sets->setze_use_timeline( !sets->gib_use_timeline() );
	}
	else if(komp==&allow_player_change) {
		sets->setze_allow_player_change( !sets->gib_allow_player_change() );
	}
	else if(komp==&load_game) {
		load = true;
	}
	else if(komp==&start_game) {
		if(loaded_heightfield) {
			load_heightfield = true;
		}
		else {
			start = true;
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



static char *ntos(int number, const char *format)
{
    static char tempstring[32];
    int r;

    if (format) {
          r = sprintf(tempstring, format, number);
    } else {
          r = sprintf(tempstring, "%d", number);
    }
    assert(r<16);

    return tempstring;
}



void welt_gui_t::zeichnen(koord pos, koord gr)
{
	if(!loaded_heightfield  && sets->heightfield.len()!=0) {
		if(update_from_heightfield(sets->heightfield)) {
			loaded_heightfield = true;
		}
		else {
			sets->heightfield = "";
		}
	}
	use_intro_dates.pressed = sets->gib_use_timeline();
	allow_player_change.pressed = sets->gib_allow_player_change();

	if(old_lang!=translator::get_language()) {
		// update button texts!
		random_map.setze_text(translator::translate("Random map"));
		load_map.setze_text(translator::translate("Lade Relief"));
		use_intro_dates.setze_text(translator::translate("Use timeline start year"));
		allow_player_change.setze_text(translator::translate("Allow player change"));
		load_game.setze_text(translator::translate("Load game"));
		start_game.setze_text(translator::translate("Starte Spiel"));
//		quit_game.setze_text(translator::translate("Beenden"));
		old_lang = translator::get_language();
	}

	gui_frame_t::zeichnen(pos, gr);

	char buf[256];
	const int x = pos.x+10;
	int y = pos.y+16+START_HEIGHT;

	display_proportional_clip(x, y-20, translator::translate("1WORLD_CHOOSE"),ALIGN_LEFT, COL_BLACK, true);
	display_ddd_box_clip(x, y-5, 240, 0, MN_GREY0, MN_GREY4);		// seperator

	display_ddd_box_clip(x+173, y, preview_size+2, preview_size+2, MN_GREY0,MN_GREY4);
	display_array_wh(x+174, y+1, preview_size, preview_size, karte);

	display_proportional_clip(x, y, translator::translate("2WORLD_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);
	if(!loaded_heightfield) {
		display_proportional_clip(x+TEXT_RIGHT, y, ntos(sets->gib_karte_nummer(), "%3d"), ALIGN_RIGHT, COL_WHITE, true);
	}
	else {
		display_proportional_clip(x+TEXT_RIGHT, y, translator::translate("file"), ALIGN_RIGHT, COL_WHITE, true);
	}
	y += 12;

	const long x2 = sets->gib_groesse_x() * sets->gib_groesse_y();
	const long memory = (sizeof(karte_t)+8l*sizeof(spieler_t)+ 100*sizeof(convoi_t) + (sets->gib_groesse_x()+sets->gib_groesse_y())*(sizeof(schiene_t)+sizeof(vehikel_t)) + (sizeof(stadt_t)*sets->gib_anzahl_staedte()) + x2*(sizeof(grund_t)+sizeof(planquadrat_t)+4*sizeof(ding_t)) )/(1024l*1024l);
	sprintf(buf, translator::translate("3WORLD_CHOOSE"), memory);
	display_proportional_clip(x, y, buf, ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(sets->gib_groesse_x(), "%3d"), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(sets->gib_groesse_y(), "%3d"), ALIGN_RIGHT, COL_WHITE, true);
	y += 12+5;

      // water level       18-Nov-01       Markus W. Added
	display_proportional_clip(x, y, translator::translate("Water level"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(sets->gib_grundwasser()/32+5,"%3d"), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Mountain height"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos((int)(sets->gib_max_mountain_height()), "%3d"), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Map roughness"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos((int)(sets->gib_map_roughness()*20.0 + 0.5)-8 , "%3d") , ALIGN_RIGHT, COL_WHITE, true);     // x = round(roughness * 10)-4  // 0.6 * 10 - 4 = 2    //29-Nov-01     Markus W. Added
	y += 12+5;

	y += BUTTON_HEIGHT+5;	// buttons

	display_proportional_clip(x, y, translator::translate("5WORLD_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(sets->gib_anzahl_staedte(), "%3d"), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Median Citizen per town"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(sets->gib_mittlere_einwohnerzahl(),"%3d"),ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Intercity road len:"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(umgebung_t::intercity_road_length,"%3d"),ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("6WORLD_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(sets->gib_verkehr_level(),"%3d"), ALIGN_RIGHT, COL_WHITE, true);
	y += 12+5;

	display_proportional_clip(x, y, translator::translate("Land industries"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(sets->gib_land_industry_chains(),"%3d"), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("City industries"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(sets->gib_city_industry_chains(),"%3d"), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Tourist attractions"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(sets->gib_tourist_attractions(),"%3d"), ALIGN_RIGHT, COL_WHITE, true);
	y += 12+5;

	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(sets->gib_starting_year(), "%3d"), ALIGN_RIGHT, COL_WHITE, true);
	y += 12+5;
	y += 12+5;

	display_ddd_box_clip(x, y, 240, 0, MN_GREY0, MN_GREY4);
}
