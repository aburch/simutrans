/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_CONVOY_LABEL_H
#define GUI_COMPONENTS_GUI_CONVOY_LABEL_H


#include "gui_label.h"

#include "../../convoihandle_t.h"


/**
 * A label with a convoy image underneath
 *
 * @author isidoro
 * @date 06-Feb-09
 */
class gui_convoy_label_t:public gui_label_t
{
private:
	// Adjustable parameters
	static const int separation=4;

	bool show_number;
	bool show_max_speed;

	/**
	 * The convoy to be painted
	 */
	convoihandle_t cnv;

public:
	gui_convoy_label_t(convoihandle_t cnv, bool show_number_of_convoys=false, bool show_max_speed=false);

	scr_size get_image_size() const;

	scr_size get_size() const;

	virtual void draw(scr_coord offset);
};

#endif
