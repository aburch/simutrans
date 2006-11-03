/*
 * simhalt.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/* simhalt.cc
 *
 * Haltestellen fuer Simutrans
 * 03.2000 getrennt von simfab.cc
 *
 * Hj. Malthaner
 */
#ifdef _MSC_VER
#include <string.h>
#include <malloc.h> // for alloca
#endif
#include "simdebug.h"
#include "simmem.h"
#include "simplan.h"
#include "boden/grund.h"
#include "boden/boden.h"
#include "boden/wasser.h"
#include "boden/wege/weg.h"
#include "boden/wege/strasse.h"
#include "simhalt.h"
#include "simfab.h"
#include "simplay.h"
#include "simconvoi.h"
#include "simwin.h"
#include "simworld.h"
#include "simintr.h"
#include "simpeople.h"
#include "freight_list_sorter.h"

#include "simtime.h"
#include "simcolor.h"
#include "simgraph.h"
#include "freight_list_sorter.h"

#include "gui/halt_info.h"
#include "gui/halt_detail.h"
#include "dings/gebaeude.h"
#ifdef LAGER_NOT_IN_USE
#include "dings/lagerhaus.h"
#endif
#include "dataobj/fahrplan.h"
#include "dataobj/warenziel.h"
#include "dataobj/einstellungen.h"
#include "dataobj/umgebung.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"

#include "utils/tocstring.h"
#include "utils/simstring.h"

#include "besch/ware_besch.h"
#include "bauer/warenbauer.h"

//#include "tpl/minivec_tpl.h"

/**
 * Max number of hops in route calculation
 * @author Hj. Malthaner
 */
static int max_hops = 300;


/**
 * Sets max number of hops in route calculation
 * @author Hj. Malthaner
 */
void haltestelle_t::set_max_hops(int hops)
{
  if(hops > 9994) {
    hops = 9994;
  }

  max_hops = hops;
}


// Helfer Klassen

class HNode {
public:
  halthandle_t halt;
  int depth;
  HNode *link;
};


// Klassenvariablen

slist_tpl<halthandle_t> haltestelle_t::alle_haltestellen;


halthandle_t
haltestelle_t::gib_halt(karte_t *welt, const koord pos)
{
	const planquadrat_t *plan = welt->lookup(pos);
	if(plan) {
		if(plan->gib_halt().is_bound()) {
			return plan->gib_halt();
		}
		// no halt? => we do the water check
		if(plan->gib_kartenboden()->ist_wasser()) {
			const minivec_tpl<halthandle_t> &haltlist = welt->access(pos)->get_haltlist();
#if 0
			for( unsigned i=0;  i<haltlist.get_count();  i++  ) {
				if(haltlist.get(i)->get_station_type()&dock) {
					// ok, this is a ship stop
					halt = haltlist.get(i);
					break;
				}
			}
#else
			// may catch bus stops close to water ...
			if(haltlist.get_count()>0) {
				return haltlist.get(0);
			}
#endif
		}
	}
	return halthandle_t();
}


koord
haltestelle_t::gib_basis_pos() const
{
	if(!grund.is_empty()) {
		return grund.at(0)->gib_pos().gib_2d();
	}
	else {
		return koord::invalid;
	}
}

koord3d
haltestelle_t::gib_basis_pos3d() const
{
	if(!grund.is_empty()) {
		return grund.at(0)->gib_pos();
	}
	else {
		return koord3d::invalid;
	}
}



/**
 * Station factory method. Returns handles instead of pointers.
 * @author Hj. Malthaner
 */
halthandle_t haltestelle_t::create(karte_t *welt, koord pos, spieler_t *sp)
{
    haltestelle_t * p = new haltestelle_t(welt, pos, sp);
    return p->self;
}


/*
 * removes a ground tile from a station
 * @author prissi
 */
bool
haltestelle_t::remove(karte_t *welt, spieler_t *sp, koord3d pos, const char *&msg)
{
	msg = NULL;
	grund_t *bd = welt->lookup(pos);

	// wrong ground?
	if(bd==NULL) {
		dbg->error("haltestelle_t::remove()","illegal ground at %d,%d,%d", pos.x, pos.y, pos.z);
		return false;
	}

	halthandle_t halt = gib_halt(welt,pos);
	if(!halt.is_bound()) {
		dbg->error("haltestelle_t::remove()","no halt at %d,%d,%d", pos.x, pos.y, pos.z);
		return false;
	}

DBG_MESSAGE("haltestelle_t::remove()","removing segment from %d,%d,%d", pos.x, pos.y, pos.z);

	// remove station building?
	gebaeude_t *gb=dynamic_cast<gebaeude_t *>(bd->suche_obj(ding_t::gebaeude));
	if(gb) {
DBG_MESSAGE("haltestelle_t::remove()",  "removing building" );
		const haus_tile_besch_t *tile  = gb->gib_tile();
		koord size = tile->gib_besch()->gib_groesse( tile->gib_layout() );
		const sint32 costs = tile->gib_besch()->gib_level()*(sint32)umgebung_t::cst_multiply_post;

		// get startpos
		koord k=tile->gib_offset();
		if(k != koord(0,0)) {
			return haltestelle_t::remove(welt, sp, pos-koord3d(k,0),msg);
		}
		else {
			for(k.y = 0; k.y < size.y; k.y ++) {
				for(k.x = 0; k.x < size.x; k.x ++) {
					grund_t *gr = welt->lookup(koord3d(k,0)+pos);
					if(welt->max_hgt(k+pos.gib_2d())<=welt->gib_grundwasser()  ||  gr->gib_typ()==grund_t::fundament) {
						msg=gr->kann_alle_obj_entfernen(sp);
						if(msg) {
							return false;
						}
					}
				}
			}
			// remove all gound
DBG_MESSAGE("haltestelle_t::remove()", "removing building: cleanup");
			for(k.y = 0; k.y < size.y; k.y ++) {
				for(k.x = 0; k.x < size.x; k.x ++) {
					grund_t *gr = welt->lookup(koord3d(k,0)+pos);
					sp->buche(costs, k, COST_CONSTRUCTION);
					if(gr) {
						halt->rem_grund(gr);
						gebaeude_t *gb=static_cast<gebaeude_t *>(gr->suche_obj(ding_t::gebaeude));
						gr->obj_remove(gb,sp);	// remove building
						delete gb;
						gr->setze_text(NULL);
						if(gr->gib_typ()==grund_t::fundament) {
							uint8 new_slope = gr->gib_hoehe()==welt->min_hgt(k+pos.gib_2d()) ? 0 : welt->calc_natural_slope(k+pos.gib_2d());
							welt->access(k+pos.gib_2d())->kartenboden_setzen(new boden_t(welt, koord3d(k+pos.gib_2d(),welt->min_hgt(k+pos.gib_2d())), new_slope), false);
						}
						else if(welt->max_hgt(k+pos.gib_2d())<=welt->gib_grundwasser()) {
							welt->access(k+pos.gib_2d())->kartenboden_setzen(new wasser_t(welt, k+pos.gib_2d()), false);
						}
						else {
							weg_t *weg = bd->gib_weg(road_wt);
							if(weg && static_cast<strasse_t *>(weg)->hat_gehweg()) {
								// Stadtstrassen sollten entfernbar sein, deshalb
								// dürfen sie keinen Besitzer haben.
								bd->setze_besitzer( NULL );
							}
						}
						sp->buche(costs, pos.gib_2d()+k, COST_CONSTRUCTION);
					}
				}
			}
			bd = NULL;	// no need to do things twice ...
		}
	}
	else {
		halt->rem_grund(bd);
	}

	if(!halt->existiert_in_welt()) {
DBG_DEBUG("haltestelle_t::remove()","remove last");
		// all deleted?
		halt->gib_besitzer()->halt_remove( halt );
DBG_DEBUG("haltestelle_t::remove()","destroy");
		haltestelle_t::destroy( halt );
	}
	else {
DBG_DEBUG("haltestelle_t::remove()","not last");
		// may have been changed ... (due to post office/dock/railways station deletion)
		halt->recalc_station_type();
	}

	// if building was removed this is false!
	if(bd) {
		bd->calc_bild();

DBG_DEBUG("haltestelle_t::remove()","reset city way owner");
		weg_t *weg = bd->gib_weg(road_wt);
		if(weg && static_cast<strasse_t *>(weg)->hat_gehweg()) {
			// Stadtstrassen sollten entfernbar sein, deshalb
			// dürfen sie keinen Besitzer haben.
			bd->setze_besitzer( NULL );
		}
		bd->setze_text( NULL );
	}
	return true;
}





