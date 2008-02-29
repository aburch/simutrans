/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "../../simdebug.h"
#include "gui_speedbar.h"
#include "../../simevent.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../simwin.h"
#include "../../simtypes.h"


void gui_speedbar_t::set_base(sint32 base)
{
	this->base = base!=0 ? base : 1;
}


void gui_speedbar_t::add_color_value(const sint32 *value, uint8 color)
{
    info_t  next =  { color, value, -1 };
    values.insert(next);
}


void gui_speedbar_t::zeichnen(koord offset)
{
	offset += pos;

	slist_iterator_tpl<info_t> iter(values);
	sint32 from, to;

	if(vertical) {
		for(from = groesse.y; iter.next();  ) {
			to = groesse.y - min(*iter.get_current().value, base) * groesse.y / base;

			if(to < from) {
				display_fillbox_wh_clip( offset.x, offset.y + to, groesse.x, from - to, iter.get_current().color, true);
				from = to - 1;
			}
		}
		if(from > 0) {
			display_fillbox_wh_clip( offset.x, offset.y, groesse.x, from, MN_GREY0, true);
		}
	}
	else {
		for(from = 0; iter.next(); ) {
			to = min(*iter.get_current().value, base) * groesse.x / base;

			if(to > from) {
				display_fillbox_wh_clip( offset.x + from, offset.y, to - from, groesse.y, iter.get_current().color, true);
				from = to + 1;
			}
		}
		if(from < groesse.x) {
			display_fillbox_wh_clip(offset.x + from, offset.y, groesse.x - from, groesse.y, MN_GREY0, true);
		}
	}
}
