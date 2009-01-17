/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_button_h
#define gui_button_h

#include "../../ifc/gui_action_creator.h"
#include "../../ifc/gui_komponente.h"
#include "../../simcolor.h"


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
class button_t :
	public gui_action_creator_t,
	public gui_komponente_t
{
public:
	/* the button with the postfix state do not automatically change their state like the normal button do
	 * the _state buttons must be changed by the caller!
	 *
	 * square: button with text on the right side next to it
	 * box:  button with is used for many selection purposes; can have colored background
	 * roundbox: button for "load" cancel and such options
	 * arrow-buttons: buttons with arrows, cannot have text
	 * repeat arrows: calls the caller until the mouse is released
	 * scrollbar: well you guess it. Not used by gui_frame_t things ...
	 */
	enum type { square=1, box, roundbox, arrowleft, arrowright, arrowup, arrowdown, scrollbar, repeatarrowleft, repeatarrowright, posbutton,
					   square_state=129, box_state, roundbox_state, arrowleft_state, arrowright_state, arrowup_state, arrowdown_state, scrollbar_state, repeatarrowleft_state, repeatarrowright_state };

private:
	/**
	 * Tooltip ofthis button
	 * @author Hj. Malthaner
	 */
	const char * tooltip, *translated_tooltip;

	enum type type;

	/**
	 * if buttons is disabled show only grey label
	 * @author hsiegeln
	 */
	uint8 b_enabled:1;
	uint8 b_no_translate:1;

	/**
	 * Der im Button angezeigte Text
	 * direct acces provided to avoid translations
	 * @author Hj. Malthaner
	 */
	union {
		const char * text;
		struct { sint16 x,y; } targetpos;
	};
	const char *translated_text;

public:
	PLAYER_COLOR_VAL background; //@author hsiegeln
	PLAYER_COLOR_VAL foreground;

	bool pressed:1;

	button_t();

	void init(enum type typ, const char *text, koord pos, koord size = koord::invalid);

	void set_typ(enum type typ);

	const char * get_text() const {return text;}

	/**
	 * Setzt den im Button angezeigten Text
	 * @author Hj. Malthaner
	 */
	void set_text(const char * text);

	/**
	 * Get/Set text to position
	 * @author prissi
	 */
	void set_targetpos(const koord k ) { this->targetpos.x = k.x; this->targetpos.y = k.y; }

	/**
	 * Setzt den im Button angezeigten Text
	 * @author Hj. Malthaner
	 */
	void set_no_translate(bool b) { b_no_translate = b; }

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
	bool getroffen(int x,int y);

	/**
	 * Events werden hiermit an die GUI-Komponenten
	 * gemeldet
	 * @author Hj. Malthaner
	 */
	void infowin_event(const event_t *);

	/**
	 * Zeichnet die Komponente
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord offset);

	void enable() { b_enabled = true; }

	void disable() { b_enabled = false; }

	bool enabled() { return b_enabled; }

private:
	button_t(const button_t&);        // forbidden
	void operator =(const button_t&); // forbidden
};

#endif
