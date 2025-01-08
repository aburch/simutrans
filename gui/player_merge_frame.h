
#ifndef GUI_PLAYER_MERGE_FRAME_H
#define GUI_PLAYER_MERGE_FRAME_H


#include "../simconst.h"

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "../tpl/vector_tpl.h"
#include "simwin.h"

class player_merge_frame_t : public gui_frame_t, private action_listener_t
{
	private:
		struct player_item_t {
			button_t* button;
			uint8 player_num;
		};
		
		gui_label_t lb_description, lb_merged, lb_merge_to;

    vector_tpl<player_item_t> players_merged;
    vector_tpl<player_item_t> players_merge_to;
    
		button_t bt_merge;
		
		sint8 selected_merged_player_num, selected_merger_player_num;

	public:
		player_merge_frame_t();

		/**
		 * Set the window associated helptext
		 * @return the filename for the helptext, or NULL
		 */
		const char * get_help_filename() const OVERRIDE {return "player_merger.txt";}

		bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

		// since no information are needed to be saved to restore this, returning magic is enough
		// uint32 get_rdwr_id() OVERRIDE { return magic_ki_kontroll_t; }
};

#endif
