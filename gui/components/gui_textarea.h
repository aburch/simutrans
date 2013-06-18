/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * A text display component
 *
 * @autor Hj. Malthaner
 */

#ifndef gui_textarea_h
#define gui_textarea_h

#include "gui_komponente.h"

class cbuffer_t;

class gui_textarea_t : public gui_komponente_t
{
private:
	/**
	* The text to display. May be multi-lined.
	* @autor Hj. Malthaner
	*/
	cbuffer_t* buf;
	/**
	 * gui_textarea_t(const char* text) constructor will allocate new cbuffer_t
	 * but gui_textarea_t(cbuffer_t* buf_) don't do it.
	 * we need track it for destructor
	 */
	bool my_own_buf;

	// we cache the number of lines, to dynamically recalculate the size, if needed
	uint16	lines;

public:
	gui_textarea_t(cbuffer_t* buf_);
	gui_textarea_t(const char* text);
	~gui_textarea_t();

	void set_text(const char *text);

	/**
	 * recalc the current size, needed for speculative size calculations
	 */
	void recalc_size();

	/**
	* Draw the component
	* @author Hj. Malthaner
	*/
	virtual void zeichnen(koord offset);
};

#endif
