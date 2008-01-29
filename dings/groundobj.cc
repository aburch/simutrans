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


#include "groundobj.h"

/******************************** static stuff for forest rules ****************************************************************/


/*
 * Diese Tabelle ermöglicht das Auffinden dient zur Auswahl eines Baumtypen
 */
vector_tpl<const groundobj_besch_t *> groundobj_t::groundobj_typen(0);

/*
 * Diese Tabelle ermöglicht das Auffinden einer Beschreibung durch ihren Namen
 */
stringhashtable_tpl<uint32> groundobj_t::besch_names;


bool groundobj_t::alles_geladen()
{
	if (besch_names.empty()) {
		DBG_MESSAGE("groundobj_t", "No groundobj found - feature disabled");
	}
	return true;
}



bool groundobj_t::register_besch(groundobj_besch_t *besch)
{
	if(groundobj_typen.get_count()==0) {
		// NULL for empty object
		groundobj_typen.append(NULL,4);
	}
	besch_names.put(besch->gib_name(), groundobj_typen.get_count() );
	groundobj_typen.append(besch,4);
	return true;
}




/* also checks for distribution values
 * @author prissi
 */
const groundobj_besch_t *groundobj_t::random_groundobj_for_climate(climate cl, hang_t::typ slope, uint16 max_speed  )
{
	int weight = 0;

	for( unsigned i=1;  i<groundobj_typen.get_count();  i++  ) {
		if(  groundobj_typen[i]->is_allowed_climate(cl)  &&  (slope==hang_t::flach  ||  groundobj_typen[i]->gib_phases()==16  ||  groundobj_typen[i]->get_speed()<max_speed)  ) {
			weight += groundobj_typen[i]->gib_distribution_weight();
		}
	}

	// now weight their distribution
	if (weight > 0) {
		const int w=simrand(weight);
		weight = 0;
		for( unsigned i=1; i<groundobj_typen.get_count();  i++  ) {
			if(  groundobj_typen[i]->is_allowed_climate(cl)  &&  (slope==hang_t::flach  ||  groundobj_typen[i]->gib_phases()==16  ||  groundobj_typen[i]->get_speed()<max_speed)  ) {
				weight += groundobj_typen[i]->gib_distribution_weight();
				if(weight>=w) {
					return groundobj_typen[i];
				}
			}
		}
	}
	return NULL;
}



/******************************* end of static ******************************************/



// recalculates only the seasonal image
void groundobj_t::calc_bild()
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
	// check for slopes?
	uint16 phase = 0;
	if(besch->gib_phases()==16) {
		phase = welt->lookup(gib_pos())->gib_grund_hang();
	}
	const bild_besch_t *bild_ptr = gib_besch()->gib_bild( season, phase );
	bild = bild_ptr ? bild_ptr->gib_nummer() : IMG_LEER;
}



groundobj_t::groundobj_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	rdwr(file);
	if(gib_besch()) {
		calc_bild();
	}
}



groundobj_t::groundobj_t(karte_t *welt, koord3d pos, const groundobj_besch_t *b ) : ding_t(welt, pos)
{
	season = 0xFF;	// mark dirty
	groundobjtype = groundobj_typen.index_of(b);
	calc_bild();
}



bool groundobj_t::check_season(long month)
{
	const uint8 old_season = season;
	calc_bild();
	if(season!=old_season) {
		mark_image_dirty( gib_bild(), 0 );
	}
	return true;
}



void groundobj_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);

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
void groundobj_t::zeige_info()
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
void groundobj_t::info(cbuffer_t & buf) const
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
groundobj_t::entferne(spieler_t *sp)
{
	spieler_t::accounting(sp, gib_besch()->gib_preis(), gib_pos().gib_2d(), COST_CONSTRUCTION);
	mark_image_dirty( gib_bild(), 0 );
}



void *
groundobj_t::operator new(size_t /*s*/)
{
	return freelist_t::gimme_node(sizeof(groundobj_t));
}



void
groundobj_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(groundobj_t),p);
}
