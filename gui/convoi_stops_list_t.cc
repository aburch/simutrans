/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simline.h"
#include "../simcolor.h"
#include "../simhalt.h"
#include "../simworld.h"
#include "../simmenu.h"
#include "../simconvoi.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "../dataobj/schedule.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"

#include "../player/simplay.h"

#include "../tpl/vector_tpl.h"

#include "convoi_stops_list_t.h"
#include "halt_info.h"

#include "components/gui_image.h"
#include "components/gui_textarea.h"
#include "simwin.h"

static karte_ptr_t welt;
class gui_halt_detail_t;

/**
 * One entry in the list of schedule entries.
 */
class convoi_stops_list_item_t : public gui_aligned_container_t, public gui_action_creator_t
{
	schedule_entry_t entry;
	bool is_current;
	uint8 number;
	player_t* player;
	gui_image_t arrow;
	gui_label_buf_t stop;

public:
	convoi_stops_list_item_t(player_t* pl, schedule_entry_t e, uint n)
	{
		player = pl;
		entry  = e;
		number = n;
		is_current = false;
		set_table_layout(2,1);
		add_component(&arrow);
		arrow.set_image(gui_theme_t::pos_button_img[0], true);
		add_component(&stop);
		update_label();
	}
	void update_label()
	{
		stop.buf().printf("%i) ", number+1);
		schedule_t::gimme_stop_name(stop.buf(), welt, player, entry, -1);
		stop.set_color(is_current ? SYSCOL_TEXT_HIGHLIGHT : SYSCOL_TEXT);
		stop.update();
	}

	void draw(scr_coord offset) OVERRIDE
	{
		if (is_current) {
			display_fillbox_wh_clip_rgb(pos.x + offset.x, pos.y + offset.y, size.w, size.h, SYSCOL_LIST_BACKGROUND_SELECTED_F, false);
		}
		gui_aligned_container_t::draw(offset);
	}

	void set_active(bool yesno)
	{
		is_current = yesno;
	}

	bool is_active()
	{
		return is_current;
	}

	bool infowin_event(const event_t *ev) OVERRIDE
	{
		if( ev->ev_class != EVENT_CLICK ) {
			return false;
		}
		if(  IS_RIGHTCLICK(ev)  ||  ev->mx < stop.get_pos().x) {
			// move to the entry tile.
			view_stop();
		}
		else {
			halthandle_t h = haltestelle_t::get_stoppable_halt( entry.pos,player );
			if (  h.is_bound()  ) {
				// show halt info window.
				create_win(new halt_info_t(h), w_info, magic_halt_info);		
			}
			else {
				call_listeners(number);
			}
		}
		return true;
	}
	void view_stop()
	{
		// just center on it
		world()->get_viewport()->change_world_position( entry.pos );		
	}
};


convoi_stops_list_t::convoi_stops_list_t(convoihandle_t cnv_)
{	
	set_table_layout(1,0);
	this->cnv = cnv_;
	this->player = cnv_->get_owner();
	if( !cnv_.is_bound() ) {
		return;
	}
	update_schedule();
	add_listener(this);
}



void convoi_stops_list_t::update_schedule()
{
	remove_all();
	entries.clear();
	gui_schedule = cnv->get_schedule()->copy();
	static cbuffer_t buf;
	if (gui_schedule->empty()) {
		new_component<gui_textarea_t>(&buf);
	}
	else {
		for(uint i=0; i<gui_schedule->entries.get_count(); i++) {
			entries.append( new_component<convoi_stops_list_item_t>(player, gui_schedule->entries[i], i));
			entries.back()->add_listener( this );
		}
		entries[ gui_schedule->get_current_stop() ]->set_active(true);
	}
	set_size(get_min_size());
}

void convoi_stops_list_t::draw(scr_coord offset)
{
	if(  !cnv->get_schedule()->matches(world(),gui_schedule) || !entries[ cnv->get_schedule()->get_current_stop() ]->is_active()  ) {
		update_schedule();
	}
	gui_aligned_container_t::draw(offset);
}

bool convoi_stops_list_t::action_triggered(gui_action_creator_t *comp, value_t p)
{
	update_schedule();
	// has to be one of the entries
	call_listeners(p);
	return false;
}

