/*
 * gui_textarea.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_textarea_h
#define gui_textarea_h

#include "../../ifc/gui_komponente.h"


/**
 * Eine textanzeigekomponente
 *
 * @autor Hj. Malthaner
 */
class gui_textarea_t : public gui_komponente_t
{
private:
	/**
	* The text to display. May be multi-lined.
	* @autor Hj. Malthaner
	*/
	const char *text;

	// we cache the number of lines, to dynamically recalculate the size, if needed
	uint16	lines;

public:
	gui_textarea_t(const char *text);

	void setze_text(const char *text);

	/**
	 * recalc the current size, needed for speculative size calculations
	 */
	void recalc_size();

	/**
	* Zeichnet die Komponente
	* @author Hj. Malthaner
	*/
	virtual void zeichnen(koord offset);
};

#endif
