/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#ifndef gui_map_frame_h
#define gui_map_frame_h

#include "gui_frame.h"
//#include "karte.h"
#include "../simwin.h"
#include "components/gui_scrollpane.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "../besch/fabrik_besch.h"
#include "../tpl/stringhashtable_tpl.h"

class karte_t;

#define MAP_MAX_BUTTONS (22)

/**
 * Reliefkartenfenster für Simutrans.
 *
 * @author Hj. Malthaner
 * @date 03-Mar-01
 */
class map_frame_t :
	public gui_frame_t,
	public action_listener_t
{
private:
	static karte_t *welt;

	/**
	 * This is kind of hack: we know there can only be one map frame
	 * at atime,and we want to save the current size for the next object
	 * so we use a static variable here.
	 * @author Hj. Malthaner
	 */
	static koord size;
	static koord screenpos;

	static bool legend_visible;
	static bool scale_visible;
	static bool directory_visible;
	static bool filter_factory_list;

	static bool is_cursor_hidden;

	// Cache of factories in current game world
	static stringhashtable_tpl<const fabrik_besch_t *> factory_list;

	  /**
	   * We need to keep track of trag/click events
	   * @author Hj. Malthaner
	   */
	bool is_dragging;

	/**
	 * remember that we zoomed
	 * to center map
	 */
	bool zoomed;

	gui_scrollpane_t scrolly;

	button_t filter_buttons[MAP_MAX_BUTTONS];

	void zoom(bool zoom_out);
	button_t zoom_buttons[2];
	gui_label_t zoom_label;
	button_t b_rotate45;

	button_t b_show_legend;
	button_t b_show_scale;
	button_t b_show_directory;
	button_t b_overlay_networks;
	button_t b_filter_factory_list;

	void update_factory_legend();
	void show_hide_legend(const bool show);
	void show_hide_scale(const bool show);
	void show_hide_directory(const bool show);

public:

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const {return "map.txt";}

	/**
	 * Does this window need a min size button in the title bar?
	 * @return true if such a button is needed
	 * @author Hj. Malthaner
	 */
	bool has_min_sizer() const {return true;}

	/**
	 * Konstruktor. Erzeugt alle notwendigen Subkomponenten.
	 * @author Hj. Malthaner
	 */
	map_frame_t( karte_t *welt );

	void rdwr( loadsave_t *file );

	virtual uint32 get_rdwr_id() { return magic_reliefmap; }

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Setzt die Fenstergroesse
	 * @author (Mathew Hounsell)
	 * @date   11-Mar-2003
	 */
	void set_fenstergroesse(koord groesse);

	/**
	 * resize window in response to a resize event
	 * @author Hj. Malthaner
	 * @date   01-Jun-2002
	 */
	void resize(const koord delta);

	/**
	 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	 * in dem die Komponente dargestellt wird.
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord pos, koord gr);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
