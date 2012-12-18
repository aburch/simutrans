/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_components_gui_textinput_h
#define gui_components_gui_textinput_h

#include "gui_action_creator.h"
#include "gui_komponente.h"
#include "../../simcolor.h"
#include "../../simgraph.h"


/**
 * Ein einfaches Texteingabefeld. Es hat keinen eigenen Textpuffer,
 * nur einen Zeiger auf den Textpuffer, der von jemand anderem Bereitgestellt
 * werden muss.
 *
 * @date 19-Apr-01
 * @author Hj. Malthaner
 */
class gui_textinput_t :
	public gui_action_creator_t,
	public gui_komponente_t
{
protected:

	/**
	 * Der Stringbuffer.
	 * @author Hj. Malthaner
	 */
	char *text;

	/**
	 * Maximallänge des Stringbuffers
	 * @author Hj. Malthaner
	 */
	size_t max;

	/**
	 * position of head cursor to the text
	 * represents front end of the selected text portion
	 * @author hsiegeln
	 */
	size_t head_cursor_pos;

	/**
	 * position of tail cursor to the text
	 * represent rear end of the selected text portion
	 * @author Knightly
	 */
	size_t tail_cursor_pos;

	/**
	  * offset for controlling horizontal text scroll
	  * Dwachs: made private to check for mouse induced cursor moves
	  */
	KOORD_VAL scroll_offset;

	/**
	 * text alignment
	 * @author: Dwachs
	 */
	uint8 align;

	COLOR_VAL textcol;

	// true if there were changed but no notification was sent yet
	bool text_dirty;

	/**
	 * reference time for regulating cursor blinking
	 * @author Knightly
	 */
	unsigned long cursor_reference_time;

	/**
	 * whether focus has been received
	 * @author Knightly
	 */
	bool focus_recieved;

	/**
	 * determine new cursor position from event coordinates
	 * @author Knightly
	 */
	size_t calc_cursor_pos(const int x);

	/**
	 * Remove selected text portion, if any.
	 * Returns true if some selected text is actually deleted.
	 * @author Knightly
	 */
	bool remove_selection();

public:
	gui_textinput_t();

	/**
	 * Setzt den Textpuffer
	 *
	 * @author Hj. Malthaner
	 */
	void set_text(char *text, size_t max);

	/**
	 * Holt den Textpuffer
	 *
	 * @author Hj. Malthaner
	 */
	char *get_text() const { return text; }

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Zeichnet die Komponente
	 * @author Hj. Malthaner
	 */
	virtual void zeichnen(koord offset);

	/**
	 * Detect change of focus state and determine whether cursor should be displayed,
	 * and call the function that performs the actual display
	 * @author Knightly
	 */
	void display_with_focus(koord offset, bool has_focus);

	// function that performs the actual display
	virtual void display_with_cursor(koord offset, bool cursor_active, bool cursor_visible);

	// to allow for right-aligned text
	void set_alignment(uint8 _align){ align = _align;}

	// to set text color
	void set_color(COLOR_VAL col){ textcol = col;}
};


class gui_hidden_textinput_t : public gui_textinput_t
{
	// and set the cursor right when clicking with the mouse
	bool infowin_event(event_t const*) OVERRIDE;

	// function that performs the actual display; just draw with stars ...
	virtual void display_with_cursor(koord offset, bool cursor_active, bool cursor_visible);
};


#endif
