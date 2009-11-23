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
#include "../player/simplay.h"
#include "../simwerkz.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/ribi.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"

#include "../besch/bruecke_besch.h"
#include "../besch/tunnel_besch.h"
#include "../besch/way_obj_besch.h"

#include "../gui/werkzeug_waehler.h"

#include "../boden/grund.h"
#include "../boden/wege/weg.h"

#include "../tpl/stringhashtable_tpl.h"

#include "../utils/simstring.h"

#include "bruecke.h"
#include "tunnel.h"
#include "wayobj.h"

// the descriptions ...
const way_obj_besch_t *wayobj_t::default_oberleitung=NULL;

vector_tpl<way_obj_besch_t *> wayobj_t::liste;
stringhashtable_tpl<way_obj_besch_t *> wayobj_t::table;


wayobj_t::wayobj_t(karte_t *welt, loadsave_t *file) : ding_t (welt)
{
	rdwr(file);
}



wayobj_t::wayobj_t(karte_t *welt, koord3d pos, spieler_t *besitzer, ribi_t::ribi d, const way_obj_besch_t *b) :  ding_t(welt, pos)
{
	besch = b;
	dir = d;
	set_besitzer(besitzer);
}



wayobj_t::~wayobj_t()
{
	if(!besch) {
		return;
	}
	spieler_t *sp = get_besitzer();
	if(sp) {
		sp->add_maintenance(-besch->get_wartung());
	}
	if(besch->get_own_wtyp()==overheadlines_wt) {
		grund_t *gr=welt->lookup(get_pos());
		weg_t *weg=NULL;
		if(gr) {
			const waytype_t wt = (besch->get_wtyp()==tram_wt) ? track_wt : besch->get_wtyp();
			weg = gr->get_weg(wt);
			if(weg) {
				// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
				weg->set_electrify(false);
				// restore old speed limit and way constraints
				weg->reset_way_constraints();
				uint32 max_speed = weg->hat_gehweg() ? 50 : weg->get_besch()->get_topspeed();
				if(gr->get_typ()==grund_t::tunnelboden) {
					tunnel_t *t = gr->find<tunnel_t>(1);
					if(t) {
						max_speed = t->get_besch()->get_topspeed();
						weg->add_way_constraints(t->get_besch()->get_way_constraints_permissive(), t->get_besch()->get_way_constraints_prohibitive());
					}
				}
				if(gr->get_typ()==grund_t::brueckenboden) {
					bruecke_t *b = gr->find<bruecke_t>(1);
					if(b) {
						max_speed = b->get_besch()->get_topspeed();
						weg->add_way_constraints(b->get_besch()->get_way_constraints_permissive(), b->get_besch()->get_way_constraints_prohibitive());
					}
				}
				weg->set_max_speed(max_speed);
			}
			else {
				dbg->warning("wayobj_t::~wayobj_t()","ground was not a way!");
			}
		}
	}

	mark_image_dirty( get_after_bild(), 0 );
	mark_image_dirty( get_bild(), 0 );
	grund_t *gr = welt->lookup( get_pos() );
	if( gr ) {
		for( uint8 i = 0; i < 4; i++ ) {
			// Remove ribis from adjacent wayobj.
			if( ribi_t::nsow[i] & get_dir() ) {
				grund_t *next_gr;
				if( gr->get_neighbour( next_gr, besch->get_wtyp(), ribi_t::nsow[i] ) ) {
					wayobj_t *wo2 = next_gr->get_wayobj( besch->get_wtyp() );
					if( wo2 ) {
						wo2->mark_image_dirty( wo2->get_after_bild(), 0 );
						wo2->mark_image_dirty( wo2->get_bild(), 0 );
						wo2->set_dir( wo2->get_dir() & ribi_t::get_forward(ribi_t::nsow[i]) );
						wo2->mark_image_dirty( wo2->get_after_bild(), 0 );
						wo2->mark_image_dirty( wo2->get_bild(), 0 );
						wo2->set_flag(ding_t::dirty);
					}
				}
			}
		}
	}
}




