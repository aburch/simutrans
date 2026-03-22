/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_ROUTE_CACHE_H
#define DATAOBJ_ROUTE_CACHE_H

#include <unordered_map>
#include "../dataobj/koord3d.h"
#include "../simtypes.h"
#include "../dataobj/route.h"
#include "../linehandle_t.h"

/**
 * Global route cache keyed by (line, start, ziel, max_speed_kmh, need_electric).
 * Only convoys assigned to a line use this cache.
 * Entries expire after one game month (ticks_per_world_month).
 * Memory-only: not saved/loaded with the game.
 */
struct route_cache_t {
	struct key_t {
		koord3d start;
		koord3d ziel;
		sint32  max_speed_kmh;
		bool    need_electric;
		bool operator==(const key_t &o) const {
			return start == o.start && ziel == o.ziel
			    && max_speed_kmh == o.max_speed_kmh
			    && need_electric == o.need_electric;
		}
	};
	struct key_hash {
		size_t operator()(const key_t &k) const {
			size_t h = (size_t)(uint16)k.start.x ^ ((size_t)(uint16)k.start.y << 16) ^ ((size_t)(uint8)k.start.z << 24);
			h ^= ((size_t)(uint16)k.ziel.x << 3) ^ ((size_t)(uint16)k.ziel.y << 19) ^ ((size_t)(uint8)k.ziel.z << 27);
			h ^= (size_t)(uint32)k.max_speed_kmh;
			h ^= k.need_electric ? 0x80000000u : 0u;
			return h;
		}
	};

	struct entry_t {
		route_t route;
		uint32  added_ticks;
	};

	struct line_hash {
		size_t operator()(const linehandle_t &l) const {
			return (size_t)l.get_id();
		}
	};

	static const size_t MAX_ROUTES_PER_ENTRY = 4;

	typedef std::unordered_map<key_t, entry_t, key_hash> route_map_t;
	typedef std::unordered_map<uint8, route_map_t> entry_map_t;
	typedef std::unordered_map<linehandle_t, entry_map_t, line_hash> line_map_t;

	// Outer key: linehandle_t, Middle key: schedule entry index, Inner key: route key
	line_map_t map;

	route_t* find(linehandle_t line, uint8 schedule_entry, koord3d start, koord3d ziel,
	              sint32 max_speed_kmh, bool need_electric);

	void add(linehandle_t line, uint8 schedule_entry, koord3d start, koord3d ziel,
	         sint32 max_speed_kmh, bool need_electric,
	         const route_t &r);

	void remove(linehandle_t line, uint8 schedule_entry, koord3d start, koord3d ziel,
	            sint32 max_speed_kmh, bool need_electric);

	// Erase all cache entries for a given line
	void remove_line(linehandle_t line);
};

#endif
