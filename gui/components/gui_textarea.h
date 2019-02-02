/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_TEXTAREA_H
#define GUI_COMPONENTS_GUI_TEXTAREA_H


#include "gui_component.h"

class cbuffer_t;

/*
 * A text display component
 *
 * @author Hj. Malthaner
 */
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

	/**
	 * recalc the current size, needed for speculative size calculations
	 * @returns size necessary to show the component
	 */
	scr_size calc_size() const;


public:
	gui_textarea_t(cbuffer_t* buf_);
	gui_textarea_t(const char* text);
	~gui_textarea_t();

	void set_text(const char *text);

	void set_buf( cbuffer_t* buf_ );

	/**
	 * recalc the current size, needed for speculative size calculations
	 */
	void recalc_size();

	/**
	* Draw the component
	* @author Hj. Malthaner
	*/
	virtual void draw(scr_coord offset);

	scr_size get_min_size() const;

	scr_size get_max_size() const;
};

#endif
