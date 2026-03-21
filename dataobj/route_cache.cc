/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "route_cache.h"
#include "../simworld.h"


route_t* route_cache_t::find(linehandle_t line, koord3d start, koord3d ziel,
                             sint32 max_speed_kmh, bool need_electric)
{
	auto line_it = map.find(line);
	if (line_it == map.end()) return nullptr;
	auto it = line_it->second.find({start, ziel, max_speed_kmh, need_electric});
	if (it == line_it->second.end()) return nullptr;
	// Expiration check (unsigned subtraction handles wraparound)
	karte_ptr_t welt;
	if (welt->get_ticks() - it->second.added_ticks > welt->ticks_per_world_month) {
		line_it->second.erase(it);
		return nullptr;
	}
	return &it->second.route;
}


void route_cache_t::add(linehandle_t line, koord3d start, koord3d ziel,
                        sint32 max_speed_kmh, bool need_electric,
                        const route_t &r)
{
	karte_ptr_t welt;
	map[line][{start, ziel, max_speed_kmh, need_electric}] = {r, welt->get_ticks()};
}


void route_cache_t::remove(linehandle_t line, koord3d start, koord3d ziel,
                           sint32 max_speed_kmh, bool need_electric)
{
	auto line_it = map.find(line);
	if (line_it != map.end()) {
		line_it->second.erase({start, ziel, max_speed_kmh, need_electric});
	}
}


void route_cache_t::remove_line(linehandle_t line)
{
	map.erase(line);
}
