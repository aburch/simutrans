/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../world/simworld.h"
#include "../simevent.h"

#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../descriptor/ground_desc.h"

#include "../player/simplay.h"

#include "extend_edit.h"

gui_rotation_item_t::gui_rotation_item_t(uint8 r) : gui_scrolled_list_t::const_text_scrollitem_t(NULL, SYSCOL_TEXT)
{
	rotation = r;
	switch(rotation) {
		case 0:         text = translator::translate("[0] south-facing");       break;
		case 1:         text = translator::translate("[1] east-facing");        break;
		case 2:         text = translator::translate("[2] north-facing");       break;
		case 3:         text = translator::translate("[3] west-facing");        break;
		case 4:         text = translator::translate("[4] southeast corner");   break;
		case 5:         text = translator::translate("[5] northeast corner");   break;
		case 6:         text = translator::translate("[6] northwest corner");   break;
		case 7:         text = translator::translate("[7] southwest corner");   break;
		case automatic: text = translator::translate("auto");               break;
		case random:    text = translator::translate("random");             break;
		default:        text = "";
	}
}

gui_climates_item_t::gui_climates_item_t(uint8 c) : gui_scrolled_list_t::const_text_scrollitem_t(NULL, SYSCOL_TEXT)
{
	if(c<MAX_CLIMATES){
		climate_ = 1<<c;
		text = translator::translate(ground_desc_t::get_climate_name_from_bit((climate)c));
	}
	else{
		climate_ = ALL_CLIMATES;
		text = translator::translate("All");
	}
}

gui_sorting_item_t::gui_sorting_item_t(uint8 s) : gui_scrolled_list_t::const_text_scrollitem_t(NULL, SYSCOL_TEXT)
{
	sorted_by=s;
	switch(sorted_by) {
		case BY_NAME_TRANSLATED:     text = translator::translate("Translation"); break;
		case BY_NAME_OBJECT:         text = translator::translate("Object"); break;
		case BY_LEVEL_PAX:           text = translator::translate("Pax level"); break;
		case BY_LEVEL_MAIL:          text = translator::translate("Mail level"); break;
		case BY_DATE_INTRO:          text = translator::translate("Intro. date"); break;
		case BY_DATE_RETIRE:         text = translator::translate("Retire date"); break;
		case BY_SIZE:                text = translator::translate("Size (area)"); break;
		case BY_COST:                text = translator::translate("Price"); break;
		case BY_GOODS_NUMBER:        text = translator::translate("Goods"); break;
		case BY_REMOVAL:             text = translator::translate("cost for removal"); break;
		default:                     text = "";
	}
}


