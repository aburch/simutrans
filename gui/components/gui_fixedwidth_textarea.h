/*
 * Copyright (c) 1997 - 2010 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_fixedwidth_textarea_h
#define gui_fixedwidth_textarea_h

#include "../../ifc/gui_komponente.h"
#include "../../simgraph.h"

/**
 * Knightly :
 *	A fixed-width, automatically line-wrapping text-area,
 *	optionally with a reserved area in the upper right corner.
 *	It does *not* add 10px margins from the top and the left.
 *	Core code borrowed from ding_infowin_t::calc_draw_info() with adaptation.
 */
class gui_fixedwidth_textarea_t : public gui_komponente_t
{
private:
	// Pointer to the text to be displayed. The text is *not* copied.
	const char *text;

	// reserved area in the upper right corner
	koord reserved_area;

	// for calculating text height and/or displaying the text
	// borrowed from ding_infowin_t::calc_draw_info() with adaptation
	void calc_display_text(const koord offset, const bool draw);

public:
	gui_fixedwidth_textarea_t(const char *const text, const sint16 width, const koord reserved_area = koord(0,0));

	void recalc_size();

	// after using any of these setter functions, remember to call recalc_size() to recalculate textarea height
	void set_width(const sint16 width);
	void set_text(const char *const text);
	void set_reserved_area(const koord area);

	// it will deliberately ignore the y-component (height) of groesse
	virtual void set_groesse(koord groesse);

	virtual void zeichnen(koord offset);
};

#endif