/**
 * Station factory method. Returns handles instead of pointers.
 * @author Hj. Malthaner
 */
halthandle_t haltestelle_t::create(karte_t *welt, loadsave_t *file)
{
    haltestelle_t *p = new haltestelle_t(welt, file);
    return p->self;
}


/**
 * Station destruction method.
 * @author Hj. Malthaner
 */
void haltestelle_t::destroy(halthandle_t &halt)
{
    haltestelle_t * p = halt.detach();
    delete p;
}


/**
 * Station destruction method.
 * Da destroy() alle_haltestellen modifiziert kann kein Iterator benutzt
 * werden! V. Meyer
 * @author Hj. Malthaner
 */
void haltestelle_t::destroy_all()
{
    while(alle_haltestellen.count() > 0) {
        halthandle_t halt = alle_haltestellen.at(0);
        destroy(halt);
    }
    alle_haltestellen.clear();
}


// Konstruktoren

haltestelle_t::haltestelle_t(karte_t *wl, loadsave_t *file)
	: reservation(0), registered_lines(0)
{
	self = halthandle_t(this);
	alle_haltestellen.insert(self);
	grund.clear();

	welt = wl;
	marke = 0;

	pax_happy = 0;
	pax_unhappy = 0;
	pax_no_route = 0;

	status_color = COL_YELLOW;

	reroute_counter = welt->get_schedule_counter()-1;
	rebuilt_destination_counter = reroute_counter;

	enables = NOT_ENABLED;

	// @author hsiegeln
	sortierung = freight_list_sorter_t::by_name;
	resort_freight_info = true;

  // Lazy init at opening!
  halt_info = NULL;

	rdwr(file);
}


haltestelle_t::haltestelle_t(karte_t *wl, koord pos, spieler_t *sp)
	: reservation(0), registered_lines(0)
{
	self = halthandle_t(this);
	alle_haltestellen.insert(self);

	welt = wl;
	marke = 0;

	this->pos = pos;
	besitzer_p = sp;
#ifdef LAGER_NOT_IN_USE
	lager = NULL;
#endif

	enables = NOT_ENABLED;

	reroute_counter = welt->get_schedule_counter()-1;
	rebuilt_destination_counter = reroute_counter;

	pax_happy = 0;
	pax_unhappy = 0;
	pax_no_route = 0;

	sortierung = freight_list_sorter_t::by_name;
	init_financial_history();

	// Lazy init at opening!
	halt_info = NULL;
}


haltestelle_t::~haltestelle_t()
{
	while(grund.count()>0) {
		rem_grund(grund.at(0));
	}

	if(halt_info) {
		destroy_win(halt_info);
		delete halt_info;
		halt_info = 0;
	}
	alle_haltestellen.remove(self);
	self.unbind();

	for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
		const ware_besch_t *ware = warenbauer_t::gib_info(i);
		slist_tpl<ware_t> *wliste = waren.get(ware);

		if(wliste) {
			waren.remove(ware);
			delete wliste;
		}
	}
	// route may have changed without this station ...
	welt->set_schedule_counter();
}


/**
 * Sets the name. Creates a copy of name.
 * @author Hj. Malthaner
 */
void
haltestelle_t::setze_name(const char *name)
{
	tstrncpy(this->name, translator::translate(name), 128);
	if(grund.count() > 0) {
		grund.at(0)->setze_text(this->name);
	}
}


void
haltestelle_t::step()
{
//	DBG_MESSAGE("haltestelle_t::step()","%s (cnt %i)",gib_name(),reroute_counter);
	// hsiegeln: update amount of waiting ware
	financial_history[0][HALT_WAITING] = sum_all_waiting_goods();
	if(rebuilt_destination_counter!=welt->get_schedule_counter()) {
		// schedule has changed ...
		rebuild_destinations();
		rebuilt_destination_counter = welt->get_schedule_counter();
	}
	else {
		// all new connection updated => recalc routes
		if(reroute_counter!=welt->get_schedule_counter()) {
			reroute_goods();
			reroute_counter = welt->get_schedule_counter();
	//		DBG_MESSAGE("haltestelle_t::step()","rerouting goods at %s",gib_name());
		}
	}
	recalc_status();
}



/**
 * Called every month
 * @author Hj. Malthaner
 */
void haltestelle_t::neuer_monat()
{
	if(enables&CROWDED) {
		besitzer_p->bescheid_station_voll(self);
		enables &= (PAX|POST|WARE);
	}

	// Hajo: reset passenger statistics
	pax_happy = 0;
	pax_no_route = 0;
	pax_unhappy = 0;

		// hsiegeln: roll financial history
	for (int j = 0; j<MAX_HALT_COST; j++) {
		for (int k = MAX_MONTHS-1; k>0; k--) {
			financial_history[k][j] = financial_history[k-1][j];
		}
		financial_history[0][j] = 0;
	}
}



/**
 * Called every 255 steps
 * will distribute the goods to changed routes (if there are any)
 * @author Hj. Malthaner
 */
void haltestelle_t::reroute_goods()
{
	// reroute only monthly!
	ptrhashtable_iterator_tpl<const ware_besch_t *, slist_tpl<ware_t> *> waren_iter(waren);

	while(waren_iter.next()) {
		static slist_tpl<ware_t> waren_kill_queue;
		slist_tpl<ware_t> * wliste = waren_iter.get_current_value();
		slist_iterator_tpl <ware_t> ware_iter(wliste);

		waren_kill_queue.clear();

		// Hajo:
		// Step 1: re-route goods now and then to adapt to changes in
		// world layout, remove all goods which destination was removed
		// from the map

		while(ware_iter.next()) {
			ware_t & ware = ware_iter.access_current();

			// since also the factory halt list is added to the ground, we can use just this ...
			const minivec_tpl <halthandle_t> &halt_list = welt->access(ware.gib_zielpos())->get_haltlist();
			if(halt_list.is_contained(self)) {
				// we are already there!
				if(warenbauer_t::ist_fabrik_ware(ware.gib_typ())) {
					liefere_an_fabrik(ware);
				}
				waren_kill_queue.insert(ware);
			}
			else {
				suche_route(ware);
			}

			INT_CHECK("simhalt 457");

			// check if this good can still reach its destination
			if(!gib_halt(welt,ware.gib_ziel()).is_bound() ||  !gib_halt(welt,ware.gib_zwischenziel()).is_bound()) {
				// schedule it for removal
				waren_kill_queue.insert(ware);
			}
		}

		while( waren_kill_queue.count() ) {
			ware_t w = waren_kill_queue.remove_first();
DBG_MESSAGE("haltestelle_t::reroute_goods()","removing %i %s",w.menge,w.gib_name() );
			wliste->remove(w);
		}
	}
}



/**
 * Calculates a status color for status bars
 * @author Hj. Malthaner
 */
void haltestelle_t::recalc_status()
{
	status_color = financial_history[0][HALT_CONVOIS_ARRIVED] > 0 ? COL_GREEN : COL_YELLOW;

	// has passengers
	if(get_pax_happy() > 0 || get_pax_no_route() > 0) {

		if(get_pax_unhappy() > 200 ) {
			status_color = COL_RED;
		} else if(get_pax_unhappy() > 40) {
			status_color = COL_ORANGE;
		}
	}

	// check for goods
	if(status_color!=COL_RED  &&  get_ware_enabled()) {
		const int count = warenbauer_t::gib_waren_anzahl();
		const int max_ware = get_capacity();

		for( int i=0; i+1<count; i++) {
			const ware_besch_t *wtyp = warenbauer_t::gib_info(i+1);
			if(  gib_ware_summe(wtyp)>max_ware  ) {
				status_color = COL_RED;
				break;
			}
		}
	}
}


