/*
 * selection of paks at the start time
 */

#ifndef themeselector_h
#define themeselector_h

#include "savegame_frame.h"
#include "components/gui_textarea.h"
#include "simwin.h"

class themeselector_t : public savegame_frame_t
{
protected:
	virtual void action(const char *fullpath);
	virtual bool del_action(const char *fullpath);
	virtual const char *get_info(const char *fname);

public:
	themeselector_t();

	void fill_list() OVERRIDE;
	const char * get_hilfe_datei() const { return ""; }

	uint32 get_rdwr_id() { return magic_themes; }
	void rdwr( loadsave_t *file );
};

#endif
