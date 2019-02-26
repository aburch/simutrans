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
	cbuffer_t      notice_buffer;
	gui_textarea_t notice_label;

	const char *get_info    ( const char *filename ) OVERRIDE;
	bool        check_file  ( const char *filename, const char *suffix ) OVERRIDE;
	bool        item_action ( const char *fullpath ) OVERRIDE;
	bool        del_action  ( const char *fullpath ) OVERRIDE;
	void        fill_list   ( void ) OVERRIDE;

public:
	pakselector_t();

	const char *get_help_filename ( void ) const OVERRIDE { return ""; }
	bool        has_title         ( void ) const OVERRIDE { return false; }
	bool        has_pak           ( void ) const          { return !entries.empty(); }
};

#endif
