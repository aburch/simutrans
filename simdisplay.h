/*
 * simdisplay.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * simdisplay.h
 *
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

void display_progress(int part, int total);
void display_icon_leiste(const int color, int basi_bild_nr);
//void display_divider(int x, int y, int w);

/**
 * Kopiert Puffer ins Fenster
 * @author Hj. Malthaner
 */
 void display_flush(int stunden4, int color, double konto, const char *day_str, const char *info, const char *player_name, const int player_color);

#ifdef __cplusplus
}
#endif


#endif