/**
 * Draws some nice colored bars giving some status information
 * @author Hj. Malthaner
 */
void haltestelle_t::display_status(int xpos, int ypos) const
{
  const int count = warenbauer_t::gib_waren_anzahl();

  ypos -= 11;
  // all variables in the bracket MUST be signed, otherwise nothing may be drawn at all
  xpos -= (count*4 - get_tile_raster_width())/2;

  for( int i=0; i+1<count; i++) {
    const ware_besch_t *wtyp = warenbauer_t::gib_info(i+1);

    const int v = min((gib_ware_summe(wtyp) >> 2) + 2, 128);

    display_fillbox_wh_clip(xpos+i*4, ypos-v-1, 1, v, COL_GREY4, true);

    display_fillbox_wh_clip(xpos+i*4+1, ypos-v-1, 2, v,
			    // (i & 7) * 4 + 1,
			    255 - i*4, true);

    display_fillbox_wh_clip(xpos+i*4+3, ypos-v-1, 1, v, COL_GREY1, true);

    // Hajo: show up arrow for capped values
    if(v == 128) {
      display_fillbox_wh_clip(xpos+i*4+1, ypos-v-6, 2, 4, COL_WHITE, true);
      display_fillbox_wh_clip(xpos+i*4, ypos-v-5, 4, 1, COL_WHITE, true);
    }
  }

  const int color = gib_status_farbe();

  display_fillbox_wh_clip(xpos-1, ypos, count*4-2, 4, color, true);
}

/*
 * connects a factory to a halt
 */
void
haltestelle_t::verbinde_fabriken()
{
	if(!grund.is_empty()) {

		{	// unlink all
			slist_iterator_tpl <fabrik_t *> fab_iter(fab_list);
			while( fab_iter.next() ) {
				fab_iter.get_current()->unlink_halt(self);
			}
		}
		fab_list.clear();

		slist_iterator_tpl<grund_t *> iter( grund );
		while(iter.next()) {
			grund_t *gb = iter.get_current();
			koord p = gb->gib_pos().gib_2d();

			vector_tpl<fabrik_t *> &fablist = fabrik_t::sind_da_welche( welt,
																								p-koord( welt->gib_einstellungen()->gib_station_coverage(), welt->gib_einstellungen()->gib_station_coverage()),
																								p+koord( welt->gib_einstellungen()->gib_station_coverage(), welt->gib_einstellungen()->gib_station_coverage())
																								);
			for(unsigned i=0; i<fablist.get_count(); i++) {
				fabrik_t * fab = fablist.at(i);
				if(!fab_list.contains(fab)) {
					fab_list.insert(fab);
					fab->link_halt(self);
				}
			}
		}

	}
}


/*
 * removes factory to a halt
 */
void
haltestelle_t::remove_fabriken(fabrik_t *fab)
{
DBG_MESSAGE("haltestelle_t::remove_fabriken()","removing %p",fab);
	for(unsigned i=0;  i<fab_list.count();  i++ ) {
DBG_MESSAGE("haltestelle_t::remove_fabriken()","fab_list at(%i) = %p",i,fab_list.at(i));
	}
DBG_MESSAGE("haltestelle_t::remove_fabriken()","removing %s",fab->gib_name());
	bool ok=fab_list.remove(fab);
DBG_MESSAGE("karte_t::remove_fabriken()","fab_list now %i(%i)",fab_list.count(),ok);
	for(unsigned i=0;  i<fab_list.count();  i++ ) {
DBG_MESSAGE("haltestelle_t::remove_fabriken()","fab_list at(%i) = %p",i,fab_list.at(i));
	}
}


/**
 * Rebuilds the list of reachable destinations
 *
 * @author Hj. Malthaner
 */
void haltestelle_t::rebuild_destinations()
{
	// Hajo: first, remove all old entries
	warenziele.clear();

// DBG_MESSAGE("haltestelle_t::rebuild_destinations()", "Adding new table entries");
	// Hajo: second, calculate new entries

	for(unsigned i=0;  i<welt->get_convoi_count();  i++ ) {
		convoihandle_t cnv = welt->get_convoi_array().get(i);
		// DBG_MESSAGE("haltestelle_t::rebuild_destinations()", "convoi %d %p", cnv.get_id(), cnv.get_rep());

		if(gib_besitzer()==welt->gib_spieler(1)  ||  cnv->gib_besitzer()==gib_besitzer()) {

			INT_CHECK("simhalt.cc 612");

			fahrplan_t *fpl = cnv->gib_fahrplan();
			if(fpl) {
				for(int i=0; i<fpl->maxi(); i++) {

					// Hajo: Hält dieser convoi hier?
					if(gib_halt(welt,fpl->eintrag.get(i).pos)==self) {

						const int anz = cnv->gib_vehikel_anzahl();
						for(int j=0; j<anz; j++) {

							vehikel_t *v = cnv->gib_vehikel(j);
							hat_gehalten(0,v->gib_fracht_typ(), fpl );
						}
					}
				}
			}
		}
	}
}


void
haltestelle_t::liefere_an_fabrik(const ware_t ware)
{
	slist_iterator_tpl<fabrik_t *> fab_iter(fab_list);

	while(fab_iter.next()) {
		fabrik_t * fab = fab_iter.get_current();

		const vector_tpl<ware_t> * eingang = fab->gib_eingang();

		for(uint32 i=0; i<eingang->get_count(); i++) {
			if(eingang->get(i).gib_typ() == ware.gib_typ()  &&  ware.gib_zielpos()==fab->gib_pos().gib_2d()) {
				fab->liefere_an(ware.gib_typ(), ware.menge);
				return;
			}
		}
	}
}


/**
 * Kann die Ware nicht zum Ziel geroutet werden (keine Route), dann werden
 * Ziel und Zwischenziel auf koord::invalid gesetzt.
 *
 * @param ware die zu routende Ware
 * @param start die Starthaltestelle
 * @author Hj. Malthaner
 */
