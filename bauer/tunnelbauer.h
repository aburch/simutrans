/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef __simtunnel_h
#define __simtunnel_h

#include "../simtypes.h"
#include "../dataobj/koord.h"
#include "../dataobj/koord3d.h"
#include "../tpl/stringhashtable_tpl.h"

class karte_ptr_t;
class player_t;               // Hajo: 22-Nov-01: Added forward declaration
class tunnel_besch_t;
class weg_besch_t;
class tool_selector_t;

/**
 * Baut Tunnel. Tunnel sollten nicht direkt instanziiert werden
 * sondern immer vom tunnelbauer_t erzeugt werden.
 *
 * Es gibt keine Instanz - nur statische Methoden.
 *
 * @author V. Meyer
 */
class tunnelbauer_t {
private:
	static karte_ptr_t welt;

	static bool baue_tunnel(player_t *player, koord3d pos, koord3d end, koord zv, const tunnel_besch_t *besch);
	static void baue_einfahrt(player_t *player, koord3d end, koord zv, const tunnel_besch_t *besch, const weg_besch_t *weg_besch, sint64 &cost);

	tunnelbauer_t() {} // private -> no instance please

public:
	static koord3d finde_ende(player_t *player, koord3d pos, koord zv, const tunnel_besch_t *besch, bool full_tunnel=true, const char** msg=NULL);

	static void register_besch(tunnel_besch_t *besch);

	static const tunnel_besch_t *get_besch(const char *);

	static stringhashtable_tpl <tunnel_besch_t *> * get_all_tunnels();

	static const tunnel_besch_t *find_tunnel(const waytype_t wtyp, const sint32 min_speed,const uint16 time);

	static void fill_menu(tool_selector_t *tool_selector, const waytype_t wtyp, sint16 sound_ok);

	static const char *baue( player_t *player, koord pos, const tunnel_besch_t *besch, bool full_tunnel  );

	static const char *remove(player_t *player, koord3d pos, waytype_t wegtyp, bool all);
};

#endif
