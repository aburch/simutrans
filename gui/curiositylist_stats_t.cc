/*
 * curiositylist_stats_t.cc
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "curiositylist_stats_t.h"

#include "../simgraph.h"
#include "../simtypes.h"
#include "../simcolor.h"
#include "../simworld.h"
#include "../simhalt.h"
#include "../simskin.h"

#include "../dings/gebaeude.h"

#include "../besch/haus_besch.h"
#include "../besch/skin_besch.h"

#include "../dataobj/translator.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "components/list_button.h"

curiositylist_stats_t::curiositylist_stats_t(karte_t * w,
					     const curiositylist::sort_mode_t& sortby,
					     const bool& sortreverse) :
    attractions(10)
{
	welt = w;
    get_unique_attractions(sortby,sortreverse);
    setze_groesse(koord(210, attractions.get_count()*(LINESPACE+1)-10));
}



void curiositylist_stats_t::get_unique_attractions(const curiositylist::sort_mode_t& sortby,
						   const bool& sortreverse)
{
    const weighted_vector_tpl<gebaeude_t *> &ausflugsziele = welt->gib_ausflugsziele();
    attractions.clear();
    attractions.resize(welt->gib_ausflugsziele().get_count());

    for (uint32 i=0; i<ausflugsziele.get_count(); ++i) {

	gebaeude_t *geb = ausflugsziele.at(i);
	// now check for paranoia, first tile on multitile buildings and real attraction
	if (geb==NULL  ||
	    geb->gib_tile()->gib_offset()!=koord(0,0)  ||
	    geb->gib_passagier_level()==0) {
	    continue;
	}

	bool append = true;
	for (unsigned int j=0; j<attractions.get_count(); ++j) {
	    if (sortby == curiositylist::by_name) {
		const char *token = translator::translate(geb->gib_tile()->gib_besch()->gib_name());
		const char *check_token = translator::translate(attractions.at(j)->gib_tile()->gib_besch()->gib_name());

		if (sortreverse)
		    append = STRICMP(token,check_token)<0;
		else
		    append = STRICMP(token,check_token)>=0;
	    }
	    else if (sortby == curiositylist::by_paxlevel) {
		const int paxlevel = geb->gib_passagier_level();
		const int check_paxlevel = attractions.at(j)->gib_passagier_level();

		if (sortreverse)
		    append = (paxlevel < check_paxlevel);
		else
		    append = (paxlevel >= check_paxlevel);
	    }

	    if (!append) {
//		DBG_MESSAGE("curiositylist_stats_t::get_unique_attractions()","insert %s at (%i,%i)",geb->gib_tile()->gib_besch()->gib_name(),geb->gib_pos().x, geb->gib_pos().y );
		attractions.insert_at(j,geb);
		break;
	  }
	}

	if (append) {
//	    DBG_MESSAGE("curiositylist_stats_t::get_unique_attractions()","append %s at (%i,%i)",geb->gib_tile()->gib_besch()->gib_name(),	geb->gib_pos().x, geb->gib_pos().y );
	    attractions.append(geb);
	}

    }
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void curiositylist_stats_t::infowin_event(const event_t * ev)
{
    const unsigned int line = (ev->cy) / (LINESPACE+1);

    if (line >= attractions.get_count())
	return;

    gebaeude_t * geb = attractions.at(line);
    if (!geb)
	return;

    if (IS_LEFTRELEASE(ev)) {
	geb->zeige_info();
    }
    else if (IS_RIGHTRELEASE(ev)) {
	const koord3d pos = geb->gib_pos();
	if(welt->ist_in_kartengrenzen(pos.gib_2d())) {
	    welt->setze_ij_off(pos.gib_2d() + koord(-5,-5));
	}
    }
} // end of function curiositylist_stats_t::infowin_event(const event_t * ev)



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void curiositylist_stats_t::zeichnen(koord offset)
{
	const struct clip_dimension cd = display_gib_clip_wh();
	const int start = cd.y-LINESPACE+1;
	const int end = cd.yy;

	static cbuffer_t buf(256);
	int yoff = offset.y;

	for (unsigned int i=0; i<attractions.get_count()  &&  yoff<end; i++) {
		const gebaeude_t *geb = attractions.get(i);

		int xoff = offset.x;

		// skip invisible lines
		if(yoff<start) {
			yoff += LINESPACE+1;
			continue;
		}

		buf.clear();

		// is connected? => decide on indicatorcolor
		int indicatorfarbe;
		bool post=false;
		bool pax=false;
		bool all_crowded=true;
		bool some_crowded=false;
		const minivec_tpl<halthandle_t> &halt_list = welt->access(geb->gib_pos().gib_2d())->get_haltlist();
		for(  unsigned h=0;  (post&pax)==0  &&  h<halt_list.get_count();  h++ ) {
			if(halt_list.get(h)->get_pax_enabled()) {
				pax = true;
				if(halt_list.get(h)->get_pax_unhappy()>40) {
					some_crowded |= true;
				}
				else {
					all_crowded = false;
				}
			}
			if(halt_list.get(h)->get_post_enabled()) {
				post = true;
				if(halt_list.get(h)->get_pax_unhappy()>40) {
					some_crowded |= true;
				}
				else {
					all_crowded = false;
				}
			}
		}
		// now decide on color
		if(some_crowded) {
			indicatorfarbe = all_crowded ? COL_RED : COL_ORANGE;
		}
		else if(pax) {
			indicatorfarbe = post ? 190 : COL_GREEN;
		}
		else {
			indicatorfarbe = post ? COL_BLUE : COL_YELLOW;
		}

		display_fillbox_wh_clip(xoff+7, yoff+2, INDICATOR_WIDTH, INDICATOR_HEIGHT, indicatorfarbe, true);

		// the other infos
		const unsigned char *name = (const unsigned char *)ltrim( translator::translate(geb->gib_tile()->gib_besch()->gib_name()) );
		char short_name[256];
		int i=0, cr=0;
		for( int j=0;  j<255  &&  name[j]>='\n'  &&  cr<2;  j++  ) {
			if(name[j]>0  &&  name[j]<=' ') {
				cr++;
				if(i>0  &&  short_name[i-1]!=' ') {
					short_name[i++] = ' ';
				}
			}
			else {
				short_name[i++] = name[j];
			}
		}
		short_name[i] = 0;
		// now we have a short name ...
		buf.append(short_name);
		buf.append(" (");
		buf.append(geb->gib_passagier_level());
		buf.append(") ");

		display_proportional_clip(xoff+INDICATOR_WIDTH+10+9,yoff,buf,ALIGN_LEFT,COL_BLACK,true);

		if (geb->gib_tile()->gib_besch()->gib_bauzeit() != 0)
		    display_color_img(skinverwaltung_t::intown->gib_bild_nr(0), xoff+INDICATOR_WIDTH+9, yoff, 0, false, false);

	    yoff +=LINESPACE+1;
	}
}
