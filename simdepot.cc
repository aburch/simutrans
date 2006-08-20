/*
 * simgui.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "simvehikel.h"
#include "simwin.h"
#include "simware.h"
#include "simhalt.h"
#include "simconvoi.h"
#include "simplay.h"
#include "simworld.h"
#include "blockmanager.h"
#include "simio.h"
#include "simdepot.h"
#include "simline.h"
#include "simlinemgmt.h"

#include "gui/depot_frame.h"
#include "gui/messagebox.h"

#include "dataobj/fahrplan.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"

#include "bauer/vehikelbauer.h"


depot_t::depot_t(karte_t *welt) :
    gebaeude_t(welt)
{
    depot_info = NULL;
}


depot_t::depot_t(karte_t *welt, koord3d pos, spieler_t *sp, const haus_tile_besch_t *t) :
    gebaeude_t(welt, pos, sp, t)
{
    depot_info = NULL;
}


depot_t::~depot_t()
{
    if(depot_info) {
	destroy_win(depot_info);
	delete depot_info;
	depot_info = NULL;
    }
    slist_iterator_tpl<convoihandle_t> iter(convois);
    while(iter.next()) {
	iter.access_current().unbind();
    }
    convois.clear();
}


void
depot_t::convoi_arrived(convoihandle_t acnv, bool fpl_adjust)
{
  for(int i=0; i< acnv->gib_vehikel_anzahl(); i++) {

    vehikel_t *v = acnv->gib_vehikel(i);

    if(v) {
      // Hajo: reset vehikel data
      v->loesche_fracht();
      v->setze_fahrtrichtung( ribi_t::sued );
    }
  }
  if(fpl_adjust) {
      // Volker: remove depot from schedule
      fahrplan_t *fpl = acnv->gib_fahrplan();
      fpl->remove();
      acnv->setze_fahrplan(fpl);
  }
  convois.append(acnv);
  if(depot_info) {
      depot_info->action_triggered(NULL);
  }

  dbg->message("depot_t::convoi_arrived()", "convoi %d, %p entered depot",
	       acnv.get_id(), acnv.get_rep());
}


void
depot_t::zeige_info()
{
  if(depot_info==NULL) {
    depot_info = new depot_frame_t(welt, this, gib_besitzer()->kennfarbe);
  } else {
    depot_info->build_vehicle_lists();
  }

  create_win(20, 20, depot_info, w_info);
}



bool depot_t::can_convoi_start(int /*icnv*/) const
{
  return true;
}




int depot_t::buy_vehicle(int image)
{
    if(image != -1) {
	// Offen: prüfen ob noch platz im depot ist???
	const vehikel_besch_t * info = vehikelbauer_t::gib_info(image);

        vehikel_t *veh = vehikelbauer_t::baue(welt, gib_pos(), gib_besitzer(), NULL, info);

        if(veh) {
	    vehicles.append(veh);

	    return vehicles.count() - 1;
	}
    }
    return -1;
}



void depot_t::append_vehicle(int icnv, int iveh, bool infront)
{
    vehikel_t *veh = get_vehicle(iveh);

    if(veh) {
	convoihandle_t cnv = get_convoi(icnv);
	/*
	 *  create  a new convoi, if necessary
	 */
	if(!cnv.is_bound()) {
	    cnv = add_convoi();
	}

	veh->setze_pos(gib_pos());
	cnv->add_vehikel(veh, infront);

	vehicles.remove(veh);
    }
}



void depot_t::remove_vehicle(int icnv, int ipos)
{
    convoihandle_t cnv = get_convoi(icnv);

    if(cnv.is_bound()) {
	vehikel_t *veh = cnv->remove_vehikel_bei(ipos);

	if(veh) {
	    vehicles.append(veh);
	}
    }
}



void depot_t::sell_vehicle(int iveh)
{
    vehikel_t *veh = get_vehicle(iveh);

    if(veh) {
	vehicles.remove(veh);
	gib_besitzer()->buche(veh->calc_restwert(), gib_pos().gib_2d(), COST_NEW_VEHICLE);
	delete veh;
    }
}


convoihandle_t depot_t::add_convoi()
{
    convoi_t *new_cnv = new convoi_t(welt, gib_besitzer());

    convois.append(new_cnv->self);

    return new_cnv->self;
}

