/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Intro and everything else
 */

#include "../simcolor.h"
#include "../simevent.h"
#include "../simimg.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../simwin.h"
#include "../simsys.h"
#include "../simversion.h"
#include "../simgraph.h"
#include "../besch/skin_besch.h"

#include "banner.h"


banner_t::banner_t()
{
	last_ms = dr_time();
	line = 0;
	xoff = (display_get_width()  / 2) - 180;
	yoff = (display_get_height() / 2) - 125;
}



void banner_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class==EVENT_RELEASE  ||  (ev->ev_class==EVENT_KEYBOARD  &&  ev->ev_code!=0)) {
		destroy_win(this);
	}
}



void banner_t::zeichnen(koord /*pos*/, koord)
{
	display_ddd_box(xoff,  yoff, 360, 270, COL_GREY6, COL_GREY2);
	display_fillbox_wh(xoff + 1, yoff + 1, 358, 268, COL_GREY5, true);
	display_ddd_box(xoff + 4, yoff + 4, 352, 262, COL_GREY2, COL_GREY6);
	display_fillbox_wh(xoff + 5, yoff + 5, 350, 260, COL_GREY4, true);

	display_color_img(skinverwaltung_t::logosymbol->get_bild_nr(0), xoff + 264, yoff + 40, 0, false, true);

	// first the fixed part
	// shadow effect by drawing two times with an offset
	for (int s = 1; s >= 0; s--) {
		int heading = (s == 0 ? 7 : COL_BLACK);
		int color   = (s == 0 ? COL_WHITE : COL_BLACK);

<<<<<<< HEAD:gui/banner.cc
		display_proportional(xoff + s + 24+30, yoff + s +  10, "This is an experimental version of Simutrans:", ALIGN_LEFT, heading, true);
		display_proportional(xoff + s + 48+30, yoff + s +  22, "Version " VERSION_NUMBER " "  VERSION_DATE, ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 24+30, yoff + s +  40, "This experimental version", ALIGN_LEFT, heading, true);
		display_proportional(xoff + s + 24+30+155, yoff + s +  40, NARROW_EXPERIMENTAL_VERSION, ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 48+30, yoff + s +  56, "is modified by James E. Petts", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 48+30, yoff + s +  70, "from Simutrans, 1997-2005", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 48+30, yoff + s +  82, "(c) Hj. Malthaner; and", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 48+30, yoff + s +  94, "2005-2009 maintained by", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 24+30, yoff + s + 112, "Markus Pristovsek and the Simutrans team,", ALIGN_LEFT, heading, true);
		display_proportional(xoff + s + 48+30, yoff + s + 128, "released under the Artistic Licence.", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 48+30, yoff + s + 140, "For more information, please", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 24+30, yoff + s + 158, "visit the Simutrans website or forum", ALIGN_LEFT, heading, true);
=======
		display_proportional(xoff + s + 24+30, yoff + s +  10, "This is a beta version of Simutrans:", ALIGN_LEFT, heading, true);
		display_proportional(xoff + s + 48+30, yoff + s +  22, "Version " VERSION_NUMBER " " VERSION_DATE, ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 24+30, yoff + s +  40, "This version is developed by", ALIGN_LEFT, heading, true);
		display_proportional(xoff + s + 48+30, yoff + s +  56, "the simutrans team, based on", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 48+30, yoff + s +  70, "Simutrans 0.84.21.2 by", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 48+30, yoff + s +  82, "Hansjörg Malthaner et al.", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 48+30, yoff + s +  94, "under Artistic Licence.", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 24+30, yoff + s + 112, "Please send ideas and questions to:", ALIGN_LEFT, heading, true);
		display_proportional(xoff + s + 48+30, yoff + s + 128, "Markus Pristovsek", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 48+30, yoff + s + 140, "<team@www.simutrans.com>", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 24+30, yoff + s + 158, "or visit the Simutrans pages on the web:", ALIGN_LEFT, heading, true);
>>>>>>> Simutrans-base/master:gui/banner.cc
		display_proportional(xoff + s + 48+30, yoff + s + 174, "http://www.simutrans.com", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 48+30, yoff + s + 186, "http://forum.simutrans.com", ALIGN_LEFT, color, true);
	}

	// now the scrolling
	static const char* const scrolltext[] = {
#include "../scrolltext.h"
	};

	const int text_line = (line / 9) * 2;
	const int text_offset = line % 9;
	const int left = 60;
	const int top = 196+10;

	display_fillbox_wh(xoff + left, yoff + top, 240, 48, COL_GREY1, true);

	display_proportional(xoff + left +   4, yoff +  1 + top - text_offset, scrolltext[text_line +  0], ALIGN_LEFT,  COL_WHITE, true);
	display_proportional(xoff + left + 236, yoff +  1 + top - text_offset, scrolltext[text_line +  1], ALIGN_RIGHT, COL_WHITE, true);
	display_proportional(xoff + left +   4, yoff + 10 + top - text_offset, scrolltext[text_line +  2], ALIGN_LEFT,  COL_WHITE, true);
	display_proportional(xoff + left + 236, yoff + 10 + top - text_offset, scrolltext[text_line +  3], ALIGN_RIGHT, COL_WHITE, true);
	display_proportional(xoff + left +   4, yoff + 19 + top - text_offset, scrolltext[text_line +  4], ALIGN_LEFT,  COL_GREY6, true);
	display_proportional(xoff + left + 236, yoff + 19 + top - text_offset, scrolltext[text_line +  5], ALIGN_RIGHT, COL_GREY6, true);
	display_proportional(xoff + left +   4, yoff + 28 + top - text_offset, scrolltext[text_line +  6], ALIGN_LEFT,  COL_GREY5, true);
	display_proportional(xoff + left + 236, yoff + 28 + top - text_offset, scrolltext[text_line +  7], ALIGN_RIGHT, COL_GREY5, true);
	display_proportional(xoff + left +   4, yoff + 37 + top - text_offset, scrolltext[text_line +  8], ALIGN_LEFT,  COL_GREY4, true);
	display_proportional(xoff + left + 236, yoff + 37 + top - text_offset, scrolltext[text_line +  9], ALIGN_RIGHT, COL_GREY4, true);
	display_proportional(xoff + left +   4, yoff + 46 + top - text_offset, scrolltext[text_line + 10], ALIGN_LEFT,  COL_GREY3, true);
	display_proportional(xoff + left + 236, yoff + 46 + top - text_offset, scrolltext[text_line + 11], ALIGN_RIGHT, COL_GREY3, true);

	display_fillbox_wh(xoff + left, yoff + top - 8, 240, 7, COL_GREY4, true);
	display_fillbox_wh(xoff + left, yoff + top - 1, 240, 1, COL_GREY3, true);

	display_fillbox_wh(xoff + left, yoff + top + 48, 240, 1, COL_GREY6, true);
	display_fillbox_wh(xoff + left, yoff + top + 49, 240, 7, COL_GREY4, true);

	// scroll on every 70 ms
	if(dr_time()>last_ms+70u) {
		last_ms += 70u;
		line ++;
	}

	if (scrolltext[text_line + 12] == 0) {
		line = 0;
	}
}
