/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "label_info.h"
#include "../simworld.h"
#include "../simcolor.h"
#include "../gui/simwin.h"
#include "../simmenu.h"
#include "../descriptor/skin_desc.h"
#include "../dataobj/translator.h"
#include "../obj/label.h"
#include "../player/simplay.h"
#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"






label_info_t::label_info_t(label_t* l) :
	gui_frame_t( translator::translate("Marker"), l->get_owner()),
	player_name(""),
	view(l->get_pos(), scr_size( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width()*7)/8) ))
{

	label = l;

	const char *const p_name = label->get_owner()->get_name();
	const int min_width = max(290, display_calc_proportional_string_len_width(p_name, strlen(p_name)) + view.get_size().w + 30);

	view.set_pos( scr_coord(min_width - view.get_size().w - 10 , 21) );
	add_component( &view );

	grund_t *gr = welt->lookup(l->get_pos());
	if(gr && gr->get_text()) {
		tstrncpy(edit_name, gr->get_text(), lengthof(edit_name));
	}
	else {
		edit_name[0] = '\0';
	}
	// text input
	input.set_pos(scr_coord(10,4));
	input.set_size(scr_size(min_width-20, 13));
	input.set_text(edit_name, lengthof(edit_name));
	add_component(&input);
	input.add_listener(this);

	// text (player name)
	player_name.set_pos(scr_coord(10, 21));
	player_name.set_text(p_name);
	add_component(&player_name);

	set_focus(&input);
	set_windowsize(scr_size(min_width, view.get_size().h+47));
}



/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool label_info_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(comp == &input  &&  welt->get_active_player()==label->get_owner()) {
		// check owner to change text
		const grund_t *ground = welt->lookup(label->get_pos());
		if(ground && ((ground->get_text() == NULL && edit_name[0] != '\0') || strcmp(ground->get_text(), edit_name) != 0))
		{
			// text changed => call tool
			cbuffer_t buf;
			buf.printf( "m%s,%s", label->get_pos().get_str(), edit_name );
			tool_t *tool = create_tool( TOOL_RENAME | SIMPLE_TOOL );
			tool->set_default_param( buf );
			welt->set_tool( tool, label->get_owner() );
			// since init always returns false, it is safe to delete immediately
			delete tool;
		}
		destroy_win(this);
	}

	return true;
}



void label_info_t::map_rotate90( sint16 new_ysize )
{
	view.map_rotate90(new_ysize);
}
