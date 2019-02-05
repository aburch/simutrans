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

	virtual const char *get_info    ( const char *filename );
	virtual       bool  check_file  ( const char *filename, const char *suffix );
	virtual       bool  item_action ( const char *fullpath );
	virtual       bool  del_action  ( const char *fullpath );
	void  fill_list       ( void );

public:
	pakselector_t();

	const char *get_help_filename ( void ) const { return ""; }
			bool  has_title       ( void ) const { return false; }
			bool  has_pak         ( void ) const { return !entries.empty(); }
};

#endif
