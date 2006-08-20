/*
 * simdisplay.c
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * simdisplay.c
 *
 * Benutzt simgraph.c um an Simutrans angepasste Zeichenfunktionen
 * anzubieten.
 *
 * Hj. Malthaner, Jan. 2001
 */

#include <stdio.h>

#include "simgraph.h"
#include "simdisplay.h"
#include "simcolor.h"
#include "utils/simstring.h"


/**
 * Zeichnet eine Fortschrittsanzeige
 * @author Hj. Malthaner
 */
void
display_progress(int part, int total)
{
    extern int disp_width;
    extern int disp_height;

    // umriß
    display_ddd_box((disp_width-total-4)/2, disp_height/2-9, total+3, 18, GRAU6, GRAU4);
    display_ddd_box((disp_width-total-2)/2, disp_height/2-8, total+1, 16, GRAU4, GRAU6);

    // flaeche
    display_fillbox_wh((disp_width-total)/2, disp_height/2-7, total, 14, GRAU5, TRUE);

    // progress
    display_fillbox_wh((disp_width-total)/2, disp_height/2-5, part, 10, BLAU, TRUE);
}


/**
 * Zeichnet die Iconleiste
 * @author Hj. Malthaner
 */
void
display_icon_leiste(const int color, int basis_bild)
{
    static int old_color = -1;
    int dirty;
    extern int disp_width;
    extern int disp_height;

    if(color != old_color
#ifdef USE_SOFTPOINTER
       || old_my <= 32
#endif
       ) {
        dirty = TRUE;
	old_color = color;
    } else {
	dirty = FALSE;
    }

    display_fillbox_wh(0,0, disp_width, 32, MN_GREY1, dirty);

    display_color_img(basis_bild++,0,0, color, FALSE, dirty);
    display_color_img(basis_bild++,64,0, color, FALSE, dirty);
    display_color_img(basis_bild++,128,0, color, FALSE, dirty);
    display_color_img(basis_bild++,192,0, color, FALSE, dirty);
    display_color_img(basis_bild++,256,0, color, FALSE, dirty);
    display_color_img(basis_bild++,320,0, color, FALSE, dirty);

    //display_color_img(157,320,0, color, FALSE, dirty);
    display_color_img(basis_bild++,384,0, color, FALSE, dirty);
    display_color_img(basis_bild++,448,0, color, FALSE, dirty);
    display_color_img(basis_bild++,512,0, color, FALSE, dirty);
    display_color_img(basis_bild++,576,0, color, FALSE, dirty);
	// added for extended menus
    display_color_img(basis_bild++,640,0, color, FALSE, dirty);

    /*
    display_img(basis_bild++,0,0, FALSE, dirty);
    display_img(basis_bild++,64,0, FALSE, dirty);
    display_img(basis_bild++,128,0, FALSE, dirty);
    display_img(basis_bild++,192,0, FALSE, dirty);
    display_img(basis_bild++,256,0, FALSE, dirty);
    display_img(basis_bild++,320,0, FALSE, dirty);

    //display_img(157,320,0, FALSE, dirty);
    display_img(basis_bild++,384,0, FALSE, dirty);
    display_img(basis_bild++,448,0, FALSE, dirty);
    display_img(basis_bild++,512,0, FALSE, dirty);
    display_img(basis_bild++,576,0, FALSE, dirty);
    */
}


#if 0
/**
 * Zeichnet eine Trennlinie
 * @author Niels Roest
 */
void display_divider(int x, int y, int w)
{
  display_fillbox_wh(x,y   ,w,1, MN_GREY4, FALSE);
  display_fillbox_wh(x,y+1 ,w,1, MN_GREY0, FALSE);
}
#endif


/**
 * Kopiert Puffer ins Fenster
 * @author Hj. Malthaner
 */
void
display_flush(int stunden4, int color, double konto, const char *day_str, const char *info, const char *player_name, const int player_color)
{
    char buffer[256];
    extern int disp_width;
    extern int disp_height;

    display_fillbox_wh(0, disp_height-16, disp_width, 1, MN_GREY4, FALSE);
    display_fillbox_wh(0, disp_height-15, disp_width, 15, MN_GREY1, FALSE);

    sprintf(buffer,"%s %2d:%02dh", day_str, stunden4 >> 2, (stunden4 & 3)*15);
    display_proportional(24, disp_height-12, buffer, ALIGN_LEFT, SCHWARZ, TRUE);
    money_to_string(buffer, konto);

    if(konto >= 0.0) {
	display_proportional(344, disp_height-12, buffer, ALIGN_LEFT, SCHWARZ, TRUE);
    } else {
	display_proportional(344, disp_height-12, buffer, ALIGN_LEFT, DUNKELROT, TRUE);
    }

    display_proportional(480, disp_height-12, info, ALIGN_LEFT, BLACK, TRUE);

	if(player_name!=NULL) {
		display_proportional(255, disp_height-12, player_name, ALIGN_MIDDLE, player_color, TRUE);
	}

    display_flush_buffer();
}
