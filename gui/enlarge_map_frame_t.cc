/*
 * Dialogue to increase map size.
 *
 * Gerd Wachsmuth
 *
 * October 2008
 */

#include <string.h>

#include "enlarge_map_frame_t.h"
#include "karte.h"
#include "messagebox.h"

#include "../simdebug.h"
#include "../simio.h"
#include "../simworld.h"
#include "../simwin.h"
#include "../simimg.h"
#include "../simtools.h"
#include "../simskin.h"

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



koord enlarge_map_frame_t::koord_from_rotation( einstellungen_t *sets, sint16 x, sint16 y, sint16 w, sint16 h )
{
	koord offset( sets->get_origin_x(), sets->get_origin_y() );
	switch( sets->get_rotation() ) {
		case 0: return offset+koord(x,y);
		case 1: return offset+koord(y,w-x);
		case 2: return offset+koord(w-x,h-y);
		case 3: return offset+koord(h-y,x);
	}
	assert(sets->get_rotation()<3);
}



enlarge_map_frame_t::enlarge_map_frame_t(spieler_t *spieler, karte_t *welt) :
	gui_frame_t("enlarge map"),
	memory(memory_str),
	xsize(xsize_str,COL_WHITE,gui_label_t::centered),
	ysize(ysize_str,COL_WHITE,gui_label_t::centered)
{
	this->welt = welt;

	sets = new einstellungen_t(*welt->gib_einstellungen()); // Make a copy.
	sets->setze_groesse_x(welt->gib_groesse_x());
	sets->setze_groesse_y(welt->gib_groesse_y());

	changed_number_of_towns = false;
	int intTopOfButton = 24;

	memory.setze_pos( koord(10,intTopOfButton) );
	add_komponente( &memory );
	x_size[0].init( button_t::repeatarrowleft, NULL, koord(LEFT_ARROW,intTopOfButton) );
	x_size[0].add_listener( this );
	add_komponente( x_size+0 );
	xsize.setze_pos( koord((LEFT_ARROW+RIGHT_ARROW+14)/2,intTopOfButton) );
	add_komponente( &xsize );
	x_size[1].init( button_t::repeatarrowright, NULL, koord(RIGHT_ARROW,intTopOfButton) );
	x_size[1].add_listener( this );
	add_komponente( x_size+1 );

	intTopOfButton += 12;

	y_size[0].init( button_t::repeatarrowleft, NULL, koord(LEFT_ARROW,intTopOfButton) );
	y_size[0].add_listener( this );
	add_komponente( y_size+0 );
	ysize.setze_pos( koord((LEFT_ARROW+RIGHT_ARROW+14)/2,intTopOfButton) );
	add_komponente( &ysize );
	y_size[1].init( button_t::repeatarrowright, NULL, koord(RIGHT_ARROW,intTopOfButton) );
	y_size[1].add_listener( this );
	add_komponente( y_size+1 );

	// city stuff
	intTopOfButton = 64+10;
	number_of_towns[0].init( button_t::repeatarrowleft, NULL, koord(LEFT_WIDE_ARROW,intTopOfButton) );
	number_of_towns[0].add_listener( this );
	add_komponente( number_of_towns+0 );
	number_of_towns[1].init( button_t::repeatarrowright, NULL, koord(RIGHT_WIDE_ARROW,intTopOfButton) );
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
	intTopOfButton += 12+5;

	// start game
	intTopOfButton += 5;
	start_button.init( button_t::roundbox, "enlarge map", koord(10, intTopOfButton), koord(240, 14) );
	start_button.add_listener( this );
	add_komponente( &start_button );

	setze_fenstergroesse( koord(260, intTopOfButton+14+8+16) );

	update_preview();
}