void
haltestelle_t::suche_route(ware_t &ware, koord *next_to_ziel)
{
	const ware_besch_t * warentyp = ware.gib_typ();
	const koord ziel = ware.gib_zielpos();

	// since also the factory halt list is added to the ground, we can use just this ...
	const minivec_tpl <halthandle_t> &halt_list = welt->access(ziel)->get_haltlist();
	// but we can only use a subset of these
	minivec_tpl <halthandle_t> ziel_list (halt_list.get_count());
	for( unsigned h=0;  h<halt_list.get_count();  h++ ) {
		halthandle_t halt = halt_list.at(h);
		if(	halt->is_enabled(warentyp)  ) {
			ziel_list.append( halt );
		}
		else {
//DBG_MESSAGE("suche_route()","halt %s near (%i,%i) does not accept  %s!",halt->gib_name(),ziel.x,ziel.y,warentyp->gib_name());
		}
	}

	if(ziel_list.get_count()==0) {
		ware.setze_ziel(koord::invalid);
		ware.setze_zwischenziel(koord::invalid);
		// printf("keine route zu %d,%d nach %d steps\n", ziel.x, ziel.y, step);
		if(next_to_ziel!=NULL) {
			*next_to_ziel = koord::invalid;
		}
//DBG_MESSAGE("suche_route()","no target near (%i,%i) out of %i stations!",ziel.x,ziel.y,halt_list.get_count());
		return;
	}

	// check, if the shortest connection is not right to us ...
	if(ziel_list.is_contained(self)) {
		ware.setze_ziel(pos);
		ware.setze_zwischenziel(koord::invalid);
		if(next_to_ziel!=NULL) {
			*next_to_ziel = koord::invalid;
		}
	}

	static HNode nodes[10000];
	static uint32 current_mark = 0;
	const int max_transfers = umgebung_t::max_transfers;

	INT_CHECK("simhalt 452");

	// Need to clean up ?
	if(current_mark > (1u<<31)) {
		slist_iterator_tpl<halthandle_t > halt_iter (alle_haltestellen);

		while(halt_iter.next()) {
			halt_iter.get_current()->marke = 0;
		}

		current_mark = 0;
	}

	// alle alten markierungen ungültig machen
	current_mark++;

	// die Berechnung erfolgt durch eine Breitensuche fuer Graphen
	// Warteschlange fuer Breitensuche
	static slist_tpl <HNode *> queue;
	queue.clear();

	int step = 1;
	HNode *tmp;

	nodes[0].halt = self;
	nodes[0].link = 0;
	nodes[0].depth = 0;

	queue.insert( &nodes[0] );	// init queue mit erstem feld

	self->marke = current_mark;

	// Breitensuche
	// long t0 = get_current_time_millis();

	do {
		tmp = queue.remove_first();
		const halthandle_t halt = tmp->halt;

		if(ziel_list.is_contained(halt)) {
			// ziel gefunden
			goto found;
		}

		// Hajo: check for max transfers -> don't add more stations
		//      to queue if the limit is reached
		if(tmp->depth < max_transfers) {

			// ziele prüfen
			slist_iterator_tpl<warenziel_t> iter(halt->warenziele);

			while(iter.next() && step<max_hops) {

				// check if destination if for the goods type
				warenziel_t wz = iter.get_current();

				if(wz.gib_typ()->is_interchangeable(warentyp)) {

					// since these are precalculated, they should be always pointing to a valid ground
					// (if not, we were just under construction, and will be fine after 16 steps)
					const halthandle_t tmp_halt = welt->lookup(wz.gib_ziel())->gib_halt();
					if(tmp_halt.is_bound() && tmp_halt->marke != current_mark &&  tmp_halt->is_enabled(warentyp)) {

						HNode *node = &nodes[step++];

						node->halt = tmp_halt;
						node->depth = tmp->depth + 1;
						node->link = tmp;
						queue.append( node );

						// betretene Haltestellen markieren
						tmp_halt->marke = current_mark;
					}
				}
			}

		} // max transfers
		/*
		else {
			printf("routing %s to %s -> transfer limit reached\n",
				ware.gib_name(),
				gib_halt(ware.gib_ziel())->gib_name());

		}
		*/

	} while(queue.count() && step < max_hops);

	// if the loop ends, nothing was found
	tmp = 0;

	// printf("No route found in %d steps\n", step);

found:

	// long t1 = get_current_time_millis();
	// printf("Route calc took %ld ms, %d steps\n", t1-t0, step);

	INT_CHECK("simhalt 606");

	// long t2 = get_current_time_millis();

	if(tmp) {
		// ziel gefunden
		ware.setze_ziel( tmp->halt->gib_basis_pos() );

		if(tmp->link == NULL) {
			// kein zwischenziel
			ware.setze_zwischenziel(ware.gib_ziel());
			if(next_to_ziel!=NULL) {
				// for reverse route the next hop, but not hop => enter start
//DBG_DEBUG("route","zwischenziel %s",tmp->halt->gib_name() );
				*next_to_ziel = self->gib_basis_pos();
			}
		}
		else {
			// next to start
			if(next_to_ziel!=NULL) {
				// for reverse route the next hop
				*next_to_ziel = tmp->link->halt->gib_basis_pos();
//DBG_DEBUG("route","zwischenziel %s",tmp->halt->gib_name(), start->gib_name() );
			}
			// zwischenziel ermitteln
			while(tmp->link->link) {
				tmp = tmp->link;
			}
			ware.setze_zwischenziel(tmp->halt->gib_basis_pos());
		}

		/*
		printf("route %s to %s via %s in %d steps\n",
		ware.gib_name(),
		gib_halt(ware.gib_ziel())->gib_name(),
		gib_halt(ware.gib_zwischenziel())->gib_name(),
		step);
		*/
	}
	else {
		// Kein Ziel gefunden

		ware.setze_ziel(koord::invalid);
		ware.setze_zwischenziel(koord::invalid);
		// printf("keine route zu %d,%d nach %d steps\n", ziel.x, ziel.y, step);
		if(next_to_ziel!=NULL) {
			*next_to_ziel = koord::invalid;
		}
	}
}


/**
 * Found route and station uncrowded
 * @author Hj. Malthaner
 */
void haltestelle_t::add_pax_happy(int n)
{
  pax_happy += n;
  book(n, HALT_HAPPY);
}


/**
 * Found no route
 * @author Hj. Malthaner
 */
void haltestelle_t::add_pax_no_route(int n)
{
  pax_no_route += n;
  book(n, HALT_NOROUTE);
}


/**
 * Station crowded
 * @author Hj. Malthaner
 */
void haltestelle_t::add_pax_unhappy(int n)
{
  pax_unhappy += n;
  book(n, HALT_UNHAPPY);
}




bool
haltestelle_t::add_grund(grund_t *gr)
{
	assert(gr != NULL);

	// neu halt?
	if(!grund.contains(gr)) {

		koord pos=gr->gib_pos().gib_2d();

		gr->setze_halt(self);
		grund.append(gr);
		reservation.append(0,16);

		// appends this to the ground
		// after that, the surrounding ground will know of this station
		for(  int y=-welt->gib_einstellungen()->gib_station_coverage();  y<=welt->gib_einstellungen()->gib_station_coverage();  y++ ) {
			for(  int x=-welt->gib_einstellungen()->gib_station_coverage();  x<=welt->gib_einstellungen()->gib_station_coverage();  x++ ) {
				koord p=pos+koord(x,y);
				if(welt->ist_in_kartengrenzen(p)) {
					welt->access(p)->add_to_haltlist( self );
					welt->lookup(p)->gib_kartenboden()->set_flag(grund_t::dirty);
				}
			}
		}
		welt->access(pos)->setze_halt(self);

		//DBG_MESSAGE("haltestelle_t::add_grund()","pos %i,%i,%i to %s added.",pos.x,pos.y,pos.z,gib_name());

		vector_tpl<fabrik_t *> &fablist = fabrik_t::sind_da_welche( welt,
			pos-koord(welt->gib_einstellungen()->gib_station_coverage(), welt->gib_einstellungen()->gib_station_coverage()),
			pos+koord(welt->gib_einstellungen()->gib_station_coverage(), welt->gib_einstellungen()->gib_station_coverage())
			);
		for(unsigned i=0; i<fablist.get_count(); i++) {
			fabrik_t * fab = fablist.at(i);
			if(!fab_list.contains(fab)) {
				fab_list.insert(fab);
				fab->link_halt(self);
			}
		}

		assert(gr->gib_halt() == self);
		return true;
	}
	else {
		return false;
	}
}

void
haltestelle_t::rem_grund(grund_t *gb)
{
    // namen merken
    const char *tmp = gib_name();
    if(gb) {

		int idx=grund.index_of(gb);
		if(idx==-1) {
			// was not part of station => do nothing
			dbg->error("haltestelle_t::rem_grund()","removed illegal ground from halt");
			return;
		}

		reservation.remove_at(idx);
		grund.remove(gb);

		planquadrat_t *pl = welt->access( gb->gib_pos().gib_2d() );
		if(pl) {
			// no longer connected (upper level)
			gb->setze_halt(halthandle_t());
			// still connected elsewhere?
			for(unsigned i=0;  i<pl->gib_boden_count();  i++  ) {
				if(pl->gib_boden_bei(i)->gib_halt().is_bound()) {
					// still connected => do not remove from ground ...
DBG_DEBUG("haltestelle_t::rem_grund()","keep floor, count=%i",grund.count());
					return;
				}
			}
DBG_DEBUG("haltestelle_t::rem_grund()","remove also floor, count=%i",grund.count());
			// otherwise remove ground ...
			pl->setze_halt(halthandle_t());
			pl->gib_kartenboden()->set_flag(grund_t::dirty);
		}

		for(  int y=-welt->gib_einstellungen()->gib_station_coverage();  y<=welt->gib_einstellungen()->gib_station_coverage();  y++  ) {
			for(  int x=-welt->gib_einstellungen()->gib_station_coverage();  x<=welt->gib_einstellungen()->gib_station_coverage();  x++  ) {
				planquadrat_t *pl = welt->access( gb->gib_pos().gib_2d()+koord(x,y) );
				if(pl) {
					pl->remove_from_haltlist(welt,self);
					pl->gib_kartenboden()->set_flag(grund_t::dirty);
				}
			}
		}

		if(!grund.is_empty()) {
			grund_t *bd = grund.at(0);

			if(bd->gib_text() != tmp) {
				bd->setze_text( tmp );
			}

			verbinde_fabriken();
		}
		else {
			slist_iterator_tpl <fabrik_t *> iter(fab_list);

			while( iter.next() ) {
				iter.get_current()->unlink_halt(self);
			}

			fab_list.clear();
		}
	}
}

