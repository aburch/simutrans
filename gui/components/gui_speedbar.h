/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_components_gui_speedbar_h
#define gui_components_gui_speedbar_h

#include "../../ifc/gui_komponente.h"
#include "../../tpl/slist_tpl.h"


/**
 *
 * @author Volker Meyer
 * @date  12.06.2003
 */
class gui_speedbar_t : public gui_komponente_t
{
private:
	struct info_t {
		sint32 color;
		const sint32 *value;
		sint32 last;
	};

	slist_tpl <info_t> values;

	sint32 base;
	bool vertical;

public:
	gui_speedbar_t() { base = 100; vertical = false; }

	void add_color_value(const sint32 *value, int color);

	void set_base(sint32 base);

	void set_vertical(bool vertical) { this->vertical = vertical; }

	/**
	 * Zeichnet die Komponente
	 */
	void zeichnen(koord offset);
};

#endif
