/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * The trees builder
 */

#include <stdio.h>
#include "../player/finance.h" // convert_money

#include "../world/simworld.h"
#include "../tool/simtool.h"
#include "../tool/simmenu.h"

#include "../dataobj/translator.h"


#include "../descriptor/image.h"
#include "../descriptor/ground_desc.h"

#include "../utils/cbuffer.h"
#include "../utils/simrandom.h"
#include "../utils/simstring.h"

#include "groundobj_edit.h"


// new tool definition
tool_plant_groundobj_t groundobj_edit_frame_t::groundobj_tool;
cbuffer_t groundobj_edit_frame_t::param_str;
bool groundobj_edit_frame_t::sortreverse = false;


static bool compare_groundobj_desc(const groundobj_desc_t* a, const groundobj_desc_t* b)
{
	int diff = strcmp( a->get_name(), b->get_name() );
	return groundobj_edit_frame_t::sortreverse ? diff > 0 : diff < 0;
}
static bool compare_groundobj_desc_name(const groundobj_desc_t* a, const groundobj_desc_t* b)
{
	int diff = strcmp( translator::translate(a->get_name()), translator::translate(b->get_name()) );
	if(diff ==0) {
		diff = strcmp( a->get_name(), b->get_name() );
	}
	return groundobj_edit_frame_t::sortreverse ? diff > 0 : diff < 0;
}
static bool compare_groundobj_desc_cost(const groundobj_desc_t* a, const groundobj_desc_t* b)
{
	int diff = a->get_price() - b->get_price();
	if(diff ==0) {
		diff = strcmp( a->get_name(), b->get_name() );
	}
	return groundobj_edit_frame_t::sortreverse ? diff > 0 : diff < 0;
}


groundobj_edit_frame_t::groundobj_edit_frame_t(player_t* player_) :
	extend_edit_gui_t(translator::translate("groundobj builder"), player_),
	groundobj_list(16)
{
	cont_timeline.set_visible(false);
	cb_sortedby.new_component<gui_sorting_item_t>(gui_sorting_item_t::BY_REMOVAL);

	desc = NULL;
	groundobj_tool.set_default_param(NULL);

	fill_list();

	// since we do not have a building image, we have to add the image again ourselves ...
	cont_scrolly.remove_all();
	cont_scrolly.set_table_layout(2, 0);
	cont_scrolly.set_margin(scr_size(0, D_V_SPACE), scr_size(0, D_V_SPACE));
	// add object description
	cont_scrolly.add_component(&info_text, 2);
	// add object image
	cont_scrolly.add_component(&groundobj_image);
	cont_scrolly.new_component<gui_fill_t>(true, false);
	cont_scrolly.new_component_span<gui_fill_t>(false, true, 2);
}


// fill the current groundobj_list
void groundobj_edit_frame_t::fill_list()
{
	groundobj_list.clear();
	const uint8 sortedby = get_sortedby();
	sortreverse = sort_order.pressed;
	for(groundobj_desc_t const* const i : groundobj_t::get_all_desc()) {
		if ( i  &&  (i->get_allowed_climate_bits() & get_climate()) ) {
			switch(sortedby) {
				case gui_sorting_item_t::BY_NAME_TRANSLATED:
					groundobj_list.insert_ordered( i, compare_groundobj_desc_name );
					break;
				case gui_sorting_item_t::BY_REMOVAL:
					groundobj_list.insert_ordered( i, compare_groundobj_desc_cost );
					break;
				default:
					groundobj_list.insert_ordered( i, compare_groundobj_desc );
			}
		}
	}

	// now build scrolled list
	scl.clear_elements();
	scl.set_selection(-1);
	for(groundobj_desc_t const* const i : groundobj_list) {
		char const* const name = sortedby==gui_sorting_item_t::BY_NAME_OBJECT ?  i->get_name() : translator::translate(i->get_name());
		scl.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(name, SYSCOL_TEXT);
		if (i == desc) {
			scl.set_selection(scl.get_count()-1);
		}
	}
	// always update current selection (since the tool may depend on it)
	change_item_info( scl.get_selection() );
}



void groundobj_edit_frame_t::change_item_info(sint32 entry)
{
	buf.clear();
	if(entry>=0  &&  entry<(sint32)groundobj_list.get_count()) {

		desc = groundobj_list[entry];

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

		uint8 seasons = desc->get_seasons();
		if( seasons > 1){
			buf.printf( "\n%s\n", translator::translate("Has Snow"));
			buf.printf( "%s %i\n", translator::translate("Seasons"), desc->get_seasons()-1 );
		}
		if(desc->get_phases() > 2){
			buf.printf( "\n%s\n", translator::translate("Has slope graphics"));
		}
		if(desc->can_build_trees_here()){
			buf.printf( "\n%s\n", translator::translate("Can be overgrown") );
		}
		buf.printf("\n%s ", translator::translate("cost for removal"));
		buf.append_money( convert_money( desc->get_price() ) );
		buf.append("\n");

		if (char const* const maker = desc->get_copyright()) {
			buf.append("\n");
			buf.printf(translator::translate("Constructed by %s"), maker);
			buf.append("\n");
		}

		groundobj_image.set_image(desc->get_image_id( seasons>2 ? 2 : 0, 0 ), true);

		param_str.clear();
		param_str.printf( "%i%i,%s", bt_climates.pressed, bt_timeline.pressed, desc->get_name() );
		groundobj_tool.set_default_param(param_str);
		groundobj_tool.cursor = tool_t::general_tool[TOOL_PLANT_GROUNDOBJ]->cursor;
		welt->set_tool( &groundobj_tool, player );
	}
	else if(welt->get_tool(player->get_player_nr())==&groundobj_tool) {
		desc = NULL;
		groundobj_image.set_image(IMG_EMPTY, true);
		welt->set_tool( tool_t::general_tool[TOOL_QUERY], player );
	}
	info_text.recalc_size();
	reset_min_windowsize();
}
