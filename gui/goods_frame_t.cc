/*
 * goods_frame_t.cpp
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include "goods_frame_t.h"
#include "components/gui_scrollpane.h"
#include "components/list_button.h"

#include "../bauer/warenbauer.h"
#include "../besch/ware_besch.h"
#include "../dataobj/translator.h"

#include "../simcolor.h"
#include "../simworld.h"
#include "../boden/wege/weg.h"

/**
 * This variable defines the current speed for bonus calculation
 * @author prissi
 */
int goods_frame_t::relative_speed_change=100;

/**
 * This variable defines by which column the table is sorted
 * Values: 0 = Unsorted (passengers and mail first)
 *         1 = Alphabetical
 *         2 = Revenue
 * @author prissi
 */
goods_frame_t::sort_mode_t goods_frame_t::sortby = unsortiert;

/**
 * This variable defines the sort order (ascending or descending)
 * Values: 1 = ascending, 2 = descending)
 * @author Markus Weber
 */
bool goods_frame_t::sortreverse = false;

const char *goods_frame_t::sort_text[SORT_MODES] = {
    "gl_btn_unsort",
    "gl_btn_sort_name",
    "gl_btn_sort_revenue",
    "gl_btn_sort_bonus"
};



goods_frame_t::goods_frame_t(karte_t *wl) :
	gui_frame_t("Goods list"),
	sort_label(translator::translate("hl_txt_sort")),
	change_speed_label(speed_bonus,COL_WHITE,gui_label_t::right),
	goods_stats(wl),
	scrolly(&goods_stats)
{
	this->welt = wl;
	int y=BUTTON_HEIGHT+4-16;

	speed_bonus[0] = 0;
	change_speed_label.setze_pos(koord(BUTTON4_X+5, y));
	add_komponente(&change_speed_label);

	speed_down.init(button_t::repeatarrowleft, "", koord(BUTTON4_X-20, y), koord(10,BUTTON_HEIGHT));
	speed_down.add_listener(this);
	add_komponente(&speed_down);

	speed_up.init(button_t::repeatarrowright, "", koord(BUTTON4_X+10, y), koord(10,BUTTON_HEIGHT));
	speed_up.add_listener(this);
	add_komponente(&speed_up);

	y=4+3*LINESPACE+4;

	sort_label.setze_pos(koord(BUTTON1_X, y));
	add_komponente(&sort_label);

	y += LINESPACE;

	sortedby.init(button_t::roundbox, "", koord(BUTTON1_X, y), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sortedby.add_listener(this);
	add_komponente(&sortedby);

	sorteddir.init(button_t::roundbox, "", koord(BUTTON2_X, y), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sorteddir.add_listener(this);
	add_komponente(&sorteddir);

	y += BUTTON_HEIGHT+2;

	scrolly.setze_pos(koord(1, y));
	scrolly.set_show_scroll_x(false);
	scrolly.setze_groesse(koord(TOTAL_WIDTH-16, 191+16+16-y));
	add_komponente(&scrolly);

	setze_opaque(true);
	int h = (warenbauer_t::gib_waren_anzahl()-1)*LINESPACE+y;
	if(h>450) {
		h = y+10*LINESPACE+2;
	}
	setze_fenstergroesse(koord(TOTAL_WIDTH, h));
	set_min_windowsize(koord(TOTAL_WIDTH,y+4*LINESPACE+2));
	set_resizemode(vertical_resize);

	sort_list();

	resize (koord(0,0));
}




/**
* @author Markus Weber
*/
int goods_frame_t::compare_goods(const void *p1, const void *p2)
{
	const ware_besch_t * w1 = warenbauer_t::gib_info(*(const unsigned short *)p1);
	const ware_besch_t * w2 = warenbauer_t::gib_info(*(const unsigned short *)p2);

	int order;

	switch (sortby) {
		case 0: // sort by number
			order = *(const unsigned short *)p1 - *(const unsigned short *)p2;
			break;
		case 2: // sort by revenue
			{
				const sint32 grundwert1281 = w1->gib_preis()<<7;
				const sint32 grundwert_bonus1 = w1->gib_preis()*(1000l+(relative_speed_change-100l)*w1->gib_speed_bonus());
				const sint32 price1 = (grundwert1281>grundwert_bonus1 ? grundwert1281 : grundwert_bonus1);
				const sint32 grundwert1282 = w2->gib_preis()<<7;
				const sint32 grundwert_bonus2 = w2->gib_preis()*(1000l+(relative_speed_change-100l)*w2->gib_speed_bonus());
				const sint32 price2 = (grundwert1282>grundwert_bonus2 ? grundwert1282 : grundwert_bonus2);
				order = price2-price1;
			}
			break;
		case 3: // sort by speed bonus
			order = w2->gib_speed_bonus()-w1->gib_speed_bonus();
			break;
		default:	// sort by name
			order = strcmp(translator::translate(w1->gib_name()), translator::translate(w2->gib_name()));
			break;
	}
	return sortreverse ? -order : order;
}



// creates the list and pass it to the child finction good_stats, which does the display stuff ...
void goods_frame_t::sort_list()
{
	sortedby.setze_text(translator::translate(sort_text[sortby]));
	sorteddir.setze_text(translator::translate( sortreverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc"));

	int n=0;
	for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
		const ware_besch_t * wtyp = warenbauer_t::gib_info(i);

		// Hajo: we skip goods that don't generate income
		//       this should only be true for the special good 'None'
		if(wtyp->gib_preis()!=0) {
			good_list[n++] = i;
		}
	}

	// now sort
	qsort((void *)good_list, n, sizeof(unsigned short), compare_goods);

	goods_stats.update_goodslist( good_list, relative_speed_change );
}

/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void goods_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);
	koord groesse = gib_fenstergroesse()-koord(0,4+4*LINESPACE+4+BUTTON_HEIGHT+2+16);
	scrolly.setze_groesse(groesse);
}




/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool goods_frame_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
	if(komp == &sortedby) {
		// sort by what
		sortby = (sort_mode_t)((int)(sortby+1)%(int)SORT_MODES);
		sort_list();
	}
	else if(komp == &sorteddir) {
		// order
		sortreverse ^= 1;
		sort_list();
	}
	else if(komp == &speed_down) {
		if(relative_speed_change>1) {
			relative_speed_change --;
			sort_list();
		}
	}
	else if(komp == &speed_up) {
		relative_speed_change ++;
		sort_list();
	}
	return true;
}



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void goods_frame_t::zeichnen(koord pos, koord gr)
{
	gui_frame_t::zeichnen(pos, gr);

	sprintf(speed_bonus,"%i",relative_speed_change-100);

	sprintf(speed_message,translator::translate("Speedbonus\nroad %i km/h, rail %i km/h\nships %i km/h, planes %i km/h."),
		(welt->get_average_speed(weg_t::strasse)*relative_speed_change)/100,
		(welt->get_average_speed(weg_t::schiene)*relative_speed_change)/100,
		(welt->get_average_speed(weg_t::wasser)*relative_speed_change)/100,
		(welt->get_average_speed(weg_t::luft)*relative_speed_change)/100
	);
	display_multiline_text(pos.x+11, pos.y+BUTTON_HEIGHT+4, speed_message, COL_WHITE);
}
