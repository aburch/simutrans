/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 * written by Volker Meyer
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Stations/stops list filter dialog
 * Displays filter settings for the halt list
 * @author V. Meyer
 */

#include "halt_list_filter_frame.h"
#include "../simcolor.h"

#include "../besch/ware_besch.h"
#include "../bauer/warenbauer.h"
#include "../dataobj/translator.h"

koord halt_list_filter_frame_t::filter_buttons_pos[FILTER_BUTTONS] = {
	koord(4, 2),
	koord(125, 2),
	koord(265, 2),
	koord(4, 2*D_BUTTON_HEIGHT+4),
	koord(9, 3*D_BUTTON_HEIGHT+4),
	koord(9, 4*D_BUTTON_HEIGHT+4),
	koord(9, 5*D_BUTTON_HEIGHT+4),
	koord(9, 6*D_BUTTON_HEIGHT+4),
	koord(9, 7*D_BUTTON_HEIGHT+4),
	koord(9, 8*D_BUTTON_HEIGHT+4),
	koord(9, 9*D_BUTTON_HEIGHT+4),
	koord(9, 10*D_BUTTON_HEIGHT+4),
	koord(9, 11*D_BUTTON_HEIGHT+4),
	koord(4, 12*D_BUTTON_HEIGHT+8),
	koord(9, 13*D_BUTTON_HEIGHT+8),
	koord(9, 14*D_BUTTON_HEIGHT+8)
};

const char *halt_list_filter_frame_t::filter_buttons_text[FILTER_BUTTONS] = {
	"hlf_chk_name_filter",
	"hlf_chk_waren_annahme",
	"hlf_chk_waren_abgabe",
	"hlf_chk_type_filter",
	"hlf_chk_frachthof",
	"hlf_chk_bushalt",
	"hlf_chk_bahnhof",
	"hlf_chk_tramstop",
	"hlf_chk_anleger",
	"hlf_chk_airport",
	"hlf_chk_monorailstop",
	"hlf_chk_maglevstop",
	"hlf_chk_narrowgaugestop",
	"hlf_chk_spezial_filter",
	"hlf_chk_overflow",
	"hlf_chk_keine_verb"
};

halt_list_frame_t::filter_flag_t halt_list_filter_frame_t::filter_buttons_types[FILTER_BUTTONS] = {
	halt_list_frame_t::name_filter,
	halt_list_frame_t::ware_an_filter,
	halt_list_frame_t::ware_ab_filter,
	halt_list_frame_t::typ_filter,
	halt_list_frame_t::frachthof_filter,
	halt_list_frame_t::bushalt_filter,
	halt_list_frame_t::bahnhof_filter,
	halt_list_frame_t::tramstop_filter,
	halt_list_frame_t::dock_filter,
	halt_list_frame_t::airport_filter,
	halt_list_frame_t::monorailstop_filter,
	halt_list_frame_t::maglevstop_filter,
	halt_list_frame_t::narrowgaugestop_filter,
	halt_list_frame_t::spezial_filter,
	halt_list_frame_t::ueberfuellt_filter,
	halt_list_frame_t::ohneverb_filter
};


