/*
 * button.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_button_h
#define gui_button_h

#include "../ifc/gui_komponente.h"


// forward decls
class action_listener_t;
template <class T> class slist_tpl;


#define SQUARE_BUTTON 0
#define ARROW_LEFT 1
#define ARROW_RIGHT 2
#define ARROW_UP 3
#define ARROW_DOWN 4
#define SCROLL_BAR 5

/**
 * Klasse für Buttons in Fenstern
 *
 * @author Hj. Malthaner, Niels Roest
 * @date December 2000
 */
class button_t : public gui_komponente_t
{
public:
	enum type { square, box, roundbox, arrowleft, arrowright, arrowup, arrowdown, scrollbar, repeatarrowleft, repeatarrowright };

private:

	/**
	 * Tooltip ofthis button
	 * @author Hj. Malthaner
	 */
	const char * tooltip;

	enum type type;

	/**
	 * Our listeners.
	 * @author Hj. Malthaner
	 */
	slist_tpl <action_listener_t *> * listeners;

	/**
	 * Inform all listeners that an action was triggered.
	 * @author Hj. Malthaner
	 */
	void call_listeners();

	/**
	 * if buttons is disabled show only grey label
	 * @author hsiegeln
	 */
	uint8 b_enabled:1;

public:
	uint8 background; //@author hsiegeln

	/**
	 * Add a new listener to this button.
	 * @author Hj. Malthaner
	 */
	void add_listener(action_listener_t * l);

	/**
	 * Der im Button angezeigte Text
	 * direct acces provided to avoid translations
	 * @author Hj. Malthaner
	 */
	const char * text;


	bool pressed:1;

	uint8 kennfarbe;

	button_t(const button_t & other);

	button_t();
	virtual ~button_t();

	void init(enum type typ, const char *text, koord pos, koord size = koord::invalid);

	void setze_typ(enum type typ);

	const char * gib_text() const {return text;};

	/**
	 * Setzt den im Button angezeigten Text
	 * @author Hj. Malthaner
	 */
	void setze_text(const char * text);

	/**
	 * Sets the tooltip of this button
	 * @author Hj. Malthaner
	 */
	void set_tooltip(const char * tooltip);

	/**
	 * @return true wenn x,y innerhalb der Buttonflaeche liegt, d.h. der
	 * Button getroffen wurde, false wenn x, y ausserhalb liegt
	 * @author Hj. Malthaner
	 */
	bool getroffen(int x,int y) {
		pressed = gui_komponente_t::getroffen(x, y);
		return pressed;
	};

	/**
	 * Events werden hiermit an die GUI-Komponenten
	 * gemeldet
	 * @author Hj. Malthaner
	 */
	void infowin_event(const event_t *);

	/**
	 * Zeichnet den Button.
	 * @author Niels Roest
	 */
	void zeichnen(koord offset, int button_farbe) const;

	/**
	 * Zeichnet die Komponente
	 * @author Hj. Malthaner
	 */
	virtual void zeichnen(koord offset) const;

	void operator= (const button_t & other);

	void enable() { b_enabled = true; };

	void disable() { b_enabled = false; };

	bool enabled() { return b_enabled; };
};

#endif
