/*
 * wayobj.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 *
 * Things on ways like catenary or something like this
 *
 * von prissi
 */

#include "../boden/grund.h"
#include "../simworld.h"
#include "../simimg.h"
#include "../simdings.h"
#include "../simplay.h"

#include "../dataobj/ribi.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"

#include "../besch/way_obj_besch.h"
#include "wayobj.h"

#include "../gui/werkzeug_parameter_waehler.h"

#include "../tpl/stringhashtable_tpl.h"

#include "../utils/simstring.h"

// the descriptions ...
const way_obj_besch_t *default_oberleitung=NULL;
//wayobj_t::default_oberleitung=NULL;
slist_tpl<const way_obj_besch_t *> wayobj_t::liste;
stringhashtable_tpl<const way_obj_besch_t *> wayobj_t::table;



wayobj_t::wayobj_t(karte_t *welt, loadsave_t *file) : ding_t (welt)
{
	rdwr(file);
	step_frequency = 0;
	if(gib_besitzer()) {
		gib_besitzer()->add_maintenance(besch->gib_wartung());
	}
}



wayobj_t::wayobj_t(karte_t *welt, koord3d pos, spieler_t *besitzer, ribi_t::ribi dir, const way_obj_besch_t *besch) :  ding_t(welt, pos)
{
	this->besch = besch;
	this->dir = dir;
	step_frequency =  0;
	setze_besitzer(besitzer);
	if(gib_besitzer()) {
		gib_besitzer()->add_maintenance(besch->gib_wartung());
	}
}



wayobj_t::~wayobj_t()
{
	if(gib_besitzer()) {
		gib_besitzer()->add_maintenance(-besch->gib_wartung());
	}
	if(besch->gib_own_wtyp()==weg_t::overheadlines) {
		weg_t *weg = welt->lookup(gib_pos())->gib_weg((weg_t::typ)besch->gib_wtyp());
		if(weg) {
			// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
			weg->set_electrify(false);
			// restore old speed limit
			if(weg->gib_besch()->gib_topspeed()>besch->gib_topspeed()) {
				weg->setze_max_speed(weg->hat_gehweg()?50:weg->gib_besch()->gib_topspeed());
			}
		}
		else {
			dbg->warning("wayobj_t::~wayobj_t()","ground was not a way!");
		}
	}
}




void wayobj_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);
	if(file->get_version()>=89000) {
		file->rdwr_byte(dir, "\n");
		if(file->is_saving()) {
			const char *s = besch->gib_name();
			file->rdwr_str(s, "N");
		}

		if(file->is_loading()) {
			char bname[128];
			file->rd_str_into(bname, "N");

			besch = (const way_obj_besch_t *)wayobj_t::table.get(bname);
			if(!besch) {
				besch = (const way_obj_besch_t *)wayobj_t::table.get(translator::compatibility_name(bname));
			}
			if(!besch) {
				DBG_MESSAGE("wayobj_t::rwdr", "description %s for wayobj_t at %d,%d not found, will be removed!", bname, gib_pos().x, gib_pos().y);
			}
		}
	}
	else {
		besch = default_oberleitung;
		dir = 255;
	}
}



void
wayobj_t::entferne(spieler_t *sp)
{
	if(sp!=NULL) {
		sp->buche(-besch->gib_preis(), gib_pos().gib_2d(), COST_CONSTRUCTION);
	}
	if(besch->gib_own_wtyp()==weg_t::overheadlines) {
		grund_t *gr = welt->lookup(gib_pos());
		if(gr) {
			weg_t *w=gr->gib_weg((weg_t::typ)besch->gib_wtyp());
			if(w) {
				w->set_electrify(false);
			}
		}
	}
}



void
wayobj_t::laden_abschliessen()
{
	if(dir==255) {
		weg_t *w=welt->lookup(gib_pos())->gib_weg((weg_t::typ)besch->gib_wtyp());
		if(w) {
			dir = w->gib_ribi_unmasked();
		}
		else {
			dir = ribi_t::alle;
		}
	}
	// electrify a way if we are a catenary
	if(besch->gib_own_wtyp()==weg_t::overheadlines) {
		weg_t *weg = welt->lookup(gib_pos())->gib_weg((weg_t::typ)besch->gib_wtyp());
		if(weg) {
			// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
			weg->set_electrify(true);
			if(weg->gib_max_speed()>besch->gib_topspeed()) {
				weg->setze_max_speed(besch->gib_topspeed());
			}
		}
		else {
			dbg->warning("wayobj_t::laden_abschliessen()","ground was not a way!");
		}
	}
	calc_bild();
}