bool
haltestelle_t::existiert_in_welt()
{
//	DBG_MESSAGE("haltestelle_t::existiert_in_welt()","count=%i",grund.count());
	if(grund.count()>0) {
//		DBG_MESSAGE("haltestelle_t::existiert_in_welt()","grund(0)=%i,%i,%i",grund.at(0)->gib_pos().x,grund.at(0)->gib_pos().y,grund.at(0)->gib_pos().z);
	}

	return !grund.is_empty();
}



/* return the closest square that belongs to this halt
 * @author prissi
 */
koord
haltestelle_t::get_next_pos( koord start ) const
{
	koord find = koord::invalid;

	if(!grund.is_empty()) {
		// find the closest one
		int	dist = 0x7FFF;
		slist_iterator_tpl<grund_t *> iter( grund );

		while(iter.next()) {
			koord p = iter.get_current()->gib_pos().gib_2d();
			int d = abs_distance(start, p );
			if(d<dist) {
				// ok, this one is closer
				dist = d;
				find = p;
			}
		}
	}
	return find;
}



bool
haltestelle_t::gibt_ab(const ware_besch_t *wtyp) const
{
  // Exact match?
  bool ok = waren.get(wtyp) != 0;

  if(!ok) {
    // Check for category match
    ptrhashtable_iterator_tpl<const ware_besch_t *, slist_tpl<ware_t> *> iter (waren);
    while (!ok && iter.next()) {
      ok = wtyp->is_interchangeable(iter.get_current_key());
    }
  }

  return ok;
}



// will load something compatible with wtyp into the car which schedule is fpl
ware_t
haltestelle_t::hole_ab(const ware_besch_t *wtyp, int maxi, fahrplan_t *fpl)
{
	// prissi: first iterate over the next stop, then over the ware
	// might be a little slower, but ensures that passengers to nearest stop are served first
	// this allows for separate high speed and normal service

	const int count = fpl->maxi();

	// da wir schon an der aktuellem haltestelle halten
	// startet die schleife ab 1, d.h. dem naechsten halt

	for(int i=1; i<count; i++) {
		const int wrap_i = (i + fpl->aktuell) % count;

		halthandle_t plan_halt = gib_halt(welt,fpl->eintrag.get(wrap_i).pos.gib_2d());

		if(plan_halt == self) {
			// we will come later here again ...
			break;
		}
		else {

			for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
				const ware_besch_t *ware = warenbauer_t::gib_info(i);
				slist_tpl<ware_t> * wliste = waren.get(ware);

				if(wliste) {
					slist_iterator_tpl<ware_t> iter (wliste);

					while(iter.next()) {
						ware_t &tmp = iter.access_current();

						// passt der Warentyp?
						bool ok = wtyp->is_interchangeable(tmp.gib_typ());

						// ok, wants to go here
						if(ok  &&  gib_halt(welt,tmp.gib_zwischenziel())==plan_halt ) {

							// not too much?
							if(tmp.menge <= maxi) {

								// ok, all can be loaded
								ware_t neu (tmp);
								bool ok = wliste->remove( tmp );
								assert(ok);

								if(wliste->count() == 0) {
									waren.remove(ware);
									delete wliste;
								}
								book(neu.menge, HALT_DEPARTED);
								resort_freight_info = true;
								return neu;

							}
							else {

								// too much, divide
								ware_t neu (tmp.gib_typ());
								neu.setze_ziel(tmp.gib_ziel());
								neu.setze_zwischenziel(tmp.gib_zwischenziel());
								neu.setze_zielpos(tmp.gib_zielpos());
								neu.menge = maxi;

								// abgegebene Menge von wartender Menge abziehen
								tmp.menge -= maxi;

								book(neu.menge, HALT_DEPARTED);
								resort_freight_info = true;
								return neu;
							}
						}
					}

					// es ist gar nichts passendes da zum abholen!
				}
			}
		}
	}

	// empty quantity of required type -> no effect
	return ware_t (wtyp);
}



int
haltestelle_t::gib_ware_summe(const ware_besch_t *typ) const
{
    int sum = 0;

    slist_tpl<ware_t> * wliste = waren.get(typ);
    if(wliste) {
	slist_iterator_tpl<ware_t> iter (wliste);

	while(iter.next()) {
	    sum += iter.get_current().menge;
	}
    }
    return sum;
}


int
haltestelle_t::gib_ware_fuer_ziel(const ware_besch_t *typ,
				  const koord ziel) const
{
  int sum = 0;

  slist_tpl<ware_t> * wliste = waren.get(typ);
  if(wliste) {
    slist_iterator_tpl<ware_t> iter (wliste);

    while(iter.next()) {
      const ware_t &ware = iter.get_current();

      if(ware.gib_ziel() == ziel) {
	sum += ware.menge;
      }
    }
  }

  return sum;
}




int
haltestelle_t::gib_ware_fuer_zwischenziel(const ware_besch_t *typ, const koord zwischenziel) const
{
  int sum = 0;

  slist_tpl<ware_t> * wliste = waren.get(typ);
  if(wliste) {
    slist_iterator_tpl<ware_t> iter (wliste);

    while(iter.next()) {
      const ware_t &ware = iter.get_current();

      if(ware.gib_zwischenziel() == zwischenziel) {
	sum += ware.menge;
      }
    }
  }

  return sum;
}



bool
haltestelle_t::vereinige_waren(const ware_t &ware)
{
        // pruefen ob die ware mit bereits wartender ware vereinigt werden kann
        slist_tpl<ware_t> * wliste = waren.get(ware.gib_typ());
        const bool is_pax = (ware.gib_typ()==warenbauer_t::passagiere  ||  ware.gib_typ()==warenbauer_t::post);

        if(wliste) {
                slist_iterator_tpl<ware_t> iter(wliste);

                while(iter.next()) {
                        ware_t &tmp = iter.access_current();

                        // es wird auf basis von Haltestellen vereinigt
                        // prissi: das ist aber ein Fehler für alle anderen Güter, daher Zielkoordinaten für alles, was kein passagier ist ...
                        if(  tmp.gib_zielpos()==ware.gib_zielpos()
                        	||  (is_pax   &&   gib_halt(welt,tmp.gib_ziel())==gib_halt(welt,ware.gib_ziel()) )
                        ) {
                                tmp.menge += ware.menge;
						resort_freight_info = true;
                                return true;
                        }
                }
        }

        return false;
}



/* same as liefere an, but there will be no route calculated, since it hase be calculated just before
 * @author prissi
 */