convoihandle_t depot_t::copy_convoi(int icnv)
{
    convoihandle_t old_cnv = get_convoi(icnv);
    if (old_cnv.is_bound()) {
	    convoihandle_t new_cnv = add_convoi();
	    char new_name[128];
	    sprintf(new_name, "%s", old_cnv->gib_name());
	    new_cnv->setze_name(new_name);
			int vehicle_count = old_cnv->gib_vehikel_anzahl();
			for (int i = 0; i<vehicle_count; i++) {
				const vehikel_besch_t * info = old_cnv->gib_vehikel(i)->gib_besch();
				if (info != NULL) {
					// search in depot for an existing vehicle of correct type
					int oldest_vehicle = get_oldest_vehicle(old_cnv->gib_vehikel(i)->gib_basis_bild());
					if (oldest_vehicle != -1) {
						// append existing vehicle
						append_vehicle(convoi_count()-1, oldest_vehicle, false);
					} else {
						// buy new vehicle
			    	vehikel_t *veh = vehikelbauer_t::baue(welt, gib_pos(), gib_besitzer(), NULL, info);
						veh->setze_pos(gib_pos());
						new_cnv->add_vehikel(veh, false);
			    }
				}
			}
			if (old_cnv->has_line()) {
				new_cnv->set_line(old_cnv->get_line());
			} else {
				if (old_cnv->gib_fahrplan() != NULL) {
					new_cnv->setze_fahrplan(old_cnv->gib_fahrplan()->copy());
				}
			}
	    return new_cnv->self;
	  }
	return NULL;
}

bool depot_t::disassemble_convoi(int icnv, bool sell)
{
    convoihandle_t cnv = get_convoi(icnv);

    if(cnv.is_bound()) {
	if(cnv->gib_vehikel(0)) {
	    vehikel_t *v = NULL;

	    if(sell) {
		gib_besitzer()->buche(cnv->calc_restwert(), gib_pos().gib_2d(), COST_NEW_VEHICLE);
	    }
	    do {
		v = cnv->remove_vehikel_bei(0);

		if( v ) {

		    if(sell) {
			delete v;
		    } else {
			// Hajo: reset vehikel data
			v->loesche_fracht();
			v->setze_fahrtrichtung( ribi_t::sued );
			v->setze_erstes(false);
			v->setze_letztes(false);
			vehicles.append(v);
		    }
		}
	    } while(v != NULL);
	}

	// Hajo: update all stations from schedule that this convoi des not
	// drive any more
	fahrplan_t * fpl = cnv->gib_fahrplan();
	if(fpl) {

	  for(int i=0; i<=fpl->maxi; i++) {

	    halthandle_t halt;

	    dbg->message("depot_t::convoi_aufloesen()", "Unlinking schedule entry %d", i);

	    // Hajo: Hält dieser convoi hier?
	    if( (halt = haltestelle_t::gib_halt(welt, fpl->eintrag.get(i).pos.gib_2d())).is_bound() ) {
	      halt->rebuild_destinations();
	    }
	  }
	}

	convois.remove_at(icnv);
	// Hajo: destruktor also removes convoi from convoi list
	delete cnv.detach();
	return true;
    }
    return false;
}


bool
depot_t::start_convoi(int icnv)
{
    convoihandle_t cnv = get_convoi(icnv);

    if(cnv.is_bound() &&
       cnv->gib_fahrplan() != NULL &&
       cnv->gib_fahrplan()->ist_abgeschlossen() &&
       cnv->gib_fahrplan()->maxi >= 0) {

	// pruefen ob zug vollstaendig
	if(cnv->gib_sum_leistung() == 0 || !cnv->pruefe_alle()) {
	    create_win(100, 64, MESG_WAIT, new nachrichtenfenster_t(welt, "Diese Zusammenstellung kann nicht fahren!\n"), w_autodelete);
	} else {

	    if(can_convoi_start(icnv)) {

		// Hajo: alle Haltestellen auf dem Fahrplan werden informiert,
		// daß sie jetzt angefahren werden.
		fahrplan_t *fpl = cnv->gib_fahrplan();

		for(int i=0; i<=fpl->maxi; i++) {
		    halthandle_t halt = haltestelle_t::gib_halt(welt, fpl->eintrag.at(i).pos.gib_2d());

		    if(halt.is_bound()) {
			for(int j=0; j<cnv->gib_vehikel_anzahl(); j++) {
			    halt->hat_gehalten(0, cnv->gib_vehikel(j)->gib_fracht_typ(), fpl);
			}
		    }
		}

		// Hajo: OK, die Haltestellen sind informiert,
		// der Convoi kann losdüsen
		cnv->setze_fahrplan( fpl );     // das ist nicht redundant, nicht löschen!!!
		welt->sync_add( cnv.get_rep() );
		cnv->start();

		// Hajo: don't wait for map step to trigger the first vehicle
		// do that step immediately to shorten the delay until vehicle
		// starts to move. There are three steps until the route is
		// found
		cnv->step();
		cnv->sync_step(20);
		cnv->step();

		convois.remove_at(icnv);
		return true;
	    } else {
		create_win(100, 64, new nachrichtenfenster_t(welt, "Blockstrecke ist\nbelegt\n"), w_autodelete);
	    }
	}
    } else {
	create_win(100, 64, new nachrichtenfenster_t(welt, "Noch kein Fahrzeug\nmit Fahrplan\nvorhanden\n"), w_autodelete);

	if(!cnv.is_bound()) {
	    dbg->warning("depot_t::starte_convoi()",
			 "No convoi to start!");
	}
	if(cnv.is_bound() && cnv->gib_fahrplan() == NULL) {
	    dbg->warning("depot_t::starte_convoi()",
			 "No schedule for convoi.");
	}
	if(cnv.is_bound() &&
	   cnv->gib_fahrplan() != NULL &&
	   cnv->gib_fahrplan()->ist_abgeschlossen() == false) {
	    dbg->warning("depot_t::starte_convoi()",
			 "Schedule is incomplete/not finished");
	}
    }
    return false;
}



