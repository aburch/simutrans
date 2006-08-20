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
#include "../simtime.h"
#include "../simcolor.h"
#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../simgraph.h"
#include "../simdisplay.h"

#include "../utils/simstring.h"

#define DAY_NIGHT (10)
#define BRIGHTNESS (23)

#define SCROLL_INVERS (39)
#define SCROLL_SPEED (52)

#define CITY_WALKER (68)
#define STOP_WALKER (81)
#define DENS_TRAFFIC (94)

#define SEPERATE (110)
#define FPS_DATA (116)
#define IDLE_DATA (127)
#define FRAME_DATA (138)
#define LOOP_DATA (149)

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

	for(int i=0;  i<10;  i++ ) {
		buttons[i].add_listener(this);
		add_komponente( buttons+i );
	}

	setze_opaque(true);
	setze_fenstergroesse( koord(176, 180) );
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

	gui_frame_t::zeichnen(pos, gr);

	display_ddd_box_clip(x+10, y+SEPERATE, 156, 0, MN_GREY0, MN_GREY4);

	display_proportional_clip(x+10, y+BRIGHTNESS, translator::translate("1LIGHT_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+NUMBER, y+BRIGHTNESS, ntos(display_get_light(), 0), ALIGN_RIGHT, COL_WHITE, true);

	display_proportional_clip(x+10, y+SCROLL_SPEED, translator::translate("3LIGHT_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+NUMBER, y+SCROLL_SPEED, ntos(abs(sets->gib_scroll_multi()), 0), ALIGN_RIGHT, COL_WHITE, true);

	display_proportional_clip(x+10, y+DENS_TRAFFIC, translator::translate("6WORLD_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+NUMBER, y+DENS_TRAFFIC, ntos(sets->gib_verkehr_level(),0), ALIGN_RIGHT, COL_WHITE, true);

	int len=15+display_proportional_clip(x+10, y+FPS_DATA, translator::translate("Frame time:"), ALIGN_LEFT, COL_BLACK, true);
	sprintf(buf,"%d ms/%ld ms", 1000/max(1,welt->gib_realFPS()), get_frame_time() );
	display_proportional_clip(x+len, y+FPS_DATA, buf, ALIGN_LEFT, COL_WHITE, true);

	len = 15+display_proportional_clip(x+10, y+IDLE_DATA, translator::translate("Idle:"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+len, y+IDLE_DATA, ntos(welt->gib_schlaf_zeit(), "%d ms"), ALIGN_LEFT, COL_WHITE, true);

	uint8 farbe;
	int loops;
	loops=welt->gib_FPS();
	farbe = COL_WHITE;
	if(loops<=10) {
	   	farbe = (loops<=7) ? COL_RED : COL_YELLOW;
   	}
	len = 15+display_proportional_clip(x+10, y+FRAME_DATA, translator::translate("FPS:"), ALIGN_LEFT, COL_BLACK, true);
	sprintf(buf,"%d fps (real: %d)", loops, welt->gib_realFPS() );
	display_proportional_clip(x+len, y+FRAME_DATA, buf, ALIGN_LEFT, farbe, true);

	loops=welt->gib_simloops();
	farbe = COL_WHITE;
	if(loops<=5) {
   		farbe = (loops<2) ? COL_RED : COL_YELLOW;
   	}
	len = 15+display_proportional_clip(x+10, y+LOOP_DATA, translator::translate("Sim:"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+len, y+LOOP_DATA, ntos(loops, "%d loops"), ALIGN_LEFT, farbe, true);
}
