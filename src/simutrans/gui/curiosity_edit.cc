/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../world/simworld.h"
#include "../tool/simtool.h"

#include "../builder/hausbauer.h"

#include "../descriptor/ground_desc.h"
#include "../descriptor/intro_dates.h"

#include "../dataobj/translator.h"

#include "../utils/cbuffer.h"


#include "curiosity_edit.h"
#include "components/gui_label.h"


// new tool definition
tool_build_house_t curiosity_edit_frame_t::haus_tool=tool_build_house_t();
cbuffer_t curiosity_edit_frame_t::param_str;
bool curiosity_edit_frame_t::sortreverse = false;


static bool compare_building_desc(const building_desc_t* a, const building_desc_t* b)
{
	int diff = strcmp( a->get_name(), b->get_name() );
	return curiosity_edit_frame_t::sortreverse ? diff > 0 : diff < 0;
}
static bool compare_building_desc_name(const building_desc_t* a, const building_desc_t* b)
{
	int diff = strcmp( translator::translate(a->get_name()), translator::translate(b->get_name()) );
	if(  diff==0  ) {
		diff = strcmp(a->get_name(), b->get_name());
	}
	return curiosity_edit_frame_t::sortreverse ? diff > 0 : diff < 0;
}
static bool compare_building_desc_level_pax(const building_desc_t* a, const building_desc_t* b)
{
	int diff = a->get_level() - b->get_level();
	if(  diff==0  ) {
		diff = strcmp(a->get_name(), b->get_name());
	}
	return curiosity_edit_frame_t::sortreverse ? diff > 0 : diff < 0;
}
static bool compare_building_desc_level_mail(const building_desc_t* a, const building_desc_t* b)
{
	int diff = a->get_mail_level() - b->get_mail_level();
	if(  diff==0  ) {
		diff = strcmp(a->get_name(), b->get_name());
	}
	return curiosity_edit_frame_t::sortreverse ? diff > 0 : diff < 0;
}
static bool compare_building_desc_date_intro(const building_desc_t* a, const building_desc_t* b)
{
	int diff = a->get_intro_year_month() - b->get_intro_year_month();
	if(  diff==0  ) {
		diff = strcmp(a->get_name(), b->get_name());
	}
	return curiosity_edit_frame_t::sortreverse ? diff > 0 : diff < 0;
}
static bool compare_building_desc_date_retire(const building_desc_t* a, const building_desc_t* b)
{
	int diff = a->get_retire_year_month() - b->get_retire_year_month();
	if(  diff==0  ) {
		diff = strcmp(a->get_name(), b->get_name());
	}
	return curiosity_edit_frame_t::sortreverse ? diff > 0 : diff < 0;
}
static bool compare_building_desc_size(const building_desc_t* a, const building_desc_t* b)
{
	koord a_koord = a->get_size();
	koord b_koord = b->get_size();
	int diff = a_koord.x * a_koord.y - b_koord.x * b_koord.y;
	if(  diff==0  ) {
		//same area - sort by side to seperate different shapes
		diff = a_koord.x - b_koord.x;
	}
	if(  diff==0  ) {
		diff = strcmp(a->get_name(), b->get_name());
	}
	return curiosity_edit_frame_t::sortreverse ? diff > 0 : diff < 0;
}

curiosity_edit_frame_t::curiosity_edit_frame_t(player_t* player_) :
	extend_edit_gui_t(translator::translate("curiosity builder"), player_),
	building_list(16)
{
	desc = NULL;
	haus_tool.set_default_param(NULL);
	haus_tool.cursor = tool_t::general_tool[TOOL_BUILD_HOUSE]->cursor;

	bt_city_attraction.init( button_t::square_state, "City attraction");
	bt_city_attraction.add_listener(this);
	bt_city_attraction.pressed = true;
	cont_filter.add_component(&bt_city_attraction,3);

	bt_land_attraction.init( button_t::square_state, "Land attraction");
	bt_land_attraction.add_listener(this);
	bt_land_attraction.pressed = true;
	cont_filter.add_component(&bt_land_attraction,3);

	bt_monuments.init( button_t::square_state, "Monument");
	bt_monuments.add_listener(this);
	cont_filter.add_component(&bt_monuments,3);

	// add to sorting selection
	cb_sortedby.new_component<gui_sorting_item_t>(gui_sorting_item_t::BY_LEVEL_PAX);
	cb_sortedby.new_component<gui_sorting_item_t>(gui_sorting_item_t::BY_LEVEL_MAIL);
	cb_sortedby.new_component<gui_sorting_item_t>(gui_sorting_item_t::BY_DATE_INTRO);
	cb_sortedby.new_component<gui_sorting_item_t>(gui_sorting_item_t::BY_DATE_RETIRE);
	cb_sortedby.new_component<gui_sorting_item_t>(gui_sorting_item_t::BY_SIZE);


	// rotation
	gui_aligned_container_t *tbl = cont_options.add_table(2,0);
	tbl->new_component<gui_label_t>("Rotation");
	tbl->add_component(&cb_rotation);
	cb_rotation.add_listener(this);
	cb_rotation.new_component<gui_rotation_item_t>(gui_rotation_item_t::random);
	cont_options.end_table();

	fill_list();

	reset_min_windowsize();
}


