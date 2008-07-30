/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Things on ways like catenary or something like this
 *
 * von prissi
 */

#include <algorithm>

#include "../boden/grund.h"
#include "../simworld.h"
#include "../simimg.h"
#include "../simdings.h"
#include "../simplay.h"

#include "../dataobj/ribi.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"

#include "../besch/bruecke_besch.h"
#include "../besch/tunnel_besch.h"
#include "../besch/way_obj_besch.h"

#include "../gui/werkzeug_waehler.h"

#include "../boden/grund.h"

#include "../tpl/stringhashtable_tpl.h"

#include "../utils/simstring.h"

#include "bruecke.h"
#include "tunnel.h"
#include "wayobj.h"

// the descriptions ...
const way_obj_besch_t *wayobj_t::default_oberleitung=NULL;

vector_tpl<const way_obj_besch_t *> wayobj_t::liste;
stringhashtable_tpl<const way_obj_besch_t *> wayobj_t::table;



wayobj_t::wayobj_t(karte_t *welt, loadsave_t *file) : ding_t (welt)
{
	rdwr(file);
}



wayobj_t::wayobj_t(karte_t *welt, koord3d pos, spieler_t *besitzer, ribi_t::ribi d, const way_obj_besch_t *b) :  ding_t(welt, pos)
{
	besch = b;
	dir = d;
	setze_besitzer(besitzer);
}



