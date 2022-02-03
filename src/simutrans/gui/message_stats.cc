/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "message_stats.h"
#include "components/gui_button.h"
#include "components/gui_label.h"

#include "messagebox.h"
#include "simwin.h"

#include "../world/simworld.h"
#include "../display/viewport.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

static karte_ptr_t welt;

message_stats_t::message_stats_t(const message_t::node *m, uint32 sid) :
	msg(m), sortid(sid)
{
	set_table_layout(2,1);

	// pos-button, visible or not
	button_t *b = new_component<button_t>();
	b->set_typ(button_t::posbutton_automatic);
	if (msg->pos!=koord::invalid) {
		b->set_targetpos(msg->pos);
	}
	else {
		b->set_visible(false);
		b->set_rigid(true);
	}

	// text buffer
	gui_label_buf_t *label = new_component<gui_label_buf_t>(msg->get_player_color(welt));

	// now fill buffer, first teh date
	label->buf().printf( "(%s) ", translator::get_short_date( msg->time/12, msg->time%12 ) );

	// then the text (without line break)
	for(int j=0; ;  j++) {
		char c = msg->msg[j];
		if (c==0) {
			break;
		}
		label->buf().printf("%c", c == '\n' ? ' ': c);
	}

	label->update();
}


bool message_stats_t::compare(const gui_component_t *aa, const gui_component_t *bb )
{
	const message_stats_t* a = dynamic_cast<const message_stats_t*>(aa);
	const message_stats_t* b = dynamic_cast<const message_stats_t*>(bb);
	assert(a  &&  b);

	return a->sortid < b->sortid;
}


/**
 * Click on message => go to position
 */
bool message_stats_t::infowin_event(const event_t * ev)
{
	bool swallowed = gui_aligned_container_t::infowin_event(ev);

	if(  !swallowed  &&  IS_LEFTRELEASE(ev)  ) {
		// show message window again
		news_window* news;
		if(  msg->pos==koord::invalid  ) {
			news = new news_img( msg->msg, msg->image, msg->get_player_color(welt) );
		}
		else {
			news = new news_loc( msg->msg, msg->pos, msg->get_player_color(welt) );
		}
		create_win(-1, -1, news, w_info, magic_none);
		swallowed = true;
	}
	else if(  !swallowed  &&  IS_RIGHTRELEASE(ev)  &&  msg->pos!=koord::invalid  ) {
		welt->get_viewport()->change_world_position( msg->pos );
		swallowed = true;
	}
	return swallowed;
}
