/*
 * selection of paks at the start time
 */

#ifndef pakselector_h
#define pakselector_h


#include "savegame_frame.h"


class pakselector_t : public savegame_frame_t
{
protected:
	virtual void action(const char *fullpath);
	virtual bool del_action(const char *fullpath);
	virtual const char *get_info(const char *fname);
	virtual bool check_file(const char *filename, const char *suffix);
public:
	void fill_list();
	virtual bool has_title() const { return false; }
	bool has_pak() const { return !entries.empty(); }
	const char * get_hilfe_datei() const { return ""; }
	// since we only want to see the frames ...
	void zeichnen(koord pos, koord gr);
	void set_fenstergroesse(koord groesse);
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
	pakselector_t();
};

#endif