halt_list_filter_frame_t::halt_list_filter_frame_t(spieler_t *sp, halt_list_frame_t *main_frame) :
	gui_frame_t( translator::translate("hlf_title"), sp),
	ware_scrolly_ab(&ware_cont_ab),
	ware_scrolly_an(&ware_cont_an)
{
	this->main_frame = main_frame;

	for(  int i=0;  i<FILTER_BUTTONS;  i++  ) {
		filter_buttons[i].init(button_t::square, filter_buttons_text[i], filter_buttons_pos[i]);
		filter_buttons[i].add_listener(this);
		add_komponente(filter_buttons + i);
		if(  filter_buttons_types[i] < halt_list_frame_t::sub_filter  ) {
			filter_buttons[i].foreground = COL_WHITE;
		}
	}
	name_filter_input.set_text(main_frame->access_name_filter(), 30);
	name_filter_input.set_groesse(koord(100, D_BUTTON_HEIGHT));
	name_filter_input.set_pos(koord(5, D_BUTTON_HEIGHT));
	name_filter_input.add_listener(this);
	add_komponente(&name_filter_input);

	ware_alle_an.init(button_t::roundbox, "hlf_btn_alle", koord(125, D_BUTTON_HEIGHT), koord(41, D_BUTTON_HEIGHT));
	ware_alle_an.add_listener(this);
	add_komponente(&ware_alle_an);
	ware_keine_an.init(button_t::roundbox, "hlf_btn_keine", koord(167, D_BUTTON_HEIGHT), koord(41, D_BUTTON_HEIGHT));
	ware_keine_an.add_listener(this);
	add_komponente(&ware_keine_an);
	ware_invers_an.init(button_t::roundbox, "hlf_btn_invers", koord(209, D_BUTTON_HEIGHT), koord(41, D_BUTTON_HEIGHT));
	ware_invers_an.add_listener(this);
	add_komponente(&ware_invers_an);

	ware_scrolly_an.set_pos(koord(125, 2*D_BUTTON_HEIGHT+4));
	ware_scrolly_an.set_scroll_amount_y(D_BUTTON_HEIGHT);
	add_komponente(&ware_scrolly_an);

	int n=0;
	for(  int i=0;  i<warenbauer_t::get_waren_anzahl();  i++  ) {
		const ware_besch_t *ware = warenbauer_t::get_info(i);
		if(  ware != warenbauer_t::nichts  ) {
			ware_item_t *item = new ware_item_t(this, NULL, ware);
			item->init(button_t::square, translator::translate(ware->get_name()), koord(5, D_BUTTON_HEIGHT*n++));
			ware_cont_an.add_komponente(item);
		}
	}
	ware_cont_an.set_groesse(koord(100, n*D_BUTTON_HEIGHT));
	ware_scrolly_an.set_groesse(koord(125, 13*D_BUTTON_HEIGHT));

	ware_alle_ab.init(button_t::roundbox, "hlf_btn_alle", koord(265, D_BUTTON_HEIGHT), koord(41, D_BUTTON_HEIGHT));
	ware_alle_ab.add_listener(this);
	add_komponente(&ware_alle_ab);
	ware_keine_ab.init(button_t::roundbox, "hlf_btn_keine", koord(307, D_BUTTON_HEIGHT), koord(41, D_BUTTON_HEIGHT));
	ware_keine_ab.add_listener(this);
	add_komponente(&ware_keine_ab);
	ware_invers_ab.init(button_t::roundbox, "hlf_btn_invers", koord(349, D_BUTTON_HEIGHT), koord(41, D_BUTTON_HEIGHT));
	ware_invers_ab.add_listener(this);
	add_komponente(&ware_invers_ab);

	ware_scrolly_ab.set_pos(koord(265, 2*D_BUTTON_HEIGHT+4));
	ware_scrolly_ab.set_scroll_amount_y(D_BUTTON_HEIGHT);
	add_komponente(&ware_scrolly_ab);

	n=0;
	for(  int i=0;  i<warenbauer_t::get_waren_anzahl();  i++  ) {
		const ware_besch_t *ware = warenbauer_t::get_info(i);
		if(  ware != warenbauer_t::nichts  ) {
		ware_item_t *item = new ware_item_t(this, ware, NULL);
		item->init(button_t::square, translator::translate(ware->get_name()), koord(5, D_BUTTON_HEIGHT*n++));
			ware_cont_ab.add_komponente(item);
		}
	}
	ware_cont_ab.set_groesse(koord(100, n*D_BUTTON_HEIGHT));
	ware_scrolly_ab.set_groesse(koord(125, 13*D_BUTTON_HEIGHT));

	set_fenstergroesse(koord(488, D_TITLEBAR_HEIGHT+(FILTER_BUTTONS-1)*D_BUTTON_HEIGHT+8+10));
	set_min_windowsize(koord(395, D_TITLEBAR_HEIGHT+(FILTER_BUTTONS-1)*D_BUTTON_HEIGHT+8-2));

	set_resizemode(diagonal_resize);
	resize(koord(0,0));
}



halt_list_filter_frame_t::~halt_list_filter_frame_t()
{
	main_frame->filter_frame_closed();
}



/**
 * This method is called if an action is triggered
 * @author V. Meyer
 */
