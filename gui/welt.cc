/*
 * welt.cc
 *
 * dialog zur Eingabe der Werte fuer die Kartenerzeugung
 *
 * Hj. Malthaner
 *
 * April 2000
 */

#include "welt.h"
#include "karte.h"

#include "../simdebug.h"
#include "../simworld.h"
#include "../simwin.h"
#include "../simimg.h"
#include "../simplay.h"
#include "../simtools.h"
#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../simcolor.h"

#include "../simgraph.h"
#include "../simdisplay.h"


welt_gui_t::welt_gui_t(karte_t *welt, einstellungen_t *sets)
 : infowin_t(welt), buttons(35)
{
    this->sets = sets;
    load_heightfield = false;
    load = start = close = quit = false;

    // init button_defs
    button_t button_def;
    int i;
    int intTopOfButton=47;
    int intLeftOfButton=113;

	button_def.setze_pos( koord(200-24,intTopOfButton) );
	button_def.setze_typ( button_t::arrowleft );
	buttons.append(button_def);

	button_def.setze_pos( koord(250-24,intTopOfButton) );
	button_def.setze_typ(button_t::arrowright);
	buttons.append(button_def);

    button_def.setze_pos( koord(10, intTopOfButton) );
    button_def.setze_typ(button_t::square);
    buttons.append(button_def);
    intTopOfButton += 12+5;

    for (i=3; i<27; i+=2) {
		button_def.setze_pos( koord(intLeftOfButton,intTopOfButton) );
		button_def.setze_typ( button_t::arrowleft );
		buttons.append(button_def);

		button_def.setze_pos( koord((i%25)?intLeftOfButton+37:intLeftOfButton+57,intTopOfButton) );
		button_def.setze_typ(button_t::arrowright);
		buttons.append(button_def);
		intTopOfButton += 12;

		switch(i)
		{
			case 7 :
				intTopOfButton += 5;
			break;
			case 9 :
				intTopOfButton += 5;
			break;
			case 17:
				intTopOfButton += 5;
				intLeftOfButton += 40;
			break;
			case 23:
				intTopOfButton += 5;
			break;
		}
    }


    intTopOfButton += 10;

    // 28-Oct-2001 Markus Weber  added Button

    // Random map
    //----------------------
    button_def.setze_pos( koord(10, intTopOfButton) );
    button_def.setze_groesse( koord(104, 14) );
    button_def.setze_typ(button_t::roundbox);
    buttons.append(button_def);
    intTopOfButton += 23;

    // single line switch button
    //----------------------

    button_def.setze_pos( koord(10, intTopOfButton) );
    button_def.setze_typ(button_t::square);
    buttons.append(button_def);
    intTopOfButton += 12;

    button_def.setze_pos( koord(10, intTopOfButton) );
    button_def.setze_typ(button_t::square);
    buttons.append(button_def);
    intTopOfButton += 12;

    button_def.setze_pos( koord(10, intTopOfButton) );
    button_def.setze_typ(button_t::square);
    buttons.append(button_def);
    intTopOfButton += 28 ;


    // Load game
    //----------------------
    button_def.setze_pos( koord(10, intTopOfButton) );
    button_def.setze_groesse( koord(104, 14) );
    button_def.setze_typ(button_t::roundbox);
    buttons.append(button_def);
    intTopOfButton += 18;

    // Start game
    //----------------------

    button_def.setze_pos( koord(10, intTopOfButton) );
    button_def.setze_groesse( koord(104, 14) );
    button_def.setze_typ(button_t::roundbox);
    buttons.append(button_def);


    // Quit
    //----------------------

    button_def.setze_pos( koord(104+11+20, intTopOfButton) );
    button_def.setze_groesse( koord(104, 14) );
    button_def.setze_typ(button_t::roundbox);
    buttons.append(button_def);

    // Load relief
    //----------------------

    button_def.setze_pos( koord(104+11+20, intTopOfButton-18) );
    button_def.setze_groesse( koord(104, 14) );
    button_def.setze_typ(button_t::roundbox);
    buttons.append(button_def);

    update_preview();
}

welt_gui_t::~welt_gui_t()
{
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

	unsigned long old_seed = setsimrand( 0xFFFFFFFF, sets->gib_karte_nummer() );

	for(int j=0; j<preview_size; j++) {
		for(int i=0; i<preview_size; i++) {
			karte[j*preview_size+i] = reliefkarte_t::calc_hoehe_farbe(karte_t::perlin_hoehe(i*mx, j*my, sets->gib_map_roughness(), sets->gib_max_mountain_height())+16, sets->gib_grundwasser());
		}
	}
//	setsimrand( 0xFFFFFFFF, old_seed );
}



