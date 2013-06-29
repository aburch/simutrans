/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "gui_speedbar.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
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

	if(vertical) {
		sint32 from = groesse.y;
		FOR(slist_tpl<info_t>, const& i, values) {
			sint32 const to = groesse.y - min(*i.value, base) * groesse.y / base;
			if(to < from) {
				display_fillbox_wh_clip(offset.x, offset.y + to, groesse.x, from - to, i.color, true);
				from = to - 1;
			}
		}
		if(from > 0) {
			display_fillbox_wh_clip( offset.x, offset.y, groesse.x, from, MN_GREY0, true);
		}
	}
	else {
		sint32 from = 0;
		FOR(slist_tpl<info_t>, const& i, values) {
			sint32 const to = min(*i.value, base) * groesse.x / base;
			if(to > from) {
				display_fillbox_wh_clip(offset.x + from, offset.y, to - from, groesse.y, i.color, true);
				from = to + 1;
			}
		}
		if(from < groesse.x) {
			display_fillbox_wh_clip(offset.x + from, offset.y, groesse.x - from, groesse.y, MN_GREY0, true);
		}
	}
}
