/*
 * colors.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include "colors.h"

#include "../simdebug.h"
#include "../simworld.h"
#include "../simimg.h"
#include "../simintr.h"
#include "../simcolor.h"
#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../simgraph.h"
#include "../simdisplay.h"

#include "../utils/simstring.h"

// y coordinates
#define UNDERGROUND (0*13+6)
#define DAY_NIGHT (1*13+6)
#define BRIGHTNESS (2*13+6)
#define SCROLL_INVERS (3*13+6)
#define SCROLL_SPEED (4*13+6)

#define SEPERATE1 (5*13+6)

#define USE_TRANSPARENCY (5*13+6+4)
#define HIDE_TREES (6*13+6+4)
#define HIDE_CITY_HOUSES (7*13+6+4)
#define HIDE_ALL_HOUSES (8*13+6+4)

#define SEPERATE2 (9*13+6+4)

#define USE_TRANSPARENCY_STATIONS (9*13+6+8)
#define SHOW_STATION_COVERAGE (10*13+6+8)
#define CITY_WALKER (11*13+6+8)
#define STOP_WALKER (12*13+6+8)
#define DENS_TRAFFIC (13*13+6+8)

#define SEPERATE3 (14*13+6+8)

#define FPS_DATA (14*13+6+12)
#define IDLE_DATA (15*13+6+12)
#define FRAME_DATA (16*13+6+12)
#define LOOP_DATA (17*13+6+12)

#define BOTTOM (18*13+6+12+16)

// x coordinates
#define RIGHT_WIDTH (220)
#define ARR_LEFT (125)
#define ARR_RIGHT (150)
#define NUMBER (148)



color_gui_t::color_gui_t(karte_t *welt) :
	gui_frame_t("Helligk. u. Farben")
{
	this->welt = welt;

	// brightness
	buttons[0].setze_pos( koord(ARR_LEFT,BRIGHTNESS) );
	buttons[0].setze_typ(button_t::repeatarrowleft);

	buttons[1].setze_pos( koord(ARR_RIGHT,BRIGHTNESS) );
	buttons[1].setze_typ(button_t::repeatarrowright);

	// scrollspeed
	buttons[2].setze_pos( koord(ARR_LEFT,SCROLL_SPEED) );
	buttons[2].setze_typ(button_t::repeatarrowleft);

	buttons[3].setze_pos( koord(ARR_RIGHT,SCROLL_SPEED) );
	buttons[3].setze_typ(button_t::repeatarrowright);

	// traffic density
	buttons[4].setze_pos( koord(ARR_LEFT,DENS_TRAFFIC) );
	buttons[4].setze_typ(button_t::repeatarrowleft);

	buttons[5].setze_pos( koord(ARR_RIGHT,DENS_TRAFFIC) );
	buttons[5].setze_typ(button_t::repeatarrowright);

	// other settings
	buttons[6].setze_pos( koord(10,SCROLL_INVERS) );
	buttons[6].setze_typ(button_t::square_state);
	buttons[6].setze_text("4LIGHT_CHOOSE");
	buttons[6].pressed = welt->gib_einstellungen()->gib_scroll_multi() < 0;

	buttons[7].setze_pos( koord(10,STOP_WALKER) );
	buttons[7].setze_typ(button_t::square_state);
	buttons[7].setze_text("5LIGHT_CHOOSE");
	buttons[7].pressed = welt->gib_einstellungen()->gib_show_pax();

	buttons[8].setze_pos( koord(10,CITY_WALKER) );
	buttons[8].setze_typ(button_t::square_state);
	buttons[8].setze_text("6LIGHT_CHOOSE");
	buttons[8].pressed = umgebung_t::fussgaenger;

	buttons[9].setze_pos( koord(10,DAY_NIGHT) );
	buttons[9].setze_typ(button_t::square_state);
	buttons[9].setze_text("8WORLD_CHOOSE");
	buttons[9].pressed = umgebung_t::night_shift;

	buttons[10].setze_pos( koord(10,USE_TRANSPARENCY) );
	buttons[10].setze_typ(button_t::square_state);
	buttons[10].setze_text("hide transparent");
	buttons[10].pressed = umgebung_t::hide_with_transparency;

	buttons[11].setze_pos( koord(10,HIDE_TREES) );
	buttons[11].setze_typ(button_t::square_state);
	buttons[11].setze_text("hide trees");

	buttons[12].setze_pos( koord(10,HIDE_CITY_HOUSES) );
	buttons[12].setze_typ(button_t::square_state);
	buttons[12].setze_text("hide city building");

	buttons[13].setze_pos( koord(10,HIDE_ALL_HOUSES) );
	buttons[13].setze_typ(button_t::square_state);
	buttons[13].setze_text("hide all building");

	buttons[14].setze_pos( koord(10,USE_TRANSPARENCY_STATIONS) );
	buttons[14].setze_typ(button_t::square_state);
	buttons[14].setze_text("transparent station coverage");
	buttons[14].pressed = umgebung_t::use_transparency_station_coverage;

	buttons[15].setze_pos( koord(10,SHOW_STATION_COVERAGE) );
	buttons[15].setze_typ(button_t::square_state);
	buttons[15].setze_text("show station coverage");

	buttons[16].setze_pos( koord(10,UNDERGROUND) );
	buttons[16].setze_typ(button_t::square_state);
	buttons[16].setze_text("underground mode");

	for(int i=0;  i<17;  i++ ) {
		buttons[i].add_listener(this);
		add_komponente( buttons+i );
	}

	setze_opaque(true);
	setze_fenstergroesse( koord(RIGHT_WIDTH, BOTTOM) );
}



bool
color_gui_t::action_triggered(gui_komponente_t *komp, value_t)
{
	einstellungen_t * sets = welt->gib_einstellungen();

	if((buttons+0)==komp) {
	    display_set_light(display_get_light()-1);
	} else if((buttons+1)==komp) {
	  if(display_get_light() < 0) {
	    display_set_light(display_get_light()+1);
	  }
	} else if((buttons+4)==komp) {
		if(sets->gib_verkehr_level() > 0 ) {
			sets->setze_verkehr_level( sets->gib_verkehr_level() - 1 );
		}
	} else if((buttons+5)==komp) {
		if(sets->gib_verkehr_level() < 16 ) {
			sets->setze_verkehr_level( sets->gib_verkehr_level() + 1 );
		}
	} else if((buttons+2)==komp) {
		if(sets->gib_scroll_multi() > 1) {
			welt->setze_scroll_multi(sets->gib_scroll_multi()-1);
		}
		if(sets->gib_scroll_multi() < -1) {
			welt->setze_scroll_multi(sets->gib_scroll_multi()+1);
		}
	} else if((buttons+3)==komp) {
		if(sets->gib_scroll_multi() >= 1) {
			welt->setze_scroll_multi(sets->gib_scroll_multi()+1);
		}
		if(sets->gib_scroll_multi() <= -1) {
			welt->setze_scroll_multi(sets->gib_scroll_multi()-1);
		}
	} else if((buttons+6)==komp) {
		welt->setze_scroll_multi(- sets->gib_scroll_multi());
		buttons[6].pressed ^= 1;
	} else if((buttons+7)==komp) {
		welt->gib_einstellungen()->setze_show_pax( !welt->gib_einstellungen()->gib_show_pax() );
		buttons[7].pressed ^= 1;
	} else if((buttons+8)==komp) {
		umgebung_t::fussgaenger = !umgebung_t::fussgaenger;
		buttons[8].pressed ^= 1;
	} else if((buttons+9)==komp) {
		umgebung_t::night_shift = !umgebung_t::night_shift;
		buttons[9].pressed ^= 1;
	} else if((buttons+10)==komp) {
		umgebung_t::hide_with_transparency = !umgebung_t::hide_with_transparency;
		buttons[10].pressed ^= 1;
	} else if((buttons+11)==komp) {
		umgebung_t::hide_trees = !umgebung_t::hide_trees;
	} else if((buttons+12)==komp) {
		umgebung_t::hide_buildings = !umgebung_t::hide_buildings;
	} else if((buttons+13)==komp) {
		umgebung_t::hide_buildings = umgebung_t::hide_buildings>1 ? 0 : 2;
	} else if((buttons+14)==komp) {
		umgebung_t::use_transparency_station_coverage = !umgebung_t::use_transparency_station_coverage;
		buttons[14].pressed ^= 1;
	} else if((buttons+15)==komp) {
		umgebung_t::station_coverage_show = umgebung_t::station_coverage_show==0 ? 0xFF : 0;
	} else if((buttons+16)==komp) {
		grund_t::underground_mode = !grund_t::underground_mode;
		for(int y=0; y<welt->gib_groesse_y(); y++) {
			for(int x=0; x<welt->gib_groesse_x(); x++) {
				const planquadrat_t *plan = welt->lookup(koord(x,y));
				const int boden_count = plan->gib_boden_count();
				for(int schicht=0; schicht<boden_count; schicht++) {
					grund_t *gr = plan->gib_boden_bei(schicht);
					gr->calc_bild();
				}
			}
		}
    welt->setze_dirty();
	}
	welt->setze_dirty();
	return true;
}




void color_gui_t::zeichnen(koord pos, koord gr)
{
	const int x = pos.x;
	const int y = pos.y+16;	// compensate for title bar
	const einstellungen_t * sets = welt->gib_einstellungen();
	char buf[128];

	// can be changed also with keys ...
	buttons[11].pressed = umgebung_t::hide_trees;
	buttons[12].pressed = umgebung_t::hide_buildings>0;
	buttons[13].pressed = umgebung_t::hide_buildings>1;
	buttons[15].pressed = umgebung_t::station_coverage_show;
	buttons[16].pressed = grund_t::underground_mode;

	gui_frame_t::zeichnen(pos, gr);

	// seperator
	display_ddd_box_clip(x+10, y+SEPERATE1, RIGHT_WIDTH-20, 0, MN_GREY0, MN_GREY4);
	display_ddd_box_clip(x+10, y+SEPERATE2, RIGHT_WIDTH-20, 0, MN_GREY0, MN_GREY4);
	display_ddd_box_clip(x+10, y+SEPERATE3, RIGHT_WIDTH-20, 0, MN_GREY0, MN_GREY4);

	display_proportional_clip(x+10, y+BRIGHTNESS, translator::translate("1LIGHT_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+NUMBER, y+BRIGHTNESS, ntos(display_get_light(), 0), ALIGN_RIGHT, COL_WHITE, true);

	display_proportional_clip(x+10, y+SCROLL_SPEED, translator::translate("3LIGHT_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+NUMBER, y+SCROLL_SPEED, ntos(abs(sets->gib_scroll_multi()), 0), ALIGN_RIGHT, COL_WHITE, true);

	display_proportional_clip(x+10, y+DENS_TRAFFIC, translator::translate("6WORLD_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+NUMBER, y+DENS_TRAFFIC, ntos(sets->gib_verkehr_level(),0), ALIGN_RIGHT, COL_WHITE, true);

	int len=15+display_proportional_clip(x+10, y+FPS_DATA, translator::translate("Frame time:"), ALIGN_LEFT, COL_BLACK, true);
	sprintf(buf,"%ld ms", get_frame_time() );
	display_proportional_clip(x+len, y+FPS_DATA, buf, ALIGN_LEFT, COL_WHITE, true);

	len = 15+display_proportional_clip(x+10, y+IDLE_DATA, translator::translate("Idle:"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+len, y+IDLE_DATA, ntos(welt->gib_schlaf_zeit(), "%d ms"), ALIGN_LEFT, COL_WHITE, true);

	uint8 farbe;
	uint32 loops;
	loops=welt->gib_realFPS();
	farbe = COL_WHITE;
	uint32 target_fps = welt->is_fast_forward() ? 10 : umgebung_t::fps;
	if(loops<(target_fps*3)/4) {
		farbe = (loops<=target_fps/2) ? COL_RED : COL_YELLOW;
  }
	len = 15+display_proportional_clip(x+10, y+FRAME_DATA, translator::translate("FPS:"), ALIGN_LEFT, COL_BLACK, true);
	sprintf(buf,"%d fps", loops );
	display_proportional_clip(x+len, y+FRAME_DATA, buf, ALIGN_LEFT, farbe, true);

	loops=welt->gib_simloops();
	farbe = COL_WHITE;
	if(loops<=30) {
		farbe = (loops<=20) ? COL_RED : COL_YELLOW;
	}
	len = 15+display_proportional_clip(x+10, y+LOOP_DATA, translator::translate("Sim:"), ALIGN_LEFT, COL_BLACK, true);
	sprintf( buf, "%d%c%d", loops/10, get_fraction_sep(), loops%10 );
	display_proportional_clip(x+len, y+LOOP_DATA, buf, ALIGN_LEFT, farbe, true);
}