/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool enlarge_map_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp==x_size+0) {
		if(sets->gib_groesse_x() > max(512,this->welt->gib_groesse_x()) ) {
			sets->setze_groesse_x( (sets->gib_groesse_x()-1)&0x1F80 );
			update_preview();
		} else if(sets->gib_groesse_x() > max(64,this->welt->gib_groesse_x()) ) {
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
		if(sets->gib_groesse_y() > max(512,this->welt->gib_groesse_y()) ) {
			sets->setze_groesse_y( (sets->gib_groesse_y()-1)&0x1F80 );
			update_preview();
		} else if(sets->gib_groesse_y() > max(64,this->welt->gib_groesse_y()) ) {
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
			changed_number_of_towns = true;
		}
	}
	else if(komp==number_of_towns+1) {
		sets->setze_anzahl_staedte( sets->gib_anzahl_staedte() + 1 );
		changed_number_of_towns = true;
	}
	else if(komp==town_size+0) {
		if(sets->gib_mittlere_einwohnerzahl()>100 ) {
			sets->setze_mittlere_einwohnerzahl( sets->gib_mittlere_einwohnerzahl() - 100 );
		}
	}
	else if(komp==town_size+1) {
		sets->setze_mittlere_einwohnerzahl( sets->gib_mittlere_einwohnerzahl() + 100 );
	}
	else if(komp==&start_button) {
			destroy_win(this);
			news_img* info_win = new news_img("Vergroessere die Karte\n", skinverwaltung_t::neueweltsymbol->gib_bild_nr(0));
			create_win(200, 100, info_win, w_info, magic_none);
			intr_refresh_display(true);
			welt->enlarge_map(sets, NULL);
			destroy_win( info_win );
	}
	else {
		return false;
	}
	return true;
}



void enlarge_map_frame_t::zeichnen(koord pos, koord gr)
{
	while(  welt->gib_einstellungen()->get_rotation() != sets->get_rotation()  ) {
		// map was rotated while we are active ... => rotate too!
		sets->rotate90();
		sets->setze_groesse( sets->gib_groesse_y(), sets->gib_groesse_x() );
		update_preview();
	}

	gui_frame_t::zeichnen(pos, gr);

	int x = pos.x+10;
	int y = pos.y+4+16;

	display_ddd_box_clip(x+173, y, preview_size+2, preview_size+2, MN_GREY0,MN_GREY4);
	display_array_wh(x+174, y+1, preview_size, preview_size, karte);

	y = pos.y+64+10+16;
	display_proportional_clip(x, y, translator::translate("5WORLD_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(sets->gib_anzahl_staedte(), 0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Median Citizen per town"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_WIDE_RIGHT, y, ntos(sets->gib_mittlere_einwohnerzahl(),0),ALIGN_RIGHT, COL_WHITE, true);
	y += 12+5;

	display_ddd_box_clip(x, y, 240, 0, MN_GREY0, MN_GREY4);
}



/**
 * Berechnet Preview-Karte neu. Inititialisiert RNG neu!
 * @author Hj. Malthaner
 */
void
enlarge_map_frame_t::update_preview()
{
	// reset noise seed
	setsimrand( 0xFFFFFFFF, welt->gib_einstellungen()->gib_karte_nummer() );

	const int mx = sets->gib_groesse_x()/preview_size;
	const int my = sets->gib_groesse_y()/preview_size;

	// "welt" still nows the old size. The new size is saved in "sets".
	sint16 old_x = welt->gib_groesse_x();
	sint16 old_y = welt->gib_groesse_y();

	for(  int j=0;  j<preview_size;  j++  ) {
		for(  int i=0;  i<preview_size;  i++  ) {
			COLOR_VAL color;
			koord pos(i*mx,j*my);

			if(  pos.x<=old_x  &&  pos.y<=old_y  ){
				if(  i==(old_x/mx)  ||  j==(old_y/my)  ){
					// border
					color = COL_WHITE;
				}
				else {
					const sint16 height = welt->lookup_hgt( pos )*Z_TILE_STEP;
					color = reliefkarte_t::calc_hoehe_farbe(height, sets->gib_grundwasser()/Z_TILE_STEP);
				}
			}
			else {
				// new part
				const sint16 height = karte_t::perlin_hoehe(sets, pos, koord(old_x,old_y) );
				color = reliefkarte_t::calc_hoehe_farbe(height*Z_TILE_STEP, sets->gib_grundwasser()/Z_TILE_STEP);
			}
			karte[j*preview_size+i] = color;
		}
	}
	sets->heightfield = "";

	x_size[0].enable();
	x_size[1].enable();
	y_size[0].enable();
	y_size[1].enable();

	if(!changed_number_of_towns){// Interpolate number of towns.
		sint32 new_area = sets->gib_groesse_x() * sets->gib_groesse_y();
		sint32 old_area = old_x * old_y;
		sint32 towns = welt->gib_einstellungen()->gib_anzahl_staedte();
		sets->setze_anzahl_staedte( towns * new_area / old_area - towns );
	}

	sprintf( xsize_str, "%i", sets->gib_groesse_x() );
	sprintf( ysize_str, "%i", sets->gib_groesse_y() );

	// geuss the new memory needed
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
	sprintf(memory_str, translator::translate("3WORLD_CHOOSE"), memory);

}
