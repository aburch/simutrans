/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 * written by Volker Meyer
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "gui_container.h"
#include "gui_convoiinfo.h"
#include "convoi_frame.h"
#include "convoi_filter_frame.h"
#include "../simconvoi.h"
#include "../simcolor.h"

#include "../besch/ware_besch.h"
#include "../bauer/warenbauer.h"
#include "../dataobj/translator.h"

koord convoi_filter_frame_t::filter_buttons_pos[FILTER_BUTTONS] = {
	koord(5,   4),
	koord(5,  43),
	koord(21, 58),
	koord(21, 74),
	koord(21, 90),
	koord(21, 106),
	koord(5,  191),
	koord(21, 207),
	koord(21, 223),
	koord(21, 239),
	koord(21, 255),
	koord(21, 271),
	koord(21, 287),
	koord(125, 4),
	koord(21, 122),
	koord(21, 138),
	koord(21, 154),
	koord(21, 170)
};

const char *convoi_filter_frame_t::filter_buttons_text[FILTER_BUTTONS] = {
	"clf_chk_name_filter",
	"clf_chk_type_filter",
	"clf_chk_cars",
	"clf_chk_trains",
	"clf_chk_ships",
	"clf_chk_aircrafts",
	"clf_chk_spezial_filter",
	"clf_chk_noroute",
	"clf_chk_stucked",
	"clf_chk_noincome",
	"clf_chk_indepot",
	"clf_chk_noschedule",
	"clf_chk_noline",
	"clf_chk_waren",
	"clf_chk_monorail",
	"clf_chk_maglev",
	"clf_chk_narrowgauge",
	"clf_chk_trams"
};

convoi_frame_t::filter_flag_t convoi_filter_frame_t::filter_buttons_types[FILTER_BUTTONS] = {
    convoi_frame_t::name_filter,
    convoi_frame_t::typ_filter,
    convoi_frame_t::lkws_filter,
    convoi_frame_t::zuege_filter,
    convoi_frame_t::schiffe_filter,
    convoi_frame_t::aircraft_filter,
    convoi_frame_t::spezial_filter,
    convoi_frame_t::noroute_filter,
    convoi_frame_t::stucked_filter,
    convoi_frame_t::noincome_filter,
    convoi_frame_t::indepot_filter,
    convoi_frame_t::nofpl_filter,
    convoi_frame_t::noline_filter,
    convoi_frame_t::ware_filter,
    convoi_frame_t::monorail_filter,
    convoi_frame_t::maglev_filter,
    convoi_frame_t::narrowgauge_filter,
	convoi_frame_t::tram_filter
};


convoi_filter_frame_t::convoi_filter_frame_t(spieler_t *sp, convoi_frame_t *main_frame) :
    gui_frame_t("clf_title", sp),
    ware_scrolly(&ware_cont)
{
	unsigned i;
	unsigned n;

	this->main_frame = main_frame;

	for(i = 0; i < FILTER_BUTTONS; i++) {
		filter_buttons[i].init(button_t::square, filter_buttons_text[i], filter_buttons_pos[i]);
		filter_buttons[i].add_listener(this);
		add_komponente(filter_buttons + i);
		if(filter_buttons_types[i] < convoi_frame_t::sub_filter) {
			filter_buttons[i].foreground = COL_WHITE;
		}
	}
	name_filter_input.set_text(main_frame->access_name_filter(), 30);
	name_filter_input.set_groesse(koord(100, 14));
	name_filter_input.set_pos(koord(5, 17));
	name_filter_input.add_listener(this);
	add_komponente(&name_filter_input);

	ware_alle.init(button_t::roundbox, "clf_btn_alle", koord(125, 17), koord(41, 14));
	ware_alle.add_listener(this);
	add_komponente(&ware_alle);
	ware_keine.init(button_t::roundbox, "clf_btn_keine", koord(167, 17), koord(41, 14));
	ware_keine.add_listener(this);
	add_komponente(&ware_keine);
	ware_invers.init(button_t::roundbox, "clf_btn_invers", koord(209, 17), koord(41, 14));
	ware_invers.add_listener(this);
	add_komponente(&ware_invers);

	ware_scrolly.set_pos(koord(125, 33));
	add_komponente(&ware_scrolly);

	for(i=n=0; i<warenbauer_t::get_waren_anzahl(); i++) {
		const ware_besch_t *ware = warenbauer_t::get_info(i);
		if(ware == warenbauer_t::nichts) {
			continue;
		}
		if(ware->get_catg()==0) {
			// Sonderfracht: Each good is special
			ware_item_t *item = new ware_item_t(this, ware);
			item->init(button_t::square, translator::translate(ware->get_name()), koord(16, 16*n++ + 4));
			ware_cont.add_komponente(item);
		}
	}
	// no add goo categories
	for(i=1; i<warenbauer_t::get_max_catg_index(); i++) {
		if(warenbauer_t::get_info_catg(i)->get_catg()!=0) {
			ware_item_t *item = new ware_item_t(this, warenbauer_t::get_info_catg(i));
			item->init(button_t::square, translator::translate(warenbauer_t::get_info_catg(i)->get_catg_name()), koord(16, 16*n++ + 4));
			ware_cont.add_komponente(item);
		}
	}
	ware_cont.set_groesse(koord(100, 16*n + 4));
	ware_scrolly.set_groesse(koord(125, 269));

	set_fenstergroesse(koord(255, 323));
}



convoi_filter_frame_t::~convoi_filter_frame_t()
{
	main_frame->filter_frame_closed();
}



/**
 * This method is called if an action is triggered
 * @author V. Meyer
 */
bool convoi_filter_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	int i;

	for(i = 0; i < FILTER_BUTTONS; i++) {
		if(komp == filter_buttons + i) {
			main_frame->set_filter(filter_buttons_types[i], !main_frame->get_filter(filter_buttons_types[i]));
			main_frame->sort_list();
			return true;
		}
	}
	if(komp == &ware_alle) {
		main_frame->set_alle_ware_filter(1);
		main_frame->sort_list();
		return true;
	}
	if(komp == &ware_keine) {
		main_frame->set_alle_ware_filter(0);
		main_frame->sort_list();
		return true;
	}
	if(komp == &ware_invers) {
		main_frame->set_alle_ware_filter(-1);
		main_frame->sort_list();
		return true;
	}
	if(komp == &name_filter_input) {
		main_frame->sort_list();
		return true;
	}
	return false;
}



void convoi_filter_frame_t::ware_item_triggered(const ware_besch_t *ware)
{
	if(ware->get_catg()==0) {
		main_frame->set_ware_filter(ware, -1);
	}
	else {
		for(uint8 i=0; i<warenbauer_t::get_waren_anzahl(); i++) {
			const ware_besch_t *testware = warenbauer_t::get_info(i);
			if(testware->get_catg() == ware->get_catg()) {
				main_frame->set_ware_filter(testware, -1);
			}
		}
	}
	main_frame->sort_list();
}



void convoi_filter_frame_t::zeichnen(koord pos, koord gr)
{
	for(int  i=0;  i<FILTER_BUTTONS;  i++) {
		filter_buttons[i].pressed = main_frame->get_filter(filter_buttons_types[i]);
	}
	gui_frame_t::zeichnen(pos, gr);
}
