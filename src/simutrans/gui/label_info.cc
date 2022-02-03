/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "label_info.h"
#include "components/gui_label.h"
#include "../world/simworld.h"
#include "simwin.h"
#include "../tool/simmenu.h"
#include "../obj/label.h"
#include "../player/simplay.h"
#include "../utils/simstring.h"
#include "../utils/cbuffer.h"


label_info_t::label_info_t(label_t* l) :
	gui_frame_t( translator::translate("Marker"), l->get_owner()),
	view(l->get_pos(), scr_size( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width()*7)/8) ))
{
	label = l;

	set_table_layout(1,0);
	// input
	add_component(&input);
	input.add_listener(this);

	add_table(3,0)->set_alignment(ALIGN_TOP);
	// left: player name
	new_component<gui_label_t>(label->get_owner()->get_name());
	new_component<gui_fill_t>();
	// right column: view
	add_component( &view );
	end_table();

	grund_t *gr = welt->lookup(l->get_pos());
	if(gr->get_text()) {
		tstrncpy(edit_name, gr->get_text(), lengthof(edit_name));
	}
	else {
		edit_name[0] = '\0';
	}
	// text input
	input.set_text(edit_name, lengthof(edit_name));
	set_focus(&input);

	reset_min_windowsize();
}


/**
 * This method is called if an action is triggered
 */
bool label_info_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(comp == &input  &&  welt->get_active_player()==label->get_owner()) {
		// check owner to change text
		grund_t *gd = welt->lookup(label->get_pos());
		if(  strcmp(gd->get_text(),edit_name)  ) {
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
