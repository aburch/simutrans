/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_savegame_frame_h
#define gui_savegame_frame_h


#include "../tpl/slist_tpl.h"
#include "components/action_listener.h"
#include "gui_frame.h"
#include "gui_container.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textinput.h"
#include "components/gui_divider.h"
#include "components/gui_label.h"
#include "components/gui_button.h"


class savegame_frame_t : public gui_frame_t, action_listener_t
{
private:
	char ibuf[64];

	/**
	 * Filename suffix, i.e. ".sve", must be four characters
	 * @author Hj. Malthaner
	 */
	const char *suffix;

	// path, to be put in front
	const char *fullpath;

	// true, if there is additional information, i.e. loading a game
	bool use_pak_extension;

	void add_file(const char *filename, const char *pak, const bool no_cutting_suffix );

protected:

	gui_textinput_t input;
	gui_divider_t divider1;                               // 30-Oct-2001  Markus Weber    Added
	button_t savebutton;                                  // 29-Oct-2001  Markus Weber    Added
	button_t cancelbutton;                               // 29-Oct-2001  Markus Weber    Added
	gui_label_t fnlabel;        //filename                // 31-Oct-2001  Markus Weber    Added
	gui_container_t button_frame;
	gui_scrollpane_t scrolly;

	/**
	 * Name des Spieles in der Datei.
	 * @aparam filename Name der Spielstandsdatei
	 * @author Hansjörg Malthaner
	 */
	const char *gib_spiel_name(const char *filename);

	/**
	 * Aktion, die nach Knopfdruck gestartet wird.
	 * @author Hansjörg Malthaner
	 */
	virtual void action(const char *filename) = 0;

	/**
	 * Aktion, die nach X-Knopfdruck gestartet wird.
	 * @author Volker Meyer
	 */
	virtual void del_action(const char *filename) = 0;

	// sets the filename in the edit field
	void set_filename( const char *fn );

	static bool check_file( const char *filename, const char *suffix );

public:
	/**
	 * @param suffix Filename suffix, i.e. ".sve", must be four characters
	 * @author Hj. Malthaner
	 */
	savegame_frame_t(const char *suffix, const char *path, bool (*check)(const char *, const char *) = savegame_frame_t::check_file );

	virtual ~savegame_frame_t();

	/**
	 * Setzt die Fenstergroesse
	 * @author (Mathew Hounsell)
	 * @date   11-Mar-2003
	 */
	virtual void setze_fenstergroesse(koord groesse);

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered(gui_komponente_t *komp, value_t extra);

	private:
		struct entry
		{
			entry(button_t* button_, button_t* del_, gui_label_t* label_) :
				button(button_),
				del(del_),
				label(label_)
			{}

			button_t*    button;
			button_t*    del;
			gui_label_t* label;
		};

		slist_tpl<entry> entries;
};

#endif