void
depot_t::rdwr_vehikel(slist_tpl<vehikel_t *> &list, loadsave_t *file)
{
    int count;
    char buf[80];

    if(file->is_saving()) {
        count = list.count();
    }
    file->rdwr_int(count, "\n");

    if(file->is_loading()) {
        for(int i=0; i<count; i++) {
            ding_t::typ typ = (ding_t::typ)file->rd_obj_id();

	    vehikel_t *v = NULL;

	    switch( typ ) {
	    case automobil: v = new automobil_t(welt, file);    break;
	    case waggon:    v = new waggon_t(welt, file);       break;
	    case schiff:    v = new schiff_t(welt, file);       break;
	    default:
	        dbg->fatal("depot_t::vehikel_laden()",
		           "invalid vehicle type %s", buf);
	    }

	    dbg->message("depot_t::vehikel_laden()",
			 "loaded %s", v->gib_besch()->gib_name());

	    list.insert( v );
        }
    }
    else {
        slist_iterator_tpl<vehikel_t *> l_iter ( list);

        while(l_iter.next()) {
	    l_iter.get_current()->rdwr( file );
        }
    }
}


void
depot_t::rdwr(loadsave_t *file)
{
    gebaeude_t::rdwr(file);

    if(file->get_version() < 81033) {
	slist_tpl<vehikel_t *> waggons;

	rdwr_vehikel(vehicles, file);
	rdwr_vehikel(waggons, file);

	while(waggons.count() > 0) {
	    vehicles.append(waggons.at(0));
	    waggons.remove_at(0);
	}
    } else {
	rdwr_vehikel(vehicles, file);
    }
}


/**
 * @returns NULL wenn OK, ansonsten eine Fehlermeldung
 * @author Hj. Malthaner
 */
const char * depot_t::ist_entfernbar(const spieler_t *)
{
    if(vehicles.count()) {
        return "There are still vehicles\nstored in this depot!\n";
    }
    slist_iterator_tpl<convoihandle_t> iter(convois);

    while(iter.next()) {
	if(iter.get_current()->gib_vehikel_anzahl() > 0) {
	    return "There are still convois\nstored in this depot!\n";
	}
    }
    return NULL;
}

void
depot_t::build_line_list() {
	welt->simlinemgmt->build_line_list(simline_t::line, &lines);
}

int
depot_t::get_oldest_vehicle(int id)
{
	int oldest_veh = -1;
	long insta_time_old = 0;

	int i = 0;
	slist_iterator_tpl<vehikel_t *> iter(get_vehicle_list());
	while(iter.next()) {
		if(iter.get_current()->gib_basis_bild() == id) {
			if(oldest_veh == -1 || insta_time_old > iter.get_current()->gib_insta_zeit()) {
				oldest_veh = i;
				insta_time_old = iter.get_current()->gib_insta_zeit();
			}
		}
		i++;
	}
	return oldest_veh;
}


bahndepot_t::bahndepot_t(karte_t *welt, loadsave_t *file) : depot_t(welt)
{
  rdwr(file);
}


bahndepot_t::bahndepot_t(karte_t *welt, koord3d pos,spieler_t *sp, const haus_tile_besch_t *t) : depot_t(welt, pos, sp, t)
{

}


