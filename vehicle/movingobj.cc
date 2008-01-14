/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>
#include <math.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../simimg.h"
#include "../simplay.h"
#include "../simmem.h"
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
stringhashtable_tpl<uint32> movingobj_t::besch_names;


bool movingobj_t::alles_geladen()
{
	if (besch_names.empty()) {
		DBG_MESSAGE("movingobj_t", "No movingobj found - feature disabled");
	}
	return true;
}



bool movingobj_t::register_besch(groundobj_besch_t *besch)
{
	if(movingobj_typen.get_count()==0) {
		// NULL for empty object
		movingobj_typen.append(NULL,4);
	}
	besch_names.put(besch->gib_name(), movingobj_typen.get_count() );
	movingobj_typen.append(besch,4);
	return true;
}




/* also checks for distribution values
 * @author prissi
 */
const groundobj_besch_t *movingobj_t::random_movingobj_for_climate(climate cl)
{
	int weight = 0;

	for( unsigned i=1;  i<movingobj_typen.get_count();  i++  ) {
		if(  movingobj_typen[i]->is_allowed_climate(cl)   ) {
			weight += movingobj_typen[i]->gib_distribution_weight();
		}
	}

	// now weight their distribution
	if (weight > 0) {
		const int w=simrand(weight);
		weight = 0;
		for( unsigned i=1; i<movingobj_typen.get_count();  i++  ) {
			if(  movingobj_typen[i]->is_allowed_climate(cl) ) {
				weight += movingobj_typen[i]->gib_distribution_weight();
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
	const groundobj_besch_t *besch=gib_besch();
	const sint16 seasons = besch->gib_seasons()-1;
	season=0;

	// two possibilities
	switch(seasons) {
				// summer only
		case 0: season = 0;
				break;
				// summer, snow
		case 1: season = welt->get_snowline()<=gib_pos().z;
				break;
				// summer, winter, snow
		case 2: season = welt->get_snowline()<=gib_pos().z ? 2 : welt->gib_jahreszeit()==1;
				break;
		default: if(welt->get_snowline()<=gib_pos().z) {
					season = seasons;
				}
				else {
					// resolution 1/8th month (0..95)
					const uint32 yearsteps = ((welt->get_current_month()+11)%12)*8 + ((welt->gib_zeit_ms()>>(welt->ticks_bits_per_tag-3))&7) + 1;
					season = (seasons*yearsteps-1)/96;
				}
				break;
	}
	setze_bild( gib_besch()->gib_bild( season, ribi_t::gib_dir(gib_fahrtrichtung()) )->gib_nummer() );
}




movingobj_t::movingobj_t(karte_t *welt, loadsave_t *file) : vehikel_basis_t(welt)
{
	rdwr(file);
	if(gib_besch()) {
		calc_bild();
		welt->sync_add( this );
	}
}



movingobj_t::movingobj_t(karte_t *welt, koord3d pos, const groundobj_besch_t *b ) : vehikel_basis_t(welt, pos)
{
	static koord startdir[4] = { koord::sued, koord::ost, koord(1,1), koord(1,-1) };
	groundobjtype = movingobj_typen.index_of(b);
	season = 0xFF;	// mark dirty
	weg_next = 0;
	timetochange = simrand(speed_to_kmh(gib_besch()->get_speed())/3);
	sint8 dir = simrand(3);
	fahrtrichtung = calc_set_richtung( pos.gib_2d(), pos.gib_2d()+startdir[dir] );
	koord k( fahrtrichtung );
	if(  dir>1  ) {
		k = koord::ost;
	}
	pos_next = pos + k;
	calc_bild();
	welt->sync_add( this );
}



bool movingobj_t::check_season(long month)
{
	if(season>1) {
		const uint8 old_season = season;
		calc_bild();
		if(season!=old_season) {
			mark_image_dirty( gib_bild(), 0 );
		}
	}
	return true;
}



void movingobj_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);

	pos_next_next.rdwr(file);
	file->rdwr_short( timetochange, "" );

	if(file->is_saving()) {
		const char *s = gib_besch()->gib_name();
		file->rdwr_str(s, "N");
	}

	if(file->is_loading()) {
		char bname[128];
		file->rd_str_into(bname, "N");
		groundobjtype = besch_names.get(bname);
		// if not there, besch will be zero
	}
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

	buf.append("\n");
	buf.append(translator::translate(gib_besch()->gib_name()));
	const char *maker=gib_besch()->gib_copyright();
	if(maker!=NULL  && maker[0]!=0) {
		buf.append("\n");
		buf.printf(translator::translate("Constructed by %s"), maker);
	}
	buf.append("\n");
	buf.append(translator::translate("cost for removal"));
	char buffer[128];
	money_to_string( buffer, gib_besch()->gib_preis()/100.0 );
	buf.append( buffer );
}



void
movingobj_t::entferne(spieler_t *sp)
{
	sp->buche(gib_besch()->gib_preis(), gib_pos().gib_2d(), COST_CONSTRUCTION);
	mark_image_dirty( gib_bild(), 0 );
	welt->sync_remove( this );
}




bool movingobj_t::sync_step(long delta_t)
{
	weg_next += gib_besch()->get_speed() * delta_t;
	while(SPEED_STEP_WIDTH < weg_next) {
		weg_next -= SPEED_STEP_WIDTH;
		setze_yoff( gib_yoff() - hoff );
		fahre_basis();
		if(use_calc_height) {
			hoff = calc_height();
		}
		setze_yoff( gib_yoff() + hoff );
	}

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

	const groundobj_besch_t *besch = gib_besch();
	if( !besch->is_allowed_climate( welt->get_climate(gr->gib_hoehe()) ) ) {
		// not an allowed climate zone!
		return false;
	}

	if(besch->get_waytype()==road_wt) {
		// can cross roads
		if(gr->gib_typ()!=grund_t::boden) {
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
		return gr->gib_typ()==grund_t::boden  ||  gr->gib_typ()==grund_t::wasser;
	}
	else if(besch->get_waytype()==water_wt) {
		// floating object
		return gr->gib_typ()==grund_t::wasser  ||  gr->hat_weg(water_wt);
	}
	else if(besch->get_waytype()==ignore_wt) {
		// crosses nothing
		if(!gr->ist_natur()) {
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
		pos_next_next = gib_pos().gib_2d()+k;
	}
	else {
		pos_next_next = pos_next.gib_2d()+k;
	}

	if(timetochange==0  ||  !ist_befahrbar(welt->lookup_kartenboden(pos_next_next))) {
		// direction change needed
		timetochange = simrand(speed_to_kmh(gib_besch()->get_speed())/3);
		const koord pos=pos_next.gib_2d();
		const grund_t *to[4];
		uint8 until=0;
		// find all tiles we can go
		for(  int i=0;  i<4;  i++  ) {
			const grund_t *check = welt->lookup_kartenboden(pos+koord::nsow[i]);
			if(ist_befahrbar(check)  &&  check->gib_pos()!=gib_pos()) {
				to[until++] = check;
			}
		}
		// if nothing found, return
		if(until==0) {
			pos_next = gib_pos();
			pos_next_next = gib_pos().gib_2d() - koord(fahrtrichtung);
			// (better would be destruction?)
		}
		else {
			// else prepare for direction change
			const grund_t *next = to[simrand(until)];
			pos_next_next = next->gib_pos().gib_2d();
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
	const grund_t *next = welt->lookup_kartenboden(pos_next.gib_2d());
	assert(next);

	verlasse_feld();

	if(next->gib_pos()==gib_pos()) {
		fahrtrichtung = ribi_t::rueckwaerts(fahrtrichtung);
		dx = -dx;
		dy = -dy;
		calc_bild();
	}
	else {
		ribi_t::ribi old_dir = fahrtrichtung;
		fahrtrichtung = calc_set_richtung( gib_pos().gib_2d(), pos_next_next );
		if(old_dir!=fahrtrichtung) {
			calc_bild();
		}
	}

	setze_pos(next->gib_pos());
	betrete_feld();
	pos_next = koord3d(pos_next_next,0);
/*
	switch( fahrtrichtung ) {
		case ribi_t::nord:
			dx = 2;
			dy = -1;
			break;
		case ribi_t::sued:
			dx = -2;
			dy = 1;
			break;
		case ribi_t::west:
			dx = -2;
			dy = -1;
			break;
		case ribi_t::ost:
			dx = 2;
			dy = 1;
			break;
		case ribi_t::suedost:
			dx = 0;
			dy = 2;
			break;
		case ribi_t::nordwest:
			dx = 0;
			dy = -2;
			break;
		case ribi_t::nordost:
			dx = 4;
			dy = 0;
			break;
		case ribi_t::suedwest:
			dx = -4;
			dy = 0;
			break;
	}
*/
}



void *
movingobj_t::operator new(size_t /*s*/)
{
	return freelist_t::gimme_node(sizeof(movingobj_t));
}



void
movingobj_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(movingobj_t),p);
}
