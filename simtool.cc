/*
 * Tools for the players
 *
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "path_explorer.h"
#include "simdebug.h"
#include "simsound.h"
#include "simevent.h"
#include "simcity.h"
#include "utils/simrandom.h"
#include "simmesg.h"
#include "simconvoi.h"
#include "gui/simwin.h"
#include "display/viewport.h"

#include "bauer/fabrikbauer.h"
#include "bauer/vehikelbauer.h"

#include "boden/grund.h"
#include "boden/wasser.h"
#include "boden/wege/schiene.h"
#include "boden/wege/strasse.h"
#include "boden/tunnelboden.h"
#include "boden/monorailboden.h"

#include "simdepot.h"
#include "simsignalbox.h"
#include "simfab.h"
#include "display/simimg.h"
#include "simintr.h"
#include "simhalt.h"
#include "simskin.h"

#include "descriptor/ground_desc.h"
#include "descriptor/building_desc.h"
#include "descriptor/roadsign_desc.h"
#include "descriptor/tunnel_desc.h"

#include "vehicle/simvehicle.h"
#include "vehicle/simroadtraffic.h"
#include "vehicle/simpeople.h"

#include "gui/line_management_gui.h"
#include "gui/tool_selector.h"
#include "gui/station_building_select.h"
#include "gui/karte.h"	// to update map after construction of new industry
#include "gui/depot_frame.h"
#include "gui/schedule_gui.h"
#include "gui/player_frame_t.h"
#include "gui/schedule_list.h"
#include "gui/signal_spacing.h"
#include "gui/onewaysign_info.h"
#include "gui/overtaking_mode.h"
#include "gui/city_info.h"
#include "gui/trafficlight_info.h"
#include "gui/privatesign_info.h"
#include "gui/messagebox.h"

#include "obj/zeiger.h"
#include "obj/bruecke.h"
#include "obj/tunnel.h"
#include "obj/groundobj.h"
#include "obj/signal.h"
#include "obj/crossing.h"
#include "obj/roadsign.h"
#include "obj/wayobj.h"
#include "obj/leitung2.h"
#include "obj/baum.h"
#include "obj/field.h"
#include "obj/label.h"

#include "dataobj/settings.h"
#include "dataobj/environment.h"
#include "dataobj/schedule.h"
#include "dataobj/route.h"
#include "dataobj/replace_data.h"
#include "dataobj/scenario.h"
#include "network/network_cmd_ingame.h" // for dragging raise / lower tools

#include "bauer/tunnelbauer.h"
#include "bauer/brueckenbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/hausbauer.h"

#include "descriptor/way_desc.h"
#include "descriptor/roadsign_desc.h"

#include "tpl/vector_tpl.h"
#include "tpl/weighted_vector_tpl.h"

#include "network/memory_rw.h"
#include "utils/simstring.h"

#include "simtool.h"
#include "player/finance.h"

#define is_scenario()  welt->get_scenario()->is_scripted()


/****************************************** static helper functions **************************************/

/**
 * Creates a tooltip from tip text and money value
 * @author Hj. Malthaner
 */
char *tooltip_with_price(const char * tip, sint64 price)
{
	const int n = sprintf(tool_t::toolstr, "%s: ", translator::translate(tip) );
	money_to_string(tool_t::toolstr+n, (double)price/-100.0);
	return tool_t::toolstr;
}

char *tooltip_with_price_and_distance(const char * tip, sint64 price, uint32 distance)
{
	int n = sprintf(tool_t::toolstr, "%s: ", translator::translate(tip));
	money_to_string(tool_t::toolstr + n, (double)price / -100.0);

	n = strlen(tool_t::toolstr);
	n += sprintf(tool_t::toolstr + n, ", %u%s", distance, translator::translate("m"));

	return tool_t::toolstr;
}

// TODO: merge this into building_layout defined in simcity.cc
static int const building_layout[] = { 0, 0, 1, 4, 2, 0, 5, 1, 3, 7, 1, 0, 6, 3, 2, 0 };

/**
 * Creates a tooltip from tip text and money value
 * @author Hj. Malthaner
 */
char *tooltip_with_price_maintenance(karte_t *welt, const char *tip, sint64 price, sint64 maintenance)
{
	int n = sprintf(tool_t::toolstr, "%s, ", translator::translate(tip) );
	money_to_string(tool_t::toolstr+n, (double)price/-100.0);
	strcat( tool_t::toolstr, " (" );
	n = strlen(tool_t::toolstr);

	money_to_string(tool_t::toolstr+n, (double)welt->calc_adjusted_monthly_figure(maintenance)/100.0 );
	strcat( tool_t::toolstr, ")" );
	return tool_t::toolstr;
}

/**
 * Creates a tooltip from tip text and money value
 */
char *tooltip_with_price_maintenance_level(karte_t *welt, const char *tip, sint64 price, sint64 maintenance, uint16 capacity, uint16 enables)
{
	int n = sprintf(tool_t::toolstr, "%s, ", translator::translate(tip) );
	money_to_string(tool_t::toolstr+n, (double)price/-100.0);
	strcat( tool_t::toolstr, " (" );
	n = strlen(tool_t::toolstr);

	money_to_string(tool_t::toolstr+n, (double)welt->calc_adjusted_monthly_figure(maintenance)/100.0 );
	strcat( tool_t::toolstr, ")" );
	n = strlen(tool_t::toolstr);

	if((enables&7)!=0) {
		n += sprintf( tool_t::toolstr+n, ", %d", capacity );
		if(enables&1) {
			n += sprintf( tool_t::toolstr+n, " %s", translator::translate("Passagiere") );
		}
		if(enables&2) {
			n += sprintf( tool_t::toolstr+n, " %s", translator::translate("Post") );
		}
		if(enables&4) {
			n += sprintf( tool_t::toolstr+n, " %s", translator::translate("Fracht") );
		}
	}
	else if (!welt->get_settings().is_separate_halt_capacities()) {
		n += sprintf( tool_t::toolstr+n, ", %s %d", translator::translate("Storage capacity"), capacity );
	}

	return tool_t::toolstr;
}



/**
 * sucht Haltestelle um Umkreis +1/-1 um (pos, b, h)
 * extended to search first in our direction
 * @author Hj. Malthaner, V.Meyer, prissi
 */
static halthandle_t suche_nahe_haltestelle(player_t *player, karte_t *welt, koord3d pos, sint16 b=1, sint16 h=1)
{
	koord k(pos.get_2d());
	planquadrat_t* plan = welt->access(k);

	// any other ground with a valid stop here?
	halthandle_t my_halt = plan->get_halt( player );
	if(  my_halt.is_bound()  ) {
		return my_halt;
	}

	grund_t *bd = plan->get_boden_in_hoehe(pos.z);
	if(  bd==NULL  ) {
		bd = plan->get_kartenboden();
	}

	// first we try to connect to a stop straight in our direction; otherwise our station may break during construction
	if(  bd->hat_wege()  ) {
		ribi_t::ribi ribi = bd->get_weg_nr(0)->get_ribi_unmasked();
		for(  int i=0;  i<4;  i++ ) {
			if(  ribi_t::nsew[i] & ribi ) {
				if(  planquadrat_t* plan=welt->access(k+koord::nsew[i])  ) {
					my_halt = plan->get_halt( player );
					if(  my_halt.is_bound()  ) {
						return my_halt;
					}
				}
			}
		}
	}

	// now just search all neighbours
	for(  sint16 y=-1;  y<=h;  y++  ) {
		for(  sint16 x=-1;  x<=b;  (x==-1 && y>-1 && y<h) ? x=b:x++  ) {
			if(  planquadrat_t* plan=welt->access(k+koord(x,y))  ) {
				my_halt = plan->get_halt( player );
				if(  my_halt.is_bound()  ) {
					return my_halt;
				}
			}
		}
	}

#if AUTOJOIN_PUBLIC
	// now search everything for public stops
	for(  int i=0;  i<8;  i++ ) {
		if(  planquadrat_t* plan=welt->access(k+koord::neighbours[i])  ) {
			my_halt = plan->get_halt( welt->get_public_player() );
			if(  my_halt.is_bound()  ) {
				return my_halt;
			}
		}
	}
#endif

	// here we reach only with a non-found station, i.e. a non-bounded handle
	return halthandle_t();
}


// converts a 2d koord to a suitable ground pointer
static grund_t *tool_intern_koord_to_weg_grund(player_t *player, karte_t *welt, koord3d pos, waytype_t wt, bool allow_public = false)
{
	// check for valid ground
	grund_t *gr=welt->lookup(pos);
	if (gr==NULL) {
		return NULL;
	}

	if(  wt==powerline_wt  &&  gr->get_leitung()  ) {
		// check for ownership
		if(gr->get_leitung()-> is_deletable(player)!=NULL) {
			return NULL;
		}
		// ok
		else {
			return gr;
		}
	}

	// tram
	if(wt==tram_wt) {
		weg_t *way = gr->get_weg(track_wt);
		if (way && way->get_desc()->get_styp() == type_tram &&  way-> is_deletable(player)==NULL) {
			return gr;
		}
		else {
			return NULL;
		}
	}


	// has some rail or monorail?
	if(  !gr->hat_weg(wt)  ) {
		return NULL;
	}
	// check for ownership
	if(gr->get_weg(wt)-> is_deletable(player, allow_public) != NULL)
	{
		return NULL;
	}
	// ok, now we have a valid ground
	return gr;
}



/****************************************** now the actual tools **************************************/

// werkzeuge (tool)
//// returns ground at pos if visible else NULL
//// if no grund at pos exists try kartenboden
//grund_t* get_grund(karte_t *welt, koord3d pos )
//{
//	grund_t *gr = welt->lookup(pos);
//	if (gr == NULL || !gr->is_visible()) {
//		gr = welt->lookup_kartenboden(pos.get_2d());
//		if (gr && !gr->is_visible()) {
//			gr = NULL;
//		}
//	}
//	return(gr);
//}
//
//const char *tool_query_t::work( karte_t *welt, player_t *player, koord3d pos )
const char *tool_query_t::work( player_t *player, koord3d pos )
{
	grund_t *gr = welt->lookup(pos);
	if(gr) {
		DBG_MESSAGE("tool_abfrage()","checking map square %s", pos.get_str());

		if(  env_t::single_info  ) {

			int old_count = win_get_open_count();

			if(  is_ctrl_pressed()  ) {
				// reverse order
				for(int n=0;  n<gr->get_top();  n++  ) {
					obj_t *obj = gr->obj_bei(n);
					if(  obj && obj->get_typ()!=obj_t::wayobj && obj->get_typ()!=obj_t::pillar && obj->get_typ()!=obj_t::label  ) {
						DBG_MESSAGE("tool_abfrage()", "index %d", n);
						obj->show_info();
						// did some new window open?
						if(old_count!=win_get_open_count()) {
							return NULL;
						}
					}
				}

				if(  gr->get_flag(grund_t::marked)  ) {
					label_t *lb = gr->find<label_t>();
					if(  lb  ) {
						lb->show_info();
						if(  old_count!=win_get_open_count()  ) {
							return NULL;
						}
					}
				}

				if(  gr->get_halt().is_bound()  ) {
					gr->get_halt()->show_info();
					if(  old_count!=win_get_open_count()  ) {
						return NULL;
					}
				}

			}
			else {

				// show halt and labels first ...
				if(  gr->get_halt().is_bound()  ) {
					gr->get_halt()->show_info();
					if(  old_count!=win_get_open_count()  ) {
						return NULL;
					}
				}
				if(  gr->get_flag(grund_t::marked)  ) {
					label_t *lb = gr->find<label_t>();
					if(  lb  ) {
						lb->show_info();
						if(  old_count!=win_get_open_count()  ) {
							return NULL;
						}
					}
				}

				for (size_t n = gr->get_top(); n-- != 0;) {
					obj_t *obj = gr->obj_bei(n);
					if(  obj && obj->get_typ()!=obj_t::wayobj && obj->get_typ()!=obj_t::pillar && obj->get_typ()!=obj_t::label  ) {
						DBG_MESSAGE("tool_abfrage()", "index %u", (unsigned)n);
						obj->show_info();
						if(obj->get_typ() == obj_t::signalbox && obj->get_owner() == player)
						{
							if(is_local_execution()  &&  player == welt->get_active_player())
							{
								player->set_selected_signalbox((signalbox_t*)obj);
							}
						}
						// did some new window open?
						if(old_count!=win_get_open_count()) {
							return NULL;
						}
					}
				}
			}

			// no window yet opened -> try ground info
			gr->show_info();
		}
		else {
			// lowest (less interesting) first
			gr->show_info();
			for(int n=0; n<gr->get_top();  n++  ) {
				obj_t *obj = gr->obj_bei(n);
				if(  obj && obj->get_typ()!=obj_t::wayobj && obj->get_typ()!=obj_t::pillar  ) {
					obj->show_info();
					if(obj->get_typ() == obj_t::signalbox && obj->get_owner() == player)
					{
						if(is_local_execution()  &&  player == welt->get_active_player())
						{
							player->set_selected_signalbox((signalbox_t*)obj);
						}
					}
				}
			}
		}

		if(gr->get_depot()  &&  gr->get_depot()->get_owner()==player) {
			int old_count = win_get_open_count();
			gr->get_depot()->show_info();
			// did some new window open?
			if(env_t::single_info  &&  old_count!=win_get_open_count()) {
				return NULL;
			}
		}
	}
	return NULL;
}


/* delete things from a tile
 * citycars and pedestrian first and then go up to queue to more important objects
 */
bool tool_remover_t::tool_remover_intern(player_t *player, koord3d pos, const char *&msg)
{
DBG_MESSAGE("tool_remover_intern()","at (%s)", pos.get_str());

	grund_t *gr = welt->lookup(pos);
	if (!gr) {
		msg = "";
		return false;
	}

	// check if there is something to remove from here ...
	if(  gr->get_top()==0  ) {
		msg = "";
		return false;
	}

	// marker?
	label_t* l = gr->find<label_t>();
	if(l) {
		msg = l-> is_deletable(player);
		if(msg==NULL) {
			delete l;
			// Refund the cost of land if the player is deleting the marker and therefore selling it.
			player_t::book_construction_costs(l->get_owner(), -welt->get_land_value(gr->get_pos()), gr->get_pos().get_2d());
			return true;
		}
		else if(  gr->get_top()==1  ) {
			// only complain if this is the last object on this tile ...
			return false;
		}
		msg = NULL;
		// not deletable: skip it
	}

	// citycar? (we allow always)
	private_car_t* citycar = gr->find<private_car_t>();
	if(citycar) {
		delete citycar;
		return true;
	}

	// pedestrians?
	pedestrian_t* pedestrian = gr->find<pedestrian_t>();
	if(pedestrian) {
		delete pedestrian;
		return true;
	}

	koord k(pos.get_2d());

	// prissi: check powerline (can cross ground of another player)
	leitung_t* lt = gr->get_leitung();
	if(lt!=NULL  &&  lt-> is_deletable(player)==NULL) {
		if(  gr->ist_bruecke()  ) {
			bruecke_t* br = gr->find<bruecke_t>();
			if(  br == NULL  ) {
				// no bridge? most likely transformer on a former bridge tile...
				grund_t *gr_new = new boden_t(pos, gr->get_grund_hang());
				gr_new->take_obj_from( gr );
				welt->access(k)->kartenboden_setzen( gr_new );
				gr = gr_new;
			}
			else if(  br->get_desc()->get_waytype() == powerline_wt  ) {
				msg = bridge_builder_t::remove(player, gr->get_pos(), powerline_wt );
				return msg == NULL;
			}
		}
		if(gr->ist_tunnel()  &&  gr->ist_karten_boden()) {
			if (gr->find<tunnel_t>()->get_desc()->get_waytype()==powerline_wt) {
				msg = tunnel_builder_t::remove(player, gr->get_pos(), powerline_wt, is_ctrl_pressed() );
				return msg == NULL;
			}
		}
		if(  gr->ist_im_tunnel()  ) {
			lt->cleanup(player);
			delete lt;
			// now everything gone?
			if(  gr->get_top() == 1  ) {
				// delete tunnel too
				tunnel_t *t = gr->find<tunnel_t>();
				t->cleanup(player);
				delete t;
			}
			// unmark kartenboden (is marked during underground mode deletion)
			welt->lookup_kartenboden(k)->clear_flag(grund_t::marked);
			// remove upper or lower ground
			welt->access(k)->boden_entfernen(gr);
			delete gr;
		}
		else {
			lt->cleanup(player);
			if(lt->get_typ() == obj_t::senke)
			{
				// This is an awful fudge, but there does not seem to be any other
				// way of stopping crashes, since, inexplicably, the destructor
				// is not properly called when "lt" is a substation in a city.
				// @author: jamespetts. May 2009
				senke_t* sn = (senke_t*)lt;
				if(sn->city != NULL)
				{
					welt->sync.remove(sn);
				}
			}
			delete lt;
		}
		return true;
	}

	// check for signal
	roadsign_t* rs = gr->find<signal_t>();
	if (rs == NULL) rs = gr->find<roadsign_t>();
	if(rs!=NULL) {
		msg = rs-> is_deletable(player);
		if(msg) {
			return false;
		}
DBG_MESSAGE("tool_remover()",  "removing roadsign at (%s)", pos.get_str());
		weg_t *weg = gr->get_weg(rs->get_desc()->get_wtyp());
		if(  weg==NULL  &&  rs->get_desc()->get_wtyp()==tram_wt  ) {
			weg = gr->get_weg(track_wt);
		}
		rs->cleanup(player);
		delete rs;
		assert( weg );
		weg->count_sign();
		if(weg->get_waytype() == road_wt)
		{
			welt->set_recheck_road_connexions();
		}
		return true;
	}

	// check stations
	halthandle_t halt = gr->get_halt();
DBG_MESSAGE("tool_remover()", "bound=%i",halt.is_bound());
	if (gr->is_halt()  &&  halt.is_bound()  &&  fabrik_t::get_fab(k)==NULL) {
		// halt and not a factory (oil rig etc.)
		const player_t* owner = halt->get_owner();

		if(player_t::check_owner( owner, player ) || (player && player->is_public_service()))
		{
			return haltestelle_t::remove(player, gr->get_pos());
		}
	}

	// catenary or something like this
	wayobj_t* wo = gr->find<wayobj_t>();
	if(wo) {
		msg = wo-> is_deletable(player);
		if(msg) {
			return false;
		}
		wo->cleanup(player);
		delete wo;
		depot_t *dep = gr->get_depot();
		if( dep ) {
			dep->update_win();
		}
		return true;
	}

DBG_MESSAGE("tool_remover()", "check tunnel/bridge");

	// bridge?
	if(gr->ist_bruecke()) {
DBG_MESSAGE("tool_remover()",  "removing bridge from %d,%d,%d",gr->get_pos().x, gr->get_pos().y, gr->get_pos().z);
		bruecke_t* br = gr->find<bruecke_t>();
		// If this is a public right of way being deleted by anyone other than the public service player,
		// then it cannot be deleted unless there is a diversionary route within a specified number of tiles.

		weg_t* w = gr->get_weg(br->get_waytype());
		if(!player->is_public_service() && w && w->is_public_right_of_way())
		{
			msg = check_diversionary_route(pos, gr->get_weg(br->get_waytype()), player);
			if(msg)
			{
				return false;
			}
		}

		if(br->get_desc()->get_waytype() == road_wt)
		{
			welt->set_recheck_road_connexions();
		}
		msg = bridge_builder_t::remove(player, gr->get_pos(), br->get_desc()->get_waytype());
		return msg == NULL;
	}

	// beginning/end of tunnel
	if(gr->ist_tunnel()  &&  gr->ist_karten_boden()) {
DBG_MESSAGE("tool_remover()",  "removing tunnel  from %d,%d,%d",gr->get_pos().x, gr->get_pos().y, gr->get_pos().z);

		if(!player->is_public_service() && gr->get_weg_nr(0)->is_public_right_of_way())
		{
			msg = check_diversionary_route(pos, gr->get_weg_nr(0), player);
			if(msg)
			{
				return false;
			}
		}

		if(gr->get_weg_nr(0)->get_waytype() == road_wt)
		{
			welt->set_recheck_road_connexions();
		}
		waytype_t wegtyp =  gr->get_leitung() ? powerline_wt : gr->get_weg_nr(0)->get_waytype();
		msg = tunnel_builder_t::remove(player, gr->get_pos(), wegtyp, is_ctrl_pressed());
		return msg == NULL;
	}

	// fields
	field_t* f = gr->find<field_t>();
	if (f) {
		msg = f-> is_deletable(player);
		if(msg==NULL) {
			f->cleanup(player);
			delete f;
			// fields have foundations ...
			sint8 dummy;
			welt->access(k)->boden_ersetzen( gr, new boden_t(gr->get_pos(), welt->recalc_natural_slope(k,dummy) ) );
			welt->lookup_kartenboden(k)->calc_image();
			welt->lookup_kartenboden(k)->set_flag( grund_t::dirty );
		}
		return msg == NULL;
	}

	// depots
	depot_t* dep = gr->get_depot();
	if (dep) {
		msg = dep-> is_deletable(player);
		if(msg) {
			return false;
		}
		dep->cleanup(player);
		delete dep;
		return true;
	}

	// Signal boxes
	signalbox_t* sb = gr->get_signalbox();
	if (sb)
	{
		msg = sb-> is_deletable(player);
		if(msg)
		{
			return false;
		}
		hausbauer_t::remove( player, sb, false );
		welt->set_dirty(); // May delete lots of signals
		return true;
	}

	// since buildings can have more than one tile, we must handle them together
	gebaeude_t* gb = gr->find<gebaeude_t>();
	if(gb != NULL) {
		msg = gb-> is_deletable(player);
		if(msg) {
			return false;
		}
		const building_desc_t *building_desc = gb->get_tile()->get_desc();
		if(!building_desc->can_rotate()  &&  welt->cannot_save()) {
			msg = "Not possible in this rotation!";
			return false;
		}
		DBG_MESSAGE("tool_remover()",  "removing building" );

		// remove town? (when removing townhall)
		if(gb->is_townhall()) {
			stadt_t *stadt = welt->find_nearest_city(k);
			if(!welt->remove_city( stadt )) {
				msg = "Das Feld gehoert\neinem anderen Spieler\n";
				return false;
			}
			// townhall is also removed during town removal
		}
		else {
			// Not town hall -- normal building
			// Check affordability for everyone
			sint64 cost = welt->get_settings().cst_multiply_remove_haus * building_desc->get_level();
			if(player != gb->get_owner())
			{
				cost += building_desc->get_level() * 5;
			}
			if(!player_t::can_afford(player, -cost))
			{
				msg = NOTICE_INSUFFICIENT_FUNDS;
				return false;
			}
			// townhall is also removed during town removal
			hausbauer_t::remove( player, gb, false );
		}
		return true;
	}

	// there is a powerline above this tile, but we do not own it
	// so we take it out and add it later again
	if(lt) {
DBG_MESSAGE("tool_remover()",  "took out powerline");
		gr->obj_remove(lt);
	}

	// do not delete crossing, so we remove it
	crossing_t *cr = gr->find<crossing_t>(2);
	if(cr) {
		gr->obj_remove(cr);
	}
	// do not delete pointers - they may come from players on other clients
	zeiger_t *zeiger = gr->find<zeiger_t>();
	if(zeiger) {
		gr->obj_remove(zeiger);
	}
	// do not delete other players label
	label_t *label = gr->find<label_t>();
	if(label) {
		gr->obj_remove(label);
	}

	// remove all other stuff (clouds, ...)
	bool return_ok = false;
	uint8 num_obj = gr->obj_count();
	if(num_obj>0) {
		msg = gr->kann_alle_obj_entfernen(player);
		return_ok = (msg==NULL  &&  !(gr->get_typ()==grund_t::brueckenboden  ||  gr->get_typ()==grund_t::tunnelboden)  &&  gr->obj_loesche_alle(player));
		DBG_MESSAGE("tool_remover()",  "removing everything from %d,%d,%d",gr->get_pos().x, gr->get_pos().y, gr->get_pos().z);
	}

	if(lt) {
		DBG_MESSAGE("tool_remover()",  "add again powerline");
		gr->obj_add(lt);
	}
	if(cr) {
		gr->obj_add(cr);
	}
	if(zeiger) {
		gr->obj_add(zeiger);
	}
	if(label) {
		gr->obj_add(label);
	}

	// could not delete everything
	if(msg) {
		return false;
	}
	if(return_ok) {
		// no sound
		msg = "";
		return true;
	}

	// ok, now we remove every object that should be removed - one by one.
	// the following objects will be removed together
DBG_MESSAGE("tool_remover()", "removing way");

	waytype_t wt = ignore_wt;
	if(gr->get_typ()!=grund_t::tunnelboden  ||  gr->has_two_ways()) {
		weg_t *w = gr->get_weg_nr(1);
		if(gr->get_typ()==grund_t::brueckenboden  &&  w==NULL) {
			// do not delete the middle of a bridge
			return false;
		}
		if(  w  &&  w->get_waytype()==water_wt  ) {
			// remove the other way first
			w = NULL;
		}
		if(w==NULL  ||  w-> is_deletable(player)!=NULL) {
			w = gr->get_weg_nr(0);
			if(w==NULL) {
				// no way at all ...
				return true;
			}
			if(w-> is_deletable(player)!=NULL){
				msg = w-> is_deletable(player);
				return false;
			}
		}

		// If this is a public right of way being deleted by anyone other than the public service player,
		// then it cannot be deleted unless there is a diversionary route within a specified number of tiles.
		if(!player->is_public_service() && w->is_public_right_of_way())
		{
			msg = check_diversionary_route(pos, w, player);
			if(msg)
			{
				return false;
			}
		}

		if(w->get_waytype() == road_wt)
		{
			const koord pos = gr->get_pos().get_2d();
			stadt_t *city = welt->get_city(pos);

			if(city && !player->is_public_service()) {
				if (gr->removing_road_would_disconnect_city_building()) {
					msg = "Cannot delete a road where to do so would leave a city building unconnected by road.";
					return false;
				}
			}
			welt->set_recheck_road_connexions();
		}

		wt = w->get_desc()->get_finance_waytype();
		const sint64 land_refund_cost = welt->get_land_value(w->get_pos()); // Refund the land value to the player who owned the way, as by bulldozing, the player is selling the land.
		player_t* const owner = w->get_owner(); // We must take this here, as the owner is deleted in the next line.
		sint64 cost_sum = gr->weg_entfernen(w->get_waytype(), true);
		if(player == owner)
		{
			cost_sum += land_refund_cost;
		}
		else
		{
			player_t::book_construction_costs(owner, -land_refund_cost, k, wt);
		}
		player_t::book_construction_costs(player, -cost_sum, k, wt);
	}
	else {
		// remove ways and tunnel
		if(  weg_t *weg = gr->get_weg_nr(0)  ) {
			if(gr->get_weg_nr(0)->get_waytype() == road_wt)
			{
				welt->set_recheck_road_connexions();
			}
			gr->remove_everything_from_way(player, weg->get_waytype(), ribi_t::none);
		}
		// tunnel without way: delete anything else
		if(  !gr->hat_wege()  ) {
			gr->obj_loesche_alle(player);
		}
	}

	// remove empty tile
	if(  !gr->ist_karten_boden()  &&  gr->get_top()==0  ) {
		// unmark kartenboden (is marked during underground mode deletion)
		welt->lookup_kartenboden(k)->clear_flag(grund_t::marked);
		// remove upper or lower ground
		welt->access(k)->boden_entfernen(gr);
		delete gr;
	}

	return true;
}

char const* tool_remover_t::check_diversionary_route(koord3d pos, weg_t* w, player_t* player)
{
	grund_t *gr = welt->lookup(pos);

	if(gr->removing_way_would_disrupt_public_right_of_way(w->get_waytype())) {
		return "Cannot remove a public right of way without providing an adequate diversionary route";
	}

	return NULL;
}

const char *tool_remover_t::work( player_t *player, koord3d pos )
{
	DBG_MESSAGE("tool_remover()","at %d,%d", pos.x, pos.y);
	const char *fail = NULL;
	if(!tool_remover_intern(player, pos, fail)) {
		return fail;
	}

	// must recalc neighbourhood for slopes etc.
	if(pos.x>1) {
		welt->lookup_kartenboden(pos.get_2d()+koord::west)->calc_image();
	}
	if(pos.y>1) {
		welt->lookup_kartenboden(pos.get_2d()+ koord::north)->calc_image();
	}

	if(pos.x<welt->get_size().x-1) {
		welt->lookup_kartenboden(pos.get_2d()+koord::east)->calc_image();
	}
	if(pos.y<welt->get_size().y-1) {
		welt->lookup_kartenboden(pos.get_2d()+koord::south)->calc_image();
	}

	return NULL;
}



const char *tool_raise_lower_base_t::move( player_t *player, uint16 buttonstate, koord3d pos )
{
	// This is rough and ready: if you can afford something costing 1, we decide you can afford this
	// --neroden
	if (! player_t::can_afford( player, 1 ) ) {
		return NOTICE_INSUFFICIENT_FUNDS;
	}

	const char *result = NULL;
	if(  buttonstate==1  ) {
		char buf[16];
		if(!is_dragging) {
			drag_height = get_drag_height(pos.get_2d());
		}
		is_dragging = true;
		sprintf( buf, "%i", drag_height );
		default_param = buf;
		if (env_t::networkmode) {
			// queue tool for network
			nwc_tool_t *nwc = new nwc_tool_t(player, this, pos, welt->get_steps(), welt->get_map_counter(), false);
			network_send_server(nwc);
		}
		else {
			result = work( player, pos );
		}
		default_param = NULL;
	}
	return result;
}


const char* tool_raise_lower_base_t::drag(player_t *player, koord k, sint16 height, int &n, bool allow_deep_water)
{
	if(  !welt->is_within_grid_limits(k)  ) {
		return "";
	}
	const char* err = NULL;

	// dragging may be going up or down!
	while(  welt->lookup_hgt(k) < height  &&  height <= welt->get_maximumheight()  ) {
		int diff = welt->grid_raise( player, k, allow_deep_water, err);
		if(  diff == 0  ) {
			break;
		}
		n += diff;
	}

	// when going down need to check here we will not be going below sea level
	// cannot rely on check within lower as water height can be recalculated
	while(  height >= welt->get_water_hgt(k)  &&  welt->lookup_hgt(k) > height  &&  height >= welt->get_minimumheight()  ) {
		int diff = welt->grid_lower( player, k, err );
		if(  diff == 0  ) {
			break;
		}
		n += diff;
	}

	return err; //height == welt->lookup_hgt(k);
}


bool tool_raise_lower_base_t::check_dragging()
{
	// reset dragging
	if(  is_dragging  &&  strempty(default_param)  ) {
		is_dragging = false;
		return false;
	}
	return true;
}


sint16 tool_raise_t::get_drag_height(koord k)
{
	const grund_t *gr = welt->lookup_kartenboden_gridcoords(k);

	return  gr->get_hoehe(welt->get_corner_to_operate(k)) + 1;
}


const char *tool_raise_t::check_pos(player_t *player, koord3d pos )
{
	// check for underground mode
	if(  is_dragging  &&  drag_height-1 > grund_t::underground_level  ) {
		is_dragging = false;
		return "";
	}
	if(  !welt->is_within_grid_limits(pos.get_2d())  ) {
		return "";
	}
	sint8 h = (sint8) get_drag_height(pos.get_2d());
	if(  h > grund_t::underground_level  ) {
		return "Terraforming not possible\nhere in underground view";
	}
	const sint64 cost = welt->get_settings().cst_alter_land;
	if(! player_t::can_afford(player, -cost) )
	{
		return NOTICE_INSUFFICIENT_FUNDS;
	}
	return NULL;
}


const char *tool_raise_t::work(player_t* player, koord3d pos )
{
	if (!check_dragging()) {
		return NULL;
	}

	const char* err = NULL;
	koord k = pos.get_2d();

	// This is rough and ready: if you can afford something costing 1, we decide you can afford this
	// --neroden
	if (! player_t::can_afford( player, 1 ) ) {
		return NOTICE_INSUFFICIENT_FUNDS;
	}

	if(welt->is_within_grid_limits(k)) {

		const sint8 hgt = (sint8) get_drag_height(k);

		if(  hgt <= welt->get_maximumheight()  ) {

			int n = 0;	// tiles changed
			if(  !strempty(default_param)  ) {
				// called by dragging or by AI
				err = drag(player, k, atoi(default_param), n, player->is_public_service());
			}
			else {
				n = welt->grid_raise(player, k, player->is_public_service(), err);
			}



			if(n>0)
			{
				const sint64 cost = welt->get_settings().cst_alter_land * n;
				player_t::book_construction_costs(player, cost, pos.get_2d(), ignore_wt);
				// update image
				for(int j=-n; j<=n; j++)
				{
					for(int i=-n; i<=n; i++)
					{
						const planquadrat_t* p = welt->access(pos.get_2d() + koord(i, j));
						if (p)
						{
							grund_t* g = p->get_kartenboden();
							if (g) g->calc_image();
						}
					}
				}
			}
			return err == NULL ? (n ? NULL : "")
			                   : (*err == 0 ? NOTICE_TILE_FULL : err);
		}
		else {
			// no mountains higher than welt->get_maximumheight() ...
			return "Maximum tile height difference reached.";
		}
	}
	return "Zu nah am Kartenrand";
}


sint16 tool_lower_t::get_drag_height(koord k)
{
	const grund_t *gr = welt->lookup_kartenboden_gridcoords(k);

	return  gr->get_hoehe(welt->get_corner_to_operate(k)) - 1;
}


const char *tool_lower_t::check_pos( player_t *player, koord3d pos )
{
	// check for underground mode
	if (is_dragging  &&  drag_height+1 > grund_t::underground_level) {
		is_dragging = false;
		return "";
	}
	if (! welt->is_within_grid_limits(pos.get_2d())) {
		return "";
	}
	sint8 h = (sint8) get_drag_height(pos.get_2d());
	if (h > grund_t::underground_level) {
			return "Terraforming not possible\nhere in underground view";
	}

	const sint64 cost = welt->get_settings().cst_alter_land;
	if(!player_t::can_afford(player, -cost))
	{
		return NOTICE_INSUFFICIENT_FUNDS;
	}
	return NULL;
}


const char *tool_lower_t::work( player_t *player, koord3d pos )
{
	if (!check_dragging()) {
		return NULL;
	}

	const char* err = NULL;
	koord k = pos.get_2d();

	// This is rough and ready: if you can afford something costing 1, we decide you can afford this
	// --neroden
	if (! player_t::can_afford( player, 1 ) ) {
		return NOTICE_INSUFFICIENT_FUNDS;
	}

	if(welt->is_within_grid_limits(k)) {
		const sint8 hgt = (sint8) get_drag_height(k);

		if(  hgt >= welt->get_water_hgt( k )  ) {
			int n = 0; // tiles changed
			if (!strempty(default_param)) {
				// called by dragging or by AI
				err = drag(player, k, atoi(default_param), n, player->is_public_service());
			}
			else {
				n = welt->grid_lower(player, k, err);
			}
			if(n>0) {
				player_t::book_construction_costs(player, welt->get_settings().cst_alter_land * n, k, ignore_wt);
			}
			return err == NULL ? (n ? NULL : "")
			                   : (*err == 0 ? NOTICE_TILE_FULL : err);
		}
		else {
			// below water level
			return "";
		}
	}
	return "Zu nah am Kartenrand";
}


const char *tool_setslope_t::check_pos( player_t *, koord3d pos)
{
	grund_t *gr1 = welt->lookup(pos);
	if(gr1)
	{
		// check for underground mode
		if(  grund_t::underground_mode == grund_t::ugm_all  &&  !gr1->ist_tunnel()  )
		{
			return "Terraforming not possible\nhere in underground view";
		}
	}
	else
	{
		return "";
	}
	return NULL;
}

const char *tool_restoreslope_t::check_pos( player_t *, koord3d pos)
{
	grund_t *gr1 = welt->lookup(pos);
	if(gr1) {
		// check for underground mode
		if(  grund_t::underground_mode == grund_t::ugm_all  &&  !gr1->ist_tunnel()  ) {
			return "Terraforming not possible\nhere in underground view";
		}
	}
	else {
		return "";
	}
	return NULL;
}

const char *tool_setslope_t::tool_set_slope_work( player_t *player, koord3d pos, int new_slope )
{
	if(  !ground_desc_t::double_grounds  ) {
		// translate old single slope parameter to new double slope
		if(  0 < new_slope  &&  new_slope < ALL_UP_SLOPE_SINGLE  ) {
			new_slope = scorner_sw(new_slope) + scorner_se(new_slope) * 3 + scorner_ne(new_slope) * 9 + scorner_nw(new_slope) * 27;
		}
		else {
			switch(  new_slope  ) {
				case ALL_UP_SLOPE:
				case ALL_UP_SLOPE_SINGLE:   new_slope = ALL_UP_SLOPE;   break;
				case ALL_DOWN_SLOPE:
				case ALL_DOWN_SLOPE_SINGLE: new_slope = ALL_DOWN_SLOPE; break;
				case RESTORE_SLOPE:
				case RESTORE_SLOPE_SINGLE:  new_slope = RESTORE_SLOPE;  break;
				default:
					return ""; // invalid parameter
			}
		}
	}

	bool ok = false;

	grund_t *gr1 = welt->lookup(pos);
	if(  gr1  ) {
		koord k(pos.get_2d());

		sint8 water_hgt = welt->get_water_hgt( k );

		const uint8 max_hdiff = ground_desc_t::double_grounds ?  2 : 1;

		// at least a pixel away from the border?
		if(  pos.z < water_hgt  &&  !gr1->ist_tunnel()  ) {
			return "Maximum tile height difference reached.";
		}

		if(  new_slope==RESTORE_SLOPE  &&  !(gr1->get_typ()==grund_t::boden  ||  gr1->get_typ()==grund_t::wasser)  ) {
			return NOTICE_UNSUITABLE_GROUND;
		}

		// finally: empty enough
		if(  gr1->get_grund_hang()!=gr1->get_weg_hang()  ||  gr1->get_halt().is_bound()  ||  gr1->kann_alle_obj_entfernen(player)  ||
			gr1->find<gebaeude_t>()  ||  gr1->get_depot() || gr1->get_signalbox()  ||  (gr1->get_leitung() && gr1->hat_wege())  ||  gr1->get_weg(air_wt)  ||  gr1->find<label_t>()  ||  gr1->get_typ()==grund_t::brueckenboden) {
			return NOTICE_TILE_FULL;
		}

		if(  !welt->is_within_limits(k+koord(1,1))  ||  !welt->is_within_limits(k+koord(-1,-1))) {
			return "Zu nah am Kartenrand";
		}

		// slopes may affect the position and the total height!
		koord3d new_pos = pos;

		if(  gr1->hat_wege() || gr1->get_leitung() ) {
			// check the resulting slope
			ribi_t::ribi ribis = 0;
			if( gr1->hat_wege()) {
				ribis |= gr1->get_weg_nr(0)->get_ribi_unmasked();
				if(  gr1->get_weg_nr(1)  ) {
					ribis |= gr1->get_weg_nr(1)->get_ribi_unmasked();
				}
			}
			if( gr1->get_leitung()) {
				ribis |= gr1->get_leitung()->get_ribi();
			}

			if(  new_slope==RESTORE_SLOPE  ||  !ribi_t::is_single(ribis)  ||  (new_slope<slope_t::raised  &&  ribi_t::backward(ribi_type(new_slope))!=ribis)  ) {
				// has the wrong tilt
				return NOTICE_TILE_FULL;
			}
			/* new things getting tricky:
			 * A single way on an all up or down slope will result in
			 * a slope with the way as hinge.
			 */
			if(  new_slope==ALL_UP_SLOPE  ) {
				if(  gr1->get_weg_hang()==slope_t::flat  ) {
					new_slope = slope_type(ribis);
				}
				else if(  gr1->get_weg_hang() == slope_type(ribis)  ) {
					// check that way_desc supports such steep slopes
					if(  (gr1->get_weg_nr(0)  &&  !gr1->get_weg_nr(0)->get_desc()->has_double_slopes())
					  ||  (gr1->get_weg_nr(1)  &&  !gr1->get_weg_nr(1)->get_desc()->has_double_slopes())
					  ||  (gr1->get_leitung()  &&  !gr1->get_leitung()->get_desc()->has_double_slopes())  ) {
						return NOTICE_TILE_FULL;
					}
					new_slope = slope_type(ribis) * 2;
				}
				else if(  gr1->get_weg_hang() == slope_type( ribi_t::backward(ribis) ) * 2  ) {
					new_pos.z++;
					if(  welt->lookup(new_pos)  ) {
						return NOTICE_TILE_FULL;
					}
					new_slope = slope_type( ribi_t::backward(ribis) );
				}
				else if(  gr1->get_weg_hang() != slope_type( ribi_t::backward(ribis) )  ) {
					return "Maximum tile height difference reached.";
				}
			}
			else if(  new_slope==ALL_DOWN_SLOPE  ) {
				if(  gr1->get_grund_hang()==slope_type(ribis)  ) {
					// do not lower tiles to sea
					if(  pos.z == water_hgt  &&  !gr1->ist_tunnel()  ) {
						return NOTICE_TILE_FULL;
					}
				}
				else if(  gr1->get_grund_hang() == slope_type(ribis) * 2  ) {
					if(  pos.z == water_hgt  &&  !gr1->ist_tunnel()  ) {
						return NOTICE_TILE_FULL;
					}
					new_slope = slope_type(ribis);
				}
				else if(  gr1->get_grund_hang() == slope_t::flat  ) {
					new_slope = slope_type( ribi_t::backward(ribis) );
					new_pos.z--;
					if(  welt->lookup(new_pos)  ) {
						return NOTICE_TILE_FULL;
					}
				}
				else if(  gr1->get_grund_hang() == slope_type( ribi_t::backward(ribis) )  ) {
					// check that way_desc supports such steep slopes
					if(  (gr1->get_weg_nr(0)  &&  !gr1->get_weg_nr(0)->get_desc()->has_double_slopes())
					  ||  (gr1->get_weg_nr(1)  &&  !gr1->get_weg_nr(1)->get_desc()->has_double_slopes())
					  ||  (gr1->get_leitung()  &&  !gr1->get_leitung()->get_desc()->has_double_slopes())  ) {
						return NOTICE_TILE_FULL;
					}
					new_slope = slope_type( ribi_t::backward(ribis) ) * 2;
					new_pos.z--;
					if(  welt->lookup(new_pos)  ) {
						return NOTICE_TILE_FULL;
					}
				}
				else {
					return "Maximum tile height difference reached.";
				}
			}
		}

		if(  new_slope == ALL_DOWN_SLOPE  ||  new_slope == RESTORE_SLOPE  ) {
			if(  new_slope == RESTORE_SLOPE  ) {
				// prissi: special action: set to natural slope
				sint8 min_hgt;
				new_slope = welt->recalc_natural_slope( k, min_hgt );
				new_pos = koord3d( k, min_hgt );
				DBG_MESSAGE("natural_slope","%i",new_slope);
			}
			else {
				new_slope = slope_t::flat;
				// is more intuitive: if there is a slope, first downgrade it
				if(  gr1->get_grund_hang() == 0  ) {
					new_pos.z--;
				}
			}

			// now prevent being lowered below neighbouring water
			sint8 water_table = (water_hgt >= (gr1->get_hoehe() + (gr1->get_grund_hang() ? 1 : 0))) ? water_hgt : welt->get_groundwater() - 4;
			sint8 min_neighbour_height = gr1->get_hoehe();

			for(  sint16 i = 0 ;  i < 8 ;  i++  ) {
				const koord neighbour = k + koord::neighbours[i];

				if(  welt->is_within_grid_limits( neighbour )  ) {
					grund_t *gr2 = welt->lookup_kartenboden( neighbour );
					const sint8 water_hgt_neighbour = welt->get_water_hgt( neighbour );
					if(  gr2  &&  (water_hgt_neighbour >= (gr2->get_hoehe() + (gr2->get_grund_hang() ? 1 : 0)))  ) {
						water_table = max( water_table, water_hgt_neighbour );
					}
					if(  gr2  &&  gr2->get_hoehe() < min_neighbour_height  ) {
						min_neighbour_height = gr2->get_hoehe();
					}
				}
			}

			if(  water_table>new_pos.z  ||  (water_table == new_pos.z  &&  min_neighbour_height < new_pos.z)  ) {
				// do not lower tiles when it will be below water level
				return NOTICE_TILE_FULL;
			}
			welt->set_water_hgt( k, water_table );
			water_hgt = water_table;
		}
		else if(  new_slope == ALL_UP_SLOPE  ) {
			new_slope = slope_t::flat;
			new_pos.z++;
		}

		// already some ground here (tunnel, bridge, monorail?)
		if(  new_pos.z != pos.z  &&  welt->lookup(new_pos) != NULL  ) {
			return NOTICE_TILE_FULL;
		}
		// check for grounds above / below
		if(  new_pos.z >= pos.z  ) {
			grund_t *gr2 = welt->lookup( new_pos + koord3d(0, 0, 1) );
			if(  !gr2  ) {
				gr2 = welt->lookup( new_pos + koord3d(0, 0, 2) );
			}
			if(  !gr2  &&  welt->get_settings().get_way_height_clearance()==2  &&  (gr1->hat_wege()  ||  gr1->get_leitung())  ) {
				gr2 = welt->lookup( new_pos + koord3d(0, 0, 3) );
			}
			// slope may alter amount of clearance required
			if(  gr2  &&  gr2->get_pos().z - new_pos.z + slope_t::min_diff( gr2->get_weg_hang(), new_slope ) < welt->get_settings().get_way_height_clearance()  ) {
				return NOTICE_TILE_FULL;
			}
		}
		if(  new_pos.z <= pos.z  ) {
			grund_t *gr2 = welt->lookup( new_pos + koord3d(0, 0, -1) );
			if(  !gr2  ) {
				gr2 = welt->lookup( new_pos + koord3d(0, 0, -2) );
			}
			if(  !gr2  &&  welt->get_settings().get_way_height_clearance()==2  ) {
				gr2 = welt->lookup( new_pos + koord3d(0, 0, -3) );
			}
			// slope may alter amount of clearance required
			if(  gr2  &&  new_pos.z - gr2->get_pos().z + slope_t::min_diff( new_slope, gr2->get_weg_hang() ) < welt->get_settings().get_way_height_clearance()  ) {
				return NOTICE_TILE_FULL;
			}
		}

		// check, if action is valid ...
		const sint16 hgt=new_pos.z;
		// maximum difference check with tiles to north, south east and west
		const sint8 test_hgt = hgt+(new_slope!=0);

		if(  gr1->get_typ()==grund_t::boden  ) {
			for(  sint16 i = 0 ;  i < 4 ;  i++  ) {
				const koord neighbour = k + koord::nsew[i];

				const grund_t *gr_neighbour=welt->lookup_kartenboden(neighbour);
				if(gr_neighbour) {
					const sint16 gr_neighbour_hgt=gr_neighbour->get_hoehe() + (new_slope==ALL_DOWN_SLOPE && gr_neighbour->get_grund_hang()? 1 : 0);
					const sint8 diff_from_ground = abs(gr_neighbour_hgt-test_hgt);
					if(  diff_from_ground > 2 * max_hdiff  ) {
						return "Maximum tile height difference reached.";
					}
				}
			}
		}

		// ok, now we set the slope ...
		ok = (new_pos!=pos);
		bool slope_changed = new_slope!=gr1->get_grund_hang();
		ok |= slope_changed;

		if(ok) {
			if(  gr1->kann_alle_obj_entfernen(player)  ) {
				// not empty ...
				return NOTICE_TILE_FULL;
			}
			// check way ownership
			if(gr1->hat_wege()) {
				if(gr1->get_weg_nr(0)-> is_deletable(player)!=NULL) {
					return NOTICE_TILE_FULL;
				}
				if(gr1->has_two_ways()  &&  gr1->get_weg_nr(1)-> is_deletable(player)!=NULL) {
					return NOTICE_TILE_FULL;
				}
			}

			// ok, it was a success
			if(  !gr1->is_water()  &&  new_slope == 0  &&  hgt == water_hgt  &&  gr1->get_typ() != grund_t::tunnelboden  ) {
				// now water
				gr1->obj_loesche_alle(player);
				welt->access(k)->kartenboden_setzen( new wasser_t(new_pos) );
				gr1 = welt->lookup_kartenboden(k);
			}
			else if(  gr1->is_water()  &&  (new_pos.z > water_hgt  ||  new_slope != 0)  ) {
				// build underwater hill first
				if(  !welt->flatten_tile( player, k, water_hgt, false, true )  ) {
					return NOTICE_TILE_FULL;
				}
				gr1->obj_loesche_alle(player);
				welt->access(k)->kartenboden_setzen( new boden_t(new_pos,new_slope) );
				gr1 = welt->lookup_kartenboden(k);
				welt->set_water_hgt(k, welt->get_groundwater()-4);
			}
			else {
				gr1->set_grund_hang(new_slope);
				gr1->set_pos(new_pos);
				gr1->clear_flag(grund_t::marked);
				gr1->set_flag(grund_t::dirty);
				// update new positions if changed
				if(  new_pos!=pos  ) {
					for(  int i=0;  i<gr1->get_top();  i++  ) {
						gr1->obj_bei(i)->set_pos( new_pos );
					}
				}
				// correct tree offsets if slope has changed
				if(  slope_changed  ) {
					for(  int i=0;  i<gr1->get_top();  i++  ) {
						baum_t *tree = obj_cast<baum_t>(gr1->obj_bei(i));
						if (tree) {
							tree->recalc_off();
						}
					}
				}
				if(  !gr1->ist_karten_boden()  ) {
					gr1->calc_image();
				}
			}

			// if there is a powerline here we need to treat it as newly built as it may connect to neighbours
			leitung_t *lt = gr1->get_leitung();
			if(  lt  ) {
				// remove maintenance for existing powerline
				player_t::add_maintenance(lt->get_owner(), -lt->get_desc()->get_maintenance(), powerline_wt);
				lt->finish_rd();
			}

			if(  gr1->ist_karten_boden()  ) {
				if(  new_slope!=slope_t::flat  ) {
					// no lakes on slopes ...
					groundobj_t *obj = gr1->find<groundobj_t>();
					if(  obj  &&  obj->get_desc()->get_phases()!=16  ) {
						obj->cleanup(player);
						delete obj;
					}
					// connect canals to sea
					if(  gr1->get_hoehe() == water_hgt  &&  gr1->hat_weg(water_wt)  ) {
						grund_t *sea = welt->lookup_kartenboden(k - koord( ribi_type(new_slope ) ));
						if (sea  &&  sea->is_water()) {
							gr1->weg_erweitern(water_wt, ribi_t::backward(ribi_type(new_slope)));
							sea->calc_image();
						}
					}
				}
				// recalc slope walls on neighbours
				for(int y=-1; y<=1; y++) {
					for(int x=-1; x<=1; x++) {
						grund_t *gr = welt->lookup_kartenboden(k+koord(x,y));
						gr->calc_image();
					}
				}
				// correct the grid height
				if(  gr1->is_water()  ) {
					sint8 grid_hgt = min( water_hgt, welt->lookup_hgt( k ) );
					welt->set_grid_hgt(k, grid_hgt );
				}
				else {
					welt->set_grid_hgt(k, gr1->get_hoehe()+ corner_nw(gr1->get_grund_hang()) );
				}
				reliefkarte_t::get_karte()->calc_map_pixel(k);

				welt->calc_climate( k, true );
			}
			settings_t const& s = welt->get_settings();
			player_t::book_construction_costs(player, new_slope == RESTORE_SLOPE ? s.cst_alter_land : s.cst_set_slope, k, ignore_wt);
		}
		// update limits
		if (welt->min_height > gr1->get_hoehe()) {
			welt->min_height = gr1->get_hoehe();
		}
		else if (welt->max_height < gr1->get_hoehe()) {
			welt->max_height = gr1->get_hoehe();
		}
	}
	return ok ? NULL : "";
}



// set marker
const char *tool_marker_t::work( player_t *player, koord3d pos )
{
	if(!welt->is_within_limits(pos.get_2d()))
		return "Cannot set marker off the edge of the world.\n";

	grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
	if (!gr)
		return "Cannot set marker here.\n";

	// check for underground mode
	if (gr->get_hoehe() > pos.z)
		return "";

	if(!gr->get_text())	{
		const obj_t* thing = gr->obj_bei(0);
		//const label_t* l = gr->find<label_t>();

		if(thing == NULL  ||  thing->get_owner() == player  ||  (player_t::check_owner(thing->get_owner(), player)  &&  (thing->get_typ() != obj_t::gebaeude)))
		{
			const sint64 cost = welt->get_land_value(gr->get_pos());
			if(! player_t::can_afford(player, -cost) )
			{
				return NOTICE_INSUFFICIENT_FUNDS;
			}
			gr->obj_add(new label_t(gr->get_pos(), player, default_param ? default_param : "\0"));
			if (is_local_execution()) {
				gr->find<label_t>()->show_info();
			}
			return "";
		}
		return "Das Feld gehoert\neinem anderen Spieler\n";
	}

	return "There is already a marker here.\n";
}



// show/repair blocks
bool tool_clear_reservation_t::init( player_t * )
{
	if (is_local_execution()) {
		schiene_t::show_reservations = true;
		welt->set_dirty();
	}
	return true;
}

bool tool_clear_reservation_t::exit( player_t * )
{
	if (is_local_execution()) {
		schiene_t::show_reservations = false;
		welt->set_dirty();
	}
	return true;
}

const char *tool_clear_reservation_t::work( player_t *player, koord3d k )
{
	grund_t *gr = welt->lookup(k);
	const char* err = NULL;
	if(gr) {

		for(unsigned wnr=0;  wnr<2;  wnr++  ) {

			schiene_t const* const w = obj_cast<schiene_t>(gr->get_weg_nr(wnr));

			if(w == NULL)
			{
				continue;
			}

			// Does this way belong to the player using the tool?
			// The public player can use it universally.
			if(player->get_player_nr() != 1 && w->get_player_nr() != player->get_player_nr())
			{
				err = "Cannot edit block reservations on another player's way.";
				continue;
			}

			// is this a reserved track?
			if(w->is_reserved(schiene_t::block) || w->is_reserved(schiene_t::directional) || w->is_reserved(schiene_t::priority))
			{
				/* now we do a very crude procedure:
				 * - we search all ways for reservations of this convoi and remove them
				 * - we set the convoi state to ROUTING_1; it must reserve again its ways then
				 */
				const waytype_t waytype = w->get_waytype();
				const convoihandle_t cnv = w->get_reserved_convoi();
				if(cnv->get_state()==convoi_t::DRIVING) {
					// reset driving state
					if (cnv->get_is_choosing())
					{
						// There is no need to recalculate the convoy's route unless it has a route set by a choose signal.
						cnv->suche_neue_route();
					}
					vehicle_t* veh = cnv->front();
					if (veh->get_waytype() == track_wt || veh->get_waytype() == tram_wt || veh->get_waytype() == narrowgauge_wt || veh->get_waytype() == maglev_wt || veh->get_waytype() == monorail_wt)
					{
						rail_vehicle_t* rv = (rail_vehicle_t*)veh;
						rv->set_working_method(drive_by_sight);
					}
				}
				cnv->unreserve_route();
				cnv->reserve_own_tiles();
			}
		}
	}
	return err;
}


// transformer for electricity supply
const char* tool_transformer_t::get_tooltip(const player_t *) const
{
	settings_t const& s = welt->get_settings();
	sprintf(toolstr, "%s, %ld$ (%ld$)",
		translator::translate("Build drain"),
		(long)(s.cst_transformer/-100l),
		(long)(welt->calc_adjusted_monthly_figure(s.cst_maintain_transformer))/-100l );
	return toolstr;
}

image_id tool_transformer_t::get_icon(player_t*) const
{
	return way_builder_t::waytype_available( powerline_wt, welt->get_timeline_year_month() ) ? icon : IMG_EMPTY;
}

bool tool_transformer_t::init( player_t *)
{
	return way_builder_t::waytype_available( powerline_wt, welt->get_timeline_year_month() );
}


const char *tool_transformer_t::check_pos( player_t *, koord3d pos )
{
	if(grund_t::underground_mode == grund_t::ugm_all  &&  env_t::networkmode) {
		// clients cannot guess at which height transformer should be build
		return "Cannot built this station/building\nin underground mode here.";
	}
	if(grund_t::underground_mode == grund_t::ugm_level) {
		// only above or directly under surface
		// taking into account way clearance requirements
		grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
		return (gr->get_pos() == pos || gr->get_hoehe() == grund_t::underground_level + welt->get_settings().get_way_height_clearance()) ? NULL : "";
	}
	return NULL;
}


const char *tool_transformer_t::work( player_t *player, koord3d pos )
{
	DBG_MESSAGE("tool_transformer_t()","called on %d,%d", pos.x, pos.y);
	const sint64 cost = welt->get_settings().cst_transformer + welt->get_land_value(pos);
	if(!player_t::can_afford(player, -cost) )
	{
		return NOTICE_INSUFFICIENT_FUNDS;
	}
	grund_t *gr=welt->lookup_kartenboden(pos.get_2d());

	if(  !welt->get_settings().get_allow_underground_transformers()  &&  pos.z!=gr->get_hoehe()  ) {
		// no underground transformers allowed
		return "Cannot built this station/building\nin underground mode here.";
	}

	bool underground = false;
	fabrik_t *fab = NULL;
	// full underground mode: coordinate is on ground, adjust it to one level below ground
	// not possible in network mode!
	if (!env_t::networkmode  &&  grund_t::underground_mode == grund_t::ugm_all) {
		pos = gr->get_pos() - koord3d( 0, 0, welt->get_settings().get_way_height_clearance() );
	}
	// search for factory
	// must be independent of network mode
	if (gr->get_pos().z <= pos.z) {
		fab = leitung_t::suche_fab_4(pos.get_2d());
	}
	else if( gr->get_pos().z == pos.z+welt->get_settings().get_way_height_clearance()  ) {
		fab = fabrik_t::get_fab(pos.get_2d());
		underground = true;
	}

	// Check whether the transformer (substation) is within city limits.
	// @author: jamespetts
	stadt_t* city = welt->get_city(pos.get_2d());

	if(fab != NULL)
	{
		if(fab->is_transformer_connected() && city == NULL)
		{
			return "Only one transformer per factory!";
		}
	}
	else
	{
		if(city == NULL)
		{
			return "Transformer only next to factory or in city!";
		}
	}

	// underground: first build tunnel tile	at coordinate pos
	if(underground) {
		if(gr->is_water()) {
			return "Cannot build transformer in water.";
		}

		if(welt->lookup(pos)) {
			return NOTICE_TILE_FULL;
		}

		if(  welt->get_settings().get_way_height_clearance()==2 && welt->lookup(pos + koord3d( 0, 0, 1 ))  ) {
		        return NOTICE_TILE_FULL;
		}

		const tunnel_desc_t *tunnel_desc = tunnel_builder_t::get_tunnel_desc(powerline_wt, 0, 0);
		if(  tunnel_desc==NULL  ) {
			return "Cannot built this station/building\nin underground mode here.";
		}

		tunnelboden_t* tunnel = new tunnelboden_t(pos, 0);
		welt->access(pos.get_2d())->boden_hinzufuegen(tunnel);
		tunnel->obj_add(new tunnel_t(pos, player, tunnel_desc));
		player_t::add_maintenance( player, tunnel_desc->get_maintenance(), tunnel_desc->get_finance_waytype() );
		gr = tunnel;
	}
	else {
		// above ground: check for clear tile
		if(gr->get_grund_hang()!=0  ||  !gr->ist_natur()) {
			return "Transformer only on flat bare land!";
		}
		// remove everything on that spot
		const char *fail = gr->kann_alle_obj_entfernen(player);
		if(fail)
		{
			return fail;
		}
		gr->obj_loesche_alle(player);
	}
	// transformer will be build on tile pointed to by gr

	// build source or drain depending on factory type
	leitung_t* check = NULL;
	if(fab != NULL && fab->get_desc()->is_electricity_producer())
	{
		pumpe_t *p = new pumpe_t(gr->get_pos(), player);
		gr->obj_add( p );
		p->finish_rd();
		check = (leitung_t*)p;
	}
	else
	{
		senke_t *s = new senke_t(gr->get_pos(), player, city);
		gr->obj_add(s);
		s->finish_rd();
		check = (leitung_t*)s;
	}
	if (fab != NULL && (city == NULL) || (fab && fab->get_desc()->is_electricity_producer()))
	{
		// Do not connect directly to factories that are in cities, except for power stations.
		fab->set_transformer_connected(check);
	}

	return NULL;	// ok
}



/**
 * found a new city
 * @author Hj. Malthaner
 */
const char *tool_add_city_t::work( player_t *player, koord3d pos )
{
	const sint64 cost = welt->get_settings().cst_found_city;
	if (! player_t::can_afford(player, -cost) )
	{
		return NOTICE_INSUFFICIENT_FUNDS;
	}

	koord k(pos.get_2d());

	grund_t *gr = welt->lookup_kartenboden(k);
	if(gr) {
		if(gr->ist_natur() &&
			!gr->is_water() &&
			gr->get_grund_hang() == 0  &&
			hausbauer_t::get_special( 0, building_desc_t::townhall, welt->get_timeline_year_month(), 0, welt->get_climate( k ) ) != NULL  ) {

			gebaeude_t const* const gb = obj_cast<gebaeude_t>(gr->first_obj());
			if(gb && gb->is_townhall()) {
				dbg->warning("tool_add_city()", "Already a city here");
				return NOTICE_TILE_FULL;
			}
			else {

				// Hajo: if city is owned by player and player removes special
				// buildings the game crashes. To avoid this problem cities
				// always belong to player 1

				int const citizens = (int)(welt->get_settings().get_mean_einwohnerzahl() * 0.9);
				//  stadt_t *stadt = new stadt_t(welt->get_public_player();, pos,citizens/10+simrand(2*citizens+1));

				// always start with 1/10 citizens
				stadt_t* stadt = new stadt_t(welt->get_public_player(), k, citizens / 10);
				if (stadt->get_buildings() == 0) {
					delete stadt;
					return NOTICE_UNSUITABLE_GROUND;
				}

				welt->add_city(stadt);
				stadt->finish_rd();

				player_t::book_construction_costs(player, welt->get_settings().cst_found_city, k, ignore_wt);
				reliefkarte_t::get_karte()->calc_map();
				return NULL;
			}
		}
		else {
			return NOTICE_UNSUITABLE_GROUND;
		}
	}
	return "";
}

// buy a house
const char *tool_buy_house_t::work( player_t *player, koord3d pos)
{
	if ( player == welt->get_public_player() ) {
		return "";
	}
	grund_t* gr = welt->lookup_kartenboden(pos.get_2d());
	if(!gr  ||  gr->hat_wege()  ||  gr->get_halt().is_bound()) {
		return "";
	}

	// since buildings can have more than one tile, we must handle them together
	gebaeude_t* gb = gr->find<gebaeude_t>();
	if(  gb== NULL  ||  gb->is_city_building()  ||  !player_t::check_owner(gb->get_owner(),player)  ) {
		return "Das Feld gehoert\neinem anderen Spieler\n";
	}

	if(  gb->get_owner()==player  ) {
		// I bought this already ...
		return "";
	}

	player_t *old_owner = gb->get_owner();
	const building_tile_desc_t *tile  = gb->get_tile();
	const building_desc_t * bdsc = tile->get_desc();
	koord size = bdsc->get_size( tile->get_layout() );

	koord k;
	for(k.y = 0; k.y < size.y; k.y ++) {
		for(k.x = 0; k.x < size.x; k.x ++) {
			const grund_t *gr = welt->lookup(koord3d(k,0)+pos);
			if(gr) {
				gebaeude_t *gb_part = gr->find<gebaeude_t>();
				// there may be buildings with holes
				if(  gb_part  &&  gb_part->get_tile()->get_desc()==bdsc  &&  player_t::check_owner(gb_part->get_owner(),player)  ) {
					const sint64 cost = welt->get_land_value(gr->get_pos()) * bdsc->get_level() * 5; // Developed land is more valuable than undeveloped land.
					if(!player_t::can_afford(player, -cost))
					{
						return NOTICE_INSUFFICIENT_FUNDS;
					}
					sint32 const maint = welt->get_settings().maint_building * bdsc->get_level();
					player_t::add_maintenance(old_owner, -maint, gb->get_waytype());
					player_t::add_maintenance(player,        +maint, gb->get_waytype());
					gb->set_owner(player);
					player_t::book_construction_costs(player, cost, k + pos.get_2d(), gb->get_waytype());
				}
			}
		}
	}
	return NULL;
}

/* change city size
 * @author prissi
 */
bool tool_change_city_size_t::init( player_t * )
{
	cursor = atoi(default_param)>0 ? tool_t::general_tool[TOOL_RAISE_LAND]->cursor : tool_t::general_tool[TOOL_LOWER_LAND]->cursor;
	return true;
}

const char *tool_change_city_size_t::work( player_t *, koord3d pos )
{
	stadt_t *city = welt->find_nearest_city(pos.get_2d());
	if(city!=NULL) {
		city->change_size( atoi(default_param) );
		return NULL;
	}
	return "";
}


/* change climate
 * @author kieron
 */
const char *tool_set_climate_t::get_tooltip(player_t const*) const
{
	char temp[1024];
	sprintf( temp, translator::translate( "Set tile climate" ), translator::translate( ground_desc_t::get_climate_name_from_bit((climate)atoi(default_param)) ) );
	return tooltip_with_price( temp,  welt->get_settings().cst_alter_climate );
}

uint8 tool_set_climate_t::is_valid_pos(player_t *player, const koord3d &, const char *& error, const koord3d &)
{
	error = NULL;
	// no dragging in networkmode but for admin
	return env_t::networkmode && !player->is_public_service() ? 1 /*no dragging*/ : 2 /*dragging allowed*/;
}

void tool_set_climate_t::mark_tiles(player_t *, const koord3d &start, const koord3d &end)
{
	koord k1, k2;
	k1.x = start.x < end.x ? start.x : end.x;
	k1.y = start.y < end.y ? start.y : end.y;
	k2.x = start.x + end.x - k1.x;
	k2.y = start.y + end.y - k1.y;
	koord k;
	for(  k.x = k1.x;  k.x <= k2.x;  k.x++  ) {
		for(  k.y = k1.y;  k.y <= k2.y;  k.y++  ) {
			grund_t *gr = welt->lookup_kartenboden( k );

			zeiger_t *marker = new zeiger_t(gr->get_pos(), NULL );

			const uint8 grund_hang = gr->get_grund_hang();
			const uint8 weg_hang = gr->get_weg_hang();
			const uint8 hang = max( corner_sw(grund_hang), corner_sw(weg_hang) ) + 3 * max( corner_se(grund_hang), corner_se(weg_hang) ) + 9 * max( corner_ne(grund_hang), corner_ne(weg_hang) ) + 27 * max( corner_nw(grund_hang), corner_nw(weg_hang) );
			uint8 back_hang = (hang % 3) + 3 * ((uint8)(hang / 9)) + 27;
			marker->set_after_image( ground_desc_t::marker->get_image( grund_hang % 27 ) );
			marker->set_image( ground_desc_t::marker->get_image( back_hang ) );

			marker->mark_image_dirty( marker->get_image(), 0 );
			gr->obj_add( marker );
			marked.insert( marker );
		}
	}
}


const char *tool_set_climate_t::do_work( player_t *player, const koord3d &start, const koord3d &end )
{
	int n = 0;	// tiles altered
	climate cl = (climate) atoi(default_param);
	koord k1, k2;
	if(  end == koord3d::invalid  ) {
		k1.x = k2.x = start.x;
		k1.y = k2.y = start.y;
	}
	else {
		k1.x = start.x < end.x ? start.x : end.x;
		k1.y = start.y < end.y ? start.y : end.y;
		k2.x = start.x + end.x - k1.x;
		k2.y = start.y + end.y - k1.y;
	}
	koord k;
	for(  k.x = k1.x;  k.x <= k2.x;  k.x++  ) {
		for(  k.y = k1.y;  k.y <= k2.y;  k.y++  ) {
			if(  grund_t *gr=welt->lookup_kartenboden(k)  ) {
				if(  cl != water_climate  ) {
					bool ok = true;
					if(  gr->is_water()  ) {
						const sint8 hgt = welt->lookup_hgt(k);
						ok = welt->get_water_hgt(k) == hgt  &&  welt->is_plan_height_changeable( k.x, k.y );
						// check s, se, e - these must not be deep water!
						for(  int i = 3 ;  i < 6 && ok ;  i++  ) {
							koord k_neighbour(k + koord::neighbours[i]);
							if(  welt->is_within_grid_limits(k_neighbour)  ) {
								ok = welt->lookup_hgt(k_neighbour) >= hgt;
							}
						}
						if(  ok  ) {
							gr->obj_loesche_alle( NULL );
							welt->set_water_hgt( k, hgt - 1 );
							welt->access(k)->correct_water();
						}
					}
					if(  ok  ) {
						welt->set_climate( k, cl, true );
						reliefkarte_t::get_karte()->calc_map_pixel( k );
						n ++;
					}
				}
				else if(  !gr->is_water()  &&  gr->get_grund_hang() == slope_t::flat  &&  welt->is_plan_height_changeable( k.x, k.y )  ) {
					bool ok = true;
					for(  int i = 0 ;  i < 8;  i++  ) {
						grund_t *gr2 = welt->lookup_kartenboden( k + koord::neighbours[i] );
						if(  gr2  &&  ok  ) {
							ok = gr2->get_pos().z >= gr->get_pos().z;
						}
					}
					if(  ok  ) {
						gr->obj_loesche_alle( NULL );
						welt->set_water_hgt( k, gr->get_pos().z );
						welt->access(k)->correct_water();
						welt->set_climate( k, water_climate, true );
						reliefkarte_t::get_karte()->calc_map_pixel( k );
						n ++;
					}
				}

			}
		}
	}
	if(n>0) {
		player_t::book_construction_costs(player, welt->get_settings().cst_alter_climate * n, k, ignore_wt);
	}
	return NULL;
}


/* change water height
 * @author kieron
 */
bool tool_change_water_height_t::init( player_t *player )
{
	cursor = atoi(default_param) > 0 ? tool_t::general_tool[TOOL_RAISE_LAND]->cursor : tool_t::general_tool[TOOL_LOWER_LAND]->cursor;
	return !env_t::networkmode || player->get_player_nr()==1;
}


const char *tool_change_water_height_t::work( player_t *, koord3d pos )
{
	if(  pos == koord3d::invalid  ) {
		return "Cannot alter water";
	}

	// calculate new height to use:
	bool raising = atoi(default_param) > 0;
	koord k = pos.get_2d();
	sint8 new_water_height;
	grund_t *gr = welt->lookup_kartenboden(k);

	if(  gr->is_water()  ) {
		// lower + control removes shallow water only. If this tile is deep water this will fail
		if(  !raising  &&  is_ctrl_pressed()  &&  welt->min_hgt(k)!=gr->get_hoehe()  ) {
			return "Cannot alter water";
		}

		// if currently water, raise = +1, lower = -1
		new_water_height = gr->get_hoehe() + (raising ? 1 : -1);
	}
	// if not water then raise = set water height to ground height, lower = error
	else if(  raising  ) {
		slope_t::type slope = gr->get_grund_hang();
		new_water_height = gr->get_hoehe() + max( max( corner_sw(slope), corner_se(slope) ),max( corner_ne(slope), corner_nw(slope) ) );
	}
	else {
		return "Cannot alter water";
	}
	if(  new_water_height < welt->get_groundwater() - 3  ) {
		return "Cannot alter water";
	}
	sint8 test_height = max( new_water_height, gr->get_hoehe() );

	// make a list of tiles to change
	// cannot use a recursive method as stack is not large enough!

	sint8 *from_dir = new sint8[welt->get_size().x * welt->get_size().y];
	sint8 *stage = new sint8[welt->get_size().x * welt->get_size().y];
	memset( from_dir, -1, sizeof(sint8) * welt->get_size().x * welt->get_size().y );
	memset( stage, -1, sizeof(sint8) * welt->get_size().x * welt->get_size().y );
#define array_koord(px,py) (px + py * welt->get_size().x)
	stage[array_koord(k.x,k.y)]=0;
	do {
		// firstly we must be able to change ground height
		bool ok = welt->is_plan_height_changeable( k.x, k.y )  &&  k.x > 0  &&  k.y > 0  &&  k.x < welt->get_size().x - 1  &&  k.y < welt->get_size().y - 1;
		const planquadrat_t *plan = welt->access(k);

		// next there cannot be any grounds directly above this tile
		sint8 h = plan->get_kartenboden()->get_hoehe() + 1;
		while(  ok  &&  h < new_water_height + welt->get_settings().get_way_height_clearance()  ) {
			if(  plan->get_boden_in_hoehe(h)  ) {
				ok = false;
			}
			h++;
		}

		if(  !ok  ) {
			delete [] from_dir;
			delete [] stage;
			return "Cannot alter water";
		}

		// get neighbour corner heights
		sint8 neighbour_heights[8][4];
		welt->get_neighbour_heights( k, neighbour_heights );

		for(  int i = stage[array_koord(k.x,k.y)];  i < 8;  i++  ) {
			koord k_neighbour = k + koord::neighbours[i];
			grund_t *gr2 = welt->lookup_kartenboden(k_neighbour);
			if(  gr2  ) {
				sint8 neighbour_height = gr2->get_hoehe();

				// move onto this tile if it hasn't been processed yet
				bool ok = stage[array_koord(k_neighbour.x, k_neighbour.y)] == -1;



				if(  raising  ) {
					// test whether points adjacent to current tile will be flooded
					// if control key modifier pressed, level ground will be left alone, but then need to check for spills

					// for neighbour i test corners adjacent to tile
					// nw (i = 0), test se (corner 1)
					// w (i = 1), test se (corner 1) and ne (corner 2)
					// sw (i = 2), test ne (corner 2)
					// s (i = 3), test ne (corner 2) and nw (corner 3)
					// se (i = 4), test nw (corner 3)
					// e (i = 5), test nw (corner 3) and sw (corner 0)
					// ne (i = 6), test sw (corner 0)
					// n (i = 7), test sw (corner 0) and se (corner 1)

					if(  is_ctrl_pressed()  ) {
						ok = ok  &&  ( (gr2->get_grund_hang()!=slope_t::flat  &&  welt->max_hgt(k_neighbour) <= test_height) ||
							neighbour_heights[i][((i >> 1) + 1) & 3] < test_height ||
							( (i & 1)  &&  neighbour_heights[i][((i >> 1) + 2) & 3] < test_height) );
					}
					else {
						ok = ok  &&  (neighbour_heights[i][((i >> 1) + 1) & 3] <= test_height ||
							( (i & 1)  &&  neighbour_heights[i][((i >> 1) + 2) & 3] <= test_height));
					}
					// move onto this tile unless it already has water at new level, or the land level is above new level
					ok = ok  &&  welt->get_water_hgt(k_neighbour) < new_water_height;
				}
				else {
					if(  is_ctrl_pressed()  ) {
						ok = ok  &&  welt->min_hgt(k_neighbour) == test_height;
					}
					else {
						ok = ok  &&  neighbour_height <= test_height;
					}
					// move onto this tile unless it already has water at new level, or the land level is above new level
					ok = ok  &&  welt->get_water_hgt(k_neighbour) > new_water_height;
				}

				if(  ok  ) {
					//move on to next tile
					from_dir[array_koord(k_neighbour.x,k_neighbour.y)] = i;
					stage[array_koord(k_neighbour.x,k_neighbour.y)] = 0;
					stage[array_koord(k.x,k.y)] = i;
					k = k_neighbour;
					break;
				}
			}
			//return back to previous tile
			if(  i==7  ) {
				stage[array_koord(k.x,k.y)] = 8;
				if(  from_dir[array_koord(k.x,k.y)] != -1  ) {
					k = k - koord::neighbours[from_dir[array_koord(k.x,k.y)]];
				}
			}
		}
	} while(  from_dir[array_koord(k.x,k.y)] != -1  ||  stage[array_koord(k.x,k.y)] < 7  );

	delete [] from_dir;

	// loop over map to find marked tiles
	for(  int y = 1;  y<welt->get_size().y - 1;  y++  ) {
		for(  int x = 1;  x<welt->get_size().x - 1;  x++  ) {
			if(  stage[array_koord(x,y)] > -1  ) {
				// calculate new height, slope and climate and set water height
				grund_t *gr2 =welt->lookup_kartenboden(x, y);

				// remove any objects on this tile
				gr2->obj_loesche_alle( NULL );

				const sint8 h0 = gr2->get_hoehe();
				const sint8 min_grid_hgt = welt->min_hgt( koord( x, y ) );

				sint8 h0_nw, h0_ne, h0_se, h0_sw;

				if(  gr2->is_water()  ) {
					// water - maximum existing height can be is old water height no matter what surrounding grids are
					h0_nw = min(h0, welt->lookup_hgt(x, y));
					h0_ne = min(h0, welt->lookup_hgt(x+1, y));
					h0_se = min(h0, welt->lookup_hgt(x+1, y+1));
					h0_sw = min(h0, welt->lookup_hgt(x, y+1));
				}
				else if(  h0 > min_grid_hgt  ) {
					// if min grid height here is less than ground height it will be because we are partially water
					h0_nw = welt->lookup_hgt(x, y);
					h0_ne = welt->lookup_hgt(x+1, y);
					h0_se = welt->lookup_hgt(x+1, y+1);
					h0_sw = welt->lookup_hgt(x, y+1);
					if(  !gr2->is_water()  ) {
						// while this appears to be a single height slope actually it is a double height slope half underwater
						const sint8 water_hgt = welt->get_water_hgt(x, y);
						h0_nw >= water_hgt ? h0_nw = h0 + corner_nw( gr2->get_grund_hang() ) : 0;
						h0_ne >= water_hgt ? h0_ne = h0 + corner_ne( gr2->get_grund_hang() ) : 0;
						h0_se >= water_hgt ? h0_se = h0 + corner_se( gr2->get_grund_hang() ) : 0;
						h0_sw >= water_hgt ? h0_sw = h0 + corner_sw( gr2->get_grund_hang() ) : 0;
					}
				}
				else {
					// fully land
					h0_nw = h0 + corner_nw( gr2->get_grund_hang() );
					h0_ne = h0 + corner_ne( gr2->get_grund_hang() );
					h0_se = h0 + corner_se( gr2->get_grund_hang() );
					h0_sw = h0 + corner_sw( gr2->get_grund_hang() );
				}


				const sint8 hneu_nw = max( new_water_height, h0_nw );
				const sint8 hneu_ne = max( new_water_height, h0_ne );
				const sint8 hneu_se = max( new_water_height, h0_se );
				const sint8 hneu_sw = max( new_water_height, h0_sw );
				const sint8 hneu = min( min( hneu_nw, hneu_ne ), min( hneu_se, hneu_sw ) );

				gr2->set_hoehe( hneu );

				const uint8 sneu = (hneu_sw - hneu > 2 ? 2 : hneu_sw - hneu) + ((hneu_se - hneu > 2 ? 2 : hneu_se-hneu) * 3) + ((hneu_ne - hneu > 2 ? 2 : hneu_ne - hneu) * 9) + ((hneu_nw - hneu > 2 ? 2 : hneu_nw - hneu) * 27);
				gr2->set_grund_hang( sneu );

				welt->set_water_hgt(x, y, new_water_height );
				welt->access(x, y)->correct_water();
				welt->calc_climate( koord( x, y ), true );
			}
		}
	}

	delete [] stage;

	return NULL;
}


char const* tool_plant_tree_t::move(player_t* const player, uint16 const b, koord3d const pos)
{
	if (b==0) {
		return NULL;
	}
	if (env_t::networkmode) {
		// queue tool for network
		nwc_tool_t *nwc = new nwc_tool_t(player, this, pos, welt->get_steps(), welt->get_map_counter(), false);
		network_send_server(nwc);
		return NULL;
	}
	else {
		return work( player, pos );
	}
}


const char *tool_plant_tree_t::work( player_t *player, koord3d pos )
{
	const koord& k(pos.get_2d());

	grund_t *gr = welt->lookup_kartenboden(k);
	if(gr) {

		// check funds
		const sint64 cost = welt->get_settings().cst_remove_tree;
		if (!player_t::can_afford(player, -cost))
		{
			return NOTICE_INSUFFICIENT_FUNDS;
		}
		const tree_desc_t *desc = NULL;
		bool check_climates = true;
		bool random_age = false;
		if(default_param==NULL  ||  strlen(default_param)==0) {
			desc = baum_t::random_tree_for_climate( welt->get_climate( k ) );
		}
		else {
			// parse default_param: bbdesc_nr b=1 ignore climate b=1 random age
			check_climates = default_param[0]=='0';
			random_age = default_param[1]=='1';
			desc = baum_t::find_tree(default_param+3);
		}
		if(desc  &&  baum_t::plant_tree_on_coordinate( k, desc, check_climates, random_age )  ) {
			player_t::book_construction_costs(player, welt->get_settings().cst_remove_tree, k, ignore_wt);
			return NULL;
		}
		return "";
	}
	return NULL;
}


/* the following routines add waypoints/halts to a schedule
 * Players' vehicles can stop at other players' stops when they are allowed access rights.
 * @author prissi
 */
static const char *tool_schedule_insert_aux(karte_t *welt, player_t *player, koord3d pos, schedule_t *schedule, bool append)
{
	if(schedule == NULL)
	{
		dbg->warning("tool_schedule_insert_aux()","Schedule is (null), doing nothing");
		return 0;
	}
	grund_t *bd = welt->lookup(pos);
	if(bd)
	{
		// now just for error messages, we're assuming a valid ground
		// check for right way type
		if(!schedule->is_stop_allowed(bd))
		{
			return schedule->get_error_msg();
		}
		// and check for ownership
		weg_t *w = bd->get_weg(schedule->get_waytype());
		if(w == NULL && schedule->get_waytype() == tram_wt)
		{
			w = bd->get_weg( track_wt );
		}
		if(!bd->is_halt())
		{
			if(w != NULL && w->get_owner() && !w->get_owner()->allows_access_to(player->get_player_nr()))
			{
				return "Das Feld gehoert\neinem anderen Spieler\n";
			}
			if(  bd->get_depot()  &&  !player_t::check_owner( bd->get_depot()->get_owner(), player ))
			{
				return "Das Feld gehoert\neinem anderen Spieler\n";
			}
		}
		if(bd->is_halt() && (!player_t::check_owner(player, bd->get_halt()->get_owner()) && (w != NULL && !(w->get_owner() == NULL || w->get_owner()->allows_access_to(player->get_player_nr())))))
		{
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		// ok, now we have a valid ground
		if(append)
		{
			schedule->append(bd);
		}
		else
		{
			schedule->insert(bd);
		}
	}
	return NULL;
}

const char *tool_schedule_add_t::work( player_t *player, koord3d pos )
{
	return tool_schedule_insert_aux( welt, player, pos, (schedule_t*)const_cast<char *>(default_param), true );
}

const char *tool_schedule_ins_t::work( player_t *player, koord3d pos )
{
	return tool_schedule_insert_aux( welt, player, pos, (schedule_t*)const_cast<char *>(default_param), false );
}


/* way construction */
const way_desc_t *tool_build_way_t::defaults[18] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const way_desc_t *tool_build_way_t::get_desc( uint16 timeline_year_month, bool remember ) const
{
	const way_desc_t *desc = default_param ? way_builder_t::get_desc(default_param,0) : NULL;
	if(  desc==NULL  &&  default_param  ) {
		waytype_t wt = (waytype_t)atoi(default_param);
		desc = defaults[wt & 63];
		if(desc == NULL || !desc->is_available(timeline_year_month)) {
			// Search for default way
			desc = way_builder_t::weg_search(wt, 0xffffffff, timeline_year_month, type_flat);
		}
	}
	if( desc && remember ) {
		if(  desc->get_styp() == type_tram  ) {
			defaults[ tram_wt ] = desc;
		}
		else {
			defaults[desc->get_wtyp()&63] = desc;
		}
	}
	return desc;
}

image_id tool_build_way_t::get_icon(player_t *) const
{
	const way_desc_t *desc = way_builder_t::get_desc(default_param,0);
	bool const elevated = desc ? desc->get_styp() == type_elevated  &&  desc->get_wtyp() != air_wt : false;
	return (grund_t::underground_mode == grund_t::ugm_all && elevated) ? IMG_EMPTY : icon;
}

const char* tool_build_way_t::get_tooltip(const player_t *) const
{
	const way_desc_t *desc = get_desc(welt->get_timeline_year_month(),false);
	if(!desc)
	{
		return "";
	}
	const sint64 tile_price = desc->get_value();
	const sint64 km_price = (tile_price * welt->get_settings().get_base_meters_per_tile()) / welt->get_settings().get_meters_per_tile();
	const sint64 tile_forge_cost = welt->get_settings().get_forge_cost(desc->get_waytype());
	const sint64 km_forge_cost = (tile_forge_cost * welt->get_settings().get_base_meters_per_tile()) / welt->get_settings().get_meters_per_tile();
	const sint64 tile_maint = desc->get_maintenance();
	const sint64 km_maint = (tile_maint * welt->get_settings().get_base_meters_per_tile()) / welt->get_settings().get_meters_per_tile();
	tooltip_with_price_maintenance(welt, desc->get_name(), (-km_price - km_forge_cost), km_maint);
	size_t n= strlen(toolstr);
	n += sprintf(toolstr+n, " / km, %dkm/h, %dt",
		desc->get_topspeed(),
		desc->get_max_axle_load());
	char durability_string[16]; // Need to represent billions plus commas.
	const long double wear_capacity_fractional = (long double)desc->get_wear_capacity() / 10000.0;
	number_to_string(durability_string, wear_capacity_fractional, 4);
	n += sprintf(toolstr+n, ", %s: %s", translator::translate("Durability"), durability_string);
	bool any_prohibitive = false;
	for(sint8 i = 0; i < desc->get_way_constraints().get_count(); i ++)
	{
		if(desc->get_way_constraints().get_prohibitive(i))
		{
			if(!any_prohibitive)
			{
				n += sprintf(toolstr + n, " (" );
				n += sprintf(toolstr + n, translator::translate("Restrictions:"));
			}
			any_prohibitive = true;
			char tmpbuf[30];
			sprintf(tmpbuf, "Prohibitive %i-%i", desc->get_wtyp(), i);
			n += sprintf(toolstr + n, " ");
			n += sprintf(toolstr + n, translator::translate(tmpbuf));
		}
	}
	n = strlen(toolstr);
	if(any_prohibitive)
	{
		strcat(toolstr, ")");
	}
	return toolstr;
}

// default ways are not initialized synchronously for different clients
// always return the name of a way, never the string containing the waytype
const char* tool_build_way_t::get_default_param(player_t *player) const
{
	if (player==NULL) {
		return default_param;
	}
	if (desc) {
		return desc->get_name();
	}
	else {
		if (default_param == NULL) {
			// no distribution_weight to guess anything sensible
			return NULL;
		}
		const way_desc_t* test_desc = get_desc(0, false);
		if (test_desc) {
			return test_desc->get_name();
		}
		else {
			return default_param;
		}
	}
}

bool tool_build_way_t::is_selected() const
{
	tool_t const* const tool = welt->get_tool(welt->get_active_player_nr());
	if (tool->get_id() != get_id()) {
		return false;
	}
	tool_build_way_t const* const selected = dynamic_cast<tool_build_way_t const*>(tool);
	return (selected  &&  selected->get_desc(welt->get_timeline_year_month(),false) == get_desc(welt->get_timeline_year_month(),false));
}

bool tool_build_way_t::init( player_t *player, bool called_from_move )
{
	two_click_tool_t::init( player );
	if( ok_sound == NO_SOUND ) {
		ok_sound = SFX_CASH;
	}

	// now get current desc
	desc = get_desc( welt->get_timeline_year_month(), /*is_local_execution()*/ true );
	if(  desc  &&  desc->get_cursor()->get_image_id(0) != IMG_EMPTY  ) {
		cursor = desc->get_cursor()->get_image_id(0);
	}
	if(  desc  &&  !desc->is_available(welt->get_timeline_year_month())  &&  player!=NULL  &&  player!=welt->get_public_player() ) {
		// non available way => fail
		return false;
	}

  if (  !called_from_move  &&  is_ctrl_pressed()  &&  can_use_gui()  &&  desc->get_waytype()==road_wt  ) {
    create_win(new overtaking_mode_frame_t(player, this), w_info, (ptrdiff_t)this);
  }
	return desc!=NULL;
}

bool tool_build_way_t::exit( player_t *player )
{
	destroy_win((ptrdiff_t)this);
	return two_click_tool_t::exit(player);
}

void tool_build_way_t::draw_after(scr_coord k, bool dirty) const
{
	if(  desc  &&  desc->get_waytype()==road_wt  ) {
		if(  icon!=IMG_EMPTY  &&  is_selected()  ) {
			display_img_blend( icon, k.x, k.y, TRANSPARENT50_FLAG|OUTLINE_FLAG|color_idx_to_rgb(COL_BLACK), false, dirty );
			char level_str[16];
			tool_build_way_t::set_mode_str(level_str, overtaking_mode);
			display_proportional_rgb( k.x+4, k.y+4, level_str, ALIGN_LEFT, color_idx_to_rgb(COL_YELLOW), true );
		}
	} else {
		two_click_tool_t::draw_after(k,dirty);
	}
}

waytype_t tool_build_way_t::get_waytype() const
{
	const way_desc_t *desc = get_desc( welt->get_timeline_year_month(), false );
	if (desc) {
		return desc->is_tram() ? tram_wt : desc->get_wtyp();
	}
	return invalid_wt;
}

uint8 tool_build_way_t::is_valid_pos( player_t *player, const koord3d &pos, const char *&error, const koord3d & )
{
	error = NULL;
	grund_t *gr=welt->lookup(pos);
	if(  gr  &&  slope_t::is_way(gr->get_weg_hang())  ) {

		bool const elevated = desc->get_styp() == type_elevated  &&  desc->get_wtyp() != air_wt;
		// ignore water
		if(  desc->get_wtyp() != water_wt  &&  gr->get_typ() == grund_t::wasser  ) {
			if(  !elevated  ||  welt->lookup_hgt( pos.get_2d() ) < welt->get_water_hgt( pos.get_2d() )  ) {
				return 0;
			}
			// here either channel or elevated way over not too deep water
		}
		// elevated ways have to check tile above
		if(  elevated  ) {
			gr = welt->lookup( pos + koord3d( 0, 0, welt->get_settings().get_way_height_clearance() ) );
			if(  gr == NULL  ) {
				return 2;
			}
			if(  gr->get_typ() != grund_t::monorailboden  ) {
				return 0;
			}
		}
		// test if way already exists on the way and if we are allowed to connect
		weg_t *way = gr->get_weg(desc->get_wtyp());

		if(way) {
			// allow to connect to any road, or anywhere where the player has been granted access rights.
			if(desc->get_wtyp() == road_wt || way->get_owner() == NULL || way->get_owner()->allows_access_to(player->get_player_nr()))
			{
				return 2;
			}
			error = way-> is_deletable(player);
			return error==NULL ? 2 : 0;
		}
		leitung_t* lt = gr->get_leitung();
		if(lt)
		{
			if(!lt->get_owner() || lt->get_owner()->allows_access_to(player->get_player_nr()))
			{
				return 2;
			}
		}
		// check for ownership but ignore moving things
		if(player!=NULL) {
			for(uint8 i=0; i<gr->obj_count(); i++) {
				obj_t* dt = gr->obj_bei(i);
				if (!dt->is_moving() && dt->is_deletable(player) != NULL  &&  dt->get_typ() != obj_t::label) {
					error =  dt-> is_deletable(player); // "Das Feld gehoert\neinem anderen Spieler\n";
					return 0;
				}
			}
		}
	}
	else {
		return 0;
	}
	return 2;
}

void tool_build_way_t::calc_route( way_builder_t &bauigel, const koord3d &start, const koord3d &end )
{
	// recalc type of construction
	way_builder_t::bautyp_t bautyp = (way_builder_t::bautyp_t)desc->get_wtyp();
	if (desc->is_tram()) {
		bautyp = way_builder_t::schiene_tram;
	}
	// elevated track?
	if(desc->get_styp()==type_elevated  &&  desc->get_wtyp()!=air_wt) {
		bautyp |= way_builder_t::elevated_flag;
	}

	bauigel.init_builder(bautyp, desc);
	if(  is_ctrl_pressed()  ) {
		bauigel.set_keep_existing_ways( false );
	}
	else {
		bauigel.set_keep_existing_faster_ways( true );
	}

	if(is_shift_pressed())
	{
		bauigel.set_mark_way_for_upgrade_only(true);
	}
	else
	{
		bauigel.set_mark_way_for_upgrade_only(false);
	}

	if(  is_ctrl_pressed()  ||  (env_t::straight_way_without_control  &&  !env_t::networkmode)  ) {
		DBG_MESSAGE("tool_wegebau()", "try straight route");
		bauigel.calc_straight_route(start,end);
	}
	else {
		bauigel.calc_route(start,end);
	}

	DBG_MESSAGE("tool_wegebau()", "builder found route with %d squares length.", bauigel.get_count());
}

tool_build_way_t* get_build_way_tool_from_toolbar(const way_desc_t* desc) {
	FOR(stringhashtable_tpl<way_desc_t *>, const& i, *(way_builder_t::get_all_ways())) {
		way_desc_t const* const cand = i.value;
		if(  cand==desc  &&  cand->get_builder()  ) {
			return dynamic_cast<tool_build_way_t*> (cand->get_builder());
		}
	}
	return NULL;
}

const char *tool_build_way_t::do_work( player_t *player, const koord3d &start, const koord3d &end )
{
	way_builder_t bauigel(player);
	calc_route( bauigel, start, end );
	overtaking_mode_t mode = overtaking_mode;
	tool_build_way_t* toolbar_tool;
	if(  look_toolbar  &&  (toolbar_tool=get_build_way_tool_from_toolbar(desc))!=NULL ) {
		// look toolbar variable indicates this tool is called from a shortcut key. When a tool is called from a shortcut key, we have to use overtaking_mode of the tool in a toolbar.
		mode = toolbar_tool->get_overtaking_mode();
	}
  bauigel.set_overtaking_mode(mode);
	if(  bauigel.get_route().get_count()>1  ) {
		sint64 cost = bauigel.calc_costs();

		// Check affordability unless just marking to replace later
		if(!is_shift_pressed() && !player_t::can_afford(player, cost))
		{
			return NOTICE_INSUFFICIENT_FUNDS;
		}
		welt->mute_sound(true);
		bauigel.build();
		welt->mute_sound(false);
		if(!is_shift_pressed())
		{
			sound_play(SFX_CASH);
		}

		return NULL;
	}
	return "";
}

void tool_build_way_t::rdwr_custom_data(memory_rw_t *packet)
{
	two_click_tool_t::rdwr_custom_data(packet);
	sint8 i = overtaking_mode;
	// If this tool is called from a shortcut key, overtaking_mode of the tool in a toolbar has to be used.
	if(  packet->is_saving()  &&  look_toolbar  ) {
		tool_build_way_t* toolbar_tool = get_build_way_tool_from_toolbar(desc);
		if(  toolbar_tool  ) {
			i = toolbar_tool->get_overtaking_mode();
		}
	}
	packet->rdwr_byte(i);
	overtaking_mode = (overtaking_mode_t)i;
}

void tool_build_way_t::set_mode_str(char* str, overtaking_mode_t overtaking_mode) {
	assert(str);
	switch (overtaking_mode) {
		case halt_mode:
			sprintf(str, "H");
		break;
		case oneway_mode:
			sprintf(str, "O");
		break;
		case twoway_mode:
			sprintf(str, "T");
		break;
		case loading_only_mode:
			sprintf(str, "L");
		break;
		case prohibited_mode:
			sprintf(str, "P");
		break;
		case inverted_mode:
			sprintf(str, "I");
		break;
		default:
			sprintf(str, "X");
		break;
	}
}

void tool_build_way_t::mark_tiles(  player_t *player, const koord3d &start, const koord3d &end )
{
	way_builder_t bauigel(player);
	calc_route( bauigel, start, end );

	uint8 offset = (desc->get_styp() == type_elevated  &&  desc->get_wtyp() != air_wt) ? welt->get_settings().get_way_height_clearance() : 0;

	if(  bauigel.get_count()>1  ) {
		// Set tooltip first (no dummygrounds, if bauigel.calc_casts() is called).
		const uint32 distance = bauigel.get_count() * welt->get_settings().get_meters_per_tile();
		win_set_static_tooltip( tooltip_with_price_and_distance("Building costs estimates", -bauigel.calc_costs(), distance) );

		// make dummy route from bauigel
		for(  uint32 j=0;  j<bauigel.get_count();  j++   ) {
			koord3d pos = bauigel.get_route()[j] + koord3d(0,0,offset);
			grund_t *gr = welt->lookup( pos );
			if( !gr ) {
				gr = new monorailboden_t(pos, 0);
				// should only be here when elevated/monorail, therefore will be at height offset above ground
				gr->set_grund_hang( welt->lookup( pos - koord3d( 0, 0, offset ) )->get_grund_hang() );
				welt->access(pos.get_2d())->boden_hinzufuegen(gr);
			}
			ribi_t::ribi zeige = gr->get_weg_ribi_unmasked(desc->get_wtyp()) | bauigel.get_route().get_ribi( j );

			zeiger_t *way = new zeiger_t(pos, NULL );
			if(gr->get_weg_hang()) {
				way->set_image( desc->get_slope_image_id(gr->get_weg_hang(),0) );
			}
			else if(desc->get_wtyp()!=powerline_wt  &&  ribi_t::is_bend(zeige)  &&  desc->has_diagonal_image()) {
				way->set_image( desc->get_diagonal_image_id(zeige,0) );
			}
			else {
				way->set_image( desc->get_image_id(zeige,0) );
			}
			gr->obj_add( way );
			way->set_yoff(-gr->get_weg_yoff() );
			marked.insert( way );
			way->mark_image_dirty( way->get_image(), 0 );
		}
	}
}


/* city road construction */
const way_desc_t *tool_build_cityroad::get_desc(uint16,bool) const
{
	return welt->get_city_road();
}

const char *tool_build_cityroad::do_work( player_t *player, const koord3d &start, const koord3d &end )
{
	way_builder_t bauigel(player);
	bauigel.set_build_sidewalk(true);
  bauigel.set_overtaking_mode(overtaking_mode);
	calc_route( bauigel, start, end );
	if(  bauigel.get_route().get_count()>1  ) {
		welt->mute_sound(true);
		bauigel.build();
		welt->mute_sound(false);

		return NULL;
	}
	return "";
}


/* bridge construction */
const char* tool_build_bridge_t::get_tooltip(const player_t *) const
{
	const bridge_desc_t * desc = bridge_builder_t::get_desc(default_param);
	if (desc == NULL) {
		return "";
	}
	tooltip_with_price_maintenance( welt, desc->get_name(), -desc->get_value(), desc->get_maintenance());
	size_t n= strlen(toolstr);
	if(desc->get_waytype()!=powerline_wt) {
		n += sprintf(toolstr+n, ", %dkm/h, %dt",
			desc->get_topspeed(),
			desc->get_max_weight());
	}
	if(desc->get_max_length()>0) {
		n += sprintf(toolstr+n, ", %d %s", desc->get_max_length(), translator::translate("tiles"));
	}
	bool any_prohibitive = false;
	for(sint8 i = 0; i < desc->get_way_constraints().get_count(); i ++)
	{
		if(desc->get_way_constraints().get_prohibitive(i))
		{
			if(!any_prohibitive)
			{
				n += sprintf(toolstr + n, " (" );
				n += sprintf(toolstr + n, translator::translate("Restrictions:"));
			}
			any_prohibitive = true;
			char tmpbuf[30];
			sprintf(tmpbuf, "Prohibitive %i-%i", desc->get_wtyp(), i);
			n += sprintf(toolstr + n, " ");
			n += sprintf(toolstr + n, translator::translate(tmpbuf));
		}
	}
	n = strlen(toolstr);
	if(any_prohibitive)
	{
		strcat(toolstr, ")");
	}
	return toolstr;
}

waytype_t tool_build_bridge_t::get_waytype() const
{
	const bridge_desc_t * desc = bridge_builder_t::get_desc(default_param);
	return desc ? desc->get_waytype() : invalid_wt;
}

bool tool_build_bridge_t::init( player_t *player )
{
	two_click_tool_t::init( player );
	// now get current desc
	const bridge_desc_t *desc = bridge_builder_t::get_desc(default_param);
	if(  desc  &&  !desc->is_available(welt->get_timeline_year_month())  &&  player!=NULL  &&  player!=welt->get_public_player()) {
		return false;
	}
	if (is_ctrl_pressed()  &&  can_use_gui()  &&  desc->get_waytype()==road_wt  ) {
		create_win(new overtaking_mode_frame_t(player, this), w_info, (ptrdiff_t)this);
	}
	return desc!=NULL;
}

bool tool_build_bridge_t::exit( player_t *player )
{
	destroy_win((ptrdiff_t)this);
	return two_click_tool_t::exit(player);
}

void tool_build_bridge_t::draw_after(scr_coord k, bool dirty) const
{
	const bridge_desc_t *desc = bridge_builder_t::get_desc(default_param);
	if(  desc  &&  desc->get_waytype()==road_wt  ) {
		if(  icon!=IMG_EMPTY  &&  is_selected()  ) {
			display_img_blend( icon, k.x, k.y, TRANSPARENT50_FLAG|OUTLINE_FLAG|color_idx_to_rgb(COL_BLACK), false, dirty );
			char level_str[16];
			tool_build_way_t::set_mode_str(level_str, overtaking_mode);
			display_proportional_rgb( k.x+4, k.y+4, level_str, ALIGN_LEFT, color_idx_to_rgb(COL_YELLOW), true );
		}
	} else {
		two_click_tool_t::draw_after(k,dirty);
	}
}

const char *tool_build_bridge_t::do_work( player_t *player, const koord3d &start, const koord3d &end )
{
	const bridge_desc_t *desc = bridge_builder_t::get_desc(default_param);
	if (end==koord3d::invalid) {
		return bridge_builder_t::build( player, start, desc, overtaking_mode );
	}
	else {
		const koord zv(ribi_type(end-start));
		sint8 bridge_height;
		const char *error;
		koord3d end2 = bridge_builder_t::find_end_pos(player, start, zv, desc, error, bridge_height, false, koord_distance(start, end), is_ctrl_pressed());
		assert(end2 == end); (void)end2;
		waytype_t wt = desc->get_waytype();
		if (way_desc == NULL || (way_desc->get_styp() == type_elevated  &&  way_desc->get_wtyp() != air_wt))
		{
			// Cannot build an elevated way on top of a bridge
			way_desc = desc->get_way_desc();
		}
		bridge_builder_t::build_bridge( player, start, end, zv, bridge_height, desc, way_desc ? way_desc : way_builder_t::weg_search(desc->get_waytype(), desc->get_topspeed(), welt->get_timeline_year_month(), type_flat), overtaking_mode);
		return NULL; // all checks are performed before building.
	}
}

void tool_build_bridge_t::rdwr_custom_data(memory_rw_t *packet)
{
	two_click_tool_t::rdwr_custom_data(packet);
	uint8 i = ribi;
	packet->rdwr_byte(i);
	ribi = (ribi_t::ribi)i;
	sint8 j = overtaking_mode;
	packet->rdwr_byte(j);
	overtaking_mode = (overtaking_mode_t)j;
	if (packet->is_loading())
	{
		plainstring way_desc_string;
		packet->rdwr_str(way_desc_string);
		way_desc = way_builder_t::get_desc(way_desc_string);
	}
	else
	{
		const bridge_desc_t* bb = bridge_builder_t::get_desc(default_param);
		const waytype_t wt = bb ? bb->get_waytype() : invalid_wt;
		way_desc = tool_build_way_t::defaults[wt & 63];

		plainstring way_desc_string = way_desc ? way_desc->get_name() : "none";
		packet->rdwr_str(way_desc_string);
	}
}

void tool_build_bridge_t::mark_tiles(  player_t *player, const koord3d &start, const koord3d &end )
{
	const ribi_t::ribi ribi_mark = ribi_type(end-start);
	const koord zv(ribi_mark);
	const bridge_desc_t *desc = bridge_builder_t::get_desc(default_param);
	const char *error;
	sint8 bridge_height;
	koord3d end2 = bridge_builder_t::find_end_pos(player, start, zv, desc, error, bridge_height, false, koord_distance(start, end), is_ctrl_pressed());

	assert(end == end2);

	sint64 costs = 0;
	// start
	grund_t *gr = welt->lookup(start);

	// get initial height of bridge from start tile
	// flat -> height is 1 if conversion factor 1, 2 if conversion factor 2
	// single height -> height is 1
	// double height -> height is 2
	const slope_t::type slope = gr->get_grund_hang();
	uint8 max_height = slope ?  slope_t::max_diff(slope) : bridge_height;

	zeiger_t *way = new zeiger_t(start, player );
	const bridge_desc_t::img_t img0 = desc->get_end( slope, slope, slope_type(zv)*max_height );

	gr->obj_add( way );
	way->set_image( desc->get_background( img0, 0 ) );
	way->set_after_image( desc->get_foreground( img0, 0 ) );

	if(  gr->get_grund_hang() != 0  ) {
		way->set_yoff( -TILE_HEIGHT_STEP * max_height );
	}
	// eventually we have to remove trees on start tile
	if (desc->get_waytype() != powerline_wt) {
		for(  uint8 i=0;  i<gr->get_top();  i++  ) {
			obj_t *obj = gr->obj_bei(i);
			switch(obj->get_typ()) {
				case obj_t::baum:
					costs -= welt->get_settings().cst_remove_tree;
					break;
				case obj_t::groundobj:
					costs += ((groundobj_t *)obj)->get_desc()->get_value();
					break;
				default: break;
			}
		}
	}
	marked.insert( way );
	way->mark_image_dirty( way->get_image(), 0 );
	// loop
	koord3d pos( start + zv + koord3d( 0, 0, max_height ) );
	while (pos.get_2d()!=end.get_2d()) {
		grund_t *gr = welt->lookup( pos );
		if( !gr ) {
			gr = new monorailboden_t(pos, 0);
			gr->set_grund_hang( 0 );
			welt->access(pos.get_2d())->boden_hinzufuegen(gr);
		}
		if (gr->is_water()) {
				continue;
			}
		zeiger_t *way = new zeiger_t(pos, player);
		gr->obj_add( way );
		grund_t *kb = welt->lookup_kartenboden(pos.get_2d());
		sint16 height = pos.z - kb->get_pos().z;
		way->set_image(desc->get_background(desc->get_straight(ribi_mark,height-slope_t::max_diff(kb->get_grund_hang())),0));
		way->set_after_image(desc->get_foreground(desc->get_straight(ribi_mark,height-slope_t::max_diff(kb->get_grund_hang())), 0));
		marked.insert( way );
		way->mark_image_dirty( way->get_image(), 0 );
		pos = pos + zv;
	}
	costs += desc->get_value() * koord_distance(start, pos);
	// end
	gr = welt->lookup(end);

	// get initial height of bridge from start tile
	// flat -> height is 1 if conversion factor 1, 2 if conversion factor 2
	// single height -> height is 1
	// double height -> height is 2
	const slope_t::type end_slope = gr->get_weg_hang();
	const uint8 end_max_height = end_slope ? ((end_slope & 7) ? 1 : 2) : (pos.z-end.z);

	if(  gr->ist_karten_boden()  &&  end.z + end_max_height == start.z + max_height  ) {
		zeiger_t *way = new zeiger_t(end, player );
		const bridge_desc_t::img_t img1 = desc->get_end( end_slope, end_slope, end_slope?0:(pos.z-end.z)*slope_type(-zv) );
		gr->obj_add( way );
		way->set_image(desc->get_background(img1, 0));
		way->set_after_image(desc->get_foreground(img1, 0));
		if(  gr->get_grund_hang() != 0  ) {
			way->set_yoff( -TILE_HEIGHT_STEP * end_max_height );
		}
		marked.insert( way );
		way->mark_image_dirty( way->get_image(), 0 );
		costs += desc->get_value();
	}
	else {
		if (desc->get_waytype() == powerline_wt  ? !gr->find<leitung_t>() : !gr->hat_weg(desc->get_waytype())) {
			const way_desc_t *way_desc = way_builder_t::weg_search(desc->get_waytype(), desc->get_topspeed(), desc->get_max_weight(), welt->get_timeline_year_month(), type_flat, desc->get_wear_capacity());
			costs += way_desc->get_value();
		}
	}
	// eventually we have to remove trees on end tile
	if (desc->get_waytype() != powerline_wt) {
		for(  uint8 i=0;  i<gr->get_top();  i++  ) {
			obj_t *obj = gr->obj_bei(i);
			switch(obj->get_typ()) {
				case obj_t::baum:
					costs -= welt->get_settings().cst_remove_tree;
					break;
				case obj_t::groundobj:
					costs += ((groundobj_t *)obj)->get_desc()->get_value();
					break;
				default: break;
			}
		}
	}
	win_set_static_tooltip( tooltip_with_price("Building costs estimates", costs ) );
}

uint8 tool_build_bridge_t::is_valid_pos(  player_t *player, const koord3d &pos, const char *&error, const koord3d &start )
{
	const bridge_desc_t *desc = bridge_builder_t::get_desc(default_param);
	const waytype_t wt = desc->get_waytype();

	error = NULL;
	grund_t *gr = welt->lookup(pos);
	if(  gr==NULL  ||  !slope_t::is_way(gr->get_grund_hang())  ||  !bridge_builder_t::can_place_ramp( player, gr, wt, (is_first_click() ? 0 : ribi_type(pos-start)) )  ) {
		return 0;
	}

	if(  is_first_click()  ) {
		if(  gr->ist_karten_boden()  ) {
		// first click
		ribi_t::ribi rw = ribi_t::none;
		if (wt==powerline_wt) {
			if (gr->hat_wege()) {
				return 0;
			}
			if (gr->find<leitung_t>()) {
				rw |= gr->find<leitung_t>()->get_ribi();
			}
		}
		else {
			if (gr->find<leitung_t>()) {
				return 0;
			}
			if(wt!=road_wt) {
			// only road bridges can have other ways on it (ie trams)
				if(gr->has_two_ways()  ||  (gr->hat_wege() && gr->get_weg_nr(0)->get_waytype()!=wt) ) {
					return 0;
				}
				if(gr->hat_wege()){
					rw |= gr->get_weg_nr(0)->get_ribi_unmasked();
				}
			}
			else {
				// If road and tram, we have to check both ribis.
				for(int i=0;i<2;i++) {
					const weg_t *w = gr->get_weg_nr(i);
					if (w) {
						if (w->get_waytype()!=road_wt  && !w->get_desc()->is_tram()) {
							return 0;
						}
						rw |= w->get_ribi_unmasked();
					}
					else break;
				}
			}
		}
		// ribi from slope
		rw |= ribi_type(gr->get_grund_hang());
		if(  rw!=ribi_t::none && !ribi_t::is_single(rw)  ) {
			return 0;
		}
		// determine possible directions
		ribi = ribi_t::backward(rw);
		return (ribi!=ribi_t::none ? 2 : 0) | (ribi_t::is_single(ribi) ? 1 : 0);
		} else {
			if(  gr->get_weg_hang()  ) {
				return 0;
			}

			if(  gr->get_typ() != grund_t::monorailboden  &&
			     gr->get_typ() != grund_t::tunnelboden  ) {
				return 0;
			}

			return 2;
		}
	}
	else {
		// second click

		// dragging in the right direction?
		ribi_t::ribi test = ribi_type(pos - start);
		if (!ribi_t::is_single(test)  ||  ((test & (~ribi))!=0) ) {
			return 0;
		}

		// check whether we can build a bridge here
		const char *error = NULL;
		sint8 bridge_height;
 		koord3d end = bridge_builder_t::find_end_pos(player, start, koord(test), desc, error, bridge_height, false, koord_distance(start, pos), is_ctrl_pressed());
		if (end!=pos) {
			return 0;
		}
		return 2;
	}
}


/* more difficult, since this builds also underground ways */
tool_build_tunnel_t::tool_build_tunnel_t() : two_click_tool_t(TOOL_BUILD_TUNNEL | GENERAL_TOOL)
{
	if (!env_t::networkmode)
	{
		way_desc = default_param ? way_builder_t::get_desc(default_param, 0) : tool_build_way_t::defaults[get_waytype() & 63];
	}
  overtaking_mode = twoway_mode;
}

const char* tool_build_tunnel_t::get_tooltip(const player_t *) const
{
	const tunnel_desc_t * desc = tunnel_builder_t::get_desc(default_param);
	tooltip_with_price_maintenance( welt, desc->get_name(), -desc->get_base_price(), desc->get_base_maintenance());
	strcat(toolstr, " / km");
	size_t n= strlen(toolstr);

	if(desc->get_waytype()!=powerline_wt) {
				n += sprintf(toolstr+n, ", %dkm/h, %dt",
			desc->get_topspeed(),
			desc->get_max_axle_load());
	}
	bool any_prohibitive = false;
	for(sint8 i = 0; i < desc->get_way_constraints().get_count(); i ++)
	{
		if(desc->get_way_constraints().get_prohibitive(i))
		{
			if(!any_prohibitive)
			{
				n += sprintf(toolstr + n, " (" );
				n += sprintf(toolstr + n, translator::translate("Restrictions:"));
			}
			any_prohibitive = true;
			char tmpbuf[30];
			sprintf(tmpbuf, "Prohibitive %i-%i", desc->get_wtyp(), i);
			n += sprintf(toolstr + n, " ");
			n += sprintf(toolstr + n, translator::translate(tmpbuf));
		}
	}
	n = strlen(toolstr);
	if(any_prohibitive)
	{
		strcat(toolstr, ")");
	}
	return toolstr;
}

waytype_t tool_build_tunnel_t::get_waytype() const
{
	const tunnel_desc_t * desc = tunnel_builder_t::get_desc(default_param);
	return desc ? desc->get_waytype() : invalid_wt;
}

bool tool_build_tunnel_t::init( player_t *player )
{
	two_click_tool_t::init( player );
	// now get current desc
	const tunnel_desc_t * desc = tunnel_builder_t::get_desc(default_param);
	if(  desc  &&  !desc->is_available(welt->get_timeline_year_month())  &&  player!=NULL  &&  player!=welt->get_public_player()) {
		return false;
	}
	if (is_ctrl_pressed()  &&  can_use_gui()  &&  desc->get_waytype()==road_wt  ) {
		create_win(new overtaking_mode_frame_t(player, this), w_info, (ptrdiff_t)this);
	}
	return desc!=NULL;
}

void tool_build_tunnel_t::draw_after(scr_coord k, bool dirty) const
{
	const tunnel_desc_t *desc = tunnel_builder_t::get_desc(default_param);
	if(  desc  &&  desc->get_waytype()==road_wt  ) {
		if(  icon!=IMG_EMPTY  &&  is_selected()  ) {
			display_img_blend( icon, k.x, k.y, TRANSPARENT50_FLAG|OUTLINE_FLAG|color_idx_to_rgb(COL_BLACK), false, dirty );
			char level_str[16];
			tool_build_way_t::set_mode_str(level_str, overtaking_mode);
			display_proportional_rgb( k.x+4, k.y+4, level_str, ALIGN_LEFT, color_idx_to_rgb(COL_YELLOW), true );
		}
	} else {
		two_click_tool_t::draw_after(k,dirty);
	}
}

bool tool_build_tunnel_t::exit( player_t *player )
{
	destroy_win((ptrdiff_t)this);
	return two_click_tool_t::exit(player);
}

void tool_build_tunnel_t::rdwr_custom_data(memory_rw_t *packet)
{
	two_click_tool_t::rdwr_custom_data(packet);
	sint8 i = overtaking_mode;
	packet->rdwr_byte(i);
	overtaking_mode = (overtaking_mode_t)i;
	if (packet->is_loading())
	{
		plainstring way_desc_string;
		packet->rdwr_str(way_desc_string);
		way_desc = way_builder_t::get_desc(way_desc_string);
	}
	else
	{
		const tunnel_desc_t* tb = tunnel_builder_t::get_desc(default_param);
		const waytype_t wt = tb ? tb->get_waytype() : invalid_wt;
		way_desc = tool_build_way_t::defaults[wt & 63];

		plainstring way_desc_string = way_desc ? way_desc->get_name() : "none";
		packet->rdwr_str(way_desc_string);
	}
}

const char *tool_build_tunnel_t::check_pos( player_t *player, koord3d pos)
{
	if (grund_t::underground_mode == grund_t::ugm_all) {
		return NULL;
	}
	else {
		return two_click_tool_t::check_pos(player, pos);
	}
}

void tool_build_tunnel_t::calc_route( way_builder_t &bauigel, const koord3d &start, const koord3d &end)
{
	const tunnel_desc_t *desc = tunnel_builder_t::get_desc(default_param);
	way_builder_t::bautyp_t bt = (way_builder_t::bautyp_t)(desc->get_waytype());

	const way_desc_t *wb = desc->get_way_desc();
	if(wb==NULL) {
		// ignore timeline to get consistent results
		wb = way_builder_t::weg_search(desc->get_waytype(), desc->get_topspeed(), desc->get_max_axle_load(), 0, type_flat, desc->get_wear_capacity());
	}

	bauigel.init_builder(bt | way_builder_t::tunnel_flag, wb, desc);
	bauigel.set_keep_existing_faster_ways( !is_ctrl_pressed() );
	// wegbauer (way builder) tries to find route to 3d coordinate if no ground at end exists or is not kartenboden (map ground)
	bauigel.calc_straight_route(start,end);
}

const char *tool_build_tunnel_t::do_work( player_t *player, const koord3d &start, const koord3d &end )
{
	if( end == koord3d::invalid ) {
		// Build tunnel mouths
		if (welt->lookup_kartenboden(start.get_2d())->get_hoehe() == start.z) {
			const tunnel_desc_t *desc = tunnel_builder_t::get_desc(default_param);
			return tunnel_builder_t::build( player, start.get_2d(), desc, !is_ctrl_pressed(), overtaking_mode, way_desc );
		}
		else {
			return "";
		}
	}
	else {
		// Build tunnels
		way_builder_t bauigel(player);
		calc_route( bauigel, start, end );
		welt->mute_sound(true);
		bauigel.set_overtaking_mode(overtaking_mode);
		bauigel.set_desc(way_desc);
		bauigel.build();
		welt->mute_sound(false);
		welt->lookup_kartenboden(end.get_2d())->clear_flag(grund_t::marked);
		return NULL;
	}
}

uint8 tool_build_tunnel_t::is_valid_pos(  player_t *player, const koord3d &pos, const char *&error, const koord3d & )
{
	if(  !is_first_click()  ) {
		error = NULL;
		// All pos are valid for the second click!
		return 2;
	}
	// search for ground
	// start needs valid tile!
	grund_t *gr = welt->lookup(pos);
	if(  gr  ) {
		if( gr->hat_wege() ) {
			const tunnel_desc_t *desc = tunnel_builder_t::get_desc(default_param);
			// use the check_owner routine of way_builder_t (not player_t!), needs an instance
			weg_t *w = gr->get_weg_nr(0);
			if(  w==NULL  ||  w->get_desc()->get_wtyp()!=desc->get_waytype()  ) {
				error = NOTICE_UNSUITABLE_GROUND;
				return 0;
			}
			way_builder_t bauigel(player);
			if(!bauigel.check_owner( w->get_owner(), player )) {
				error = "Das Feld gehoert\neinem anderen Spieler\n";
				return 0;
			}
		}
	}
	else {
		error = NOTICE_UNSUITABLE_GROUND;
		return 0;
	}
	// if starting tile is tunnel .. build underground tracks
	error = NULL;
	if(gr->ist_tunnel()) {
		return 2;
	}
	// .. otherwise build tunnel mouths (and tunnel behind)
	else {
		return 1;
	}
}

void tool_build_tunnel_t::mark_tiles(  player_t *player, const koord3d &start, const koord3d &end )
{
	way_builder_t bauigel(player);
	calc_route( bauigel, start, end );

	const tunnel_desc_t *desc = tunnel_builder_t::get_desc(default_param);
	// now we search a matching way for the tunnels top speed
	const way_desc_t *wb = desc->get_way_desc();
	if(wb==NULL) {
		// ignore timeline to get consistent results
		wb = way_builder_t::weg_search(desc->get_waytype(), desc->get_max_axle_load(), desc->get_topspeed(), 0, type_flat, desc->get_wear_capacity());
	}

	welt->lookup_kartenboden(end.get_2d())->clear_flag(grund_t::marked);

	if(  bauigel.get_count()>1  ) {
		// Set tooltip first (no dummygrounds, if bauigel.calc_casts() is called).
		win_set_static_tooltip( tooltip_with_price("Building costs estimates", -bauigel.calc_costs() ) );

		// make dummy route from bauigel
		for(  uint32 j=0;  j<bauigel.get_count();  j++  ) {
			koord3d pos = bauigel.get_route()[j];
			grund_t *gr = welt->lookup(pos);
			if( !gr ) {
				// We need to create a dummy ground.
				gr = new tunnelboden_t(pos, 0);
				welt->access(pos.get_2d())->boden_hinzufuegen(gr);
			}
			ribi_t::ribi zeige = gr->get_weg_ribi_unmasked(wb->get_wtyp()) | bauigel.get_route().get_ribi( j );

			zeiger_t *way = new zeiger_t(pos, player );
			if(gr->get_weg_hang()) {
				way->set_image( wb->get_slope_image_id(gr->get_weg_hang(),0) );
			}
			else if(wb->get_wtyp()!=powerline_wt  &&  ribi_t::is_bend(zeige)  &&  wb->has_diagonal_image()) {
				way->set_image( wb->get_diagonal_image_id(zeige,0) );
			}
			else {
				way->set_image( wb->get_image_id(zeige,0) );
			}
			gr->obj_add( way );
			marked.insert( way );
			way->mark_image_dirty( way->get_image(), 0 );
		}
		welt->lookup(end)->set_flag(grund_t::marked);
	}
}


/* removes a way like a driving car ... */
char const* tool_wayremover_t::get_tooltip(player_t const*) const
{
	switch(atoi(default_param)) {
		case road_wt: return translator::translate("remove roads");
		case tram_wt:
		case track_wt: return translator::translate("remove tracks");
		case maglev_wt: return translator::translate("remove maglev tracks");
		case narrowgauge_wt: return translator::translate("remove narrowgauge tracks");
		case monorail_wt: return translator::translate("remove monorails");
		case water_wt: return translator::translate("remove channels");
		case air_wt: return translator::translate("remove airstrips");
		case powerline_wt: return translator::translate("remove powerlines");
	}
	return NULL;
}

image_id tool_wayremover_t::get_icon(player_t *) const
{
	if(  default_param  &&  way_builder_t::waytype_available( (waytype_t)atoi(default_param), welt->get_timeline_year_month() )  ) {
		return icon;
	}
	return IMG_EMPTY;
}

waytype_t tool_wayremover_t::get_waytype() const
{
	return default_param ? (waytype_t)atoi(default_param) : invalid_wt;
}

class electron_t : public test_driver_t {
	bool check_next_tile(const grund_t* gr) const { return gr->get_leitung()!=NULL; }
	virtual ribi_t::ribi get_ribi(const grund_t* gr) const { return gr->get_leitung()->get_ribi(); }
	virtual waytype_t get_waytype() const { return invalid_wt; }
	virtual int get_cost(const grund_t *, const sint32, koord) { return 1; }
	virtual bool  is_target(const grund_t *,const grund_t *) { return false; }
};

class scenario_checker_t : public test_driver_t {
public:
	test_driver_t *other;
	scenario_t *scenario;
	uint16 id;
	player_t *player;
	~scenario_checker_t() { delete other; }

	/**
	 * checks for active scenario,
	 * @returns scenario_checker_t if scenario active, the supplied test_driver otherwise
	 */
	static test_driver_t* apply(test_driver_t *test_driver, player_t *player, tool_t *tool) {
		karte_t *welt = world();
		if (is_scenario()) {
			scenario_checker_t *td2 = new scenario_checker_t();
			td2->other = test_driver;
			td2->scenario = welt->get_scenario();
			td2->id = tool->get_id();
			td2->player = player;
			return td2;
		}
		return test_driver;
	}
private:
	bool check_next_tile(const grund_t* gr) const { return other->check_next_tile(gr)  &&  scenario->is_work_allowed_here(player, id, other->get_waytype(), gr->get_pos())==NULL;}
	virtual ribi_t::ribi get_ribi(const grund_t* gr) const { return other->get_ribi(gr); }
	virtual waytype_t get_waytype() const { return other->get_waytype(); }
	virtual int get_cost(const grund_t *gr, const sint32 c, koord p) { return other->get_cost(gr,c,p); }
	virtual bool  is_target(const grund_t *gr,const grund_t *gr2) { return other-> is_target(gr,gr2); }
};

void tool_wayremover_t::mark_tiles( player_t *player, const koord3d &start, const koord3d &end )
{
	route_t verbindung;
	bool can_built = calc_route( verbindung, player, start, end );
	if( can_built ) {
		FOR(vector_tpl<koord3d>, const& pos, verbindung.get_route()) {
			zeiger_t *marker = new zeiger_t(pos, NULL );
			marker->set_image( cursor );
			marker->mark_image_dirty( marker->get_image(), 0 );
			marked.insert( marker );
			welt->lookup(pos)->obj_add( marker );
		}
	}
}

uint8 tool_wayremover_t::is_valid_pos( player_t *player, const koord3d &pos, const char *&error, const koord3d & )
{
	// search for starting ground
	waytype_t wt = get_waytype();
	grund_t *gr=tool_intern_koord_to_weg_grund(player,welt,pos,wt);
	if(gr==NULL) {
		DBG_MESSAGE("tool_wayremover()", "no ground on %i,%i",pos.x, pos.y);
		// wrong ground or not this way here => exit
		return 0;
	}
	// do not remove ground from depot
	if(gr->get_depot() || gr->get_signalbox()) {
		error = NOTICE_UNSUITABLE_GROUND;
		return 0;
	}
	if(is_scenario()) {
		error = welt->get_scenario()->is_work_allowed_here(player, get_id(), wt, pos);
		if (error) {
			dbg->warning("tool_wayremover_t::is_within_limits()", error);
			return 0;
		}
	}
	error = NULL;
	return 2;
}

bool tool_wayremover_t::calc_route( route_t &verbindung, player_t *player, const koord3d &start, const koord3d &end )
{
	waytype_t wt = get_waytype();
	if (wt == tram_wt) {
		wt = track_wt;
	}

	if(  start == end  ) {
		verbindung.clear();
		grund_t *gr=welt->lookup(start);
		if(  gr  &&  (wt!=powerline_wt ? gr->get_weg(wt)!=NULL : gr->get_leitung()!=NULL) ) {
			verbindung.append( start );
		}
	}
	else {
		// get a default vehicle
		test_driver_t* test_driver;
		if(  wt!=powerline_wt  ) {
			vehicle_desc_t remover_desc(wt, 500, vehicle_desc_t::diesel );
			vehicle_t *driver = vehicle_builder_t::build(start, player, NULL, &remover_desc);
			driver->set_flag( obj_t::not_on_map );
			test_driver = driver;
		}
		else {
			test_driver = new electron_t();
		}
		test_driver = scenario_checker_t::apply(test_driver, player, this);

		verbindung.calc_route(welt, start, end, test_driver, 0, 0, false, 0);
		delete test_driver;
	}
	DBG_MESSAGE("tool_wayremover()","route with %d tile found",verbindung.get_count());

	calc_route_error = NULL;
	bool can_delete = start == end  ||  verbindung.get_count()>1;
	if(  can_delete  ) {
		// found a route => check if I can delete anything on it
		FOR(koord3d_vector_t, const& i, verbindung.get_route()) {
			if (!can_delete) break;
			grund_t const* const gr = welt->lookup(i);
			if(  wt!=powerline_wt  ) {
				// no way found
				if(  gr==NULL  ||  gr->get_weg(wt)==NULL  ) {
					can_delete = false;
					break;
				}
				// check all if we want to delete the first on a no-ground tile
				bool check_all = !gr->ist_karten_boden()  &&  gr->has_two_ways()  &&  gr->get_weg_nr(0)->get_waytype()==wt;
				// we have to do a fine check
				for( uint i=0;  i<gr->get_top()  &&  can_delete;  i++  ) {
					obj_t *obj = gr->obj_bei(i);
					const uint8 type = obj->get_typ();
					// ignore pillars, powerlines
					if (type == obj_t::pillar  ||  type==obj_t::leitung) {
						continue;
					}
					// ignore flying aircraft
					if (type == obj_t::air_vehicle  &&  !(static_cast<air_vehicle_t*>(obj)->is_on_ground())) {
						continue;
					}
					const waytype_t obj_wt = obj->get_waytype();
					// way-related things
					if (obj_wt != invalid_wt) {
						// check this thing if it has the same waytype or if we want to remove the whole bridge/tunnel tile
						// special case: stations - take care not to produce station without any way
						const bool lonely_station = type==obj_t::gebaeude  &&  !gr->has_two_ways();
						if (check_all ||  obj_wt == wt  ||  lonely_station) {
							can_delete = (calc_route_error = obj-> is_deletable(player)) == NULL;
						}
					}
					// all other stuff
					// Ignore crossings: look only to the underlying way.
					else if (!obj->get_typ() == obj_t::crossing)
					{
						can_delete = (calc_route_error = obj->is_deletable(player)) == NULL;
					}
				}
			}
			else {
				// for powerline: only a ground and a powerline to remove
				if(  gr==NULL  ||  gr->get_leitung()==NULL  ||  (calc_route_error = gr->get_leitung()-> is_deletable(player))!=NULL  ) {
					can_delete = false;
					break;
				}
			}
		}
	}
	DBG_MESSAGE("tool_wayremover()", "route search returned %d", can_delete);

	return can_delete;
}

const char *tool_wayremover_t::do_work( player_t *player, const koord3d &start, const koord3d &end )
{
	waytype_t wt = get_waytype();
	route_t verbindung;
	if( !calc_route( verbindung, player, start, end )  ) {
		DBG_MESSAGE("tool_wayremover()","no route found");
		if (calc_route_error  &&  *calc_route_error) {
			return calc_route_error;
		}
		else {
			return "Ways not connected";
		}
	}

	bool can_delete = true;	// assume success

	// if successful => delete everything
	for( uint32 i=0;  i<verbindung.get_count();  i++  )
	{
		grund_t *gr = welt->lookup(verbindung.at(i));

		if(is_shift_pressed())
		{
			// Do not remove the way: just set it not to be renewed.
			weg_t* const way = gr->get_weg(wt);
			if(way)
			{
				way->set_replacement_way(NULL);
			}
		}
		else
		{
			// ground can be missing after deleting a bridge ...
			if(gr  &&  !gr->is_water())
			{
				if(gr->ist_bruecke())
				{
					bruecke_t* bridge = (gr->find<bruecke_t>());
					if(bridge->get_desc()->get_waytype()==wt)
					{
						if(gr->ist_karten_boden())
						{
							weg_t* bridge_way = gr->get_weg(wt);

							if (!player->is_public_service() && bridge_way && bridge_way->is_public_right_of_way())
							{
								if (gr->removing_way_would_disrupt_public_right_of_way(bridge_way->get_waytype()))
								{
									return "Cannot remove a public right of way without providing an adequate diversionary route";
								}
							}

							const char *err = NULL;
							err = bridge_builder_t::remove(player,verbindung.at(i),wt);
							if(err)
							{
								return err;
							}
							gr = welt->lookup(verbindung.at(i));
						}
						else
						{
							// do not remove asphalt from a bridge ...
							continue;
						}
					}
				}

				if(wt == road_wt)
				{
					const koord pos = gr->get_pos().get_2d();
					stadt_t *city = welt->get_city(pos);

					if(city && !player->is_public_service()) {
						if (gr->removing_road_would_disconnect_city_building()) {
							return "Cannot delete a road where to do so would leave a city building unconnected by road.";
						}
					}
					welt->set_recheck_road_connexions();
				}

				// now the tricky part: delete just part of a way (or everything, if possible)
				// calculate remaining directions
				ribi_t::ribi rem = 15 ^ ( verbindung.get_route().get_ribi(i) );
				// if start=end tile then delete every direction
				if(  verbindung.get_count() <= 1  ) {
					rem = 0;
				}

				if(  wt!=powerline_wt  ) {
					if(!gr->get_flag(grund_t::is_kartenboden)  &&  (gr->get_typ()==grund_t::tunnelboden  ||  gr->get_typ()==grund_t::monorailboden)  &&  gr->get_weg_nr(0)->get_waytype()==wt) {
						can_delete &= gr->remove_everything_from_way(player,wt,rem);
						if(can_delete  &&  gr->get_weg(wt)==NULL) {
							if(gr->get_weg_nr(0)!=0) {
								gr->remove_everything_from_way(player,gr->get_weg_nr(0)->get_waytype(),ribi_t::none);
							}
							gr->obj_loesche_alle(player);
							gr->mark_image_dirty();
							if (gr->is_visible() && gr->get_typ()==grund_t::tunnelboden && i>0) { // visibility test does not influence execution
								grund_t *bd = welt->access(verbindung.at(i-1).get_2d())->get_kartenboden();
								bd->calc_image();
								bd->set_flag(grund_t::dirty);
							}
							// delete tunnel ground too, if empty
							welt->access(gr->get_pos().get_2d())->boden_entfernen(gr);
							delete gr;
						}
					}
					else
					{
						// If this is a public right of way being deleted by anyone other than the public service player,
						// then it cannot be deleted unless there is a diversionary route within a specified number of tiles.
						weg_t* weg = gr->get_weg(wt);
						if(!player->is_public_service() && weg && weg->is_public_right_of_way())
						{
							if(gr->removing_way_would_disrupt_public_right_of_way(weg->get_waytype()))
							{
								return "Cannot remove a public right of way without providing an adequate diversionary route";
							}
						}

						can_delete &= gr->remove_everything_from_way(player,wt,rem);

						if(weg)
						{
							weg->count_sign();
						}
					}
				}
				else {
					leitung_t *lt = gr->get_leitung();
					if(  lt  &&  (rem&lt->get_ribi())==0  ) {
						// remove only single connections
						lt->cleanup(player);
						delete lt;
						// delete tunnel ground too, if empty
						if (gr->get_typ()==grund_t::tunnelboden) {
							gr->obj_loesche_alle(player);
							gr->mark_image_dirty();
							if (!gr->get_flag(grund_t::is_kartenboden)) {
								welt->access(gr->get_pos().get_2d())->boden_entfernen(gr);
								delete gr;
							}
							else {
								grund_t *gr_new = new boden_t(gr->get_pos(), gr->get_grund_hang());
								welt->access(gr->get_pos().get_2d())->boden_ersetzen(gr, gr_new);
								gr_new->calc_image();
							}
						}
					}
					// otherwise it is a crossing ...
				}
			}
			// ok, now everything removed ...
		}
	}

	// return success
	return can_delete ? NULL : "";
}



/* add catenary during construction */
const way_obj_desc_t *tool_build_wayobj_t::default_electric = NULL;

const char* tool_build_wayobj_t::get_tooltip(const player_t *) const
{
	if(  build  ) {
		const way_obj_desc_t *desc = get_desc();
		if(desc) {
			tooltip_with_price_maintenance( welt, desc->get_name(), -desc->get_base_price(),  desc->get_base_maintenance());
			strcat(toolstr, " / km");
			size_t n = strlen(toolstr);
			int topspeed = desc->get_topspeed();
			if (desc->is_overhead_line()) {
				// only overheadlines impose topspeed
				sprintf(toolstr+n, ", %dkm/h", topspeed);
			}
			bool any_prohibitive = false;
			for(sint8 i = 0; i < desc->get_way_constraints().get_count(); i ++)
			{
				if(desc->get_way_constraints().get_prohibitive(i))
				{
					if(!any_prohibitive)
					{
						n += sprintf(toolstr + n, " (" );
						n += sprintf(toolstr + n, translator::translate("Restrictions:"));
					}
					any_prohibitive = true;
					char tmpbuf[30];
					sprintf(tmpbuf, "Prohibitive %i-%i", desc->get_waytype(), i);
					n += sprintf(toolstr + n, " ");
					n += sprintf(toolstr + n, translator::translate(tmpbuf));
				}
			}
			n = strlen(toolstr);
			if(any_prohibitive)
			{
				strcat(toolstr, ")");
			}
			return toolstr;
		}
		return NULL;
	}
	else {
		waytype_t wt = (waytype_t)atoi( default_param );
		sprintf( toolstr, translator::translate("Remove wayobj %s"), translator::translate(weg_t::waytype_to_string(wt)) );
		return toolstr;
	}
}

const way_obj_desc_t *tool_build_wayobj_t::get_desc() const
{
	const way_obj_desc_t *desc = default_param ? wayobj_t::find_desc(default_param) : NULL;
	if(desc==NULL) {
		desc = default_electric;
		if(desc==NULL) {
			desc = wayobj_t::get_overhead_line( track_wt, welt->get_timeline_year_month() );
		}
	}
	return desc;
}

waytype_t tool_build_wayobj_t::get_waytype() const
{
	if(  build  ) {
		const way_obj_desc_t *desc = get_desc();
		return desc ? desc->get_wtyp() : invalid_wt;
	}
	else {
		return default_param ? (waytype_t)atoi( default_param ) : invalid_wt;
	}
}

bool tool_build_wayobj_t::is_selected() const
{
	const tool_build_wayobj_t *selected = dynamic_cast<const tool_build_wayobj_t *>(welt->get_tool(welt->get_active_player_nr()));
	return (selected  &&  selected->build==build  &&  selected->get_desc() == get_desc());
}

bool tool_build_wayobj_t::init( player_t *player )
{
	two_click_tool_t::init( player );

	if( build ) {
		desc = get_desc();
		if(desc==NULL) {
			desc = default_electric;
			if(desc==NULL) {
				desc = default_electric = wayobj_t::get_overhead_line( track_wt, welt->get_timeline_year_month() );
			}
		}
		else {
			default_electric = desc;
		}

		if( desc ) {
			cursor = desc->get_cursor()->get_image_id(0);
			wt = desc->get_wtyp();
			default_electric = desc;
		}
		return desc!=NULL;
	}
	else {
		desc = NULL;
		wt = (waytype_t)atoi( default_param );
		return wt != 0;
	}
}

bool tool_build_wayobj_t::calc_route( route_t &verbindung, player_t *player, const koord3d& start, const koord3d& to )
{
	waytype_t waytype = wt;
	if (waytype == any_wt) {
		waytype = welt->lookup(start)->get_weg(wt)->get_waytype();
	}
	// get a default vehicle
	vehicle_desc_t remover_desc(waytype, 500, vehicle_desc_t::diesel );
	vehicle_t* test_vehicle = vehicle_builder_t::build(start, player, NULL, &remover_desc);
	test_vehicle->set_flag( obj_t::not_on_map );
	test_driver_t* test_driver = scenario_checker_t::apply(test_vehicle, player, this);

	bool can_built;
	if( start != to ) {
		can_built = verbindung.calc_route(welt, start, to, test_driver, 0, 0, false, 0);
	}
	else {
		verbindung.clear();
		verbindung.append( start );
		can_built = true;
	}
	delete test_driver;
	return can_built;
}

uint8 tool_build_wayobj_t::is_valid_pos( player_t * player, const koord3d& pos, const char *&error, const koord3d & )
{
	// search for starting ground
	grund_t *gr=tool_intern_koord_to_weg_grund(player, welt, pos, wt );
	if(  gr == NULL  ) {
		DBG_MESSAGE("tool_build_wayobj_t::is_within_limits()", "no ground on %s",pos.get_str());
		// wrong ground or not this way here => exit
		return 0;
	}
	error = NULL;
	return 2;
}

void tool_build_wayobj_t::mark_tiles( player_t * player, const koord3d &start, const koord3d &end )
{
	route_t verbindung;
	bool can_built = calc_route( verbindung, player, start, end );
	if( can_built ) {
		sint32 cost_estimate = 0;

		for( uint32 j = 0; j < verbindung.get_count(); j++ ) {
			koord3d pos = verbindung.at(j);
			grund_t *gr = welt->lookup(pos);

			ribi_t::ribi show = verbindung.get_route().get_ribi(j);
			// Search a matching catenary on gr.
			wayobj_t *wayobj = gr->get_wayobj( wt );
			if( build ) {
					cost_estimate += desc->get_value();
				if( wayobj ) {
					show = show | wayobj->get_dir();
					// Already a catenary here -> costs only, if new catenary is faster
					if(  wayobj->get_desc()->get_topspeed() >= desc->get_topspeed()  ) {
						cost_estimate -= desc->get_value();
					}
				}
			}
			else if( wayobj ) {
				cost_estimate += wayobj->get_desc()->get_value();
			}
			zeiger_t *way_obj = NULL;
			if( build ) {
				way_obj = new zeiger_t(pos, player );
				if(  gr->get_weg_hang()  ) {
					way_obj->set_after_image( desc->get_front_slope_image_id(gr->get_weg_hang()) );
					way_obj->set_image( desc->get_back_slope_image_id(gr->get_weg_hang()) );
				}
				else if(  ribi_t::is_bend(show)  &&  desc->has_diagonal_image()  ) {
					way_obj->set_after_image( desc->get_front_diagonal_image_id(show) );
					way_obj->set_image( desc->get_back_diagonal_image_id(show) );
				}
				else {
					way_obj->set_after_image( desc->get_front_image_id(show) );
					way_obj->set_image( desc->get_back_image_id(show) );
				}
			}
			else {
				if( gr->get_wayobj( wt ) ) {
					way_obj = new zeiger_t(pos, player );
					way_obj->set_image( cursor ); //skinverwaltung_t::bauigelsymbol->get_image_id(0));
				}
			}
			if( way_obj ) {
				way_obj->mark_image_dirty( way_obj->get_image(), 0 );
				gr->obj_add( way_obj );
				marked.insert( way_obj );
			}
		}
		win_set_static_tooltip( tooltip_with_price("Building costs estimates", -cost_estimate ) );
	}
}

const char *tool_build_wayobj_t::do_work( player_t * player, const koord3d &start, const koord3d &end )
{
	route_t verbindung;
	bool can_built = calc_route( verbindung, player, start, end );
	DBG_MESSAGE("tool_build_wayobj_t::work()","route search returned %d",can_built);

	const char *err = NULL;

	if(!can_built) {
		return "Ways not connected";
	}

	// built wayobj ...
	koord3d_vector_t const& r = verbindung.get_route();
	for (uint32 i = 0; i<verbindung.get_count(); i++) {
		if (build) {
			wayobj_t::extend_wayobj_t(r[i], player, r.get_ribi(i), desc);
		}
		else {
			grund_t *gr = welt->lookup(r[i]);
			for (int n = 0; n<gr->get_top(); n++) {
				obj_t *obj = gr->obj_bei(n);
				if (obj  &&  obj->get_typ() == obj_t::wayobj) {
					wayobj_t *wo = static_cast<wayobj_t *>(obj);
					if (wo->get_waytype() == wt) {
						// only remove matching waytype
						const char *err = wo->is_deletable(player);
						if (!err) {
							wo->cleanup(player);
							delete wo;
						}
						else {
							break;
						}
					}
				}
			}
		}
	}

	// Update depots (maybe remove electric tab?). Depots can only be on first and last tile.
	if (depot_t *dep = welt->lookup(r[0])->get_depot()) {
		dep->update_win();
	}
	if (depot_t *dep = welt->lookup(r.back())->get_depot()) {
		dep->update_win();
	}

	return err;;
}


/* build all kind of station extension buildings */
const char *tool_build_station_t::tool_station_building_aux(player_t *player, bool extend_public_halt, koord3d pos, const building_desc_t *desc, sint8 rotation )
{
	const koord& k = pos.get_2d();

	// need kartenboden (map ground)
	if((welt->lookup_kartenboden(k)->get_hoehe() != pos.z) && desc->get_allow_underground() == 0)
	{
		return "";
	}
DBG_MESSAGE("tool_station_building_aux()", "building post office/station building on square %d,%d", k.x, k.y);

	// Player player pays for the construction
	// but we try to extend stations of Player new_owner that may be the public player
	player_t *new_owner = extend_public_halt ? welt->get_public_player() : player;

	koord offsets;
	halthandle_t halt;
	const char *msg = NOTICE_TILE_FULL;

	if(  rotation==-1  ) {
		//no predefined rotation

		int best_halt = 0;
		int any_halt = 0;

		// find valid rotations (since halt extensions are symmetric, we need to check only two)
		bool any_ok = false;
		for( int r=0;  r<2;  r++  ) {
			koord testsize = desc->get_size(r);
			for(  sint8 j=3;  j>=0;  j-- ) {
				bool ok = true;
				koord offset(((j&1)^1)*(testsize.x-1),((j>>1)&1)*(testsize.y-1));
				if(welt->square_is_free(k-offset, testsize.x, testsize.y, NULL, desc->get_allowed_climate_bits())) {
					// first we must check over/under halt
					halthandle_t last_halt;
					for(  sint16 x=0;  x<testsize.x;  x++  ) {
						for(  sint16 y=0;  y<testsize.y;  y++  ) {
							const planquadrat_t *pl = welt->access( k-offset+koord(x,y) );
							if (pl) {
								for(  uint8 i=0;  i < pl->get_boden_count();  i++  ) {
									halthandle_t test_halt = pl->get_boden_bei(i)->get_halt();
									if(test_halt.is_bound()) {
										if(!player_t::check_owner( new_owner, test_halt->get_owner())) {
											// there is another player's halt
											ok = false;
											msg = "Das Feld gehoert\neinem anderen Spieler\n";
										}
										else if(!last_halt.is_bound()) {
											last_halt = test_halt;
										}
										else if(last_halt != test_halt) {
											// there are several halts
											ok = false;
											msg = "Several halts found.";
										}
									}
								}
							}
						}
					}
					if(!ok) {
						continue;
					}
					// well, at least this is theoretical possible here
					any_ok = true;
					if(rotation==-1) {
						// we can build it. reserve this one
						// This needs to build a building at under/over a halt.
						rotation = r;
						offsets = offset;
					}
					koord test_start = k-offset;

					// find all surrounding tiles with a stop
					// for following section of code arrays are arranged such that
					// 0 - facing north
					// 1 - facing west
					// 2 - facing south
					// 3 - facing east
					int neighbour_halts[4] = { 0, 0, 0, 0 };
					int best_halts[4] = { 0, 0, 0, 0 };

					// test also diagonal corners (that is why from -1 to size!)
					for(  sint16 y=-1;  y<=testsize.y;  y++  ) {
						for(  sint16 x=-1;  x<=testsize.x;  (x==-1 && y>-1 && y<testsize.y)?x=testsize.x:x++  ) {
							const planquadrat_t *pl = welt->access( test_start+koord(x,y) );
							if(  pl  ) {
								for(  uint b=0;  b < pl->get_boden_count();  b++  ) {
									grund_t *gr = pl->get_boden_bei(b);
									if(  gr->is_halt()  &&  gr->get_halt().is_bound() &&  new_owner == gr->get_halt()->get_owner()  ) {
										halt = gr->get_halt();
										gebaeude_t *gb = gr->find<gebaeude_t>();
										bool best = gr->hat_wege()  &&  gb  &&  gb->get_tile()->get_desc()->get_extra()==desc->get_extra();

										// north
										if(  y==-1  ) {
											neighbour_halts[0] ++;
											if(  best  ) {
												best_halts[0] ++;
											}
										}

										// west
										if(  x==-1  ) {
											neighbour_halts[1] ++;
											if(  best  ) {
												best_halts[1] ++;
											}
										}

										// south
										if(  y==testsize.y  ) {
											neighbour_halts[2] ++;
											if(  best  ) {
												best_halts[2] ++;
											}
										}

										// east
										if(  x==testsize.x  ) {
											neighbour_halts[3] ++;
											if(  best  ) {
												best_halts[3] ++;
											}
										}
									}
								}
							}
						}
					}

					// now find out, if this offset/rotation is better ... (i.e. matches more fitting buildings)
					// for r=0 we check north and south, for r=1 we check east and west
					for(  int i=r;  i<4;  i+=2  ) {
						if(  best_halts[i]>best_halt  ||  (best_halt==0  &&  neighbour_halts[i]>any_halt)  ) {
							best_halt = best_halts[i];
							any_halt = neighbour_halts[i];
							rotation = i;
							offsets = offset;
						}
					}
				}
			}
		}

		// no suitable ground here ...
		if(  !any_ok  ) {
			return msg;
		}
		// check over/under halt again
		for(  sint16 x=0;  x<desc->get_x(rotation);  x++  ) {
			for(  sint16 y=0;  y<desc->get_y(rotation);  y++  ) {
				const planquadrat_t *pl = welt->access( k-offsets+koord(x,y) );
				for(  uint8 i=0;  i < pl->get_boden_count();  i++  ) {
					halthandle_t test_halt = pl->get_boden_bei(i)->get_halt();
					if( test_halt.is_bound()  &&  player_t::check_owner( new_owner, test_halt->get_owner()) ) {
						halt = test_halt;
						break;
					}
				}
			}
		}
		// is there no halt to connect?
		if(  !halt.is_bound()  ) {
			return "Post muss neben\nHaltestelle\nliegen!\n";
		}
	}
	else {
		// rotation was pre-selected; just search for stop now
		assert(  rotation < desc->get_all_layouts()  );
		koord testsize = desc->get_size(rotation);
		offsets = koord(0,0);

		if(  !welt->square_is_free(k, testsize.x, testsize.y, NULL, desc->get_allowed_climate_bits())  ) {
			return NOTICE_TILE_FULL;
		}
		// check over/under halt
		for(  sint16 x=0;  x<testsize.x;  x++  ) {
			for(  sint16 y=0;  y<testsize.y;  y++  ) {
				const planquadrat_t *pl = welt->access(k+koord(x,y));
				for(  uint8 i=0;  i < pl->get_boden_count();  i++  ) {
					halthandle_t test_halt = pl->get_boden_bei(i)->get_halt();
					if(test_halt.is_bound()) {
						if(!player_t::check_owner( new_owner, test_halt->get_owner())) {
							return "Das Feld gehoert\neinem anderen Spieler\n";
						}
						else if(!halt.is_bound()) {
							halt = test_halt;
						}
						else if(halt != test_halt) {
							 return "Several halts found.";
						}
					}
				}
			}
		}
		if(!halt.is_bound()) {
			halt = suche_nahe_haltestelle(new_owner, welt, welt->lookup_kartenboden(k)->get_pos(), desc->get_x(rotation), desc->get_y(rotation) );
			// is there no halt to connect?
			if(  !halt.is_bound()  ) {
				return "Post muss neben\nHaltestelle\nliegen!\n";
			}
		}
	}

	if(  rotation>desc->get_all_layouts()  )
	{
		rotation %= desc->get_all_layouts();
	}

	settings_t const& s = welt->get_settings();

	sint64 cost;
	sint32 const factor = desc->get_level() * desc->get_x() * desc->get_y();
	if(desc->get_base_price() == PRICE_MAGIC)
	{
		cost = s.cst_multiply_post * factor;
	}
	else
	{
		cost = -desc->get_price() * desc->get_x() * desc->get_y();
	}

	// Must buy land to place buildings
	cost += welt->get_land_value(pos) * desc->get_size(rotation).x * desc->get_size(rotation).y;

	if(player != halt->get_owner() && halt->get_owner()==welt->get_public_player() && player != welt->get_public_player())
	{
		// public stops are expensive!
		// (Except for the public player itself)
		cost -= (s.maint_building * factor * 60);
	}

	if(!player_t::can_afford(player, -cost))
	{
		return NOTICE_INSUFFICIENT_FUNDS;
	}

	gebaeude_t* gb = hausbauer_t::build(halt->get_owner(), pos-offsets, rotation, desc, &halt);
	stadt_t* city = welt->get_city(pos.get_2d()-offsets);
	if(city)
	{
		gb->set_stadt(city);
		city->update_city_stats_with_building(gb, false);
	}
	welt->add_building_to_world_list(gb->access_first_tile());

	// Difficult to distinguish correctly most suitable waytype
	player_t::book_construction_costs(player, cost, k, desc->get_finance_waytype());

	halt->recalc_station_type();

	return NULL;
}

/* build a dock either small or large */
const char *tool_build_station_t::tool_station_dock_aux(player_t *player, koord3d pos, const building_desc_t *desc)
{
	// the cursor cannot be outside the map from here on
	const koord& k = pos.get_2d();
	grund_t *gr = welt->lookup_kartenboden(k);
	if (gr->get_hoehe()!= pos.z) {
		return "";
	}
	slope_t::type hang = gr->get_grund_hang();
	// first get the size
	int len = desc->get_y() - 1;
	koord dx(hang);
	koord last_k = k - dx*len;
	halthandle_t halt;

	sint64 costs;
	if(desc->get_base_price() == PRICE_MAGIC)
	{
		costs =welt->get_settings().cst_multiply_dock * desc->get_level();
	}
	else
	{
		costs = -desc->get_price();
	}

	if(!player_t::can_afford(player, -costs*(len+1)))
	{
		return NOTICE_INSUFFICIENT_FUNDS;
	}

	// check, if we can build here ...

	if(!slope_t::is_single(hang)) {
		return "Dock must be built on single slope!";
	}
	else {
		// iterate up to max(len,1) to ensure that there is at least one tile of water
		// in front of the dock
			for (int i = 0; i <= max(len, 1); i++) {
			if(!welt->is_within_limits(k-dx*i)) {
				// need at least a single tile to navigate ...
				return "Zu nah am Kartenrand";
			}
			// search for nearby stops
			const planquadrat_t* pl = welt->access(k-dx*i);
			for(  uint8 j=0;  j < pl->get_boden_count();  j++  ) {
				halthandle_t test_halt = pl->get_boden_bei(j)->get_halt();
				if(test_halt.is_bound()) {
					if(!player_t::check_owner( player, test_halt->get_owner())) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
					else if(!halt.is_bound()) {
						halt = test_halt;
					}
					else if(halt != test_halt) {
						return "Several halts found.";
					}
				}
			}

			// check whether we can build something
			const grund_t *gr=welt->lookup_kartenboden(k-dx*i);
			if (gr->get_hoehe() != pos.z) {
				return NOTICE_UNSUITABLE_GROUND;
			}
			if (i <= len) {
				if (const char *msg = gr->kann_alle_obj_entfernen(player)) {
					return msg;
				}
			}

			if (i==0) {
				// start tile on slope near water
				if(gr->hat_wege()  ||  gr->get_typ()!=grund_t::boden  ||  gr->is_halt()) {
					return NOTICE_TILE_FULL;
				}
			}
			else {
				// all other tiles in water (allowing one-tile docks on rivers)
				if (!gr->is_water() && !(len == 0 && i == 1 && gr->hat_weg(water_wt))) {
					return NOTICE_UNSUITABLE_GROUND;
				}
				if (gr->find<gebaeude_t>() || gr->get_depot() || gr->is_halt()) {
					return NOTICE_TILE_FULL;
				}
			}
		}
	}

	// remove everything from tile
	gr->obj_loesche_alle(player);

DBG_MESSAGE("tool_build_station_t::tool_station_dock_aux()","building dock from square (%d,%d) to (%d,%d)", k.x, k.y, last_k.x, last_k.y);
	int layout = 0;
	koord3d bau_pos = welt->lookup_kartenboden(k)->get_pos();
	koord dx2;
	switch(hang) {
		case slope_t::south:
		case slope_t::south*2:
			layout = 0;
			dx2 = koord::west;
			break;
		case slope_t::east:
		case slope_t::east*2:
			layout = 1;
			dx2 =  koord::north;
			break;
		case slope_t::north:
		case slope_t::north*2:
			layout = 2;
			dx2 = koord::west;
			bau_pos = welt->lookup_kartenboden(last_k)->get_pos();
			break;
		case slope_t::west:
		case slope_t::west*2:
			layout = 3;
			dx2 =  koord::north;
			bau_pos = welt->lookup_kartenboden(last_k)->get_pos();
			break;
	}

	// handle 16 layouts
	bool change_layout = false;
	if(desc->get_all_layouts()==16) {
		if(  layout<2  ) {
			layout = 15-layout;
		}
		else {
			layout = 9-layout;
		}
		change_layout = true;
	}

	// oriented buildings here - get neighbouring layouts
	const grund_t* gr_neigh = welt->lookup_kartenboden(k + dx2);

	// find out if middle end or start tile
	if (gr_neigh  &&  gr_neigh->is_halt() && player_t::check_owner(player, gr_neigh->get_halt()->get_owner())) {
		gebaeude_t *gb = gr_neigh->find<gebaeude_t>();
		if(gb  &&  (gb->get_tile()->get_desc()->get_type()==building_desc_t::dock  ||  gb->get_tile()->get_desc()->get_type()==building_desc_t::flat_dock)  ) {

			if(change_layout) {
				layout -= 4;
			}
			if(gb->get_tile()->get_desc()->get_all_layouts()==16) {
				sint8 ly = gb->get_tile()->get_layout();
				if(ly&0x02) {
					gb->set_tile( gb->get_tile()->get_desc()->get_tile(ly&0xFD,0,0), false );
				}
			}
		}
	}

	gr_neigh = welt->lookup_kartenboden(k - dx2);
	if (gr_neigh  &&  gr_neigh->is_halt() && player_t::check_owner(player, gr_neigh->get_halt()->get_owner())) {
		gebaeude_t *gb = gr_neigh->find<gebaeude_t>();
		if(gb  &&  (gb->get_tile()->get_desc()->get_type()==building_desc_t::dock  ||  gb->get_tile()->get_desc()->get_type()==building_desc_t::flat_dock)  ) {
			if(change_layout) {
				layout -= 2;
			}
			if(gb->get_tile()->get_desc()->get_all_layouts()==16) {
				sint8 ly = gb->get_tile()->get_layout();
				if(ly&0x04) {
					gb->set_tile( gb->get_tile()->get_desc()->get_tile(ly&0xFB,0,0), false );
				}
			}
		}
	}

//DBG_MESSAGE("tool_dockbau()","search for stop");
	if(!halt.is_bound()) {
		halt = suche_nahe_haltestelle(player, welt, welt->lookup_kartenboden(k)->get_pos() );
	}
	bool neu = !halt.is_bound();

	if(neu)
	{
		if( gr && gr->get_halt().is_bound()  ) {
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		// ok, really new stop on this tile then
		halt = haltestelle_t::create(pos.get_2d(), player);
		if(halt.is_bound() && env_t::networkmode)
		{
			cbuffer_t message;
			const stadt_t* nearest_city = welt->find_nearest_city(pos.get_2d());
			const char * city_name = nearest_city ? nearest_city->get_name() : "open countryside";
			const char* preposition = welt->get_city(pos.get_2d()) || !nearest_city ? "in" : "near";
			message.printf("%s has built a new %s %s %s.", player->get_name(), "Dock", preposition, city_name);
			welt->get_message()->add_message(message, pos.get_2d(), message_t::ai, player->get_player_color1());
		}
	}
	hausbauer_t::build(halt->get_owner(), bau_pos, layout, desc, &halt);

	if(player != halt->get_owner() && player != welt->get_public_player())
	{
		// public stops are expensive!
		// (Except for the public player itself)
		sint64 maint;
		if(desc->get_base_maintenance() == PRICE_MAGIC)
		{
			maint = welt->get_settings().maint_building * desc->get_level();
		}
		else
		{
			maint = desc->get_maintenance();
		}

		costs -= (maint * 60);
	}
	for(  int i=0;  i<=len;  i++  ) {
		koord p=k-dx*i;
		player_t::book_construction_costs(player,  costs, p, water_wt);
	}

	halt->recalc_station_type();
	if(  env_t::station_coverage_show  &&  welt->get_zeiger()->get_pos().get_2d()==k  ) {
		// since we are larger now ...
		halt->mark_unmark_coverage( true );
	}

	if(neu) {
		char* const name = halt->create_name(k, "Dock");
		halt->set_name( name );
		free(name);
	}
	return NULL;
}


/* build a dock either small or large */
const char *tool_build_station_t::tool_station_flat_dock_aux(player_t *player, koord3d pos, const building_desc_t *desc, sint8 rotation)
{
	// the cursor cannot be outside the map from here on
	koord k = pos.get_2d();
	grund_t *gr = welt->lookup_kartenboden(k);
	if (gr->get_hoehe()!= pos.z) {
		return "";
	}
	// first get the size
	int len = desc->get_size().y-1;

	sint64 costs;
	if(desc->get_base_price() == PRICE_MAGIC)
	{
		costs =welt->get_settings().cst_multiply_dock * desc->get_level();
	}
	else
	{
		costs = -desc->get_price();
	}

	if(!player_t::can_afford(player, -costs*(len+1)))
	{
		return NOTICE_INSUFFICIENT_FUNDS;
	}

	// check, if we can build here ...
	if(  !gr->ist_natur()  ||  gr->get_grund_hang() != slope_t::flat  ) {
		return NOTICE_UNSUITABLE_GROUND;
	}

	// now find the direction
	// first: find the next water
	ribi_t::ribi water_dir = 0;
	uint8        total_dir = 0;
	for(  uint8 i=0;  i<4;  i++  ) {
		if(  grund_t *gr = welt->lookup_kartenboden(k+koord::nsew[i])  ) {
			if(  gr->is_water()  &&  gr->get_hoehe() == pos.z) {
				water_dir |= ribi_t::nsew[i];
				total_dir ++;
			}
		}
	}

	// not surrounded by water => fail
	if(  total_dir == 0  ) {
		return NOTICE_UNSUITABLE_GROUND;
	}

	// prefer layouts that reach an existing halt
	ribi_t::ribi halt_dir = 0;
	halthandle_t test_halt[4];

	for(  uint8 ii=0;  ii<4;  ii++  ) {

		if(  (water_dir & ribi_t::nsew[ii]) == 0  ) {
			continue;
		}
		const koord dx = koord::nsew[ii];
		const char *last_error = NULL;

		for(int i=0;  i<=len;  i++  ) {

			// check whether we can build something
			const grund_t *gr = welt->lookup_kartenboden(k+dx*i);
			if( !gr ) {
				// need at least a single tile to navigate ...
				last_error = "Zu nah am Kartenrand";
				break;
			}

			if (gr->get_hoehe() != pos.z) {
				return NOTICE_UNSUITABLE_GROUND;
				break;
			}

			// search for nearby stops
			const planquadrat_t* pl = welt->access(k+dx*i);
			for(  uint8 j=0;  j < pl->get_boden_count()  &&  !test_halt[ii].is_bound();  j++  ) {
				halthandle_t halt = pl->get_boden_bei(j)->get_halt();
				if (halt.is_bound()  &&  player_t::check_owner( player, halt->get_owner()) ) {
					test_halt[ii] = halt;
					halt_dir |= ribi_t::nsew[ii];
				}
			}

			if(  (last_error = gr->kann_alle_obj_entfernen(player))  ) {
				break;
			}

			if (i>0) {
				// all other tiles in water
				if (!gr->is_water()  ||  gr->find<gebaeude_t>()  ||  gr->get_depot()  ||  gr->is_halt()) {
					last_error = NOTICE_TILE_FULL;
				}
			}
		}

		// error: then remove this direction
		if(  last_error  ) {
			water_dir &= ~ribi_t::nsew[ii];
			if(  --total_dir == 0  ) {
				// no duitable directions found
				return last_error;
			}
		}
	}

	// now we may have more than one dir left
	if (rotation == -1  &&  total_dir > 1  &&  !ribi_t::is_single(water_dir & halt_dir) ) {
		return "More than one possibility to build this dock found.";
	}

	// remove everything from tile
	gr->obj_loesche_alle(player);

	koord3d bau_pos = welt->lookup_kartenboden(k)->get_pos();
	koord dx = koord::invalid;
	koord last_k;
	uint8 layout = 0; // building orientation
	halthandle_t halt;

	for(  uint8 i=0;  i<4;  i++  ) {
		if(  water_dir & ribi_t::nsew[i]  ) {
			dx = koord::nsew[i];
			halt = test_halt[i];
			koord last_k = k + dx*len;
			// layout: north 2, west 3, south 0, east 1
			static const uint8 nsew_to_layout[4] = { 2, 0, 1, 3 };
			layout = nsew_to_layout[i];
			if(  layout>=2  ) {
				// reverse construction in these directions
				bau_pos = welt->lookup_kartenboden(last_k)->get_pos();
				if (rotation == layout) {
				// desired rotation works
				break;
			}
			if (rotation != -1) {
				// desired rotation not possible, try others
				if(  --total_dir == 0  ) {
					return NOTICE_UNSUITABLE_GROUND;
				}
			}
			}
		}
	}

	// handle 16 layouts
	bool change_layout = false;
	if(desc->get_all_layouts()==16) {
		if(  layout<2  ) {
			layout = 15-layout;
		}
		else {
			layout = 9-layout;
		}
		change_layout = true;
	}

	// oriented buildings here - get neighbouring layouts
	koord dx2 = dx;
	dx2.rotate90(0);
	gr = welt->lookup_kartenboden(k+dx2);

	// find out if middle end or start tile
	if(gr  &&  gr->is_halt()  &&  player_t::check_owner( player, gr->get_halt()->get_owner() )) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb  &&  (gb->get_tile()->get_desc()->get_type()==building_desc_t::dock  ||  gb->get_tile()->get_desc()->get_type()==building_desc_t::flat_dock)  ) {
			if(change_layout) {
				layout -= 4;
			}
			if(gb->get_tile()->get_desc()->get_all_layouts()==16) {
				sint8 ly = gb->get_tile()->get_layout();
				if(ly&0x02) {
					gb->set_tile( gb->get_tile()->get_desc()->get_tile(ly&0xFD,0,0), false );
				}
			}
		}
	}

	gr = welt->lookup_kartenboden(k-dx2);
	if(gr  &&  gr->is_halt()  &&  player_t::check_owner( player, gr->get_halt()->get_owner() )) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb  &&  (gb->get_tile()->get_desc()->get_type()==building_desc_t::dock  ||  gb->get_tile()->get_desc()->get_type()==building_desc_t::flat_dock)  ) {
			if(change_layout) {
				layout -= 2;
			}
			if(gb->get_tile()->get_desc()->get_all_layouts()==16) {
				sint8 ly = gb->get_tile()->get_layout();
				if(ly&0x04) {
					gb->set_tile( gb->get_tile()->get_desc()->get_tile(ly&0xFB,0,0), false );
				}
			}
		}
	}

	DBG_MESSAGE("tool_station_flat_dock_aux()","building dock from square (%d,%d) to (%d,%d) layout=%i", k.x, k.y, last_k.x, last_k.y, layout );

	if(!halt.is_bound()) {
		halt = suche_nahe_haltestelle(player, welt, welt->lookup_kartenboden(k)->get_pos() );
	}
	bool neu = !halt.is_bound();

	if(neu) {
		if(  gr  &&  gr->get_halt().is_bound()  ) {
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		// ok, really new stop on this tile then
		halt = haltestelle_t::create(k, player);
	}

	hausbauer_t::build(halt->get_owner(), bau_pos, layout, desc, &halt);

	if(player != halt->get_owner() && player != welt->get_public_player())
	{
		// public stops are expensive!
		// (Except for the public player itself)
		sint64 maint;
		if(desc->get_base_maintenance() == PRICE_MAGIC)
		{
			maint = welt->get_settings().maint_building * desc->get_level();
		}
		else
		{
			maint = desc->get_maintenance();
		}

		costs -= (maint * 60);
	}

	for(  int i=0;  i<=len;  i++  ) {
		koord p=k-dx*i;
		player_t::book_construction_costs(player,  costs, p, water_wt);
	}

	halt->recalc_station_type();
	if(  env_t::station_coverage_show  &&  welt->get_zeiger()->get_pos().get_2d()==k  ) {
		// since we are larger now ...
		halt->mark_unmark_coverage( true );
	}

	if(neu) {
		char* const name = halt->create_name(k, "Dock");
		halt->set_name( name );
		free(name);
	}
	return NULL;
}

// build all types of stops but sea harbours
const char *tool_build_station_t::tool_station_aux(player_t *player, koord3d pos, const building_desc_t *desc, waytype_t wegtype, const char *type_name )
{
	const koord& k = pos.get_2d();
DBG_MESSAGE("tool_halt_aux()", "building %s on square %d,%d for waytype %x", desc->get_name(), k.x, k.y, wegtype);
	const char *p_error=(desc->get_all_layouts()==4) ? "No terminal station here!" : "No through station here!";

	// underground is checked in work(); if underground only simple stations are allowed
	// get valid ground
	grund_t *bd = tool_intern_koord_to_weg_grund(player, welt, pos, wegtype, (wegtype == road_wt || wegtype == water_wt));

	if(  !bd  ||  bd->get_weg_hang()!=slope_t::flat  ) {
		// only flat tiles, only one stop per map square
		return "No suitable way on the ground!";
	}

	if(  bd->ist_tunnel()  &&  bd->ist_karten_boden()  ) {
		// do not build on tunnel entries
		return "No suitable way on the ground!";
	}

	if(  bd->get_depot() || bd->get_signalbox() ) {
		// not on depots or signalboxes
		return NOTICE_UNSUITABLE_GROUND;
	}

	if(  bd->hat_weg(air_wt)  &&  bd->get_weg(air_wt)->get_desc()->get_styp()!=type_flat) {
		return "Flugzeughalt muss auf\nRunway liegen!\n";
	}

	wayobj_t *wo = bd->get_wayobj(wegtype);
	if(  wo && wo->clashes_with_halt()  ) {
		return "Cannot combine way object and halt.";
	}

	// find out orientation ...
	uint32 layout = 0;
	ribi_t::ribi ribi=ribi_t::none;
	if(  desc->get_all_layouts()==2  ||  desc->get_all_layouts()==8  ||  desc->get_all_layouts()==16  ) {
		// through station
		if(  bd->has_two_ways()  ) {
			// a crossing or maybe just a tram track on a road ...
			ribi = bd->get_weg_nr(0)->get_ribi_unmasked()  |  bd->get_weg_nr(1)->get_ribi_unmasked();
		}
		else if(  bd->hat_wege()  ) {
			ribi = bd->get_weg_nr(0)->get_ribi_unmasked();
		}
		// not straight: sorry cannot build here ...
		if(  !ribi_t::is_straight(ribi)  ) {
			return p_error;
		}
		layout = (ribi & ribi_t::northsouth)?0 :1;
	}
	else if(  desc->get_all_layouts()==4  ) {
		// terminal station
		if(  bd->hat_wege()  ) {
			ribi = bd->get_weg_nr(0)->get_ribi_unmasked();
		}
		// sorry cannot build here ... (not a terminal tile)
		if(  !ribi_t::is_single(ribi)  ) {
			return p_error;
		}

		switch(ribi) {
			//case ribi_t::south:layout = 0;  break;
			case ribi_t::east:   layout = 1;    break;
			case ribi_t::north:  layout = 2;    break;
			case ribi_t::west:  layout = 3;    break;
		}
	}
	else {
		// something wrong with station number of layouts
		dbg->fatal( "tool_build_station_t::tool_station_aux", "%s has wrong number of layouts (must be 2,4,8,16!)", desc->get_name() );
		return p_error;
	}

	if(  desc->get_all_layouts() == 8  ||  desc->get_all_layouts() == 16  ) {
		// through station - complex layout
		// bits
		// 1 = north south/east west (as simple layout)
		// 2 = use far end image  \ can be combined
		// 3 = use near end image / to use both end image
		// 4 = platform face - 0 = far, 1 = near

		// bit 1 has already been set

//		ribi_t::ribi next_halt = ribi_t::none;
		ribi_t::ribi next_own = ribi_t::none;

		sint8 offset = bd->get_hoehe()+bd->get_weg_yoff()/TILE_HEIGHT_STEP;

		grund_t *gr;
		sint32 neighbour_layout[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
		for(  unsigned i=0;  i<4;  i++  ) {
			// oriented buildings here - get neighbouring layouts
			gr = welt->lookup(koord3d(k+koord::nsew[i],offset));
			if(!gr) {
				// check whether bridge end tile
				grund_t * gr_tmp = welt->lookup( pos+koord3d( (layout & 1 ? koord::west :  koord::north),offset - 1) );
				if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 2) {
					gr = gr_tmp;
				}
				else {
					gr_tmp = welt->lookup( pos+koord3d( (layout & 1 ? koord::west :  koord::north),offset - 2) );
					if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 2) {
						gr = gr_tmp;
					}
				}
			}
			if(  gr && gr->get_halt().is_bound()  ) {
				// check, if there is an oriented stop
				const gebaeude_t* gb = gr->find<gebaeude_t>();
				if(gb  &&  gb->get_tile()->get_desc()->get_all_layouts()>4  &&  (gb->get_tile()->get_desc()->get_type()>building_desc_t::dock  ||  gb->get_tile()->get_desc()->get_type()>building_desc_t::flat_dock)  ) {
					next_own |= ribi_t::nsew[i];
					neighbour_layout[ribi_t::nsew[i]] = gb->get_tile()->get_layout();
				}
			}
		}

		// now for the details
		ribi_t::ribi senkrecht = ~ribi_t::doubles(ribi);
		ribi_t::ribi waagerecht = ribi_t::doubles(ribi);
		if(next_own!=ribi_t::none) {
			// oriented buildings here
			if(ribi_t::is_single(ribi & next_own)) {
				// only a single next neighbour on the same track
				layout |= neighbour_layout[ribi & next_own] & 8;
			}
			else if(ribi_t::is_straight(ribi & next_own)) {
				// two neighbours on the same track, use the north/west one
				layout |= neighbour_layout[ribi & next_own & ribi_t::northwest] & 8;
			}
			else if(ribi_t::is_single((~ribi) & waagerecht & next_own)) {
				// neighbour across break in track
				layout |= neighbour_layout[(~ribi) & waagerecht & next_own] & 8;
			}
			else {
				// no buildings left and right
				// oriented buildings left and right
				if(neighbour_layout[senkrecht & next_own & ribi_t::northwest] != -1) {
					// just rotate layout
					layout |= 8-(neighbour_layout[senkrecht & next_own & ribi_t::northwest]&8);
				}
				else {
					if(neighbour_layout[senkrecht & next_own & ribi_t::southeast] != -1) {
						layout |= 8-(neighbour_layout[senkrecht & next_own & ribi_t::southeast]&8);
					}
				}
			}
		}
		// avoid orientation on 8 tiled buildings
		layout &= (desc->get_all_layouts()-1);
	}

	halthandle_t old_halt = bd->get_halt();
	sint64 old_cost = 0;
	bool recalc_schedule = false;

	halthandle_t halt;

	if(  old_halt.is_bound()  ) {
		gebaeude_t* gb = bd->find<gebaeude_t>();
		if(gb)
		{
			const building_desc_t *old_desc = gb->get_tile()->get_desc();
			if (old_desc == desc)
			{
				// Do nothing if old and new are alike: do not charge the player for doing nothing.
				return NULL;
			}
			if(old_desc->get_level() > desc->get_level() && old_desc->get_capacity() > desc->get_capacity() && !is_ctrl_pressed())
			{
				return ""; // An error message is intrusive here and not very useful.
				//return "Upgrade must have\na higher level";
			}

			if(old_desc->get_base_price() == PRICE_MAGIC)
			{
				switch(old_desc->get_extra()) {
					case road_wt:
						old_cost = welt->get_settings().cst_multiply_roadstop * old_desc->get_level();
						break;
					case track_wt:
					case monorail_wt:
					case maglev_wt:
					case narrowgauge_wt:
					case tram_wt:
						old_cost = welt->get_settings().cst_multiply_station * old_desc->get_level();
						break;
					case water_wt:
						old_cost = welt->get_settings().cst_multiply_dock * old_desc->get_level();
						break;
					case air_wt:
						old_cost = welt->get_settings().cst_multiply_airterminal * old_desc->get_level();
						break;
				}
			}
			else
			{
				old_cost = -old_desc->get_price();
			}
			old_cost *= old_desc->get_x()*old_desc->get_y();
			gb->cleanup( NULL );
			delete gb;
			halt = old_halt;
			if(  old_desc->get_enabled() != desc->get_enabled()  ) {
				recalc_schedule = true;
			}
		}
	}
	else {
		halt = suche_nahe_haltestelle(player,welt,bd->get_pos());
	}

	// seems everything ok, lets build
	bool neu = !halt.is_bound();

	if(neu)
	{
		if(  bd && bd->get_halt().is_bound()  ) {
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		halt = haltestelle_t::create(pos.get_2d(), player);
		if(halt.is_bound() && env_t::networkmode)
		{
			cbuffer_t message;
			const stadt_t* nearest_city = welt->find_nearest_city(pos.get_2d());
			const char * city_name = nearest_city ? nearest_city->get_name() : "open countryside";
			const char* preposition = welt->get_city(pos.get_2d()) || !nearest_city ? "in" : "near";
			int const lang = welt->get_settings().get_name_language_id();
			const char *stop = translator::translate(type_name, lang);
			message.printf("%s has built a new %s %s %s.", player->get_name(), stop, preposition, city_name);
			welt->get_message()->add_message(message, pos.get_2d(), message_t::ai, player->get_player_color1());
		}
	}
	hausbauer_t::build_station_extension_depot(halt->get_owner(), bd->get_pos(), layout, desc, &halt);
	halt->recalc_station_type();

	if(neu) {
		char* const name = halt->create_name(k, type_name);
		halt->set_name(name);
		free(name);
	}

	sint64 cost;
	if(desc->get_base_price() == PRICE_MAGIC)
	{
		switch(desc->get_extra()) {
			case road_wt:
				cost = welt->get_settings().cst_multiply_roadstop * desc->get_level();
				break;
			case track_wt:
			case monorail_wt:
			case maglev_wt:
			case narrowgauge_wt:
			case tram_wt:
				cost = welt->get_settings().cst_multiply_station * desc->get_level();
				break;
			case water_wt:
				cost = welt->get_settings().cst_multiply_dock * desc->get_level();
				break;
			case air_wt:
				cost = welt->get_settings().cst_multiply_airterminal * desc->get_level();
				break;
		}
	}
	else
	{
		cost = -desc->get_price();
	}

	// cost to build new new station
	sint64 adjusted_cost = cost * desc->get_x() * desc->get_y();
	// discount for existing station

	adjusted_cost -= old_cost / 2;

	if(!player->can_afford(-adjusted_cost))
	{
		return NOTICE_INSUFFICIENT_FUNDS;
	}

	if(player != halt->get_owner() && player != welt->get_public_player())
	{
		// public stops are expensive!
		// (Except for the public player itself, of course)
		sint64 maint;
		if(desc->get_base_maintenance() == PRICE_MAGIC)
		{
			maint = welt->get_settings().maint_building * desc->get_level();
		}
		else
		{
			maint = desc->get_maintenance();
		}
		adjusted_cost -= welt->calc_adjusted_monthly_figure(maint * 60);
	}
	player_t::book_construction_costs(player,  adjusted_cost, pos.get_2d(), wegtype);
	if(  env_t::station_coverage_show  &&  welt->get_zeiger()->get_pos().get_2d()==pos.get_2d()  ) {
		// since we are larger now ...
		halt->mark_unmark_coverage( true );
	}

	// the new station (after upgrading) might accept different goods => needs new schedule
	if(  recalc_schedule  ) {
		welt->set_schedule_counter();
	}

	return NULL;
}

// gives the description and sets the rotation value
const building_desc_t *tool_build_station_t::get_desc( sint8 &rotation ) const
{
	char *building = strdup( default_param );
	const building_tile_desc_t *tdsc = NULL;
	if(  building  ) {
		char *p = strrchr( building, ',' );
		if(  p  ) {
			*p++ = 0;
			rotation = atoi( p );
		}
		else {
			rotation = -1;
		}
		tdsc = hausbauer_t::find_tile(building,0);
		free( building );
	}
	if(  tdsc==NULL  ) {
		return NULL;
	}
	return tdsc->get_desc();
}

bool tool_build_station_t::init( player_t * )
{
	sint8 rotation = -1;
	const building_desc_t *bdsc = get_desc( rotation );
	if(  bdsc==NULL  ) {
		return false;
	}
	cursor = bdsc->get_cursor()->get_image_id(0);
	if(  !is_local_execution()  ) {
		// do not change cursor
		return true;
	}
	if(  (bdsc->get_type()==building_desc_t::generic_extension  ||  bdsc->get_type()==building_desc_t::flat_dock)  &&  bdsc->get_all_layouts()>1  ) {
		if(  is_ctrl_pressed()  &&  rotation==-1  ) {
			// call station dialog instead
			destroy_win( magic_station_building_select );
			create_win( new station_building_select_t(bdsc), w_info, magic_station_building_select);
			// we do not activate building yet; else uncomment the return statement
			return false;
		}
		else if(  rotation>=0  ) {
			// rotation is already fixed
			cursor_area = koord( bdsc->get_x(rotation), bdsc->get_y(rotation) );
			cursor_centered = false;
			cursor_offset = koord(0,0);
			if (bdsc->get_type()==building_desc_t::flat_dock  &&  rotation >= 2 ) {
				cursor_offset = cursor_area - koord(1,1);
			}
		}
		else {
			goto set_area_cov;
		}
	}
	else {
set_area_cov:
		uint16 base_cov;
		const bool freight_enabled = bdsc->get_enabled() & 4;
		if(is_shift_pressed() != freight_enabled)
		{
			base_cov = welt->get_settings().get_station_coverage_factories();
		}
		else
		{
			base_cov = welt->get_settings().get_station_coverage();
		}
		uint16 const cov = base_cov * 2 + 1;
		cursor_area = koord(cov, cov);
		cursor_centered = true;
		cursor_offset = koord(0,0);
	}
	return true;
}


image_id tool_build_station_t::get_icon( player_t * ) const
{
	sint8 dummy;
	const building_desc_t *desc=get_desc(dummy);
	if (desc == NULL) {
		return IMG_EMPTY;
	}
	if(  grund_t::underground_mode==grund_t::ugm_all  ) {
		// in underground mode, buildings will be done invisible above ground => disallow such confusion
//		if(desc->get_allow_underground() == 0 ||  desc->get_extra()==air_wt) {
		if(  desc->get_type()!=building_desc_t::generic_stop  ||  desc->get_extra()==air_wt) {
			return IMG_EMPTY;
		}
		if(  desc->get_type()==building_desc_t::generic_stop  &&  !desc->can_be_built_underground()) {
			return IMG_EMPTY;
		}
	}
	if(  grund_t::underground_mode==grund_t::ugm_none  ) {
		if(  desc->get_type()==building_desc_t::generic_stop  &&  !desc->can_be_built_aboveground()) {
			return IMG_EMPTY;
		}
	}
	else if(desc->get_allow_underground() == 1 && grund_t::underground_mode == grund_t::ugm_none)
	{
		// Allow both underground and above ground buildings in sliced underground mode.
		return IMG_EMPTY;
	}
	return icon;
}


char const* tool_build_station_t::get_tooltip(player_t const*player) const
{
	sint8               dummy;
	building_desc_t const* desc    = get_desc(dummy);
	if (desc == NULL) {
		return "";
	}
	sint64 price = 0;
	sint64 maint = 0;
	uint32 cap = desc->get_capacity(); // This is always correct in the desc object.
	if(  desc->get_type()==building_desc_t::generic_stop  )
	{
		if(desc->get_base_maintenance() != PRICE_MAGIC)
		{
			maint = desc->get_maintenance();
		}
		else
		{
			maint = welt->get_settings().maint_building*desc->get_level();
		}
		if(desc->get_base_price() != PRICE_MAGIC)
		{
			price = -desc->get_price();
		}
		else
		{
			switch (desc->get_extra())
			{
			case track_wt:
			case monorail_wt:
			case maglev_wt:
			case tram_wt:
			case narrowgauge_wt:
				price = welt->get_settings().cst_multiply_station * desc->get_level();
				break;
			case road_wt:
				price = welt->get_settings().cst_multiply_roadstop * desc->get_level();
				break;
			case water_wt:
				price = welt->get_settings().cst_multiply_dock * desc->get_level();
				break;
			case air_wt:
				price = welt->get_settings().cst_multiply_airterminal * desc->get_level();
				break;
			case 0:
				price = welt->get_settings().cst_multiply_post * desc->get_level();
				break;
			default:
				return "Illegal description";
			}
		}
	}
	else if(desc->get_type()==building_desc_t::generic_extension || desc->get_type()==building_desc_t::dock || desc->get_type()==building_desc_t::flat_dock)
	{
		if(desc->get_base_maintenance() != PRICE_MAGIC)
		{
			maint = desc->get_maintenance();
		}
		else
		{
			maint = welt->get_settings().maint_building * desc->get_level();
		}

		if(desc->get_base_price() != PRICE_MAGIC)
		{
			price = -desc->get_price();
		}
		else
		{
			if(desc->get_type()==building_desc_t::dock || desc->get_type()==building_desc_t::flat_dock)
			{
				price = welt->get_settings().cst_multiply_dock * desc->get_level();
			}
			else
			{
				price = welt->get_settings().cst_multiply_post * desc->get_level();
			}
		}
		const sint16 size_multiplier = desc->get_size().x * desc->get_size().y;
		price *= size_multiplier;
		cap *= size_multiplier;
		maint *= size_multiplier;
	}

	else
	{
		return "Illegal description";
	}

	return tooltip_with_price_maintenance_level( welt, desc->get_name(), price, maint, cap, desc->get_enabled() );
}

waytype_t tool_build_station_t::get_waytype() const
{
	sint8 dummy;
	building_desc_t const* desc = get_desc(dummy);
	switch (desc ? desc->get_type() : building_desc_t::generic_extension) {
		case building_desc_t::generic_stop:
			return (waytype_t)desc->get_extra();
		case building_desc_t::dock:
		case building_desc_t::flat_dock:
			return water_wt;
		case building_desc_t::generic_extension:
		default:
			return invalid_wt;
	}
}


const char *tool_build_station_t::check_pos( player_t*,  koord3d pos )
{
	if(  grund_t *gr = welt->lookup( pos )  ) {
		sint8 rotation;
		const building_desc_t *desc = get_desc(rotation);
		if(  grund_t *bd = welt->lookup_kartenboden( pos.get_2d() )  ) {
			const bool underground = bd->get_hoehe()>gr->get_hoehe();
			if(  underground  ) {
				// in underground mode, buildings will be done invisible above ground => disallow such confusion
				if(  desc->get_type()!=building_desc_t::generic_stop  ||  desc->get_extra()==air_wt) {
					return "Cannot built this station/building\nin underground mode here.";
				}
				if(  desc->get_type()==building_desc_t::generic_stop  &&  !desc->can_be_built_underground()) {
					return "Cannot built this station/building\nin underground mode here.";
				}
			}
			else if(  desc->get_type()==building_desc_t::generic_stop  &&  !desc->can_be_built_aboveground()) {
				return "This station/building\ncan only be built underground.";
			}
			return NULL;
		}
	}
	// no ground here???
	return "Missing ground (fatal!)";
}


const char *tool_build_station_t::move( player_t *player, uint16 buttonstate, koord3d pos )
{
	// This is rough and ready: if you can afford something costing 1, we decide you can afford this
	// This is a backup check; it gets checked against in ::work.
	// In network mode the check in ::work may go off too late, however.
	// --neroden
	if (! player_t::can_afford( player, 1 ) ) {
		return NOTICE_INSUFFICIENT_FUNDS;
	}

	const char *result = NULL;
	if(  buttonstate==1  ) {
		const grund_t *gr = welt->lookup(pos);
		if(!gr) {
			return "";
		}

		// ownership allowed?
		halthandle_t halt = gr->get_halt();
		if(halt.is_bound()  &&  !player_t::check_owner( player, halt->get_owner())) {
			return "";
		}

		if(  env_t::networkmode  ) {
			// queue tool for network
			nwc_tool_t *nwc = new nwc_tool_t(player, this, pos, welt->get_steps(), welt->get_map_counter(), false);
			network_send_server(nwc);
		}
		else {
			result = work( player, pos );
		}
	}
	return result;
}


const char *tool_build_station_t::work( player_t *player, koord3d pos )
{
	const grund_t *gr = welt->lookup(pos);
	if(!gr) {
		return "";
	}

	// ownership allowed?
	halthandle_t halt = gr->get_halt();
	if(halt.is_bound()  &&  !player_t::check_owner( player, halt->get_owner())) {
		return "Das Feld gehoert\neinem anderen Spieler\n";
	}

	// check underground / above ground
	if (const char* msg = check_pos(player, pos)) {
		return msg;
	}

	sint8 rotation = 0;
	const building_desc_t *desc=get_desc(rotation);
	const char *msg = NULL;
	sint64 cost;
	settings_t const& s = welt->get_settings();
	switch (desc->get_type())
	{
		case building_desc_t::dock:
			msg = tool_build_station_t::tool_station_dock_aux(player, pos, desc );
			break;
		case building_desc_t::flat_dock:
			msg = tool_build_station_t::tool_station_flat_dock_aux(player, pos, desc, rotation );
			break;
		case building_desc_t::generic_extension:
			msg = tool_build_station_t::tool_station_building_aux(player, false, pos, desc, rotation );
			if (msg) {
				// try to build near a public halt
				msg = tool_build_station_t::tool_station_building_aux(player, true, pos, desc, rotation );
			}
			break;
		case building_desc_t::generic_stop: {
			switch(desc->get_extra()) {
				case road_wt:
					msg = tool_build_station_t::tool_station_aux(player, pos, desc, road_wt, "H");
					break;
				case track_wt:
				case monorail_wt:
				case maglev_wt:
				case narrowgauge_wt:
				case tram_wt:
					msg = tool_build_station_t::tool_station_aux(player, pos, desc, (waytype_t)desc->get_extra(), "BF");
					break;
				case water_wt:
					msg = tool_build_station_t::tool_station_aux(player, pos, desc, water_wt, "Dock");
					break;
				case air_wt:
					msg = tool_build_station_t::tool_station_aux(player, pos, desc, air_wt, "Airport");
					break;
			}
			break;
		}

		default:
			dbg->warning("tool_build_station_t::work()","tool called for illegal desc \"%\"", default_param );
			msg = "Illegal station tool";
	}
	return msg;
}

uint8 tool_build_roadsign_t::signal_info::spacing = 16;

char const* tool_build_roadsign_t::get_tooltip(player_t const*) const
{
	const roadsign_desc_t * desc = roadsign_t::find_desc(default_param);
	if(desc)
	{
		char tip[256];
		if(desc->is_signal())
		{
			sprintf(tip, "%s, %s %i%s, %s: %u%s, %s", translator::translate(desc->get_name()), translator::translate("Max. speed:"), speed_to_kmh(desc->get_max_speed()), translator::translate("km/h"), translator::translate("distance"), desc->get_max_distance_to_signalbox(), translator::translate("m"), translator::translate(roadsign_t::get_working_method_name(desc->get_working_method())));
		}
		else
		{
			sprintf(tip, "%s, %s %i%s", translator::translate(desc->get_name()), translator::translate("Max. speed:"), speed_to_kmh(desc->get_max_speed()), translator::translate("km/h"));
		}

		return tooltip_with_price_maintenance(welt, tip, -desc->get_value(), desc->get_maintenance());
	}
	return NULL;
}

void tool_build_roadsign_t::draw_after(scr_coord k, bool dirty) const
{
	if(  icon!=IMG_EMPTY  &&  is_selected()  ) {
		display_img_blend( icon, k.x, k.y, TRANSPARENT50_FLAG|OUTLINE_FLAG|COL_BLACK, false, dirty );
		char level_str[16];
		sprintf(level_str, "%i", signal[welt->get_active_player_nr()].spacing);
		display_proportional( k.x+4, k.y+4, level_str, ALIGN_LEFT, COL_YELLOW, true );
	}
}

const char* tool_build_roadsign_t::check_pos_intern(player_t *player, koord3d pos)
{
	const char * error = "Hier kann kein\nSignal aufge-\nstellt werden!\n";
	if (desc==NULL) {
		// read data from string
		read_default_param(player);
	}
	if (desc==NULL) {
		return error;
	}

	if(welt->lookup_kartenboden(pos.get_2d())->get_hoehe() > pos.z)
	{
		// Do not build above ground only signals underground
		if(desc->get_allow_underground() == 0)
		{
			return "Cannot build this signal underground.";
		}
	}
	else if(desc->get_allow_underground() == 1)
	{
		// Do not build underground only signals above ground.
		return "This can only be built underground.";
	}

	// search for starting ground
	grund_t *gr = tool_intern_koord_to_weg_grund(player, welt, pos, desc->get_wtyp());
	if(gr)
	{

		signal_t *s = gr->find<signal_t>();
		if(s  &&  s->get_desc()!=desc) {
			// only one sign per tile
			return error;
		}

		if(desc->is_signal()  &&  gr->find<roadsign_t>())  {
			// only one sign per tile
			return error;
		}

		// get the sign direction
		weg_t *weg = gr->get_weg( desc->get_wtyp()!=tram_wt ? desc->get_wtyp() : track_wt);
		ribi_t::ribi dir = weg->get_ribi_unmasked();

		// no signs on runways
		if(  weg->get_waytype() == air_wt  &&  weg->get_desc()->get_styp() == type_runway  ) {
			return error;
		}

		// no signals on switches
		if(  ribi_t::is_threeway(dir)  &&  desc->is_signal_type()  ) {
			return error;
		}

		if(desc->is_private_way() &&
		   (!ribi_t::is_straight(dir) || weg->get_owner() != player ||
		    gr->removing_road_would_disconnect_city_building() ||
		    gr->removing_way_would_disrupt_public_right_of_way(road_wt)) &&
			(!player->is_public_service() || weg->get_owner() != NULL))
		{
			// Private way signs only on straight tiles, and only on ways belonging to the player building them, not public rights of way.
			return error;
		}
		if(desc->is_private_way()) {
			weg->set_gehweg(false);
			weg->set_public_right_of_way(false);
		}

		if(!player->is_public_service() && desc->is_single_way() && weg->is_public_right_of_way() && gr->removing_way_would_disrupt_public_right_of_way(road_wt))
		{
			// Cannot interfere with the traffic flow on a public right of way.
			return error;
		}

		// Check whether this is close enough to the selected signalbox (if applicable)
		if(desc->is_signal_type() && desc->get_signal_group())
		{
			signalbox_t* sb = NULL;
			const grund_t* gr_signalbox = welt->lookup(signal[player->get_player_nr()].signalbox);
			if(gr_signalbox)
			{
				const gebaeude_t* gb = gr_signalbox->get_building();
				if(gb && gb->get_tile()->get_desc()->is_signalbox())
				{
					sb = (signalbox_t*)gb;
				}
			}

			if(!sb)
			{
				// The signalbox has been deleted: cannot build this signal here.
				if(is_local_execution())
				{
					player->set_selected_signalbox(NULL);
				}
				return "Cannot build this signal without a signalbox. Build a new signalbox or select an existing one to build this signal.";
			}

			const uint16 distance = shortest_distance(signal[player->get_player_nr()].signalbox.get_2d(), pos.get_2d()) * welt->get_settings().get_meters_per_tile();
			if(distance > sb->get_tile()->get_desc()->get_radius() && sb->get_tile()->get_desc()->get_radius() > 0)
			{
				return "Cannot build any signal beyond the maximum radius of the currently selected signalbox.";
			}
			if(distance > desc->get_max_distance_to_signalbox() && desc->get_max_distance_to_signalbox() > 0 && desc->get_working_method() != moving_block) // Moving block signalling uses this for other purposes
			{
				return "Cannot build this signal this far beyond any signalbox.";
			}

			if(!sb->can_add_more_signals())
			{
				signal_t* signal = gr->find<signal_t>();
				if(!signal)
				{
					return "Cannot build any more signals connected to this signalbox: capacity exceeded.";
				}
			}
		}

		const bool two_way = desc->is_single_way()  ||  desc->is_signal() ||  desc->is_pre_signal();

		if(!(desc->is_traffic_light() || two_way)  ||  (two_way  &&  ribi_t::is_twoway(dir))  ||  (desc->is_traffic_light()  &&  ribi_t::is_threeway(dir))) {
			roadsign_t* rs;
			if (desc->is_signal_type()) {
				// if there is already a signal, we might need to inverse the direction
				rs = gr->find<signal_t>();
				if (rs) {
					if(  !player_t::check_owner( rs->get_owner(), player )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
				}
			}
			else {
				// if there is already a sign, we might need to inverse the direction
				rs = gr->find<roadsign_t>();
				if (rs) {
					if(  !player_t::check_owner( rs->get_owner(), player )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
				}
			}
			error = NULL;
		}
	}
	return error;
}

char tool_build_roadsign_t::toolstring[256];

const char* tool_build_roadsign_t::get_default_param(player_t *player) const
{
	if (desc  &&  player) {
		signal_info const& s = signal[player->get_player_nr()];
		sprintf(toolstring, "%s,%d,%d,%d,%d,%d,%d", desc->get_name(), s.spacing, s.remove_intermediate, s.replace_other, s.signalbox.x, s.signalbox.y, s.signalbox.z);
		return toolstring;
	}
	else {
		return default_param;
	}
}

waytype_t tool_build_roadsign_t::get_waytype() const
{
	return desc ? desc->get_wtyp() : invalid_wt;
}

// read variables from default_param if cmd comes from network
// default_param: sign_name,signal_spacing,remove,replace
// if the static variable toolstring is the default_param then reset default_param to name of signal
void tool_build_roadsign_t::read_default_param(player_t * player)
{
	char name[256]="";
	uint32 i;
	for(i=0; default_param[i]!=0  &&  default_param[i]!=','; i++) {
		name[i]=default_param[i];
	}
	name[i]=0;
	desc = roadsign_t::find_desc(name);

	if (default_param[i]) {
		signal_info& s = signal[player->get_player_nr()];
		int i_signal_spacing              = s.spacing;
		int i_remove_intermediate_signals = s.remove_intermediate;
		int i_replace_other_signals       = s.replace_other;
		int i_x							  = s.signalbox.x;
		int i_y							  = s.signalbox.y;
		int i_z							  = s.signalbox.z;
		sscanf(default_param+i, ",%d,%d,%d,%d,%d,%d", &i_signal_spacing, &i_remove_intermediate_signals, &i_replace_other_signals, &i_x, &i_y, &i_z);
		s.spacing             = (uint8)i_signal_spacing;
		s.remove_intermediate = i_remove_intermediate_signals != 0;
		s.replace_other       = i_replace_other_signals       != 0;
		s.signalbox.x = i_x;
		s.signalbox.y = i_y;
		s.signalbox.z = i_z;
	}
	if (default_param==toolstring) {
		default_param = desc->get_name();
	}
}

bool tool_build_roadsign_t::init( player_t * player)
{
	// read data from string
	read_default_param(player);

	if(is_local_execution())
	{
		signal[player->get_player_nr()].signalbox = player->get_selected_signalbox() ? player->get_selected_signalbox()->get_pos() : koord3d::invalid;
	}

	if (is_ctrl_pressed()  &&  is_local_execution()) {
		create_win(new signal_spacing_frame_t(player, this), w_info, (ptrdiff_t)this);
	}
	return two_click_tool_t::init(player) && (desc!=NULL);
}

void tool_build_roadsign_t::rotate90(sint16 y_diff)
{
	for(sint32 i = 0; i < MAX_PLAYER_COUNT; i++)
	{
		signal[i].signalbox.rotate90(y_diff);
	}
}

bool tool_build_roadsign_t::exit( player_t *player )
{
	destroy_win((ptrdiff_t)this);
	return two_click_tool_t::exit(player);
}

uint8 tool_build_roadsign_t::is_valid_pos( player_t *player, const koord3d &pos, const char *&error, const koord3d &start)
{
	// first click
	if (start==koord3d::invalid) {
		error = check_pos_intern(player, pos);
		return (error==NULL ? 3 : 0);
	}
	// second click
	else {
		error = NULL;
		return 2;
	}
}


bool tool_build_roadsign_t::calc_route( route_t &verbindung, player_t *player, const koord3d& start, const koord3d& to )
{
	// get a default vehicle
	vehicle_desc_t rs_desc( desc->get_wtyp(), 500, vehicle_desc_t::diesel );
	vehicle_t* test_vehicle = vehicle_builder_t::build(start, player, NULL, &rs_desc);
	test_vehicle->set_flag(obj_t::not_on_map);
	test_driver_t* test_driver = scenario_checker_t::apply(test_vehicle, player, this);

	bool can_built;
	if( start != to ) {
		can_built = verbindung.calc_route(welt, start, to, test_driver, 0, 0, false, 0);
		// prevent building of many signals if start and to are adjacent
		// but the step start->to is now allowed
		if (can_built  &&  koord_distance(start, to)==1  &&  verbindung.get_count()>2) {
			grund_t *gr, *grto = welt->lookup(to);
			if(  welt->lookup(start)->get_neighbour(gr, desc->get_wtyp(), ribi_type(to-start) )  &&  gr==grto) {
				can_built = false;
			}
		}
	}
	else {
		verbindung.clear();
		verbindung.append( start );
		can_built = true;
	}
	delete test_driver;
	return can_built;
}

void tool_build_roadsign_t::mark_tiles( player_t *player, const koord3d &start, const koord3d &ziel )
{
	route_t route;
	if (!calc_route(route, player, start, ziel)) {
		return;
	}
	signal_info const& s              = signal[player->get_player_nr()];
	uint8       const  signal_density = 2 * s.spacing;      // measured in half tiles (straight track count as 2, diagonal as 1, since sqrt(1/2) = 1/2 ;)
	uint8              next_signal    = signal_density + 1; // to place a sign asap
	sint32             cost           = 0;
	directions.clear();
	// dummy roadsign to get images for preview
	roadsign_t *dummy_rs;
	if (desc->is_signal_type()) {
		dummy_rs = new signal_t(player, koord3d::invalid, ribi_t::none, desc, koord3d::invalid, true);
	}
	else {
		dummy_rs = new roadsign_t(player, koord3d::invalid, ribi_t::none, desc, true);
	}
	dummy_rs->set_flag(obj_t::not_on_map);

	bool single_ribi = desc->is_signal_type() || desc->is_single_way() || desc->is_choose_sign();
	for(  uint16 i = 0;  i < route.get_count();  i++  ) {
		grund_t* gr = welt->lookup( route.at(i) );

		weg_t *weg = gr->get_weg(desc->get_wtyp());
		ribi_t::ribi ribi=weg->get_ribi_unmasked(); // set full ribi when signal is on a crossing.
		if(  single_ribi  ) {
			if(i>0) {
				// take backward direction
				ribi = ribi_type(route.at(i), route.at(i-1));
			}
			else {
				// clear one direction bit to get single direction for signal
				ribi &= ~ribi_type(route.at(i), route.at(i+1));
			}
		}

		roadsign_t *rs = gr->find<signal_t>();
		if (rs==NULL) {
			rs = gr->find<roadsign_t>();
		}

		if (rs  &&  rs->get_waytype() != desc->get_waytype()) {
			// do not delete signs from other ways
			continue;
		}

		// check owner .. other signals...
		bool straight = (i == 0)  ||  (i == route.get_count()-1)  ||  ribi_t::is_straight(ribi_type(route.at(i-1), route.at(i+1)));
		next_signal += straight ? 2 : 1;
		if(  next_signal >= signal_density  ) {
			// can we place signal here?
			if (check_pos_intern(player, route.at(i))==NULL  ||
					(s.replace_other && rs && !rs-> is_deletable(player))) {
				zeiger_t* zeiger = new zeiger_t(gr->get_pos(), player );
				marked.append(zeiger);
				zeiger->set_image( skinverwaltung_t::bauigelsymbol->get_image_id(0) );
				gr->obj_add( zeiger );
				directions.append(ribi /* !=0 -> place sign*/);
				next_signal = 0;
				dummy_rs->set_pos(gr->get_pos());
				dummy_rs->set_dir(ribi); // calls calc_image()
				zeiger->set_after_image(dummy_rs->get_front_image());
				zeiger->set_image(dummy_rs->get_image());
				cost += rs ? (rs->get_desc()==desc ? 0  : desc->get_value()+rs->get_desc()->get_value()) : desc->get_value();
			}
		} else if (s.remove_intermediate && rs && !rs-> is_deletable(player)) {
				zeiger_t* zeiger = new zeiger_t(gr->get_pos(), player );
				marked.append(zeiger);
				zeiger->set_image( tool_t::general_tool[TOOL_REMOVER]->cursor );
				gr->obj_add( zeiger );
				directions.append(ribi_t::none /*remove sign*/);
				cost += rs->get_desc()->get_value();
		}
	}
	delete dummy_rs;
	win_set_static_tooltip( tooltip_with_price("Building costs estimates", cost ) );
}

const char *tool_build_roadsign_t::do_work( player_t *player, const koord3d &start, const koord3d &end)
{
	// read data from string
	read_default_param(player);
	// single click ->place signal
	if( end == koord3d::invalid  ||  start == end ) {
		grund_t *gr = welt->lookup(start);
		return place_sign_intern( player, gr );
	}
	// mark tiles to calculate positions of signals
	mark_tiles(player, start, end);
	// only search the marked tiles
	uint32 j=0;
	FOR(slist_tpl<zeiger_t*>, const i, marked) {
		grund_t* const gr = welt->lookup(i->get_pos());
		weg_t *weg = gr->get_weg(desc->get_wtyp());
		ribi_t::ribi dir = directions[j++];
		if (dir) {
			// try to place signal
			const char* error_text =  place_sign_intern( player, gr );
			if(  error_text  ) {
				if (signal[player->get_player_nr()].replace_other) {
					roadsign_t* rs = gr->find<signal_t>();
					if(rs == NULL) rs = gr->find<roadsign_t>();
					if(  rs != NULL  &&  rs-> is_deletable(player) == NULL  ) {
						rs->cleanup(player);
						delete rs;
						error_text =  place_sign_intern( player, gr );
					}
				}
			}
			if(  error_text  ) {
				return error_text;
			}
			roadsign_t* rs = gr->find<signal_t>();
			if(rs == NULL) rs = gr->find<roadsign_t>();
			assert(rs);
			rs->set_dir(dir);
		}
		else {
			// Place no signal -> remove existing signal
			roadsign_t* rs = gr->find<signal_t>();
			if(rs == NULL) rs = gr->find<roadsign_t>();
			if(  rs != NULL  &&  rs-> is_deletable(player) == NULL  ) {
				rs->cleanup(player);
				delete rs;
			};
		}
		weg->count_sign();
		gr->calc_image();
	}
	cleanup();
	directions.clear();
	return NULL;
}

/*
 * Called by the GUI (gui/signal_spacing.*)
 */
void tool_build_roadsign_t::set_values( player_t *player, uint8 spacing, bool remove, bool replace, koord3d signalbox )
{
	signal_info& s = signal[player->get_player_nr()];
	s.spacing             = spacing;
	s.remove_intermediate = remove;
	s.replace_other       = replace;
	s.signalbox			  = signalbox;
}


void tool_build_roadsign_t::get_values( player_t *player, uint8 &spacing, bool &remove, bool &replace, koord3d &signalbox )
{
	signal_info &s = signal[player->get_player_nr()];
	spacing = s.spacing;
	remove  = s.remove_intermediate;
	replace = s.replace_other;
	signalbox = s.signalbox;
}


const char *tool_build_roadsign_t::place_sign_intern( player_t *player, grund_t* gr, const roadsign_desc_t*)
{
	const char * error = "Hier kann kein\nSignal aufge-\nstellt werden!\n";
	// search for starting ground
	if(gr) {
		// get the sign direction
		weg_t *weg = gr->get_weg( desc->get_wtyp()!=tram_wt ? desc->get_wtyp() : track_wt);
		roadsign_t *s = gr->find<signal_t>();
		if(s==NULL) {
			s = gr->find<roadsign_t>();
		}
		if(s  &&  s->get_desc()!=desc) {
			// only one sign per tile
			return error;
		}
		ribi_t::ribi dir = weg->get_ribi_unmasked();

		const bool two_way = desc->is_single_way()  ||  desc->is_signal() ||  desc->is_pre_signal();

		if(!(desc->is_traffic_light() || two_way)  ||  (two_way  &&  ribi_t::is_twoway(dir))  ||  (desc->is_traffic_light()  &&  ribi_t::is_threeway(dir))) {
			roadsign_t* rs;
			if (desc->is_signal_type()) {
				// if there is already a signal, we might need to inverse the direction
				signal_t* sig = gr->find<signal_t>();
				if (sig) {
					if(  !player_t::check_owner( sig->get_owner(), player )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
					// signals have three options
					ribi_t::ribi sig_dir = sig->get_dir();
					uint8 i = 0;
					const bool old_direction_was_double = ribi_t::is_twoway(sig_dir);
					if (!old_direction_was_double) {
						// inverse first dir
						for (; i < 4; i++) {
							if ((dir & ribi_t::nsew[i]) == sig_dir) {
								i++;
								break;
							}
						}
					}
					// find the second dir ...
					for (; i < 4; i++) {
						if ((dir & ribi_t::nsew[i]) != 0) {
							dir = ribi_t::nsew[i];
						}
					}
					// if nothing found, we have two ways again ...
					if(ribi_t::is_twoway(dir))
					{
						if(sig->get_desc()->get_working_method() != track_circuit_block && sig->get_desc()->get_working_method() != cab_signalling && sig->get_desc()->get_working_method() != moving_block)
						{
							// Only some types of signals can work properly as bidirectional: not this type.
							dir = ~sig->get_dir() & weg->get_ribi_unmasked();
						}
						else
						{
							// This type can be built bidirectionally, but it costs twice as much to do so.
							player_t::book_construction_costs(player, -desc->get_value(), gr->get_pos().get_2d(), weg->get_waytype());
							player_t::add_maintenance(player, desc->get_maintenance(), weg->get_waytype());
						}
					}
					else if(old_direction_was_double)
					{
						// Reduce the maintenance cost and refund the price (refunding is necessary, as cycling through a bidirectional type is unavoidable).
						player_t::book_construction_costs(player, desc->get_value(), gr->get_pos().get_2d(), weg->get_waytype());
						player_t::add_maintenance(player, -desc->get_maintenance(), weg->get_waytype());
					}
					sig->set_dir(dir);
				} else {
					// add a new signal at position zero!

					if(weg->get_waytype() == track_wt || weg->get_waytype() == monorail_wt || weg->get_waytype() == narrowgauge_wt || weg->get_waytype() == maglev_wt)
					{
						const schiene_t* sch = (schiene_t*)weg;
						if (!sch->is_diagonal())
						{
							dir = sch->get_reserved_direction();

							switch (dir) // These are reversed for some odd reason.
							{
							case ribi_t::northsouth:
							case ribi_t::north:
							case ribi_t::northwest:
								dir = ribi_t::south;
								break;
							case ribi_t::south:
							case ribi_t::southwest:
								dir = ribi_t::north;
								break;
							case ribi_t::eastwest:
							case ribi_t::east:
							case ribi_t::southeast:
							case ribi_t::northeast:
								dir = ribi_t::west;
								break;
							case ribi_t::west:
								dir = ribi_t::east;
								break;

							default:
								switch (weg->get_ribi_unmasked())
								{
								case ribi_t::northsouth:
									dir = ribi_t::south;
									break;
								case ribi_t::eastwest:
									dir = ribi_t::west;
									break;
								};
							};
						}
					}
					rs = new signal_t(player, gr->get_pos(), dir, desc, signal[player->get_player_nr()].signalbox);
					DBG_MESSAGE("tool_roadsign()", "new signal, dir is %i", dir);
					goto built_sign;
				}
			} else {
				// if there is already a sign, we might need to inverse the direction
				rs = gr->find<roadsign_t>();
				if (rs) {
					if(  !player_t::check_owner( rs->get_owner(), player )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
					// reverse only if single way sign
					if (desc->is_single_way() || desc->is_choose_sign()) {
						dir = ~rs->get_dir() & weg->get_ribi_unmasked();
						rs->set_dir(dir);
						DBG_MESSAGE("tool_roadsign()", "reverse ribi %i", dir);
					}
				}
				else {
					// add a new roadsign at position zero!
					// if single way, we need to reduce the allowed ribi to one
					if (desc->is_single_way() || desc->is_choose_sign()) {
						for(  int i=0;  i<4;  i++  ) {
							if ((dir & ribi_t::nsew[i]) != 0) {
								dir = ribi_t::nsew[i];
								break;
							}
						}
					}
					DBG_MESSAGE("tool_roadsign()", "new roadsign, dir is %i", dir);
					rs = new roadsign_t(player, gr->get_pos(), dir, desc);
built_sign:
					gr->obj_add(rs);
					rs->finish_rd();	// to make them visible
					weg->count_sign();
					player_t::book_construction_costs(player, -desc->get_value(), gr->get_pos().get_2d(), weg->get_waytype());
					player_t::add_maintenance(player, desc->get_maintenance(), weg->get_waytype());
				}
			}
			if(desc->get_wtyp() == road_wt)
			{
				welt->set_recheck_road_connexions();
			}
			error = NULL;
		}
	}
	return error;
}

// Build signalboxes
const char* tool_signalbox_t::tool_signalbox_aux(player_t* player, koord3d pos, const building_desc_t* desc, sint64 cost)
{
	if (cost == PRICE_MAGIC)
	{
		cost = -welt->get_settings().cst_multiply_station * desc->get_level();
	}

	cost += welt->get_land_value(pos);

	if ( !player_t::can_afford(player, -cost) )
	{
		return NOTICE_INSUFFICIENT_FUNDS;
	}

	grund_t *gr = welt->lookup_kartenboden(pos.get_2d());

	if(welt->is_within_limits(pos.get_2d()))
	{
		// full underground mode: coordinate is on ground, adjust it to one level below ground
		// not possible in network mode!
		if (!env_t::networkmode  &&  grund_t::underground_mode == grund_t::ugm_all)
		{
			pos = gr->get_pos() - koord3d(0,0,1);
		}

		if(gr->get_hoehe() != pos.z)
		{
			// Do not build above ground only signals underground
			if(desc->get_allow_underground() == 0)
			{
				return "Cannot built this station/building\nin underground mode here.";
			}
		}

		if(desc->get_allow_underground() == 1 && gr->get_hoehe() <= pos.z)
		{
			// Do not build underground only signalboxes above ground.
			if(env_t::networkmode)
			{
				return "Cannot built this station/building\nin underground mode here.";
			}
			else
			{
				return "This can only be built underground.";
			}
		}

		if(!gr || gr->is_water() || (gr->get_weg_nr(0) && gr->get_weg_nr(0)->get_pos().z == pos.z) || (gr->get_building() && gr->get_building()->get_pos().z == pos.z) || gr->is_halt())
		{
			// No ground, water, or the ground has a way or building on it.
			// TODO: Consider allowing special gantry signalboxes
			// that can be built upon ways.
			return "A signalbox cannot be built here.";
		}

		bool underground = gr->get_pos().z == pos.z+1;

		// underground: first build tunnel tile	at coordinate pos
		if(underground)
		{
			if(gr->is_water())
			{
				return "Cannot build signalbox underwater.";
			}

			if(welt->lookup(pos))
			{
				return NOTICE_TILE_FULL;
			}

			const tunnel_desc_t *tunnel_desc = tunnel_builder_t::get_tunnel_desc(track_wt, 0, 0);
			if(tunnel_desc == NULL)
			{
				return "Cannot built this station/building\nin underground mode here.";
			}

			tunnelboden_t* tunnel = new tunnelboden_t(pos, 0);
			welt->access(pos.get_2d())->boden_hinzufuegen(tunnel);
			tunnel->obj_add(new tunnel_t(pos, player, tunnel_desc));
			player_t::add_maintenance( player, tunnel_desc->get_maintenance(), tunnel_desc->get_finance_waytype() );
			gr = tunnel;
		}

		const char *error = gr->kann_alle_obj_entfernen(player);
		if(error)
		{
			return error;
		}

		if(gr->get_grund_hang() == 0)
		{
			int layout = 0;
			koord k(pos.get_2d());
			int trackdir = 0;
			for(  int i = 1;  i < 8;  i+=2  )
			{
				grund_t *gr2 = welt->lookup_kartenboden(k + koord::neighbours[i]);
				if(  gr2  &&  gr2->get_weg_hang() == gr2->get_grund_hang()  &&  (gr2->get_weg(track_wt) != NULL || gr2->get_weg(monorail_wt) != NULL || gr2->get_weg(maglev_wt) != NULL || gr2->get_weg(narrowgauge_wt) != NULL))
				{
					// update directions - note this is SENW, conversion from neighbours to SENW is
					// neighbours SENW
					// 3          0
					// 5          1
					// 7          2
					// 1          3
					trackdir += (1 << (((i-3)/2)&3));
				}
			}
			layout = building_layout[trackdir];

			uint8 rotation = desc->get_all_layouts();
			koord size = desc->get_size(rotation);

			bool hat_platz = welt->square_is_free( k, desc->get_x(rotation), desc->get_y(rotation), NULL, desc->get_allowed_climate_bits() );
			if(!hat_platz  &&  size.y!=size.x  &&  desc->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#'  ||  default_param[1]=='A')) {
				// try other rotation too ...
				rotation = (rotation+1) % desc->get_all_layouts();
				hat_platz = welt->square_is_free( k, desc->get_x(rotation), desc->get_y(rotation), NULL, desc->get_allowed_climate_bits() );
			}

			if(hat_platz)
			{
				gebaeude_t* gb = hausbauer_t::build(player, gr->get_pos(), layout, desc);
				player_t::book_construction_costs(player, -cost, pos.get_2d(), desc->get_finance_waytype());
				if (is_local_execution() && player == welt->get_active_player())
				{
					player->set_selected_signalbox((signalbox_t*)gb);
					welt->set_tool(general_tool[TOOL_QUERY], player);
				}

				return NULL;
			}
		}
		return "A signalbox cannot be built here.";
	}
	return "";
}

image_id tool_signalbox_t::get_icon(player_t* player) const
{
	const building_desc_t *desc = hausbauer_t::find_tile(default_param,0)->get_desc();
	const uint16 time = welt->get_timeline_year_month();
	if( desc && desc->is_available(time) )
	{
		return desc->get_cursor()->get_image_id(1);
	}
	return IMG_EMPTY;
}

char const* tool_signalbox_t::get_tooltip(player_t const*) const
{
	building_desc_t const* desc = hausbauer_t::find_tile(default_param, 0)->get_desc();
	if (desc == NULL)
	{
		return NULL;
	}
	char tip[256];
	sprintf(tip, "%s, %s: %i%s, %s: %i", translator::translate(desc->get_name()), translator::translate("Radius"), desc->get_radius(), translator::translate("m"), translator::translate("Max. signals"), desc->get_capacity());

	sint64 price = desc->get_price();
	sint64 maintenance = desc->get_maintenance();

	if (price == PRICE_MAGIC)
	{
		price = desc->get_level() * world()->get_settings().cst_multiply_station;
	}
	else
	{
		price = -price;
	}

	if (maintenance == PRICE_MAGIC)
	{
		maintenance = desc->get_level() * world()->get_settings().maint_building;
	}

	return tooltip_with_price_maintenance(welt, tip, price, maintenance);
}

const char* tool_signalbox_t::check_pos(player_t *, koord3d pos)
{
	if(grund_t::underground_mode == grund_t::ugm_all  &&  env_t::networkmode)
	{
		// clients cannot guess at which height this signalbox should be built
		return "Cannot built this station/building\nin underground mode here.";
	}
	if(grund_t::underground_mode == grund_t::ugm_level)
	{
		// only above or directly under the surface
		grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
		return (gr->get_pos() == pos  ||  gr->get_hoehe() == grund_t::underground_level+1) ? NULL : "";
	}

	return NULL;
}

bool tool_signalbox_t::init(player_t *player)
{
	building_desc_t const* desc = hausbauer_t::find_tile(default_param, 0)->get_desc();
	if (desc == NULL)
	{
		return false;
	}

	cursor = desc->get_cursor()->get_image_id(0);
	return true;
}

const char *tool_signalbox_t::work( player_t *player, koord3d pos )
{
	building_desc_t const* const desc = hausbauer_t::find_tile(default_param,0)->get_desc();

	return tool_signalbox_t::tool_signalbox_aux(player, pos, desc, desc->get_price());
}

// build all types of depots
const char *tool_depot_t::tool_depot_aux(player_t *player, koord3d pos, const building_desc_t *desc, waytype_t wegtype, sint64 cost)
{
	if ( !player_t::can_afford(player, -cost) ) {
		return NOTICE_INSUFFICIENT_FUNDS;
	}

	if(welt->is_within_limits(pos.get_2d())) {
		grund_t *bd=NULL;
		// special for the seven seas ...
		if(wegtype==water_wt) {
			bd = welt->lookup_kartenboden(pos.get_2d());
			if(!bd->is_water()) {
				bd = NULL;
			}
		}
		if(bd==NULL) {
			bd = tool_intern_koord_to_weg_grund(player,welt,pos,wegtype);
		}
		if(!bd  ||  bd->has_two_ways()) {
			return NOTICE_DEPOT_BAD_POS;
		}

		// no depots on runways!
		if(desc->get_extra()==air_wt  &&  bd->get_weg(air_wt)->get_desc()->get_styp()!= type_flat) {
			return NOTICE_DEPOT_BAD_POS;
		}

		const char *p=bd->kann_alle_obj_entfernen(player);
		if(p) {
			return p;
		}

		// avoid building over a stop
		if(bd->is_halt()  ||  bd->get_depot()!=NULL) {
			return NOTICE_DEPOT_BAD_POS;
		}

		ribi_t::ribi ribi;
		if(bd->is_water()) {
			// assume one orientation with water
			ribi = ribi_t::south;
		}
		else {
			ribi = bd->get_weg_ribi_unmasked(wegtype);
		}

		if(ribi_t::is_single(ribi)  &&  bd->get_weg_hang()==0) {

			int layout = 0;
			switch(ribi) {
				//case ribi_t::south:layout = 0;  break;
				case ribi_t::east:   layout = 1;    break;
				case ribi_t::north:  layout = 2;    break;
				case ribi_t::west:  layout = 3;    break;
			}
			hausbauer_t::build_station_extension_depot(player, bd->get_pos(), layout, desc );
			player_t::book_construction_costs(player, cost, pos.get_2d(), desc->get_finance_waytype());
			if(is_local_execution()  &&  player == welt->get_active_player()) {
				welt->set_tool( general_tool[TOOL_QUERY], player );
			}

			return NULL;
		}
		return NOTICE_DEPOT_BAD_POS;
	}
	return "";
}

image_id tool_depot_t::get_icon(player_t *player) const
{
	if(  player  &&  player->get_player_nr()!=1  ) {
		const building_desc_t *desc = hausbauer_t::find_tile(default_param,0)->get_desc();
		const uint16 time = welt->get_timeline_year_month();
		if( desc && desc->is_available(time) ) {
			return desc->get_cursor()->get_image_id(1);
		}
	}
	return IMG_EMPTY;
}

bool tool_depot_t::init( player_t *player )
{
	building_desc_t const* desc = hausbauer_t::find_tile(default_param, 0)->get_desc();
	if (desc == NULL) {
		return false;
	}
	// no depots for player 1
	if(player!=welt->get_public_player()) {
		cursor = desc->get_cursor()->get_image_id(0);
		return true;
	}
	return false;
}

const char* tool_depot_t::get_tooltip(const player_t *) const
{
	settings_t   const& settings = welt->get_settings();
	building_desc_t const* desc    = hausbauer_t::find_tile(default_param, 0)->get_desc();
	if (desc == NULL) {
		return NULL;
	}
	char         const* tip      = translator::translate(desc->get_name());
	sint64              price;
	switch (desc->get_extra()) {
		case road_wt:        price = settings.cst_depot_road; break;
		case track_wt:       price = settings.cst_depot_rail; break;
		case monorail_wt:    price = settings.cst_depot_rail; break;
		case maglev_wt:      price = settings.cst_depot_rail; break;
		case narrowgauge_wt: price = settings.cst_depot_rail; break;
		case tram_wt:        price = settings.cst_depot_rail; break;
		case water_wt:       price = settings.cst_depot_ship; break;
		case air_wt:         price = settings.cst_depot_air; break;
		default:             return 0;
	}
	const uint16 level =  desc->get_level();
	return tooltip_with_price_maintenance(welt, tip, price * level, settings.maint_building * level);
}

waytype_t tool_depot_t::get_waytype() const
{
	const building_desc_t *desc = hausbauer_t::find_tile(default_param,0)->get_desc();
	return desc ? (waytype_t)desc->get_extra() : invalid_wt;
}

const char *tool_depot_t::work( player_t *player, koord3d pos )
{
	if(player==welt->get_public_player()) {
		// no depots for player 1
		return 0;
	}

	building_desc_t const* const desc = hausbauer_t::find_tile(default_param,0)->get_desc();
	settings_t   const&       s     = welt->get_settings();
	switch(desc->get_extra()) {
		case road_wt:
			return tool_depot_t::tool_depot_aux( player, pos, desc, road_wt, s.cst_depot_road * desc->get_level());
		case track_wt:
			return tool_depot_t::tool_depot_aux( player, pos, desc, track_wt, s.cst_depot_rail  * desc->get_level());
		case monorail_wt:
			{
				// since it needs also a foundation, this is slightly more complex ...
				char const* const err = tool_depot_t::tool_depot_aux(player, pos, desc, monorail_wt, s.cst_depot_rail);
				if(err==NULL) {
					grund_t *bd = welt->lookup_kartenboden(pos.get_2d());
					if(hausbauer_t::elevated_foundation_desc  &&  pos.z-bd->get_pos().z==1  &&  bd->ist_natur()) {
						hausbauer_t::build(player, bd->get_pos(), 0, hausbauer_t::elevated_foundation_desc );
					}
				}
				return err;
			}
		case tram_wt:
			return tool_depot_t::tool_depot_aux(player, pos, desc, track_wt, s.cst_depot_rail * desc->get_level());
		case water_wt:
			return tool_depot_t::tool_depot_aux(player, pos, desc, water_wt, s.cst_depot_ship * desc->get_level());
		case air_wt:
			return tool_depot_t::tool_depot_aux(player, pos, desc, air_wt, s.cst_depot_air * desc->get_level());
		case maglev_wt:
			return tool_depot_t::tool_depot_aux(player, pos, desc, maglev_wt, s.cst_depot_rail * desc->get_level());
		case narrowgauge_wt:
			return tool_depot_t::tool_depot_aux(player, pos, desc, narrowgauge_wt, s.cst_depot_rail * desc->get_level());

		default:
			dbg->warning("tool_depot()","called with unknown desc %s",desc->get_name() );
			return "Unknown depot object";
	}
	return NULL;
}



/* builds (random) tourist attraction and maybe adds it to the next city
 * the parameter string is a follow:
 * 1#theater
 * first letter: ignore climates
 * second letter: rotation (0,1,2,3,#=random)
 * finally building name
 * @author prissi
 */
bool tool_build_house_t::init( player_t * )
{
	if (is_local_execution() && !strempty(default_param)) {
		const char *c = default_param+2;
		const building_tile_desc_t *tile = hausbauer_t::find_tile(c,0);
		if(tile!=NULL) {
			int rotation = (default_param[1]-'0') % tile->get_desc()->get_all_layouts();
			cursor_area = tile->get_desc()->get_size(rotation);
		}
	}
	return true;
}

const char *tool_build_house_t::work( player_t *player, koord3d pos )
{
	koord k(pos.get_2d());

	const grund_t* gr = welt->lookup_kartenboden(k);
	if(gr==NULL) {
		return "";
	}

	// Parsing parameter (if there)
	const building_desc_t *desc = NULL;
	if (!strempty(default_param)) {
		const char *c = default_param+2;
		const building_tile_desc_t *tile = hausbauer_t::find_tile(c,0);
		if(tile) {
			desc = tile->get_desc();
		}
	}
	else {
		desc = hausbauer_t::get_random_attraction( welt->get_timeline_year_month(), false, welt->get_climate( k ) );
	}

	if(desc==NULL) {
		return "";
	}
	int rotation;
	if(  !default_param || default_param[1]=='#'  ) {
		rotation = simrand(desc->get_all_layouts(), "const char *tool_build_house_t::work");
	}
	else if(  default_param[1]=='A'  ) {
		if(  desc->get_type()!=building_desc_t::attraction_land  &&  desc->get_type()!=building_desc_t::attraction_city  ) {
			// auto rotation only valid for city buildings
			int streetdir = 0;
			for(  int i = 1;  i < 8;  i+=2  ) {
				grund_t *gr2 = welt->lookup_kartenboden(k + koord::neighbours[i]);
				if(  gr2  &&  gr2->get_weg_hang() == gr2->get_grund_hang()  &&  gr2->get_weg(road_wt) != NULL  ) {
					// update directions - note this is SENW, conversion from neighbours to SENW is
					// neighbours SENW
					// 3          0
					// 5          1
					// 7          2
					// 1          3
					streetdir += (1 << (((i-3)/2)&3));
				}
			}
			rotation = building_layout[streetdir];
		}
		else {
			rotation = simrand(desc->get_all_layouts(), "const char *tool_build_house_t::work");
		}
	}
	else {
		rotation = (default_param[1]-'0') % desc->get_all_layouts();
	}

	koord size = desc->get_size(rotation);

	// process ignore climates switch
	climate_bits cl = (default_param  &&  default_param[0]=='1') ? ALL_CLIMATES : desc->get_allowed_climate_bits();

	bool hat_platz = welt->square_is_free( k, desc->get_x(rotation), desc->get_y(rotation), NULL, cl );
	if(!hat_platz  &&  size.y!=size.x  &&  desc->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#'  ||  default_param[1]=='A')) {
		// try other rotation too ...
		rotation = (rotation+1) % desc->get_all_layouts();
		hat_platz = welt->square_is_free( k, desc->get_x(rotation), desc->get_y(rotation), NULL, cl );
	}

	// Place found...
	if(hat_platz) {
		player_t *gb_player = desc->is_city_building() ? NULL : welt->get_public_player();
		gebaeude_t *gb = hausbauer_t::build(gb_player, gr->get_pos(), rotation, desc);
		if(gb) {
			// building successful
			// ought to be added to the city.
			stadt_t *city = welt->get_city( pos.get_2d() );
			welt->add_building_to_world_list(gb->access_first_tile());
			if(city) {
				city->add_gebaeude_to_stadt(gb->access_first_tile());
				city->reset_city_borders();
			}
			player_t::book_construction_costs(player, welt->get_settings().cst_multiply_remove_haus * desc->get_level() * size.x * size.y, k, gb->get_waytype());
			return NULL;
		}
	}
	return NOTICE_UNSUITABLE_GROUND;
}



// show industry size in cursor (in known)
bool tool_build_land_chain_t::init( player_t * )
{
	if (is_local_execution() && !strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		const factory_desc_t *fab = factory_builder_t::get_desc(c);
		if(fab==NULL) {
			// wrong tool!
			return false;
		}
		int rotation = (default_param[1]-'0') % fab->get_building()->get_all_layouts();
		cursor_area = fab->get_building()->get_size(rotation);
	}
	return true;
}

/* builds an (if param=NULL random) industry chain starting here *
 * the parameter string is a follow:
 * 1#34,oelfeld
 * first letter: ignore climates
 * second letter: rotation (0,1,2,3,#=random)
 * next number is production value
 * finally industry name
 */
const char *tool_build_land_chain_t::work( player_t *player, koord3d pos )
{
	const grund_t* gr = welt->lookup_kartenboden(pos.get_2d());
	if(gr==NULL) {
		return "";
	}

	const factory_desc_t *fab = NULL;
	if (!strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		fab = factory_builder_t::get_desc(c);
	}
	else {
		fab = factory_builder_t::get_random_consumer( false, (climate_bits)(1 << welt->get_climate( pos.get_2d() )), welt->get_timeline_year_month() );
	}

	if(fab==NULL) {
		return "";
	}
	int rotation = (default_param  &&  default_param[1]!='#') ? (default_param[1]-'0') % fab->get_building()->get_all_layouts() : simrand(fab->get_building()->get_all_layouts()-1, "const char *tool_build_land_chain_t::work");
	koord size = fab->get_building()->get_size(rotation);

	// process ignore climates switch
	bool ignore_climates = default_param  &&  default_param[0] == '1';
	climate_bits cl = ignore_climates ? ALL_CLIMATES : fab->get_building()->get_allowed_climate_bits();

	bool hat_platz = false;
	if(fab->get_placement()==factory_desc_t::Water) {
		// at sea
		hat_platz = welt->is_water( pos.get_2d(), fab->get_building()->get_size(rotation) );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_building()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_building()->get_all_layouts();
			hat_platz = welt->is_water( pos.get_2d(), fab->get_building()->get_size(rotation) );
		}
	}
	else {
		// and on solid ground
		hat_platz = welt->square_is_free( pos.get_2d(), fab->get_building()->get_x(rotation), fab->get_building()->get_y(rotation), NULL, cl );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_building()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_building()->get_all_layouts();
			hat_platz = welt->square_is_free( pos.get_2d(), fab->get_building()->get_x(rotation), fab->get_building()->get_y(rotation), NULL, cl );
		}
	}

	if(hat_platz) {
		// eventually adjust production
		sint32 initial_prod = -1;
		if (!strempty(default_param)) {
			sint32 value = atol(default_param + 2);
			initial_prod = welt->calc_adjusted_monthly_figure(value);
		}

		koord3d build_pos = gr->get_pos();
		int count = factory_builder_t::build_link(NULL, fab, initial_prod, rotation, &build_pos, welt->get_public_player(), 10000, ignore_climates);

		if(count>0) {
			// at least one factory has been built
			welt->get_viewport()->change_world_position( build_pos );
			player_t::book_construction_costs(player, count * welt->get_settings().cst_multiply_found_industry, build_pos.get_2d(), ignore_wt);

			// crossconnect all?
			if(welt->get_settings().is_crossconnect_factories())
			{
				FOR(vector_tpl<fabrik_t*>, factory, welt->get_fab_list())
				{
					factory->add_all_suppliers();
				}
			}
			return NULL;
		}
	}
	return NOTICE_UNSUITABLE_GROUND;
}


// show industry size in cursor (in known)
bool tool_city_chain_t::init( player_t * )
{
	if (is_local_execution() && !strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		const factory_desc_t *fab = factory_builder_t::get_desc(c);
		if(fab==NULL) {
			// wrong tool!
			return false;
		}
		int rotation = (default_param[1]-'0') % fab->get_building()->get_all_layouts();
		cursor_area = fab->get_building()->get_size(rotation);
	}
	return true;
}

/* builds a industry chain in the next town
 * defaukt_param see previous function
 */
const char *tool_city_chain_t::work( player_t *player, koord3d pos )
{
	const grund_t* gr = welt->lookup_kartenboden(pos.get_2d());
	if(gr==NULL) {
		return "";
	}

	const factory_desc_t *fab = NULL;
	if (!strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		fab = factory_builder_t::get_desc(c);
	}
	else {
		fab = factory_builder_t::get_random_consumer( false, (climate_bits)(1 << welt->get_climate( pos.get_2d() )), welt->get_timeline_year_month() );
	}

	if(fab==NULL) {
		return "";
	}

	// eventually adjust production
	sint32 initial_prod = -1;
	if (!strempty(default_param)) {
		sint32 value = atol(default_param+2);
		initial_prod = welt->calc_adjusted_monthly_figure(value);
	}

	pos = gr->get_pos();
	int count = factory_builder_t::build_link(NULL, fab, initial_prod, 0, &pos, welt->get_public_player(), 10000, false);
	if(count>0) {
		// at least one factory has been built
		welt->get_viewport()->change_world_position( pos );

		// crossconnect all?
		if(welt->get_settings().is_crossconnect_factories())
		{
			FOR(vector_tpl<fabrik_t*>, factory, welt->get_fab_list())
			{
				factory->add_all_suppliers();
			}
		}
		// ain't going to be cheap
		player_t::book_construction_costs(player, count * welt->get_settings().cst_multiply_found_industry, pos.get_2d(), ignore_wt);
		return NULL;
	}
	return NOTICE_UNSUITABLE_GROUND;
}



// show industry size in cursor (must be known!)
bool tool_build_factory_t::init( player_t * )
{
	if (is_local_execution() && !strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		const factory_desc_t *fab = factory_builder_t::get_desc(c);
		if(fab==NULL) {
			// wrong tool!
			return false;
		}
		int rotation = (default_param[1]-'0') % fab->get_building()->get_all_layouts();
		cursor_area = fab->get_building()->get_size(rotation);
		return true;
	}
	return true;
}

/* builds an industry next to the cursor (default_param see above) */
const char *tool_build_factory_t::work( player_t *player, koord3d pos )
{
	const grund_t* gr = welt->lookup_kartenboden(pos.get_2d());
	if(gr==NULL) {
		return "";
	}

	const factory_desc_t *fab = NULL;
	if (!strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		fab = factory_builder_t::get_desc(c);
	}
	else
	{
		fab = factory_builder_t::get_random_consumer( false, (climate_bits)(1<<welt->get_climate_at_height(gr->get_hoehe())), welt->get_timeline_year_month() );
//		fab = factory_builder_t::get_random_consumer( false, (climate_bits)(1 << welt->get_climate( pos.get_2d() )), welt->get_timeline_year_month() );
	}

	if(fab==NULL)
	{
		return "";
	}
	int rotation = (default_param  &&  default_param[1]!='#') ? (default_param[1]-'0') % fab->get_building()->get_all_layouts() : simrand(fab->get_building()->get_all_layouts(), "const char *tool_build_factory_t::work");
	koord size = fab->get_building()->get_size(rotation);

	// process ignore climates switch
	climate_bits cl = (default_param  &&  default_param[0] == '1') ? ALL_CLIMATES : fab->get_building()->get_allowed_climate_bits();

	bool hat_platz = false;
	if(fab->get_placement()==factory_desc_t::Water)
	{
		// at sea
		hat_platz = welt->is_water( pos.get_2d(), fab->get_building()->get_size(rotation) );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_building()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#'))
		{
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_building()->get_all_layouts();
			hat_platz = welt->is_water( pos.get_2d(), fab->get_building()->get_size(rotation) );
		}
	}
	else
	{
		// and on solid ground
		hat_platz = welt->square_is_free( pos.get_2d(), fab->get_building()->get_x(rotation), fab->get_building()->get_y(rotation), NULL, cl );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_building()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#'))
		{
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_building()->get_all_layouts();
			hat_platz = welt->square_is_free( pos.get_2d(), fab->get_building()->get_x(rotation), fab->get_building()->get_y(rotation), NULL, cl );
		}
	}

	if(hat_platz) {
		// eventually adjust production
		sint32 initial_prod = -1;
		if (!strempty(default_param)) {
			sint32 value = atol(default_param+2);
			initial_prod = welt->calc_adjusted_monthly_figure(value);
		}

		fabrik_t *f = factory_builder_t::build_factory(NULL, fab, initial_prod, rotation, gr->get_pos(), welt->get_public_player());
		if(f) {
			// at least one factory has been built
			welt->get_viewport()->change_world_position( pos );
			player_t::book_construction_costs(player, welt->get_settings().cst_multiply_found_industry, pos.get_2d(), ignore_wt);

			// crossconnect all?
			if(welt->get_settings().is_crossconnect_factories())
			{
				FOR(vector_tpl<fabrik_t*>, factory,  welt->get_fab_list())
				{
					factory->add_all_suppliers();
				}
			}
			return NULL;
		}
	}
	return NOTICE_UNSUITABLE_GROUND;
}



/**	link tool: links products of factory one with factory two (if possible)
 */
image_id tool_link_factory_t::get_marker_image()
{
	return cursor;
}


uint8 tool_link_factory_t::is_valid_pos( player_t *, const koord3d &pos, const char *&error, const koord3d & )
{
	fabrik_t *fab = fabrik_t::get_fab( pos.get_2d() );
	if (fab == NULL) {
		error = "";
		return 0;
	}
	return 2;
}


const char *tool_link_factory_t::do_work( player_t *, const koord3d &start, const koord3d &pos )
{
	fabrik_t *last_fab = fabrik_t::get_fab( start.get_2d() );
	fabrik_t *fab = fabrik_t::get_fab( pos.get_2d() );

	if(fab!=NULL  &&  last_fab!=NULL  &&  last_fab!=fab) {
		// It's a factory
		if(!is_ctrl_pressed()) {
			if(fab->add_supplier(last_fab) || last_fab->add_supplier(fab)) {
				//ok! they are connected
				return NULL;
			}
		}
		else {
			// remove connections
			fab->rem_supplier(last_fab->get_pos().get_2d());
			fab->rem_lieferziel(last_fab->get_pos().get_2d());
			last_fab->rem_supplier(fab->get_pos().get_2d());
			last_fab->rem_lieferziel(fab->get_pos().get_2d());
			return NULL;
		}
	}
	return "";
}


/* builds company headquarters
 * @author prissi
 */
const building_desc_t *tool_headquarter_t::next_level( const player_t *player ) const
{
	return hausbauer_t::get_headquarter(player->get_headquarters_level(), welt->get_timeline_year_month());
}

const char* tool_headquarter_t::get_tooltip(const player_t *player) const
{
	if (building_desc_t const* const desc = next_level(player)) {
		settings_t  const& s      = welt->get_settings();
		char const* const  tip    = player->get_headquarters_level() == 0 ? "build HQ" : "upgrade HQ";
		sint64      const  factor = (sint64)desc->get_level() * desc->get_x() * desc->get_y();
		return tooltip_with_price_maintenance(welt, tip, factor * s.cst_multiply_headquarter, factor * s.maint_building);
	}
	return NULL;
}

bool tool_headquarter_t::init( player_t *player )
{
	// do no use this, if there is no next level to build ...
	const building_desc_t *desc = next_level(player);
	if (desc) {
		if (is_local_execution()) {
			const int rotation = 0;
			cursor_area = desc->get_size(rotation);
		}
		return true;
	}
	return false;
}


const char *tool_headquarter_t::work( player_t *player, koord3d pos )
{
	bool ok=false;
	bool built = false;
DBG_MESSAGE("tool_headquarter()", "building headquarters at (%d,%d)", pos.x, pos.y);

	const building_desc_t* desc = next_level(player);
	if(desc==NULL) {
		// no further headquarters level
		dbg->message( "tool_headquarter()", "Already at maximum level!" );
		return "";
	}

	koord size = desc->get_size();
	sint64 const cost = welt->get_settings().cst_multiply_headquarter * desc->get_level() * size.x * size.y;
	if(! player_t::can_afford(player, -cost) ) {
		return NOTICE_INSUFFICIENT_FUNDS;
	}


	const koord& k(pos.get_2d());

	grund_t *gr = welt->lookup_kartenboden(k);

	if(gr) {
		gebaeude_t *hq = NULL;
		// check for current head quarter
		koord previous = player->get_headquarter_pos();
		if(previous!=koord::invalid) {
			grund_t *gr_hq = welt->lookup_kartenboden(previous);
			gebaeude_t *prev_hq = gr_hq->find<gebaeude_t>();
			// check if upgrade should be built at same place as current one
			gebaeude_t *gb = gr->find<gebaeude_t>();
			if (gb  &&  gb->get_owner()==player  &&  prev_hq->get_tile()->get_desc()==gb->get_tile()->get_desc()) {
				const building_desc_t* prev_desc = prev_hq->get_tile()->get_desc();
				// check if sizes fit
				uint8 prev_layout = prev_hq->get_tile()->get_layout();
				uint8 layout =  prev_layout % desc->get_all_layouts();
				koord size = desc->get_size(layout);
				if (prev_desc->get_size(prev_layout) == size) {
					// check for same tile structure
					ok = true;
					for (sint16 x=0; x<size.x  &&  ok; x++) {
						for (sint16 y=0; y<size.y  &&  ok; y++) {
							ok = (prev_desc->get_tile(prev_layout, x, y)==NULL)==(desc->get_tile(layout, x, y)==NULL);
						}
					}
					hq = gb;
					if( ok )
					{
						welt->remove_building_from_world_list(gb);
						// upgrade the tiles
						koord k_hq = k - gb->get_tile()->get_offset();
						for(  sint16 x = 0;  x < size.x;  x++  ) {
							for(  sint16 y = 0;  y < size.y;  y++  ) {
								if(  const building_tile_desc_t *tile = desc->get_tile(layout, x, y)  )
								{
									if(  grund_t *gr2 = welt->lookup_kartenboden(k_hq + koord(x, y))  )
									{
										if(  gebaeude_t *gb = gr2->find<gebaeude_t>()  ) {
											if(  gb  &&  gb->get_owner() == player  &&  prev_desc == gb->get_tile()->get_desc()  )
											{
												player_t::add_maintenance( player, -prev_desc->get_maintenance(), prev_desc->get_finance_waytype() );
												gb->set_tile( tile, true );
												player_t::add_maintenance( player, desc->get_maintenance(), desc->get_finance_waytype() );
											}
										}
									}
								}
							}
						}
						built = true;
						welt->add_building_to_world_list(hq->access_first_tile());
					}

				}
			}
			// did not upgrade old one, need to remove it
			if( !built ) {
				// remove previous one
				hausbauer_t::remove( player, prev_hq, false );
				// resize cursor
				init(player);
			}
		}


		// build new one
		if (!built) {
			int rotate = 0;

			if(welt->square_is_free(k, size.x, size.y, NULL, desc->get_allowed_climate_bits())) {
				ok = true;
			}
			if(!ok  &&  desc->get_all_layouts()>1  &&  size.y != size.x  &&  welt->square_is_free(k, size.y, size.x, NULL, desc->get_allowed_climate_bits())) {
				rotate = 1;
				ok = true;
			}

			if(ok) {
				// then build it
				hq = hausbauer_t::build(player, gr->get_pos(), rotate, desc, NULL);
				stadt_t *city = welt->get_city( pos.get_2d() );
				if(city) {
					city->add_gebaeude_to_stadt(hq);
					welt->add_building_to_world_list(hq->access_first_tile());
					city->reset_city_borders();
				}
				built = true;
			}
			else {
				return NOTICE_UNSUITABLE_GROUND;
			}
		}


		if( built ) {
			// sometimes those are not correct after rotation ...
			player->add_headquarter( desc->get_extra() + 1, hq->get_pos().get_2d() - hq->get_tile()->get_offset() );
			player_t::book_construction_costs(player,  cost, k, ignore_wt);
			// tell the world of it ...
			cbuffer_t buf;
			buf.printf( translator::translate("%s s\nheadquarter now\nat (%i,%i)."), player->get_name(), pos.x, pos.y );
			welt->get_message()->add_message( buf, k, message_t::ai, PLAYER_FLAG|player->get_player_nr(), hq->get_tile()->get_background(0,0,0) );
			// reset to query tool, since costly relocations should be avoided
			if(is_local_execution()  &&  player == welt->get_active_player()) {
				welt->set_tool( tool_t::general_tool[TOOL_QUERY], player );
			}
			return NULL;
		}
	}
	return "";
}

const char *tool_lock_game_t::work( player_t *, koord3d )
{
	// tool can never be executed in network mode
	if (env_t::networkmode) {
		return "";
	}
	// as the result depends on the local locked state of public player
	if (welt->get_public_player()->is_locked() || !welt->get_settings().get_allow_player_change()) {
		return "Only public player can lock games!";
	}
	welt->clear_player_password_hashes();
	if(  !welt->get_public_player()->is_locked() ) {
		return "In order to lock the game, you have to protect the public player by password!";
	}
	welt->get_settings().set_allow_player_change(false);
	destroy_all_win( true );
	welt->switch_active_player( 0, true );
	welt->set_tool( general_tool[TOOL_QUERY], welt->get_player(0) );
	return NULL;
}


const char *tool_add_citycar_t::work( player_t *player, koord3d pos )
{
	if( private_car_t::list_empty() ) {
		// No citycar
		return "";
	}
	grund_t *gr = tool_intern_koord_to_weg_grund( player, welt, pos, road_wt );

	if(  gr != NULL  &&  ribi_t::is_twoway(gr->get_weg_ribi_unmasked(road_wt))  &&  gr->find<private_car_t>() == NULL) {
		// add citycar
		private_car_t* vt = new private_car_t(gr, koord::invalid);
		gr->obj_add(vt);
		welt->sync.add(vt);
		return NULL;
	}
	return "";
}


uint8 tool_forest_t::is_valid_pos( player_t *, const koord3d &, const char *&, const koord3d & )
{
	// do really nothing ...
	return 2;
}


void tool_forest_t::mark_tiles(  player_t *, const koord3d &start, const koord3d &end )
{
	koord k1, k2;
	k1.x = start.x < end.x ? start.x : end.x;
	k1.y = start.y < end.y ? start.y : end.y;
	k2.x = start.x + end.x - k1.x;
	k2.y = start.y + end.y - k1.y;
	koord k;
	for(  k.x = k1.x;  k.x <= k2.x;  k.x++  ) {
		for(  k.y = k1.y;  k.y <= k2.y;  k.y++  ) {
			grund_t *gr = welt->lookup_kartenboden( k );

			zeiger_t *marker = new zeiger_t(gr->get_pos(), NULL );

			const uint8 grund_hang = gr->get_grund_hang();
			const uint8 weg_hang = gr->get_weg_hang();
			const uint8 hang = max( corner_sw(grund_hang), corner_sw(weg_hang)) +
					3 * max( corner_se(grund_hang), corner_se(weg_hang)) +
					9 * max( corner_ne(grund_hang), corner_ne(weg_hang)) +
					27 * max( corner_nw(grund_hang), corner_nw(weg_hang));
			uint8 back_hang = (hang % 3) + 3 * ((uint8)(hang / 9)) + 27;
			marker->set_after_image( ground_desc_t::marker->get_image( grund_hang % 27 ) );
			marker->set_image( ground_desc_t::marker->get_image( back_hang ) );

			marker->mark_image_dirty( marker->get_image(), 0 );
			gr->obj_add( marker );
			marked.insert( marker );
		}
	}
}


const char *tool_forest_t::do_work( player_t *player, const koord3d &start, const koord3d &end )
{
	koord wh, nw;
	wh.x = abs(end.x-start.x)+1;
	wh.y = abs(end.y-start.y)+1;
	nw.x = min(start.x, end.x)+(wh.x/2);
	nw.y = min(start.y, end.y)+(wh.y/2);

	sint64 costs = baum_t::create_forest( nw, wh );
	player_t::book_construction_costs(player, costs * welt->get_settings().cst_remove_tree, end.get_2d(), ignore_wt);

	return NULL;
}


image_id tool_stop_moving_t::get_marker_image()
{
	return cursor;
}


void tool_stop_moving_t::read_start_position(player_t *player, const koord3d &pos)
{
	waytype[0] = invalid_wt;
	waytype[1] = invalid_wt;
	last_halt = halthandle_t();

	grund_t *bd = welt->lookup(pos);
	if (bd==NULL) {
		return;
	}
	// now assign waytypes
	if(bd->is_water()) {
		waytype[0] = water_wt;
	}
	else {
		waytype[0] = bd->get_weg_nr(0)->get_waytype();
		if(bd->get_weg_nr(1)) {
			waytype[1] = bd->get_weg_nr(1)->get_waytype();
		}
	}
	// .. and halt
	last_halt = haltestelle_t::get_halt(pos,player);
}


uint8 tool_stop_moving_t::is_valid_pos(  player_t *player, const koord3d &pos, const char *&error, const koord3d &start)
{
	grund_t *bd = welt->lookup(pos);
	if (bd==NULL) {
		error = "";
		return 0;
	}
	// check halt ownership
	halthandle_t h = haltestelle_t::get_halt(pos,player);
	if(  h.is_bound()  &&  !player_t::check_owner( player, h->get_owner() )  ) {
		error = "Das Feld gehoert\neinem anderen Spieler\n";
		return 0;
	}
	// check for halt on the tile
	if(  h.is_bound()  &&  !(bd->is_halt()  ||  (h->get_station_type()&haltestelle_t::dock  &&  bd->is_water())  )  ) {
		error = NOTICE_UNSUITABLE_GROUND;
		return 0;
	}

	if (start==koord3d::invalid) {
		// check for existing ways
		if (bd->is_water()  ||  bd->hat_wege()) {
			return 2;
		}
		else {
			error = NOTICE_UNSUITABLE_GROUND;
			return 0;
		}
	}
	else {
		// read conditions at start point
		read_start_position(player, start);
		// check halts vs waypoints
		if(h.is_bound() ^ last_halt.is_bound()) {
			error = "Can only move from halt to halt or waypoint to waypoint.";
			return 0;
		}
		// check waytypes
		if(  (waytype[0] == water_wt  &&  bd->is_water())  ||  bd->hat_weg(waytype[0])  ||  bd->hat_weg(waytype[1])  ) {
			// ok
			return 2;
		}
		else {
			error = NOTICE_UNSUITABLE_GROUND;
			return 0;
		}
	}
}

const char *tool_stop_moving_t::do_work( player_t *player, const koord3d &last_pos, const koord3d &pos)
{
	// read conditions at start point
	read_start_position(player, last_pos);

	// second click
	grund_t *bd = welt->lookup(pos);
	halthandle_t h = haltestelle_t::get_halt(pos,player);

	if (bd) {
		const halthandle_t new_halt = h;
		// depending on the waytype we simply build replacements lists
		// in the worst case we have to iterate over all tiles twice ...
		for(  uint i=0;  i<2;  i++  ) {
			const waytype_t wt = waytype[i];
			slist_tpl <koord3d>old_platform;

			if(bd->is_water()) {
				if(wt!=water_wt) {
					break;
				}
			}
			else if(!bd->hat_weg(wt)) {
				continue;
			}
			// platform, stop or just tile moving?
			const bool catch_all_halt = (wt==water_wt  ||  wt==air_wt)  &&  last_halt.is_bound();
			if(!last_halt.is_bound()) {
				old_platform.append(last_pos);
			}
			else if(!catch_all_halt) {
				// builds a coordinate list
				if(wt==road_wt) {
					old_platform.append(last_pos);
				}
				else {
					// all connected tiles for start pos
					uint8 ribi = welt->lookup(last_pos)->get_weg_ribi_unmasked(wt);
					koord delta = ribi_t::is_straight_ns(ribi) ? koord(0,1) : koord(1,0);
					koord3d start_pos=last_pos;
					while(ribi&12) {
						koord3d test_pos = start_pos+delta;
						grund_t *gr = welt->lookup(test_pos);
						if(!gr  ||  !gr->is_halt()  ||  (ribi=gr->get_weg_ribi_unmasked(wt))==0) {
							break;
						}
						start_pos = test_pos;
					}
					// now add all of them
					while(ribi&3) {
						koord3d test_pos = start_pos-delta;
						grund_t *gr = welt->lookup(test_pos);
						old_platform.append(start_pos);
						if(!gr  ||  !gr->is_halt()  ||  (ribi=gr->get_weg_ribi_unmasked(wt))==0) {
							break;
						}
						start_pos = test_pos;
					}
				}
			}

			// first, check convoi without line
			FOR(vector_tpl<convoihandle_t>, const cnv, welt->convoys()) {
				// check line and owner
				if(!cnv->get_line().is_bound()  &&  cnv->get_owner()==player) {
					schedule_t *schedule = cnv->get_schedule();
					// check waytype
					if(schedule  &&  schedule->is_stop_allowed(bd)) {
						bool updated = false;
						FOR(minivec_tpl<schedule_entry_t>, & k, schedule->entries) {
							if ((catch_all_halt && haltestelle_t::get_halt( k.pos, cnv->get_owner()) == last_halt) ||
									old_platform.is_contained(k.pos)) {
								k.pos   = pos;
								updated = true;
							}
						}
						if(updated) {
							schedule->cleanup();
							// Knightly : remove lineless convoy from old stop
							if(  last_halt.is_bound()  ) {
								last_halt->remove_convoy(cnv);
							}
							// Knightly : register lineless convoy at new stop
							if(  new_halt.is_bound()  ) {
								new_halt->add_convoy(cnv);
							}
							if(  !schedule->is_editing_finished()  ) {
								// schedule is not owned by schedule window ...
								// ... thus we can set this schedule
								cnv->set_schedule(schedule);
								// otherwise the schedule window will reset it
							}
						}
					}
				}
			}
			// next, check lines serving old_halt (no owner check needed for own lines ...
			vector_tpl<linehandle_t>lines;
			player->simlinemgmt.get_lines(simline_t::line,&lines);
			FOR(vector_tpl<linehandle_t>, const line, lines) {
				schedule_t *schedule = line->get_schedule();
				// check waytype
				if(schedule->is_stop_allowed(bd)) {
					bool updated = false;
					FOR(minivec_tpl<schedule_entry_t>, & k, schedule->entries) {
						// ok!
						if ((catch_all_halt && haltestelle_t::get_halt( k.pos, line->get_owner()) == last_halt) ||
								old_platform.is_contained(k.pos)) {
							k.pos   = pos;
							updated = true;
						}
					}
					// update line
					if(updated) {
						schedule->cleanup();
						// remove line from old stop is needed at here
						if(last_halt.is_bound()) {
							last_halt->remove_line(line);
						}
						simlinemgmt_t::update_line(line);
					}
				}
			}
		}
		// since factory connections may have changed

		// Modified by : Knightly
#ifdef MULTI_THREAD
		world()->stop_path_explorer();
#endif
		path_explorer_t::refresh_all_categories(true);
	}
	return NULL;
}

image_id tool_reassign_signal_t::get_marker_image()
{
	return cursor;
}

void tool_reassign_signal_t::read_start_position(const koord3d &pos)
{
	last_selected_location = pos;
}

uint8 tool_reassign_signal_t::is_valid_pos(player_t *player, const koord3d &pos, const char *&error, const koord3d &start)
{
	if(start == koord3d::invalid)
	{
		// First click
		return welt->lookup(pos) == NULL ? 0 : 2;
	}

	grund_t *gr = welt->lookup(pos);
	grund_t *gr_start = welt->lookup(start);
	if(!gr || !gr_start)
	{
		error = "";
		return 0;
	}

	// The end tile must be a signalbox. The start tile may either be a signal or a signalbox.

	// Check the end tile first.
	gebaeude_t* gb = gr->get_building();
	if(gb)
	{
		gb = gb->access_first_tile();
	}
	const signalbox_t* sb_end;
	if(gb && gb->get_tile()->get_desc()->is_signalbox())
	{
		sb_end = (signalbox_t*)gb;
		if(!(sb_end->get_owner() == player))
		{
			error = "Cannot transfer signals to a signalbox belonging to another player.";
			return 0;
		}
			if(!sb_end->can_add_more_signals())
		{
			error = "Cannot transfer any signals to this signalbox: it does not have any spare capacity for more signals.";
			return 0;
		}
	}
	else
	{
		return 0;
	}

	bool is_valid_start = false;

	gb = gr_start->get_building();
	if(gb)
	{
		gb = gb->access_first_tile();
	}
	if(gb && gb->get_tile()->get_desc()->is_signalbox())
	{
		const signalbox_t* sb_start = (signalbox_t*)gb;
		if(sb_start->get_owner() != player)
		{
			error = "Cannot transfer signals from a signalbox belonging to another player.";
			return 0;
		}
		else if(!(sb_start->get_tile()->get_desc()->get_clusters() & sb_end->get_tile()->get_desc()->get_clusters()))
		{
			error = "Cannot transfer any signals between these signalboxes because none of the signals will be compatible.";
			return 0;
		}
		else
		{
			is_valid_start = true;
		}
	}

	const signal_t* sig = welt->lookup(start)->find<signal_t>();
	is_valid_start |= sig && sb_end->can_add_signal(sig);
	if(sig && !is_valid_start)
	{
		error = "This signal is not compatible with this signalbox.";
	}

	if(!sig && !sb_end)
	{
		error = "";
	}

	if(is_valid_start)
	{
		return 2;
	}
	else
	{
		return 0;
	}
}

const char *tool_reassign_signal_t::do_work( player_t *player, const koord3d &last_pos, const koord3d &pos)
{
	// read conditions at start point
	read_start_position(last_pos);

	if(pos == koord3d::invalid)
	{
		return "";
	}

	// second click
	grund_t* gr_end = welt->lookup(pos);
	gebaeude_t* gb_end = gr_end->get_building();
	if(gb_end)
	{
		gb_end = gb_end->access_first_tile();
	}
	signalbox_t* sb_end = NULL;
	if(gb_end && gb_end->get_tile()->get_desc()->is_signalbox())
	{
		sb_end = (signalbox_t*)gb_end;
	}

	if(!sb_end)
	{
		return "Cannot transfer signals: unknown error.";
	}

	grund_t* gr_start = welt->lookup(last_pos);
	gebaeude_t* gb_start = gr_start->get_building();
	if(gb_start)
	{
		gb_start = gb_start->access_first_tile();
	}
	signalbox_t* sb_start = NULL;
	if(gb_start && gb_start->get_tile()->get_desc()->is_signalbox())
	{
		sb_start = (signalbox_t*)gb_start;
		koord succeed_fail = sb_start->transfer_all_signals(sb_end);
		if(succeed_fail.y)
		{
			return "Not all signals were transferred successfully.";
		}
		else
		{
			return "";
		}

		// For some reason, this produces a mangled error message.
		/*char buf[256];
		sprintf(buf, "%s: %d\n%s: %d", "Signals transferred successfully", succeed_fail.x, "Signals not transferred", succeed_fail.y);
		return buf; */
	}


	signal_t* sig = welt->lookup(last_pos)->find<signal_t>();
	if(sig)
	{
		koord3d sb_location = sig->get_signalbox();
		grund_t* gr_sb = welt->lookup(sb_location);
		gebaeude_t* building_x = gr_sb->get_building();
		gebaeude_t* building_sb = building_x ? building_x->access_first_tile() : NULL;
		signalbox_t* sb = NULL;
		if(sb_end && building_sb && building_sb->get_tile()->get_desc()->is_signalbox())
		{
			sb = (signalbox_t*)building_sb;
			if(sb_end->transfer_signal(sig, sb))
			{
				return "";
			}
		}
		else if (sb_end)
		{
			// It is theoretically possible that a player might transfer a signal
			// not assigned to a signalbox to a signalbox. Account for this here.
			if (sb_end->add_signal(sig))
			{
				return "";
			}
		}
	}

	return "Cannot transfer signals: unknown error.";
}


char const* tool_daynight_level_t::get_tooltip(player_t const*) const
{
	if (!strempty(default_param)) {
		if(default_param[0]=='+'  ||  default_param[0]=='-') {
			sprintf(toolstr, "%s %s",
			translator::translate("1LIGHT_CHOOSE"),
			&default_param[0]);
			return toolstr;
		}
		else {
			return translator::translate("Toggle day/night view");
		}
	}
	else {
		return "";
	}
}

bool tool_daynight_level_t::init( player_t * ) {
	if(grund_t::underground_mode==grund_t::ugm_all  ||  env_t::night_shift) {
		return false;
	}
	if (!strempty(default_param)) {
		if(default_param[0]=='+'  &&  env_t::daynight_level > 0) {
			// '+': fade in one level
			env_t::daynight_level = env_t::daynight_level-1;
		}
		else if (default_param[0]=='-') {
			// '-': fade out one level
			env_t::daynight_level = env_t::daynight_level+1;
		}
		else {
			// number: toggle number/0. 4 or 5 is good for night
			const sint8 level = atoi(default_param);
			env_t::daynight_level = (env_t::daynight_level==0) ? level : 0;
		}
	}
	return false;
}



/* make all tiles of this player a public stop
 * if this player is public, make all connected tiles a public stop */
bool tool_make_stop_public_t::init( player_t *player )
{
	win_set_static_tooltip( NULL );
	return welt->get_settings().get_allow_making_public() || player && player->is_public_service();
}

const char *tool_make_stop_public_t::get_tooltip(const player_t *player) const
{
	if(player->get_player_nr() != 1 && !welt->get_settings().get_allow_making_public())
	{
		sprintf(toolstr, translator::translate("Only %s can use this tool"), welt->get_public_player()->get_name());
		return toolstr;
	}
	sprintf(toolstr, translator::translate("Make way or stop public (will join with neighbours), %i times maintainance"), welt->get_settings().cst_make_public_months);
	return toolstr;
}

const char *tool_make_stop_public_t::move(player_t *player, uint16, koord3d p)
{
	// queue tool for network
	if (env_t::networkmode) {
		nwc_tool_t *nwc = new nwc_tool_t(player, this, p, welt->get_steps(), welt->get_map_counter(), false);
		network_send_server(nwc);
		return NULL;
	}

	return work(player, p);
}

const char *tool_make_stop_public_t::work( player_t *player, koord3d p )
{
	sint64 const COST_MONTHS_MAINTAINANCE = welt->calc_adjusted_monthly_figure(welt->get_settings().cst_make_public_months);
	player_t *const psplayer = welt->get_public_player();
	grund_t const *gr = welt->lookup(p);
	if (!gr || !gr->get_halt().is_bound() || gr->get_halt()->get_owner() == psplayer) {
		weg_t *w = NULL;

		if (player != psplayer  &&  welt->get_settings().get_disable_make_way_public()) {
			return NOTICE_DISABLED_PUBLIC_WAY;

		}
		//convert a way here, if there is no halt or already public halt
		{
			// The below is only really relevant in Standard, where ordinary players
			// are allowed to use this tool and can thus pass off maintenance
			// of expensive bridges to the public player.
			/*if (gr->get_typ() == grund_t::brueckenboden) {
				// not making ways public on bridges
				return NOTICE_UNSUITABLE_GROUND;
			}*/
			w = gr->get_weg_nr(0);
			if(  !(w  &&  (  (w->get_owner()==player)  |  (player== psplayer) )) ) {
				w = gr->get_weg_nr(1);
				if(  !(w  &&  (  (w->get_owner()==player)  |  (player== psplayer) )) ) {
					w = NULL;
				}
			}
			if(  w  ) {
				// no public way with signs
				if(  w->has_sign()  ) {
					return NOTICE_UNSUITABLE_GROUND;
				}
				// change maintenance and ownership
				sint64 maintenance_cost = w->get_desc()->get_maintenance();
				sint64 construction_cost;
				if(player == psplayer)
				{
					construction_cost = w->get_desc()->get_value();
				}
				else
				{
					construction_cost = welt->calc_adjusted_monthly_figure(maintenance_cost * welt->get_settings().cst_make_public_months);
				}

#ifdef MULTI_THREAD
				if (env_t::networkmode)
				{
					// In network mode, we cannot have anything that alters a way running concurrently with
					// convoy path-finding because  whether the convoy path-finder is called
					// on this tile of way before or after this function is indeterminate.
					if (!world()->get_first_step())
					{
						simthread_barrier_wait(&karte_t::step_convoys_barrier_external);
						welt->set_first_step(1);
					}
				}
#endif

				if(gr->ist_im_tunnel())
				{
					tunnel_t *t = gr->find<tunnel_t>();
					maintenance_cost = t->get_desc()->get_maintenance();
					if(player == psplayer)
					{
						construction_cost = t->get_desc()->get_value();
					}
					else
					{
						construction_cost = welt->calc_adjusted_monthly_figure(maintenance_cost * welt->get_settings().cst_make_public_months);
					}
					if(t->get_owner() == psplayer)
					{
						t->set_owner(NULL);
					}
					else
					{
						t->set_owner(psplayer);
					}
				}

				if(gr->ist_bruecke())
				{
					bruecke_t* b = gr->find<bruecke_t>();
					maintenance_cost = b->get_desc()->get_maintenance();
					if(player == psplayer)
					{
						construction_cost = b->get_desc()->get_value();
					}
					else
					{
						construction_cost = welt->calc_adjusted_monthly_figure(maintenance_cost * welt->get_settings().cst_make_public_months);
					}
					if(b->get_owner() == psplayer)
					{
						b->set_owner(NULL);
					}
					else
					{
						b->set_owner(psplayer);
					}
				}

				if(w->get_owner() == psplayer)
				{
					w->set_owner(NULL);
				}
				else
				{
					player_t::add_maintenance(w->get_owner(), -maintenance_cost, w->get_desc()->get_finance_waytype());
					player_t::book_construction_costs(player, -construction_cost, gr->get_pos().get_2d(), w->get_desc()->get_finance_waytype());
					if(player == psplayer)
					{
						// If this is done by the public player, pay compensation.
						player_t::book_construction_costs(w->get_owner(), construction_cost, gr->get_pos().get_2d(), w->get_desc()->get_finance_waytype());
					}
					w->set_owner(psplayer);
					player_t::add_maintenance(psplayer, maintenance_cost);
				}

				w->set_flag(obj_t::dirty);
				// now search for wayobjects
				for(uint8 i = 1; i < gr->get_top(); i++)
				{
					if(wayobj_t *wo = obj_cast<wayobj_t>(gr->obj_bei(i)))
					{
						maintenance_cost = wo->get_desc()->get_maintenance();

						player_t::add_maintenance(player, -maintenance_cost, w->get_desc()->get_finance_waytype() );

						if(player == psplayer)
						{
							// If this is done by the public player, pay compensation.
							construction_cost = wo->get_desc()->get_value();
							player_t::book_construction_costs(wo->get_owner(), construction_cost, gr->get_pos().get_2d(), w->get_desc()->get_finance_waytype());
						}
						else
						{
							// Price is based on 60 months' maintenance
							construction_cost = maintenance_cost * 60;
						}
						player_t::book_construction_costs(player, -construction_cost, gr->get_pos().get_2d(), w->get_waytype());
						wo->set_owner(psplayer);
						wo->set_flag(obj_t::dirty);
						player_t::add_maintenance(psplayer, maintenance_cost, w->get_desc()->get_finance_waytype());
					}
				}
				// and add message
				if(/*player->get_player_nr()!=1  &&*/env_t::networkmode)
				{
					cbuffer_t buf;
					buf.printf(translator::translate("(%s) now public way."), w->get_pos().get_str());
					welt->get_message()->add_message(buf, w->get_pos().get_2d(), message_t::ai, PLAYER_FLAG|player->get_player_nr(), IMG_EMPTY);
				}
			}
		}
		if(  w==NULL  ) {
			return "No stop here!";
		}
	}
	else
	{
		halthandle_t halt = gr->get_halt();
		if(player != psplayer && !(player_t::check_owner(halt->get_owner(), player) || halt->get_owner() == psplayer))
		{
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		else
		{
			if(!halt->make_public_and_join(player))
			{
				return NOTICE_INSUFFICIENT_FUNDS;
			}
		}
	}
	return NULL;
}



bool tool_show_trees_t::init( player_t * )
{
	env_t::hide_trees = !env_t::hide_trees;
	baum_t::recalc_outline_color();
	welt->set_dirty();
	return false;
}

sint8 tool_show_underground_t::save_underground_level = -128;

bool tool_show_underground_t::init( player_t * )
{
	koord3d zpos = welt->get_zeiger()->get_pos();
	// move zeiger (pointer) to invalid position -> unmark tiles
	welt->get_zeiger()->change_pos( koord3d::invalid);

	sint8 old_underground_level = grund_t::underground_level;

	// map needs update?
	bool ok = true;
	// need an extra click?
	bool needs_click = false;

	// default default-param = U for backward compatibility
	if (default_param == NULL) {
		default_param = strdup("U");
	}
	// now check the default parameter
	switch(default_param[0]) {
		// toggle sliced view by toolbar - height taken from extra mouse click
		case 'C':
			if(grund_t::underground_mode==grund_t::ugm_level) {
				grund_t::set_underground_mode( grund_t::ugm_none, 0);
			}
			else if(grund_t::underground_mode==grund_t::ugm_none) {
				needs_click = true;
				ok = false;
			}
			else {
				ok = false;
			}
			break;
		// decrease slice level
		case 'D':
			if (grund_t::underground_mode == grund_t::ugm_level) {
				if (grund_t::underground_level > welt->min_height) {
					grund_t::underground_level--;
				}
			}
			else {
				ok = false;
			}
			break;
			// increase slice level
		case 'I':
			if (grund_t::underground_mode == grund_t::ugm_level) {
				if (grund_t::underground_level < welt->max_height) {
					grund_t::underground_level++;
				}
			}
			else {
				ok = false;
			}
			break;

		// toggle sliced view by keyboard - height taken from cursor
		case 'K':
			if(grund_t::underground_mode==grund_t::ugm_level) {
				// switch to normal or full-underground
				grund_t::set_underground_mode( grund_t::ugm_none, 0);
			}
			else if(grund_t::underground_mode==grund_t::ugm_none) {
				grund_t::set_underground_mode( grund_t::ugm_level, zpos.z);
			}
			else {
				ok = false;
			}
			break;

		//  switch between full underground or normal/sliced view
		case 'U':
			if (grund_t::underground_mode==grund_t::ugm_all) {
				// check if the old level is valid then switch back to sliced view
				if (-128<save_underground_level && save_underground_level<127) {
					grund_t::set_underground_mode(grund_t::ugm_level, save_underground_level);
				}
				else {
					grund_t::set_underground_mode(grund_t::ugm_none, 0);
				}
			}
			else {
				grund_t::set_underground_mode( grund_t::ugm_all, 0);
			}
			break;

		default:
			ok = false;
			dbg->error( "tool_show_underground_t::init()", "Unknown command string \"%s\"", default_param );

	}

	// move zeiger (pointer) back
	welt->get_zeiger()->change_pos( zpos);

	if (ok) {
		save_underground_level = old_underground_level;

		// renew toolbar
		tool_t::update_toolbars();

		// recalc all images on map
		welt->update_map();
	}
	return needs_click;
}

const char *tool_show_underground_t::work( player_t *player, koord3d pos)
{
	koord3d zpos = welt->get_zeiger()->get_pos();
	// move zeiger (pointer) to invalid position -> unmark tiles
	welt->get_zeiger()->change_pos( koord3d::invalid);

	save_underground_level = grund_t::underground_level;
	grund_t::set_underground_mode( grund_t::ugm_level, pos.z);

	// move zeiger (pointer) back
	welt->get_zeiger()->change_pos( zpos);

	// renew toolbar
	tool_t::update_toolbars();

	// recalc all images on map
	welt->update_map();

	if(player == welt->get_active_player()) {
		welt->set_tool( general_tool[TOOL_QUERY], player );
	}

	return NULL;
}


char const* tool_show_underground_t::get_tooltip(player_t const*) const
{
	// no default-param == U for backward compatibility
	if(  default_param == NULL  ) {
		return translator::translate("underground mode");
	}
	// now check the default parameter
	switch(default_param[0]) {
		// toggle sliced view by toolbar - height taken from extra mouse click
		case 'C':
			return translator::translate("sliced underground mode");
		// decrease slice level
		case 'D':
			return translator::translate("decrease underground view level");
		// increase slice level
		case 'I':
			return translator::translate("increase underground view level");
		// toggle sliced view by keyboard - height taken from cursor
		case 'K':
			return translator::translate("sliced underground mode");
		//  switch between full underground or normal/sliced view
		case 'U':
		default:
			return translator::translate("underground mode");
	}
}

bool tool_show_underground_t::is_selected() const
{
	// default default-param = U for backward compatibility
	if(  default_param == NULL  ) {
		return grund_t::underground_mode==grund_t::ugm_all;
	}
	// now check the default parameter
	switch(default_param[0]) {
		// toggle sliced view by toolbar - height taken from extra mouse click
		case 'C':
			return grund_t::underground_mode==grund_t::ugm_level;
		// decrease slice level
		case 'D':
			return false;
		// increase slice level
		case 'I':
			return false;
		// toggle sliced view by keyboard - height taken from cursor
		case 'K':
			return grund_t::underground_mode==grund_t::ugm_level;
		//  switch between full underground or normal/sliced view
		case 'U':
			return grund_t::underground_mode==grund_t::ugm_all;
	}
	return false;
}

void tool_show_underground_t::draw_after(scr_coord k, bool dirty) const
{
	if(  icon!=IMG_EMPTY  &&  is_selected()  ) {
		display_img_blend( icon, k.x, k.y, TRANSPARENT50_FLAG|OUTLINE_FLAG|COL_BLACK, false, dirty );
		// additionally show level in sliced mode
		if(  default_param!=NULL  &&  grund_t::underground_mode==grund_t::ugm_level  ) {
			char level_str[16];
			sprintf( level_str, "%i", grund_t::underground_level );
			display_proportional( k.x+4, k.y+4, level_str, ALIGN_LEFT, COL_YELLOW, true );
		}
	}
}


bool tool_rotate90_t::init( player_t * )
{
	if(  !env_t::networkmode  ) {
		welt->rotate90();
		welt->update_map();
	}
	return false;
}


bool tool_quit_t::init( player_t * )
{
	destroy_all_win( true );
	welt->stop( true );
	return false;
}


bool tool_screenshot_t::init( player_t * )
{
	if(  is_ctrl_pressed()  ) {
		if(  const gui_frame_t * topwin = win_get_top()  ) {
			const scr_coord k = win_get_pos(topwin);
			const scr_size size = topwin->get_windowsize();
			display_snapshot( k.x, k.y, size.w, size.h );
		}
		else {
			display_snapshot( 0, 0, display_get_width(), display_get_height() );
		}
	}
	else {
		display_snapshot( 0, 0, display_get_width(), display_get_height() );
	}
	create_win( new news_img("Screenshot\ngespeichert.\n"), w_time_delete, magic_none);
	return false;
}


bool tool_undo_t::init( player_t *player )
{
	if(!player->undo()  &&  is_local_execution()) {
		create_win( new news_img("UNDO failed!"), w_time_delete, magic_none);
	}
	return false;
}


bool tool_increase_industry_t::init( player_t * )
{
	factory_builder_t::increase_industry_density( true, false );
	return false;
}


bool tool_zoom_in_t::init( player_t * )
{
	win_change_zoom_factor(true);
	welt->set_dirty();
	return false;
}


bool tool_zoom_out_t::init( player_t * )
{
	win_change_zoom_factor(false);
	welt->set_dirty();
	return false;
}

/************************* internal tools, only need for networking ***************/

static bool scenario_check_schedule(karte_t *welt, player_t *player, schedule_t *schedule, bool local)
{
	if (!is_scenario()) {
		return true;
	}
	const char* err = welt->get_scenario()->is_schedule_allowed(player, schedule);
	if (err) {
		if (*err  &&  local) {
			create_win( new news_img(err), w_time_delete, magic_none);
		}
		return false;
	}
	return true;
}


/* Handles all action of convois in depots. Needs a default param:
 * [function],[convoi_id],addition stuff
 * following simple command exists:
 * 'x' : self destruct
 * 'f' : open the schedule window
 * 'g' : apply a schedule
 * 'n' : toggle 'no load'
 * 'w' : toggle withdraw
 * 'd' : dissassemble convoi and store vehicle in this depot
 * 'T' : toggle 'retire'
 * 's' : change state to [number] (and maybe set open schedule flag)
 * 'l' : apply new line [number]
 * 'c' : reassign classes
 */
bool tool_change_convoi_t::init( player_t *player )
{
	char tool = 0;
	uint16 convoi_id = 0;

	// skip the rest of the command
	const char *p = default_param;
	while(  *p  &&  *p<=' '  ) {
		p++;
	}
	sscanf( p, "%c,%hi", &tool, &convoi_id );

	// skip to the commands ...
	for(  int z = 2;  *p  &&  z>0;  p++  ) {
		if(  *p==','  ) {
			z--;
		}
	}

	convoihandle_t cnv;
	cnv.set_id( convoi_id );
	// double click on remove button will send two such commands
	// the first will delete the convoi, the second should not trigger an assertion
	// catch such commands here
	if( !cnv.is_bound()) {
#if DEBUG>=4
		if (is_local_execution()) {
			create_win( new news_img("Convoy already deleted!"), w_time_delete, magic_none);
		}
#endif
		dbg->warning("tool_change_convoi_t::init", "no convoy with id=%d found", convoi_id);
		return false;
	}
	// ownership check for network games
	if (cnv.is_bound()  &&  env_t::networkmode  &&  !player_t::check_owner(cnv->get_owner(), player)) {
		return false;
	}

	// first letter is now the actual command
	switch (tool) {
	case 'x': // self destruction ...
		if (cnv.is_bound()) {
			if (cnv->get_state() == convoi_t::INITIAL) {
				// delete cnv in depot
				if (grund_t *gr = welt->lookup(cnv->get_pos())) {
					if (depot_t *dep = gr->get_depot()) {
						dep->disassemble_convoi(cnv, true);
						return false;
					}
				}
			}
			cnv->self_destruct();
		}
		return false;

	case 'f': // open schedule
	{
		// we open the window only when executed on the same client that triggered the tool
		// but the all clients must call the function anyway
		cnv->open_schedule_window(is_local_execution());
	}
	break;

	case 'g': // change schedule
	{
		schedule_t *schedule = cnv->create_schedule()->copy();
		schedule->finish_editing();
		if (schedule->sscanf_schedule(p) && scenario_check_schedule(welt, player, schedule, is_local_execution())) {
			cnv->set_schedule(schedule);
		}
		else {
			// could not read schedule, do not assign
			delete schedule;
		}
	}
	break;

	case 'l': // change line
	{
		// read out id and new current_stop index
		uint16 id = 0, current_stop = 0;
		int count = sscanf(p, "%hi,%hi", &id, &current_stop);
		linehandle_t l;
		l.set_id(id);
		if (l.is_bound()) {
			// sanity check for right line-type (compare schedule types ..)
			schedule_t *schedule = cnv->create_schedule();
			if (schedule  &&  l->get_schedule() && schedule->get_type() != l->get_schedule()->get_type()) {
				dbg->warning("tool_change_convoi_t::init", "types of convoi and line do not match");
				return false;
			}
			if (count == 1) {
				// current_stop was not supplied -> take it from line schedule
				current_stop = l->get_schedule()->get_current_stop();
			}
			cnv->set_line(l);
			cnv->get_schedule()->set_current_stop((uint8)current_stop);
			cnv->get_schedule()->finish_editing();
		}
	}
	break;

	case 'n': // change no_load
		cnv->set_no_load(!cnv->get_no_load());
		if (!cnv->get_no_load()) {
			cnv->set_withdraw(false);
		}
		break;

	case 'C': // Copy a replace datum
	{
		uint16 cnv_rpl_id;
		sscanf(p, "%hi", &cnv_rpl_id);
		convoihandle_t cnv_rpl;
		cnv_rpl.set_id(cnv_rpl_id);
		if (cnv_rpl.is_bound() && cnv_rpl->get_replace())
		{
			cnv->set_replace(cnv_rpl->get_replace());
			cnv->set_depot_when_empty(cnv->get_replace()->get_autostart());
			cnv->set_no_load(cnv->get_depot_when_empty());
			// If already empty, no need to be emptied
			if (cnv->get_replace() && cnv->get_depot_when_empty() && cnv->has_no_cargo())
			{
				cnv->set_depot_when_empty(false);
				cnv->set_no_load(false);
				cnv->go_to_depot(false);
			}
			break;
		}
		// Else fallthrough
	}

	case 'R': // Add new replace
	{
		replace_data_t* rpl = new replace_data_t();
		if (rpl->sscanf_replace(p))
		{
			// If the above method returns false, the replace creating has not worked,
			// possibly because the data are corrupted. The replace ought not be set
			// in this case.

			cnv->set_replace(rpl);
			cnv->set_depot_when_empty(rpl->get_autostart());
			cnv->set_no_load(cnv->get_depot_when_empty());
			// If already empty, no need to be emptied
			if (cnv->get_replace() && cnv->get_depot_when_empty() && cnv->has_no_cargo())
			{
				cnv->set_depot_when_empty(false);
				cnv->set_no_load(false);
				cnv->go_to_depot(false, rpl->get_use_home_depot());
			}
			break;
		}
	}

	case 'P': // Go to depot
	{
		cnv->go_to_depot(true);
		break;
	}

	case 'V': // Reverse button
	{
		const bool rs = cnv->get_reverse_schedule();
		cnv->set_reverse_schedule(!rs);
		break;
	}

	case 'D': // Used for when "depot" is set in the replace frame
	{
		cnv->set_depot_when_empty(true);
		cnv->set_no_load(true);
	}
	break;

	case 'T': // change retire
		if (player != welt->get_active_player() && !env_t::networkmode) {
			// pop up error message here!
			return false;
		}
		cnv->set_depot_when_empty(!cnv->get_depot_when_empty());
		cnv->set_no_load(cnv->get_depot_when_empty());
		cnv->set_replace(NULL);
		break;

	case 'X': // Clear replace
		if (cnv->get_replace())
		{
			cnv->get_replace()->clear_all();
			// This convoy might already have been sent to a depot. This will need to be undone.
			schedule_t* sch = cnv->get_schedule();
			const schedule_entry_t le = sch->get_current_entry();
			const grund_t* gr = welt->lookup(le.pos);
			if (gr && gr->get_depot())
			{
				sch->remove();
				cnv->set_state(2);
			}
		}
		break;

	case 'w': // change withdraw
		cnv->set_withdraw(!cnv->get_withdraw());
		cnv->set_no_load(cnv->get_withdraw());
		cnv->set_replace(NULL);
		break;

	case 's': // change state
	{
		int new_state = atoi(p);
		if (new_state > 0) {
			cnv->set_state(new_state);
			if (new_state == convoi_t::EDIT_SCHEDULE) {
				cnv->get_schedule()->start_editing();
			}
		}
		break;
	}
	case 'c': // reassign class

		uint8 compartment, new_class;
		sint32 good_type; // 0 = Passenger, 1 = Mail,
		sint32 reset; // 0 = reset only single class, 1 = reset all classes
		sscanf(p, "%hi,%hi,%i,%i", &compartment, &new_class, &good_type, &reset);
		//uint16 new_class = atoi(p);
		if (reset == 1)
		{
			for (uint8 veh = 0; veh < cnv->get_vehicle_count(); veh++)
			{
				vehicle_t* v = cnv->get_vehicle(veh);
				if (good_type == 0 && v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS)
				{
					uint8 classes_amount = v->get_desc()->get_number_of_classes();
					for (sint32 i = 0; i < classes_amount; i++)
					{
						v->set_class_reassignment(i, i);
					}
				}
				if (good_type == 1 && v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_MAIL)
				{
					uint8 classes_amount = v->get_desc()->get_number_of_classes();
					for (sint32 i = 0; i < classes_amount; i++)
					{
						v->set_class_reassignment(i, i);
					}
				}
			}
		}
		else if (reset == 0)
		{
			for (uint8 veh = 0; veh < cnv->get_vehicle_count(); veh++)
			{
				vehicle_t* v = cnv->get_vehicle(veh);
				uint8 classes_amount = v->get_desc()->get_number_of_classes();
				if (good_type == 0 && v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS)
				{
					if (compartment < classes_amount)
					{
						v->set_class_reassignment(compartment, new_class);
					}
				}
				if (good_type == 1 && v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_MAIL)
				{
					if (compartment < classes_amount)
					{
						v->set_class_reassignment(compartment, new_class);
					}
				}
			}
		}
		cnv->calc_classes_carried();
		linehandle_t line = cnv->get_line();
		if(line.is_bound())
		{
			line->calc_classes_carried();
		}
		break;
	}

	if(  cnv->in_depot()  &&  (tool=='g'  ||  tool=='l')  ) {
		const grund_t *const ground = welt->lookup( cnv->get_home_depot() );
		if(  ground  ) {
			const depot_t *const depot = ground->get_depot();
			if(  depot  ) {
				depot_frame_t *const frame = dynamic_cast<depot_frame_t *>( win_get_magic( (ptrdiff_t)depot ) );
				if(  frame  ) {
					frame->update_data();
				}
			}
		}
	}

	return false;	// no related work tool ...
}



/* Handles all action of lines. Needs a default param:
 * [function],[line_id],addition stuff
 * following simple command exists:
 * 'g' : apply new schedule to line [schedule follows]
 * 'V' : Apply a new livery scheme to the line [livery scheme index follows]
 */
bool tool_change_line_t::init( player_t *player )
{
	uint16 line_id = 0;

	// skip the rest of the command
	const char *p = default_param;
	while(  *p  &&  *p<=' '  ) {
		p++;
	}

	char tool=*p++;
	while(  *p  &&  *p++!=','  ) {
	}
	if(  *p==0  ) {
		dbg->warning( "tool_change_line_t::init()", "too short command \"%s\"", default_param );
		return false;
	}

	line_id = atoi(p);
	while(  *p  &&  *p++!=','  ) {
	}

	linehandle_t line;
	line.set_id( line_id );

	// ownership check for network games
	if (line.is_bound()  &&  env_t::networkmode  && !player_t::check_owner(line->get_owner(), player)) {
		return false;
	}

	// first letter is now the actual command
	switch(  tool  ) {
		case 'c': // create line, next parameter line type and magic of schedule window (only right window gets updated)
			{
				line = player->simlinemgmt.create_line( atoi(p), player );
				while(  *p  &&  *p++!=','  ) {
				}
				long t;
				sscanf( p, "%ld", &t );
				while(  *p  &&  *p++!=','  ) {
				}

				// no need to check schedule for scenario conditions, as schedule is only copied
				line->get_schedule()->sscanf_schedule( p );
				line->get_schedule()->finish_editing();	// just in case ...
				if(  is_local_execution()  ) {
					schedule_gui_t *fg = dynamic_cast<schedule_gui_t *>(win_get_magic((ptrdiff_t)t));
					if(  fg  ) {
						fg->init_line_selector();
					}
					schedule_list_gui_t *sl = dynamic_cast<schedule_list_gui_t *>(win_get_magic(magic_line_management_t+player->get_player_nr()));
					if(  sl  ) {
						sl->show_lineinfo( line );
					}
					// no schedule window open => then open one
					if(  fg==NULL  ) {
						create_win( new line_management_gui_t(line, player), w_info, (ptrdiff_t)line.get_rep() );
					}
				}
			}
			break;

		case 'd':	// delete line
			{
				if (line.is_bound()  &&  line->count_convoys()==0) {
					// close a schedule window, if still active
					gui_frame_t *w = win_get_magic( (ptrdiff_t)line.get_rep() );
					if(w) {
						destroy_win( w );
					}
					player->simlinemgmt.delete_line(line);
				}
			}
			break;

		case 'g': // change schedule
			{
				if (line.is_bound()) {
					schedule_t *schedule = line->get_schedule()->copy();
					if(schedule->sscanf_schedule( p )  && schedule->get_count() > 1 && scenario_check_schedule(welt, player, schedule, is_local_execution()) ) {
						schedule->finish_editing();
						line->set_schedule( schedule );
						simlinemgmt_t::update_line(line);
					}
					else {
						// could not read schedule, do not assign
						delete schedule;
					}
				}
			}
			break;

		case 't':	// trims away convois on all lines of linetype with this default parameter
			{
				vector_tpl<linehandle_t> const& lines = player->simlinemgmt.get_line_list();
				// what kind of lines to trim
				const simline_t::linetype linetype = (simline_t::linetype)atoi(p);
				while(  *p  &&  *p++!=','  ) {
				}
				// how much as target
				sint32 percentage = *p ? atoi(p) : 10;
				if(  percentage == 0  ) {
					break;
				}

				FOR(vector_tpl<linehandle_t>,line,lines) {
					if(  line->get_linetype() == linetype  &&  line->get_convoys().get_count() > 3  ) {
						// correct waytpe and more than one,n now some up usage for the last six months
						sint64 transported = 0, capacity = 0;
						for(  int i=0;  i<6;  i++  ) {
							capacity += line->get_finance_history( i , LINE_CAPACITY );
							transported += line->get_finance_history( i , LINE_TRANSPORTED_GOODS );
						}
						// sanity check for non-moving lines
						if(  capacity == 0  ) {
							continue;
						}

						if(  (transported*100) / capacity < percentage  ) {
							// less than 33 % usage => remove concois
							vector_tpl<convoihandle_t> const& cnvs = line->get_convoys();
							sint64 old_sum_capacity = 0;
							FOR(vector_tpl<convoihandle_t>,cnv,cnvs) {
								for(  int i=0;  i<cnv->get_vehicle_count();  i++  ) {
									old_sum_capacity += cnv->get_vehicle(i)->get_desc()->get_capacity();
								}
							}
							/* now we have the total capacity. We will now remove convois until this capacity
							 * is reduced by the ration suggested from transported = 1/3 of total capacity
							 * x is the percentage of used capacity
							 * Then is the new target sum_capacity = x*old_sum_capacity
							 */
							sint64 new_sum_capacity = (transported * 1000 * old_sum_capacity) / (capacity * percentage * 10);

							// first we remove the totally empty convois (nowbody will miss them)
							int destroyed = 0, initial = line->get_convoys().get_count();
							for(  int j = initial - 1;  j >= 0  &&  initial-destroyed > 3  &&  new_sum_capacity < old_sum_capacity;  j--  ) {
								convoihandle_t cnv = line->get_convoy(j);
								if(  cnv->get_loading_level() == 0  ||  cnv->get_state() == convoi_t::INITIAL  ) {
									for(  int i=0;  i<cnv->get_vehicle_count();  i++  ) {
										old_sum_capacity -= cnv->get_vehicle(i)->get_desc()->get_capacity();
									}
									cnv->self_destruct();
									destroyed ++;
								}
							}
							// not enough? Then remove from the end ...
							for(  uint32 j=0;  j < line->get_convoys().get_count()  &&  initial-destroyed > 3  &&  new_sum_capacity < old_sum_capacity;  j++  ) {
								convoihandle_t cnv = line->get_convoy(j);
								if(  cnv->get_state() != convoi_t::SELF_DESTRUCT  ) {
									for(  int i=0;  i<cnv->get_vehicle_count();  i++  ) {
										old_sum_capacity -= cnv->get_vehicle(i)->get_desc()->get_capacity();
									}
									cnv->self_destruct();
									destroyed ++;
								}
							}
							// done
							if(  destroyed  ) {
								dbg->message( "tool_change_line_t::init", "trim line %s: Reduced from %d to %d convois", line->get_name(), initial, initial-destroyed );
							}
						}
					}
				}
			}
			break;

		case 'u':	// unite all lineless convois with similar schedules
			{
				array_tpl<vector_tpl<convoihandle_t> > cnvs(welt->convoys().get_count());
				uint32 max_cnvs=0;
				FOR(vector_tpl<convoihandle_t>, cnv, welt->convoys()) {
					// only check lineless convoys
					if(  !cnv->get_line().is_bound()  ) {
						bool found = false;
						// check, if already matches existing convois schedule
						for(  uint32 i=0;  i<max_cnvs  &&  !found;  i++  ) {
							FOR(vector_tpl<convoihandle_t>, cnvcomp, cnvs[i] ) {
								if(  cnvcomp->get_schedule()->matches( welt, cnv->get_schedule() )  ) {
									found = true;
									cnvs[i].append( cnv );
									break;
								}
							}
						}
						// not added: then may be new line for this?
						if(  !found  ) {
							cnvs[max_cnvs++].append( cnv );
						}
					}
				}
				// now we have an array of one or more lineless convois
				for(  uint32 i=0;  i<max_cnvs;  i++  ) {
					// if there is more than one convois => new line
					if(  cnvs[i].get_count()>1  ) {
						line = player->simlinemgmt.create_line( cnvs[i][0]->get_schedule()->get_type(), player, cnvs[i][0]->get_schedule() );
						FOR(vector_tpl<convoihandle_t>, cnv, cnvs[i] ) {
							line->add_convoy( cnv );
							cnv->set_line( line );
						}
					}
				}
			}
			break;

		case 'w': // change widthdraw
			{
				if (line.is_bound()) {
					line->set_withdraw( atoi(p) );
				}
			}
			break;

		case 'V': // Change livery
			{
				uint16 livery_scheme_index = atoi(p);
				if(line.is_bound())
				{
					line->set_livery_scheme_index(livery_scheme_index);
					line->propogate_livery_scheme();
				}
			}
	}
	return false;
}



/* Handles all action of convois in depots. Needs a default param:
 * [function],[depot_pos_3d],[convoi_id],addition stuff
 * following simple command exists:
 * 'l' : creates a new line (convoi_id might be invalid) (+printf'd initial schedule)
 * 'b' : starts the convoy
 * 'B' : starts all convoys
 * 'c' : copies this convoy
 * 'd' : disassembles convoy
 * 's' : sells convoy
 * 'a' : appends a vehicle (+vehicle_name) uses the oldest
 * 'i' : inserts a vehicle in front (+vehicle_name) uses the oldest
 * 's' : sells a vehicle (+vehicle_name) uses the newest
 * 'r' : removes a vehicle (+number in convoi)
 * 'R' : removes all vehicles including (+number in convoi) to end
 */
bool tool_change_depot_t::init( player_t *player )
{
	char tool=0;
	koord3d pos = koord3d::invalid;
	sint16 z;
	uint16 convoi_id;
	uint16 livery_scheme_index;

	// skip the rest of the command
	const char *p = default_param;
	while(  *p  &&  *p<=' '  ) {
		p++;
	}
	sscanf( p, "%c,%hi,%hi,%hi,%hi,%hi", &tool, &pos.x, &pos.y, &z, &convoi_id, &livery_scheme_index );
	pos.z = (sint8)z;

	// skip to the commands ...
	z = 6;
	while(  *p  &&  z>0  ) {
		if(  *p==','  ) {
			z--;
		}
		p++;
	}

	grund_t *gr = welt->lookup(pos);
	depot_t *depot = gr ? gr->get_depot() : NULL;
	if(  depot==NULL  ){
		dbg->warning("tool_change_depot_t::init", "no depot found at (%s)", pos.get_str());
		return false;
	}
	if(  !player_t::check_owner( depot->get_owner(), player)  ) {
		dbg->warning("tool_change_depot_t::init", "depot at (%s) belongs to another player", pos.get_str());
		return false;
	}

	convoihandle_t cnv;
	cnv.set_id( convoi_id );

	// ok now do our stuff
	switch(  tool  ) {
		case 'l': { // create line schedule window
			linehandle_t selected_line = depot->get_owner()->simlinemgmt.create_line(depot->get_line_type(),depot->get_owner());
			// no need to check schedule for scenario conditions, as schedule is only copied
			selected_line->get_schedule()->sscanf_schedule( p );

			depot_frame_t *depot_frame = dynamic_cast<depot_frame_t *>(win_get_magic( (ptrdiff_t)depot ));
			if(cnv.is_bound())
			{
				selected_line->set_livery_scheme_index(cnv->get_livery_scheme_index());
			}
			if(  is_local_execution()  ) {
				if(  welt->get_active_player()==player  &&  depot_frame  ) {
					create_win( new line_management_gui_t( selected_line, depot->get_owner() ), w_info, (ptrdiff_t)selected_line.get_rep() );
				}
			}

			if(  depot_frame  ) {
				if(  is_local_execution()  ) {
					depot_frame->set_selected_line( selected_line );
					depot_frame->apply_line();
				}
				depot_frame->update_data();
			}

			schedule_list_gui_t *sl = dynamic_cast<schedule_list_gui_t *>(win_get_magic( magic_line_management_t + player->get_player_nr() ));
			if(  sl  ) {
				sl->update_data( selected_line );
			}
			DBG_MESSAGE("depot_frame_t::new_line()","id=%d",selected_line.get_id() );
			break;
		}
		case 'b': { // start a convoi from the depot
			if(  cnv.is_bound()  ) {
				depot->start_convoi(cnv, is_local_execution());
			}
			break;
		}
		case 'B': { // start all convoys
			depot->start_all_convoys();
			break;
		}
		case 'd':   // disassemble convoi
		case 'v': { // sell convoi
			depot->disassemble_convoi( cnv, tool=='v' );
			break;
		}
		case 'c': { // copy this convoi
			if(  cnv.is_bound()  ) {
				if(  convoihandle_t::is_exhausted()  ) {
					if(  is_local_execution()  ) {
						create_win( new news_img("Convoi handles exhausted!"), w_time_delete, magic_none );
					}
					return false;
				}
				depot->copy_convoi( cnv, is_local_execution() );
			}
			break;
		}
		case 'a':   // append a vehicle
		case 'i':   // insert a vehicle in front
		case 's':   // sells a vehicle
		case 'u':   // Upgrades a vehicle
		case 'r':   // removes a vehicle (assumes a valid depot)
		case 'R': { // removes all vehicles to end (assumes a valid depot)
			if(  tool=='r'  ||  tool=='R'  ) {
				// test may fail after double-click on the button:
				// two remove cmds are sent, only the first will remove, the second should not trigger assertion failure
				if ( cnv.is_bound() ) {
					int start_nr = atoi(p);
					int nr = start_nr;

					// find end
					while(nr<cnv->get_vehicle_count()) {
						const vehicle_desc_t *info = cnv->get_vehicle(nr)->get_desc();
						nr ++;
						if(info->get_trailer_count()!=1) {
							break;
						}
					}
					// now remove the vehicles
					if(  cnv->get_vehicle_count()==nr-start_nr  ||  (tool=='R'  &&  start_nr==0)  ) {
						depot->disassemble_convoi(cnv, false);
					}
					else if(  tool=='R'  ) {
						depot->remove_vehicles_to_end( cnv, start_nr );
					}
					else {
						for(  int i=start_nr;  i<nr;  i++  ) {
							depot->remove_vehicle(cnv, start_nr);
						}
					}
				}
			}
			else {
				// create and append it
				const vehicle_desc_t *info = vehicle_builder_t::get_info( p );
				// we have a valid vehicle there => now check for details
				if(  info  ) {
					// we buy/sell all vehicles together!
					slist_tpl<const vehicle_desc_t *>new_vehicle_info;
					const vehicle_desc_t *start_info = info;

					if(tool!='a') {
						// start of composition
						while(  info->get_leader_count() == 1  &&  info->get_leader(0) != NULL  &&  !new_vehicle_info.is_contained(info)  ) {
							info = info->get_leader(0);
							new_vehicle_info.insert(info);
						}
						info = start_info;
					}
					while(info) {
						new_vehicle_info.append( info );
						if(info->get_trailer_count()!=1  ||  (tool=='i'  &&  info==start_info) || tool == 'u'   ||  new_vehicle_info.is_contained(info->get_trailer(0)))
						{
							// Auto-complete: not used for upgrading.
							break;
						}
						info = info->get_trailer(0);
					}
					// now we have a valid composition together
					if(  tool=='s'  ) {
						while(  new_vehicle_info.get_count()  ) {
							// We sell the newest vehicle - gives most money back.
							vehicle_t* veh = depot->find_oldest_newest(new_vehicle_info.remove_first(), false);
							if(veh != NULL) {
								depot->sell_vehicle(veh);
							}
						}
					}
					else {
						// now check if we are allowed to buy this (we test only leading vehicle, so one can still buy hidden stuff)
						info = new_vehicle_info.front();
						if(  !info->is_available(welt->get_timeline_year_month())  &&  !welt->get_settings().get_allow_buying_obsolete_vehicles()  ) {
							// only allow append/insert, if in depot do not create new obsolete vehicles
							if(  !depot->find_oldest_newest(info, true)  ) {
								// just fail silent
								return false;
							}
						}

						// append/insert into convoi; create one if needed
						depot->clear_command_pending();
						if(  !cnv.is_bound()  ) {
							if(  convoihandle_t::is_exhausted()  ) {
								if(  is_local_execution()  ) {
									create_win( new news_img("Convoi handles exhausted!"), w_time_delete, magic_none);
								}
								return false;
							}
							// create a new convoi
							cnv = depot->add_convoi( is_local_execution() );
							cnv->set_name( new_vehicle_info.front()->get_name() );
							cnv->set_livery_scheme_index(livery_scheme_index);
						}

						// now we have a valid cnv
						if(  cnv->get_vehicle_count()+new_vehicle_info.get_count() <= depot->get_max_convoi_length()  ) {

							for(  unsigned i=0;  i<new_vehicle_info.get_count();  i++  ) {
								// insert/append needs reverse order
								unsigned nr = (tool=='i') ? new_vehicle_info.get_count()-i-1 : i;
								// We add the oldest vehicle - newer stay for selling
								const vehicle_desc_t* vb = new_vehicle_info.at(nr);
								vehicle_t* veh = depot->find_oldest_newest(vb, true);
								if (veh == NULL && tool != 'u')
								{
									// If there are no matching vehicles in the depot,
									// a new vehicle needs to be purchased.

									// If upgrading, we assume that we want to upgrade
									// rather than use vehicles already in the depot.

									veh = depot->buy_vehicle(vb, livery_scheme_index);
								}

								if(tool == 'u')
								{
									depot->upgrade_vehicle(cnv, vb);
								}
								else
								{
									depot->append_vehicle(cnv, veh, tool=='i', is_local_execution());
								}
							}
						}
					}
				}
			}
			if(cnv.is_bound())
			{
				cnv->set_livery_scheme_index(livery_scheme_index);
			}
			depot->update_win();
			break;
		}
	}
	return false;
}


/* Handles all player stuff default_param:
 * [function],[player_id],[state]
 * following command exists:
 * 'a' : activate/deactivate player (depends on state)
 * 'n' : create player at id of type state
 * 'f' : activates/deactivates freeplay
 * 'c' : change player color
 */
bool tool_change_player_t::init( player_t *player)
{
	if(  default_param==NULL  ) {
		dbg->warning( "tool_change_player_t::init()", "nothing to do!" );
		return false;
	}

	char tool=0;
	int id=0;
	int state=0;

	// skip the rest of the command
	const char *p = default_param;
	while(  *p  &&  *p<=' '  ) {
		p++;
	}
	sscanf( p, "%c,%i,%i", &tool, &id, &state );

	player_t *setting_player = welt->get_player(id);

	// ok now do our stuff
	switch(  tool  ) {
		case 'a': // activate/deactivate AI
			if(  player  &&  player->get_ai_id()!=player_t::HUMAN  &&  (setting_player==welt->get_public_player()  ||  !env_t::networkmode)  ) {
				player->set_active(state);
				welt->get_settings().set_player_active(id, player->is_active());
			}
			break;
		case 'c': // change player color
			if(  player  &&  player==setting_player  ) {
				int c1, c2, dummy;
				sscanf( p, "%c,%i,%i,%i", &tool, &dummy, &c1, &c2 );
				player->set_player_color( c1, c2 );
			}
			break;
		case 'n': // WAS: new player with type state
		case 'f': // WAS: activate/deactivate freeplay
			dbg->error( "tool_change_player_t::init()", "deprecated command called" );
			break;
	}

	// update the window
	ki_kontroll_t* playerwin = (ki_kontroll_t*)win_get_magic(magic_ki_kontroll_t);
	if (playerwin) {
		playerwin->update_data();
	}

	return false;
}


/* Sets traffic light phases via default_param:
 * [pos],[ns_flag],[ticks]
 */
bool tool_change_traffic_light_t::init( player_t *player )
{
	koord3d pos;
	sint16 z, ns, ticks;
	if(  5!=sscanf( default_param, "%hi,%hi,%hi,%hi,%hi", &pos.x, &pos.y, &z, &ns, &ticks )  ) {
		return false;
	}
	pos.z = (sint8)z;
	if(  grund_t *gr = welt->lookup(pos)  ) {
		if( roadsign_t *rs = gr->find<roadsign_t>()  ) {
			if(  (  rs->get_desc()->is_traffic_light()  ||  rs->get_desc()->is_private_way()  )  &&  player_t::check_owner(rs->get_owner(),player)  ) {
				if(  ns == 1  ) {
					rs->set_ticks_ns( (uint8)ticks );
				}
				else if(  ns == 0  ) {
					rs->set_ticks_ow( (uint8)ticks );
				}
				else if(  ns == 2  ) {
					rs->set_ticks_offset( (uint8)ticks );
				}
        else if(  ns == 3  ) {
          rs->set_open_direction( (uint8)ticks );
        }
				// update the window
				if(  rs->get_desc()->is_traffic_light()  ) {
					trafficlight_info_t* trafficlight_win = (trafficlight_info_t*)win_get_magic((ptrdiff_t)rs);
					if (trafficlight_win) {
						trafficlight_win->update_data();
					}
				}
				else {
					privatesign_info_t* trafficlight_win = (privatesign_info_t*)win_get_magic((ptrdiff_t)rs);
					if (trafficlight_win) {
						trafficlight_win->update_data();
					}
				}
			}
		}
	}
	return false;
}

/* Sets overtaking_mode via default_param:
 *
 */
bool tool_change_roadsign_t::init( player_t *player )
{
	koord3d pos;
	sint16 z, inst;
	if(  4!=sscanf( default_param, "%hi,%hi,%hi,%hi", &pos.x, &pos.y, &z, &inst )  ) {
		return false;
	}
	pos.z = (sint8)z;
	if(  grund_t *gr = welt->lookup(pos)  ) {
		if( roadsign_t *rs = gr->find<roadsign_t>()  ) {
			if(  rs->get_intersection()!=koord3d::invalid  ) {
				rs->set_lane_affinity(inst);
			}
			onewaysign_info_t* onewaysign_win = (onewaysign_info_t*)win_get_magic((ptrdiff_t)rs);
			if (onewaysign_win) {
				onewaysign_win->update_data();
			}
		}
	}
	return false;
}

/**
 * change city:
 * g[x],[y],[allow_city_growth]
 */
bool tool_change_city_t::init( player_t *player )
{
	if (player != welt->get_public_player()) {
		return false;
	}
	koord k;
	sint16 allow_growth;
	if(  3!=sscanf( default_param, "g%hi,%hi,%hi", &k.x, &k.y, &allow_growth )  ) {
		return false;
	}
	stadt_t* city = welt->get_city(k);
	if (city)
	{
		city->set_citygrowth_yesno(allow_growth);
		city_info_t *stinfo = dynamic_cast<city_info_t*>(win_get_magic((ptrdiff_t)city));
		if (stinfo)
		{
			stinfo->update_data();
		}
	}
	return false;
}


/* Handles renaming of ingame entities. Needs a default param:
 * [object='c|h|l|m|t|p|f'][id|pos],[name]
 * c=convoi, h=halt, l=line,  m=marker, t=town, p=player, f=factory
 * in case of marker / factory, id is a pos3d string
 */
bool tool_rename_t::init(player_t *player)
{
	uint16 id = 0;
	koord3d pos = koord3d::invalid;

	// skip the rest of the command
	const char *p = default_param;
	const char what = *p++;
	switch(  what  ) {
		case 'h':
		case 'l':
		case 'c':
		case 't':
		case 'p':
			id = atoi(p);
			while(  *p>0  &&  *p++!=','  ) {
			}
			break;
		case 'm':
		case 'f':
			if(  3!=sscanf( p, "%hi,%hi,%hi", &pos.x, &pos.y, &id )  ) {
				dbg->error( "tool_rename_t::init", "no position given for marker/factory! (%s)", default_param );
				return false;
			}
			while(  *p>0  &&  *p++!=','  ) {
			}
			while(  *p>0  &&  *p++!=','  ) {
			}
			while(  *p>0  &&  *p++!=','  ) {
			}
			pos.z = (sint8)id;
			id = 0;
			break;
		default:
			dbg->error( "tool_rename_t::init", "illegal request! (%s)", default_param );
			return false;
	}

	// now for action ...
	switch(  what  ) {
		case 'h':
		{
			halthandle_t halt;
			halt.set_id( id );
			if(  halt.is_bound()  &&  (!env_t::networkmode  ||  player_t::check_owner(halt->get_owner(), player))  ) {
				halt->set_name( p );
				return false;
			}
			break;
		}

		case 'l':
		{
			linehandle_t line;
			line.set_id( id );
			if(  line.is_bound()  &&  (!env_t::networkmode  ||  player_t::check_owner(line->get_owner(), player))  ) {
				line->set_name( p );

				schedule_list_gui_t *sl = dynamic_cast<schedule_list_gui_t *>(win_get_magic(magic_line_management_t+player->get_player_nr()));
				if(  sl  ) {
					sl->update_data( line );
				}
				return false;
			}
			break;
		}

		case 'c':
		{
			convoihandle_t cnv;
			cnv.set_id( id );
			if(  cnv.is_bound()  &&  (!env_t::networkmode  ||  player_t::check_owner(cnv->get_owner(), player))  ) {
				//  set name without ID
				cnv->set_name( p, false );
				return false;
			}
			break;
		}

		case 't':
		{
			if(  player == welt->get_public_player()  &&   id<welt->get_cities().get_count()  ) {
				welt->get_cities()[id]->set_name( p );
				return false;
			}
			break;
		}

		case 'm':
			if(  grund_t *gr = welt->lookup(pos)  ) {
				label_t *label = gr->find<label_t>();
				if (label  &&  (!env_t::networkmode  ||  player_t::check_owner(label->get_owner(), player))  ) {
					gr->set_text(p);
				}
				return false;
			}
			break;

		case 'p': {
			player_t *other = welt->get_player((uint8)id);
			if(  other  &&  other == player  ) {
				other->set_name(p);
				return false;
			}
			break;
		}

		case 'f':
		{
			if(  player == welt->get_public_player()) {
				if(  grund_t *gr = welt->lookup(pos)  ) {
					if(  gebaeude_t* gb = gr->find<gebaeude_t>()  ) {
						if (  fabrik_t *fab = gb->get_fabrik()  ) {
							fab->set_name(p);
							return false;
						}
					}
				}
			}
		}
	}
	// we are only getting here, if we could not process this request
	dbg->warning( "tool_rename_t::init", "could not perform (%s)", default_param );
	return false;
}

bool tool_recolour_t::init(player_t *player)
{
	uint16 id = 0;

	// skip the rest of the command
	const char *p = default_param;
	const char what = *p++;
	switch(  what  )
	{
		case '1':
		case '2':
			id = atoi(p);
			while(  *p>0  &&  *p++!=','  )
			{
				// Do nothing
			}
			break;
		default:
			dbg->error( "tool_recolour_t::init", "illegal request! (%s)", default_param );
			return false;
	}

	const uint8 colour = atoi(p);

	// now for action ...
	switch(  what  )
	{
		case '1': // Change player colour 1
		{
			if(welt->get_player(id))
			{
				welt->get_player(id)->set_player_color(colour, welt->get_player(id)->get_player_color2());
				return false;
			}
		}

		case '2': // Change player colour 2
		{
			if(welt->get_player(id))
			{
				welt->get_player(id)->set_player_color( welt->get_player(id)->get_player_color1(), colour);
				return false;
			}
		}
	}

	// we are only getting here, if we could not process this request
	dbg->error( "tool_recolour_t::init", "could not perform (%s)", default_param );
	return false;
}

bool tool_access_t::init(player_t *player)
{
	uint16 id_setting_player;
	uint16 id_receiving_player;
	sint16 allow_access;

	if(3 != sscanf(default_param, "g%hi,%hi,%hi", &id_setting_player, &id_receiving_player, &allow_access))
	{
		dbg->error( "tool_access_t::init", "could not perform (%s)", default_param );
		return false;
	}

	player_t* const setting_player = welt->get_player(id_setting_player);
	if(!setting_player)
	{
		return false;
	}

	setting_player->set_allow_access_to(id_receiving_player, allow_access);
	if(allow_access == false)
	{
		// If access is withdrawn, the routing/scheduling must be updated to take account of the fact
		// that convoys may not be able to stop where once they were able to stop.
		player_t* const receiving_player = welt->get_player(id_receiving_player);
		schedule_t* schedule;
		koord3d pos;
		waytype_t    wtyp;

		uint8 current_aktuell;
		halthandle_t halt;
		linehandle_t current_line;
		vector_tpl<uint8> entries_to_remove;
		for (vector_tpl<linehandle_t>::const_iterator j = receiving_player->simlinemgmt.get_all_lines().begin(),
					 end = receiving_player->simlinemgmt.get_all_lines().end(); j != end; j++)
		{
			current_line = *j;
			if(current_line.is_bound())
			{
				schedule = current_line->get_schedule();
				if(!schedule)
				{
					continue;
				}
				current_aktuell = schedule->get_current_stop();
				wtyp = schedule->get_waytype();
				for(uint8 n = 0; n < schedule->get_count(); n ++)
				{
					bool tram_stop_public = false;
					pos = schedule->entries[n].pos;
					const grund_t* gr = welt->lookup(pos);
					halt = gr ? gr->get_halt() : halthandle_t();
					const weg_t *w = gr ? gr->get_weg(schedule->get_waytype()) : NULL;
					if(halt.is_bound())
					{
						const grund_t* gr = welt->lookup(schedule->get_current_entry().pos);
						if(schedule->get_waytype() == tram_wt)
						{
							const weg_t *street = gr ? gr->get_weg(road_wt) : NULL;
							if(street && (street->get_owner() == receiving_player || street->get_owner() == NULL || street->get_owner()->allows_access_to(receiving_player->get_player_nr())))
							{
								tram_stop_public = true;
							}
						}
					}
					const player_t *way_owner = w ? w->get_owner() : NULL;
					if((!halt.is_bound() || !halt->check_access(receiving_player)) && way_owner && !way_owner->allows_access_to(receiving_player->get_player_nr()) && !tram_stop_public)
					{
						// The player no longer allows access: remove the stop.
						entries_to_remove.append(n);
					}
				}

				ITERATE(entries_to_remove, j)
				{
					schedule->set_current_stop(j);
					schedule->remove();
					schedule->set_current_stop(current_aktuell);
					schedule->cleanup();
					if(halt.is_bound())
					{
						halt->remove_line(current_line);
					}
				}
				if(!entries_to_remove.empty())
				{
					simlinemgmt_t::update_line(current_line);
				}
			}
		}

		entries_to_remove.clear();
		const uint32 convoy_count = welt->convoys().get_count();
		convoihandle_t cnv;
		for(uint32 i = 0; i < convoy_count; i ++)
		{
			cnv = welt->convoys()[i];
			if(!cnv.is_bound() || cnv->get_line().is_bound() || cnv->get_owner() != receiving_player)
			{
				// We dealt above with lines.
				continue;
			}
			schedule = cnv->get_schedule();
			if(!schedule)
			{
				continue;
			}
			current_aktuell = schedule->get_current_stop();
			wtyp = schedule->get_waytype();
			for(uint8 n = 0; n < schedule->get_count(); n ++)
			{
				bool tram_stop_public = false;
				pos = schedule->entries[n].pos;
				const grund_t* gr = welt->lookup(pos);
				halt = gr ? gr->get_halt() : halthandle_t();
				const weg_t *w = gr ? gr->get_weg(schedule->get_waytype()) : NULL;
				if(halt.is_bound())
				{
					const grund_t* gr = welt->lookup(schedule->get_current_entry().pos);
					if(schedule->get_waytype() == tram_wt)
					{
						const weg_t *street = gr ? gr->get_weg(road_wt) : NULL;
						if(street && (street->get_owner() == receiving_player || street->get_owner() == NULL || street->get_owner()->allows_access_to(receiving_player->get_player_nr())))
						{
							tram_stop_public = true;
						}
					}
				}
				const player_t *way_owner = w ? w->get_owner() : NULL;
				if((!halt.is_bound() || !halt->check_access(receiving_player)) && way_owner && !way_owner->allows_access_to(receiving_player->get_player_nr()) && !tram_stop_public)
				{
					// The player no longer allows access: remove the stop.
					entries_to_remove.append(n);
				}

			}

			ITERATE(entries_to_remove, j)
			{
				schedule->set_current_stop(j);
				schedule->remove();
				schedule->set_current_stop(current_aktuell);
				schedule->cleanup();
				if(halt.is_bound())
				{
					halt->remove_convoy(cnv);
				}
			}

			if(!cnv->in_depot() && schedule->get_count() < 2)
			{
				// We need to make sure that convoys with fewer than two
				// stops are not left stranded and blocking things.
				cnv->emergency_go_to_depot();
				// Note that this may destroy the convoy in extreme cases.
			}
		}
#ifdef MULTI_THREAD
		world()->stop_path_explorer();
#endif
		path_explorer_t::refresh_all_categories(false);
		if (cnv.is_bound())
		{
			cnv->clear_departures();
		}
	}

	cbuffer_t message;
	if(allow_access)
	{
		message.printf(translator::translate("%s has allowed access rights to %s"), welt->get_player(id_setting_player)->get_name(), welt->get_player(id_receiving_player)->get_name());
	}
	else
	{
		message.printf(translator::translate("%s has withdrawn access rights from %s"),  welt->get_player(id_setting_player)->get_name(), welt->get_player(id_receiving_player)->get_name());
	}
	welt->get_message()->add_message(message, koord::invalid, message_t::ai, setting_player->get_player_color1());
	return false;
}


/*
 * Add a message to the message queue
 */
bool tool_add_message_t::init(player_t *player)
{
	if (*default_param) {
		uint32 type = atoi(default_param);
		const char* text = strchr(default_param, ',');
		if ((type & ~message_t::local_flag) >= message_t::MAX_MESSAGE_TYPE || text == NULL) {
			return false;

		}
		welt->get_message()->add_message(text + 1, koord::invalid, type,
		type & message_t::local_flag ? COL_BLACK : (player ? PLAYER_FLAG | player->get_player_nr() : COL_WHITE), IMG_EMPTY);
	}
	return false;
}