const char *
welt_gui_t::gib_name() const
{
    return "Neue Welt";
}

int welt_gui_t::gib_bild() const
{
    return IMG_LEER;
}

koord welt_gui_t::gib_bild_offset() const {  return koord(-12,16); }


void welt_gui_t::info(cbuffer_t &) const
{
}


koord welt_gui_t::gib_fenstergroesse() const
{
    return koord(250, 304+48);
}

void welt_gui_t::infowin_event(const event_t *ev)
{
    infowin_t::infowin_event(ev);

    if(IS_LEFTCLICK(ev) || IS_LEFTREPEAT(ev)) {
	if(buttons.at(0).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_karte_nummer() > -2500 ) {
		sets->setze_starting_year( sets->gib_starting_year() - 1 );
		update_preview();
	    }
	} else if(buttons.at(1).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_karte_nummer() <2500 ) {
	      sets->setze_starting_year( sets->gib_starting_year() + 1 );
	      update_preview();
	   }
	} else if(buttons.at(2).getroffen(ev->cx, ev->cy)) {
	      sets->setze_use_timeline( !sets->gib_use_timeline() );

	} else if(buttons.at(3).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_karte_nummer() > 0 ) {
		sets->setze_karte_nummer( sets->gib_karte_nummer() - 1 );
		update_preview();
	    }
	} else if(buttons.at(4).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_karte_nummer() <9999 ) {
	      sets->setze_karte_nummer( sets->gib_karte_nummer() + 1 );
	      update_preview();
	   }

	} else if(buttons.at(5).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_groesse_x() > 64 ) {
		sets->setze_groesse_x( sets->gib_groesse_x() - 64 );
		update_preview();
	    }

	} else if(buttons.at(6).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_groesse_x() < 4096 ) {
		sets->setze_groesse_x( sets->gib_groesse_x() + 64 );
		update_preview();
	    }
	} else if(buttons.at(7).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_groesse_y() > 64 ) {
		sets->setze_groesse_y( sets->gib_groesse_y() - 64 );
		update_preview();
	    }

	} else if(buttons.at(8).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_groesse_y() < 4096 ) {
		sets->setze_groesse_y( sets->gib_groesse_y() + 64 );
		update_preview();
	    }

	} else if(buttons.at(9).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_anzahl_staedte() > 0 ) {
		sets->setze_anzahl_staedte( sets->gib_anzahl_staedte() - 1 );
	    }
	} else if(buttons.at(10).getroffen(ev->cx, ev->cy)) {
	    // die Anzahl der städte is auf 64 begrenzt, da die stadtnummern
	    // modulo 64 gerechnet werden beim step der staedte
	    if(sets->gib_anzahl_staedte() < 64 ) {
		sets->setze_anzahl_staedte( sets->gib_anzahl_staedte() + 1 );
	    }
	} else if(buttons.at(11).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_verkehr_level() < 16 ) {
		sets->setze_verkehr_level( sets->gib_verkehr_level() + 1 );
	    }

	} else if(buttons.at(12).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_verkehr_level() > 0 ) {
		sets->setze_verkehr_level( sets->gib_verkehr_level() - 1 );
	    }

            // Button 'Grundwasser'                                            // 25-Nov-01        Markus Weber    Added
  	} else if(buttons.at(13).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_grundwasser() > -160 ) {
		sets->setze_grundwasser( sets->gib_grundwasser() - 32 );
                update_preview();
	    }

	} else if(buttons.at(14).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_grundwasser() < 0 ) {
		sets->setze_grundwasser( sets->gib_grundwasser() + 32 );
                update_preview();
	    }


            // Button 'Max. Mountain height'                                             // 01-Dec-01        Markus Weber    Added
  	} else if(buttons.at(15).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_max_mountain_height() > 0.0 ) {
		sets->setze_max_mountain_height( sets->gib_max_mountain_height() - 10 );
                update_preview();
	    }

   	} else if(buttons.at(16).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_max_mountain_height() < 320.0 ) {
		sets->setze_max_mountain_height( sets->gib_max_mountain_height() + 10 );
                update_preview();
	    }


                // Button 'Map roughness'                                             // 01-Dec-01        Markus Weber    Added

          // values of 0.5 .. 0.7 seem to be ok, less is boring flat, more is too crumbled

  	} else if(buttons.at(17).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_map_roughness() > 0.4 ) {
		sets->setze_map_roughness( sets->gib_map_roughness() - 0.1 );
                update_preview();
	    }

   	} else if(buttons.at(18).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_map_roughness() < 0.7 ) {
		sets->setze_map_roughness( sets->gib_map_roughness() + 0.1 );
                update_preview();
	    }

	// finer industrie and tourist settings
	} else if(buttons.at(19).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_land_industry_chains() > 0) {
		sets->setze_land_industry_chains( sets->gib_land_industry_chains() -1 );
	    }
	} else if(buttons.at(20).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_land_industry_chains() < 100) {
		sets->setze_land_industry_chains( sets->gib_land_industry_chains() + 1 );
	    }
	} else if(buttons.at(21).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_city_industry_chains() > 0) {
		sets->setze_city_industry_chains( sets->gib_city_industry_chains() - 1 );
	    }
	} else if(buttons.at(22).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_city_industry_chains() < 100 ) {
		sets->setze_city_industry_chains( sets->gib_city_industry_chains() + 1 );
	    }
	} else if(buttons.at(23).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_tourist_attractions() > 0) {
		sets->setze_tourist_attractions( sets->gib_tourist_attractions() - 1 );
	    }
	} else if(buttons.at(24).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_tourist_attractions() < 500) {
		sets->setze_tourist_attractions( sets->gib_tourist_attractions() + 1 );
	    }
	} else if(buttons.at(25).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_mittlere_einwohnerzahl() > 0) {
		sets->setze_mittlere_einwohnerzahl( sets->gib_mittlere_einwohnerzahl() - 25 );
	    }
	} else if(buttons.at(26).getroffen(ev->cx, ev->cy)) {
	    if(sets->gib_mittlere_einwohnerzahl() < 15000) {
		sets->setze_mittlere_einwohnerzahl( sets->gib_mittlere_einwohnerzahl() + 25 );
	    }

	    // Button 'Random map'                                                      // 28-Oct-2001 Markus Weber Added
	} else if(buttons.at(27).getroffen(ev->cx, ev->cy)) {
		sets->setze_karte_nummer(simrand(9999));
		update_preview();


	} else if(buttons.at(28).getroffen(ev->cx, ev->cy)) {
	    sets->setze_show_pax( !sets->gib_show_pax() );

	} else if(buttons.at(29).getroffen(ev->cx, ev->cy)) {
            umgebung_t::night_shift = !umgebung_t::night_shift;
	} else if(buttons.at(30).getroffen(ev->cx, ev->cy)) {
	      sets->setze_allow_player_change( !sets->gib_allow_player_change() );

	}  else if(buttons.at(31).getroffen(ev->mx, ev->my)) {
	    if (IS_LEFTCLICK(ev)) {
		load = true;
	    }
	}  else if(buttons.at(32).getroffen(ev->mx, ev->my)) {
	    if (IS_LEFTCLICK(ev)) {
		start = true;
	    }
	}  else if(buttons.at(33).getroffen(ev->mx, ev->my)) {
	    if (IS_LEFTCLICK(ev)) {
		quit = true;
	    }
	} else if(buttons.at(34).getroffen(ev->mx, ev->my)) {
	    if (IS_LEFTCLICK(ev)) {
		load_heightfield = true;
	    }
	}
    }
    else if(ev->ev_class == INFOWIN &&
	    ev->ev_code == WIN_CLOSE) {
	close = true;
    }
}