wayobj_t::~wayobj_t()
{
	if(!besch) {
		return;
	}
	if(gib_besitzer()) {
		spieler_t::add_maintenance(gib_besitzer(), -besch->gib_wartung());
	}
	if(besch->gib_own_wtyp()==overheadlines_wt) {
		grund_t *gr=welt->lookup(gib_pos());
		weg_t *weg=NULL;
		if(gr) {
			const waytype_t wt = (besch->gib_wtyp()==tram_wt) ? track_wt : besch->gib_wtyp();
			weg = gr->gib_weg(wt);
			if(weg) {
				// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
				weg->set_electrify(false);
				// restore old speed limit
				uint32 max_speed = weg->hat_gehweg() ? 50 : weg->gib_besch()->gib_topspeed();
				if(gr->gib_typ()==grund_t::tunnelboden) {
					tunnel_t *t = gr->find<tunnel_t>(1);
					if(t) {
						max_speed = t->gib_besch()->gib_topspeed();
					}
				}
				if(gr->gib_typ()==grund_t::brueckenboden) {
					bruecke_t *b = gr->find<bruecke_t>(1);
					if(b) {
						max_speed = b->gib_besch()->gib_topspeed();
					}
				}
				weg->setze_max_speed(max_speed);
			}
			else {
				dbg->warning("wayobj_t::~wayobj_t()","ground was not a way!");
			}
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

			besch = wayobj_t::table.get(bname);
			if(!besch) {
				besch = wayobj_t::table.get(translator::compatibility_name(bname));
				if(strstr(bname,"atenary")  ||  strstr(bname,"electri")) {
					besch = default_oberleitung;
				}
				if(!besch) {
					dbg->warning("wayobj_t::rwdr", "description %s for wayobj_t at %d,%d not found, will be removed!", bname, gib_pos().x, gib_pos().y );
				}
				else {
					dbg->warning("wayobj_t::rwdr", "wayobj %s at %d,%d rpleaced by %s", bname, gib_pos().x, gib_pos().y, besch->gib_name() );
				}
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
	if(besch) {
		spieler_t::accounting(sp, -besch->gib_preis(), gib_pos().gib_2d(), COST_CONSTRUCTION);
	}
}



void
wayobj_t::laden_abschliessen()
{
	// (re)set dir
	if(dir==255) {
		const waytype_t wt = (besch->gib_wtyp()==tram_wt) ? track_wt : besch->gib_wtyp();
		weg_t *w=welt->lookup(gib_pos())->gib_weg(wt);
		if(w) {
			dir = w->gib_ribi_unmasked();
		}
		else {
			dir = ribi_t::alle;
		}
	}
	else {
		set_dir(dir);
	}

	// electrify a way if we are a catenary
	if(besch->gib_own_wtyp()==overheadlines_wt) {
		const waytype_t wt = (besch->gib_wtyp()==tram_wt) ? track_wt : besch->gib_wtyp();
		weg_t *weg = welt->lookup(gib_pos())->gib_weg(wt);
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

	if(gib_besitzer()) {
		spieler_t::add_maintenance(gib_besitzer(), besch->gib_wartung());
	}

	calc_bild();
}



void wayobj_t::rotate90()
{
	ding_t::rotate90();
	dir = ribi_t::rotate90( dir);
	hang = hang_t::rotate90( hang );
}


// helper function: gets the ribi on next tile
ribi_t::ribi
wayobj_t::find_next_ribi(const grund_t *start, const koord dir, const waytype_t wt) const
{
	grund_t *to;
	ribi_t::ribi r1 = ribi_t::keine;
	if(start->get_neighbour(to,wt,dir)) {
		const wayobj_t* wo = to->find<wayobj_t>();
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
	diagonal = false;
	if(gr) {

		const waytype_t wt = (besch->gib_wtyp()==tram_wt) ? track_wt : besch->gib_wtyp();
		weg_t *w=gr->gib_weg(wt);
		if(!w) {
			dbg->error("wayobj_t::calc_bild()","without way at (%s)", gib_pos().gib_str() );
			// well, we are not on a way anymore? => delete us
			gr->obj_remove(this);
			entferne(gib_besitzer());
			delete this;
			gr->set_flag(grund_t::dirty);
			return;
		}

		setze_yoff( -gr->gib_weg_yoff() );
		dir &= w->gib_ribi_unmasked();

		// if there is a slope, we are finished, only four choices here (so far)
		hang = gr->gib_weg_hang();
		if(hang!=hang_t::flach) {
			return;
		}

		// find out whether using diagonals or straight lines
		if(ribi_t::ist_kurve(dir)  &&  besch->has_diagonal_bild()) {
			ribi_t::ribi r1 = ribi_t::keine, r2 = ribi_t::keine;

			switch(dir) {
				case ribi_t::nordost:
					r1 = find_next_ribi( gr, koord::ost, wt );
					r2 = find_next_ribi( gr, koord::nord, wt );
					diagonal =
						(r1 == ribi_t::suedwest || r2 == ribi_t::suedwest) &&
						r1 != ribi_t::nordwest &&
						r2 != ribi_t::suedost;
				break;

				case ribi_t::suedwest:
					r1 = find_next_ribi( gr, koord::west, wt );
					r2 = find_next_ribi( gr, koord::sued, wt );
					diagonal =
						(r1 == ribi_t::nordost || r2 == ribi_t::nordost) &&
						r1 != ribi_t::suedost &&
						r2 != ribi_t::nordwest;
					break;

				case ribi_t::nordwest:
					r1 = find_next_ribi( gr, koord::west, wt );
					r2 = find_next_ribi( gr, koord::nord, wt );
					diagonal =
						(r1 == ribi_t::suedost || r2 == ribi_t::suedost) &&
						r1 != ribi_t::nordost &&
						r2 != ribi_t::suedwest;
				break;

				case ribi_t::suedost:
					r1 = find_next_ribi( gr, koord::ost, wt );
					r2 = find_next_ribi( gr, koord::sued, wt );
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
						if(gr->get_neighbour(to, wt, koord::nsow[r])) {
							wayobj_t* wo = to->find<wayobj_t>();
							if(wo) {
								wo->calc_bild();
							}
						}
					}
					rekursion--;
				}

				image_id after = besch->get_front_diagonal_image_id(dir);
				image_id bild = besch->get_back_diagonal_image_id(dir);
				if(bild==IMG_LEER  &&  after==IMG_LEER) {
					// ok, we have diagonals
					diagonal = false;
				}
			}
		}
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
		if (gr->find<wayobj_t>()) {
			// since there might be more than one, we have to iterate through all of them
			for( uint8 i=0;  i<gr->gib_top();  i++  ) {
				ding_t *d=gr->obj_bei(i);
				if(d  &&  d->gib_typ()==ding_t::wayobj  &&  ((wayobj_t *)d)->gib_besch()->gib_wtyp()==besch->gib_wtyp()) {
					if(((wayobj_t *)d)->gib_besch()->gib_topspeed()<besch->gib_topspeed()  &&  spieler_t::check_owner(besitzer, d->gib_besitzer())) {
						// replace slower by faster
						gr->obj_remove(d);
						gr->set_flag(grund_t::dirty);
						delete d;
						break;
					}
					else {
						// extend this one instead
						((wayobj_t *)d)->set_dir(dir|((wayobj_t *)d)->get_dir());
						d->calc_bild();
						d->mark_image_dirty( d->gib_after_bild(), 0 );
						d->set_flag(ding_t::dirty);
						return;
					}
				}
			}
		}
		// nothing found => make a new one
		wayobj_t *wo = new wayobj_t(welt,pos,besitzer,dir,besch);
		gr->obj_add(wo);
		wo->laden_abschliessen();
		wo->mark_image_dirty( wo->gib_after_bild(), 0 );
		wo->set_flag(ding_t::dirty);
		spieler_t::accounting( besitzer,  -besch->gib_preis(), pos.gib_2d(), COST_CONSTRUCTION);
	}
}



// to sort wayobj for always the same menu order
static bool compare_wayobj_besch(const way_obj_besch_t* a, const way_obj_besch_t* b)
{
	int diff = a->gib_wtyp() - b->gib_wtyp();
	if (diff == 0) {
		diff = a->gib_topspeed() - b->gib_topspeed();
	}
	if (diff == 0) {
		/* Some speed: sort by name */
		diff = strcmp(a->gib_name(), b->gib_name());
	}
	return diff < 0;
}



bool wayobj_t::alles_geladen()
{
	if (wayobj_t::liste.empty()) {
		dbg->warning("wayobj_t::alles_geladen()", "No obj found - may crash when loading catenary.");
	}
	else {
		std::sort(liste.begin(), liste.end(), compare_wayobj_besch);
	}
	return true;
}



bool
wayobj_t::register_besch(way_obj_besch_t *besch)
{
	table.put(besch->gib_name(), besch);
	liste.push_back(besch);
	if(besch->gib_own_wtyp()==overheadlines_wt  &&  besch->gib_wtyp()==track_wt  &&
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
void wayobj_t::fill_menu(werkzeug_waehler_t *wzw, waytype_t wtyp, const karte_t *welt)
{
	static stringhashtable_tpl<wkz_wayobj_t *> wayobj_tool;

	const uint16 time=welt->get_timeline_year_month();
DBG_DEBUG("wayobj_t::fill_menu()","maximum %i",liste.get_count());
	for (vector_tpl<const way_obj_besch_t*>::const_iterator iter = liste.begin(), end = liste.end();  iter != end;  ++iter  ) {
		const way_obj_besch_t* besch = (*iter);
		if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {

			DBG_DEBUG("wayobj_t::fill_menu()", "try to add %s(%p)", besch->gib_name(), besch);
			if(besch->gib_cursor()->gib_bild_nr(1)!=IMG_LEER  &&  wtyp==besch->gib_wtyp()) {
				// only add items with a cursor
				wkz_wayobj_t *wkz = wayobj_tool.get(besch->gib_name());
				if(wkz==NULL) {
					// not yet in hashtable
					wkz = new wkz_wayobj_t();
					wkz->set_icon( besch->gib_cursor()->gib_bild_nr(1) );
					wkz->cursor = besch->gib_cursor()->gib_bild_nr(0);
					wkz->default_param = besch->gib_name();
					wayobj_tool.put(besch->gib_name(),wkz);
				}
				wzw->add_werkzeug( (werkzeug_t*)wkz );
			}
		}
	}
}



const way_obj_besch_t*
wayobj_t::wayobj_search(waytype_t wt,waytype_t own,uint16 time)
{
	for (vector_tpl<const way_obj_besch_t*>::const_iterator i = liste.begin(), end = liste.end();  i != end;  ++i  ) {
		const way_obj_besch_t* besch = (*i);
		if((time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time))
			&&  besch->gib_wtyp()==wt  &&  besch->gib_own_wtyp()==own) {
				return besch;
		}
	}
	return NULL;
}


const way_obj_besch_t* wayobj_t::find_besch(const char *str)
{
	return wayobj_t::table.get(str);
}

