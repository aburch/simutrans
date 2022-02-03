/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simwin.h"
#include "message_frame.h"
#include "message_option.h"
#include "message_stats.h"

#include "../world/simworld.h"
#include "../tool/simmenu.h"
#include "../simmesg.h"
#include "../sys/simsys.h"

#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../network/network_cmd_ingame.h"
#include "../player/simplay.h"


#define MAX_MESG_TABS (8)


static sint32 categories[MAX_MESG_TABS] =
{
	(1 << message_t::chat),
	(1 << message_t::scenario),
	(1 << message_t::problems),
	(1 << message_t::traffic_jams) | (1 << message_t::warnings),
	(1 << message_t::full),
	(1 << message_t::city) | (1 << message_t::industry),
	(1 << message_t::ai),
	(1 << message_t::general) | (1 << message_t::new_vehicle)
};

static char const* const tab_strings[]=
{
	"Chat_msg",
	"Scenario_msg",
	"Problems_msg",
	"Warnings_msg",
	"Station_msg",
	"Town_msg",
	"Company_msg",
	"Game_msg"
};



message_frame_t::message_frame_t() :
	gui_frame_t( translator::translate("Mailbox") ),
	scrolly(gui_scrolled_list_t::windowskin, message_stats_t::compare)
{
	ibuf[0] = 0;
	last_count = 0;
	message_type = -1;

	set_table_layout(1,0);

	add_table(3,0);
	{
		option_bt.init(button_t::roundbox, translator::translate("Optionen"));
		option_bt.add_listener(this);
		add_component(&option_bt);

		copy_bt.init(button_t::roundbox, translator::translate("Copy to clipboard"));
		copy_bt.add_listener(this);
		add_component(&copy_bt);

		new_component<gui_fill_t>();

		if(  env_t::networkmode  ) {
			input.set_text(ibuf, lengthof(ibuf) );
			input.add_listener(this);
			add_component(&input,3);
			set_focus( &input );
		}
	}
	end_table();

	// add tabs for classifying messages
	tabs.add_tab( &scrolly, translator::translate("All") );
	tab_categories.append( -1 );

	if (env_t::networkmode) {
		tabs.add_tab( &scrolly, translator::translate(tab_strings[0]) );
		tab_categories.append( categories[0] );
	}
	if (welt->get_scenario()->is_scripted()) {
		tabs.add_tab( &scrolly, translator::translate(tab_strings[1]) );
		tab_categories.append( categories[1] );
	}
	for(  int i=2;  i<MAX_MESG_TABS;  ++i  ) {
		tabs.add_tab( &scrolly, translator::translate(tab_strings[i]) );
		tab_categories.append( categories[i] );
	}
	tabs.add_listener(this);
	add_component(&tabs);

	set_resizemode(diagonal_resize);
	if(  env_t::networkmode  && env_t::chat_window_transparency!=100  ) {
		set_transparent( 100-env_t::chat_window_transparency, gui_theme_t::gui_color_chat_window_network_transparency );
		scrolly.set_skin_type(gui_scrolled_list_t::transparent);
	}

	fill_list();

	reset_min_windowsize();
	set_windowsize(scr_size(D_DEFAULT_WIDTH, D_DEFAULT_HEIGHT));
}


void message_frame_t::fill_list()
{
	uint32 id = 0;
	scrolly.clear_elements();
	FOR( slist_tpl<message_t::node*>, const i, welt->get_message()->get_list() ) {
		scrolly.new_component<message_stats_t>(i, id++);
	}

	last_count = welt->get_message()->get_list().get_count();

	// trigger filtering
	sint32 t = message_type;
	message_type = -1;
	// filter & sort
	filter_list(t);

	scrolly.set_size( scrolly.get_size());
}


void message_frame_t::filter_list(sint32 type)
{
	if (type != message_type) {
		for(int i=0, end=scrolly.get_count(); i<end; i++) {
			message_stats_t *a = dynamic_cast<message_stats_t*>(scrolly.get_element(i));
			// message type filtering controls visibility
			if (a) {
				a->set_visible(type == -1  ||  a->get_msg()->get_type_shifted() & type);
			}
		}
		message_type = type;
	}
	scrolly.sort(0);
	scrolly.set_size( scrolly.get_size());
}


bool message_frame_t::action_triggered( gui_action_creator_t *comp, value_t v )
{
	if(  comp==&option_bt  ) {
		create_win(320, 200, new message_option_t(), w_info, magic_message_options );
	}
	if(  comp==&copy_bt  ) {
		cbuffer_t clipboard;
		const sint32 message_type = tab_categories[ tabs.get_active_tab_index() ];
		int count = 20; // just copy the last 20
		FOR( slist_tpl<message_t::node*>, const i, welt->get_message()->get_list() ) {
			if( i->get_type_shifted() & message_type ) {
				// add them to clipboard
				char msg_no_break[ 258 ];
				for( int j = 0; j < 256; j++ ) {
					msg_no_break[ j ] = i->msg[ j ] == '\n' ? ' ' : i->msg[ j ];
					if( msg_no_break[ j ] == 0 ) {
						msg_no_break[ j++ ] = '\n';
						msg_no_break[ j ] = 0;
						break;
					}
				}
				clipboard.append( msg_no_break );
				if( count-- < 0 ) {
					break;
				}
			}
		}
		// copy, if there was anything ...
		if( clipboard.len() > 0 ) {
			dr_copy( clipboard, clipboard.len() );
		}

	}
	else if(  comp==&input  &&  ibuf[0]!=0  ) {
		// Send chat message to server for distribution
		nwc_chat_t* nwchat = new nwc_chat_t( ibuf, welt->get_active_player()->get_player_nr(), env_t::nickname.c_str() );
		network_send_server( nwchat );

		ibuf[0] = 0;
	}
	else if(  comp==&tabs  ) {
		// filter messages by type where necessary
		filter_list(tab_categories[v.i]);
	}
	return true;
}


void message_frame_t::draw(scr_coord pos, scr_size size)
{
	if(  welt->get_message()->get_list().get_count() != last_count  ) {
		fill_list();
	}
	gui_frame_t::draw(pos, size);
}


void message_frame_t::rdwr(loadsave_t *file)
{
	// window size
	scr_size size = get_windowsize();
	size.rdwr( file );

	scrolly.rdwr(file);
	tabs.rdwr(file);
	file->rdwr_str( ibuf, lengthof(ibuf) );

	if(  file->is_loading()  ) {
		fill_list();

		reset_min_windowsize();
		set_windowsize(size);
	}
}
