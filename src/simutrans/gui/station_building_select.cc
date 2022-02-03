/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * Building facing selector, when CTRL+Clicking over a building icon in menu
 */
#include "../world/simworld.h"
#include "../tool/simtool.h"
#include "simwin.h"
#include "../utils/cbuffer.h"
#include "station_building_select.h"
#include "components/gui_label.h"
#include "components/gui_building.h"

#include "../descriptor/building_desc.h"


static const char label_text[4][64] = {
	"sued",
	"ost",
	"nord",
	"west"
};


tool_build_station_t station_building_select_t::tool=tool_build_station_t();


station_building_select_t::station_building_select_t(const building_desc_t *desc) :
	gui_frame_t( translator::translate("Choose direction") )
{
	this->desc = desc;

	set_table_layout(1,0);
	set_alignment(ALIGN_CENTER_H);

	gui_label_buf_t *lb = new_component<gui_label_buf_t>();
	lb->buf().printf( translator::translate("Width = %d, Height = %d"), desc->get_x(0), desc->get_y(0));
	lb->update();

	// the image array
	add_table(2,2);
	{
		for( sint16 i=0;  i<min(desc->get_all_layouts(),4);  i++ ) {
			add_table(1,0)->set_alignment(ALIGN_CENTER_H);
			{
				gui_building_t *g = new_component<gui_building_t>(desc, i);
				g->add_listener(this);

				actionbutton[i].init( button_t::roundbox | button_t::flexible, translator::translate(label_text[i]) );
				actionbutton[i].add_listener(this);
				add_component(&actionbutton[i]);
			}
			end_table();
		}

	}
	end_table();

	reset_min_windowsize();
}


/**
 * This method is called if an action is triggered
 */
bool station_building_select_t::action_triggered( gui_action_creator_t *comp, value_t v)
{
	for(int i=0; i<4; i++) {
		if(comp == &actionbutton[i]  ||  v.i == i) {
			static cbuffer_t default_str;
			default_str.clear();
			default_str.printf("%s,%i", desc->get_name(), i );
			tool.set_default_param(default_str);
			welt->set_tool( &tool, welt->get_active_player() );
			destroy_win(this);
		}
	}
	return true;
}