int
haltestelle_t::starte_mit_route(ware_t ware)
{
	// no valid next stops? Or we are the next stop?
	if(ware.gib_zwischenziel()!=koord::invalid  &&   gib_halt(welt,ware.gib_zwischenziel())==self) {
		dbg->error("haltestelle_t::starte_mit_route()","route cannot contain us as first transfer stop => recalc route!");
		suche_route(ware);
		// no route found?
		if(ware.gib_ziel()==koord::invalid) {
			dbg->error("haltestelle_t::starte_mit_route()","no route found!");
			return ware.menge;
		}
	}

	if(gib_halt(welt,ware.gib_ziel())==self) {
		if(warenbauer_t::ist_fabrik_ware(ware.gib_typ())) {
			// muss an fabrik geliefert werden
			liefere_an_fabrik(ware);
		}
		// already there: finished (may be happen with overlapping areas and returning passengers)
		return ware.menge;
	}

	// passt das zu bereits wartender ware ?
	if(vereinige_waren(ware)) {
		// dann sind wir schon fertig;
		return ware.menge;
	}

	// wenn wir hier angekommen sind, konnte die ware
	// nicht vereinigt werden, sie wird neu in die Liste
	// eingefügt
	slist_tpl<ware_t> * wliste = waren.get(ware.gib_typ());
	if(!wliste) {
		wliste = new slist_tpl<ware_t>;
		waren.set(ware.gib_typ(), wliste);
	}
	wliste->insert( ware );
	resort_freight_info = true;

	return ware.menge;
}



/* Recieves ware and tries to route it further on
 * if no route is found, it will be removed
 * @author prissi
 */
int
haltestelle_t::liefere_an(ware_t ware)
{
	// no valid next stops?
	if(ware.gib_ziel() == koord::invalid ||  ware.gib_zwischenziel() == koord::invalid) {
		// write a log entry and discard the goods
dbg->warning("haltestelle_t::liefere_an()","%d %s delivered to %s have no longer a route to their destination!", ware.menge, translator::translate(ware.gib_name()), gib_name() );
		return ware.menge;
	}

//debug
//if(ware.gib_typ()!=warenbauer_t::passagiere  &&  ware.gib_typ()!=warenbauer_t::post)
//DBG_MESSAGE("haltestelle_t::liefere_an()","%s: took %i %s",gib_name(), ware.menge, translator::translate(ware.gib_name()) );		// dann sind wir schon fertig;

	// since also the factory halt list is added to the ground, we can use just this ...
	const minivec_tpl <halthandle_t> &halt_list = welt->access(ware.gib_zielpos())->get_haltlist();

	// did we arrived?
	if(halt_list.is_contained(self)) {
		if(warenbauer_t::ist_fabrik_ware(ware.gib_typ())) {
			// muss an fabrik geliefert werden
			liefere_an_fabrik(ware);
		}
		else if(ware.gib_typ()==warenbauer_t::passagiere) {
			// arriving passenger may create pedestrians
			if(welt->gib_einstellungen()->gib_show_pax()) {
				slist_iterator_tpl<grund_t *> iter (grund);

				int menge = ware.menge;
				while(menge > 0 && iter.next()) {
					grund_t *gr = iter.get_current();
					menge = erzeuge_fussgaenger(welt, gr->gib_pos(), menge);
				}

				INT_CHECK("simhalt 938");
			}
		}
		return ware.menge;
	}

	// passt das zu bereits wartender ware ?
	if(vereinige_waren(ware)) {
		return ware.menge;
	}

	// not near enough => we need to do a rerouting
	suche_route(ware);

	// target no longer there => delete
	if(!gib_halt(welt,ware.gib_ziel()).is_bound() ||  !gib_halt(welt,ware.gib_zwischenziel()).is_bound()) {
		DBG_MESSAGE("haltestelle_t::liefere_an()","%s: delivered goods (%d %s) to ??? via ??? could not be routed to their destination!",gib_name(), ware.menge, translator::translate(ware.gib_name()) );
		return ware.menge;
	}

	// passt das zu bereits wartender ware ?
	if(vereinige_waren(ware)) {
		// dann sind wir schon fertig;
		return ware.menge;
	}

	// wenn wir hier angekommen sind, konnte die ware
	// nicht vereinigt werden, sie wird neu in die Liste
	// eingefügt
	slist_tpl<ware_t> * wliste = waren.get(ware.gib_typ());
	if(!wliste) {
		wliste = new slist_tpl<ware_t>;
		waren.set(ware.gib_typ(), wliste);
	}
	wliste->insert( ware );
	resort_freight_info = true;

	return ware.menge;
}



/* true, if there is a conncetion between these places
 * @author prissi
 */
bool
haltestelle_t::is_connected(const halthandle_t halt, const ware_besch_t * wtyp)
{
	slist_iterator_tpl<warenziel_t> iter(warenziele);
	while(iter.next()) {
		warenziel_t &tmp = iter.access_current();
		if(tmp.gib_typ()->is_interchangeable(wtyp) && gib_halt(welt,tmp.gib_ziel())==halt) {
			return true;
		}
	}
	return true;
}



void
haltestelle_t::hat_gehalten(int /*number_of_cars*/,const ware_besch_t *type, const fahrplan_t *fpl)
{
	if(type != warenbauer_t::nichts) {
		for(int i=0; i<fpl->maxi(); i++) {

			// Hajo: Haltestelle selbst wird nicht in Zielliste aufgenommen
			halthandle_t halt = gib_halt(welt,fpl->eintrag.get(i).pos);
			// Hajo: Nicht existierende Ziele (wegpunkte) werden übersprungen
			if(!halt.is_bound()  ||  halt==self) {
				continue;
			}
			// we need to do this here; otherwise the position of the stop (if in water) may not directly be a halt!
			const warenziel_t wz (halt->gib_basis_pos(), type);

			slist_iterator_tpl<warenziel_t> iter(warenziele);
			while(iter.next()) {
				warenziel_t &tmp = iter.access_current();
				// since we only have valid coordinates here, we can also directly look up the handle (not using gib_halt)
				if(tmp.gib_typ()->is_interchangeable(type) &&  welt->lookup(tmp.gib_ziel())->gib_halt()==welt->lookup(wz.gib_ziel())->gib_halt()) {
					goto skip;
				}
			}

			warenziele.insert(wz);
			skip:;
		}
	}
}



/* checks, if there is an unoccupied loading bay for this kind of thing
 * @author prissi
 */
bool
haltestelle_t::find_free_position(const waytype_t w,convoihandle_t cnv,const ding_t::typ d) const
{
	// iterate over all tiles
	slist_iterator_tpl<grund_t *> iter( grund );
	unsigned i=0;
	while(iter.next()) {
		if(reservation.get(i)==cnv  ||  !reservation.get(i).is_bound()) {
			// not reseved
			grund_t *gr = iter.get_current();
			if(gr) {
				// found a stop for this waytype but without object d ...
				const weg_t *weg=gr->gib_weg(w);
				if(weg  &&  gr->suche_obj(d)==NULL) {
					// not occipied
					return true;
				}
				else if(reservation.get(i)==cnv) {
					reservation.at(i) = convoihandle_t();
				}
			}
			else {
				dbg->error("haltestelle_t::find_free_position()","contains zero ground!");
			}
		}
		i++;
	}
	return false;
}



/* reserves a position (caution: railblocks work differently!
 * @author prissi
 */
bool
haltestelle_t::reserve_position(grund_t *gr,convoihandle_t cnv)
{
	int idx=grund.index_of(gr);
	if(idx>=0) {
		if(reservation.get(idx)==cnv) {
//DBG_MESSAGE("haltestelle_t::reserve_position()","gr=%d,%d already reserved by cnv=%d",gr->gib_pos().x,gr->gib_pos().y,cnv.get_id());
			return true;
		}
		// not reseved
		if(!reservation.get(idx).is_bound()) {
			grund_t *gr = grund.at(idx);
			if(gr) {
				// found a stop for this waytype but without object d ...
				const weg_t *weg=gr->gib_weg(cnv->gib_vehikel(0)->gib_wegtyp());
				if(weg  &&  gr->suche_obj(cnv->gib_vehikel(0)->gib_typ())==NULL) {
					// not occipied
//DBG_MESSAGE("haltestelle_t::reserve_position()","sucess for gr=%i,%i cnv=%d",gr->gib_pos().x,gr->gib_pos().y,cnv.get_id());
					reservation.at(idx) = cnv;
					return true;
				}
			}
		}
	}
//DBG_MESSAGE("haltestelle_t::reserve_position()","failed for gr=%i,%i, cnv=%d",gr->gib_pos().x,gr->gib_pos().y,cnv.get_id());
	return false;
}



/* frees a reserved  position (caution: railblocks work differently!
 * @author prissi
 */
