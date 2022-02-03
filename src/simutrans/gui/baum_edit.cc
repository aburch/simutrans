/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * The trees builder
 */

#include <stdio.h>

#include "../world/simworld.h"
#include "../tool/simtool.h"
#include "../tool/simmenu.h"

#include "../dataobj/translator.h"


#include "../descriptor/image.h"
#include "../descriptor/ground_desc.h"

#include "../utils/cbuffer.h"
#include "../utils/simrandom.h"
#include "../utils/simstring.h"

#include "baum_edit.h"


// new tool definition
tool_plant_tree_t baum_edit_frame_t::baum_tool;
cbuffer_t baum_edit_frame_t::param_str;
bool baum_edit_frame_t::sortreverse = false;


static bool compare_tree_desc(const tree_desc_t* a, const tree_desc_t* b)
{
	int diff = strcmp( a->get_name(), b->get_name() );
	return baum_edit_frame_t::sortreverse ? diff > 0 : diff < 0;
}
static bool compare_tree_desc_name(const tree_desc_t* a, const tree_desc_t* b)
{
	int diff = strcmp( translator::translate(a->get_name()), translator::translate(b->get_name()) );
	if(diff ==0) {
		diff = strcmp( a->get_name(), b->get_name() );
	}
	return baum_edit_frame_t::sortreverse ? diff > 0 : diff < 0;
}


baum_edit_frame_t::baum_edit_frame_t(player_t* player_) :
	extend_edit_gui_t(translator::translate("baum builder"), player_),
	tree_list(16)
{
	cont_timeline.set_visible(false);

	bt_randomage.init( button_t::square_state, "Random age");
	bt_randomage.add_listener(this);
	bt_randomage.pressed = true;
	cont_options.add_component(&bt_randomage);

	desc = NULL;
	baum_tool.set_default_param(NULL);

	fill_list();

	// since we do not have a building image, we have to add the image again ourselves ...
	cont_scrolly.remove_all();
	cont_scrolly.set_table_layout(2, 0);
	cont_scrolly.set_margin(scr_size(0, D_V_SPACE), scr_size(0, D_V_SPACE));
	// add object description
	cont_scrolly.add_component(&info_text, 2);
	// add object image
	cont_scrolly.add_component(&tree_image);
	cont_scrolly.new_component<gui_fill_t>(true, false);
	cont_scrolly.new_component_span<gui_fill_t>(false, true, 2);
}



// fill the current tree_list
void baum_edit_frame_t::fill_list()
{
	tree_list.clear();
	const bool is_sortedbyname = get_sortedby()==gui_sorting_item_t::BY_NAME_TRANSLATED;
	sortreverse = sort_order.pressed;
	for(tree_desc_t const* const i : tree_builder_t::get_all_desc()) {
		if ( i  &&  (i->get_allowed_climate_bits() & get_climate()) ) {
			tree_list.insert_ordered(i, is_sortedbyname ? compare_tree_desc_name : compare_tree_desc);
		}
	}

	// now build scrolled list
	scl.clear_elements();
	scl.set_selection(-1);
	for(tree_desc_t const* const i : tree_list) {
		char const* const name = get_sortedby()==gui_sorting_item_t::BY_NAME_OBJECT ?  i->get_name() : translator::translate(i->get_name());
		scl.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(name, SYSCOL_TEXT);
		if (i == desc) {
			scl.set_selection(scl.get_count()-1);
		}
	}
	// always update current selection (since the tool may depend on it)
	change_item_info( scl.get_selection() );
}

bool baum_edit_frame_t::action_triggered( gui_action_creator_t *comp,value_t e)
{
	if(  comp==&bt_randomage  ) {
		bt_randomage.pressed ^= 1;
		change_item_info( scl.get_selection() );
	}
	return extend_edit_gui_t::action_triggered(comp,e);
}


void baum_edit_frame_t::change_item_info(sint32 entry)
{
	buf.clear();
	if(entry>=0  &&  entry<(sint32)tree_list.get_count()) {

		desc = tree_list[entry];

		buf.append(translator::translate(desc->get_name()));
		buf.append("\n\n");

		// climates
		buf.append( translator::translate("allowed climates:\n") );
		uint16 cl = desc->get_allowed_climate_bits();
		if(cl==0) {
			buf.append( translator::translate("None") );
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

		buf.printf( "\n%s %i\n", translator::translate("Seasons"), desc->get_seasons() );

		if (char const* const maker = desc->get_copyright()) {
			buf.append("\n");
			buf.printf(translator::translate("Constructed by %s"), maker);
			buf.append("\n");
		}

		tree_image.set_image(desc->get_image_id( 0, 3 ), true);

		param_str.clear();
		param_str.printf( "%i%i,%s", bt_climates.pressed, bt_randomage.pressed, desc->get_name() );
		baum_tool.set_default_param(param_str);
		baum_tool.cursor = tool_t::general_tool[TOOL_PLANT_TREE]->cursor;
		welt->set_tool( &baum_tool, player );
	}
	else if(welt->get_tool(player->get_player_nr())==&baum_tool) {
		desc = NULL;
		tree_image.set_image(IMG_EMPTY, true);
		welt->set_tool( tool_t::general_tool[TOOL_QUERY], player );
	}
	info_text.recalc_size();
	reset_min_windowsize();
}