/**
 *
 * @author Hj. Malthaner
 * @return einen Vector von Buttons fuer das Beobachtungsfenster
 */
vector_tpl<button_t>*
welt_gui_t::gib_fensterbuttons()
{
	// enforce timeline
    buttons.at(2).pressed = sets->gib_use_timeline();
    buttons.at(2).text = translator::translate("Use timeline start year"),

      //28-Oct-2001 Markus Weber added button
    buttons.at(27).text = translator::translate("Random map");

    // setze variable anteile der Buttons
    buttons.at(28).pressed = sets->gib_show_pax();
    buttons.at(28).text = translator::translate("7WORLD_CHOOSE"),

    buttons.at(29).pressed = umgebung_t::night_shift;
    buttons.at(29).text = translator::translate("8WORLD_CHOOSE"),

    buttons.at(30).pressed = sets->gib_allow_player_change();
    buttons.at(30).text = translator::translate("Allow player change"),

    buttons.at(31).text = translator::translate("Lade Spiel");
    buttons.at(32).text = translator::translate("Starte Spiel");
    buttons.at(33).text = translator::translate("Beenden");
    buttons.at(34).text = translator::translate("Lade Relief");

    return &buttons;
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
  const int x = pos.x;
  const int y = pos.y;

  char buf[256];

  infowin_t::zeichnen(pos, gr);

  display_ddd_box_clip(x+10, y+40, 230, 0, MN_GREY0, MN_GREY4);
  display_ddd_box_clip(x+10, y+307, 230, 0, MN_GREY0, MN_GREY4);

  display_proportional_clip(x+10, y+22, translator::translate("1WORLD_CHOOSE"),ALIGN_LEFT, SCHWARZ, true);

  int yo = y+60+4;

  display_proportional_clip(x+200, yo-17, ntos(sets->gib_starting_year(), "%3d"), ALIGN_MIDDLE, WEISS, true);

  display_proportional_clip(x+10, yo, translator::translate("2WORLD_CHOOSE"),
		       ALIGN_LEFT, SCHWARZ, true);
  display_proportional_clip(x+137, yo, ntos(sets->gib_karte_nummer(), "%3d"),
		       ALIGN_MIDDLE, WEISS, true);


  const int x2 = sets->gib_groesse_x() * sets->gib_groesse_y();
  const int memory = 12 + x2/8192;

  sprintf(buf, translator::translate("3WORLD_CHOOSE"), memory);

  display_proportional_clip(x+10, yo+12, buf,
		       ALIGN_LEFT, SCHWARZ, true);

  display_proportional_clip(x+137, yo+12, ntos(sets->gib_groesse_x(), "%3d"),
		       ALIGN_MIDDLE, WEISS, true);
  display_proportional_clip(x+137, yo+24, ntos(sets->gib_groesse_y(), "%3d"),
		       ALIGN_MIDDLE, WEISS, true);

  display_proportional_clip(x+10, yo+41, translator::translate("5WORLD_CHOOSE"),
		       ALIGN_LEFT, SCHWARZ, true);
  display_proportional_clip(x+137, yo+41, ntos(sets->gib_anzahl_staedte(), "%3d"),
			 ALIGN_MIDDLE, WEISS, true);
  display_proportional_clip(x+10, yo+58, translator::translate("6WORLD_CHOOSE"),
		       ALIGN_LEFT, SCHWARZ, true);
  display_proportional_clip(x+137, yo+58, ntos(16 - sets->gib_verkehr_level(),"%3d"),
			 ALIGN_MIDDLE, WEISS, true);


        // water level       18-Nov-01       Markus W. Added
  display_proportional_clip(x+10, yo+70, translator::translate("Water level"),
		       ALIGN_LEFT, SCHWARZ, true);
  display_proportional_clip(x+137, yo+70, ntos(sets->gib_grundwasser()/32+5,"%3d"),
  			ALIGN_MIDDLE, WEISS, true);


        // Height of mountains       29-Nov-01       Markus W. Added
  display_proportional_clip(x+10, yo+82, translator::translate("Mountain height"),
		       ALIGN_LEFT, SCHWARZ, true);
  display_proportional_clip(x+137, yo+82, ntos((int)(sets->gib_max_mountain_height()),
"%3d"), ALIGN_MIDDLE, WEISS, true);


        // roughness of mountains       29-Nov-01       Markus W. Added
  display_proportional_clip(x+10, yo+94, translator::translate("Map roughness"),
		       ALIGN_LEFT, SCHWARZ, true);

  display_proportional_clip(x+137, yo+94, ntos((int)(sets->gib_map_roughness()*10.0 + 0.5)-4 ,
"%3d") , ALIGN_MIDDLE, WEISS, true);     // x = round(roughness * 10)-4  // 0.6 * 10 - 4 = 2    //29-Nov-01     Markus W. Added


  display_proportional_clip(x+10, yo+111, translator::translate("Land industries"),
		       ALIGN_LEFT, SCHWARZ, true);
  display_proportional_clip(x+137+40, yo+111, ntos(sets->gib_land_industry_chains(),"%3d"),
  			 ALIGN_MIDDLE, WEISS, true);
  display_proportional_clip(x+10, yo+123, translator::translate("City industries"),
		       ALIGN_LEFT, SCHWARZ, true);
  display_proportional_clip(x+137+40, yo+123, ntos(sets->gib_city_industry_chains(),"%3d"),
  			 ALIGN_MIDDLE, WEISS, true);
  display_proportional_clip(x+10, yo+135, translator::translate("Tourist attractions"),
		       ALIGN_LEFT, SCHWARZ, true);
  display_proportional_clip(x+137+40, yo+135, ntos(sets->gib_tourist_attractions(),"%3d"),
  			 ALIGN_MIDDLE, WEISS, true);
  display_proportional_clip(x+10, yo+152, translator::translate("Median Citizen per town"),
		       ALIGN_LEFT, SCHWARZ, true);
  display_proportional_clip(x+137+50, yo+152, ntos(sets->gib_mittlere_einwohnerzahl(),"%3d"),
  			 ALIGN_MIDDLE, WEISS, true);

  display_ddd_box_clip(x+173, yo+2, preview_size+2, preview_size+2, MN_GREY0,MN_GREY4);
  display_array_wh(x+174, yo+3, preview_size, preview_size, karte);
}
