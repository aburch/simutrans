/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simdebug.h"

#include "tunnelbauer.h"

#include "../gui/karte.h"

#include "../simworld.h"
#include "../player/simplay.h"
#include "../simwerkz.h"

#include "../besch/tunnel_besch.h"

#include "../boden/tunnelboden.h"

#include "../dataobj/scenario.h"
#include "../dataobj/environment.h"
#include "../dataobj/marker.h"

#include "../obj/tunnel.h"
#include "../obj/leitung2.h"
#include "../obj/signal.h"

#include "../gui/messagebox.h"
#include "../gui/werkzeug_waehler.h"

#include "wegbauer.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"


karte_ptr_t tunnelbauer_t::welt;

static stringhashtable_tpl<tunnel_besch_t *> tunnel_by_name;


void tunnelbauer_t::register_besch(tunnel_besch_t *besch)
{
	// avoid duplicates with same name
	if( const tunnel_besch_t *old_besch = tunnel_by_name.get(besch->get_name()) ) {
		dbg->warning( "tunnelbauer_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
		tunnel_by_name.remove(besch->get_name());
		werkzeug_t::general_tool.remove( old_besch->get_builder() );
		delete old_besch->get_builder();
		delete old_besch;
	}
	// add the tool
	wkz_tunnelbau_t *wkz = new wkz_tunnelbau_t();
	wkz->set_icon( besch->get_cursor()->get_bild_nr(1) );
	wkz->cursor = besch->get_cursor()->get_bild_nr(0);
	wkz->set_default_param( besch->get_name() );
	werkzeug_t::general_tool.append( wkz );
	besch->set_builder( wkz );
	tunnel_by_name.put(besch->get_name(), besch);
}


const tunnel_besch_t *tunnelbauer_t::get_besch(const char *name)
{
	return tunnel_by_name.get(name);
}


/**
 * Find a matching tunnel
 * @author Hj. Malthaner
 */
const tunnel_besch_t *tunnelbauer_t::find_tunnel(const waytype_t wtyp, const sint32 min_speed, const uint16 time)
{
	const tunnel_besch_t *find_besch = NULL;

	FOR(stringhashtable_tpl<tunnel_besch_t*>, const& i, tunnel_by_name) {
		tunnel_besch_t* const besch = i.value;
		if(  besch->get_waytype()==wtyp  ) {
			if(  besch->is_available(time)  ) {
				if(  find_besch==NULL  ||
					(find_besch->get_topspeed()<min_speed  &&  find_besch->get_topspeed()<besch->get_topspeed())  ||
					(besch->get_topspeed()>=min_speed  &&  besch->get_wartung()<find_besch->get_wartung())
				) {
					find_besch = besch;
				}
			}
		}
	}
	return find_besch;
}


static bool compare_tunnels(const tunnel_besch_t* a, const tunnel_besch_t* b)
{
	int cmp = a->get_topspeed() - b->get_topspeed();
	if(cmp==0) {
		cmp = (int)a->get_intro_year_month() - (int)b->get_intro_year_month();
	}
	if(cmp==0) {
		cmp = strcmp(a->get_name(), b->get_name());
	}
	return cmp<0;
}


/**
 * Fill menu with icons of given waytype
 * @author Hj. Malthaner
 */
void tunnelbauer_t::fill_menu(werkzeug_waehler_t* wzw, const waytype_t wtyp, sint16 /*sound_ok*/)
{
	// check if scenario forbids this
	if (!welt->get_scenario()->is_tool_allowed(welt->get_active_player(), WKZ_TUNNELBAU | GENERAL_TOOL, wtyp)) {
		return;
	}

	const uint16 time=welt->get_timeline_year_month();
	vector_tpl<const tunnel_besch_t*> matching(tunnel_by_name.get_count());

	FOR(stringhashtable_tpl<tunnel_besch_t*>, const& i, tunnel_by_name) {
		tunnel_besch_t* const besch = i.value;
		if(  besch->get_waytype()==wtyp  &&  besch->is_available(time)  ) {
			matching.insert_ordered(besch, compare_tunnels);
		}
	}
	// now sorted ...
	FOR(vector_tpl<tunnel_besch_t const*>, const i, matching) {
		wzw->add_werkzeug(i->get_builder());
	}
}


/* now construction stuff */


koord3d tunnelbauer_t::finde_ende(spieler_t *sp, koord3d pos, koord zv, const tunnel_besch_t *besch, bool full_tunnel, const char** msg)
{
	const grund_t *gr;
	leitung_t *lt;
	waytype_t wegtyp = besch->get_waytype();
	// use the is_allowed_step routine of wegbauer_t, needs an instance
	wegbauer_t bauigel(sp);
	bauigel.route_fuer( wegbauer_t::tunnel_flag | (wegbauer_t::bautyp_t)wegtyp, wegbauer_t::weg_search( wegtyp, 1, 0, weg_t::type_flat ), besch);
	long dummy;

	while(true) {
		pos = pos + zv;
		if(!welt->is_within_limits(pos.get_2d())) {
			return koord3d::invalid;
		}

		// check if ground is below tunnel level
		gr = welt->lookup_kartenboden(pos.get_2d());
		if(  gr->get_hoehe() < pos.z  ){
			return koord3d::invalid;
		}

		if (const char* err = welt->get_scenario()->is_work_allowed_here(sp, WKZ_TUNNELBAU|GENERAL_TOOL, wegtyp, pos)) {
			if (msg) {
				*msg = err;
			}
			return koord3d::invalid;
		}

		// next tile
		gr = welt->lookup(pos);
		if(  gr == NULL  ) {
			// check tile below
			gr = welt->lookup( pos + koord3d(0, 0, -1) );
			// check tiles one level above and below
			if(  env_t::pak_height_conversion_factor == 2  ) {
				if (gr == NULL) {
					gr = welt->lookup( pos + koord3d(0, 0, 1) );
				}
				// tile one level above/below should end on same level
				if (gr  &&  gr->get_vmove(ribi_typ(-zv)) != pos.z) {
					return koord3d::invalid;
				}
			}
			// check two levels below
			if (gr == NULL) {
				gr = welt->lookup( pos + koord3d(0, 0, -2) );
				if(  gr  &&  env_t::pak_height_conversion_factor == 2  ) {
					// should not end at -1
					if (gr->get_vmove(ribi_typ(-zv)) == pos.z-1) {
						return koord3d::invalid;
					}
				}
			}
			if(  gr  &&  gr->get_weg_hang() == hang_t::flach  ) {
				// Don't care about _flat_ tunnels below.
				gr = NULL;
			}
		}

		if(gr) {
			// if there is a tunnel try to connect
			if(  gr->ist_tunnel() ) {
				if(  gr->get_vmove(ribi_typ(-zv))!=pos.z) {
					// wrong slope
					return koord3d::invalid;
				}
				// fake tunnel tile
				tunnelboden_t from(pos - zv, hang_t::flach);
				if (bauigel.is_allowed_step(&from, gr, &dummy)) {
					return gr->get_pos();
				}
				else {
					return koord3d::invalid;
				}
			}
			const uint8 slope = gr->get_grund_hang();
			const uint8 slope_height = env_t::pak_height_conversion_factor;
			if(  gr->get_typ() != grund_t::boden  ||  slope != hang_typ(-zv) * slope_height  ||  gr->is_halt()  ||  ((wegtyp != powerline_wt) ? gr->get_leitung() != NULL : gr->hat_wege())  ) {
				// must end on boden_t and correct slope and not on halts
				// ways cannot end on powerlines, powerlines cannot end on ways
				return koord3d::invalid;
			}
			if(  gr->has_two_ways()  &&  wegtyp != road_wt  ) {
				// Only road tunnels allowed here.
				return koord3d::invalid;
			}

			ribi_t::ribi ribi = 0;
			if(wegtyp != powerline_wt) {
				ribi = gr->get_weg_ribi_unmasked(wegtyp);
			}
			else {
				if(gr->get_leitung()) {
					ribi = gr->get_leitung()->get_ribi();
				}
			}

			if(  ribi && koord(ribi) == zv  ) {
				// There is already a way (with correct ribi)
				return pos;
			}
			if(  !ribi  ) {
				// End of the slope - Missing end rail or has no ribis
				// we still consider if we interfere with a way (original: prüfen noch, ob uns dort ein anderer Weg stört)
				if(wegtyp != powerline_wt) {
					if(  !gr->hat_wege()  ||  gr->hat_weg(wegtyp)  ) {
						return pos;
					}
				}
				else {
					lt = gr->find<leitung_t>();
					if(!gr->hat_wege() || lt) {
						return pos;
					}
				}
			}
			return koord3d::invalid;  // Was im Weg (slope hillside or so)
		}

		// stop if we only want to check tile behind tunnel mouth
		if (!full_tunnel) {
			return pos;
		}
		// All free - keep looking
	}
}


const char *tunnelbauer_t::baue( spieler_t *sp, koord pos, const tunnel_besch_t *besch, bool full_tunnel )
{
	assert( besch );

	const grund_t *gr = welt->lookup_kartenboden(pos);
	if(gr==NULL) {
		return "Tunnel must start on single way!";
	}

	koord zv;
	const waytype_t wegtyp = besch->get_waytype();
	const hang_t::typ slope = gr->get_grund_hang();

	if(  wegtyp != powerline_wt  ) {
		const weg_t *weg = gr->get_weg(wegtyp);

		if(  gr->get_typ() != grund_t::boden  ||  gr->is_halt()  ||  gr->get_leitung()) {
			return "Tunnel must start on single way!";
		}
		// If there is a way on this tile, it must have the right ribis.
		if(  weg  &&  (weg->get_ribi_unmasked() & ~ribi_t::rueckwaerts( ribi_typ(slope) ))  ) {
			return "Tunnel must start on single way!";
		}
	}
	else {
		leitung_t *lt = gr->find<leitung_t>();
		if(  gr->get_typ() != grund_t::boden  ||  gr->hat_wege()  ) {
			return "Tunnel must start on single way!";
		}
		if(  lt  &&  (lt->get_ribi() & ~ribi_t::rueckwaerts( ribi_typ(slope) ))  ) {
			return "Tunnel must start on single way!";
		}
	}
	if(  !hang_t::ist_einfach(slope)  ) {
		return "Tunnel muss an\neinfachem\nHang beginnen!\n";
	}

	// for conversion factor 1, must be single height, for conversion factor 2, must be double
	if(  (env_t::pak_height_conversion_factor == 1  &&  !(slope & 7))  ||  (env_t::pak_height_conversion_factor == 2  &&  (slope & 7))  ) {
		return "Tunnel muss an\neinfachem\nHang beginnen!\n";
	}

	if(  gr->has_two_ways()  &&  wegtyp != road_wt  ) {
		return "Tunnel must start on single way!";
	}
	zv = koord(slope);

	// Search tunnel end and check intermediate tiles
	const char *err = NULL;
	koord3d end = finde_ende(sp, gr->get_pos(), zv, besch, full_tunnel, &err);
	if (err) {
		return err;
	}

	if(!welt->is_within_limits(end.get_2d())) {
		return "Tunnel must start on single way!";
	}

	// check ownership
	if (const grund_t *gr_end = welt->lookup(end)) {
		if (weg_t *weg_end = gr_end->get_weg(wegtyp)) {
			if (weg_end->ist_entfernbar(sp)!=NULL) {
				return "Das Feld gehoert\neinem anderen Spieler\n";
			}
		}
	}

	// Begin and end found, we can build
	if(!baue_tunnel(sp, gr->get_pos(), end, zv, besch)) {
		return "Ways not connected";
	}
	return NULL;
}


bool tunnelbauer_t::baue_tunnel(spieler_t *sp, koord3d start, koord3d end, koord zv, const tunnel_besch_t *besch)
{
	ribi_t::ribi ribi = 0;
	weg_t *weg = NULL;
	leitung_t *lt = NULL;
	koord3d pos = start;
	int cost = 0;
	const weg_besch_t *weg_besch;
	waytype_t wegtyp = besch->get_waytype();

DBG_MESSAGE("tunnelbauer_t::baue()","build from (%d,%d,%d) to (%d,%d,%d) ", pos.x, pos.y, pos.z, end.x, end.y, end.z );

	// now we search a matching way for the tunnels top speed
	weg_besch = besch->get_weg_besch();
	if(weg_besch==NULL) {
		// ignore timeline to get consistent results
		weg_besch = wegbauer_t::weg_search( wegtyp, besch->get_topspeed(), 0, weg_t::type_flat );
	}

	baue_einfahrt(sp, pos, zv, besch, weg_besch, cost);

	ribi = ribi_typ(-zv);
	// don't move on to next tile if only one tile long
	if(  end != start  ) {
		pos = pos + zv;
	}
	// calc new back image for the ground
	if(grund_t::underground_mode) {
		grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
		gr->calc_bild();
		gr->set_flag(grund_t::dirty);
	}

	// Now we build the invisible part
	while(pos.get_2d()!=end.get_2d()) {
		tunnelboden_t *tunnel = new tunnelboden_t( pos, 0);
		welt->access(pos.get_2d())->boden_hinzufuegen(tunnel);
		if(wegtyp != powerline_wt) {
			weg = weg_t::alloc(besch->get_waytype());
			weg->set_besch(weg_besch);
			weg->set_max_speed(besch->get_topspeed());
			tunnel->neuen_weg_bauen(weg, ribi_t::doppelt(ribi), sp);
			spieler_t::add_maintenance( sp, -weg->get_besch()->get_wartung(), weg->get_besch()->get_finance_waytype() );
		}
		else {
			lt = new leitung_t(tunnel->get_pos(), sp);
			lt->set_besch(weg_besch);
			tunnel->obj_add( lt );
			lt->laden_abschliessen();
			spieler_t::add_maintenance( sp, -weg_besch->get_wartung(), powerline_wt );
		}
		tunnel->obj_add(new tunnel_t(pos, sp, besch));
		tunnel->calc_bild();
		tunnel->set_flag(grund_t::dirty);
		assert(!tunnel->ist_karten_boden());
		spieler_t::add_maintenance( sp, besch->get_wartung(), besch->get_finance_waytype() );
		cost += besch->get_preis();
		pos = pos + zv;
	}

	// if end is tunnel then connect
	grund_t *gr_end = welt->lookup(end);
	if (gr_end) {
		if (gr_end->ist_tunnel()) {
			gr_end->weg_erweitern(besch->get_waytype(), ribi);
		}
		else if (gr_end->ist_karten_boden()) {
			// if end is above ground construct an exit
			baue_einfahrt(sp, pos, -zv, besch, weg_besch, cost);
			gr_end = NULL; // invalid - replaced by tunnel ground
			// calc new back image for the ground
			if (end!=start && grund_t::underground_mode) {
				grund_t *gr = welt->lookup_kartenboden(pos.get_2d()-zv);
				gr->calc_bild();
				gr->set_flag(grund_t::dirty);
			}
		}
		else {
			// good luck
			assert(0);
		}
	}
	else {
		// construct end tunnel tile
		tunnelboden_t *tunnel = new tunnelboden_t( pos, 0);
		welt->access(pos.get_2d())->boden_hinzufuegen(tunnel);
		if(wegtyp != powerline_wt) {
			weg = weg_t::alloc(besch->get_waytype());
			weg->set_besch(weg_besch);
			weg->set_max_speed(besch->get_topspeed());
			tunnel->neuen_weg_bauen(weg, ribi, sp);
			spieler_t::add_maintenance( sp,  -weg->get_besch()->get_wartung(), weg->get_besch()->get_finance_waytype() );
		}
		else {
			lt = new leitung_t(tunnel->get_pos(), sp);
			lt->set_besch(weg_besch);
			tunnel->obj_add( lt );
			lt->laden_abschliessen();
			spieler_t::add_maintenance( sp, -weg_besch->get_wartung(), powerline_wt );
		}
		tunnel->obj_add(new tunnel_t(pos, sp, besch));
		tunnel->calc_bild();
		tunnel->set_flag(grund_t::dirty);
		assert(!tunnel->ist_karten_boden());
		spieler_t::add_maintenance( sp,  besch->get_wartung(), besch->get_finance_waytype() );
		cost += besch->get_preis();
	}

	spieler_t::book_construction_costs(sp, -cost, start.get_2d(), besch->get_waytype());
	return true;
}


void tunnelbauer_t::baue_einfahrt(spieler_t *sp, koord3d end, koord zv, const tunnel_besch_t *besch, const weg_besch_t *weg_besch, int &cost)
{
	grund_t *alter_boden = welt->lookup(end);
	ribi_t::ribi ribi = 0;
	if(besch->get_waytype()!=powerline_wt) {
		ribi = alter_boden->get_weg_ribi_unmasked(besch->get_waytype()) | ribi_typ(zv);
	}

	tunnelboden_t *tunnel = new tunnelboden_t( end, alter_boden->get_grund_hang());
	tunnel->obj_add(new tunnel_t(end, sp, besch));

	weg_t *weg = NULL;
	if(besch->get_waytype()!=powerline_wt) {
		weg = alter_boden->get_weg( besch->get_waytype() );
	}
	// take care of everything on that tile ...
	tunnel->take_obj_from( alter_boden );
	welt->access(end.get_2d())->kartenboden_setzen( tunnel );
	if(besch->get_waytype() != powerline_wt) {
		if(weg) {
			// has already a way
			tunnel->weg_erweitern(besch->get_waytype(), ribi);
		}
		else {
			// needs still one
			weg = weg_t::alloc( besch->get_waytype() );
			if(  weg_besch  ) {
				weg->set_besch( weg_besch );
			}
			tunnel->neuen_weg_bauen( weg, ribi, sp );
		}
		spieler_t::add_maintenance( sp, -weg->get_besch()->get_wartung(), weg->get_besch()->get_finance_waytype() );
		weg->set_max_speed( besch->get_topspeed() );
	}
	else {
		leitung_t *lt = tunnel->get_leitung();
		if(!lt) {
			lt = new leitung_t(tunnel->get_pos(), sp);
			lt->set_besch(weg_besch);
			tunnel->obj_add( lt );
			spieler_t::add_maintenance( sp, -weg_besch->get_wartung(), powerline_wt );
		}
		else {
			// subtract twice maintenance: once for the already existing powerline
			// once since leitung_t::laden_abschliessen will add it again
			spieler_t::add_maintenance( sp, -2*lt->get_besch()->get_wartung(), powerline_wt );
		}
		lt->laden_abschliessen();
	}

	// remove sidewalk
	weg_t *str = tunnel->get_weg( road_wt );
	if( str  &&  str->hat_gehweg()) {
		str->set_gehweg(false);
	}

	tunnel->calc_bild();
	tunnel->set_flag(grund_t::dirty);

	// Auto-connect to a way outside the new tunnel mouth
	grund_t *ground_outside = welt->lookup(end-zv);
	if( !ground_outside ) {
		ground_outside = welt->lookup(end-zv+koord3d(0,0,-1));
		if(  ground_outside  &&  ground_outside->get_grund_hang() != tunnel->get_grund_hang()  ) {
			// Not the correct slope.
			ground_outside = NULL;
		}
	}
	if( ground_outside) {
		weg_t *way_outside = ground_outside->get_weg( besch->get_waytype() );
		if( way_outside ) {
			// use the check_owner routine of wegbauer_t (not spieler_t!), needs an instance
			wegbauer_t bauigel(sp);
			bauigel.route_fuer( (wegbauer_t::bautyp_t)besch->get_waytype(), way_outside->get_besch());
			long dummy;
			if(bauigel.is_allowed_step(tunnel, ground_outside, &dummy)) {
				tunnel->weg_erweitern(besch->get_waytype(), ribi_typ(-zv));
				ground_outside->weg_erweitern(besch->get_waytype(), ribi_typ(zv));
			}
		}
		if (besch->get_waytype()==water_wt  &&  ground_outside->ist_wasser()) {
			// connect to the sea
			tunnel->weg_erweitern(besch->get_waytype(), ribi_typ(-zv));
			ground_outside->calc_bild(); // to recalculate ribis
		}
	}


	if(sp!=NULL) {
		spieler_t::add_maintenance( sp,  besch->get_wartung(), besch->get_finance_waytype() );
	}
	cost += besch->get_preis();
}


const char *tunnelbauer_t::remove(spieler_t *sp, koord3d start, waytype_t wegtyp, bool remove_all )
{
	marker_t& marker = marker_t::instance(welt->get_size().x, welt->get_size().y);
	slist_tpl<koord3d>  end_list;
	slist_tpl<koord3d>  part_list;
	slist_tpl<koord3d>  tmp_list;
	koord3d   pos = start;

	// Erstmal das ganze Außmaß des Tunnels bestimmen und sehen,
	// ob uns was im Weg ist.
	tmp_list.insert(pos);
	grund_t *from = welt->lookup(pos);
	marker.mark(from);
	waytype_t delete_wegtyp = wegtyp==powerline_wt ? invalid_wt : wegtyp;

	do {
		pos = tmp_list.remove_first();

		// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
		grund_t *from = welt->lookup(pos);
		grund_t *to;
		koord zv = koord::invalid;

		if(from->ist_karten_boden()) {
			// Der Grund ist Tunnelanfang/-ende - hier darf nur in
			// eine Richtung getestet werden.
			zv = koord(from->get_grund_hang());
			end_list.insert(pos);
		}
		else {
			part_list.insert(pos);
		}
		// Alle Tunnelteile auf Entfernbarkeit prüfen!
		if(  from->kann_alle_obj_entfernen(sp)  ) {
			return "Der Tunnel ist nicht frei!\n";
		}
		if(  !remove_all  &&  ribi_t::is_threeway(from->get_weg_ribi_unmasked(delete_wegtyp))  ) {
			return "This tunnel branches. You can try Control+Click to remove.";
		}
		// Nachbarn raussuchen
		for(int r = 0; r < 4; r++) {
			if((zv == koord::invalid || zv == koord::nsow[r]) &&
				from->get_neighbour(to, delete_wegtyp, ribi_t::nsow[r]) &&
				!marker.is_marked(to) &&
				(wegtyp != powerline_wt || to->get_leitung()))
			{
				tmp_list.insert(to->get_pos());
				marker.mark(to);
			}
		}
	} while (!tmp_list.empty());

	// Jetzt geht es ans löschen der Tunnel
	while (!part_list.empty()) {
		pos = part_list.remove_first();
		grund_t *gr = welt->lookup(pos);
		// remove the second way first in the tunnel
		if(gr->get_weg_nr(1)) {
			gr->remove_everything_from_way(sp,gr->get_weg_nr(1)->get_waytype(),ribi_t::keine);
		}
		gr->remove_everything_from_way(sp,wegtyp,ribi_t::keine);	// removes stop and signals correctly
		// remove everything else
		gr->obj_loesche_alle(sp);
		gr->mark_image_dirty();
		welt->access(pos.get_2d())->boden_entfernen(gr);
		delete gr;

		reliefkarte_t::get_karte()->calc_map_pixel( pos.get_2d() );
	}

	// Und die Tunnelenden am Schluß
	while (!end_list.empty()) {
		pos = end_list.remove_first();

		grund_t *gr = welt->lookup(pos);
		if(wegtyp == powerline_wt) {
			// remove tunnel portals
			tunnel_t *t = gr->find<tunnel_t>();
			if(t) {
				t->entferne(sp);
				delete t;
			}
			if (leitung_t *lt = gr->get_leitung()) {
				// remove single powerlines
				if ( (lt->get_ribi()  & ~ribi_typ(gr->get_grund_hang())) == ribi_t::keine ) {
					lt->entferne(sp);
					delete lt;
				}
			}
		}
		else {
			ribi_t::ribi mask = gr->get_grund_hang()!=hang_t::flach ? ~ribi_typ(gr->get_grund_hang()) : ~ribi_typ(hang_t::gegenueber(gr->get_weg_hang()));

			// remove the second way first in the tunnel
			if(gr->get_weg_nr(1)) {
				gr->remove_everything_from_way(sp,gr->get_weg_nr(1)->get_waytype(),gr->get_weg_nr(1)->get_ribi_unmasked() & mask);
			}
			// removes single signals, bridge head, pedestrians, stops, changes catenary etc
			ribi_t::ribi ribi = gr->get_weg_ribi_unmasked(wegtyp) & mask;

			tunnel_t *t = gr->find<tunnel_t>();
			uint8 broad_type = t->get_broad_type();
			gr->remove_everything_from_way(sp,wegtyp,ribi);	// removes stop and signals correctly

			// remove tunnel portals
			t = gr->find<tunnel_t>();
			if(t) {
				t->entferne(sp);
				delete t;
			}

			if( broad_type ) {
				hang_t::typ hang = gr->get_grund_hang();
				ribi_t::ribi dir = ribi_t::rotate90( ribi_typ( hang ) );
				if( broad_type & 1 ) {
					const grund_t *gr_l = welt->lookup(pos + dir);
					tunnel_t* tunnel_l = gr_l ? gr_l->find<tunnel_t>() : NULL;
					if( tunnel_l ) {
						tunnel_l->calc_bild();
					}
				}
				if( broad_type & 2 ) {
					const grund_t *gr_r = welt->lookup(pos - dir);
					tunnel_t* tunnel_r = gr_r ? gr_r->find<tunnel_t>() : NULL;
					if( tunnel_r ) {
						tunnel_r->calc_bild();
					}
				}
			}

			// corrects the ways
			weg_t *weg=gr->get_weg_nr(0);
			if(weg) {
				// fails if it was previously the last ribi
				weg->set_besch(weg->get_besch());
				weg->set_ribi( ribi );
				if(gr->get_weg_nr(1)) {
					gr->get_weg_nr(1)->set_ribi( ribi );
				}
			}
		}

		// then add the new ground, copy everything and replace the old one
		grund_t *gr_new = new boden_t(pos, gr->get_grund_hang());
		gr_new->take_obj_from( gr );
		welt->access(pos.get_2d())->kartenboden_setzen(gr_new );

		if(gr_new->get_leitung()) {
			gr_new->get_leitung()->laden_abschliessen();
		}

		// recalc image of ground
		grund_t *kb = welt->access(pos.get_2d()+koord(gr_new->get_grund_hang()))->get_kartenboden();
		kb->calc_bild();
		kb->set_flag(grund_t::dirty);
	}
	return NULL;
}