void
bahndepot_t::convoi_arrived(convoihandle_t cnv, bool fpl_adjust)
{
    depot_t::convoi_arrived(cnv, fpl_adjust);

    blockmanager::gib_manager()->pruefe_blockstrecke(welt, gib_pos());
}



/*fahrplan_t *
bahndepot_t::erzeuge_fahrplan()
{
    return new zugfahrplan_t();
}
*/
fahrplan_t *
bahndepot_t::erzeuge_fahrplan(fahrplan_t * fpl)
{
    return new zugfahrplan_t(fpl);
}

simline_t *
bahndepot_t::create_line(karte_t * welt)
{
    return welt->simlinemgmt->create_line(simline_t::trainline);
}

bool
bahndepot_t::can_convoi_start(int icnv) const
{
    bool ok = false;

    // teste, ob abfahrt erlaubt
    blockmanager *bm = blockmanager::gib_manager();
    blockhandle_t bs = bm->finde_blockstrecke(welt, gib_pos());

    // prüfe ob blockstrecke frei
    if(bs.get_id() >= 0 && bs->ist_frei()) {
	// blockstrecke ist frei, wir können starten

	// simuliere überfahren des ersten feldes der blockstrecke
	int i = 0;

	do {
	    vehikel_t *v = get_convoi(icnv)->gib_vehikel( i++ );

	    bs->betrete( v );
	} while(get_convoi(icnv)->gib_vehikel( i ) != NULL);

	ok = true;
    }
    return ok;
}


const vehikel_besch_t *bahndepot_t::get_vehicle_type(int itype)
{
    return vehikelbauer_t::gib_info(vehikel_besch_t::schiene, itype);
}


const char *
bahndepot_t::gib_name() const
{
    return "Bahndepot";
}

void
bahndepot_t::build_line_list() {
	welt->simlinemgmt->build_line_list(simline_t::trainline, &lines);
}

slist_tpl<simline_t *> *
bahndepot_t::get_line_list()
{
	build_line_list();
	return &lines;
}


strassendepot_t::strassendepot_t(karte_t *welt, loadsave_t *file) : depot_t(welt)
{
  rdwr(file);
}


strassendepot_t::strassendepot_t(karte_t *welt, koord3d pos,spieler_t *sp, const haus_tile_besch_t *t) : depot_t(welt, pos, sp, t)
{

}


/*fahrplan_t *
strassendepot_t::erzeuge_fahrplan()
{
    return new autofahrplan_t();
}
*/
fahrplan_t *
strassendepot_t::erzeuge_fahrplan(fahrplan_t * fpl)
{
    return new autofahrplan_t(fpl);
}

simline_t *
strassendepot_t::create_line(karte_t * welt)
{
    return welt->simlinemgmt->create_line(simline_t::truckline);
}

const vehikel_besch_t *strassendepot_t::get_vehicle_type(int itype)
{
    return vehikelbauer_t::gib_info(vehikel_besch_t::strasse, itype);
}


const char *
strassendepot_t::gib_name() const
{
    return "Strassendepot";
}

void
strassendepot_t::build_line_list() {
	welt->simlinemgmt->build_line_list(simline_t::truckline, &lines);
}

slist_tpl<simline_t *> *
strassendepot_t::get_line_list()
{
	build_line_list();
	return &lines;
}

schiffdepot_t::schiffdepot_t(karte_t *welt, loadsave_t *file) : depot_t(welt)
{
    rdwr(file);
}


schiffdepot_t::schiffdepot_t(karte_t *welt, koord3d pos, spieler_t *sp, const haus_tile_besch_t *t) : depot_t(welt, pos, sp, t)
{

}



/*fahrplan_t *
schiffdepot_t::erzeuge_fahrplan()
{
    return  new schifffahrplan_t();
}
*/
fahrplan_t *
schiffdepot_t::erzeuge_fahrplan(fahrplan_t * fpl)
{
    return new schifffahrplan_t(fpl);
}

simline_t *
schiffdepot_t::create_line(karte_t * welt)
{
    return welt->simlinemgmt->create_line(simline_t::shipline);
}

const vehikel_besch_t *schiffdepot_t::get_vehicle_type(int itype)
{
    return vehikelbauer_t::gib_info(vehikel_besch_t::wasser, itype);
}


const char *
schiffdepot_t::gib_name() const
{
    return "Schiffdepot";
}

void
schiffdepot_t::build_line_list() {
	welt->simlinemgmt->build_line_list(simline_t::shipline, &lines);
}

slist_tpl<simline_t *> *
schiffdepot_t::get_line_list()
{
	build_line_list();
	return &lines;
}

//----------------- ende depots ----------------------
