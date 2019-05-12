/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Curiosity (attractions) builder dialog
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simtool.h"

#include "../bauer/hausbauer.h"

#include "../descriptor/ground_desc.h"
#include "../descriptor/intro_dates.h"

#include "../dataobj/translator.h"

#include "../utils/cbuffer_t.h"


#include "curiosity_edit.h"
#include "components/gui_label.h"


// new tool definition
tool_build_house_t* curiosity_edit_frame_t::haus_tool=new tool_build_house_t();
cbuffer_t curiosity_edit_frame_t::param_str;


static bool compare_building_desc(const building_desc_t* a, const building_desc_t* b)
{
	int diff = strcmp( a->get_name(), b->get_name() );
	return diff < 0;
}


static bool compare_building_desc_trans(const building_desc_t* a, const building_desc_t* b)
{
	int diff = strcmp( translator::translate(a->get_name()), translator::translate(b->get_name()) );
	return diff < 0;
}



curiosity_edit_frame_t::curiosity_edit_frame_t(player_t* player_) :
	extend_edit_gui_t(translator::translate("curiosity builder"), player_),
	building_list(16)
{
	desc = NULL;
	haus_tool->set_default_param(NULL);
	haus_tool->cursor = tool_t::general_tool[TOOL_BUILD_HOUSE]->cursor;

	bt_city_attraction.init( button_t::square_state, "City attraction");
	bt_city_attraction.add_listener(this);
	bt_city_attraction.pressed = true;
	cont_right.add_component(&bt_city_attraction);

	bt_land_attraction.init( button_t::square_state, "Land attraction");
	bt_land_attraction.add_listener(this);
	bt_land_attraction.pressed = true;
	cont_right.add_component(&bt_land_attraction);

	bt_monuments.init( button_t::square_state, "Monument");
	bt_monuments.add_listener(this);
	cont_right.add_component(&bt_monuments);

	// rotation
	gui_aligned_container_t *tbl = cont_right.add_table(2,0);
	tbl->new_component<gui_label_t>("Rotation");
	tbl->add_component(&cb_rotation);
	cb_rotation.add_listener(this);
	cb_rotation.new_component<gui_rotation_item_t>(gui_rotation_item_t::random);
	cont_right.end_table();

	fill_list( is_show_trans_name );

	reset_min_windowsize();
}



