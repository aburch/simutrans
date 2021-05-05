/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_timeinput.h"
#include "../gui_frame.h"
#include "../simwin.h"
#include "../../display/simgraph.h"
#include "../../macros.h"
#include "../../dataobj/translator.h"
#include "../../utils/simstring.h"
#include "../../simintr.h"
#include "../../simworld.h"
#include "../../dataobj/environment.h"


gui_timeinput_t::gui_timeinput_t(const char *)
{
	const int max_days = env_t::show_month == env_t::DATE_FMT_MONTH ? 3 : 31;
	const scr_coord_val min_button_width = D_ARROW_LEFT_WIDTH + D_ARROW_RIGHT_WIDTH + 3 * D_H_SPACE + proportional_string_width("30");

	set_table_layout(4, 1);
	set_alignment(ALIGN_LEFT | ALIGN_TOP);
	set_margin(scr_size(D_H_SPACE, 0), scr_size(D_H_SPACE, 0));

	days.init(0, 0, max_days);
	days.add_listener(this);
	days.set_size(scr_size(min_button_width, D_EDIT_HEIGHT));
	add_component(&days);

	hours.init(0, 0, 23);
	hours.set_size(scr_size(min_button_width, D_EDIT_HEIGHT));
	hours.add_listener(this);
	add_component(&hours);

	new_component<gui_label_t>(":");

	minutes.init(0, 0, 59);
	minutes.set_size(scr_size(min_button_width, D_EDIT_HEIGHT));
	minutes.add_listener(this);
	add_component(&minutes);

	b_enabled = true;
}


scr_size gui_timeinput_t::get_min_size() const
{
	const int min_button_width = D_ARROW_LEFT_WIDTH + D_ARROW_RIGHT_WIDTH + 2 * D_H_SPACE + proportional_string_width("30");
	return scr_size(3 * min_button_width + 2 * D_H_SPACE, D_EDIT_HEIGHT);
}


sint32 gui_timeinput_t::get_ticks()
{
	sint32 dms = days.get_value() * 24 * 60 + hours.get_value() * 60 + minutes.get_value();
	if (dms == 0) {
		return 0;
	}
	if (env_t::show_month == env_t::DATE_FMT_MONTH) {
		return (dms * 65536u) / (3 * 24 * 60)+1;
	}
	return (dms * 65536u) / (31 * 24 * 60)+1;
}



void gui_timeinput_t::set_ticks(uint16 t)
{
	sint32 ticks = t;
	// this is actually ticks*daylength*24*60/65536 but to avoid overflow the factor 32 was removed from both)
	sint32 new_dms = (ticks * (env_t::show_month == env_t::DATE_FMT_MONTH ? 3 : 31) * 3 * 15) / (2048);

	days.set_value(new_dms / (24 * 60));
	hours.set_value((new_dms / 60) % 24);
	minutes.set_value(new_dms % 60);
}



bool gui_timeinput_t::action_triggered(gui_action_creator_t*, value_t)
{
	uint16 t = get_ticks();
	call_listeners(t);
	return false;
}


void gui_timeinput_t::draw(scr_coord offset)
{
	gui_aligned_container_t::draw(offset);
	if(b_enabled  &&  getroffen( get_mouse_x()-offset.x, get_mouse_y()-offset.y )) {
		win_set_tooltip(get_mouse_x() + TOOLTIP_MOUSE_OFFSET_X,get_mouse_y() + TOOLTIP_MOUSE_OFFSET_Y, translator::translate("Enter intervall in days, hours minutes" ), this);
	}
}


void gui_timeinput_t::rdwr( loadsave_t *file )
{
	uint16 ticks=get_ticks();
	file->rdwr_short(ticks);
	set_ticks(ticks);
}