bool
haltestelle_t::unreserve_position(grund_t *gr, convoihandle_t cnv)
{
	int idx=grund.index_of(gr);
	if(idx>=0) {
		if(reservation.get(idx)==cnv) {
			reservation.at(idx) = convoihandle_t();
			return true;
		}
	}
DBG_MESSAGE("haltestelle_t::unreserve_position()","failed for gr=%p",gr);
	return false;
}



/* can a convoi reserve this position?
 * @author prissi
 */
bool
haltestelle_t::is_reservable(grund_t *gr,convoihandle_t cnv)
{
	int idx=grund.index_of(gr);
	if(idx>=0) {
		if(reservation.get(idx)==cnv) {
DBG_MESSAGE("haltestelle_t::is_reservable()","gr=%d,%d already reserved by cnv=%d",gr->gib_pos().x,gr->gib_pos().y,cnv.get_id());
			return true;
		}
		// not reseved
		if(!reservation.get(idx).is_bound()) {
			grund_t *gr = grund.at(idx);
			if(gr) {
				// found a stop for this waytype but without object d ...
				const weg_t *weg=gr->gib_weg(cnv->gib_vehikel(0)->gib_wegtyp());
				if(weg  &&  gr->suche_obj(cnv->gib_vehikel(0)->gib_typ())==NULL) {
					// not occipied
DBG_MESSAGE("haltestelle_t::is_reservable()","sucess for gr=%i,%i cnv=%d",gr->gib_pos().x,gr->gib_pos().y,cnv.get_id());
					return true;
				}
			}
		}
	}
DBG_MESSAGE("haltestelle_t::reserve_position()","failed for gr=%i,%i, cnv=%d",gr->gib_pos().x,gr->gib_pos().y,cnv.get_id());
	return false;
}



#ifdef USE_QUOTE
// rating of this place ...
const char *
haltestelle_t::quote_bezeichnung(int quote, convoihandle_t cnv) const
{
    const char *str = "unbekannt";

    if(quote < 0) {
	str = translator::translate("miserabel");
    } else if(quote < 30) {
	str = translator::translate("schlecht");
    } else if(quote < 60) {
	str = translator::translate("durchschnitt");
    } else if(quote < 90) {
	str = translator::translate("gut");
    } else if(quote < 120) {
	str = translator::translate("sehr gut");
    } else if(quote < 150) {
	str = translator::translate("bestens");
    } else if(quote < 180) {
	str = translator::translate("excellent");
    } else {
	str = translator::translate("spitze");
    }

    return str;
}
#endif



void haltestelle_t::info(cbuffer_t & buf) const
{
  char tmp [512];

  sprintf(tmp,
	  translator::translate("Passengers %d %c, %d %c, %d no route"),
	  pax_happy,
	  30,
	  pax_unhappy,
	  31,
	  pax_no_route,
	  get_capacity()
	  );
	buf.append(tmp);
}


/**
 * @param buf the buffer to fill
 * @return Goods description text (buf)
 * @author Hj. Malthaner
 */
void haltestelle_t::get_freight_info(cbuffer_t & buf)
{
	if(resort_freight_info) {
		// resort only inf absolutely needed ...
		resort_freight_info = false;
		buf.clear();

		for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
			const ware_besch_t *wtyp = warenbauer_t::gib_info(i);
			slist_tpl<ware_t> * wliste = waren.get(wtyp);
			if(wliste) {
				freight_list_sorter_t::sort_freight( welt, wliste, buf, (freight_list_sorter_t::sort_mode_t)sortierung, NULL, "waiting" );
			}
		}
	}
}


void haltestelle_t::get_short_freight_info(cbuffer_t & buf)
{
	bool got_one = false;

	for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
		const ware_besch_t *wtyp = warenbauer_t::gib_info(i);
		if(gibt_ab(wtyp)) {

			// ignore goods with sum=zero
			const int summe=gib_ware_summe(wtyp);
			if(summe>0) {

				if(got_one) {
					buf.append(", ");
				}

				buf.append(summe);
				buf.append(translator::translate(wtyp->gib_mass()));
				buf.append(" ");
				buf.append(translator::translate(wtyp->gib_name()));

				got_one = true;
			}
		}
	}

	if(got_one) {
		buf.append(" ");
		buf.append(translator::translate("waiting"));
		buf.append("\n");
	}
	else {
		buf.append(translator::translate("no goods waiting"));
		buf.append("\n");
	}
}


void
haltestelle_t::zeige_info()
{
    // sync name with ground
    access_name();
    // open window

    if(halt_info == 0) {
		halt_info = new halt_info_t(welt, self);
    }

    create_win(-1, -1, halt_info, w_info);
}


void
haltestelle_t::open_detail_window()
{
    create_win(-1, -1, new halt_detail_t(besitzer_p, welt, self), w_autodelete);
}


/**
 * @returns the sum of all waiting goods (100t coal + 10
 * passengers + 2000 liter oil = 2110)
 * @author Markus Weber
 */
int haltestelle_t::sum_all_waiting_goods() const      //15-Feb-2002    Markus Weber    Added
{
    int sum = 0;

    for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
	const ware_besch_t *wtyp = warenbauer_t::gib_info(i);

        if(gibt_ab(wtyp)) {
            sum += gib_ware_summe(wtyp);
        }
    }
    return sum;
}


bool
haltestelle_t::is_something_waiting() const
{
    for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
	const ware_besch_t *wtyp = warenbauer_t::gib_info(i);

        if(gibt_ab(wtyp)) {
	    return true;
        }
    }
    return false;
}



// changes this to a publix transfer exchange stop
void
haltestelle_t::transfer_to_public_owner()
{
	spieler_t *public_owner=welt->gib_spieler(1);
	if(besitzer_p==public_owner) {
		// already a public stop
		return;
	}

	// iterate over all tiles
	slist_iterator_tpl<grund_t *> iter( grund );
	while(iter.next()) {
		grund_t *gr = iter.get_current();
		gebaeude_t *gb = static_cast<gebaeude_t *>(gr->suche_obj(ding_t::gebaeude));
		if(gb) {
			// there are also water tiles, which may not have a buidling
			spieler_t *gb_sp=gb->gib_besitzer();
			if(public_owner!=gb_sp) {
				gb_sp->add_maintenance(-umgebung_t::maint_building);
				gb_sp->buche(umgebung_t::maint_building*36*(gb->gib_tile()->gib_besch()->gib_level()+1), gr->gib_pos().gib_2d(), COST_CONSTRUCTION);
				gb->setze_besitzer(public_owner);
				public_owner->add_maintenance(umgebung_t::maint_building);
			}
		}
	}
	besitzer_p->halt_remove(self);
	besitzer_p = public_owner;
	public_owner->halt_add(self);
}



/*
 * recalculated the station type(s)
 * since it iterates over all ground, this is better not done too often, because line management and station list
 * queries this information regularely; Thus, we do this, when adding new ground
 * This recalculates also the capacity from the building levels ...
 * @author Weber/prissi
 */
