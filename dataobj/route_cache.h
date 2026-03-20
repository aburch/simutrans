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

/**
 * Global route cache keyed by (start, ziel, max_speed_kmh, need_electric).
 * need_electric is part of the key because electric vehicles can only use
 * electrified track, so a diesel route and an electric route between the
 * same endpoints may differ.
 * Memory-only: not saved/loaded with the game.
 * Shared across all convoys to avoid redundant A* searches.
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
	std::unordered_map<key_t, route_t, key_hash> map;

	route_t* find(koord3d start, koord3d ziel, sint32 max_speed_kmh, bool need_electric) {
		auto it = map.find({start, ziel, max_speed_kmh, need_electric});
		return (it != map.end()) ? &it->second : nullptr;
	}
	void add(koord3d start, koord3d ziel, sint32 max_speed_kmh, bool need_electric, const route_t &r) {
		map[{start, ziel, max_speed_kmh, need_electric}] = r;
	}
	void remove(koord3d start, koord3d ziel, sint32 max_speed_kmh, bool need_electric) {
		map.erase({start, ziel, max_speed_kmh, need_electric});
	}
};

#endif
