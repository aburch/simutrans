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

#include "../descriptor/goods_desc.h"
#include "../bauer/goods_manager.h"
#include "../dataobj/translator.h"

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


halt_list_filter_frame_t::halt_list_filter_frame_t(player_t *player, halt_list_frame_t *main_frame) :
	gui_frame_t( translator::translate("hlf_title"), player),
	ware_scrolly_ab(&ware_cont_ab),
	ware_scrolly_an(&ware_cont_an)
{
	this->main_frame = main_frame;

	// init buttons
	for(  int i=0;  i<FILTER_BUTTONS;  i++  ) {
		filter_buttons[i].init(button_t::square, filter_buttons_text[i]);
		filter_buttons[i].add_listener(this);
		if(  filter_buttons_types[i] < halt_list_frame_t::sub_filter  ) {
			filter_buttons[i].background_color = SYSCOL_TEXT_HIGHLIGHT;
		}
	}

	set_table_layout(3,0);
	set_alignment(ALIGN_TOP);

	// first column
	add_table(2,0);
	{
		// name filter
		add_component(filter_buttons + 0, 2);

		name_filter_input.set_text(main_frame->access_name_filter(), 30);
		name_filter_input.add_listener(this);
		add_component(&name_filter_input, 2);

		// type and special buttons
		for(  int i=3;  i<FILTER_BUTTONS;  i++  ) {
			if (i==3 || i==13) {
				add_component(filter_buttons + i, 2);
			}
			else {
				new_component<gui_empty_t>();
				add_component(filter_buttons + i);
			}
		}
	}
	end_table();

	// second columns: accepted cargo types
	add_table(1,0);
	{
		add_component(filter_buttons + 1);
		add_table(3,0);
		{
			ware_alle_an.init(button_t::roundbox, "hlf_btn_alle");
			ware_alle_an.add_listener(this);
			add_component(&ware_alle_an);
			ware_keine_an.init(button_t::roundbox, "hlf_btn_keine");
			ware_keine_an.add_listener(this);
			add_component(&ware_keine_an);
			ware_invers_an.init(button_t::roundbox, "hlf_btn_invers");
			ware_invers_an.add_listener(this);
			add_component(&ware_invers_an);
		}
		end_table();

		ware_scrolly_an.set_scroll_amount_y(D_BUTTON_HEIGHT);
		add_component(&ware_scrolly_an);

		ware_cont_an.set_table_layout(1,0);
		for(  int i=0;  i<goods_manager_t::get_count();  i++  ) {
			const goods_desc_t *ware = goods_manager_t::get_info(i);
			if(  ware != goods_manager_t::none  ) {
				ware_item_t *item = ware_cont_an.new_component<ware_item_t>(this, (const goods_desc_t*)NULL, ware);
				item->init(button_t::square, translator::translate(ware->get_name()));
			}
		}
	}
	end_table();

	// second columns: outgoing cargo types
	add_table(1,0);
	{
		add_component(filter_buttons + 2);
		add_table(3,0);
		{
			ware_alle_ab.init(button_t::roundbox, "hlf_btn_alle");
			ware_alle_ab.add_listener(this);
			add_component(&ware_alle_ab);
			ware_keine_ab.init(button_t::roundbox, "hlf_btn_keine");
			ware_keine_ab.add_listener(this);
			add_component(&ware_keine_ab);
			ware_invers_ab.init(button_t::roundbox, "hlf_btn_invers");
			ware_invers_ab.add_listener(this);
			add_component(&ware_invers_ab);
		}
		end_table();

		ware_scrolly_ab.set_scroll_amount_y(D_BUTTON_HEIGHT);
		add_component(&ware_scrolly_ab);

		ware_cont_ab.set_table_layout(1,0);
		for(  int i=0;  i<goods_manager_t::get_count();  i++  ) {
			const goods_desc_t *ware = goods_manager_t::get_info(i);
			if(  ware != goods_manager_t::none  ) {
				ware_item_t *item = ware_cont_ab.new_component<ware_item_t>(this, ware, (const goods_desc_t*)NULL);
				item->init(button_t::square, translator::translate(ware->get_name()));
			}
		}
	}
	end_table();

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
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
	for (int i = 0; i < FILTER_BUTTONS; i++) {
		if (komp == filter_buttons + i) {
			main_frame->set_filter(filter_buttons_types[i], !main_frame->get_filter(filter_buttons_types[i]));
			main_frame->sort_list();
			return true;
		}
	}
	if (komp == &ware_alle_ab) {
		main_frame->set_alle_ware_filter_ab(1);
		main_frame->sort_list();
		return true;
	}
	if (komp == &ware_keine_ab) {
		main_frame->set_alle_ware_filter_ab(0);
		main_frame->sort_list();
		return true;
	}
	if (komp == &ware_invers_ab) {
		main_frame->set_alle_ware_filter_ab(-1);
		main_frame->sort_list();
		return true;
	}
	if (komp == &ware_alle_an) {
		main_frame->set_alle_ware_filter_an(1);
		main_frame->sort_list();
		return true;
	}
	if (komp == &ware_keine_an) {
		main_frame->set_alle_ware_filter_an(0);
		main_frame->sort_list();
		return true;
	}
	if (komp == &ware_invers_an) {
		main_frame->set_alle_ware_filter_an(-1);
		main_frame->sort_list();
		return true;
	}
	if (komp == &name_filter_input) {
		main_frame->sort_list();
		return true;
	}
	return false;
}


void halt_list_filter_frame_t::ware_item_triggered(const goods_desc_t *ware_ab, const goods_desc_t *ware_an)
{
	if (ware_ab) {
		main_frame->set_ware_filter_ab(ware_ab, -1);
	}
	if (ware_an) {
		main_frame->set_ware_filter_an(ware_an, -1);
	}
	main_frame->sort_list();
}


void halt_list_filter_frame_t::draw(scr_coord pos, scr_size size)
{
	for(int i = 0; i < FILTER_BUTTONS; i++) {
		filter_buttons[i].pressed = main_frame->get_filter(filter_buttons_types[i]);
	}
	gui_frame_t::draw(pos, size);
}