void
haltestelle_t::recalc_station_type()
{
	slist_iterator_tpl<grund_t *> iter(grund);
	int new_station_type = 0;
	capacity = 0;
	enables &= CROWDED;	// clear flags

	// iterate over all tiles
	while(iter.next()) {
		grund_t *gr = iter.get_current();
		gebaeude_t *gb = static_cast<gebaeude_t *>(gr->suche_obj(ding_t::gebaeude));
		const haus_besch_t *besch=gb?gb->gib_tile()->gib_besch():NULL;

		if(gr->ist_wasser()) {
			// may happend around oil rigs and so on
			new_station_type |= dock;
			// for water factories
			if(besch) {
				enables |= besch->get_enabled();
				capacity += besch->gib_level();
				DBG_MESSAGE("haltestelle_t::recalc_station_type()","factory enables %i",besch->get_enabled());
			}
			continue;
		}

		if(besch==NULL) {
			// no besch, but solid gound?!?
			dbg->error("haltestelle_t::get_station_type()","ground belongs to halt but no besch?");
			continue;
		}
//if(besch) DBG_DEBUG("haltestelle_t::get_station_type()","besch(%p)=%s",besch,besch->gib_name());

		// there is only one loading bay ...
		if(besch->gib_utyp()==hausbauer_t::ladebucht) {
			new_station_type |= loadingbay;
		}
		// check for trainstation
		else if(besch->gib_utyp()==hausbauer_t::bahnhof) {
			if(gr->gib_weg(monorail_wt)) {
				new_station_type |= monorailstop;
			}
			else {
				new_station_type |= railstation;
			}
		}
		// check for habour
		else if(besch->gib_utyp()==hausbauer_t::hafen  ||  besch->gib_utyp()==hausbauer_t::binnenhafen) {
			new_station_type |= dock;
		}
		// check for bus
		else if(besch->gib_utyp()==hausbauer_t::bushalt) {
			new_station_type |= busstop;
		}
		// check for airport
		else if(besch->gib_utyp()==hausbauer_t::airport) {
			new_station_type |= airstop;
		}
		// check for trainstation
		else if(besch->gib_utyp()==hausbauer_t::monorailstop) {
			new_station_type |= monorailstop;
		}

		// enabled the matching types
		enables |= besch->get_enabled();
		capacity += besch->gib_level();

	}
	station_type = (haltestelle_t::stationtyp)new_station_type;

//DBG_DEBUG("haltestelle_t::recalc_station_type()","result=%x, capacity=%i",new_station_type,capacity);
}



const char *
haltestelle_t::name_from_ground() const
{
	const char *name = "Unknown";
	if(grund.is_empty()) {
		name = "Unnamed";
	}
	else {
		grund_t *bd = grund.at(0);

		if(bd != NULL && bd->gib_text() != NULL) {
			name = bd->gib_text();
		}
	}

	return name;
}

char *
haltestelle_t::access_name()
{
	if(grund.count()>0) {
		if(name!=name_from_ground()) {
			tstrncpy(name, name_from_ground(), 128);
			grund_t *bd = grund.at(0);
			if(bd!=NULL) {
				bd->setze_text(name);
			}
		}
	}

	return name;
}


const char *
haltestelle_t::gib_name() const
{
    return name_from_ground();
}


int
haltestelle_t::erzeuge_fussgaenger(karte_t *welt, koord3d pos, int anzahl)
{
	fussgaenger_t::erzeuge_fussgaenger_an(welt, pos, anzahl);
	for(int i=0; i<4 && anzahl>0; i++) {
		fussgaenger_t::erzeuge_fussgaenger_an(welt, pos+koord::nsow[i], anzahl);
	}
	return anzahl;
}


void
haltestelle_t::rdwr(loadsave_t *file)
{
	int spieler_n;
	koord3d k;

	if(file->is_saving()) {
		spieler_n = welt->sp2num( besitzer_p );
	}
	pos.rdwr( file );
	file->rdwr_long(spieler_n, "\n");

	if(file->get_version()<=88005) {
		bool dummy;
		file->rdwr_bool(dummy, " "); // pax
		file->rdwr_bool(dummy, " "); // post
		file->rdwr_bool(dummy, "\n");	// ware
	}

	if(file->is_loading()) {
		besitzer_p = welt->gib_spieler(spieler_n);
		do {
			k.rdwr( file );
			if( k!=koord3d::invalid) {
				grund_t *gr = welt->lookup(k);
				if(!gr) {
					gr = welt->lookup(k.gib_2d())->gib_kartenboden();
					dbg->warning("haltestelle_t::rdwr()", "invalid position %s (setting to ground %s)\n",
					k3_to_cstr(k).chars(),
					k3_to_cstr(gr->gib_pos()).chars());
				}
				// prissi: now check, if there is a building -> we allow no longer ground without building!
				gebaeude_t *gb = static_cast<gebaeude_t *>(gr->suche_obj(ding_t::gebaeude));
				const haus_besch_t *besch=gb?gb->gib_tile()->gib_besch():NULL;
				if(besch) {
					add_grund(gr);
				}
				else {
					dbg->warning("haltestelle_t::rdwr()", "will no longer add ground without building at %s!",k3_to_cstr(k).chars());
				}
			}
		} while(k!=koord3d::invalid);
    } else {
	slist_iterator_tpl<grund_t*> gr_iter ( grund );

	while(gr_iter.next()) {
	    k = gr_iter.get_current()->gib_pos();
	    k.rdwr( file );
	}
	k = koord3d::invalid;
	k.rdwr( file );
    }

    short count;
    const char *s;

    if(file->is_saving()) {
      for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
	const ware_besch_t *ware = warenbauer_t::gib_info(i);
	slist_tpl<ware_t> * wliste = waren.get(ware);

	if(wliste) {
	  s = ware->gib_name();
	  file->rdwr_str(s, "N");

	  count = wliste ? wliste->count() : 0;
	  file->rdwr_short(count, " ");
	  if(wliste) {
	    slist_iterator_tpl<ware_t> wliste_iter(wliste);
	    while(wliste_iter.next()) {
	      ware_t ware = wliste_iter.get_current();
	      ware.rdwr(file);
	    }
	  }
	}
      }
      s = "";
      file->rdwr_str(s, "N");


      count = warenziele.count();
      file->rdwr_short(count, " ");

      slist_iterator_tpl<warenziel_t>ziel_iter(warenziele);
      while(ziel_iter.next()) {
	warenziel_t wz = ziel_iter.get_current();
	wz.rdwr(file);
      }

    } else {
      s = NULL;
      file->rdwr_str(s, "N");
      while(s && *s) {
	const ware_besch_t *ware = warenbauer_t::gib_info(s);

	file->rdwr_short(count, " ");
	if(count) {
	  slist_tpl<ware_t> *wlist = new slist_tpl<ware_t>;

	  for(int i = 0; i < count; i++) {
	    ware_t ware(file);
	    if(ware.menge>0) {
	    	wlist->insert(ware);
	    }
	  }
	  count = wlist->count();
	  if(count>0) {
		  waren.put(ware, wlist);
	}
	}
	file->rdwr_str(s, "N");
      }

      file->rdwr_short(count, " ");

      for(int i=0; i<count; i++) {
	warenziel_t wz (file);
	warenziele.append(wz);
      }

      guarded_free(const_cast<char *>(s));
    }

	for (int j = 0; j<MAX_HALT_COST; j++)
	{
		for (int k = MAX_MONTHS-1; k>=0; k--)
		{
			file->rdwr_longlong(financial_history[k][j], " ");
		}
	}
}


void
haltestelle_t::laden_abschliessen()
{
  recalc_station_type();
#ifdef LAGER_NOT_IN_USE
    slist_iterator_tpl<grund_t*> iter( grund );

    while(iter.next()) {
	koord3d k ( iter.get_current()->gib_pos() );

	// nach sondergebaeuden suchen

	ding_t *dt = welt->lookup(k)->suche_obj(ding_t::lagerhaus);

	if(dt != NULL) {
	    lager = dynamic_cast<lagerhaus_t *>(dt);
	    break;
	}
    }
#endif
}

void
haltestelle_t::book(sint64 amount, int cost_type)
{
	if (cost_type > MAX_HALT_COST)
	{
		// THIS SHOULD NEVER HAPPEN!
		// CHECK CODE
		dbg->warning("haltestelle_t::book()", "function was called with cost_type: %i, which is not valid (MAX_HALT_COST=%i)", cost_type, MAX_HALT_COST);
		return;
	}
	financial_history[0][cost_type] += amount;
	financial_history[0][HALT_WAITING] = sum_all_waiting_goods();
}

void
haltestelle_t::init_financial_history()
{
	for (int j = 0; j<MAX_HALT_COST; j++)
	{
		for (int k = MAX_MONTHS-1; k>=0; k--)
		{
			financial_history[k][j] = 0;
		}
	}
	financial_history[0][HALT_HAPPY] = pax_happy;
	financial_history[0][HALT_UNHAPPY] = pax_unhappy;
	financial_history[0][HALT_NOROUTE] = pax_no_route;
}