// put item in list according to filter/sorter
void curiosity_edit_frame_t::put_item_in_list( const building_desc_t* desc )
{
	const bool allow_obsolete = bt_obsolete.pressed;
	const bool use_timeline = bt_timeline.pressed | bt_timeline_custom.pressed;
	const sint32 month_now = bt_timeline.pressed ? welt->get_current_month() : bt_timeline_custom.pressed ? ni_timeline_year.get_value()*12 + ni_timeline_month.get_value()-1 : 0;
	const uint8 chosen_climate = get_climate();
	const uint8 sortedby = get_sortedby();
	sortreverse = sort_order.pressed;
	if( (!use_timeline  ||  (!desc->is_future(month_now)  &&  (!desc->is_retired(month_now)  ||  allow_obsolete)) )
		&&  ( desc->get_allowed_climate_bits() & chosen_climate) ) {
		// timeline allows for this, and so does climates setting
		switch(sortedby) {
			case gui_sorting_item_t::BY_NAME_TRANSLATED:     building_list.insert_ordered( desc, compare_building_desc_name );           break;
			case gui_sorting_item_t::BY_LEVEL_PAX:           building_list.insert_ordered( desc, compare_building_desc_level_pax );      break;
			case gui_sorting_item_t::BY_LEVEL_MAIL:          building_list.insert_ordered( desc, compare_building_desc_level_mail );     break;
			case gui_sorting_item_t::BY_DATE_INTRO:          building_list.insert_ordered( desc, compare_building_desc_date_intro );     break;
			case gui_sorting_item_t::BY_DATE_RETIRE:         building_list.insert_ordered( desc, compare_building_desc_date_retire );    break;
			case gui_sorting_item_t::BY_SIZE:                building_list.insert_ordered( desc, compare_building_desc_size );           break;
			default:                                         building_list.insert_ordered( desc, compare_building_desc );
		}
	}
}
// fill the current building_list
void curiosity_edit_frame_t::fill_list()
{
	building_list.clear();

	if(bt_city_attraction.pressed) {
		FOR(vector_tpl<building_desc_t const*>, const desc, *hausbauer_t::get_list(building_desc_t::attraction_city)) {
			put_item_in_list(desc);
		}
	}

	if(bt_land_attraction.pressed) {
		FOR(vector_tpl<building_desc_t const*>, const desc, *hausbauer_t::get_list(building_desc_t::attraction_land)) {
			put_item_in_list(desc);
		}
	}

	if(bt_monuments.pressed) {
		FOR(vector_tpl<building_desc_t const*>, const desc, *hausbauer_t::get_list(building_desc_t::monument)) {
			put_item_in_list(desc);
		}
	}

	// now build scrolled list
	scl.clear_elements();
	scl.set_selection(-1);
	FOR(vector_tpl<building_desc_t const*>, const i, building_list) {
		// color code for objects: BLACK: normal, YELLOW: consumer only, GREEN: source only
		PIXVAL color;
		switch (i->get_type()) {
			case building_desc_t::attraction_city: color = color_idx_to_rgb(COL_DARK_BLUE+env_t::gui_player_color_dark); break;
			case building_desc_t::attraction_land: color = color_idx_to_rgb(40 + env_t::gui_player_color_dark);          break;
			default:                               color = SYSCOL_TEXT;                                                  break;
		}
		char const* const name = get_sortedby()==gui_sorting_item_t::BY_NAME_OBJECT ?  i->get_name() : translator::translate(i->get_name());
		scl.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(name, color);
		if (i == desc) {
			scl.set_selection(scl.get_count()-1);
		}
	}
	// always update current selection (since the tool may depend on it)
	change_item_info( scl.get_selection() );
}



bool curiosity_edit_frame_t::action_triggered( gui_action_creator_t *comp,value_t e)
{
	// only one chain can be shown
	if(  comp==&bt_city_attraction  ) {
		bt_city_attraction.pressed ^= 1;
		fill_list();
	}
	else if(  comp==&bt_land_attraction  ) {
		bt_land_attraction.pressed ^= 1;
		fill_list();
	}
	else if(  comp==&bt_monuments  ) {
		bt_monuments.pressed ^= 1;
		fill_list();
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

			buf.trim();
			buf.append("\n\n");

			// climates
			buf.append( translator::translate("allowed climates:\n") );
			uint16 cl = desc->get_allowed_climate_bits();
			if(cl==0) {
				buf.append( translator::translate("none") );
				buf.append("\n");
			}
			else {
				for(uint16 i=0;  i<=arctic_climate;  i++  ) {
					if(cl &  (1<<i)) {
						buf.append(" - ");
						buf.append(translator::translate(ground_desc_t::get_climate_name_from_bit((climate)i)));
						buf.append("\n");
					}
				}
			}
			buf.append("\n");

			buf.printf("%s: %i\n",translator::translate("Passagierrate"),desc->get_level());
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
		haus_tool.set_default_param(param_str);
		welt->set_tool( &haus_tool, player );
	}
	else {
		desc = NULL;
		if(welt->get_tool(player->get_player_nr())==&haus_tool) {
			welt->set_tool( tool_t::general_tool[TOOL_QUERY], player );
		}
		building_image.init(NULL, 0);
		cb_rotation.clear_elements();
		cb_rotation.new_component<gui_rotation_item_t>(gui_rotation_item_t::random);
		buf.clear();
	}
	reset_min_windowsize();
}


void curiosity_edit_frame_t::draw(scr_coord pos, scr_size size)
{
	// remove constructed monuments from list
	if(desc  &&  desc->get_type()==building_desc_t::monument  &&  !hausbauer_t::is_valid_monument(desc)  ) {
		change_item_info(0x7FFFFFFF);
		scl.set_selection(-1);
		fill_list();
	}

	extend_edit_gui_t::draw(pos,size);
}