// helper function: gets the ribi on next tile
ribi_t::ribi
wayobj_t::find_next_ribi(const grund_t *start, const koord dir) const
{
	grund_t *to;
	ribi_t::ribi r1 = ribi_t::keine;
	if(start->get_neighbour(to, (weg_t::typ)besch->gib_wtyp(),dir)) {
		wayobj_t *wo=(wayobj_t *)to->suche_obj(ding_t::wayobj);
		if(wo) {
			r1 = wo->get_dir();
		}
	}
	return r1;
}



void
wayobj_t::calc_bild()
{
	grund_t *gr = welt->lookup(gib_pos());
	if(gr) {
		if(gr->ist_tunnel()) {
			setze_bild( 0, IMG_LEER );
			after = IMG_LEER;
			return;
		}

		weg_t *w=gr->gib_weg((weg_t::typ)besch->gib_wtyp());
		if(!w) {
			// well, we are not on a way anymore? => delete us
			gr->obj_remove(this,gib_besitzer());
			entferne(gib_besitzer());
			delete this;
			return;
		}

		setze_yoff( -gr->gib_weg_yoff() );
		dir &= w->gib_ribi_unmasked();

		// if there is a slope, we are finished, only four choices here (so far)
		hang_t::typ hang = gr->gib_weg_hang();
		if(hang!=hang_t::flach) {
			setze_bild( 0, besch->get_back_slope_image_id(hang) );
			after = besch->get_front_slope_image_id(hang);
			return;
		}

		// find out whether using diagonals or straight lines
		if(ribi_t::ist_kurve(dir)) {
			ribi_t::ribi r1 = ribi_t::keine, r2 = ribi_t::keine;

			bool diagonal = false;
			switch(dir) {
				case ribi_t::nordost:
					r1 = find_next_ribi( gr, koord::ost );
					r2 = find_next_ribi( gr, koord::nord );
					diagonal =
						(r1 == ribi_t::suedwest || r2 == ribi_t::suedwest) &&
						r1 != ribi_t::nordwest &&
						r2 != ribi_t::suedost;
				break;

				case ribi_t::suedwest:
					r1 = find_next_ribi( gr, koord::west );
					r2 = find_next_ribi( gr, koord::sued );
					diagonal =
						(r1 == ribi_t::nordost || r2 == ribi_t::nordost) &&
						r1 != ribi_t::suedost &&
						r2 != ribi_t::nordwest;
					break;

				case ribi_t::nordwest:
					r1 = find_next_ribi( gr, koord::west );
					r2 = find_next_ribi( gr, koord::nord );
					diagonal =
						(r1 == ribi_t::suedost || r2 == ribi_t::suedost) &&
						r1 != ribi_t::nordost &&
						r2 != ribi_t::suedwest;
				break;

				case ribi_t::suedost:
					r1 = find_next_ribi( gr, koord::ost );
					r2 = find_next_ribi( gr, koord::sued );
					diagonal =
						(r1 == ribi_t::nordwest || r2 == ribi_t::nordwest) &&
						r1 != ribi_t::suedwest &&
						r2 != ribi_t::nordost;
				break;
			}

			if(diagonal) {
				// with this, we avoid calling us endlessly
				// HACK (originally by hajo?)
				static int rekursion = 0;

				if(rekursion == 0) {
					grund_t *to;
					rekursion++;
					for(int r = 0; r < 4; r++) {
						if(gr->get_neighbour(to, (weg_t::typ)besch->gib_wtyp(), koord::nsow[r])) {
							ding_t *wo = to->suche_obj(ding_t::wayobj);
							if(wo) {
								wo->calc_bild();
							}
						}
					}
					rekursion--;
				}

				after = besch->get_front_diagonal_image_id(dir);
				image_id bild = besch->get_back_diagonal_image_id(dir);
				if(bild!=IMG_LEER  ||  after!=IMG_LEER) {
					// ok, we have diagonals
					setze_bild( 0, bild );
					return;
				}
			}
		}

		// ok, just use the normal lookup
		after = besch->get_front_image_id(dir);
		setze_bild( 0, besch->get_back_image_id(dir) );
	}
}