// fill the current building_list
void curiosity_edit_frame_t::fill_list( bool translate )
{
	const bool allow_obsolete = bt_obsolete.pressed;
	const bool use_timeline = bt_timeline.pressed;
	const sint32 month_now = bt_timeline.pressed ? welt->get_current_month() : 0;

	building_list.clear();

	if(bt_city_attraction.pressed) {
		FOR(vector_tpl<building_desc_t const*>, const desc, *hausbauer_t::get_list(building_desc_t::attraction_city)) {
			if(!use_timeline  ||  (!desc->is_future(month_now)  &&  (!desc->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				building_list.insert_ordered( desc, translate?compare_building_desc_trans:compare_building_desc );
			}
		}
	}

	if(bt_land_attraction.pressed) {
		FOR(vector_tpl<building_desc_t const*>, const desc, *hausbauer_t::get_list(building_desc_t::attraction_land)) {
			if(!use_timeline  ||  (!desc->is_future(month_now)  &&  (!desc->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				building_list.insert_ordered( desc, translate?compare_building_desc_trans:compare_building_desc );
			}
		}
	}

	if(bt_monuments.pressed) {
		FOR(vector_tpl<building_desc_t const*>, const desc, *hausbauer_t::get_list(building_desc_t::monument)) {
			if(!use_timeline  ||  (!desc->is_future(month_now)  &&  (!desc->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				building_list.insert_ordered( desc, translate?compare_building_desc_trans:compare_building_desc );
			}
		}
	}

	// now build scrolled list
	scl.clear_elements();
	scl.set_selection(-1);
	FOR(vector_tpl<building_desc_t const*>, const i, building_list) {
		// color code for objects: BLACK: normal, YELLOW: consumer only, GREEN: source only
		PIXVAL color;
		switch (i->get_type()) {
			case building_desc_t::attraction_city: color = color_idx_to_rgb(COL_BLUE);       break;
			case building_desc_t::attraction_land: color = color_idx_to_rgb(COL_DARK_GREEN); break;
			default:                               color = color_idx_to_rgb(COL_BLACK);      break;
		}
		char const* const name = translate ? translator::translate(i->get_name()) : i->get_name();
		scl.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(name, color);
		if (i == desc) {
			scl.set_selection(scl.get_count()-1);
		}
	}
	scr_coord_val em = display_get_char_width('m');
	scl.set_max_width(30*em);
	// always update current selection (since the tool may depend on it)
	change_item_info( scl.get_selection() );
}



bool curiosity_edit_frame_t::action_triggered( gui_action_creator_t *comp,value_t e)
{
	// only one chain can be shown
	if(  comp==&bt_city_attraction  ) {
		bt_city_attraction.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(  comp==&bt_land_attraction  ) {
		bt_land_attraction.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(  comp==&bt_monuments  ) {
		bt_monuments.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if( comp == &cb_rotation) {
		change_item_info( scl.get_selection() );
	}
	return extend_edit_gui_t::action_triggered(comp,e);
}



void curiosity_edit_frame_t::change_item_info(sint32 entry)
{
	if(entry>=0  &&  entry<(sint32)building_list.get_count()) {

		const building_desc_t *new_desc = building_list[entry];

		if(new_desc!=desc) {

			buf.clear();
			desc = new_desc;
			if(desc->get_type()==building_desc_t::attraction_city) {
				buf.printf("%s (%s: %i)",translator::translate( "City attraction" ), translator::translate("Bauzeit"),desc->get_extra());
			}
			else if(desc->get_type()==building_desc_t::attraction_land) {
				buf.append( translator::translate( "Land attraction" ) );
			}
			else if(desc->get_type()==building_desc_t::monument) {
				buf.append( translator::translate( "Monument" ) );
			}
			buf.append("\n\n");
			buf.append( translator::translate( desc->get_name() ) );

			buf.printf("\n\n%s: %i\n",translator::translate("Passagierrate"),desc->get_level());
			if(desc->get_type()==building_desc_t::attraction_land) {
				// same with passengers
				buf.printf("%s: %i\n",translator::translate("Postrate"),desc->get_level());
			}
			else {
				buf.printf("%s: %i\n",translator::translate("Postrate"),desc->get_mail_level());
			}

			buf.printf("%s%u", translator::translate("\nBauzeit von"), desc->get_intro_year_month() / 12);
			if(desc->get_retire_year_month()!=DEFAULT_RETIRE_DATE*12) {
				buf.printf("%s%u", translator::translate("\nBauzeit bis"), desc->get_retire_year_month() / 12);
			}
			buf.append("\n");

			if (char const* const maker = desc->get_copyright()) {
				buf.append("\n");
				buf.printf(translator::translate("Constructed by %s"), maker);
				buf.append("\n");
			}

			// reset combobox
			cb_rotation.clear_elements();
			cb_rotation.new_component<gui_rotation_item_t>(gui_rotation_item_t::random);
			for(uint8 i = 0; i<desc->get_all_layouts(); i++) {
				cb_rotation.new_component<gui_rotation_item_t>(i);
			}
			// orientation (255=random)
			if(desc->get_all_layouts()>1) {
				cb_rotation.set_selection(0);
			}
			else {
				cb_rotation.set_selection(1);
			}
		}
		uint8 rotation = get_rotation();
		uint8 rot = (rotation==255) ? 0 : rotation;
		building_image.init(desc, rot);

		// the tools will be always updated, even though the data up there might be still current
		param_str.clear();
		param_str.printf("%i%c%s", bt_climates.pressed, rotation==255 ? '#' : '0'+rotation, desc->get_name() );
		haus_tool->set_default_param(param_str);
		welt->set_tool( haus_tool, player );
	}
	else {
		desc = NULL;
		if(welt->get_tool(player->get_player_nr())==haus_tool) {
			welt->set_tool( tool_t::general_tool[TOOL_QUERY], player );
		}
		building_image.init(NULL, 0);
		cb_rotation.clear_elements();
		cb_rotation.new_component<gui_rotation_item_t>(gui_rotation_item_t::random);
	}
	reset_min_windowsize();
}


void curiosity_edit_frame_t::draw(scr_coord pos, scr_size size)
{
	// remove constructed monuments from list
	if(desc  &&  desc->get_type()==building_desc_t::monument  &&  !hausbauer_t::is_valid_monument(desc)  ) {
		change_item_info(0x7FFFFFFF);
		scl.set_selection(-1);
		fill_list( is_show_trans_name );
	}

	extend_edit_gui_t::draw(pos,size);
}
