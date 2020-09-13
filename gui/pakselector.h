/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_PAKSELECTOR_H
#define GUI_PAKSELECTOR_H


#include "savegame_frame.h"
#include "components/gui_textarea.h"
#include "../utils/cbuffer_t.h"

class pakselector_t : public savegame_frame_t
{
	protected:
		scr_coord_val  addon_button_width;
		scr_coord_val  action_button_width;
		cbuffer_t      notice_buffer;
		gui_textarea_t notice_label;
		gui_divider_t  divider;

		virtual const char *get_info    ( const char *filename );
		virtual       bool  check_file  ( const char *filename, const char *suffix );
		virtual       bool  item_action ( const char *fullpath );
		virtual       bool  del_action  ( const char *fullpath );

	public:
		pakselector_t();

		const char *get_help_filename ( void ) const { return ""; }
		      bool  has_title       ( void ) const { return false; }
		      bool  has_pak         ( void ) const { return !entries.empty(); }
		      void  fill_list       ( void );

		// If there is only one option, this will set the pak name and return true.
		// Otherwise it will return false.  (Note, it's const but it modifies global data.)
		bool check_only_one_option() const;

		// since we only want to see the frame...
		void set_windowsize(scr_size size);
};

#endif