void wayobj_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "wayobj_t" );
	ding_t::rdwr(file);
	if(file->get_version()>=89000) {
		file->rdwr_byte(dir, "\n");
		if(file->is_saving()) {
			const char *s = besch->get_name();
			file->rdwr_str(s);
		}
		else {
			char bname[128];
			file->rdwr_str(bname, 128);

			besch = wayobj_t::table.get(bname);
			if(besch==NULL) {
				besch = wayobj_t::table.get(translator::compatibility_name(bname));
				if(besch==NULL) {
					if(strstr(bname,"atenary")  ||  strstr(bname,"electri")) {
						besch = default_oberleitung;
					}
				}
				if(besch==NULL) {
					dbg->warning("wayobj_t::rwdr", "description %s for wayobj_t at %d,%d not found, will be removed!", bname, get_pos().x, get_pos().y );
				}
				else {
					dbg->warning("wayobj_t::rwdr", "wayobj %s at %d,%d replaced by %s", bname, get_pos().x, get_pos().y, besch->get_name() );
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
		spieler_t::accounting(sp, -besch->get_preis(), get_pos().get_2d(), COST_CONSTRUCTION);
	}
}



void
wayobj_t::laden_abschliessen()
{
	// (re)set dir
	if(dir==255) {
		const waytype_t wt = (besch->get_wtyp()==tram_wt) ? track_wt : besch->get_wtyp();
		weg_t *w=welt->lookup(get_pos())->get_weg(wt);
		if(w) {
			dir = w->get_ribi_unmasked();
		}
		else {
			dir = ribi_t::alle;
		}
	}
	else {
		set_dir(dir);
	}

	const waytype_t wt = (besch->get_wtyp()==tram_wt) ? track_wt : besch->get_wtyp();
	weg_t *weg = welt->lookup(get_pos())->get_weg(wt);

	// electrify a way if we are a catenary
	if(besch->get_own_wtyp()==overheadlines_wt) {	
		if(weg) {
			// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
			weg->set_electrify(true);
			if(weg->get_max_speed()>besch->get_topspeed()) {
				weg->set_max_speed(besch->get_topspeed());
			}
		}
		else {
			dbg->warning("wayobj_t::laden_abschliessen()","ground was not a way!");
		}
	}

	//Add the way constraints together.
	weg->add_way_constraints(besch->get_way_constraints_permissive(), besch->get_way_constraints_prohibitive());

	spieler_t *sp = get_besitzer();
	if(sp) {
		sp->add_maintenance(besch->get_wartung());
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
		const wayobj_t* wo = to->get_wayobj( wt );
		if(wo) {
			r1 = wo->get_dir();
		}
	}
	return r1;
}



void
wayobj_t::calc_bild()
{
	grund_t *gr = welt->lookup(get_pos());
	diagonal = false;
	if(gr) {

		const waytype_t wt = (besch->get_wtyp()==tram_wt) ? track_wt : besch->get_wtyp();
		weg_t *w=gr->get_weg(wt);
		if(!w) {
			dbg->error("wayobj_t::calc_bild()","without way at (%s)", get_pos().get_str() );
			// well, we are not on a way anymore? => delete us
			gr->obj_remove(this);
			entferne(get_besitzer());
			delete this;
			gr->set_flag(grund_t::dirty);
			return;
		}

		set_yoff( -gr->get_weg_yoff() );
		dir &= w->get_ribi_unmasked();

		// if there is a slope, we are finished, only four choices here (so far)
		hang = gr->get_weg_hang();
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
							wayobj_t* wo = to->get_wayobj( wt );
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
void wayobj_t::extend_wayobj_t(karte_t *welt, koord3d pos, spieler_t *besitzer, ribi_t::ribi dir, const way_obj_besch_t *besch)
{
	grund_t *gr=welt->lookup(pos);
	if(gr) 
	{
		wayobj_t *existing_wayobj = gr->get_wayobj( besch->get_wtyp() );
		if( existing_wayobj ) {
			if(existing_wayobj->get_besch()->get_topspeed()<besch->get_topspeed()  &&  spieler_t::check_owner(besitzer, existing_wayobj->get_besitzer())) {
				// replace slower by faster
				dir = dir | existing_wayobj->get_dir();
				gr->obj_remove(existing_wayobj);
				gr->set_flag(grund_t::dirty);
				delete existing_wayobj;
			}
			else {
				// extend this one instead
				existing_wayobj->set_dir(dir|existing_wayobj->get_dir());
				existing_wayobj->mark_image_dirty( existing_wayobj->get_after_bild(), 0 );
				existing_wayobj->mark_image_dirty( existing_wayobj->get_bild(), 0 );
				existing_wayobj->set_flag(ding_t::dirty);
				return;
			}
		}

		// nothing found => make a new one
		wayobj_t *wo = new wayobj_t(welt,pos,besitzer,dir,besch);
		gr->obj_add(wo);
		wo->laden_abschliessen();
		wo->mark_image_dirty( wo->get_after_bild(), 0 );
		wo->set_flag(ding_t::dirty);
		spieler_t::accounting( besitzer,  -besch->get_preis(), pos.get_2d(), COST_CONSTRUCTION);

		for( uint8 i = 0; i < 4; i++ ) {
		// Extend wayobjects around the new one, that aren't already connected.
			if( ribi_t::nsow[i] & ~wo->get_dir() ) {
				grund_t *next_gr;
				if( gr->get_neighbour( next_gr, besch->get_wtyp(), ribi_t::nsow[i] ) ) {
					wayobj_t *wo2 = next_gr->get_wayobj( besch->get_wtyp() );
					if( wo2 ) {
						wo2->set_dir( wo2->get_dir() | ribi_t::rueckwaerts(ribi_t::nsow[i]) );
						wo2->mark_image_dirty( wo2->get_after_bild(), 0 );
						wo->set_dir( wo->get_dir() | ribi_t::nsow[i] );
						wo->mark_image_dirty( wo->get_after_bild(), 0 );
					}
				}
			}
		}
	}
}



// to sort wayobj for always the same menu order
static bool compare_wayobj_besch(const way_obj_besch_t* a, const way_obj_besch_t* b)
{
	int diff = a->get_wtyp() - b->get_wtyp();
	if (diff == 0) {
		diff = a->get_topspeed() - b->get_topspeed();
	}
	if (diff == 0) {
		/* Some speed: sort by name */
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}



bool wayobj_t::alles_geladen()
{
	if(table.empty()) {
		dbg->warning("wayobj_t::alles_geladen()", "No obj found - may crash when loading catenary.");
	}
	return true;
}



bool wayobj_t::register_besch(way_obj_besch_t *besch)
{
	// avoid duplicates with same name
	way_obj_besch_t *old_besch = table.get(besch->get_name());
	if(old_besch) {
		dbg->warning( "wayobj_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
		table.remove(besch->get_name());
		delete old_besch->get_builder();
		delete old_besch;
	}

	if(  besch->get_cursor()->get_bild_nr(1)!=IMG_LEER  ) {
		// only add images for wayobjexts with cursor ...
		wkz_wayobj_t *wkz = new wkz_wayobj_t();
		wkz->set_icon( besch->get_cursor()->get_bild_nr(1) );
		wkz->cursor = besch->get_cursor()->get_bild_nr(0);
		wkz->default_param = besch->get_name();
		wkz->id = werkzeug_t::general_tool.get_count()|GENERAL_TOOL;
		werkzeug_t::general_tool.append( wkz );
		besch->set_builder( wkz );
	}
	else {
		besch->set_builder( NULL );
	}

	table.put(besch->get_name(), besch);
	if(besch->get_own_wtyp()==overheadlines_wt  &&  besch->get_wtyp()==track_wt  &&
		(default_oberleitung==NULL  ||  default_oberleitung->get_topspeed()<besch->get_topspeed())) {
		default_oberleitung = besch;
	}
DBG_DEBUG( "wayobj_t::register_besch()","%s", besch->get_name() );
	return true;
}




/**
 * Fill menu with icons of given wayobjects from the list
 * @author Hj. Malthaner
 */
void wayobj_t::fill_menu(werkzeug_waehler_t *wzw, waytype_t wtyp, sint16 sound_ok, const karte_t *welt)
{
	const uint16 time=welt->get_timeline_year_month();

	stringhashtable_iterator_tpl<const way_obj_besch_t *>iter(table);
	vector_tpl<const way_obj_besch_t *>matching;

	while(  iter.next()  ) {
		const way_obj_besch_t* besch = iter.get_current_value();
		if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {

			DBG_DEBUG("wayobj_t::fill_menu()", "try to add %s(%p)", besch->get_name(), besch);
			if(besch->get_builder()  &&  wtyp==besch->get_wtyp()) {
				// only add items with a cursor
				matching.append(besch);
			}
		}
	}
	// sort the tools before adding to menu
	std::sort(matching.begin(), matching.end(), compare_wayobj_besch);
	for (vector_tpl<const way_obj_besch_t*>::const_iterator i = matching.begin(), end = matching.end(); i != end; ++i) {
		wzw->add_werkzeug( (*i)->get_builder() );
	}
}



const way_obj_besch_t *wayobj_t::wayobj_search(waytype_t wt,waytype_t own,uint16 time)
{
	stringhashtable_iterator_tpl<const way_obj_besch_t *>iter(table);
	while(  iter.next()  ) {
		const way_obj_besch_t* besch = iter.get_current_value();
		if((time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time))
			&&  besch->get_wtyp()==wt  &&  besch->get_own_wtyp()==own) {
				return besch;
		}
	}
	return NULL;
}


const way_obj_besch_t* wayobj_t::find_besch(const char *str)
{
	return wayobj_t::table.get(str);
}

