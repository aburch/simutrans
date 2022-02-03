/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "halt_list_filter_frame.h"
#include "../simcolor.h"

#include "../descriptor/goods_desc.h"
#include "../builder/goods_manager.h"
#include "../dataobj/translator.h"

const char *halt_list_filter_frame_t::filter_buttons_text[FILTER_BUTTONS] = {
	"hlf_chk_spezial_filter",
	"hlf_chk_waren_annahme",
	"hlf_chk_waren_abgabe",
	"hlf_chk_overflow",
	"hlf_chk_keine_verb"
};

halt_list_frame_t::filter_flag_t halt_list_filter_frame_t::filter_buttons_types[FILTER_BUTTONS] = {
	halt_list_frame_t::spezial_filter,
	halt_list_frame_t::ware_an_filter,
	halt_list_frame_t::ware_ab_filter,
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
	}

	set_table_layout( 1, 0 );

	tabs.add_tab( &special_filter, translator::translate( filter_buttons_text[0] ) );
	tabs.add_tab( &accepts_filter, translator::translate( filter_buttons_text[1] ) );
	tabs.add_tab( &sends_filter, translator::translate( filter_buttons_text[2] ) );
	add_component( &tabs );

	// first column
	special_filter.set_table_layout(2,0);
	{
		special_filter.add_component(filter_buttons + 0,2);

		special_filter.new_component<gui_empty_t>();
		special_filter.add_component(filter_buttons + 3);

		special_filter.new_component<gui_empty_t>();
		special_filter.add_component(filter_buttons + 4);
	}

	// second columns: accepted cargo types
	accepts_filter.set_table_layout(1,0);
	{
		accepts_filter.add_component(filter_buttons + 1);
		accepts_filter.add_table(3,0);
		{
			ware_alle_an.init(button_t::roundbox, "hlf_btn_alle");
			ware_alle_an.add_listener(this);
			accepts_filter.add_component(&ware_alle_an);
			ware_keine_an.init(button_t::roundbox, "hlf_btn_keine");
			ware_keine_an.add_listener(this);
			accepts_filter.add_component(&ware_keine_an);
			ware_invers_an.init(button_t::roundbox, "hlf_btn_invers");
			ware_invers_an.add_listener(this);
			accepts_filter.add_component(&ware_invers_an);
		}
		accepts_filter.end_table();

		ware_scrolly_an.set_scroll_amount_y(D_BUTTON_HEIGHT);
		accepts_filter.add_component(&ware_scrolly_an);

		ware_cont_an.set_table_layout(2,0);
		for(  int i=0;  i<goods_manager_t::get_count();  i++  ) {
			const goods_desc_t *ware = goods_manager_t::get_info(i);
			if(  ware != goods_manager_t::none  ) {
				ware_item_t *item = ware_cont_an.new_component<ware_item_t>(this, (const goods_desc_t*)NULL, ware);
				item->init(button_t::square, translator::translate(ware->get_name()));
			}
		}
	}

	// second columns: outgoing cargo types
	sends_filter.set_table_layout(1,0);
	{
		sends_filter.add_component(filter_buttons + 2);
		sends_filter.add_table(3,0);
		{
			ware_alle_ab.init(button_t::roundbox, "hlf_btn_alle");
			ware_alle_ab.add_listener(this);
			sends_filter.add_component(&ware_alle_ab);
			ware_keine_ab.init(button_t::roundbox, "hlf_btn_keine");
			ware_keine_ab.add_listener(this);
			sends_filter.add_component(&ware_keine_ab);
			ware_invers_ab.init(button_t::roundbox, "hlf_btn_invers");
			ware_invers_ab.add_listener(this);
			sends_filter.add_component(&ware_invers_ab);
		}
		sends_filter.end_table();

		ware_scrolly_ab.set_scroll_amount_y(D_BUTTON_HEIGHT);
		sends_filter.add_component(&ware_scrolly_ab);

		ware_cont_ab.set_table_layout(2,0);
		for(  int i=0;  i<goods_manager_t::get_count();  i++  ) {
			const goods_desc_t *ware = goods_manager_t::get_info(i);
			if(  ware != goods_manager_t::none  ) {
				ware_item_t *item = ware_cont_ab.new_component<ware_item_t>(this, ware, (const goods_desc_t*)NULL);
				item->init(button_t::square, translator::translate(ware->get_name()));
			}
		}
	}

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
}


halt_list_filter_frame_t::~halt_list_filter_frame_t()
{
	main_frame->filter_frame_closed();
}


/**
 * This method is called if an action is triggered
 */
bool halt_list_filter_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	for (int i = 0; i < FILTER_BUTTONS; i++) {
		if (comp == filter_buttons + i) {
			main_frame->set_filter(filter_buttons_types[i], !main_frame->get_filter(filter_buttons_types[i]));
			main_frame->sort_list();
			return true;
		}
	}
	if (comp == &ware_alle_ab) {
		main_frame->set_alle_ware_filter_ab(1);
		main_frame->sort_list();
		return true;
	}
	if (comp == &ware_keine_ab) {
		main_frame->set_alle_ware_filter_ab(0);
		main_frame->sort_list();
		return true;
	}
	if (comp == &ware_invers_ab) {
		main_frame->set_alle_ware_filter_ab(-1);
		main_frame->sort_list();
		return true;
	}
	if (comp == &ware_alle_an) {
		main_frame->set_alle_ware_filter_an(1);
		main_frame->sort_list();
		return true;
	}
	if (comp == &ware_keine_an) {
		main_frame->set_alle_ware_filter_an(0);
		main_frame->sort_list();
		return true;
	}
	if (comp == &ware_invers_an) {
		main_frame->set_alle_ware_filter_an(-1);
		main_frame->sort_list();
		return true;
	}
	if (comp == &name_filter_input) {
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
