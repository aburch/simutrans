/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_container_h
#define gui_container_h


#include "../simdebug.h"
#include "../simevent.h"
#include "../tpl/slist_tpl.h"
#include "../ifc/gui_komponente.h"

/**
 * Ein Behälter für andere gui_komponenten. Ist selbst eine
 * gui_komponente, kann also geschachtelt werden.
 *
 * @author Hj. Malthaner
 * @date 03-Mar-01
 */
class gui_container_t : public gui_komponente_t
{
private:
	slist_tpl <gui_komponente_t *> komponenten;

	// holds the GUI Komponent that has the focus in this window
	gui_komponente_t *komp_focus;

	bool list_dirty;

public:
	gui_container_t();

	// needed for WIN_OPEN events
	void clear_dirty() {list_dirty=false;}

	/**
	* Fügt eine Komponente zum Container hinzu.
	* @author Hj. Malthaner
	*/
	void add_komponente(gui_komponente_t *komp);

	/**
	* Entfernt eine Komponente aus dem Container.
	* @author Hj. Malthaner
	*/
	void remove_komponente(gui_komponente_t *komp);

	/**
	* Events werden hiermit an die GUI-Komponenten
	* gemeldet
	* @author Hj. Malthaner
	*/
	virtual void infowin_event(const event_t *);

	/**
	* Zeichnet die Komponente
	* @author Hj. Malthaner
	*/
	virtual void zeichnen(koord offset);

	/**
	* Entfernt alle Komponenten aus dem Container.
	* @author Markus Weber
	*/
	void remove_all();

	// activates this element
	void set_focus( gui_komponente_t *komp_focus );
};

#endif
