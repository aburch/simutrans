/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BAUER_TUNNELBAUER_H
#define BAUER_TUNNELBAUER_H


#include "../simtypes.h"
#include "../dataobj/koord.h"
#include "../dataobj/koord3d.h"

class karte_ptr_t;
class player_t;
class tunnel_desc_t;
class way_desc_t;
class tool_selector_t;

/**
 * Baut Tunnel. Tunnel sollten nicht direkt instanziiert werden
 * sondern immer vom tunnel_builder_t erzeugt werden.
 *
 * Es gibt keine Instanz - nur statische Methoden.
 */
class tunnel_builder_t {
private:
	static karte_ptr_t welt;

	static bool build_tunnel(player_t *player, koord3d pos, koord3d end, koord zv, const tunnel_desc_t *desc);
	static void build_tunnel_portal(player_t *player, koord3d end, koord zv, const tunnel_desc_t *desc, const way_desc_t *way_desc, int &cost, bool connect_inside);

	tunnel_builder_t() {} // private -> no instance please

public:
	static koord3d find_end_pos(player_t *player, koord3d pos, koord zv, const tunnel_desc_t *desc, bool full_tunnel=true, const char** msg=NULL);

	static void register_desc(tunnel_desc_t *desc);

	static const tunnel_desc_t *get_desc(const char *);

	static const tunnel_desc_t *get_tunnel_desc(const waytype_t wtyp, const sint32 min_speed,const uint16 time);

	static void fill_menu(tool_selector_t *tool_selector, const waytype_t wtyp, sint16 sound_ok);
	/**
	 * Returns a list with available tunnel types.
	 */
	static const vector_tpl<const tunnel_desc_t *>& get_available_tunnels(const waytype_t wtyp);

	static const char *build( player_t *player, koord pos, const tunnel_desc_t *desc, bool full_tunnel  );

	static const char *remove(player_t *player, koord3d pos, waytype_t wegtyp, bool all);
};

#endif
