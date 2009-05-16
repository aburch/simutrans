/*
 * just displays a label with a convoy under it, the label will be auto-translated
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

//#ifndef gui_convoy_label_t_h
//#define gui_convoy_label_t_h

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

	koord get_image_size() const;

	koord get_size() const;

	virtual void zeichnen(koord offset);
};

//#endif