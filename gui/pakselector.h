// selection of paks at the start time

#ifndef pakselector_h
#define pakselector_h


#include "savegame_frame.h"


class pakselector_t : public savegame_frame_t
{
protected:
	/**
	* Aktion, die nach Knopfdruck gestartet wird.
	* @author Hansjörg Malthaner
	*/
	virtual void action(const char *filename);

	/**
	* Aktion, die nach X-Knopfdruck gestartet wird.
	* @author V. Meyer
	*/
	virtual void del_action(const char *filename);

	// returns extra file info
	virtual const char *get_info(const char *fname);

	// true, if valid
	virtual bool check_file( const char *filename, const char *suffix );

public:
	bool has_pak() const { return !entries.empty(); }

	const char * gib_hilfe_datei() const { return ""; }

	// since we only want to see the frames ...
	void zeichnen(koord pos, koord gr);

	pakselector_t();
};

#endif
