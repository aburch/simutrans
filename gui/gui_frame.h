/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#ifndef gui_gui_frame_h
#define gui_gui_frame_h

#include "../ifc/gui_fenster.h"
#include "gui_container.h"
#include "../player/simplay.h"
#include "../simcolor.h"


/**
 * Eine Klasse für Fenster mit Komponenten.
 * Anders als die anderen Fensterklasen in Simutrans ist dies
 * ein richtig Komponentenorientiertes Fenster, das alle
 * aktionen an die Komponenten delegiert.
 *
 * @author Hj. Malthaner
 */
class gui_frame_t : virtual public gui_fenster_t
{
private:
	gui_container_t container;

	const char * name;
	koord groesse;

	/**
	 * Min. size of the window
	 * @author Markus Weber
	 * @date   11-May-2002
	 */
	koord min_windowsize;

	resize_modes resize_mode; // 25-may-02  markus weber added
	const spieler_t *owner;

	// set true for total redraw
	bool dirty:1;

protected:
	void set_dirty() { dirty=1; }

	/**
	 * resize window in response to a resize event
	 * @author Markus Weber, Hj. Malthaner
	 * @date   11-May-2002
	 */
	virtual void resize(const koord delta);

	void set_owner( const spieler_t *sp ) { owner = sp; }

public:
	/**
	 * @param name Fenstertitel
	 * @param sp owner for color
	 * @author Hj. Malthaner
	 */
	gui_frame_t(const char *name, const spieler_t *sp=NULL);

	/**
	 * Fügt eine Komponente zum Fenster hinzu.
	 * @author Hj. Malthaner
	 */
	void add_komponente(gui_komponente_t *komp) { container.add_komponente(komp); }

	/**
	 * Entfernt eine Komponente aus dem Container.
	 * @author Hj. Malthaner
	 */
	void remove_komponente(gui_komponente_t *komp) { container.remove_komponente(komp); }

	/**
	 * Der Name wird in der Titelzeile dargestellt
	 * @return den nicht uebersetzten Namen der Komponente
	 * @author Hj. Malthaner
	 */
	const char * get_name() const { return name; }

	/**
	 * setzt den Namen (Fenstertitel)
	 * @author Hj. Malthaner
	 */
	void set_name(const char *name) { this->name=name; }

	/**
	 * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
	 * zurück
	 * @author Hj. Malthaner
	 */
	virtual PLAYER_COLOR_VAL get_titelcolor() const { return owner ? PLAYER_FLAG|(owner->get_player_color1()+1) : WIN_TITEL; }

	/**
	 * @return gibt wunschgroesse für das Darstellungsfenster zurueck
	 * @author Hj. Malthaner
	 */
	koord get_fenstergroesse() const { return groesse; }

	/**
	 * Setzt die Fenstergroesse
	 * @author Hj. Malthaner
	 */
	virtual void set_fenstergroesse(koord groesse);

	/**
	 * Set minimum size of the window
	 * @author Markus Weber
	 * @date   11-May-2002
	 */
	void set_min_windowsize(koord size) { min_windowsize = size; }

	/**
	 * Set minimum size of the window
	 * @author Markus Weber
	 * @date   11-May-2002
	 */
	koord get_min_windowsize() { return min_windowsize; }

	/**
	 * @return returns the usable width and heigth of the window
	 * @author Markus Weber
	 * @date   11-May-2002
	*/
	koord get_client_windowsize() const {return groesse-koord(0,16); }

	/**
	 * Events werden hiermit an die GUI-Komponenten
	 * gemeldet
	 * @author Hj. Malthaner
	 */
	virtual void infowin_event(const event_t *ev);

	/**
	 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	 * in dem die Komponente dargestellt wird.
	 * @author Hj. Malthaner
	 */
	virtual void zeichnen(koord pos, koord gr);

	/**
	 * Set resize mode
	 * @author Markus Weber
	 * @date   11-May-2002
	 */
	void set_resizemode(resize_modes mode) { resize_mode = mode; }

	/**
	 * Get resize mode
	 * @author Markus Weber
	 * @date   25-May-2002
	 */
	resize_modes get_resizemode(void) { return resize_mode; }


	void set_focus( gui_komponente_t *k ) { container.set_focus(k); }
	gui_komponente_t *get_focus() const { return container.get_focus(); }

};

#endif
