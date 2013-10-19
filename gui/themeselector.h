/*
 * selection of paks at the start time
 */

#ifndef themeselector_h
#define themeselector_h

#include "simwin.h"
#include "savegame_frame.h"
#include "components/gui_textarea.h"
#include "../utils/cbuffer_t.h"

class themeselector_t : public savegame_frame_t
{
protected:
	static std::string undo; // undo buffer

	virtual bool        item_action   ( const char *fullpath );
	virtual bool        ok_action     ( const char *fullpath );
	virtual bool        cancel_action ( const char *fullpath );
	virtual const char* get_info      ( const char *fname    );
	virtual bool        check_file    ( const char *filename, const char *suffix );

public:
	themeselector_t ( void );

	void        fill_list       ( void ) OVERRIDE;
	const char* get_hilfe_datei ( void ) const { return ""; }

	uint32      get_rdwr_id     ( void ) { return magic_themes; }
	void        rdwr            ( loadsave_t *file );
};

#endif
