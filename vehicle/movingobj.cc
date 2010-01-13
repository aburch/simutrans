/*
 * Copyright (c) 2008 by Markus Pristovsek
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#include <stdio.h>
#include <math.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../simimg.h"
#include "../player/simplay.h"
#include "../simtools.h"
#include "../simtypes.h"

#include "../boden/grund.h"

#include "../besch/groundobj_besch.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/tabfile.h"
#include "../dataobj/umgebung.h"

#include "../dings/baum.h"

#include "movingobj.h"

/******************************** static stuff for forest rules ****************************************************************/


/*
 * Diese Tabelle ermöglicht das Auffinden dient zur Auswahl eines Baumtypen
 */
vector_tpl<const groundobj_besch_t *> movingobj_t::movingobj_typen(0);

/*
 * Diese Tabelle ermöglicht das Auffinden einer Beschreibung durch ihren Namen
 */
stringhashtable_tpl<groundobj_besch_t *> movingobj_t::besch_names;


bool movingobj_t::alles_geladen()
{
	movingobj_typen.resize(besch_names.get_count());
	stringhashtable_iterator_tpl<groundobj_besch_t *>iter(besch_names);
	while(  iter.next()  ) {
		iter.access_current_value()->index = movingobj_typen.get_count();
		movingobj_typen.append( iter.get_current_value() );
	}

	if(besch_names.empty()) {
		movingobj_typen.append( NULL );
		DBG_MESSAGE("movingobj_t", "No movingobj found - feature disabled");
	}
	return true;
}



bool movingobj_t::register_besch(groundobj_besch_t *besch)
{
	// remove duplicates
	if(  besch_names.remove( besch->get_name() )  ) {
		dbg->warning( "movingobj_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
	}
	besch_names.put(besch->get_name(), besch );
	return true;
}




/* also checks for distribution values
 * @author prissi
 */
const groundobj_besch_t *movingobj_t::random_movingobj_for_climate(climate cl)
{
	// none there
	if(  besch_names.empty()  ) {
		return NULL;
	}

	int weight = 0;

	for( unsigned i=0;  i<movingobj_typen.get_count();  i++  ) {
		if(  movingobj_typen[i]->is_allowed_climate(cl)   ) {
			weight += movingobj_typen[i]->get_distribution_weight();
		}
	}

	// now weight their distribution
	if (weight > 0) {
		const int w=simrand(weight);
		weight = 0;
		for( unsigned i=0; i<movingobj_typen.get_count();  i++  ) {
			if(  movingobj_typen[i]->is_allowed_climate(cl) ) {
				weight += movingobj_typen[i]->get_distribution_weight();
				if(weight>=w) {
					return movingobj_typen[i];
				}
			}
		}
	}
	return NULL;
}



/******************************* end of static ******************************************/



// recalculates only the seasonal image
void movingobj_t::calc_bild()
{
	// alter/2048 is the age of the tree
	const groundobj_besch_t *besch=get_besch();
	const uint8 seasons = besch->get_seasons()-1;
	season=0;

	// two possibilities
	switch(seasons) {
				// summer only
		case 0: season = 0;
				break;
				// summer, snow
		case 1: season = welt->get_snowline()<=get_pos().z;
				break;
				// summer, winter, snow
		case 2: season = welt->get_snowline()<=get_pos().z ? 2 : welt->get_jahreszeit()==1;
				break;
		default: if(welt->get_snowline()<=get_pos().z) {
					season = seasons;
				}
				else {
					// resolution 1/8th month (0..95)
					const uint32 yearsteps = (welt->get_current_month()%12)*8 + ((welt->get_zeit_ms()>>(welt->ticks_bits_per_tag-3))&7) + 1;
					season = (seasons*yearsteps-1)/96;
				}
				break;
	}
	set_bild( get_besch()->get_bild( season, ribi_t::get_dir(get_fahrtrichtung()) )->get_nummer() );
}




movingobj_t::movingobj_t(karte_t *welt, loadsave_t *file) : vehikel_basis_t(welt)
{
	rdwr(file);
	if(get_besch()) {
		welt->sync_add( this );
	}
}



movingobj_t::movingobj_t(karte_t *welt, koord3d pos, const groundobj_besch_t *b ) : vehikel_basis_t(welt, pos)
{
	groundobjtype = movingobj_typen.index_of(b);
	season = 0xFF;	// mark dirty
	weg_next = 0;
	timetochange = 0;	// will do random direct change anyway during next step
	fahrtrichtung = calc_set_richtung( koord(0,0), koord::west );
	calc_bild();
	welt->sync_add( this );
}



bool movingobj_t::check_season(long /*month*/)
{
	if(season>1) {
		const uint8 old_season = season;
		calc_bild();
		if(season!=old_season) {
			mark_image_dirty( get_bild(), 0 );
		}
	}
	return true;
}



void movingobj_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "movingobj_t" );

	vehikel_basis_t::rdwr(file);

	file->rdwr_enum(fahrtrichtung, " ");
	if (file->is_loading()) {
		// restore dxdy information
		dx = dxdy[ ribi_t::get_dir(fahrtrichtung)*2];
		dy = dxdy[ ribi_t::get_dir(fahrtrichtung)*2+1];
	}

	file->rdwr_byte(steps, " ");
	file->rdwr_byte(steps_next, "\n");

	pos_next.rdwr(file);
	pos_next_next.rdwr(file);
	file->rdwr_short( timetochange, "" );

	if(file->is_saving()) {
		const char *s = get_besch()->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, 128);
		groundobj_besch_t *besch = besch_names.get(bname);
		if(  besch_names.empty()  ||  besch==NULL  ) {
			groundobjtype = simrand(movingobj_typen.get_count());
		}
		else {
			groundobjtype = besch->get_index();
		}
		// if not there, besch will be zero
		use_calc_height = true;
	}
	weg_next = 0;
}



