/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Goods list dialog
 */

#include <algorithm>

#include "goods_frame_t.h"
#include "components/gui_scrollpane.h"


#include "../bauer/goods_manager.h"
#include "../descriptor/goods_desc.h"
#include "../dataobj/translator.h"

#include "../simcolor.h"
#include "../simline.h"
#include "simwin.h"
#include "../simworld.h"

/**
 * This variable defines the current speed for bonus calculation
 * @author prissi
 */
sint16 goods_frame_t::relative_speed_change=100;

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
	"gl_btn_sort_bonus",
	"gl_btn_sort_catg"
};

/**
 * This variable controls whether all goods are displayed, or
 * just the ones relevant to the current game
 * Values: false = all goods shown, true = relevant goods shown
 * @author falconne
 */
bool goods_frame_t::filter_goods = false;

static simline_t::linetype last_scheduletype = simline_t::trainline;

goods_frame_t::goods_frame_t() :
	gui_frame_t( translator::translate("gl_title") ),
	speed_text( &speed_message, D_BUTTON_WIDTH*4+D_V_SPACE*3 ),
	goods_stats(),
	scrolly(&goods_stats)
{
	set_table_layout(1,0);

	// top line: combo, input, text
	add_table(3,1);
	{
		for (int i = 1; i < simline_t::MAX_LINE_TYPE; i++) {
			scheduletype.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(simline_t::get_linetype_name((simline_t::linetype)i), SYSCOL_TEXT);
		}
		scheduletype.set_selection( last_scheduletype-1 );
		add_component(&scheduletype);
		scheduletype.add_listener( this );

		speed.init( welt->get_average_speed(simline_t::linetype_to_waytype(last_scheduletype)), 1, 2048, gui_numberinput_t::AUTOLINEAR, false );
		add_component(&speed);
		speed.add_listener( this );

		gui_label_buf_t *lb = new_component<gui_label_buf_t>(SYSCOL_TEXT_HIGHLIGHT);
		lb->buf().printf(translator::translate("100 km/h = %i tiles/month"), welt->speed_to_tiles_per_month(kmh_to_speed(100)) );
		lb->update();
	}
	end_table();

	add_component(&speed_text);

	filter_goods_toggle.init(button_t::square_state, "Show only used");
	filter_goods_toggle.set_tooltip(translator::translate("Only show goods which are currently handled by factories"));
	filter_goods_toggle.add_listener(this);
	filter_goods_toggle.pressed = filter_goods;
	add_component(&filter_goods_toggle);

	// sort mode
	sort_row = add_table(3,1);
	{
		new_component<gui_label_t>("hl_txt_sort");

		sortedby.init(button_t::roundbox, "");
		sortedby.add_listener(this);
		add_component(&sortedby);

		sorteddir.init(button_t::roundbox, "");
		sorteddir.add_listener(this);
		add_component(&sorteddir);
	}
	end_table();

	add_component(&scrolly);

	sort_list();

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
	set_resizemode(diagonal_resize);
}


bool goods_frame_t::compare_goods(goods_desc_t const* const w1, goods_desc_t const* const w2)
{
	int order = 0;

	switch (sortby) {
		case 0: // sort by number
			order = w1->get_index() - w2->get_index();
			break;
		case 2: // sort by revenue
			{
				const sint32 grundwert1281 = w1->get_value() * goods_frame_t::welt->get_settings().get_bonus_basefactor();
				const sint32 grundwert_bonus1 = w1->get_value()*(1000l+(relative_speed_change-100l)*w1->get_speed_bonus());
				const sint32 price1 = (grundwert1281>grundwert_bonus1 ? grundwert1281 : grundwert_bonus1);
				const sint32 grundwert1282 = w2->get_value() * goods_frame_t::welt->get_settings().get_bonus_basefactor();
				const sint32 grundwert_bonus2 = w2->get_value()*(1000l+(relative_speed_change-100l)*w2->get_speed_bonus());
				const sint32 price2 = (grundwert1282>grundwert_bonus2 ? grundwert1282 : grundwert_bonus2);
				order = price1-price2;
			}
			break;
		case 3: // sort by speed bonus
			order = w1->get_speed_bonus()-w2->get_speed_bonus();
			break;
		case 4: // sort by catg_index
			order = w1->get_catg()-w2->get_catg();
			break;
		default: ; // make compiler happy, order will be determined below anyway
	}
	if(  order==0  ) {
		// sort by name if not sorted or not unique
		order = strcmp(translator::translate(w1->get_name()), translator::translate(w2->get_name()));
	}
	return sortreverse ? order > 0 : order < 0;
}


