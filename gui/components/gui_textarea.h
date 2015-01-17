/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * A text display component
 *
 * @author Hj. Malthaner
 */

#ifndef gui_textarea_h
#define gui_textarea_h

#include "gui_component.h"

class cbuffer_t;

class gui_textarea_t : public gui_component_t
{
private:
	/**
	* The text to display. May be multi-lined.
	* @author Hj. Malthaner
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

	void set_buf( cbuffer_t* buf_ ) { buf = buf_; recalc_size(); }

	/**
	 * recalc the current size, needed for speculative size calculations
	 */
	void recalc_size();

	/**
	* Draw the component
	* @author Hj. Malthaner
	*/
	virtual void draw(scr_coord offset);
};

#endif
