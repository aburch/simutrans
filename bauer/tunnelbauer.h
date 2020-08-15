/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BAUER_TUNNELBAUER_H
#define BAUER_TUNNELBAUER_H


#include "../simtypes.h"
#include "../dataobj/koord.h"
#include "../dataobj/koord3d.h"
#include "../tpl/stringhashtable_tpl.h"

class karte_ptr_t;
class player_t;               // Hajo: 22-Nov-01: Added forward declaration
class tunnel_desc_t;
class way_desc_t;
class tool_selector_t;

/**
 * Baut Tunnel. Tunnel sollten nicht direkt instanziiert werden
 * sondern immer vom tunnel_builder_t erzeugt werden.
 *
 * Es gibt keine Instanz - nur statische Methoden.
 *
 * @author V. Meyer
 */
class tunnel_builder_t {
private:
	static karte_ptr_t welt;

	static bool build_tunnel(player_t *player, koord3d pos, koord3d end, koord zv, const tunnel_desc_t *desc, overtaking_mode_t overtaking_mode, const way_desc_t *way_desc = NULL);
	static void build_tunnel_portal(player_t *player, koord3d end, koord zv, const tunnel_desc_t *desc, const way_desc_t *way_desc, sint64 &cost, overtaking_mode_t overtaking_mode, bool beginning);

	tunnel_builder_t() {} // private -> no instance please

public:
	static koord3d find_end_pos(player_t *player, koord3d pos, koord zv, const tunnel_desc_t *desc, bool full_tunnel=true, const char** msg=NULL);

	static void register_desc(tunnel_desc_t *desc);

	static bool successfully_loaded();

	static const tunnel_desc_t *get_desc(const char *);

	static stringhashtable_tpl <tunnel_desc_t *> * get_all_tunnels();

	static const tunnel_desc_t *get_tunnel_desc(const waytype_t wtyp, const sint32 min_speed,const uint16 time);

	static void fill_menu(tool_selector_t *tool_selector, const waytype_t wtyp, sint16 sound_ok);

	static const char *build( player_t *player, koord pos, const tunnel_desc_t *desc, bool full_tunnel, overtaking_mode_t overtaking_mode, const way_desc_t *way_desc = NULL  );

	static const char *remove(player_t *player, koord3d pos, waytype_t wegtyp, bool all);
};

#endif
