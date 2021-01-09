/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * The trees builder
 */

#include <stdio.h>
#include "../player/finance.h" // convert_money

#include "../simworld.h"
#include "../simtool.h"
#include "../simmenu.h"

#include "../dataobj/translator.h"


#include "../descriptor/image.h"
#include "../descriptor/ground_desc.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simrandom.h"
#include "../utils/simstring.h"

#include "groundobj_edit.h"


// new tool definition
tool_plant_groundobj_t groundobj_edit_frame_t::groundobj_tool;
cbuffer_t groundobj_edit_frame_t::param_str;

static bool compare_groundobj_desc(const groundobj_desc_t* a, const groundobj_desc_t* b)
{
	int diff = strcmp( a->get_name(), b->get_name() );
	return diff < 0;
}
static bool compare_groundobj_desc_name(const groundobj_desc_t* a, const groundobj_desc_t* b)
{
	int diff = strcmp( translator::translate(a->get_name()), translator::translate(b->get_name()) );
	if(diff ==0) {
		diff = strcmp( a->get_name(), b->get_name() );
	}
	return diff < 0;
}
static bool compare_groundobj_desc_cost(const groundobj_desc_t* a, const groundobj_desc_t* b)
{
	int diff = a->get_price() - b->get_price();
	if(diff ==0) {
		diff = strcmp( a->get_name(), b->get_name() );
	}
	return diff < 0;
}


groundobj_edit_frame_t::groundobj_edit_frame_t(player_t* player_) :
	extend_edit_gui_t(translator::translate("groundobj builder"), player_),
	groundobj_list(16)
{
//	cont_timeline.set_visible(false);
	new_component_span<gui_label_t>( "Sort by", 2 );

	cb_sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Object"), SYSCOL_TEXT);
	cb_sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Translation"), SYSCOL_TEXT);
	cb_sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("cost for removal"), SYSCOL_TEXT);
	cb_sortedby.set_selection( 0 );
	cb_sortedby.add_listener(this);
	add_component(&cb_sortedby);

	desc = NULL;
	groundobj_tool.set_default_param(NULL);

	fill_list( is_show_trans_name );

	cont_right.add_component(&groundobj_image);
	building_image.set_visible(false);
}


// fill the current groundobj_list
void groundobj_edit_frame_t::fill_list( bool translate )
{
	groundobj_list.clear();
	const uint8 sortedby = cb_rotation.get_selection();
	FOR(vector_tpl<groundobj_desc_t const*>, const i, groundobj_t::get_all_desc()) {
		switch(sortedby) {
			case BY_TRANSLATION:
				groundobj_list.insert_ordered( i, compare_groundobj_desc_name );
				break;
			case BY_COST:
				groundobj_list.insert_ordered( i, compare_groundobj_desc_cost );
				break;
			default:
				groundobj_list.insert_ordered( i, compare_groundobj_desc );
		}
	}

	// now build scrolled list
	scl.clear_elements();
	scl.set_selection(-1);
	FOR(vector_tpl<groundobj_desc_t const*>, const i, groundobj_list) {
		char const* const name = translate ? translator::translate(i->get_name()): i->get_name();
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
		buf.append_money( desc->get_price()/100.0 );
		buf.append("$\n");

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