/******************** static stuff from here **********************/


/* better use this constrcutor for new wayobj; it will extend a matching obj or make an new one
 */
void
wayobj_t::extend_wayobj_t(karte_t *welt, koord3d pos, spieler_t *besitzer, ribi_t::ribi dir, const way_obj_besch_t *besch)
{
	grund_t *gr=welt->lookup(pos);
	if(gr) {
		if(gr->suche_obj(ding_t::wayobj)) {
			// since there might be more than one, we have to iterate through all of them
			for( uint8 i=0;  i<gr->gib_top();  i++  ) {
				ding_t *d=gr->obj_bei(i);
				if(d  &&  d->gib_typ()==ding_t::wayobj  &&  ((wayobj_t *)d)->gib_besch()->gib_wtyp()==besch->gib_wtyp()) {
					// extend this one instead
					((wayobj_t *)d)->set_dir(dir|((wayobj_t *)d)->get_dir());
					d->calc_bild();
					return;
				}
			}
		}
		// nothing found => make a new one
		wayobj_t *wo = new wayobj_t(welt,pos,besitzer,dir,besch);
		gr->insert_before_moving(wo);
		wo->laden_abschliessen();
		besitzer->buche( -besch->gib_preis(), pos.gib_2d(), COST_CONSTRUCTION);
	}
}




bool
wayobj_t::alles_geladen()
{
	if(wayobj_t::liste.count() == 0) {
		dbg->warning("wayobj_t::alles_geladen()", "No obj found - may crash when loading catenary.");
	}
	return true;
}



bool
wayobj_t::register_besch(way_obj_besch_t *besch)
{
	wayobj_t::table.put(besch->gib_name(), besch);
	wayobj_t::liste.append(besch);
	if(besch->gib_own_wtyp()==weg_t::overheadlines  &&  besch->gib_wtyp()==weg_t::schiene  &&
		(default_oberleitung==NULL  ||  default_oberleitung->gib_topspeed()<besch->gib_topspeed())) {
		default_oberleitung = besch;
	}
DBG_DEBUG( "wayobj_t::register_besch()","%s", besch->gib_name() );
	return true;
}




/**
 * Fill menu with icons of given stops from the list
 * @author Hj. Malthaner
 */
void wayobj_t::fill_menu(werkzeug_parameter_waehler_t *wzw,
	weg_t::typ wtyp,
	int (* werkzeug)(spieler_t *, karte_t *, koord, value_t),
	int sound_ok,
	int sound_ko,
	const uint16 time)
{
DBG_DEBUG("wayobj_t::fill_menu()","maximum %i",liste.count());
	for( unsigned i=0;  i<wayobj_t::liste.count();  i++  ) {
		char buf[128];
		const way_obj_besch_t *besch=wayobj_t::liste.at(i);

		if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {

DBG_DEBUG("wayobj_t::fill_menu()","try at pos %i to add %s(%p)",i,besch->gib_name(),besch);
			if(besch->gib_cursor()->gib_bild_nr(1)!=IMG_LEER  &&  wtyp==besch->gib_wtyp()) {
				// only add items with a cursor
DBG_DEBUG("wayobj_t::fill_menu()","at pos %i add %s",i,besch->gib_name());
				sprintf(buf, "%s, %ld$ (%ld$), %dkm/h",
					translator::translate(besch->gib_name()),
					besch->gib_preis()/-100l,
					(besch->gib_wartung()<<(karte_t::ticks_bits_per_tag-18))/100l,
					besch->gib_topspeed());

				wzw->add_param_tool(werkzeug,
				  (const void *)besch,
				  karte_t::Z_PLAN,
				  sound_ok,
				  sound_ko,
				  besch->gib_cursor()->gib_bild_nr(1),
				  besch->gib_cursor()->gib_bild_nr(0),
				  buf );
			}
		}
	}
}



const way_obj_besch_t*
wayobj_t::wayobj_search(weg_t::typ wt,weg_t::typ own,uint16 time)
{
	for( unsigned i=0;  i<wayobj_t::liste.count();  i++  ) {
		const way_obj_besch_t *besch=wayobj_t::liste.at(i);

		if((time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time))
			&&  besch->gib_wtyp()==wt  &&  besch->gib_own_wtyp()==own) {
				return besch;
		}
	}
	return NULL;
}
