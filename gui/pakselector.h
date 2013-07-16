/*
 * selection of paks at the start time
 */

#ifndef pakselector_h
#define pakselector_h

#include "savegame_frame.h"
#include "components/gui_textarea.h"
#include "../utils/cbuffer_t.h"

class pakselector_t : public savegame_frame_t
{
	protected:
		KOORD_VAL      addon_button_width;
		cbuffer_t      notice_buffer;
		gui_textarea_t notice_label;
		gui_divider_t  divider;

		virtual void action(const char *fullpath);
		virtual bool del_action(const char *fullpath);
		virtual const char *get_info(const char *fname);
		virtual bool check_file(const char *filename, const char *suffix);

	public:
		pakselector_t();

		void fill_list();
		virtual bool has_title() const { return false; }
		bool has_pak() const { return !entries.empty(); }
		const char * get_hilfe_datei() const { return ""; }

		// since we only want to see the frames ...
		void set_fenstergroesse(koord groesse);
		bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