extend_edit_gui_t::extend_edit_gui_t(const char *name, player_t* player_) :
	gui_frame_t( name, player_ ),
	player(player_),
	info_text(&buf, D_BUTTON_WIDTH*4),
	scrolly(&cont_scrolly, true, true),
	scl(gui_scrolled_list_t::listskin, gui_scrolled_list_t::scrollitem_t::compare)
{
	// layouting - where is which element?
	set_table_layout(3,0);
	set_force_equal_columns(false);
	set_alignment(ALIGN_LEFT | ALIGN_TOP);
	set_resizemode(diagonal_resize);

	// left column
	add_component(&cont_left);
	cont_left.set_table_layout(1,0);
	cont_left.set_size(get_min_size());
	// cont_left.set_alignment(ALIGN_CENTER_H);
	// add timeline settings
	cont_left.add_component(&cont_timeline);
	cont_timeline.set_table_layout(4,0);
	// add filters
	cont_left.add_component(&cont_filter);
	cont_filter.set_table_layout(3,0);
	//add list
	cont_left.add_component(&scl);
	// add stretcher element
	cont_left.new_component<gui_fill_t>();

	// right column
	add_component(&cont_right,2);
	cont_right.set_table_layout(1,0);
	// add object settings (rotations, ...)
	cont_right.add_component(&cont_options);
	cont_options.set_table_layout(1,0);
	// add divider element
	cont_right.new_component<gui_divider_t>();
	// add scrollable element
	cont_right.add_component(&scrolly);
	cont_right.new_component<gui_fill_t>();
	//cont scrolly is already in scrolly
	cont_scrolly.set_table_layout(2,0);
	cont_scrolly.set_margin( scr_size( 0, D_V_SPACE ), scr_size( 0, D_V_SPACE ) );
	// add object description
	cont_scrolly.add_component(&info_text,2);
	// add object image
	cont_scrolly.add_component(&building_image);
	cont_scrolly.new_component<gui_fill_t>(true,false);
	cont_scrolly.new_component_span<gui_fill_t>(false,true,2);
	//end of layouting. Now fill elements

	// init scrolled list
	scl.set_selection(-1);
	scl.add_listener(this);
	scl.set_min_width( (D_DEFAULT_WIDTH-D_MARGIN_LEFT-D_MARGIN_RIGHT-2*D_H_SPACE)/2 );

	// start filling cont_timeline---------------------------------------------------------------------------------------------
	bt_timeline.init( button_t::square_state, "timeline");
	bt_timeline.pressed = welt->get_settings().get_use_timeline();
	bt_timeline.add_listener(this);
	cont_timeline.add_component(&bt_timeline, 4);

	bt_timeline_custom.init( button_t::square_state, "Available at custom date");
	bt_timeline_custom.add_listener(this);
	cont_timeline.add_component(&bt_timeline_custom, 4);

	// respect year/month order according to language settings
	bool year_month_order = (  env_t::show_month == env_t::DATE_FMT_JAPANESE || env_t::show_month == env_t::DATE_FMT_JAPANESE_NO_SEASON  );

	if( !year_month_order ) { // month first then year
		cont_timeline.new_component<gui_label_t>("Month");
		cont_timeline.add_component(&ni_timeline_month);
	}
	cont_timeline.new_component<gui_label_t>("Year");
	cont_timeline.add_component(&ni_timeline_year);
	if( year_month_order ) { // year first then month
		cont_timeline.new_component<gui_label_t>("Month");
		cont_timeline.add_component(&ni_timeline_month);
	}
	ni_timeline_month.init( (sint32)(welt->get_current_month()%12+1), 1, 12, 1, true );
	ni_timeline_month.add_listener(this);
	ni_timeline_year.init( (sint32)(welt->get_current_month()/12), 0, 2999, 1, false );
	ni_timeline_year.add_listener(this);

	bt_obsolete.init( button_t::square_state, "Show obsolete");
	bt_obsolete.add_listener(this);
	cont_timeline.add_component(&bt_obsolete, 4);
	// end filling cont_timeline---------------------------------------------------------------------------------------------

	// start filling cont_filter---------------------------------------------------------------------------------------------
	// climate filter
	cont_filter.new_component<gui_label_t>("Climate");
	cont_filter.add_component(&cb_climates, 2);
	cb_climates.add_listener(this);
	cb_climates.new_component<gui_climates_item_t>(climate::MAX_CLIMATES);
	for(uint8 i=climate::desert_climate; i<climate::MAX_CLIMATES; i++){
		cb_climates.new_component<gui_climates_item_t>(i);
	}
	cb_climates.set_selection(0);

	// Sorting box
	cont_filter.new_component<gui_label_t>("Sort by");
	cont_filter.add_component(&cb_sortedby);
	sort_order.init(button_t::sortarrow_state, "");
	sort_order.set_tooltip(translator::translate("hl_btn_sort_order"));
	sort_order.add_listener(this);
	sort_order.pressed = false;
	cont_filter.add_component(&sort_order);
	cb_sortedby.add_listener(this);
	cb_sortedby.new_component<gui_sorting_item_t>(gui_sorting_item_t::BY_NAME_TRANSLATED);
	cb_sortedby.new_component<gui_sorting_item_t>(gui_sorting_item_t::BY_NAME_OBJECT);
	cb_sortedby.set_selection(0);
	// end filling cont_filter---------------------------------------------------------------------------------------------

	//filling cont_options
	bt_climates.init( button_t::square_state, "ignore climates");
	bt_climates.add_listener(this);
	cont_options.add_component(&bt_climates);

	//setting scrollable content box
	scrolly.set_visible(true);
	scrolly.set_min_width( (D_DEFAULT_WIDTH-D_MARGIN_LEFT-D_MARGIN_RIGHT-2*D_H_SPACE)/2 );
}


/**
 * Mouse click are hereby reported to its GUI-Components
 */
bool extend_edit_gui_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE) {
		change_item_info(-1);
	}
	return gui_frame_t::infowin_event(ev);
}


// resize flowtext to avoid horizontal scrollbar
void extend_edit_gui_t::set_windowsize( scr_size s )
{
	gui_frame_t::set_windowsize( s );
	info_text.set_width( scrolly.get_client().w );
}


bool extend_edit_gui_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if (comp == &scl) {
		// select an item of scroll list ?
		change_item_info(scl.get_selection());
	}
	else if(  comp==&bt_obsolete  ) {
		bt_obsolete.pressed ^= 1;
		fill_list();
	}
	else if(  comp==&bt_climates  ) {
		bt_climates.pressed ^= 1;
		fill_list();
	}
	else if(  comp==&bt_timeline  ) {
		bt_timeline_custom.pressed = false;
		bt_timeline.pressed ^= 1;
		fill_list();
	}
	else if (  comp==&bt_timeline_custom  ) {
		bt_timeline.pressed = false;
		bt_timeline_custom.pressed ^= 1;
		fill_list();
	}
	else if (  comp==&ni_timeline_year  &&  bt_timeline_custom.pressed) {
		fill_list();
	}
	else if (  comp==&ni_timeline_month  &&  bt_timeline_custom.pressed) {
		fill_list();
	}
	else if(  comp==&cb_climates  ) {
		fill_list();
		change_item_info(scl.get_selection());
	}
	else if(  comp==&cb_sortedby  ) {
		fill_list();
	}
	else if(  comp == &sort_order  ) {
		sort_order.pressed = !sort_order.pressed;
		fill_list();
	}

	return true;
}


uint8 extend_edit_gui_t::get_rotation() const
{
	if (gui_rotation_item_t *item = dynamic_cast<gui_rotation_item_t*>( cb_rotation.get_selected_item() ) ) {
		return item->get_rotation();
	}
	return 0;
}
uint8 extend_edit_gui_t::get_climate() const
{
	if (gui_climates_item_t *item = dynamic_cast<gui_climates_item_t*>( cb_climates.get_selected_item() ) ) {
		return item->get_climate();
	}
	return ALL_CLIMATES;
}

uint8 extend_edit_gui_t::get_sortedby() const
{
	if (gui_sorting_item_t *item = dynamic_cast<gui_sorting_item_t*>( cb_sortedby.get_selected_item() ) ) {
		return item->get_sortedby();
	}
	return ALL_CLIMATES;
}
