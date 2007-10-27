/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "../simdebug.h"
#include "../simcity.h"
#include "../simworld.h"
#include "../simcolor.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../utils/simstring.h"
#include "components/list_button.h"

#include "stadt_info.h"

#include "../simgraph.h"


// @author hsiegeln
const char hist_type[MAX_CITY_HISTORY][64] =
{
  "citicens", "Growth", "Transported", "Passagiere",
};


const int hist_type_color[MAX_CITY_HISTORY] =
{
  COL_CITICENS, COL_GROWTH, COL_TRANSPORTED, COL_FREE_CAPACITY
};


stadt_info_t::stadt_info_t(stadt_t* stadt_) :
  gui_frame_t("Stadtinformation"),
	stadt(stadt_)
{
	tstrncpy( name, stadt->gib_name(), 256 );
	name_input.setze_text(name, 30);
	name_input.setze_groesse(koord(124, 14));
	name_input.setze_pos(koord(8, 8));

	add_komponente(&name_input);
	setze_fenstergroesse(koord(410, 305));

	//CHART YEAR
	chart.setze_pos(koord(1,1));
	chart.setze_groesse(koord(360,120));
	chart.set_dimension(MAX_CITY_HISTORY_YEARS, 10000);
	chart.set_seed(stadt->get_welt()->get_last_year());
	chart.set_background(MN_GREY1);
	int i;
	for (i = 0; i<MAX_CITY_HISTORY; i++) {
		chart.add_curve(hist_type_color[i], stadt->get_city_history_year(), MAX_CITY_HISTORY, i, 12, STANDARD, i < 2, true);
	}
	//CHART YEAR END

	//CHART MONTH
	mchart.setze_pos(koord(1,1));
	mchart.setze_groesse(koord(360,120));
	mchart.set_dimension(MAX_CITY_HISTORY_MONTHS, 10000);
	mchart.set_seed(0);
	mchart.set_background(MN_GREY1);
	for (i = 0; i<MAX_CITY_HISTORY; i++) {
		mchart.add_curve(hist_type_color[i], stadt->get_city_history_month(), MAX_CITY_HISTORY, i, 12, STANDARD, i < 2, true);
	}
	mchart.set_visible(false);
	//CHART MONTH END

	// tab (month/year)
	year_month_tabs.add_tab(&chart, translator::translate("Years"));
	year_month_tabs.add_tab(&mchart, translator::translate("Months"));
	year_month_tabs.setze_pos(koord(40,125));
	year_month_tabs.setze_groesse(koord(360, 125));
	add_komponente(&year_month_tabs);

	// add filter buttons
	for (int hist=0;hist<MAX_CITY_HISTORY;hist++) {
		filterButtons[hist].init(button_t::box_state, translator::translate(hist_type[hist]), koord(4+hist*100,270), koord(96, BUTTON_HEIGHT));
		filterButtons[hist].add_listener(this);
		filterButtons[hist].background = hist_type_color[hist];
		filterButtons[hist].pressed = hist<2;
		add_komponente(filterButtons + hist);
	}
}


void
stadt_info_t::zeichnen(koord pos, koord gr)
{
	if(strcmp(name,stadt->gib_name())) {
		stadt->setze_name( name );
	}

	// Hajo: update chart seed
	chart.set_seed(stadt->get_welt()->get_last_year());

	gui_frame_t::zeichnen(pos, gr);

	char buf[1024];
	stadt->info(buf);

	display_multiline_text(pos.x+8, pos.y+48, buf, COL_BLACK);

	display_array_wh(pos.x+140, pos.y+24, 128, 128, stadt->gib_pax_ziele_alt()->to_array());
	display_array_wh(pos.x+140 + 128 + 4, pos.y+24, 128, 128, stadt->gib_pax_ziele_neu()->to_array());

#if 0
    sprintf(buf, "%s: %d/%d",
            translator::translate("Passagiere"),
            stadt->gib_pax_transport(),
            stadt->gib_pax_erzeugt());

    display_proportional(pos.x+144, pos.y+180, buf, ALIGN_LEFT, COL_BLACK, true);
#endif
}



bool stadt_info_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
	for ( int i = 0; i<MAX_CITY_HISTORY; i++) {
		if (komp == &filterButtons[i]) {
			filterButtons[i].pressed ^= 1;
			if (filterButtons[i].pressed) {
				chart.show_curve(i);
				mchart.show_curve(i);
			}
			else {
				chart.hide_curve(i);
				mchart.hide_curve(i);
			}
			return true;
		}
	}
	return false;
}
