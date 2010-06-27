/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_components_gui_textinput_h
#define gui_components_gui_textinput_h

#include "../../ifc/gui_action_creator.h"
#include "../../ifc/gui_komponente.h"
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
	 * position of text cursor
	 * @author hsiegeln
	 */
	size_t cursor_pos;

	/**
	  * offset for drawing the cursor
	  * Dwachs: made private to check for mouse induced cursor moves
	  */
	KOORD_VAL cursor_offset;

	/**
	 * text alignment
	 * @author: Dwachs
	 */
	uint8 align;

	/**
	 * indicates whether listeners will be called when unfocused
	 * @author Knightly
	*/
	bool notify_unfocus;

	COLOR_VAL textcol;

public:
	gui_textinput_t(const bool _notify_unfocus = true);

	~gui_textinput_t();

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
	char *get_text() const {return text;}

	/**
	 * Events werden hiermit an die GUI-Komponenten
	 * gemeldet
	 * @author Hj. Malthaner
	 */
	virtual bool infowin_event(const event_t *);

	/**
	 * Zeichnet die Komponente
	 * @author Hj. Malthaner
	 */
	virtual void zeichnen(koord offset);

	// to allow for right-aligned text
	void set_alignment(uint8 _align){ align = _align;}

	// to allow for right-aligned text
	void set_color(COLOR_VAL col){ textcol = col;}
};


class gui_hidden_textinput_t : public gui_textinput_t
{
	// and set the cursor right when clicking with the mouse
	virtual bool infowin_event(const event_t *);

	// just draw with stars ...
	virtual void zeichnen(koord offset);
};


#endif
