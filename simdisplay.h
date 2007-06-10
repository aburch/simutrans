/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * Benutzt simgraph.c um an Simutrans angepasste Zeichenfunktionen
 * anzubieten.
 *
 * Hj. Malthaner, Jan. 2001
 */

#ifndef simdisplay_h
#define simdisplay_h

#ifdef __cplusplus
extern "C" {
#endif

void display_set_progress_text(const char *text);
void display_progress(int part, int total);
void display_icon_leiste(const int color, int basi_bild_nr);
//void display_divider(int x, int y, int w);

/**
 * Kopiert Puffer ins Fenster
 * @author Hj. Malthaner
 */
void display_flush(int season_img, double konto, const char* day_str, const char* info, const char* player_name, COLOR_VAL player_color);

#ifdef __cplusplus
}
#endif

#endif
