/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * The citybuilding editor (urban buildings builder)
 */

#include <algorithm>
#include <stdio.h>

#include "../simworld.h"
#include "../simtool.h"

#include "../bauer/hausbauer.h"

#include "../descriptor/ground_desc.h"
#include "../descriptor/intro_dates.h"

#include "../dataobj/translator.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simrandom.h"
#include "../utils/simstring.h"

#include "citybuilding_edit.h"


// new tool definition
tool_build_house_t* citybuilding_edit_frame_t::haus_tool=new tool_build_house_t();
char citybuilding_edit_frame_t::param_str[256];



static bool compare_building_desc(const building_desc_t* a, const building_desc_t* b)
{
	int diff = a->get_level()-b->get_level();
	if(  diff==0  ) {
		diff = a->get_type()-b->get_type();
	}
	if(  diff==0  ) {
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}



static bool compare_building_desc_trans(const building_desc_t* a, const building_desc_t* b)
{
	int diff = strcmp( translator::translate(a->get_name()), translator::translate(b->get_name()) );
	if(  diff==0  ) {
		diff = a->get_level()-b->get_level();
	}
	if(  diff==0  ) {
		diff = a->get_type()-b->get_type();
	}
	return diff < 0;
}



citybuilding_edit_frame_t::citybuilding_edit_frame_t(player_t* player_) :
	extend_edit_gui_t(translator::translate("citybuilding builder"), player_),
	building_list(16),
	lb_rotation( rot_str, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right ),
	lb_rotation_info( translator::translate("Rotation"), SYSCOL_TEXT, gui_label_t::left )
{
	rot_str[0] = 0;
	desc = NULL;
	haus_tool->set_default_param(NULL);
	haus_tool->cursor = tool_t::general_tool[TOOL_BUILD_HOUSE]->cursor;

	bt_res.init( button_t::square_state, "residential house", scr_coord(get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	bt_res.add_listener(this);
	bt_res.pressed = true;
	add_component(&bt_res);
	offset_of_comp += D_BUTTON_HEIGHT;

	bt_com.init( button_t::square_state, "shops and stores", scr_coord(get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	bt_com.add_listener(this);
	bt_com.pressed = true;
	add_component(&bt_com);
	offset_of_comp += D_BUTTON_HEIGHT;

	bt_ind.init( button_t::square_state, "industrial building", scr_coord(get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	bt_ind.add_listener(this);
	add_component(&bt_ind);
	bt_com.pressed = true;
	offset_of_comp += D_BUTTON_HEIGHT;

	lb_rotation_info.set_pos( scr_coord( get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	add_component(&lb_rotation_info);

	bt_left_rotate.init( button_t::repeatarrowleft, NULL, scr_coord(get_tab_panel_width()+2*MARGIN+COLUMN_WIDTH/2-16,	offset_of_comp-4 ) );
	bt_left_rotate.add_listener(this);
	add_component(&bt_left_rotate);

	bt_right_rotate.init( button_t::repeatarrowright, NULL, scr_coord(get_tab_panel_width()+2*MARGIN+COLUMN_WIDTH/2+50, offset_of_comp-4 ) );
	bt_right_rotate.add_listener(this);
	add_component(&bt_right_rotate);

	//lb_rotation.set_pos( scr_coord( get_tab_panel_width()+2*MARGIN+COLUMN_WIDTH/2+44, offset_of_comp-4 ) );
	lb_rotation.set_width( bt_right_rotate.get_pos().x - bt_left_rotate.get_pos().x - bt_left_rotate.get_size().w );
	lb_rotation.align_to(&bt_left_rotate, ALIGN_LEFT | ALIGN_EXTERIOR_H | ALIGN_CENTER_V);
	add_component(&lb_rotation);
	offset_of_comp += D_BUTTON_HEIGHT;

	fill_list( is_show_trans_name );

	resize(scr_coord(0,0));
}



// fill the current building_list
void citybuilding_edit_frame_t::fill_list( bool translate )
{
	const bool allow_obsolete = bt_obsolete.pressed;
	const bool use_timeline = bt_timeline.pressed;
	const sint32 month_now = bt_timeline.pressed ? welt->get_current_month() : 0;

	building_list.clear();

	if(bt_res.pressed) {
		FOR(vector_tpl<building_desc_t const*>, const desc, *hausbauer_t::get_citybuilding_list(building_desc_t::city_res)) {
			if(!use_timeline  ||  (!desc->is_future(month_now)  &&  (!desc->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				building_list.insert_ordered(desc, translate?compare_building_desc_trans:compare_building_desc );
			}
		}
	}

	if(bt_com.pressed) {
		FOR(vector_tpl<building_desc_t const*>, const desc, *hausbauer_t::get_citybuilding_list(building_desc_t::city_com)) {
			if(!use_timeline  ||  (!desc->is_future(month_now)  &&  (!desc->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				building_list.insert_ordered(desc, translate?compare_building_desc_trans:compare_building_desc );
			}
		}
	}

	if(bt_ind.pressed) {
		FOR(vector_tpl<building_desc_t const*>, const desc, *hausbauer_t::get_citybuilding_list(building_desc_t::city_ind)) {
			if(!use_timeline  ||  (!desc->is_future(month_now)  &&  (!desc->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				building_list.insert_ordered(desc, translate?compare_building_desc_trans:compare_building_desc );
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
			case building_desc_t::city_res: color = color_idx_to_rgb(COL_BLUE);       break;
			case building_desc_t::city_com: color = color_idx_to_rgb(COL_DARK_GREEN); break;
			default:                        color = SYSCOL_TEXT;                      break;
		}
		char const* const name = translate ? translator::translate(i->get_name()) : i->get_name();
		scl.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(name, color));
		if (i == desc) {
			scl.set_selection(scl.get_count()-1);
		}
	}
	// always update current selection (since the tool may depend on it)
	change_item_info( scl.get_selection() );
}



bool citybuilding_edit_frame_t::action_triggered( gui_action_creator_t *komp,value_t e)
{
	// only one chain can be shown
	if(  komp==&bt_res  ) {
		bt_res.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(  komp==&bt_com  ) {
		bt_com.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(  komp==&bt_ind  ) {
		bt_ind.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(desc) {
		if(  komp==&bt_left_rotate  ) {
			// random, auto, max_rotaions-1 ... 0
			if(rotation==254) {
				rotation = desc->get_all_layouts()-1;
			}
			else {
				rotation --;
			}
		}
		else if(  komp==&bt_right_rotate  ) {
			// 0...max-rotation, auto, random
			if(  rotation==desc->get_all_layouts()-1  ) {
				rotation = 254;
			}
			else {
				rotation ++;
			}
		}
		// update info ...
		change_item_info( scl.get_selection() );
	}
	return extend_edit_gui_t::action_triggered(komp,e);
}



void citybuilding_edit_frame_t::change_item_info(sint32 entry)
{
	if(entry>=0  &&  entry<(sint32)building_list.get_count()) {

		const building_desc_t *new_desc = building_list[entry];
		if(new_desc!=desc) {

			buf.clear();
			desc = new_desc;
			if(desc->get_type()==building_desc_t::city_res) {
				buf.append( translator::translate( "residential house" ) );
			}
			else if(desc->get_type()==building_desc_t::city_com) {
				buf.append( translator::translate( "shops and stores" ) );
			}
			else if(desc->get_type()==building_desc_t::city_ind) {
				buf.append( translator::translate( "industrial building" ) );
			}
			buf.append("\n\n");
			buf.append( translator::translate( desc->get_name() ) );

			buf.printf("\n\n%s: %i\n",translator::translate("Passagierrate"),desc->get_level());
			buf.printf("%s: %i\n",translator::translate("Postrate"),desc->get_mail_level());

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

			info_text.recalc_size();
			cont.set_size( info_text.get_size() + scr_size(0, 20) );

			// orientation (254=auto, 255=random)
			if(desc->get_all_layouts()>1) {
				rotation = 254; // no definition yet, choose auto
			}
			else {
				rotation = 0;
			}
		}

		// change label numbers
		if(rotation == 255) {
			tstrncpy(rot_str, translator::translate("random"), lengthof(rot_str));
		}
		else if(rotation == 254) {
			tstrncpy(rot_str, translator::translate("auto"), lengthof(rot_str));
		}
		else {
			sprintf( rot_str, "%i", rotation );
		}

		// now the images (maximum is 2x2 size)
		// since these may be affected by rotation, we do this every time ...
		for(int i=0;  i<3;  i++  ) {
			img[i].set_image( IMG_EMPTY );
		}

		uint8 rot = (rotation>253) ? 0 : rotation;
		img[3].set_image( desc->get_tile(rot,0,0)->get_background(0,0,0) );

		// the tools will be always updated, even though the data up there might be still current
		sprintf( param_str, "%i%c%s", bt_climates.pressed, rotation>253 ? (rotation==254 ? 'A' : '#') : '0'+rotation, desc->get_name() );
		haus_tool->set_default_param(param_str);
		welt->set_tool( haus_tool, player );
	}
	else if(welt->get_tool(player->get_player_nr())==haus_tool) {
		desc = NULL;
		welt->set_tool( tool_t::general_tool[TOOL_QUERY], player );
	}
}
