/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_TEXTAREA_H
#define GUI_COMPONENTS_GUI_TEXTAREA_H


#include "gui_component.h"

class cbuffer_t;

/**
 * A text display component
 */
class gui_textarea_t : public gui_component_t
{
private:
	/**
	* The text to display. May be multi-lined.
	*/
	cbuffer_t* buf;

	/**
	 * recalc the current size, needed for speculative size calculations
	 * @returns size necessary to show the component
	 */
	scr_size calc_size() const;


public:
	gui_textarea_t(cbuffer_t* buf_);

	void set_buf( cbuffer_t* buf_ );

	/**
	 * recalc the current size, needed for speculative size calculations
	 */
	void recalc_size();

	/**
	* Draw the component
	*/
	void draw(scr_coord offset) OVERRIDE;

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE;
};

#endif
