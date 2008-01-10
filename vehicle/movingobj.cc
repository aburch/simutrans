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
#include "../dataobj/freelist.h"


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
	timetolife = simrand(100000);
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

	file->rdwr_long( timetolife, "" );

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
	if(timetolife<0) {
		// remove obj
  		return false;
	}
	timetolife -= delta_t;

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



void movingobj_t::hop()
{
	/* since we are going diagonal without any road
	 * determining the next koord is a little tricky:
	 * If it is a diagonal, pos_next_next is calculated from current pos,
	 * Else pos_next_next is a single step from pos_next.
	 * otherwise objects would jump left/right on some diagonals
	 */
	koord k(fahrtrichtung);
	if(k.x&k.y) {
		k += gib_pos().gib_2d();
	}
	else {
		k += pos_next.gib_2d();
	}

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

	// currently just handle end of map
	grund_t *from = welt->lookup_kartenboden(pos_next.gib_2d());
	if(from==NULL) {
		fahrtrichtung = ribi_t::rueckwaerts( fahrtrichtung );
		dx = -dx;
		dy = -dy;
		koord k( fahrtrichtung );
		if(k.x&k.y) {
			if(  (gib_pos().x+gib_pos().y)&1  ) {
				k.y = 0;
			}
			else {
				k.x = 0;
			}
		}
		from = welt->lookup_kartenboden(gib_pos().gib_2d()+k);
		pos_next = from ? from->gib_pos() : gib_pos()+k;
		calc_bild();
	}
	else {
		pos_next = koord3d( k, from->gib_hoehe() );
		verlasse_feld();
		setze_pos(from->gib_pos());
		betrete_feld();
	}
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