bool halt_list_filter_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	int i;

	for (i = 0; i < FILTER_BUTTONS; i++) {
		if (komp == filter_buttons + i) {
			main_frame->set_filter(filter_buttons_types[i], !main_frame->get_filter(filter_buttons_types[i]));
			main_frame->display_list();
			return true;
		}
	}
	if (komp == &ware_alle_ab) {
		main_frame->set_alle_ware_filter_ab(1);
		main_frame->display_list();
		return true;
	}
	if (komp == &ware_keine_ab) {
		main_frame->set_alle_ware_filter_ab(0);
		main_frame->display_list();
		return true;
	}
	if (komp == &ware_invers_ab) {
		main_frame->set_alle_ware_filter_ab(-1);
		main_frame->display_list();
		return true;
	}
	if (komp == &ware_alle_an) {
		main_frame->set_alle_ware_filter_an(1);
		main_frame->display_list();
		return true;
	}
	if (komp == &ware_keine_an) {
		main_frame->set_alle_ware_filter_an(0);
		main_frame->display_list();
		return true;
	}
	if (komp == &ware_invers_an) {
		main_frame->set_alle_ware_filter_an(-1);
		main_frame->display_list();
		return true;
	}
	if (komp == &name_filter_input) {
		main_frame->display_list();
		return true;
	}
	return false;
}


void halt_list_filter_frame_t::ware_item_triggered(const ware_besch_t *ware_ab, const ware_besch_t *ware_an)
{
	if (ware_ab) {
		main_frame->set_ware_filter_ab(ware_ab, -1);
	}
	if (ware_an) {
		main_frame->set_ware_filter_an(ware_an, -1);
	}
	main_frame->display_list();
}


void halt_list_filter_frame_t::zeichnen(koord pos, koord gr)
{
	for(int i = 0; i < FILTER_BUTTONS; i++) {
		filter_buttons[i].pressed = main_frame->get_filter(filter_buttons_types[i]);
	}
	gui_frame_t::zeichnen(pos, gr);
}


void halt_list_filter_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);

	const koord gr = get_fenstergroesse()-koord(0, D_TITLEBAR_HEIGHT);

	const KOORD_VAL w1 = gr.x/3-4;
	const KOORD_VAL w2 = (gr.x+1)/3-4;
	const KOORD_VAL w3 = (gr.x+2)/3-4;
	const KOORD_VAL pos2 = w1;
	const KOORD_VAL pos3 = gr.x-w3;
	const KOORD_VAL h = (gr.y-2-2*D_BUTTON_HEIGHT-4);

	name_filter_input.set_groesse(koord(min(w1-18,142), D_BUTTON_HEIGHT));

	// column 2
	filter_buttons_pos[1] = koord(pos2, 2);
	filter_buttons[1].set_pos(filter_buttons_pos[1]);
	ware_alle_an.set_pos(koord(pos2, D_BUTTON_HEIGHT));
	ware_alle_an.set_groesse(koord(w2/3-D_H_SPACE, D_BUTTON_HEIGHT));
	ware_keine_an.set_pos(koord(pos2+(w2+0)/3, D_BUTTON_HEIGHT));
	ware_keine_an.set_groesse(koord((w2+1)/3-D_H_SPACE, D_BUTTON_HEIGHT));
	ware_invers_an.set_pos(koord(pos2+(w2+0)/3+(w2+1)/3, D_BUTTON_HEIGHT));
	ware_invers_an.set_groesse(koord((w2+2)/3-D_H_SPACE, D_BUTTON_HEIGHT));
	ware_scrolly_an.set_pos(koord(pos2, 2*D_BUTTON_HEIGHT+4));
	ware_scrolly_an.set_groesse(koord(w2, h));

	// column 3
	filter_buttons_pos[2] = koord(pos3, 2);
	filter_buttons[2].set_pos(filter_buttons_pos[2]);
	ware_alle_ab.set_pos(koord(pos3, D_BUTTON_HEIGHT));
	ware_alle_ab.set_groesse(koord(w3/3-D_H_SPACE, D_BUTTON_HEIGHT));
	ware_keine_ab.set_pos(koord(pos3+(w3+0)/3, D_BUTTON_HEIGHT));
	ware_keine_ab.set_groesse(koord((w3+1)/3-D_H_SPACE, D_BUTTON_HEIGHT));
	ware_invers_ab.set_pos(koord(pos3+(w3+0)/3+(w3+1)/3, D_BUTTON_HEIGHT));
	ware_invers_ab.set_groesse(koord((w3+2)/3-D_H_SPACE, D_BUTTON_HEIGHT));
	ware_scrolly_ab.set_pos(koord(pos3, 2*D_BUTTON_HEIGHT+4));
	ware_scrolly_ab.set_groesse(koord(w3, h));
}