/**
 * Öffnet ein neues Beobachtungsfenster für das Objekt.
 * @author Hj. Malthaner
 */
void movingobj_t::zeige_info()
{
	if(umgebung_t::tree_info) {
		ding_t::zeige_info();
	}
}



/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void movingobj_t::info(cbuffer_t & buf) const
{
	ding_t::info(buf);

	buf.append(translator::translate(get_besch()->get_name()));
	const char *maker=get_besch()->get_copyright();
	if(maker!=NULL  && maker[0]!=0) {
		buf.append("\n");
		buf.printf(translator::translate("Constructed by %s"), maker);
	}
	buf.append("\n");
	buf.append(translator::translate("cost for removal"));
	char buffer[128];
	money_to_string( buffer, get_besch()->get_preis()/100.0 );
	buf.append( buffer );
}



void movingobj_t::entferne(spieler_t *sp)
{
	spieler_t::accounting(sp, -get_besch()->get_preis(), get_pos().get_2d(), COST_CONSTRUCTION);
	mark_image_dirty( get_bild(), 0 );
	welt->sync_remove( this );
}




bool movingobj_t::sync_step(long delta_t)
{
	weg_next += get_besch()->get_speed() * delta_t;
	weg_next -= fahre_basis( weg_next );
	return true;
}



/* essential to find out about next step
 * returns true, if we can go here
 * (identical to fahrer)
 */
bool movingobj_t::ist_befahrbar( const grund_t *gr ) const
{
	if(gr==NULL) {
		// no ground => we cannot check further
		return false;
	}

	const groundobj_besch_t *besch = get_besch();
	if( !besch->is_allowed_climate( welt->get_climate(gr->get_hoehe()) ) ) {
		// not an allowed climate zone!
		return false;
	}

	if(besch->get_waytype()==road_wt) {
		// can cross roads
		if(gr->get_typ()!=grund_t::boden  ||  !hang_t::ist_wegbar(gr->get_grund_hang())) {
			return false;
		}
		if(gr->hat_wege()  &&  !gr->hat_weg(road_wt)) {
			return false;
		}
		if(!besch->can_built_trees_here()) {
			return gr->find<baum_t>()==NULL;
		}
	}
	else if(besch->get_waytype()==air_wt) {
		// avoid towns to avoid flying through houses
		return gr->get_typ()==grund_t::boden  ||  gr->get_typ()==grund_t::wasser;
	}
	else if(besch->get_waytype()==water_wt) {
		// floating object
		return gr->get_typ()==grund_t::wasser  ||  gr->hat_weg(water_wt);
	}
	else if(besch->get_waytype()==ignore_wt) {
		// crosses nothing
		if(!gr->ist_natur()  ||  !hang_t::ist_wegbar(gr->get_grund_hang())) {
			return false;
		}
		if(!besch->can_built_trees_here()) {
			return gr->find<baum_t>()==NULL;
		}
	}
	return true;
}



bool movingobj_t::hop_check()
{
	/* since we may be going diagonal without any road
	 * determining the next koord is a little tricky:
	 * If it is a diagonal, pos_next_next is calculated from current pos,
	 * Else pos_next_next is a single step from pos_next.
	 * otherwise objects would jump left/right on some diagonals
	 */
	koord k(fahrtrichtung);
	if(k.x&k.y) {
		pos_next_next = get_pos().get_2d()+k;
	}
	else {
		pos_next_next = pos_next.get_2d()+k;
	}

	if(timetochange==0  ||  !ist_befahrbar(welt->lookup_kartenboden(pos_next_next))) {
		// direction change needed
		timetochange = simrand(speed_to_kmh(get_besch()->get_speed())/3);
		const koord pos=pos_next.get_2d();
		const grund_t *to[4];
		uint8 until=0;
		// find all tiles we can go
		for(  int i=0;  i<4;  i++  ) {
			const grund_t *check = welt->lookup_kartenboden(pos+koord::nsow[i]);
			if(ist_befahrbar(check)  &&  check->get_pos()!=get_pos()) {
				to[until++] = check;
			}
		}
		// if nothing found, return
		if(until==0) {
			pos_next = get_pos();
			pos_next_next = get_pos().get_2d() - koord(fahrtrichtung);
			// (better would be destruction?)
		}
		else {
			// else prepare for direction change
			const grund_t *next = to[simrand(until)];
			pos_next_next = next->get_pos().get_2d();
		}
	}
	else {
		timetochange--;
	}
	// should be always true
	return true;
}



void movingobj_t::hop()
{
	verlasse_feld();

	if(pos_next.get_2d()==get_pos().get_2d()) {
		fahrtrichtung = ribi_t::rueckwaerts(fahrtrichtung);
		dx = -dx;
		dy = -dy;
		calc_bild();
	}
	else {
		ribi_t::ribi old_dir = fahrtrichtung;
		fahrtrichtung = calc_set_richtung( get_pos().get_2d(), pos_next_next );
		if(old_dir!=fahrtrichtung) {
			calc_bild();
		}
	}

	set_pos(pos_next);
	betrete_feld();
	// next position
	grund_t *gr_next = welt->lookup_kartenboden(pos_next_next);
	pos_next = gr_next ? gr_next->get_pos() : get_pos();
}



void *movingobj_t::operator new(size_t /*s*/)
{
	return freelist_t::gimme_node(sizeof(movingobj_t));
}



void movingobj_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(movingobj_t),p);
}