// creates the list and pass it to the child function good_stats, which does the display stuff ...
void goods_frame_t::sort_list()
{
	// update all strings
	relative_speed_change = (100*speed.get_value())/welt->get_average_speed(simline_t::linetype_to_waytype(last_scheduletype));

	speed_message.clear();
	speed_message.printf(translator::translate("Speedbonus\nroad %i km/h, rail %i km/h\nships %i km/h, planes %i km/h."),
		(welt->get_average_speed(road_wt)*relative_speed_change)/100,
		(welt->get_average_speed(track_wt)*relative_speed_change)/100,
		(welt->get_average_speed(water_wt)*relative_speed_change)/100,
		(welt->get_average_speed(air_wt)*relative_speed_change)/100
	);
	speed_message.printf(translator::translate("tram %i km/h, monorail %i km/h\nmaglev %i km/h, narrowgauge %i km/h."),
		(welt->get_average_speed(tram_wt)*relative_speed_change)/100,
		(welt->get_average_speed(monorail_wt)*relative_speed_change)/100,
		(welt->get_average_speed(maglev_wt)*relative_speed_change)/100,
		(welt->get_average_speed(narrowgauge_wt)*relative_speed_change)/100
	);
	speed_text.recalc_size();

	// update buttons
	sortedby.set_text(sort_text[sortby]);
	sorteddir.set_text(sortreverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
	filter_goods_toggle.pressed = filter_goods;
	sort_row->set_size(sort_row->get_min_size());

	// now prepare the sort
	// Fetch the list of goods produced by the factories that exist in the current game
	const vector_tpl<const goods_desc_t*> &goods_in_game = welt->get_goods_list();

	good_list.clear();
	for(unsigned int i=0; i<goods_manager_t::get_count(); i++) {
		const goods_desc_t * wtyp = goods_manager_t::get_info(i);

		// Hajo: we skip goods that don't generate income
		//       this should only be true for the special good 'None'
		if(  wtyp->get_value()!=0  &&  (!filter_goods  ||  goods_in_game.is_contained(wtyp))  ) {
			good_list.insert_ordered( wtyp, compare_goods );
		}
	}

	goods_stats.update_goodslist( good_list, relative_speed_change);
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool goods_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp == &sortedby) {
		// sort by what
		sortby = (sort_mode_t)((int)(sortby+1)%(int)SORT_MODES);
	}
	else if(komp == &sorteddir) {
		// order
		sortreverse ^= 1;
	}
	else if(komp == &filter_goods_toggle) {
		filter_goods = !filter_goods;
	}
	else if(komp == &scheduletype) {
		double relative_speed_change = speed.get_value()/(double)welt->get_average_speed(simline_t::linetype_to_waytype(last_scheduletype));
		last_scheduletype = (simline_t::linetype)(scheduletype.get_selection()+1);
		speed.set_value( (welt->get_average_speed(simline_t::linetype_to_waytype(last_scheduletype))*relative_speed_change)+0.5 );
	}
	sort_list();

	return true;
}


uint32 goods_frame_t::get_rdwr_id()
{
	return magic_goodslist;
}



void goods_frame_t::rdwr( loadsave_t *file )
{
	file->rdwr_short( relative_speed_change );
	file->rdwr_bool( sortreverse );
	file->rdwr_bool( filter_goods );
	sint16 s = last_scheduletype;
	file->rdwr_short( s );
	sint16 b = sortby;
	file->rdwr_short( b );

	if(  file->is_loading()  ) {
		scheduletype.set_selection( s-1 );
		sortby = (sort_mode_t)b;
		sort_list();
	}
}
