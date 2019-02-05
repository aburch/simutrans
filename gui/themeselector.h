/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_THEMESELECTOR_H
#define GUI_THEMESELECTOR_H


#include "simwin.h"
#include "savegame_frame.h"
#include "components/gui_textarea.h"
#include "../utils/cbuffer_t.h"

/*
 * selection of paks at the start time
 */
class themeselector_t : public savegame_frame_t
{
protected:
	static std::string undo; // undo buffer

	virtual bool        item_action   ( const char *fullpath ) OVERRIDE;
	virtual bool        ok_action     ( const char *fullpath ) OVERRIDE;
	virtual bool        cancel_action ( const char *fullpath ) OVERRIDE;
	virtual const char* get_info      ( const char *fname    ) OVERRIDE;
	virtual bool        check_file    ( const char *filename, const char *suffix ) OVERRIDE;
	void        fill_list       ( void ) OVERRIDE;

public:
	themeselector_t ( void );

	const char* get_help_filename ( void ) const OVERRIDE { return NULL; }

	uint32      get_rdwr_id     ( void ) OVERRIDE { return magic_themes; }
	void        rdwr            ( loadsave_t *file ) OVERRIDE;
};

#endif
